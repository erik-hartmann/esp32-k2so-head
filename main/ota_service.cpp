#include "ota_service.h"

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Bluepad32.h>

#include "led_controller.h"

namespace {

constexpr const char* kHostname = "k2so-head";

bool sRunning = false;

}  // namespace

void OtaService::begin() {
    if (sRunning) {
        return;
    }

    ArduinoOTA.setHostname(kHostname);

    ArduinoOTA.onStart([]() {
        // Park the head before the transfer. loop() stops running for several
        // seconds during an update, so anything left mid-command stays that
        // way — and more usefully, dropping the LEDs takes a few hundred mA
        // off the rail at exactly the moment the radio and the flash writer
        // are both busy.
        LedController::setBrightness(0);
        LedController::clearAll();
        LedController::showAll();
        Console.println("OTA: update starting, lights parked");
    });

    ArduinoOTA.onProgress([](unsigned int done, unsigned int total) {
        // Coarse on purpose: this runs inside the transfer, and logging every
        // packet over the same link would slow the thing it is reporting on.
        static unsigned int lastPct = 0;
        unsigned int pct = total ? (done * 100 / total) : 0;
        if (pct >= lastPct + 10 || pct == 100) {
            Console.printf("OTA: %u%%\n", pct);
            lastPct = pct;
        }
    });

    ArduinoOTA.onEnd([]() { Console.println("OTA: complete, rebooting into the new image"); });

    ArduinoOTA.onError([](ota_error_t error) {
        Console.printf("OTA: failed (error %u) — the running image is untouched\n",
                       static_cast<unsigned>(error));
    });

    ArduinoOTA.begin();
    sRunning = true;
    Console.printf("OTA: listening as \"%s\" on 192.168.4.1:3232\n", kHostname);
}

void OtaService::stop() {
    if (!sRunning) {
        return;
    }
    ArduinoOTA.end();
    sRunning = false;
    Console.println("OTA: stopped");
}

bool OtaService::isRunning() {
    return sRunning;
}

void OtaService::handle() {
    if (sRunning) {
        ArduinoOTA.handle();
    }
}
