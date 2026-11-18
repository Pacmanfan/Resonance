#include "bt_audio.h"

#include <Arduino.h>
#include <BluetoothA2DPSource.h>

namespace btaudio {

namespace {

struct Device {
    char name[kNameLength];
    esp_bd_addr_t address;
};

rc::ToneEngine* engine = nullptr;
bool started = false;

// Written by the Bluetooth task (scan results), read by the interface task.
// An entry is complete before `count` is raised, and entries never move.
Device devices[kMaxDevices];
volatile uint8_t count = 0;

volatile bool haveChoice = false;
esp_bd_addr_t chosen;
char chosenName[kNameLength] = "";

BluetoothA2DPSource& source() {
    static BluetoothA2DPSource instance;  // built on first use, after the scheduler started
    return instance;
}

// Audio for the stack. Runs in the Bluetooth task: no blocking, no logging.
int32_t provideFrames(Frame* frames, int32_t frameCount) {
    engine->render(reinterpret_cast<int16_t*>(frames), static_cast<size_t>(frameCount));
    return frameCount;
}

// Called for every audio device the scan sees. Returning true connects.
bool onDeviceFound(const char* name, esp_bd_addr_t address, int /*rssi*/) {
    if (haveChoice && memcmp(address, chosen, sizeof(esp_bd_addr_t)) == 0) return true;
    if (name == nullptr || name[0] == '\0') return false;
    const uint8_t n = count;
    for (uint8_t i = 0; i < n; ++i) {
        if (memcmp(devices[i].address, address, sizeof(esp_bd_addr_t)) == 0) return false;
    }
    if (n < kMaxDevices) {
        strlcpy(devices[n].name, name, kNameLength);
        memcpy(devices[n].address, address, sizeof(esp_bd_addr_t));
        count = static_cast<uint8_t>(n + 1);
    }
    return false;
}

}  // namespace

bool begin(const char* localName, rc::ToneEngine& toneEngine) {
    if (started) return true;
    engine = &toneEngine;
    BluetoothA2DPSource& s = source();
    s.set_local_name(localName);
    s.set_ssid_callback(onDeviceFound);
    s.set_auto_reconnect(true);
    s.set_data_callback_in_frames(provideFrames);
    s.start();
    started = true;
    return true;
}

State state() {
    if (!started) return State::Off;
    if (source().is_connected()) return State::Connected;
    return haveChoice ? State::Connecting : State::Scanning;
}

uint8_t deviceCount() { return count; }

const char* deviceName(uint8_t index) { return index < count ? devices[index].name : ""; }

void connect(uint8_t index) {
    if (index >= count) return;
    memcpy(chosen, devices[index].address, sizeof(esp_bd_addr_t));
    strlcpy(chosenName, devices[index].name, kNameLength);
    haveChoice = true;
}

void disconnect() {
    if (!started) return;
    haveChoice = false;
    chosenName[0] = '\0';
    source().disconnect();
}

const char* peerName() { return chosenName; }

}  // namespace btaudio
