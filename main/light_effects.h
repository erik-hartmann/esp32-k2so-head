// A cycle of lighting sequences, driven through LedController's global
// pixel-index API so it works whether that's 2 standalone LEDs or a long
// strip later on.
//
// Every pattern except the rainbow-family ones (Solid Rainbow, Rainbow
// Cycle, Chasing Rainbow) renders using one shared "accent color" instead
// of a color baked into the pattern, so picking a color — from the web UI's
// picker, or from the gamepad's left stick, which drives it continuously
// whenever a controller is connected — re-colors whichever pattern is
// currently running, no matter which one that is. While the stick sits in
// its center deadzone, the accent color holds at a "default" that Y cycles
// through a small rainbow palette (see cycleDefaultAccentColor), or that a
// double-click of Y overwrites with the current stick color for the rest of
// the session (see setDefaultAccentToCurrent).
//
// Purely a pattern generator: it decides colors and writes them via
// LedController, but the caller (sketch.cpp) still owns calling
// LedController::showAll() to actually push a frame to the hardware, and
// owns any global brightness control. Gamepad and web input are peers here —
// both just call into this module and neither knows the other exists.
#pragma once

#include <cstddef>
#include <cstdint>

#include "gamepad_input.h"

namespace LightEffects {

void begin();

// Advances animation state for the current pattern and writes pixel colors.
// Also feeds the gamepad's left stick into the shared accent color every
// call (when connected), regardless of which pattern is active.
void update(const GamepadState& gamepad);

// Steps through the named patterns (wraps both directions).
void next();
void previous();

// Jumps directly to pattern `index` (0..effectCount()-1).
void selectEffect(uint8_t index);

// Updates the shared accent color used by every non-rainbow pattern. Takes
// effect immediately on whatever pattern is currently running — does not
// switch patterns.
void setAccentColor(uint8_t r, uint8_t g, uint8_t b);

// Steps to the next color in a small fixed rainbow palette (red, orange,
// yellow, green, cyan, blue, violet), applies it immediately, and makes it
// the "home" color used whenever the stick returns to center.
void cycleDefaultAccentColor();

// Pins whatever color is showing right now — typically one the left stick is
// steering — as the new "home" color for stick-center, replacing the palette
// entry that was there. In RAM only: a reboot reverts to the built-in cyan.
void setDefaultAccentToCurrent();

const char* currentName();

// For building UI (e.g. a web page's pattern list) without duplicating the
// name table.
size_t effectCount();
const char* effectName(uint8_t index);

}  // namespace LightEffects
