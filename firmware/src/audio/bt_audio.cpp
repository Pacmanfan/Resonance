#include "bt_audio.h"

#include <Arduino.h>
#include <BluetoothA2DPSource.h>

#include "../storage/settings_store.h"

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
bool nameSettled = false; // the connected device's name has been stored or looked up
bool nameRequested = false;
uint32_t connectedSinceMs = 0;

// Stream timing, written by the Bluetooth task and read for diagnostics.
volatile uint32_t lastCallUs = 0;
volatile uint32_t worstGapUs = 0;
volatile uint32_t worstRenderUs = 0;
volatile uint32_t longGaps = 0;

// The library keeps its state machine protected; scanning again after a
// disconnect needs access to it.
// Name the peer gave when the link was authenticated. Written by the
// Bluetooth task at every connection, including automatic reconnections.
char authenticatedName[kNameLength] = "";

class Source : public BluetoothA2DPSource {
public:
    // The library only logs the peer's name on authentication; it is kept
    // here so that a device that reconnects by itself can be named.
    void app_gap_callback(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t* param) override {
        if (event == ESP_BT_GAP_AUTH_CMPL_EVT && param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            strlcpy(authenticatedName, reinterpret_cast<const char*>(param->auth_cmpl.device_name), kNameLength);
        } else if (event == ESP_BT_GAP_READ_REMOTE_NAME_EVT && param->read_rmt_name.stat == ESP_BT_STATUS_SUCCESS) {
            // The answer to the name request made in updateList().
            strlcpy(authenticatedName, reinterpret_cast<const char*>(param->read_rmt_name.rmt_name), kNameLength);
        }
        BluetoothA2DPSource::app_gap_callback(event, param);
    }

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
    // Timing of the stream, for diagnostics: the audio rendered should keep
    // pace with real time. A long gap between two calls is heard as a stutter.
    const uint32_t now = micros();
    if (lastCallUs != 0) {
        const uint32_t gap = now - lastCallUs;
        // A gap of seconds is the stream having been stopped and started
        // again, not a stutter, and is left out.
        if (gap < 2000000) {
            if (gap > worstGapUs) worstGapUs = gap;
            if (gap > 60000) ++longGaps;
        }
    }
    lastCallUs = now;
    engine->render(reinterpret_cast<int16_t*>(frames), static_cast<size_t>(frameCount));
    const uint32_t took = micros() - now;
    if (took > worstRenderUs) worstRenderUs = took;
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
    // The name of the connected device. In order of trust: the name the user
    // picked from the list, the name stored for this address, the name the
    // device gave when the link was authenticated. A bonded device that
    // reconnects by itself gives none, so it is then asked for its name.
    if (!source().is_connected()) {
        nameSettled = false;
        nameRequested = false;
        connectedSinceMs = 0;
    } else if (!nameSettled) {
        if (connectedSinceMs == 0) connectedSinceMs = millis() | 1u;
        uint8_t* address = *source().get_last_peer_address();
        if (chosenName[0] == '\0' && !storage::loadDeviceName(address, chosenName, kNameLength)) {
            if (authenticatedName[0] != '\0') {
                strlcpy(chosenName, authenticatedName, kNameLength);
            } else if (!nameRequested) {
                nameRequested = true;
                esp_bt_gap_read_remote_name(address);
            }
        }
        if (chosenName[0] != '\0') {
            storage::saveDeviceName(address, chosenName);
            nameSettled = true;
            Serial.printf("BT connected to '%s'\n", chosenName);
        } else if (millis() - connectedSinceMs > 15000) {
            nameSettled = true;  // the device did not answer; it stays unnamed
            Serial.println("BT connected to a device that gave no name");
        }
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
    authenticatedName[0] = '\0';  // belongs to the previous device
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
    authenticatedName[0] = '\0';
    source().disconnect();
    rescanAtMs = millis() + 1500;
    if (rescanAtMs == 0) rescanAtMs = 1;
}

const char* peerName() { return chosenName; }

StreamStats takeStreamStats() {
    const StreamStats stats = {worstGapUs / 1000u, worstRenderUs, longGaps};
    worstGapUs = 0;
    worstRenderUs = 0;
    longGaps = 0;
    return stats;
}

}  // namespace btaudio
