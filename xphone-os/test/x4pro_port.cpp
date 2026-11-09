#include "../src/FirmwareTarget.h"
#include "../src/TouchNavigation.h"
#include <cassert>
#include <cstdio>
int main() {
  uint8_t image[24]{}; image[0] = 0xE9;
  image[12] = 9;  // ESP32-S3 image header
  assert(firmware_target::matches(image, 24, 9));
  assert(!firmware_target::matches(image, 24, 5));
  assert(!firmware_target::matches(image, 23, 9));
  image[12] = 5;  // ESP32-C3
  assert(firmware_target::matches(image, 24, 5));
  assert(!firmware_target::matches(image, 24, 9));
  image[0] = 0;
  assert(!firmware_target::matches(image, 24, 5));
  using namespace touch_navigation;
  assert(swipe(.5f,.5f,.2f,.5f) == Direction::Up);
  assert(swipe(.5f,.5f,.8f,.5f) == Direction::Down);
  assert(swipe(.5f,.5f,.5f,.8f) == Direction::Left);
  assert(swipe(.5f,.5f,.5f,.2f) == Direction::Right);
  puts("X4 Pro image-target and touch-navigation tests passed");
}
