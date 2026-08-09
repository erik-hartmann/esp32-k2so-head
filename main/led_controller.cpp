#include "led_controller.h"

#include <Adafruit_NeoPixel.h>

namespace {

constexpr size_t kMaxStrips = 4;

Adafruit_NeoPixel* sStrips[kMaxStrips] = {};
size_t sStripCount = 0;
uint8_t sGlobalBrightness = 200;
uint8_t sStripDim[kMaxStrips] = {};  // set to 255 per strip in begin()
uint8_t sBlinkLevel = 255;

// Combines the global brightness, strip i's own dim factor, and the global
// blink level into the single value Adafruit_NeoPixel actually applies for
// that strip.
void applyStripBrightness(size_t i) {
    if (i < sStripCount && sStrips[i] != nullptr) {
        uint32_t effective = static_cast<uint32_t>(sGlobalBrightness) * sStripDim[i] * sBlinkLevel / (255 * 255);
        sStrips[i]->setBrightness(static_cast<uint8_t>(effective));
    }
}

// Resolves a flattened 0..totalPixelCount()-1 index into which strip it
// falls on and the pixel index within that strip.
bool resolveGlobalIndex(uint32_t globalIndex, size_t& stripIndexOut, uint16_t& pixelIndexOut) {
    uint32_t remaining = globalIndex;
    for (size_t i = 0; i < sStripCount; i++) {
        uint16_t count = sStrips[i]->numPixels();
        if (remaining < count) {
            stripIndexOut = i;
            pixelIndexOut = static_cast<uint16_t>(remaining);
            return true;
        }
        remaining -= count;
    }
    return false;
}

// Adafruit_NeoPixel's ColorHSV takes a 16-bit hue (0..65535); the rest of
// this codebase works in degrees (0..359) for readability.
uint16_t degreesToAdafruitHue(uint16_t hueDegrees) {
    return static_cast<uint16_t>((static_cast<uint32_t>(hueDegrees) * 65536UL) / 360UL);
}

}  // namespace

void LedController::begin(const int* gpios, const uint32_t* ledCounts, size_t stripCount) {
    if (stripCount > kMaxStrips) {
        stripCount = kMaxStrips;
    }
    sStripCount = stripCount;

    for (size_t i = 0; i < stripCount; i++) {
        sStripDim[i] = 255;
        sStrips[i] = new Adafruit_NeoPixel(static_cast<uint16_t>(ledCounts[i]), gpios[i], NEO_GRB + NEO_KHZ800);
        sStrips[i]->begin();
        applyStripBrightness(i);
    }
}

void LedController::setPixel(size_t stripIndex, uint32_t pixelIndex, uint8_t r, uint8_t g, uint8_t b) {
    if (stripIndex < sStripCount && sStrips[stripIndex] != nullptr && pixelIndex < sStrips[stripIndex]->numPixels()) {
        sStrips[stripIndex]->setPixelColor(pixelIndex, sStrips[stripIndex]->Color(r, g, b));
    }
}

void LedController::fill(size_t stripIndex, uint8_t r, uint8_t g, uint8_t b) {
    if (stripIndex >= sStripCount || sStrips[stripIndex] == nullptr) {
        return;
    }
    uint16_t n = sStrips[stripIndex]->numPixels();
    for (uint16_t i = 0; i < n; i++) {
        setPixel(stripIndex, i, r, g, b);
    }
}

void LedController::fillHSV(size_t stripIndex, uint16_t hue, uint8_t saturation, uint8_t value) {
    if (stripIndex >= sStripCount || sStrips[stripIndex] == nullptr) {
        return;
    }
    uint32_t color = sStrips[stripIndex]->ColorHSV(degreesToAdafruitHue(hue), saturation, value);
    uint16_t n = sStrips[stripIndex]->numPixels();
    for (uint16_t i = 0; i < n; i++) {
        sStrips[stripIndex]->setPixelColor(i, color);
    }
}

void LedController::clear(size_t stripIndex) {
    if (stripIndex < sStripCount && sStrips[stripIndex] != nullptr) {
        sStrips[stripIndex]->clear();
    }
}

void LedController::show(size_t stripIndex) {
    if (stripIndex < sStripCount && sStrips[stripIndex] != nullptr) {
        sStrips[stripIndex]->show();
    }
}

void LedController::fillAllHSV(uint16_t hue, uint8_t saturation, uint8_t value) {
    for (size_t i = 0; i < sStripCount; i++) {
        fillHSV(i, hue, saturation, value);
    }
}

void LedController::clearAll() {
    for (size_t i = 0; i < sStripCount; i++) {
        clear(i);
    }
}

void LedController::showAll() {
    for (size_t i = 0; i < sStripCount; i++) {
        show(i);
    }
}

size_t LedController::totalPixelCount() {
    size_t total = 0;
    for (size_t i = 0; i < sStripCount; i++) {
        if (sStrips[i] != nullptr) {
            total += sStrips[i]->numPixels();
        }
    }
    return total;
}

void LedController::setPixelGlobal(uint32_t globalIndex, uint8_t r, uint8_t g, uint8_t b) {
    size_t stripIndex;
    uint16_t pixelIndex;
    if (resolveGlobalIndex(globalIndex, stripIndex, pixelIndex)) {
        setPixel(stripIndex, pixelIndex, r, g, b);
    }
}

void LedController::setPixelGlobalHSV(uint32_t globalIndex, uint16_t hue, uint8_t saturation, uint8_t value) {
    size_t stripIndex;
    uint16_t pixelIndex;
    if (resolveGlobalIndex(globalIndex, stripIndex, pixelIndex)) {
        uint32_t color = sStrips[stripIndex]->ColorHSV(degreesToAdafruitHue(hue), saturation, value);
        sStrips[stripIndex]->setPixelColor(pixelIndex, color);
    }
}

void LedController::setBrightness(uint8_t brightness) {
    // Adafruit_NeoPixel applies brightness scaling inside setPixelColor()
    // based on whatever was last passed here, so calling this before this
    // frame's setPixel*/fillHSV* calls (as sketch.cpp does) scales the
    // fresh colors correctly rather than re-scaling stale buffer data.
    sGlobalBrightness = brightness;
    for (size_t i = 0; i < sStripCount; i++) {
        applyStripBrightness(i);
    }
}

uint8_t LedController::brightness() {
    return sGlobalBrightness;
}

void LedController::setStripDim(size_t stripIndex, uint8_t dim) {
    if (stripIndex < kMaxStrips) {
        sStripDim[stripIndex] = dim;
        applyStripBrightness(stripIndex);
    }
}

void LedController::setBlinkLevel(uint8_t level) {
    if (level == sBlinkLevel) {
        return;  // blink holds steady most frames; skip the redundant writes
    }
    sBlinkLevel = level;
    for (size_t i = 0; i < sStripCount; i++) {
        applyStripBrightness(i);
    }
}
