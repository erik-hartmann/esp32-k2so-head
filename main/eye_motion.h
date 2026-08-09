// Drives the eye pan/tilt servos, and owns two behaviors on top of the raw
// ServoController driver:
//
//   - Easing. Both gamepad and idle motion set a *target* angle; the servos
//     ease toward it rather than jumping. Removes the twitchiness of mapping
//     stick position straight to angle, and is what makes idle motion read
//     as organic instead of robotic.
//   - Idle behavior. Picks its own targets — mostly small darting saccades
//     with the occasional larger look — and blinks the eyes at random
//     intervals. Runs whenever no controller is connected, and also resumes
//     while one *is* connected once the right stick has sat centered for a
//     few seconds, so the eyes come back to life if you stop driving them.
//     Any stick movement takes control straight back.
//
// Sits above ServoController (a dumb PWM driver) and below sketch.cpp, which
// just calls update() once per loop and lets this decide what the eyes do.
#pragma once

#include "gamepad_input.h"

namespace EyeMotion {

void begin();

// Call once per loop() tick. While the right stick is being moved it sets
// the target and blinking is suspended; otherwise idle behavior takes over.
// Either way this eases the servos toward the current target and writes
// them out.
void update(const GamepadState& gamepad);

// Enables or disables the autonomous idle behavior — saccades and blinking.
// Used to park the eyes while the lights are off, so a dark head isn't
// quietly working its servos. Direct stick control is unaffected either way.
// Defaults to enabled.
void setIdleEnabled(bool enabled);

}  // namespace EyeMotion
