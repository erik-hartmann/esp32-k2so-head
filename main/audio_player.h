// Minimal DFPlayer Mini driver — the MP3 module with a microSD slot and a
// built-in amp, controlled over a 9600-baud serial link.
//
// Deliberately not using the DFRobot Arduino library: its begin() and query
// calls block waiting for the module to answer, and a stall inside a loop
// that's also driving servos and servicing the Bluetooth stack would be felt
// immediately. The wire protocol is a fixed 10-byte packet, so this sends
// fire-and-forget commands and never waits on a reply.
//
// SD card layout: files go in an "mp3" folder at the card root, named with
// four digits — /mp3/0001.mp3, /mp3/0002.mp3, and so on. That folder-based
// addressing is far more reliable than the flat track index, which depends
// on the order files were physically written to the card.
#pragma once

#include <cstdint>

namespace AudioPlayer {

// rxGpio is the ESP32 pin wired to the DFPlayer's TX; txGpio is the ESP32
// pin wired to its RX. Does not block — the module needs roughly a second
// after power-up before it accepts commands, and that wait is handled
// lazily on the first play() rather than stalling setup().
void begin(int rxGpio, int txGpio);

// 0..30, clamped. The module is loud; 20 is a reasonable starting point.
void setVolume(uint8_t volume);

// Last value passed to setVolume(), after clamping. Callers stepping the
// volume up and down should read this rather than tracking their own copy,
// so the two cannot drift apart.
uint8_t volume();

// Highest track number the caller should ask for. See kMaxTrack in the .cpp
// for why this is a fixed number rather than something read off the card.
uint16_t trackCount();

// Plays /mp3/NNNN.mp3. Track numbering starts at 1.
void play(uint16_t track);

void stop();

}  // namespace AudioPlayer
