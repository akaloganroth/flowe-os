# Flowe + TRMNL for Xteink X4 Pro (experimental)

This is a separate **ESP32-S3** build. X3 and original X4 remain ESP32-C3 builds.
The Pro port is compile-tested, not yet verified on a physical device. Keep a
complete backup of your Pro before first installation.

## What the port includes

- X4 Pro board profile, 16 MB flash and 8 MB OPI PSRAM configuration.
- SSD1677 / UC8179 display-controller detection using the Pro pin map.
- Native 1-bit SDMMC, GT911 touch/Home key, CW2017 battery gauge.
- Flowe's original apps and BLE companion protocol, with gesture navigation.
- Settings → Frontlight (10% steps) and Warmth (25% steps), saved across boots.
  The light turns off during naps and deep sleep and returns on wake.
- S3 power-button wake via the SDK's ext1 path; the C3 GPIO13 latch routine is
  never run on the Pro, where GPIO13 is display chip-select.
- Hosted/self-hosted TRMNL sleep screens, using the same [TRMNL setup](../trmnl/README.md).
- SD updater rejects images for another ESP chip before erasing the OTA slot.

The newer SDK is vendored separately in `freeink-sdk-x4pro` to keep the existing
X3/X4 drivers unchanged. Hardware sources: [FreeInk SDK, pinned revision](https://github.com/Free-Ink/freeink-sdk/tree/48f5949730157f335a8777959a85f13fdefd6af9)
and [eenk's X4 Pro implementation](https://github.com/t0mg/eenk/tree/main/src/hal/esp32/x4pro).
The pinned profile detects the SSD1677/UC8179 family; other panel revisions are
not claimed as supported by this build.

## Controls

Flowe's UI still uses a highlighted selection; touch does not directly select
the item under your finger.

| Action | X4 Pro control |
| --- | --- |
| Up / Down | Physical navigation buttons, or vertical swipe |
| Left / Right | Horizontal swipe |
| Confirm highlighted item | Tap screen anywhere |
| Long Confirm | Hold screen |
| Back | Tap capacitive Home key |
| Long Back | Hold capacitive Home key |
| Nap / wake | Short Power press |
| Deep sleep | Hold Power, as in Flowe |

Swipe directions assume the default portrait orientation. In landscape reading,
use the physical navigation buttons for reliable page turns. USB mass-storage
mode and direct touch hit-testing are not added in this port.

## Build

```sh
cd xphone-os
pio run -e x4pro
```

`x4pro` is the only Pro target. `x3`, `x4`, and `xteink` are C3-only.
The S3 uses the standard dual-slot Flowe partition layout. That layout may differ
from stock or another firmware, so **do not install the first Pro build by
copying its app image over an unknown existing partition layout**.

## First install over USB

Use the packaged `bootloader.bin`, `partitions.bin`, `boot_app0.bin`, and
`firmware.bin` together. Requires a firmware-unlocked/developer X4 Pro and
esptool 5 (`python -m pip install 'esptool>=5,<6'`). Replace `PORT` with your
actual USB serial port. Enter the device's download mode if it does not connect.

First save the full 16 MB flash outside the SD card:

```sh
python -m esptool --chip esp32s3 --port PORT read-flash 0 ALL x4pro-before-flowe.bin
```

Keep that backup. Then, from the unpacked Pro firmware directory:

```sh
python -m esptool --chip esp32s3 --port PORT write-flash \
  0x0 bootloader.bin 0x8000 partitions.bin \
  0xe000 boot_app0.bin 0x10000 firmware.bin
```

This changes the bootloader, partition table, OTA selection, and first app slot.
The command intentionally does not erase the entire chip or write the NVS gap.
Existing apps may no longer boot under the new layout. Do not bypass chip or
security checks with `--force`. Disconnect/restart normally after flashing.

To restore, use your own full backup:

```sh
python -m esptool --chip esp32s3 --port PORT write-flash 0x0 x4pro-before-flowe.bin
```

After Flowe is installed, its Settings → SD Firmware Update can install future
**x4pro** app images. The boot-time `/update.bin` path also works with this
layout. Remove any old C3 `update.bin` from the card before first boot; it will
be rejected but could delay normal startup.

## First hardware test

1. Confirm display output, correct orientation, and both physical buttons.
2. Confirm tap/hold, four swipe directions, and Home tap/hold navigation.
3. Open an EPUB from SD, turn pages, and return to the launcher.
4. Adjust brightness/warmth; check both survive restart and turn off in sleep.
5. Pair with Flowe and test priorities/calendar, notifications, and file transfer.
6. Test short Power nap/wake and long Power deep-sleep/wake, on USB and battery.
7. Configure TRMNL; check initial fetch, scheduled nap refresh, and offline cache.

Serial logs are 115200 baud. A successful build does not validate screen
waveforms, touch orientation, BLE interoperability, or power draw on your unit.
