// Hosts a small WiFi access point + HTTP control panel so lighting patterns,
// custom colors, and brightness can be driven from a phone/laptop browser
// alongside the gamepad. Only talks to LightEffects/LedController — the same
// peer relationship gamepad_input.cpp has to them, so gamepad and web input
// never need to know about each other.
#pragma once

namespace WebUI {

// Registers the HTTP routes. Deliberately does NOT bring up WiFi — the radio
// stays off until start() is called, since an idle soft-AP plus the BT stack
// is the largest single current draw in the system and most sessions never
// need the browser.
void begin();

// Brings up the soft-AP and HTTP server, and logs how to reach it.
void start();

// Stops the server and switches the WiFi radio off.
void stop();

bool isRunning();

// Call every loop() tick to service pending HTTP requests. No-op while the
// server is stopped.
void handleClient();

}  // namespace WebUI
