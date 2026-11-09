#include <BoardConfig.h>
#include "X4Pro.h"
#if FREEINK_DEVICE_X4PRO
#include <FrontlightManager.h>
#include <PowerManager.h>
#include <Preferences.h>
#include <esp_system.h>
#include <driver/gpio.h>
namespace {
FrontlightManager light;
unsigned level = 20, warm = 50;
void save() {
  Preferences p;
  if (p.begin("x4pro", false)) { p.putUInt("light", level); p.putUInt("warm", warm); }
}
}
namespace X4Pro {
void begin() {
  BoardConfig::selectDevice(BoardConfig::Board::XteinkX4Pro);
  BoardConfig::holdPowerRails();
  Preferences p;
  if (p.begin("x4pro", true)) {
    level = min<uint32_t>(100, p.getUInt("light", 20));
    warm = min<uint32_t>(100, p.getUInt("warm", 50));
  }
  gpio_hold_dis(GPIO_NUM_8);
  gpio_hold_dis(GPIO_NUM_9);
  light.begin();
  light.setColorTemperature(warm);
  light.setBrightness(level);
}
void nap(bool sleeping) { light.setBrightness(sleeping ? 0 : level); }
void cycleBrightness() { level = (level + 10) % 110; light.setBrightness(level); save(); }
void cycleWarmth() { warm = (warm + 25) % 125; light.setColorTemperature(warm); save(); }
unsigned brightness() { return level; }
unsigned warmth() { return warm; }
[[noreturn]] void deepSleep() {
  light.off();
  for (const int pin : {8, 9}) {
    ledcDetach(pin);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    gpio_hold_en(static_cast<gpio_num_t>(pin));
  }
  freeink::PowerManager::powerDownRailsForSleep();
  if (!freeink::PowerManager::armPowerButtonWakeup()) esp_restart();
  freeink::PowerManager::deepSleep();
}
}
#endif
