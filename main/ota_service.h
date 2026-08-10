// Over-the-air firmware updates, so a head that is already assembled and
// mounted does not have to be opened up for a USB cable.
//
// Deliberately tied to the soft-AP rather than joining a home network. The
// access point only exists while it has been switched on at the board's admin
// button, which means the update path is physically gated: someone has to be
// standing at the head to open it. It also keeps WiFi credentials out of a
// public repository entirely.
//
// The security boundary is therefore the WPA2 access point plus that button.
// There is no separate OTA password — adding one would mean committing it to
// this repository next to the AP password, which buys very little. If this
// ever runs on a network you do not control, add ArduinoOTA.setPassword() in
// the .cpp and pass --auth to the uploader.
#pragma once

namespace OtaService {

// Starts the OTA listener. Call only once WiFi is actually up — it binds a
// socket, and there is nothing to bind to before then.
void begin();

void stop();

bool isRunning();

// Call every loop() while running. Cheap until an upload actually arrives,
// at which point it blocks until the transfer finishes and the board reboots.
void handle();

}  // namespace OtaService
