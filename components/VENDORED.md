# Vendored components

Three Adafruit Arduino libraries are checked into this repository as ordinary
source rather than as git submodules. This file records where they came from
and what was changed, because that information is otherwise lost the moment a
nested `.git` directory is removed.

## Why vendored rather than submodules

Each library needs an ESP-IDF `CMakeLists.txt` that upstream either does not
ship or ships in a form that does not work here. A submodule pins an upstream
commit and cannot carry those files, so the build would break on a fresh
clone. The repository also already has one submodule (`components/arduino`)
whose pointer broke when the project directory was renamed — adding three more
would multiply that failure mode for libraries that change rarely and total
under 1 MB.

## Provenance

| Component | Upstream | Version | Commit | License |
| --------- | -------- | ------- | ------ | ------- |
| `Adafruit_BusIO` | [adafruit/Adafruit_BusIO](https://github.com/adafruit/Adafruit_BusIO) | 1.17.4 | `3b8364267c3ee6e16bad91bc2101aefbd5b5915f` | MIT (`LICENSE`) |
| `Adafruit_NeoPixel` | [adafruit/Adafruit_NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel) | 1.15.5 | `d514fc3beae85dd4c2b19781b93faa47bd6e996f` | **LGPL-3.0** (`COPYING`) |
| `Adafruit_PWMServoDriver` | [adafruit/Adafruit-PWM-Servo-Driver-Library](https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library) | 3.0.3 | `a98850b815bf9696c8ffdc2a0f89d657c52dd44b` | BSD-3-Clause (`license.txt`) |

Each library's license file is included as shipped. **They are not all the
same license** — Adafruit_NeoPixel is LGPL-3.0, which carries obligations the
permissive MIT and BSD ones do not.

For LGPL-3.0 specifically: this firmware statically links Adafruit_NeoPixel,
which triggers the relinking requirement in section 4. Distributing the
complete corresponding source — which this repository does, with the library
present and unmodified — satisfies it. Keep it that way: if you ever patch
Adafruit_NeoPixel's *source* (as opposed to adding the `CMakeLists.txt` noted
below), those changes must remain published under LGPL-3.0.

## Local changes

Confined to `CMakeLists.txt` in each. **No library source was modified** — an
update is a clean file replacement that preserves these three files.

### `Adafruit_BusIO/CMakeLists.txt` — modified

Upstream's version declares `REQUIRES arduino-esp32`. In this project the
arduino-esp32 core lives at `components/arduino`, so its ESP-IDF component
name is derived from that directory and is `arduino`. Without the change the
build fails to resolve the dependency.

### `Adafruit_NeoPixel/CMakeLists.txt` — added

Upstream ships no ESP-IDF build file; this one registers the library as a
component requiring `arduino`.

### `Adafruit_PWMServoDriver/CMakeLists.txt` — added

Same as above, and additionally requires `Adafruit_BusIO`, which it uses for
I²C transport.

## Updating a library

1. Clone the upstream repo at the tag you want, somewhere outside this tree.
2. Copy its contents over the component directory, **keeping this project's
   `CMakeLists.txt`**.
3. Delete the `.git` directory from the copy.
4. Update the version and commit in the table above.
5. Rebuild and re-flash before committing — these libraries drive the LEDs and
   servos, so a regression is immediately visible on the hardware.

Nothing here is pinned by `dependencies.lock`; that file covers ESP-IDF
managed components only, which are fetched into the gitignored
`managed_components/`.
