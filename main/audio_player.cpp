#include "audio_player.h"

#include <Arduino.h>
#include <Bluepad32.h>
#include <HardwareSerial.h>

#include "board_config.h"

namespace {

constexpr uint32_t kBaud = 9600;  // DFPlayer's fixed rate

// Command bytes from the DFPlayer serial protocol.
constexpr uint8_t kCmdSelectDevice = 0x09;
constexpr uint8_t kCmdSetVolume = 0x06;
constexpr uint8_t kCmdStop = 0x16;
constexpr uint8_t kCmdPlayMp3Folder = 0x12;

constexpr uint16_t kDeviceSdCard = 0x0002;

// The module ignores everything sent during its own power-up. Rather than
// blocking setup() waiting, the init commands are deferred until the first
// play() that happens after this window has elapsed.
constexpr uint32_t kStartupDelayMs = 1500;

HardwareSerial sSerial(2);  // UART2; UART0 is Bluepad32's console
uint32_t sReadyAtMs = 0;
bool sInitSent = false;
uint8_t sVolume = 20;
uint16_t sCurrentTrack = 0;  // 0 = nothing played yet
int sBusyGpio = -1;
uint32_t sLastPlayCommandMs = 0;
bool sHasPlayed = false;

void sendCommand(uint8_t cmd, uint16_t param) {
    uint8_t packet[10];
    packet[0] = 0x7E;  // start
    packet[1] = 0xFF;  // version
    packet[2] = 0x06;  // length of the addressed portion
    packet[3] = cmd;
    packet[4] = 0x00;  // 1 would request an ack we deliberately never read
    packet[5] = static_cast<uint8_t>(param >> 8);
    packet[6] = static_cast<uint8_t>(param & 0xFF);

    // Checksum is the two's complement of bytes 1..6.
    uint16_t sum = 0;
    for (int i = 1; i <= 6; i++) {
        sum += packet[i];
    }
    uint16_t checksum = static_cast<uint16_t>(-sum);
    packet[7] = static_cast<uint8_t>(checksum >> 8);
    packet[8] = static_cast<uint8_t>(checksum & 0xFF);
    packet[9] = 0xEF;  // end

    sSerial.write(packet, sizeof(packet));
}

// Sends the one-time setup the module needs, but only once it's had time to
// boot. Called from the command entry points so nothing has to block.
void ensureInitialized() {
    if (sInitSent || static_cast<int32_t>(millis() - sReadyAtMs) < 0) {
        return;
    }
    sendCommand(kCmdSelectDevice, kDeviceSdCard);
    delay(50);  // the module needs a moment between these two
    sendCommand(kCmdSetVolume, sVolume);
    sInitSent = true;
}

}  // namespace

void AudioPlayer::begin(int txGpio, int busyGpio) {
    // -1 for RX: this driver never reads a reply, so binding a receive pin
    // would only reserve a GPIO to do nothing.
    sSerial.begin(kBaud, SERIAL_8N1, -1, txGpio);
    sReadyAtMs = millis() + kStartupDelayMs;
    sInitSent = false;

    sBusyGpio = busyGpio;
    if (sBusyGpio >= 0) {
        // Pull-up matters: BUSY is an open output that the module pulls low.
        // Without it, an unwired pin floats and would read as playback
        // starting and stopping at random.
        pinMode(sBusyGpio, INPUT_PULLUP);
    }

    Console.printf("Audio: DFPlayer on UART2 (tx=%d), busy=%s\n", txGpio,
                   sBusyGpio >= 0 ? String(sBusyGpio).c_str() : "not wired");
}

void AudioPlayer::setVolume(uint8_t volume) {
    if (volume > 30) {
        volume = 30;
    }
    sVolume = volume;
    ensureInitialized();
    if (sInitSent) {
        sendCommand(kCmdSetVolume, sVolume);
    }
}

uint8_t AudioPlayer::volume() {
    return sVolume;
}

uint16_t AudioPlayer::currentTrack() {
    return sCurrentTrack;
}

void AudioPlayer::playNext() {
    uint16_t count = trackCount();
    if (count == 0) {
        return;
    }
    AudioPlayer::play((sCurrentTrack % count) + 1);
}

void AudioPlayer::playPrevious() {
    uint16_t count = trackCount();
    if (count == 0) {
        return;
    }
    AudioPlayer::play(sCurrentTrack <= 1 ? count : sCurrentTrack - 1);
}

void AudioPlayer::replay() {
    if (trackCount() == 0) {
        return;
    }
    AudioPlayer::play(sCurrentTrack == 0 ? 1 : sCurrentTrack);
}

uint16_t AudioPlayer::trackCount() {
    // Fixed, not discovered. The DFPlayer *can* report its file count —
    // command 0x48 — but the answer comes back on the module's TX pin, and
    // this build deliberately does not wire it: the cable is a 3-conductor
    // JST (5V / GND / RX) and this driver never reads a reply, so nothing can
    // stall the loop that also drives the servos and the Bluetooth stack.
    //
    // Making this genuinely dynamic therefore needs hardware, not just code:
    // a 4th conductor from the DFPlayer TX (pin 3) to a spare GPIO, plus a
    // non-blocking reader for the 10-byte reply frames. Until then this must
    // match the number of /mp3/NNNN.mp3 files on the card. Asking for a track
    // above the count is harmless — the module simply ignores it — but the
    // press looks broken, because there is no reply to tell us it failed.
#ifdef AUDIO_TRACK_COUNT
    return AUDIO_TRACK_COUNT;
#else
    // No audio configured for this board. Returning 0 rather than guessing a
    // count keeps the step/replay helpers above from dividing by it.
    return 0;
#endif
}

bool AudioPlayer::isPlaying() {
    if (sBusyGpio < 0) {
        return false;
    }
    return digitalRead(sBusyGpio) == LOW;  // active low
}

uint32_t AudioPlayer::sinceLastPlayCommand() {
    if (!sHasPlayed) {
        return UINT32_MAX;
    }
    return millis() - sLastPlayCommandMs;
}

void AudioPlayer::play(uint16_t track) {
    ensureInitialized();
    sCurrentTrack = track;
    sLastPlayCommandMs = millis();
    sHasPlayed = true;
    sendCommand(kCmdPlayMp3Folder, track);
    Console.printf("Audio: play /mp3/%04u.mp3\n", track);
}

void AudioPlayer::stop() {
    sendCommand(kCmdStop, 0);
}
