#include "servo_controller.h"

#include <Adafruit_PWMServoDriver.h>
#include <Arduino.h>
#include <Wire.h>

namespace {

constexpr float kPwmFreqHz = 50.0f;
constexpr uint16_t kDefaultMinUs = 1000;
constexpr uint16_t kDefaultMaxUs = 2000;
constexpr size_t kMaxChannels = 16;  // PCA9685 has 16 PWM outputs

uint16_t sMinPulseUs[kMaxChannels];
uint16_t sMaxPulseUs[kMaxChannels];
bool sRangesInitialized = false;

void ensureDefaultRanges() {
    if (sRangesInitialized) {
        return;
    }
    for (size_t i = 0; i < kMaxChannels; i++) {
        sMinPulseUs[i] = kDefaultMinUs;
        sMaxPulseUs[i] = kDefaultMaxUs;
    }
    sRangesInitialized = true;
}

Adafruit_PWMServoDriver sPwm;

}  // namespace

void ServoController::begin(int sdaGpio, int sclGpio) {
    ensureDefaultRanges();
    Wire.begin(sdaGpio, sclGpio);
    sPwm.begin();
    sPwm.setPWMFreq(kPwmFreqHz);
}

void ServoController::setAngle(uint8_t channel, float angleDegrees) {
    ensureDefaultRanges();
    if (angleDegrees < 0) {
        angleDegrees = 0;
    } else if (angleDegrees > 180) {
        angleDegrees = 180;
    }
    uint16_t minUs = channel < kMaxChannels ? sMinPulseUs[channel] : kDefaultMinUs;
    uint16_t maxUs = channel < kMaxChannels ? sMaxPulseUs[channel] : kDefaultMaxUs;
    uint16_t pulseUs = minUs + static_cast<uint16_t>((maxUs - minUs) * (angleDegrees / 180.0f));
    sPwm.writeMicroseconds(channel, pulseUs);
}

void ServoController::setPulseRange(uint8_t channel, uint16_t minPulseUs, uint16_t maxPulseUs) {
    ensureDefaultRanges();
    if (channel < kMaxChannels) {
        sMinPulseUs[channel] = minPulseUs;
        sMaxPulseUs[channel] = maxPulseUs;
    }
}
