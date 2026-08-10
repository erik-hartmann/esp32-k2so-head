# Attribution

This project is a thin layer of application code on top of a lot of other
people's work. Nearly everything difficult here — the Bluetooth stack, the
Arduino compatibility layer, the timing-critical LED driver — comes from the
projects below.

Licenses are summarised in [../LICENSE](../LICENSE). Exact vendored versions
are in [../components/VENDORED.md](../components/VENDORED.md).

## The foundation

**[esp-idf-arduino-bluepad32-template](https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template)**
— Ricardo Quesada. This repository began as a clone of that template, which
is what makes ESP-IDF, the Arduino core and Bluepad32 build together as one
project with a working `sdkconfig`. Getting that combination to link is the
hard part, and it was already solved. *Apache-2.0.*

**[Bluepad32](https://github.com/ricardoquesada/bluepad32)** — Ricardo
Quesada. The Bluetooth gamepad host. It handles pairing, the HID report
parsing, and the per-controller quirks that make an Xbox controller present as
clean axis and button values. Everything in `main/gamepad_input.cpp` is a thin
adapter over it. *Apache-2.0, and LGPL-2.1-or-later for `bluepad32_arduino`.*

**[BTstack](https://github.com/bluekitchen/btstack)** — BlueKitchen GmbH. The
Bluetooth stack underneath Bluepad32.
**Note its licensing:** BTstack is free for non-commercial use, but commercial
use requires a license from BlueKitchen. Anyone selling something based on
this project needs to deal with that.

**[ESP-IDF](https://github.com/espressif/esp-idf)** and
**[arduino-esp32](https://github.com/espressif/arduino-esp32)** — Espressif.
The SDK and the Arduino compatibility core, the latter included as a git
submodule at `components/arduino`. *Apache-2.0 and LGPL-2.1-or-later
respectively.*

## Libraries driving the hardware

**[Adafruit_NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)** —
Adafruit Industries. Drives the WS2812 rings. WS2812 timing is strict enough
that a naive bit-banged driver fails intermittently in ways that look like
wiring faults; this library gets it right, using the ESP32's RMT peripheral.
An earlier attempt on this project using a different driver produced the
classic symptom of only the first pixel responding. *LGPL-3.0.*

**[Adafruit_PWMServoDriver](https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library)**
— Adafruit Industries. PCA9685 control for the eye servos. *BSD-3-Clause.*

**[Adafruit_BusIO](https://github.com/adafruit/Adafruit_BusIO)** — Adafruit
Industries. I²C/SPI transport abstraction that PWMServoDriver depends on.
*MIT.*

## Hardware documentation

**[DFRobot](https://wiki.dfrobot.com/DFPlayer_Mini_SKU_DFR0299)** — the
DFPlayer Mini wiki, source of the serial command protocol implemented in
`main/audio_player.cpp` and of the widely-copied 1 kΩ series resistor
recommendation discussed in [wiring.md](wiring.md).

## What is original here

For clarity about what this repository actually adds, everything in `main/`
except `main.c` is project code: the board-config abstraction, the LED
controller and 13 lighting patterns, the eased servo motion with autonomous
idle behavior, the non-blocking DFPlayer driver, the soft-AP web UI, and the
control mapping. Plus the hardware design in `docs/`.

That is a small fraction of what gets flashed onto the chip.
