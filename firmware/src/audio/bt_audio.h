// Bluetooth A2DP source: the audio output of Play mode (RES-BT-001,
// RES-AUDIO-011, ADR-000204). Owns the Bluetooth stack while it runs.
//
// The stack pulls audio through a callback in its own task; the callback
// renders straight from the tone engine, which never blocks (RES-AUDIO-012).
#pragma once

#include <stdint.h>

#include "rc_audio.h"

namespace btaudio {

enum class State : uint8_t {
    Off,         // not started
    Scanning,    // looking for audio devices
    Connecting,  // a device was chosen and is being connected
    Connected,
};

constexpr uint8_t kMaxDevices = 5;
constexpr uint8_t kNameLength = 24;

// Starts Bluetooth and begins looking for audio devices. A device used
// before is reconnected automatically when it is in range. `engine` supplies
// the audio and must outlive the stack.
bool begin(const char* localName, rc::AudioEngine& engine);

State state();

// Brings the list below up to date and restarts a scan that a disconnect
// stopped. Call about once a second from the interface task.
void updateList();

// Audio devices in range, strongest signal first. Devices that have not
// been seen for 45 seconds are dropped.
uint8_t deviceCount();
const char* deviceName(uint8_t index);

// Connects to a listed device as soon as the scan sees it again. A device
// that is connected at the time is dropped first.
void connect(uint8_t index);

// Drops the connection and goes back to scanning.
void disconnect();

// Name of the connected or chosen device; empty when unknown (a device
// reconnected from memory has no name until it is found by a scan).
const char* peerName();

// Timing of the audio stream since the last call (diagnostics).
struct StreamStats {
    uint32_t worstGapMs;     // longest time between two requests for audio
    uint32_t worstRenderUs;  // longest time one request took to fill
    uint32_t longGaps;       // requests that came more than 60 ms after the previous one
};
StreamStats takeStreamStats();

}  // namespace btaudio
