// Board: ELEGOO EL-SM-012 (ESP32-WROOM-32, dual-core, BT Classic + BLE)
#pragma once

#define BOARD_NAME "ELEGOO EL-SM-012 (ESP32-WROOM-32)"
#define BOARD_HAS_BT_CLASSIC 1

// Two independent 7-LED WS2812 units, each wired directly to its own GPIO
// (not daisy-chained to each other: no DOUT->DIN link between the two).
// Moved from GPIO5/GPIO4 to D26/D27 when the board switched to external 5V
// via VIN. Strip 0 (D26) is treated as "left" (LT dims it) and strip 1
// (D27) as "right" (RT dims it) in sketch.cpp, same left/right convention
// as before — swap the two numbers here if that's backwards.
#define LED_STRIP_COUNT 2
#define LED_STRIP_GPIOS { 26, 27 }
#define LED_STRIP_LED_COUNTS { 7, 7 }

// Reserved for components you add later (uncomment and wire up as needed).
// Keeping the names here even before they're used documents which pins on
// this specific board are already spoken for (D26/D27 by the LEDs) and
// which are still free.
// #define BUTTON_GPIO      -1
// #define SERVO_GPIO       -1
// #define I2C_SDA_GPIO     -1
// #define I2C_SCL_GPIO     -1
