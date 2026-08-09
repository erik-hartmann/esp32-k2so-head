// SPDX-License-Identifier: Apache-2.0
//
// Xbox controller -> WS2812 LED demo, built on Bluepad32 + Espressif's
// led_strip component, plus a WiFi-hosted web control panel.
//
// Gamepad controls:
//   - LT (left trigger): dims strip 0 ("left" — see the active board's
//     LED_STRIP_GPIOS in board_config.h for which physical pin that is).
//     Fully released -> current full brightness; fully pulled -> off.
//   - RT (right trigger): same, for strip 1 ("right"). If your "left"/
//     "right" is physically swapped from this, swap the two setStripDim()
//     calls below.
//   - L1 / R1: step back/forward through LightEffects' lighting patterns.
//   - X: toggle everything on/off, remembering the brightness it was at.
//     Also parks the eyes' idle behavior while off, resuming it when on.
//   - A: lower brightness while held. B: raise brightness while held.
//   - Y: step the accent color's "home" (stick-centered) value through a
//     small rainbow palette. Double-click Y instead pins the color the left
//     stick is currently steering as that home value, until reboot.
//   - Right stick (if a PCA9685 is configured — see I2C_SDA_GPIO in the
//     active board_config.h): pans/tilts the two servos driving the eyes.
//     X axis -> channel 0 (pan), Y axis -> channel 1 (tilt), eased rather
//     than mapped straight through. With no controller connected the eyes
//     run autonomous idle behavior instead — see eye_motion.cpp, which is
//     also where you'd swap the axes or flip a direction for your linkage.
//   - D-pad Up: toggle the WiFi access point and web UI, which start OFF.
//   - D-pad Down: play the next audio clip (if a DFPlayer is configured —
//     see AUDIO_RX_GPIO in the active board_config.h).
// Web controls (see web_ui.cpp), once enabled: pick any pattern directly,
// set the shared accent color, or set brightness — all from a phone/laptop
// browser. Gamepad and web input are peers: both just call into
// LightEffects/LedController, so either works with or without the other.
//
// Board-specific values (pins, LED counts) live in board_config.h and its
// selected board_configs/*.h — nothing below should hardcode hardware
// specifics. Bluepad32 itself is only referenced inside gamepad_input.cpp;
// led_strip is only referenced inside led_controller.cpp; WiFi/HTTP only
// inside web_ui.cpp.

#include "sdkconfig.h"

#include <Arduino.h>
#include <Bluepad32.h>

#include "board_config.h"
#include "gamepad_input.h"
#include "led_controller.h"
#include "light_effects.h"
#include "web_ui.h"

#ifdef I2C_SDA_GPIO
#include "eye_motion.h"
#include "servo_controller.h"
#endif

#ifdef AUDIO_RX_GPIO
#include "audio_player.h"
#endif

// README FIRST:
// Bluepad32's built-in console is incompatible with Arduino's "Serial"
// class, so this file uses Bluepad32's "Console" instead (similar API).
// To use "Serial" instead, disable Bluepad32's console via
// sdkconfig.defaults: CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE=n

namespace {

// Set true to hold both servos at 0deg and disable the right-stick pan/tilt
// control below — useful when physically attaching/re-attaching linkages at
// a known position.
constexpr bool kServoZeroingMode = false;

constexpr int kBrightnessStepPerTick = 6;  // out of 0..255, per loop() tick while held
constexpr uint32_t kDoubleClickWindowMs = 400;

bool sL1WasPressed = false;
bool sR1WasPressed = false;
bool sXWasPressed = false;
bool sYWasPressed = false;
bool sDpadUpWasPressed = false;
bool sDpadDownWasPressed = false;
uint32_t sLastYPressMs = 0;
uint16_t sNextAudioTrack = 1;
uint8_t sBrightnessBeforeOff = 200;

}  // namespace

void setup() {
    Console.printf("Firmware: %s\n", BP32.firmwareVersion());
    Console.printf("Board: %s\n", BOARD_NAME);

    GamepadInput::begin();

    static const int kLedGpios[] = LED_STRIP_GPIOS;
    static const uint32_t kLedCounts[] = LED_STRIP_LED_COUNTS;
    LedController::begin(kLedGpios, kLedCounts, LED_STRIP_COUNT);
    LedController::clearAll();
    LedController::showAll();

    LightEffects::begin();
    Console.printf("Light effect: %s\n", LightEffects::currentName());

    WebUI::begin();

#ifdef AUDIO_RX_GPIO
    AudioPlayer::begin(AUDIO_RX_GPIO, AUDIO_TX_GPIO);
    AudioPlayer::setVolume(20);
#endif

#ifdef I2C_SDA_GPIO
    ServoController::begin(I2C_SDA_GPIO, I2C_SCL_GPIO);
    // Tilt (channel 1): +10deg evenly around center vs. the default
    // 0..180 range, plus another +5deg on the "up" end only (angle=180,
    // pushed by axisRY toward max pulse — see the right-stick handling
    // below). "Down" (972us / angle=0) stays untouched.
    ServoController::setPulseRange(1, 972, 2056);
    if (kServoZeroingMode) {
        ServoController::setAngle(0, 0);
        ServoController::setAngle(1, 0);
        Console.println("Servo zeroing mode: both channels held at 0deg for attaching linkages");
    } else {
        EyeMotion::begin();
        Console.println("Eye motion ready (channels 0/1) — idle behavior active until a gamepad connects");
    }
#endif
}

void loop() {
    GamepadInput::update();
    const GamepadState& gp = GamepadInput::state();
    WebUI::handleClient();

    if (gp.connected) {
        // LT/RT continuously dim their own side: released (0) -> full
        // brightness, fully pulled (1023) -> that side off. Runs every
        // frame since triggers are analog, not edge-triggered.
        uint8_t leftDim = static_cast<uint8_t>(map(gp.brake, 0, 1023, 255, 0));
        uint8_t rightDim = static_cast<uint8_t>(map(gp.throttle, 0, 1023, 255, 0));
        LedController::setStripDim(0, leftDim);
        LedController::setStripDim(1, rightDim);

        // L1/R1 step back/forward through the lighting patterns.
        // Edge-triggered so holding doesn't rapid-fire through effects.
        if (gp.l1 && !sL1WasPressed) {
            LightEffects::previous();
        }
        if (gp.r1 && !sR1WasPressed) {
            LightEffects::next();
        }
        sL1WasPressed = gp.l1;
        sR1WasPressed = gp.r1;

        // X toggles on/off, remembering the brightness it was at, and parks
        // the eyes' idle behavior while dark so a switched-off head isn't
        // still quietly driving its servos.
        if (gp.x && !sXWasPressed) {
            bool turningOn = LedController::brightness() == 0;
            if (turningOn) {
                LedController::setBrightness(sBrightnessBeforeOff);
            } else {
                sBrightnessBeforeOff = LedController::brightness();
                LedController::setBrightness(0);
            }
#ifdef I2C_SDA_GPIO
            EyeMotion::setIdleEnabled(turningOn);
#endif
        }
        sXWasPressed = gp.x;

        // Single-click Y steps the stick-centered "home" accent color through
        // a small rainbow palette. Double-click instead pins whatever color
        // is showing right now (i.e. whatever the left stick is steering) as
        // that home color, until the next reboot.
        if (gp.y && !sYWasPressed) {
            uint32_t now = millis();
            if (sLastYPressMs != 0 && (now - sLastYPressMs) <= kDoubleClickWindowMs) {
                LightEffects::setDefaultAccentToCurrent();
                sLastYPressMs = 0;  // consumed, so a third click starts fresh
            } else {
                LightEffects::cycleDefaultAccentColor();
                sLastYPressMs = now;
            }
        }
        sYWasPressed = gp.y;

        // D-pad Up toggles the WiFi AP and web UI. Off by default because an
        // idle soft-AP alongside the BT stack is the biggest current draw
        // here, and most sessions are gamepad-only.
        if (gp.dpadUp && !sDpadUpWasPressed) {
            if (WebUI::isRunning()) {
                WebUI::stop();
            } else {
                WebUI::start();
            }
        }
        sDpadUpWasPressed = gp.dpadUp;

#ifdef AUDIO_RX_GPIO
        // D-pad Down plays the next clip, cycling through the card. Sequential
        // rather than random so each file can be verified on first setup —
        // swap in random(1, AUDIO_TRACK_COUNT + 1) once they're all confirmed.
        if (gp.dpadDown && !sDpadDownWasPressed) {
            AudioPlayer::play(sNextAudioTrack);
            sNextAudioTrack = (sNextAudioTrack % AUDIO_TRACK_COUNT) + 1;
        }
        sDpadDownWasPressed = gp.dpadDown;
#endif

        // A/B ramp brightness down/up while held.
        if (gp.a) {
            int next = static_cast<int>(LedController::brightness()) - kBrightnessStepPerTick;
            LedController::setBrightness(static_cast<uint8_t>(next < 0 ? 0 : next));
        } else if (gp.b) {
            int next = static_cast<int>(LedController::brightness()) + kBrightnessStepPerTick;
            LedController::setBrightness(static_cast<uint8_t>(next > 255 ? 255 : next));
        }

    } else {
        sL1WasPressed = false;
        sR1WasPressed = false;
        sXWasPressed = false;
        sYWasPressed = false;
        sDpadUpWasPressed = false;
        sDpadDownWasPressed = false;
        sLastYPressMs = 0;
        // No triggers to read — leave both sides at full brightness rather
        // than stuck at whatever dim level they were last at.
        LedController::setStripDim(0, 255);
        LedController::setStripDim(1, 255);
    }

#ifdef I2C_SDA_GPIO
    // Runs in both states: drives the servos from the right stick when a
    // gamepad is connected, and runs autonomous idle behavior when not.
    if (!kServoZeroingMode) {
        EyeMotion::update(gp);
    }
#endif

    // Runs regardless of gamepad connection so the web UI works standalone.
    LightEffects::update(gp);
    LedController::showAll();

    // The main loop must yield to lower-priority tasks or the watchdog will
    // trip. See: https://stackoverflow.com/questions/66278271
    delay(30);
}
