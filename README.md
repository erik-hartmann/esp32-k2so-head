# esp32-k2so-head

An animatronic K-2SO head driven by an ESP32 and flown with an Xbox Wireless
Controller over Bluetooth.

Two WS2812 rings light the eyes, two servos on a PCA9685 pan and tilt them
through linkages, and a DFPlayer Mini handles audio. With no controller
connected the head runs autonomous idle behavior — it looks around, holds the
occasional stare, and blinks.

Built for someone doing this for the first time. The wiring is documented in
two stages, breadboard then soldered board, and the mistakes that cost real
debugging time are written down rather than quietly fixed.

## What it does

- **13 lighting patterns** — rainbow, chase, confetti, twinkle, fire, strobe
  and others, all across the full 7 pixels of each ring
- **Live accent color** steered by the left stick, on every pattern rather
  than a dedicated "reactive" one
- **Eased eye motion** from the right stick, with autonomous idle when the
  controller is away: weighted random saccades, occasional long stares, and
  natural blinking
- **Per-side dimming** on the triggers, so each eye responds independently
- **A web UI** over a soft AP for patterns, color and brightness — off by
  default, toggled from the gamepad
- **Audio** from a DFPlayer Mini, driven by a non-blocking driver that never
  stalls the control loop

## Controls

| Input | Action |
| ----- | ------ |
| Left stick | Accent color hue (centred = the palette default) |
| Right stick | Eye pan and tilt |
| LT / RT | Dim the left / right eye independently |
| L1 / R1 | Previous / next lighting pattern |
| A / B | Brightness down / up, while held |
| X | Lights off and on, remembering brightness; also parks idle motion |
| Y | Step the default accent color through the palette |
| Y, double-clicked | Pin the current color as the default, until reboot |
| D-pad Up | Toggle the WiFi AP and web UI |
| D-pad Down | Play the next audio clip |

The controller must be a **Bluetooth** Xbox model (2016 or later). The 2013
controller uses a proprietary radio and will not pair.

## Hardware

- **[docs/bom.md](docs/bom.md)** — every part, with the spec that actually
  matters and a power budget explaining the supply sizing
- **[docs/wiring.md](docs/wiring.md)** — the build in two parts: a breadboard
  prototype to prove the firmware against real hardware, then the move to a
  soldered perfboard laid out as a proper power junction

Start with the breadboard. The point of that stage isn't the circuit, it's
proving the firmware before anything becomes permanent.

## Building

Requires [PlatformIO](https://platformio.org/). ESP-IDF 5.4.2 and the Arduino
core are pulled in automatically.

```bash
git clone --recursive https://github.com/erik-hartmann/esp32-k2so-head.git
```

`--recursive` matters — `components/arduino` is a submodule. If you forget it,
run `git submodule update --init` afterwards.

Build, flash and monitor:

```bash
python -m platformio run -e esp32-devkitc -t upload
```

```bash
python -m platformio device monitor -e esp32-devkitc
```

Close the serial monitor before flashing, or the port will be busy. And use a
**data** USB cable — a charge-only cable powers the board and looks entirely
normal but never enumerates a COM port.

### On Windows: set a UTF-8 stdio encoding

```bash
PYTHONIOENCODING=utf-8 python -m platformio run -e esp32-devkitc -t upload
```

Without it, the upload can **hang indefinitely** partway through. esptool draws
its progress bar with `█` and `░`, which the default cp1252 console encoding
cannot represent. PlatformIO's output-reader thread dies on the
`UnicodeEncodeError`, esptool's stdout pipe fills with nobody draining it, and
esptool blocks forever writing into it.

It looks exactly like a hardware fault — no output, no error, no progress, and
the process sitting at near-zero CPU — which sends you hunting for a bad cable
or a board that will not enter bootloader mode. It is neither. If a flash sits
still for more than a couple of minutes, this is the first thing to check.

## Supported boards

| Environment | Board |
| ----------- | ----- |
| `esp32-devkitc` *(default)* | Espressif ESP32-DevKitC — the reference build |
| `esp32dev` | ELEGOO EL-SM-012 |
| `esp32-s3-devkitc-1` | ESP32-S3-DevKitC-1 |
| `esp32-c3-devkitc-02` | ESP32-C3-DevKitC-02 |

The S3 and C3 configs are untested on real hardware. The C6 and H2
environments exist but deliberately have no board config, so they fail the
build with a clear error rather than silently using wrong pins.

## How it's organised

Every hardware specific — pin numbers, LED counts, which peripherals exist —
lives in one header under `main/board_configs/`, selected per PlatformIO
environment. No logic file hardcodes a pin, so **supporting a new board is one
header and one `[env:...]` block**, not edits scattered across the tree.

| File | Responsibility |
| ---- | -------------- |
| `board_config.h` + `board_configs/` | All hardware specifics; fails loudly if a config is incomplete |
| `gamepad_input.*` | The only file that touches Bluepad32 |
| `led_controller.*` | WS2812 strips, flattened pixel indexing, per-strip dimming |
| `light_effects.*` | The 13 patterns and the shared accent color |
| `eye_motion.*` | Servo easing and autonomous idle behavior |
| `servo_controller.*` | PCA9685 wrapper with per-channel pulse ranges |
| `audio_player.*` | Non-blocking DFPlayer Mini driver |
| `web_ui.*` | Soft AP and HTTP control panel |
| `sketch.cpp` | Control mapping — where inputs meet behavior |

Gamepad and web input are peers: both call into the same subsystems, so either
works with or without the other.

### Notes for anyone extending it

The firmware does not fit the default 1 MB app partition; `partitions.csv`
gives it 3 MB. Setting this through `board_build.partitions` in
`platformio.ini` works, while `CONFIG_PARTITION_TABLE_CUSTOM` in
`sdkconfig.defaults` is silently ignored in this layout.

The soft AP starts **off**. Running it at boot pushed the board into brownout
over USB. If you make it always-on, redo the power budget in
[docs/bom.md](docs/bom.md).

`main/web_ui.cpp` hardcodes the AP credentials. They are a demo default meant
to be changed — the AP has no internet access and only controls lights and
servos, but anyone in radio range can read them here.

## Attribution

This project is a thin layer over a great deal of other people's work — the
Bluetooth stack, the Arduino compatibility layer, the timing-critical LED
driver. It began as a clone of Ricardo Quesada's
[esp-idf-arduino-bluepad32-template](https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template)
and is built on [Bluepad32](https://github.com/ricardoquesada/bluepad32),
[BTstack](https://github.com/bluekitchen/btstack), ESP-IDF, arduino-esp32 and
three Adafruit libraries.

Full credits in **[docs/attribution.md](docs/attribution.md)**.

## License

Project code is Apache-2.0. Bundled components carry their own licenses — see
[LICENSE](LICENSE) for the full inventory and
[components/VENDORED.md](components/VENDORED.md) for exact vendored versions.

Two worth knowing up front: **BTstack is free for non-commercial use only**,
and commercial use requires a license from BlueKitchen. **Adafruit_NeoPixel is
LGPL-3.0** and statically linked, so its source must stay published.
