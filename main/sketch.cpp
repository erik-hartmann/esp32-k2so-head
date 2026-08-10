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
//   - D-pad Up / Down: audio volume up/down, repeating while held (if a
//     DFPlayer is configured — see AUDIO_RX_GPIO in the active
//     board_config.h). On release, the selected clip replays at the new level
//     so it can be judged by ear.
//   - D-pad Left / Right: step back/forward through the audio clips, playing
//     each as it is selected and wrapping around at either end.
//   - Menu button (the three-lines icon): replay the currently selected clip.
// The WiFi access point and web UI have no gamepad control at all. They are
// toggled by holding the board's admin button (ADMIN_BUTTON_GPIO in the active
// board_config.h — the DevKitC's BOOT button), because bringing up an access
// point is a setup action rather than a performance one, and it needs to work
// when no controller is paired.
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
#include "ota_service.h"
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

// Volume repeats while held rather than stepping once per press, but is rate
// limited: loop() runs every ~30ms, and pushing a serial command down to the
// DFPlayer that often would flood a link the module cannot drain that fast.
constexpr int kVolumeStep = 1;               // out of 0..30
constexpr uint32_t kVolumeRepeatMs = 120;

// The admin button toggles on a deliberate hold rather than a tap, so a
// brushed button can't silently bring the access point up mid-show.
constexpr uint32_t kAdminHoldMs = 1000;

bool sL1WasPressed = false;
bool sR1WasPressed = false;
bool sXWasPressed = false;
bool sYWasPressed = false;
bool sDpadLeftWasPressed = false;
bool sDpadRightWasPressed = false;
bool sStartWasPressed = false;
bool sVolumeAdjusted = false;
uint32_t sAdminPressedAtMs = 0;
bool sAdminHoldFired = false;
uint32_t sLastYPressMs = 0;
uint32_t sLastVolumeChangeMs = 0;
uint8_t sBrightnessBeforeOff = 200;

// Two quick flashes confirming an admin toggle actually landed: green when
// the access point has just come up, red when it has gone down. Without this
// the button is undebuggable by feel — a 1s hold with no feedback gives you
// no way to tell a missed press from one you released too early.
//
// Blocking, deliberately. This is a once-in-a-while admin action on an
// otherwise idle head, and a non-blocking version would need a state machine
// in loop() for something that runs for a third of a second.
void flashAdminAck(bool apNowRunning) {
    const uint8_t savedBrightness = LedController::brightness();
    // Degrees, not Adafruit's 16-bit hue — LedController::fillAllHSV converts.
    const uint16_t hue = apNowRunning ? 120 : 0;  // green : red

    // Force both multipliers open, or the flash is invisible when the lights
    // are switched off at X or a blink happens to be in flight.
    LedController::setBlinkLevel(255);
    LedController::setBrightness(160);

    for (int i = 0; i < 2; i++) {
        LedController::fillAllHSV(hue, 255, 255);
        LedController::showAll();
        delay(120);
        LedController::clearAll();
        LedController::showAll();
        delay(90);
    }

    LedController::setBrightness(savedBrightness);
    // Pixels need no restoring — LightEffects::update() repaints every frame.
}

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

#ifdef ADMIN_BUTTON_GPIO
    pinMode(ADMIN_BUTTON_GPIO, INPUT_PULLUP);
    Console.printf("Admin button on GPIO%d: hold %ums to toggle the WiFi AP and web UI\n",
                   ADMIN_BUTTON_GPIO, kAdminHoldMs);
#endif

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
    // No-op unless the AP is up. Once an upload actually starts this blocks
    // until the board reboots into the new image.
    OtaService::handle();

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

#ifdef AUDIO_RX_GPIO
        // D-pad Up/Down ramp the volume while held. Not edge-triggered: the
        // module's range is 0..30, and stepping once per press would mean
        // thirty presses to cross it.
        if (gp.dpadUp || gp.dpadDown) {
            uint32_t now = millis();
            if (now - sLastVolumeChangeMs >= kVolumeRepeatMs) {
                int next = static_cast<int>(AudioPlayer::volume()) +
                           (gp.dpadUp ? kVolumeStep : -kVolumeStep);
                if (next < 0) {
                    next = 0;
                } else if (next > 30) {
                    next = 30;
                }
                AudioPlayer::setVolume(static_cast<uint8_t>(next));
                sLastVolumeChangeMs = now;
                sVolumeAdjusted = true;
            }
        } else if (sVolumeAdjusted) {
            // Replay once the direction is released, so the new level can be
            // heard. Deliberately not on every step: the ramp repeats every
            // kVolumeRepeatMs, and restarting the clip that often would just
            // stutter. A change made while something is already playing is
            // audible immediately anyway — the module applies volume live.
            sVolumeAdjusted = false;
            AudioPlayer::replay();
        }

        // D-pad Left/Right step through the clips and play each on selection,
        // wrapping at both ends. Edge-triggered so holding a direction doesn't
        // restart a clip every frame.
        if (gp.dpadRight && !sDpadRightWasPressed) {
            AudioPlayer::playNext();
        }
        if (gp.dpadLeft && !sDpadLeftWasPressed) {
            AudioPlayer::playPrevious();
        }
        sDpadRightWasPressed = gp.dpadRight;
        sDpadLeftWasPressed = gp.dpadLeft;

        // Menu replays whatever is currently selected, which stepping alone
        // cannot do — Left then Right would land you back on the same track
        // but play its neighbour on the way.
        if (gp.start && !sStartWasPressed) {
            AudioPlayer::replay();
        }
        sStartWasPressed = gp.start;
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
        sDpadLeftWasPressed = false;
        sDpadRightWasPressed = false;
        sStartWasPressed = false;
        sLastYPressMs = 0;
        // No triggers to read — leave both sides at full brightness rather
        // than stuck at whatever dim level they were last at.
        LedController::setStripDim(0, 255);
        LedController::setStripDim(1, 255);
    }

#ifdef ADMIN_BUTTON_GPIO
    // Checked whether or not a gamepad is connected — needing the web UI with
    // no controller paired is a normal case, and arguably the main one.
    // Active low: the pin idles high on its pull-up and is grounded when the
    // button is down.
    if (digitalRead(ADMIN_BUTTON_GPIO) == LOW) {
        uint32_t now = millis();
        if (sAdminPressedAtMs == 0) {
            sAdminPressedAtMs = now;
            // Logged on the press, not just on the toggle, so a press that is
            // released too early is still visible in the serial log. Without
            // this there is no way to tell "the pin never went low" from
            // "you let go at 800ms".
            Console.printf("Admin: button down — hold %ums to toggle\n", kAdminHoldMs);
        } else if (!sAdminHoldFired && (now - sAdminPressedAtMs) >= kAdminHoldMs) {
            if (WebUI::isRunning()) {
                // OTA first: it holds a socket on an interface WebUI::stop()
                // is about to take down.
                OtaService::stop();
                WebUI::stop();
                GamepadInput::setDiscoverable(true);
            } else {
                // Before WiFi, not after: a continuous Bluetooth scan starves
                // the shared radio badly enough that a client can associate
                // but never complete DHCP.
                GamepadInput::setDiscoverable(false);
                WebUI::start();
                if (WebUI::isRunning()) {
                    // Only after the AP is actually up — begin() binds a
                    // socket, and there is no interface to bind to before
                    // this point.
                    OtaService::begin();
                } else {
                    // The AP did not come up, so give the radio back. Without
                    // this, a failed start leaves scanning off with nothing to
                    // show for it: no access point, and no way to pair a
                    // controller, recoverable only by rebooting.
                    GamepadInput::setDiscoverable(true);
                }
            }
            // Latch until release, so one hold is one toggle rather than one
            // per loop() tick for as long as the button is down.
            sAdminHoldFired = true;
            flashAdminAck(WebUI::isRunning());
        }
    } else {
        if (sAdminPressedAtMs != 0 && !sAdminHoldFired) {
            Console.printf("Admin: released after %ums — too short, nothing toggled\n",
                           millis() - sAdminPressedAtMs);
        }
        sAdminPressedAtMs = 0;
        sAdminHoldFired = false;
    }
#endif

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
