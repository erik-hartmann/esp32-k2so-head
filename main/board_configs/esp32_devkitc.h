// Board: Espressif ESP32-DevKitC (official, ESP32-WROOM-32, dual-core, BT
// Classic + BLE) — same chip family as the ELEGOO board, different pinout.
#pragma once

#define BOARD_NAME "Espressif ESP32-DevKitC"
#define BOARD_HAS_BT_CLASSIC 1

// Two independent 7-LED WS2812 units, each wired directly to its own GPIO
// (not daisy-chained to each other: no DOUT->DIN link between the two).
// Strip 0 (GPIO25) is treated as "left" (LT dims it) and strip 1 (GPIO26)
// as "right" (RT dims it) in sketch.cpp — swap the two numbers here if
// that's backwards.
#define LED_STRIP_COUNT 2
#define LED_STRIP_GPIOS { 25, 26 }
#define LED_STRIP_LED_COUNTS { 7, 7 }

// PCA9685 servo driver (16-channel I2C PWM), two servos wired to its
// channels 0 and 1. Default ESP32 I2C pins.
#define I2C_SDA_GPIO 21
#define I2C_SCL_GPIO 22

// DFPlayer Mini on UART2. AUDIO_RX is the ESP32 pin receiving the
// DFPlayer's TX, and vice versa — cross them when wiring.
//
// GPIO16/17 are free on a WROOM-32. Note they are NOT free on a WROVER,
// where they're wired to the PSRAM — if this board is ever swapped for a
// WROVER, move these to e.g. 32/33.
#define AUDIO_RX_GPIO 16
#define AUDIO_TX_GPIO 17
#define AUDIO_TRACK_COUNT 4  // how many /mp3/NNNN.mp3 files are on the card

// Reserved for components you add later (uncomment and wire up as needed).
// Keeping the names here even before they're used documents which pins on
// this specific board are already spoken for (GPIO25/GPIO26 by the LEDs,
// GPIO21/GPIO22 by I2C, GPIO16/GPIO17 by the DFPlayer) and which are free.
// #define BUTTON_GPIO      -1
