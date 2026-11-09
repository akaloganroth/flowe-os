#pragma once
#include <Arduino.h>
namespace X4Pro {
#if FREEINK_DEVICE_X4PRO
void begin();
void nap(bool sleeping);
void cycleBrightness();
void cycleWarmth();
unsigned brightness();
unsigned warmth();
[[noreturn]] void deepSleep();
#else
inline void begin() {}
inline void nap(bool) {}
#endif
}
