#include "light_effects.h"

#include <Arduino.h>
#include <Bluepad32.h>

#include <cmath>

#include "led_controller.h"

namespace {

enum class Effect : uint8_t {
    Solid = 0,
    SolidRainbow,
    RainbowCycle,
    ChasingRainbow,
    TheaterChase,
    Chase,
    Confetti,
    Twinkle,
    Breathe,
    ColorWipe,
    Bounce,
    Fire,
    Strobe,
    kCount,
};

constexpr const char* kEffectNames[] = {
    "Solid",     "Solid Rainbow", "Rainbow Cycle", "Chasing Rainbow", "Theater Chase", "Chase", "Confetti",
    "Twinkle",   "Breathe",       "Color Wipe",    "Bounce",          "Fire",          "Strobe",
};
static_assert(sizeof(kEffectNames) / sizeof(kEffectNames[0]) == static_cast<size_t>(Effect::kCount),
              "kEffectNames must have one entry per Effect");

constexpr size_t kMaxPixels = 64;

// Default starting pattern: solid cyan. Doesn't need a gamepad connected
// or a color picked, so it looks right immediately on boot whether you
// arrive via controller or the web UI.
Effect sEffect = Effect::Solid;
uint32_t sEffectStartMs = 0;

// Y cycles through these; whichever one it lands on becomes both the
// immediate accent color and the "home" color used whenever the stick is
// back at center (see updateAccentFromGamepad's deadzone check).
constexpr uint8_t kRainbowPalette[][3] = {
    {255, 0, 0},    // red
    {255, 128, 0},  // orange
    {255, 255, 0},  // yellow
    {0, 255, 0},    // green
    {0, 255, 255},  // cyan
    {0, 0, 255},    // blue
    {160, 0, 255},  // violet
};
constexpr size_t kPaletteSize = sizeof(kRainbowPalette) / sizeof(kRainbowPalette[0]);
constexpr size_t kDefaultPaletteIndex = 4;  // cyan, to match the default Solid pattern
constexpr int32_t kStickCenterDeadzone = 60;  // out of -511..512

size_t sPaletteIndex = kDefaultPaletteIndex;

// Shared color every non-rainbow pattern below renders with. Settable live
// from the web UI's color picker, or continuously from the gamepad's left
// stick (see updateAccentFromGamepad) — either input updates the same
// state, regardless of which pattern is currently selected. Starts on the
// same cyan as the default pattern/palette entry above.
uint8_t sAccentR = kRainbowPalette[kDefaultPaletteIndex][0];
uint8_t sAccentG = kRainbowPalette[kDefaultPaletteIndex][1];
uint8_t sAccentB = kRainbowPalette[kDefaultPaletteIndex][2];
uint8_t sDefaultAccentR = sAccentR;
uint8_t sDefaultAccentG = sAccentG;
uint8_t sDefaultAccentB = sAccentB;

// Per-pixel scratch state, reused (and reinterpreted) by whichever effect is
// active. Cleared whenever the effect changes.
uint8_t sScratch[kMaxPixels] = {};

size_t pixelCount() {
    size_t n = LedController::totalPixelCount();
    return n < kMaxPixels ? n : kMaxPixels;
}

void resetEffectState() {
    sEffectStartMs = millis();
    for (size_t i = 0; i < kMaxPixels; i++) {
        sScratch[i] = 0;
    }
}

uint32_t elapsedMs() {
    return millis() - sEffectStartMs;
}

// Standard HSV(hue, 255, 255) -> RGB, used only to turn the gamepad's
// stick-selected hue into an RGB accent color.
void hueToRGB(uint16_t hueDegrees, uint8_t& r, uint8_t& g, uint8_t& b) {
    uint8_t region = static_cast<uint8_t>(hueDegrees / 60);
    uint8_t remainder = static_cast<uint8_t>((hueDegrees % 60) * 255 / 60);
    uint8_t q = static_cast<uint8_t>(255 - remainder);
    uint8_t t = remainder;
    switch (region % 6) {
        case 0:
            r = 255;
            g = t;
            b = 0;
            break;
        case 1:
            r = q;
            g = 255;
            b = 0;
            break;
        case 2:
            r = 0;
            g = 255;
            b = t;
            break;
        case 3:
            r = 0;
            g = q;
            b = 255;
            break;
        case 4:
            r = t;
            g = 0;
            b = 255;
            break;
        default:
            r = 255;
            g = 0;
            b = q;
            break;
    }
}

// Runs every frame regardless of which pattern is selected — same idea as
// the web UI's color picker, just fed by the left stick instead. Doesn't
// log (unlike the public setAccentColor()) since this fires ~30 times/sec
// whenever a gamepad is connected.
//
// Within the center deadzone, holds at the Y-selected default color instead
// of tracking raw stick position — otherwise a merely-resting stick would
// constantly fight the web UI's color picker back to whatever hue center
// happens to map to.
void updateAccentFromGamepad(const GamepadState& gamepad) {
    if (!gamepad.connected) {
        return;
    }
    if (gamepad.axisX > -kStickCenterDeadzone && gamepad.axisX < kStickCenterDeadzone) {
        sAccentR = sDefaultAccentR;
        sAccentG = sDefaultAccentG;
        sAccentB = sDefaultAccentB;
        return;
    }
    uint16_t hue = static_cast<uint16_t>(map(gamepad.axisX, -511, 512, 0, 359));
    hueToRGB(hue, sAccentR, sAccentG, sAccentB);
}

void runSolid() {
    size_t n = pixelCount();
    for (size_t i = 0; i < n; i++) {
        LedController::setPixelGlobal(i, sAccentR, sAccentG, sAccentB);
    }
}

void runSolidRainbow() {
    size_t n = pixelCount();
    for (size_t i = 0; i < n; i++) {
        uint16_t hue = n > 1 ? static_cast<uint16_t>((360 * i) / n) : 0;
        LedController::setPixelGlobalHSV(i, hue, 255, 200);
    }
}

void runRainbowCycle() {
    size_t n = pixelCount();
    uint16_t offset = static_cast<uint16_t>((elapsedMs() / 20) % 360);
    for (size_t i = 0; i < n; i++) {
        uint16_t hue = static_cast<uint16_t>((offset + (n > 1 ? (360 * i) / n : 0)) % 360);
        LedController::setPixelGlobalHSV(i, hue, 255, 200);
    }
}

void runChasingRainbow() {
    size_t n = pixelCount();
    if (n == 0) {
        return;
    }
    uint32_t pos = (elapsedMs() / 120) % n;
    uint16_t hue = static_cast<uint16_t>((elapsedMs() / 15) % 360);
    for (size_t i = 0; i < n; i++) {
        if (i == pos) {
            LedController::setPixelGlobalHSV(i, hue, 255, 220);
        } else {
            LedController::setPixelGlobal(i, 0, 0, 0);
        }
    }
}

void runTheaterChase() {
    size_t n = pixelCount();
    uint32_t phase = (elapsedMs() / 150) % 3;
    for (size_t i = 0; i < n; i++) {
        if ((i % 3) == phase) {
            LedController::setPixelGlobal(i, sAccentR, sAccentG, sAccentB);
        } else {
            LedController::setPixelGlobal(i, 0, 0, 0);
        }
    }
}

void runChase() {
    size_t n = pixelCount();
    if (n == 0) {
        return;
    }
    uint32_t pos = (elapsedMs() / 120) % n;
    for (size_t i = 0; i < n; i++) {
        if (i == pos) {
            LedController::setPixelGlobal(i, sAccentR, sAccentG, sAccentB);
        } else {
            LedController::setPixelGlobal(i, 0, 0, 0);
        }
    }
}

void runConfetti() {
    size_t n = pixelCount();
    // Fade every pixel a little each frame, then occasionally spark a
    // random one back up to full brightness in the accent color.
    for (size_t i = 0; i < n; i++) {
        uint8_t v = sScratch[i];
        v = (v > 8) ? (v - 8) : 0;
        sScratch[i] = v;
        LedController::setPixelGlobal(i, static_cast<uint8_t>(sAccentR * v / 255), static_cast<uint8_t>(sAccentG * v / 255),
                                       static_cast<uint8_t>(sAccentB * v / 255));
    }
    if (n > 0 && random(4) == 0) {
        sScratch[random(n)] = 255;
    }
}

void runTwinkle() {
    size_t n = pixelCount();
    for (size_t i = 0; i < n; i++) {
        uint8_t v = sScratch[i];
        v = (v > 15) ? (v - 15) : 0;
        sScratch[i] = v;
        LedController::setPixelGlobal(i, static_cast<uint8_t>(sAccentR * v / 255), static_cast<uint8_t>(sAccentG * v / 255),
                                       static_cast<uint8_t>(sAccentB * v / 255));
    }
    if (n > 0 && random(6) == 0) {
        sScratch[random(n)] = 255;
    }
}

void runBreathe() {
    size_t n = pixelCount();
    float phase = (elapsedMs() % 3000) / 3000.0f;
    float wave = (sinf(phase * 2.0f * static_cast<float>(PI)) + 1.0f) / 2.0f;  // 0..1
    uint8_t v = static_cast<uint8_t>(wave * 255);
    uint8_t r = static_cast<uint8_t>(sAccentR * v / 255);
    uint8_t g = static_cast<uint8_t>(sAccentG * v / 255);
    uint8_t b = static_cast<uint8_t>(sAccentB * v / 255);
    for (size_t i = 0; i < n; i++) {
        LedController::setPixelGlobal(i, r, g, b);
    }
}

void runColorWipe() {
    size_t n = pixelCount();
    if (n == 0) {
        return;
    }
    constexpr uint32_t kMsPerPixel = 400;
    uint32_t cycleMs = kMsPerPixel * n * 2;  // wipe on, then wipe off
    uint32_t withinCycle = elapsedMs() % cycleMs;
    bool wipingIn = withinCycle < kMsPerPixel * n;
    uint32_t progress = wipingIn ? withinCycle / kMsPerPixel : (withinCycle - kMsPerPixel * n) / kMsPerPixel;
    for (size_t i = 0; i < n; i++) {
        bool lit = wipingIn ? (i <= progress) : (i > progress);
        if (lit) {
            LedController::setPixelGlobal(i, sAccentR, sAccentG, sAccentB);
        } else {
            LedController::setPixelGlobal(i, 0, 0, 0);
        }
    }
}

void runBounce() {
    size_t n = pixelCount();
    if (n == 0) {
        return;
    }
    if (n == 1) {
        LedController::setPixelGlobal(0, sAccentR, sAccentG, sAccentB);
        return;
    }
    uint32_t span = 2 * (n - 1);
    uint32_t pos = (elapsedMs() / 120) % span;
    uint32_t idx = (pos < n) ? pos : (span - pos);
    for (size_t i = 0; i < n; i++) {
        if (i == idx) {
            LedController::setPixelGlobal(i, sAccentR, sAccentG, sAccentB);
        } else {
            LedController::setPixelGlobal(i, 0, 0, 0);
        }
    }
}

void runFire() {
    size_t n = pixelCount();
    for (size_t i = 0; i < n; i++) {
        int heat = 160 + random(96);  // 160..255-ish flicker intensity
        if (heat > 255) {
            heat = 255;
        }
        uint8_t r = static_cast<uint8_t>(sAccentR * heat / 255);
        uint8_t g = static_cast<uint8_t>(sAccentG * heat / 255);
        uint8_t b = static_cast<uint8_t>(sAccentB * heat / 255);
        LedController::setPixelGlobal(i, r, g, b);
    }
}

void runStrobe() {
    size_t n = pixelCount();
    bool on = ((elapsedMs() / 80) % 2) == 0;
    for (size_t i = 0; i < n; i++) {
        if (on) {
            LedController::setPixelGlobal(i, sAccentR, sAccentG, sAccentB);
        } else {
            LedController::setPixelGlobal(i, 0, 0, 0);
        }
    }
}

}  // namespace

void LightEffects::begin() {
    resetEffectState();
}

void LightEffects::update(const GamepadState& gamepad) {
    updateAccentFromGamepad(gamepad);

    switch (sEffect) {
        case Effect::Solid:
            runSolid();
            break;
        case Effect::SolidRainbow:
            runSolidRainbow();
            break;
        case Effect::RainbowCycle:
            runRainbowCycle();
            break;
        case Effect::ChasingRainbow:
            runChasingRainbow();
            break;
        case Effect::TheaterChase:
            runTheaterChase();
            break;
        case Effect::Chase:
            runChase();
            break;
        case Effect::Confetti:
            runConfetti();
            break;
        case Effect::Twinkle:
            runTwinkle();
            break;
        case Effect::Breathe:
            runBreathe();
            break;
        case Effect::ColorWipe:
            runColorWipe();
            break;
        case Effect::Bounce:
            runBounce();
            break;
        case Effect::Fire:
            runFire();
            break;
        case Effect::Strobe:
            runStrobe();
            break;
        default:
            break;
    }
}

void LightEffects::next() {
    auto count = static_cast<uint8_t>(Effect::kCount);
    sEffect = static_cast<Effect>((static_cast<uint8_t>(sEffect) + 1) % count);
    resetEffectState();
    Console.printf("Light effect: %s\n", currentName());
}

void LightEffects::previous() {
    auto count = static_cast<uint8_t>(Effect::kCount);
    sEffect = static_cast<Effect>((static_cast<uint8_t>(sEffect) + count - 1) % count);
    resetEffectState();
    Console.printf("Light effect: %s\n", currentName());
}

void LightEffects::selectEffect(uint8_t index) {
    if (index >= static_cast<uint8_t>(Effect::kCount)) {
        return;
    }
    sEffect = static_cast<Effect>(index);
    resetEffectState();
    Console.printf("Light effect: %s\n", currentName());
}

void LightEffects::setAccentColor(uint8_t r, uint8_t g, uint8_t b) {
    sAccentR = r;
    sAccentG = g;
    sAccentB = b;
    Console.printf("Accent color: (%u,%u,%u)\n", r, g, b);
}

void LightEffects::cycleDefaultAccentColor() {
    sPaletteIndex = (sPaletteIndex + 1) % kPaletteSize;
    sDefaultAccentR = kRainbowPalette[sPaletteIndex][0];
    sDefaultAccentG = kRainbowPalette[sPaletteIndex][1];
    sDefaultAccentB = kRainbowPalette[sPaletteIndex][2];
    // Apply immediately so it's visible even while the stick is off-center.
    sAccentR = sDefaultAccentR;
    sAccentG = sDefaultAccentG;
    sAccentB = sDefaultAccentB;
    Console.printf("Default accent color: (%u,%u,%u)\n", sDefaultAccentR, sDefaultAccentG, sDefaultAccentB);
}

void LightEffects::setDefaultAccentToCurrent() {
    // Deliberately not persisted — a power cycle should always come back up
    // on the built-in cyan, so a session's experimenting can't strand the rig
    // on some color you can't remember picking.
    sDefaultAccentR = sAccentR;
    sDefaultAccentG = sAccentG;
    sDefaultAccentB = sAccentB;
    Console.printf("Default accent pinned to current: (%u,%u,%u)\n", sDefaultAccentR, sDefaultAccentG,
                    sDefaultAccentB);
}

const char* LightEffects::currentName() {
    return kEffectNames[static_cast<uint8_t>(sEffect)];
}

size_t LightEffects::effectCount() {
    return static_cast<size_t>(Effect::kCount);
}

const char* LightEffects::effectName(uint8_t index) {
    if (index >= static_cast<uint8_t>(Effect::kCount)) {
        return "";
    }
    return kEffectNames[index];
}
