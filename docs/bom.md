# Bill of Materials

Everything used to build the K-2SO head. See [wiring.md](wiring.md) for how it
goes together — Part 1 is the no-solder breadboard prototype, Part 2 is the
permanent board.

Links are the exact parts used on this build. They are not endorsements and
Amazon listings rot; treat the **spec** column as the real requirement and the
link as a known-good example.

Rows marked *generic part* are commodity items where any equivalent works and
a specific listing would add nothing.

Prices intentionally omitted — they move too fast to keep accurate.

---

## Controller and compute

| Qty | Part | Spec / why this one | Link |
| --- | ---- | ------------------- | ---- |
| 1 | **ESP32-DevKitC** (ESP32-WROOM-32) | The active board. Dual-core, and Bluetooth **Classic** is required — Bluepad32 talks to the Xbox controller over BT Classic HID, not BLE. Must be WROOM, not WROVER. | [B09MQJWQN2](https://www.amazon.com/dp/B09MQJWQN2) |
| 1 | **Screw terminal breakout module for ESP32-DevKitC** | Turns the DevKitC's pins into tool-free screw terminals, and lines up with the pre-drilled mounting holes in the head's base plate. **Buy this and the dev board as a pair** — see the compatibility warning below. | [B08LGFRT87](https://www.amazon.com/dp/B08LGFRT87) |
| 1 | **Xbox Wireless Controller** | Must be a **Bluetooth** model (2016 or later — the one with plastic, not glossy, around the Guide button). The 2013 controller uses a proprietary radio and will not pair. | *any Bluetooth model* |
| — | ~~Elegoo ESP32-S~~ | **Do not buy for this build.** The project started on one, but its pin ordering does not match the breakout module above, so it will not seat. The chip is the same and the firmware still supports it (`main/board_configs/elegoo_esp32_wroom32.h`) — the problem is purely mechanical. | [B0D8T53CQ5](https://www.amazon.com/dp/B0D8T53CQ5) |

> **"ESP32 dev board" is not a standard footprint.** Pin ordering varies
> between manufacturers even for the same module. Check your dev board and
> breakout against each other before ordering, or buy them together.

> **WROOM vs WROVER matters too.** GPIO16/17, used for the DFPlayer, are wired
> to PSRAM on a WROVER. On a WROVER, move audio to GPIO32/33 in the board
> config.

## Output devices

| Qty | Part | Spec / why this one | Link |
| --- | ---- | ------------------- | ---- |
| 2 | **WS2812 7-LED ring** (4-pin: 5V, GND, DIN, DOUT) | The eyes. Sold as "7-bit RGB LED ring". 7 addressable pixels each, so `LED_STRIP_LED_COUNTS` is `{ 7, 7 }`. Separate DIN/DOUT — the two rings are **not** daisy-chained, each gets its own GPIO. | [B0C77TVKL6](https://www.amazon.com/dp/B0C77TVKL6) |
| 1 | **PCA9685** 16-channel 12-bit I²C PWM driver | Drives the eye servos. Only 2 of 16 channels used, leaving 14 for a jaw or head pan later. Has **two** power inputs — `V+` (servo, 5 V from the rails) and `VCC` (logic, 3.3 V from the ESP32). | [B0DC2XSKF9](https://www.amazon.com/dp/B0DC2XSKF9) |
| 2 | **MG90S servo** (metal gear) | Eye pan and tilt, via linkages. Metal gears matter here — the linkages backdrive the servo every time the head is handled. Tilt runs a widened pulse range (972–2056 µs) set in `sketch.cpp`. | [B0BWJ4RKGV](https://www.amazon.com/dp/B0BWJ4RKGV) |
| 1 | **DFPlayer Mini** MP3 module | Audio. Serial-controlled at 9600 baud, with a built-in amp and microSD slot. The variant received was an HW-247A clone; see the orientation and speaker-pin notes in [wiring.md](wiring.md), which differ from DFRobot's original. | [B0F1F711ZH](https://www.amazon.com/dp/B0F1F711ZH) |
| 1 | **Speaker** for the DFPlayer | Drives directly from the DFPlayer's onboard amp — no separate amplifier needed. | [B0BWYBFPW8](https://www.amazon.com/dp/B0BWYBFPW8) |
| 1 | **microSD card, under 32 GB** | **Must be FAT32, not exFAT.** The DFPlayer reads neither exFAT nor an oversized card, and fails identically to a wiring fault: silence, no error, nothing to distinguish it from a bad solder joint. A 16 GB card ships FAT32 from the factory and needs no formatting — check before reformatting anything. Files go in `/mp3/` as `0001.mp3`, `0002.mp3`, …, with `AUDIO_TRACK_COUNT` set to match. | *generic part* |

## Passives

| Qty | Part | Spec / why this one | Link |
| --- | ---- | ------------------- | ---- |
| 3 | **330 Ω resistor**, ¼ W | The only resistor value on the board — two on the LED data lines, one on the DFPlayer RX line. Buy an assortment rather than exactly three. | *generic part* |
| 1–2 | **1000 µF electrolytic capacitor**, 10 V or higher | Bulk reservoir across the 5 V rails. Two on the breadboard, one on the PCB. Polarised — the striped leg is negative. | *generic part* |

## Boards and connectors

| Qty | Part | Spec / why this one | Link |
| --- | ---- | ------------------- | ---- |
| 1 | **Quarter-size solderable perfboard** — 15 rows × columns A–J, with a `+`/`−` rail pair on **both** edges | The Part 2 interface board. The dual rail pairs are what make the star-power layout work; a board with rails on only one side forces a very different layout. 2.54 mm pitch. | [B09ZPGJ58F](https://www.amazon.com/dp/B09ZPGJ58F) |
| 8 | **2-pin screw terminal block, 2.54 mm pitch** | Five on the rails (power in, ESP32, PCA9685, AUX) and three at the GPIO landings. **The 2.54 mm pitch is essential** — it drops straight onto a rail pair. The common 5.08 mm blocks will not fit. Each body is 2 pitches wide × 3 rows long. | [B0FHGXX6SK](https://www.amazon.com/dp/B0FHGXX6SK) |
| 3 | **JST-XH 2.5 mm 3-pin connector pairs** | Keyed cable connectors for the two LED rings and the DFPlayer. This kit includes crimped pigtails, so no crimp tool is needed. | [B073SNPF2C](https://www.amazon.com/dp/B073SNPF2C) |
| 1 | **Breadboard**, 30 rows × 5 columns + rail pair | Part 1 only. Any standard size works. | *generic part* |
| — | ~~1-pin screw terminal block, 2.54 mm~~ | **Not needed — don't hunt for these.** They're hard to source, and the GPIO landings work fine with the 2-pin blocks above, mounted vertically and wired on one side only. See [wiring.md](wiring.md) for placement and the two clearance checks. | — |

## Power and cabling

| Qty | Part | Spec / why this one | Link |
| --- | ---- | ------------------- | ---- |
| 1 | **5 V 6 A 30 W power supply** — BTF-LIGHTING, ETL listed, Class 2, 100–240 V AC in, **5.5 × 2.1 mm** barrel | Roughly 2× the ~3.1 A peak budget below, so it covers the full build including audio with headroom. Class 2 / ETL listing means it is current-limited and safety certified — worth insisting on for anything left plugged in. | [B01D8FM4N4](https://www.amazon.com/dp/B01D8FM4N4) |
| 1 | **Female barrel jack socket, 5.5 × 2.1 mm** | Solders to the board and mates with the PSU. **The barrel size must match** — 5.5 × 2.5 mm looks identical, fits loosely, and drops out under vibration. | [B0BW8N7XZ7](https://www.amazon.com/dp/B0BW8N7XZ7) |
| — | **20 AWG solid core hook-up wire** | Every jumper on the PCB, rail links included. One gauge for the whole board means no sorting mid-build. See the gauge note in [wiring.md](wiring.md) — 22 AWG is easier to handle and equally adequate. | [B084DM42JS](https://www.amazon.com/dp/B084DM42JS) |
| — | **22 AWG preformed breadboard jumpers** | Part 1 only. Preformed kits sit flat and keep a prototype readable in a way loose flying leads do not. | [B01KHWEB3W](https://www.amazon.com/dp/B01KHWEB3W) |
| 1 | **USB data cable** (match your board's connector) | **Must be a data cable, not charge-only.** A charge-only cable powers the board and looks completely normal, but never enumerates a COM port — this cost a full debugging session on this project. | *generic part* |

### Power budget

Why 6 A rather than the 2 A supply that "looks like enough":

| Load | Peak |
| ---- | ---- |
| 14 × WS2812 at full white | ~840 mA |
| 2 × MG90S servos, stall | ~1.5 A |
| ESP32 with Bluetooth active | ~250 mA |
| DFPlayer + speaker | ~500 mA |
| **Total peak** | **~3.1 A** |

The 6 A supply above covers this roughly 2× over, including audio once the
DFPlayer is installed. Anything 4 A or better is fine.

These peaks are brief and rarely coincide, which is exactly why an undersized
supply appears to work and then browns out only when the eyes move and the
LEDs flash at the same moment. Sizing for the sum of peaks rather than the
average is what buys you a head that never does that.

## Tools

No specific models — any decent version of each works.

| Part | Note |
| ---- | ---- |
| Soldering iron + solder | Part 2 only. A fine conical tip suits 2.54 mm perfboard. |
| **Multimeter** | Not optional. The continuity checks in [wiring.md](wiring.md) before first power-up are what catch a rail short before it damages anything. |
| Wire strippers, flush cutters | |
