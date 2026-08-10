#include "gamepad_input.h"

#include <Bluepad32.h>

namespace {

ControllerPtr sController = nullptr;
GamepadState sState;

void onConnected(ControllerPtr ctl) {
    if (sController == nullptr) {
        Console.printf("Gamepad connected: %s\n", ctl->getModelName());
        sController = ctl;
    }
}

void onDisconnected(ControllerPtr ctl) {
    if (sController == ctl) {
        Console.println("Gamepad disconnected");
        sController = nullptr;
        sState = GamepadState{};
    }
}

}  // namespace

void GamepadInput::begin() {
    BP32.setup(&onConnected, &onDisconnected);
    // Forget prior pairings so this firmware always re-pairs cleanly.
    // Drop this call once you've got a controller you always want to
    // reconnect automatically.
    BP32.forgetBluetoothKeys();
}

bool GamepadInput::update() {
    bool dataUpdated = BP32.update();

    if (sController == nullptr || !sController->isConnected() || !sController->isGamepad()) {
        sState.connected = false;
        return dataUpdated;
    }

    if (!sController->hasData()) {
        return dataUpdated;
    }

    sState.connected = true;
    sState.axisX = sController->axisX();
    sState.axisY = sController->axisY();
    sState.axisRX = sController->axisRX();
    sState.axisRY = sController->axisRY();
    sState.brake = sController->brake();
    sState.throttle = sController->throttle();
    sState.buttons = sController->buttons();
    sState.dpad = sController->dpad();
    sState.a = sController->a();
    sState.b = sController->b();
    sState.x = sController->x();
    sState.y = sController->y();
    sState.l1 = sController->l1();
    sState.r1 = sController->r1();
    sState.select = sController->miscSelect();
    sState.start = sController->miscStart();

    // Decoded here so Bluepad32's DPAD_* bit constants stay inside this file.
    uint8_t dpad = sController->dpad();
    sState.dpadUp = (dpad & DPAD_UP) != 0;
    sState.dpadDown = (dpad & DPAD_DOWN) != 0;
    sState.dpadLeft = (dpad & DPAD_LEFT) != 0;
    sState.dpadRight = (dpad & DPAD_RIGHT) != 0;

    return dataUpdated;
}

const GamepadState& GamepadInput::state() {
    return sState;
}
