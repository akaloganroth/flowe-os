# TRMNL functional sleep

This fork preserves Flowe's apps, Bluetooth pairing, nap/off policy, and wake
restoration. With TRMNL configured, both sleep modes show your TRMNL playlist.
Without configuration, the original Flowe sleep screen remains available.

## Setup

1. Provision Wi-Fi through the Flowe companion app as usual. This integration
   uses the first saved network (2.4 GHz).
2. Copy `trmnl.example.json` to the SD card root as `trmnl.json`. Replace
   `YOUR_DEVICE_API_KEY` with the **device** API key from your TRMNL device
   settings (Developer Edition or BYOD), not an account API key.
3. Copy `trmnl-ca.pem` alongside it. HTTPS verifies the server certificate and
   synchronizes the clock using NTP. The included trust roots cover Google
   Trust Services R1–R4, Amazon Root CA 1, and ISRG Root X1; append your own
   server's CA certificate if needed (maximum combined size 16 KiB).
4. Configure TRMNL to deliver an uncompressed **1-bit BMP** screen (the OG
   800×480 format). PNG, JPEG, compressed BMP, and grayscale BMP are not
   supported. Images up to 1600×1600 are fitted without cropping; landscape
   images display in landscape on the X3/X4 panel.
5. Build from `xphone-os` with `pio run -e x3`. Copy
   `.pio/build/x3/firmware.bin` to your SD card as `update.bin`, then use the
   normal Flowe SD update procedure. The upstream x3 target is a universal
   X3/X4 image. This integration has not been validated on physical hardware.
6. Enter sleep with the power button. The first download can take several
   seconds. A failed fetch keeps the last validated SD image; if none exists,
   Flowe draws its original sleep screen. Serial logs use the `[trmnl]` prefix.

Do not commit your configured `trmnl.json`; it contains a secret. The SD card
holds that key in plaintext, just like a removable configuration file.

## Refresh and power behavior

The client calls `GET /api/display` with `Access-Token` and the device's MAC in
`ID`, then downloads `image_url`. Requests advance the TRMNL playlist. Polling
uses `refresh_rate` in seconds, clamped to 60 seconds–24 hours (default five
minutes). Wi-Fi is switched off after each attempt. The image is streamed to
SD and validated before replacing the cache, avoiding an extra framebuffer.

Refreshes occur on sleep entry when due, and periodically during a nap. Full
power-off preserves the last image without timer wakeups, matching Flowe's
original power behavior. Set Settings → Sleep → Off to Never if you want the
nap playlist to keep updating. Wi-Fi and TLS increase battery use. Downloads
run synchronously with bounded network timeouts, so a power-button action
may be delayed while a request completes. BLE remains enabled; TLS memory
pressure or an unavailable network can cause a fetch to fall back to cache.

## Moving to your own server

Change `server_url` to your server's base URL and `api_key` to its device key.
No firmware rebuild is needed. HTTPS requires the matching CA in
`trmnl-ca.pem`; HTTP is also supported for a trusted local network. The server
must implement `/api/display` and return an absolute BMP `image_url`, with
Content-Length on both API and image responses. Redirects are not followed.
An HTTPS API must also return an HTTPS image URL. Firmware update/reset
instructions from TRMNL are ignored; Flowe retains its existing updater.

Set `enabled` to false or remove `trmnl.json` to return to the original screen
(configuration is re-read on the next due refresh; restart for immediate use).

## Verification

```sh
c++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
  xphone-os/test/trmnl_bitmap.cpp -o /tmp/trmnl-bitmap-test
/tmp/trmnl-bitmap-test
```

On-device checks still needed: first download, nap refresh, wake after a
refresh, full off/on restore, unavailable Wi-Fi, invalid API key, corrupted
image, and BLE reconnection with both X3 and X4.

Protocol reference: https://docs.trmnl.com/go/private-api/introduction

Root certificate sources:
- https://pki.goog/roots.pem
- https://www.amazontrust.com/repository/AmazonRootCA1.pem
- https://letsencrypt.org/certs/isrgrootx1.pem
