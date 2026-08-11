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

// Eye centring trim, in degrees, added to every commanded angle.
//
// A servo horn can only be fitted in whole spline steps, so "mechanically
// centred" almost never lands exactly on 90 degrees. This corrects that in
// software instead of fighting the splines. It is a property of this
// particular build -- reprint the linkages or refit a horn and it changes.
//
// To calibrate: let the eyes settle at centre, see how far off they sit, and
// put that many degrees here. If they move the wrong way, flip the sign.
// Keep it small; the trim shifts the whole range, so a large value costs
// travel at the far end once the result clamps at 0/180.
#define EYE_PAN_TRIM_DEG -10.0f
#define EYE_TILT_TRIM_DEG 0.0f

// DFPlayer Mini on UART2. AUDIO_RX is the ESP32 pin receiving the
// DFPlayer's TX, and vice versa — cross them when wiring.
//
// GPIO16/17 are free on a WROOM-32. Note they are NOT free on a WROVER,
// where they're wired to the PSRAM — if this board is ever swapped for a
// WROVER, move these to e.g. 32/33.
#define AUDIO_RX_GPIO 16
#define AUDIO_TX_GPIO 17

// How many /mp3/NNNN.mp3 files are on the card. Must be kept in step with the
// card by hand: the module can report its own file count, but only over its TX
// pin, which this build does not wire (3-conductor cable, write-only driver).
// See AudioPlayer::trackCount() for what making this automatic would cost.
#define AUDIO_TRACK_COUNT 16

// Admin button — held to toggle the WiFi AP and web UI. GPIO0 is the
// DevKitC's on-board BOOT button: already fitted, already pulled up, and free
// for general input once the board is running. (Holding it *during reset*
// still drops the chip into the serial bootloader; that is a boot-time
// behaviour and unrelated to reading the pin afterwards.)
//
// Deliberately not on the gamepad. Enabling the web UI is a setup action, not
// something to do mid-performance, and it should work when no controller is
// paired — which is often exactly when you want the web UI.
#define ADMIN_BUTTON_GPIO 0

// Reserved for components you add later (uncomment and wire up as needed).
// Keeping the names here even before they're used documents which pins on
// this specific board are already spoken for (GPIO25/GPIO26 by the LEDs,
// GPIO21/GPIO22 by I2C, GPIO16/GPIO17 by the DFPlayer, GPIO0 by the admin
// button) and which are free.
