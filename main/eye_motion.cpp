#include "eye_motion.h"

#include <Arduino.h>
#include <Bluepad32.h>

#include "board_config.h"
#include "esp_random.h"
#include "led_controller.h"
#include "servo_controller.h"

// Boards that have not been trimmed sit at a true 90 degrees.
#ifndef EYE_PAN_TRIM_DEG
#define EYE_PAN_TRIM_DEG 0.0f
#endif
#ifndef EYE_TILT_TRIM_DEG
#define EYE_TILT_TRIM_DEG 0.0f
#endif

namespace {

constexpr uint8_t kPanChannel = 0;
constexpr uint8_t kTiltChannel = 1;

// Fraction of the remaining distance covered each tick. Higher = snappier.
// Gamepad control is deliberately quicker than idle so direct driving still
// feels responsive, while idle drifts more organically.
constexpr float kGamepadEase = 0.35f;
constexpr float kIdleEase = 0.22f;

constexpr float kCenterPan = 90.0f;
constexpr float kCenterTilt = 90.0f;

// Idle stays well inside the mechanical limits — real eyes rarely sit at
// full deflection, and it keeps the linkages off their end stops.
constexpr float kIdleMinAngle = 40.0f;
constexpr float kIdleMaxAngle = 140.0f;

// Largest single hop. Actual hops are biased well below this — see
// hopMagnitude() — so this is a ceiling, not a typical value.
constexpr float kIdleMaxHop = 55.0f;

// Each hop is nudged back toward center by this fraction, so a run of
// random hops can wander a region without drifting into a corner and
// parking there.
constexpr float kIdleCenterPull = 0.18f;

constexpr uint32_t kIdleHoldMinMs = 1400;
constexpr uint32_t kIdleHoldMaxMs = 5600;

// Occasionally hold much longer than a normal pause. Breaks up the rhythm
// so the movement doesn't settle into a detectable cadence.
constexpr int kIdleStareChancePct = 15;
constexpr uint32_t kIdleStareMinMs = 6000;
constexpr uint32_t kIdleStareMaxMs = 14000;

// A hard cut rather than a soft fade — photoreceptors snapping off reads
// more mechanical than an organic eyelid, and at a 30ms loop there aren't
// enough frames for a smooth fade down anyway. The ramp back up is short
// enough to feel like the optics re-lighting.
constexpr uint32_t kBlinkClosedMs = 120;
constexpr uint32_t kBlinkOpenMs = 90;
constexpr uint32_t kBlinkGapMinMs = 4500;
constexpr uint32_t kBlinkGapMaxMs = 13000;

// Idle also resumes while a controller is still connected, once the right
// stick has sat centered this long. Threshold is well above resting stick
// noise so a controller lying on a table doesn't read as "being driven".
constexpr int32_t kStickActiveThreshold = 80;  // out of -511..512
constexpr uint32_t kIdleResumeMs = 5000;

float sPan = kCenterPan;
float sTilt = kCenterTilt;
float sTargetPan = kCenterPan;
float sTargetTilt = kCenterTilt;

uint32_t sNextSaccadeMs = 0;
uint32_t sNextBlinkMs = 0;
uint32_t sBlinkStartMs = 0;
bool sBlinking = false;
uint32_t sIdleResumeAtMs = 0;
bool sIdleEnabled = true;

float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// Squaring a uniform 0..1 roll concentrates the distribution near zero, so
// most hops are small darts with occasional big looks — a continuum rather
// than two fixed sizes. Rolled independently per axis so movement isn't
// always diagonal.
float hopMagnitude() {
    float r = static_cast<float>(random(1001)) / 1000.0f;
    float sign = (random(2) == 0) ? -1.0f : 1.0f;
    return sign * kIdleMaxHop * r * r;
}

// Rollover-safe "has this deadline passed?" — millis() wraps after ~49 days.
bool deadlinePassed(uint32_t deadlineMs) {
    return static_cast<int32_t>(millis() - deadlineMs) >= 0;
}

void scheduleSaccade() {
    if (random(100) < kIdleStareChancePct) {
        sNextSaccadeMs = millis() + random(kIdleStareMinMs, kIdleStareMaxMs);
    } else {
        sNextSaccadeMs = millis() + random(kIdleHoldMinMs, kIdleHoldMaxMs);
    }
}

void scheduleBlink() {
    sNextBlinkMs = millis() + random(kBlinkGapMinMs, kBlinkGapMaxMs);
}

void pickIdleTarget() {
    // Hops are relative to where the eyes already are, not to center, so
    // they explore a region — a few small darts in one area, then a larger
    // move elsewhere — instead of always orbiting the same point.
    float pan = sTargetPan + hopMagnitude();
    float tilt = sTargetTilt + hopMagnitude();
    pan += (kCenterPan - pan) * kIdleCenterPull;
    tilt += (kCenterTilt - tilt) * kIdleCenterPull;
    sTargetPan = clampf(pan, kIdleMinAngle, kIdleMaxAngle);
    sTargetTilt = clampf(tilt, kIdleMinAngle, kIdleMaxAngle);
    scheduleSaccade();
}

void updateBlink() {
    if (sBlinking) {
        uint32_t elapsed = millis() - sBlinkStartMs;
        if (elapsed < kBlinkClosedMs) {
            LedController::setBlinkLevel(0);
        } else if (elapsed < kBlinkClosedMs + kBlinkOpenMs) {
            uint32_t t = elapsed - kBlinkClosedMs;
            LedController::setBlinkLevel(static_cast<uint8_t>(255 * t / kBlinkOpenMs));
        } else {
            LedController::setBlinkLevel(255);
            sBlinking = false;
            scheduleBlink();
        }
    } else if (deadlinePassed(sNextBlinkMs)) {
        sBlinking = true;
        sBlinkStartMs = millis();
    }
}

// Leaves the eyes lit and re-arms both timers, so handing control back and
// forth doesn't strand the LEDs mid-blink or fire a saccade instantly.
void suspendIdle() {
    if (sBlinking) {
        sBlinking = false;
        LedController::setBlinkLevel(255);
    }
    scheduleSaccade();
    scheduleBlink();
}

}  // namespace

void EyeMotion::begin() {
    randomSeed(esp_random());  // otherwise idle plays the same sequence every boot
    sPan = kCenterPan;
    sTilt = kCenterTilt;
    sTargetPan = kCenterPan;
    sTargetTilt = kCenterTilt;
    sIdleResumeAtMs = millis();  // idle is available immediately on boot
    scheduleSaccade();
    scheduleBlink();
}

void EyeMotion::setIdleEnabled(bool enabled) {
    if (enabled == sIdleEnabled) {
        return;
    }
    sIdleEnabled = enabled;

    if (enabled) {
        // Re-arm both timers so resuming doesn't immediately fire a saccade
        // or a blink the instant the lights come back.
        scheduleSaccade();
        scheduleBlink();
    } else {
        // Critical: if a blink was in flight, the blink multiplier is sitting
        // at 0. Leaving it there would mean the lights come back on still
        // dark, since nothing else would ever reset it.
        sBlinking = false;
        LedController::setBlinkLevel(255);
    }
}

void EyeMotion::update(const GamepadState& gamepad) {
    // Compared against the threshold directly rather than via abs(), which is
    // a macro in Arduino and bites on anything more complex than a variable.
    bool stickActive = gamepad.connected &&
                       (gamepad.axisRX > kStickActiveThreshold || gamepad.axisRX < -kStickActiveThreshold ||
                        gamepad.axisRY > kStickActiveThreshold || gamepad.axisRY < -kStickActiveThreshold);

    if (stickActive) {
        sIdleResumeAtMs = millis() + kIdleResumeMs;
        suspendIdle();
    }

    // Idle takes over when the controller is gone, or when it's still
    // connected but the stick has been parked long enough.
    bool idle = !gamepad.connected || deadlinePassed(sIdleResumeAtMs);

    float ease;
    if (idle) {
        // While disabled the eyes hold their last target rather than
        // freezing mid-move, so any saccade already in flight finishes.
        if (sIdleEnabled) {
            if (deadlinePassed(sNextSaccadeMs)) {
                pickIdleTarget();
            }
            updateBlink();
        }
        ease = kIdleEase;
    } else {
        // Right stick sets the target. Y is inverted so pushing the stick up
        // tilts the eyes up.
        sTargetPan = static_cast<float>(map(gamepad.axisRX, -511, 512, 0, 180));
        sTargetTilt = static_cast<float>(map(gamepad.axisRY, -511, 512, 180, 0));
        ease = kGamepadEase;
    }

    sPan += (sTargetPan - sPan) * ease;
    sTilt += (sTargetTilt - sTilt) * ease;

    // Trim is applied here, at the very last step, rather than folded into
    // the centre constants or the stick mapping. Everything above -- idle
    // hops, centre pull, easing, the stick range -- keeps reasoning in clean
    // 0..180 with 90 as centre, and the mechanical offset of this particular
    // build is corrected once, on the way out to the servos.
    ServoController::setAngle(kPanChannel, clampf(sPan + EYE_PAN_TRIM_DEG, 0.0f, 180.0f));
    ServoController::setAngle(kTiltChannel, clampf(sTilt + EYE_TILT_TRIM_DEG, 0.0f, 180.0f));
}
