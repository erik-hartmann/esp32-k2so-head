// Placeholder for a future ESP32-C3 board. BLE only (no BT Classic) — still
// fine for Xbox controller pairing, which uses BLE HID.
// Fill in real pin numbers before using this env; values below are unverified.
#pragma once

#define BOARD_NAME "ESP32-C3 (unconfigured placeholder)"
#define BOARD_HAS_BT_CLASSIC 0

#define LED_STRIP_COUNT 1
#define LED_STRIP_GPIOS { 2 }        // TODO: verify against your C3 board's pinout
#define LED_STRIP_LED_COUNTS { 60 }  // TODO: set to your strip's actual pixel count
