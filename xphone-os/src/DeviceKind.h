#pragma once
#ifndef FREEINK_DEVICE_X4PRO
#define FREEINK_DEVICE_X4PRO 0
#endif

// Which Xteink this binary woke up on. Set ONCE at the top of boot() from
// freeink::selectXteinkDevice() (I2C fingerprint of the X3-only gauge/RTC/IMU)
// before any scene, service, or SD/display bring-up runs. false = X4, the
// SDK's conservative pre-detection default. One universal update.bin serves
// both devices — a compile-time panel choice bricked testers who SD-flashed
// the other device's image (both zips ship a file named update.bin).
extern bool gDeviceIsX3;

inline const char* deviceName() {
#if FREEINK_DEVICE_X4PRO
  return "X4 Pro";
#else
  return gDeviceIsX3 ? "X3" : "X4";
#endif
}
