// Thin wrapper around Adafruit_NeoPixel.
//
// Manages N independent strips (each its own GPIO/pixel-count), since a
// board might wire several separate WS2812 outputs rather than one chain.
// Takes pins/counts as parameters rather than reading board_config.h
// itself, so this file has no idea which board it's running on — that
// decision stays in sketch.cpp, the only place config and drivers meet.
#pragma once

#include <cstddef>
#include <cstdint>

namespace LedController {

// gpios/ledCounts must each have stripCount entries, index-aligned
// (gpios[i]/ledCounts[i] describe strip i). stripCount must not exceed the
// number of strips this module was built to support.
void begin(const int* gpios, const uint32_t* ledCounts, size_t stripCount);

void setPixel(size_t stripIndex, uint32_t pixelIndex, uint8_t r, uint8_t g, uint8_t b);
void fill(size_t stripIndex, uint8_t r, uint8_t g, uint8_t b);
void fillHSV(size_t stripIndex, uint16_t hue, uint8_t saturation, uint8_t value);

void clear(size_t stripIndex);
void show(size_t stripIndex);

// Convenience helpers that apply to every strip at once.
void fillAllHSV(uint16_t hue, uint8_t saturation, uint8_t value);
void clearAll();
void showAll();

// Flattens all configured strips into one 0..totalPixelCount()-1 index
// space, so effect code can address "every pixel we have" without knowing
// how many physical strips that's split across.
size_t totalPixelCount();
void setPixelGlobal(uint32_t globalIndex, uint8_t r, uint8_t g, uint8_t b);
void setPixelGlobalHSV(uint32_t globalIndex, uint16_t hue, uint8_t saturation, uint8_t value);

// Global brightness scale (0..255) applied to every color written through
// setPixel/fill/setPixelGlobal* above, on top of whatever color the caller
// asked for. Lets one "intensity" control sit above all lighting effects
// without each effect having to know about it.
void setBrightness(uint8_t brightness);
uint8_t brightness();

// Per-strip multiplier (0..255) layered on top of the global brightness for
// just one strip — e.g. a trigger dimming only the LEDs on its own side.
// 255 = no extra dimming (strip shows the global brightness as-is), 0 =
// that strip fully off regardless of the global brightness. Defaults to
// 255 for every strip.
void setStripDim(size_t stripIndex, uint8_t dim);

// A third multiplier (0..255) applied to every strip, on top of both the
// global brightness and the per-strip dim. Kept separate so a transient
// effect — the idle blink — can dip the eyes without disturbing either of
// the settings the user controls. Defaults to 255.
void setBlinkLevel(uint8_t level);

}  // namespace LedController
