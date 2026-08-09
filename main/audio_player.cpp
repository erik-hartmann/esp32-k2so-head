#include "audio_player.h"

#include <Arduino.h>
#include <Bluepad32.h>
#include <HardwareSerial.h>

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

void AudioPlayer::begin(int rxGpio, int txGpio) {
    sSerial.begin(kBaud, SERIAL_8N1, rxGpio, txGpio);
    sReadyAtMs = millis() + kStartupDelayMs;
    sInitSent = false;
    Console.printf("Audio: DFPlayer on UART2 (rx=%d, tx=%d)\n", rxGpio, txGpio);
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

void AudioPlayer::play(uint16_t track) {
    ensureInitialized();
    sendCommand(kCmdPlayMp3Folder, track);
    Console.printf("Audio: play /mp3/%04u.mp3\n", track);
}

void AudioPlayer::stop() {
    sendCommand(kCmdStop, 0);
}
