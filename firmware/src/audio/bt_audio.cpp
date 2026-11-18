#include "bt_audio.h"

#include <Arduino.h>
#include <BluetoothA2DPSource.h>

namespace btaudio {

namespace {

// A device seen by the scan. The table keeps the strongest signals: a new
// device replaces the weakest entry when it is stronger, and an entry that
// has not been seen for a while is dropped. A headset next to the device is
// therefore always listed, however many other devices are around.
struct Device {
    char name[kNameLength];
    esp_bd_addr_t address;
    int8_t rssi;
    uint32_t seenMs;
    bool used;
};
constexpr uint8_t kTableSize = 8;
constexpr uint32_t kForgetAfterMs = 45000;

rc::AudioEngine* engine = nullptr;
bool started = false;

// `table` is written by the Bluetooth task and read by the interface task,
// under `lock`. `shown` belongs to the interface task alone.
portMUX_TYPE lock = portMUX_INITIALIZER_UNLOCKED;
Device table[kTableSize];
Device shown[kMaxDevices];
uint8_t shownCount = 0;

volatile bool haveChoice = false;
esp_bd_addr_t chosen;
char chosenName[kNameLength] = "";
uint32_t rescanAtMs = 0;  // 0 = no rescan pending

// The library keeps its state machine protected; scanning again after a
// disconnect needs access to it.
class Source : public BluetoothA2DPSource {
public:
    void rescan() {
        s_a2d_state = APP_AV_STATE_DISCOVERING;
        esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
    }
};

Source& source() {
    static Source instance;  // built on first use, after the scheduler started
    return instance;
}

// Audio for the stack. Runs in the Bluetooth task: no blocking, no logging.
int32_t provideFrames(Frame* frames, int32_t frameCount) {
    engine->render(reinterpret_cast<int16_t*>(frames), static_cast<size_t>(frameCount));
    return frameCount;
}

// Called for every audio device the scan sees. Returning true connects.
bool onDeviceFound(const char* name, esp_bd_addr_t address, int rssi) {
    if (haveChoice && memcmp(address, chosen, sizeof(esp_bd_addr_t)) == 0) return true;
    const uint32_t now = millis();
    const int8_t level = static_cast<int8_t>(rssi < -127 ? -127 : (rssi > 0 ? 0 : rssi));
    portENTER_CRITICAL(&lock);
    Device* slot = nullptr;
    Device* weakest = nullptr;
    for (Device& d : table) {
        if (d.used && memcmp(d.address, address, sizeof(esp_bd_addr_t)) == 0) {
            slot = &d;
            break;
        }
        if (!d.used || now - d.seenMs > kForgetAfterMs) {
            if (slot == nullptr) slot = &d;
        } else if (weakest == nullptr || d.rssi < weakest->rssi) {
            weakest = &d;
        }
    }
    if (slot == nullptr && weakest != nullptr && level > weakest->rssi) slot = weakest;
    if (slot != nullptr) {
        const bool known = slot->used && memcmp(slot->address, address, sizeof(esp_bd_addr_t)) == 0;
        if (name != nullptr && name[0] != '\0') {
            strlcpy(slot->name, name, kNameLength);
        } else if (!known) {
            // A device that does not send its name is listed by the end of
            // its address, so that it can still be chosen.
            snprintf(slot->name, kNameLength, "Device %02X:%02X:%02X", address[3], address[4], address[5]);
        }
        memcpy(slot->address, address, sizeof(esp_bd_addr_t));
        slot->rssi = level;
        slot->seenMs = now;
        slot->used = true;
    }
    portEXIT_CRITICAL(&lock);
    return false;
}

}  // namespace

bool begin(const char* localName, rc::AudioEngine& audioEngine) {
    if (started) return true;
    engine = &audioEngine;
    Source& s = source();
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

void updateList() {
    if (!started) return;
    // A scan that was stopped by a disconnect is started again once the
    // link is down.
    if (rescanAtMs != 0 && static_cast<int32_t>(millis() - rescanAtMs) >= 0 && !source().is_connected()) {
        rescanAtMs = 0;
        source().rescan();
    }
    const uint32_t now = millis();
    Device copy[kTableSize];
    portENTER_CRITICAL(&lock);
    memcpy(copy, table, sizeof(copy));
    portEXIT_CRITICAL(&lock);
    // Strongest first.
    shownCount = 0;
    for (uint8_t pick = 0; pick < kMaxDevices; ++pick) {
        Device* best = nullptr;
        for (Device& d : copy) {
            if (!d.used || now - d.seenMs > kForgetAfterMs) continue;
            if (best == nullptr || d.rssi > best->rssi) best = &d;
        }
        if (best == nullptr) break;
        shown[shownCount++] = *best;
        best->used = false;
    }
}

uint8_t deviceCount() { return shownCount; }

const char* deviceName(uint8_t index) { return index < shownCount ? shown[index].name : ""; }

void connect(uint8_t index) {
    if (index >= shownCount) return;
    memcpy(chosen, shown[index].address, sizeof(esp_bd_addr_t));
    strlcpy(chosenName, shown[index].name, kNameLength);
    haveChoice = true;
    // Connected to something else: drop it, then scan for the chosen one.
    if (source().is_connected()) {
        source().disconnect();
        rescanAtMs = millis() + 1500;
    }
}

void disconnect() {
    if (!started) return;
    haveChoice = false;
    chosenName[0] = '\0';
    source().disconnect();
    rescanAtMs = millis() + 1500;
    if (rescanAtMs == 0) rescanAtMs = 1;
}

const char* peerName() { return chosenName; }

}  // namespace btaudio
