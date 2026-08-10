// Thin wrapper around Bluepad32 that exposes a plain state struct.
//
// Nothing outside this file touches Bluepad32's ControllerPtr/BP32 API
// directly. That keeps effect/component code portable if the input source
// ever changes (e.g. adding a second controller, or a non-Bluepad32 input).
#pragma once

#include <cstdint>

struct GamepadState {
    bool connected = false;

    int32_t axisX = 0;   // left stick X:  -511..512
    int32_t axisY = 0;   // left stick Y:  -511..512
    int32_t axisRX = 0;  // right stick X: -511..512
    int32_t axisRY = 0;  // right stick Y: -511..512

    int32_t brake = 0;     // left trigger (LT):  0..1023
    int32_t throttle = 0;  // right trigger (RT): 0..1023

    uint16_t buttons = 0;  // raw bitmask, see Bluepad32's ControllerConst.h
    uint8_t dpad = 0;      // raw bitmask

    bool a = false;
    bool b = false;
    bool x = false;
    bool y = false;
    bool l1 = false;  // left bumper
    bool r1 = false;  // right bumper

    // "Misc" buttons — on an Xbox controller, View (the two-panes icon) and
    // Menu (the three-lines icon). Useful for functions that want a button of
    // their own rather than a combo, once the face buttons are spoken for.
    bool select = false;  // View / Back / Share
    bool start = false;   // Menu / Options

    bool dpadUp = false;
    bool dpadDown = false;
    bool dpadLeft = false;
    bool dpadRight = false;
};

namespace GamepadInput {

// Registers Bluepad32 connect/disconnect callbacks and starts scanning.
void begin();

// Call once per loop() iteration. Returns true if Bluepad32 reported fresh
// controller data this call.
bool update();

// Latest known state of the first connected gamepad.
const GamepadState& state();

// Stops or resumes scanning for new controllers.
//
// This matters far more than it sounds. With no controller paired, Bluepad32
// scans continuously, and a scan hops across the band rather than waking on a
// scheduled interval — so it occupies the shared 2.4 GHz radio more of the
// time than an established connection does. WiFi has to fit in the gaps, and
// what breaks first is exactly the traffic that cannot tolerate loss: the
// handful of DHCP packets a client needs before it has an address at all.
//
// Call with false before bringing WiFi up, true after taking it down.
// Controllers already connected stay connected either way.
void setDiscoverable(bool enabled);

}  // namespace GamepadInput
