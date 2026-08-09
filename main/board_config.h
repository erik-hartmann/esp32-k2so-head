// Selects the active board's configuration header.
//
// Each PlatformIO environment defines BOARD_CONFIG_HEADER via build_flags
// (see platformio.ini), pointing at one file under board_configs/. That file
// is the only place hardware specifics (pins, LED count, capabilities) live.
// Code elsewhere (sketch.cpp, led_controller.*, gamepad_input.*) must not
// hardcode pin numbers — it reads them from here instead, so adding a new
// board variant means adding one header, not touching the logic.
#pragma once

#ifndef BOARD_CONFIG_HEADER
#error "BOARD_CONFIG_HEADER is not defined. Set it in platformio.ini's [env:...] build_flags, e.g. -D BOARD_CONFIG_HEADER='\"board_configs/elegoo_esp32_wroom32.h\"'"
#endif

#include BOARD_CONFIG_HEADER

#ifndef BOARD_NAME
#error "Board config header must define BOARD_NAME"
#endif
#ifndef LED_STRIP_COUNT
#error "Board config header must define LED_STRIP_COUNT"
#endif
#ifndef LED_STRIP_GPIOS
#error "Board config header must define LED_STRIP_GPIOS, e.g. { 5, 4 }"
#endif
#ifndef LED_STRIP_LED_COUNTS
#error "Board config header must define LED_STRIP_LED_COUNTS, e.g. { 1, 1 }"
#endif
