#include "TrmnlSleep.h"
#include "TrmnlBitmap.h"
#include "WifiCreds.h"
#include "Gfx.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <SDCardManager.h>
#include <algorithm>
#include <time.h>

namespace {
constexpr const char* cache = "/trmnl-screen.bmp";
constexpr const char* pending = "/trmnl-screen.tmp";
uint32_t lastAttempt = 0, interval = 300000;
bool configured = false, attempted = false;
using trmnl::Bitmap;
bool inspect(FsFile& f, Bitmap& b) {
  uint8_t header[62];
  return f.seekSet(0) && f.read(header, sizeof(header)) == sizeof(header) &&
         trmnl::parse(header, sizeof(header), f.size(), b);
}
String readText(const char* path, size_t maxSize) {
  FsFile f = SdMan.open(path, O_RDONLY);
  if (!f || f.size() > maxSize) return {};
  String s;
  if (!s.reserve(f.size() + 1)) return {};
  while (f.available()) s += char(f.read());
  return s;
}
bool beginHttp(HTTPClient& http, WiFiClient& plain, WiFiClientSecure& tls,
               const String& url, const String& ca) {
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  tls.setHandshakeTimeout(5);
  http.useHTTP10(true); // bounded, non-chunked downloads
  if (url.startsWith("https://")) {
    if (ca.isEmpty()) return false;
    tls.setCACert(ca.c_str());
    return http.begin(tls, url);
  }
  return url.startsWith("http://") && http.begin(plain, url);
}
bool fetch(const String& base, const String& key, const String& ca) {
  WiFiClient plain;
  WiFiClientSecure tls;
  HTTPClient http;
  if (!beginHttp(http, plain, tls, base + "/api/display", ca)) return false;
  http.addHeader("Access-Token", key);
  http.addHeader("ID", WiFi.macAddress());
  // Authentication is only sent to the configured API, never the image host.
  if (http.GET() != 200 || http.getSize() < 0 || http.getSize() > 8192) return false;
  JsonDocument doc;
  if (deserializeJson(doc, http.getString())) return false;
  String url = doc["image_url"] | "";
  const uint32_t seconds = doc["refresh_rate"] | 300U;
  interval = std::max<uint32_t>(60, std::min<uint32_t>(86400, seconds)) * 1000U;
  http.end();
  // Never downgrade an HTTPS API's image request to plaintext.
  if (base.startsWith("https://") && !url.startsWith("https://")) return false;
  HTTPClient image;
  if (!beginHttp(image, plain, tls, url, ca) || image.GET() != 200) return false;
  const int length = image.getSize();
  if (length < 62 || length > 400000) return false;
  FsFile out = SdMan.open(pending, O_CREAT | O_TRUNC | O_WRONLY);
  if (!out) return false;
  auto* stream = image.getStreamPtr();
  uint8_t buf[512];
  int remaining = length;
  const uint32_t started = millis();
  while (remaining > 0 && millis() - started < 20000) {
    const size_t n = stream->readBytes(buf, std::min(size_t(remaining), sizeof(buf)));
    if (!n || out.write(buf, n) != n) break;
    remaining -= n;
    delay(1);
  }
  out.close();
  image.end();
  FsFile check = SdMan.open(pending, O_RDONLY);
  Bitmap b;
  const bool valid = remaining == 0 && check && inspect(check, b);
  check.close();
  if (!valid) { SdMan.remove(pending); return false; }
  // Keep a previous validated image if promotion fails.
  if (!SdMan.exists(cache) && SdMan.exists("/trmnl-screen.old") &&
      !SdMan.rename("/trmnl-screen.old", cache)) return false;
  SdMan.remove("/trmnl-screen.old");
  const bool hadCache = SdMan.exists(cache);
  if (hadCache && !SdMan.rename(cache, "/trmnl-screen.old")) return false;
  if (!SdMan.rename(pending, cache)) {
    if (hadCache) SdMan.rename("/trmnl-screen.old", cache);
    return false;
  }
  SdMan.remove("/trmnl-screen.old");
  return true;
}
}
namespace TrmnlSleep {
bool due() { return configured && millis() - lastAttempt >= interval; }
void update() {
  if (attempted && millis() - lastAttempt < interval) return;
  attempted = true;
  lastAttempt = millis();
  if (!SdMan.ready() && !SdMan.begin()) return;
  JsonDocument config;
  const String json = readText("/trmnl.json", 4096);
  configured = !deserializeJson(config, json) && config["enabled"].as<bool>();
  if (!configured) return;
  String base = config["server_url"] | "https://trmnl.com";
  while (base.endsWith("/")) base.remove(base.length() - 1);
  const String key = config["api_key"] | "";
  const String ca = readText("/trmnl-ca.pem", 16384);
  if (key.isEmpty()) return;
  // This path must not take ownership of another Wi-Fi session.
  if (WiFi.getMode() != WIFI_OFF) return;
  WifiCreds::Network network{};
  if (!WifiCreds::get(0, &network)) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(network.ssid, network.pass);
  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 10000) delay(25);
  bool ok = false;
  if (WiFi.status() == WL_CONNECTED) {
    // Certificate validity requires a real clock after a power-on reset.
    if (base.startsWith("https://") && time(nullptr) < 1700000000) {
      configTime(0, 0, "pool.ntp.org", "time.google.com");
      const uint32_t clockStart = millis();
      while (time(nullptr) < 1700000000 && millis() - clockStart < 5000) delay(25);
    }
    if (!base.startsWith("https://") || time(nullptr) >= 1700000000) ok = fetch(base, key, ca);
  }
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.printf("[trmnl] %s\n", ok ? "sleep screen updated" : "refresh failed; using cached screen");
}
bool draw(Gfx& gfx) {
  if (!configured) return false;
  FsFile f = SdMan.open(cache, O_RDONLY);
  if (!f) f = SdMan.open("/trmnl-screen.old", O_RDONLY);
  Bitmap b;
  if (!f || !inspect(f, b)) return false;
  // Landscape TRMNL screens use the panel's native landscape orientation.
  // Restore the caller's orientation so waking apps retain their layout.
  const auto previous = gfx.orientation();
  gfx.setOrientation(b.width > b.height ? Gfx::Orient::Landscape : Gfx::Orient::Portrait);
  int w = gfx.width(), h = b.height * w / b.width;
  if (h > gfx.height()) { h = gfx.height(); w = b.width * h / b.height; }
  const int ox = (gfx.width() - w) / 2, oy = (gfx.height() - h) / 2;
  uint8_t row[200];
  gfx.clear();
  for (int y = 0; y < h; ++y) {
    const int sy = y * b.height / h;
    if (!f.seekSet(b.offset + (b.top ? sy : b.height - 1 - sy) * b.stride) ||
        f.read(row, b.stride) != int(b.stride)) { gfx.setOrientation(previous); return false; }
    for (int x = 0; x < w; ++x) {
      const int sx = x * b.width / w;
      const bool one = row[sx / 8] & (0x80 >> (sx % 8));
      gfx.drawPixel(ox + x, oy + y, one != b.zeroBlack);
    }
  }
  gfx.setOrientation(previous);
  return true;
}
}
