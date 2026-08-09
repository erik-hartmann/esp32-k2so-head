// Thin wrapper around Adafruit_PWMServoDriver (PCA9685, the common
// 16-channel I2C servo breakout).
#pragma once

#include <cstdint>

namespace ServoController {

// Starts I2C on the given pins and configures the PCA9685 for 50Hz
// (standard hobby servo PWM rate).
void begin(int sdaGpio, int sclGpio);

// Points channel (0..15) at angleDegrees (0..180, clamped), mapped to that
// channel's pulse-width range (1000..2000us by default — see
// setPulseRange) — the conservative middle of the common 500..2500us hobby
// servo range, safe for most servos without knowing the exact model.
void setAngle(uint8_t channel, float angleDegrees);

// Overrides the pulse-width range angleDegrees (0..180) maps to for one
// specific channel, so e.g. a tilt servo can have more/less mechanical
// throw than a pan servo without affecting it. Defaults to 1000..2000us
// for every channel until this is called.
void setPulseRange(uint8_t channel, uint16_t minPulseUs, uint16_t maxPulseUs);

}  // namespace ServoController
