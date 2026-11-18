// Resonance portable core: device settings and their validation.
// RES-CFG-002 (validated on load, safe defaults), RES-CFG-003 (never loud at
// start), RES-AUDIO-002 / RES-AUDIO-005 (frequency ranges, invalid values
// rejected), RES-ARCH-005 (operating modes).
//
// Portable: this file must not include an Arduino, ESP-IDF or display header.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace rc {

// Operating mode (ADR-000204). A subsystem that the current mode does not
// need is never started; the mode changes only through a restart.
enum class Mode : uint8_t {
    Play = 0,   // Bluetooth, audio, playback, sessions. Wi-Fi not started.
    Setup = 1,  // Wi-Fi, web administration, files. Bluetooth not started.
};

// Ranges of RES-AUDIO-002, in hertz.
constexpr float kPrimaryMinHz = 20.0f;
constexpr float kPrimaryMaxHz = 1000.0f;
constexpr float kPrimaryDefaultHz = 200.0f;
constexpr float kBeatMinHz = 0.5f;
constexpr float kBeatMaxHz = 40.0f;
constexpr float kBeatDefaultHz = 10.0f;
constexpr float kFrequencyStepHz = 0.1f;

// Volumes are percentages. The start-up volume is capped so that a restart
// can never come back loud (RES-CFG-003).
constexpr uint8_t kVolumeMax = 100;
constexpr uint8_t kStartVolumeMax = 60;
constexpr uint8_t kStartVolumeDefault = 30;

constexpr uint8_t kBrightnessMin = 5;  // never fully dark: the screen is the only control
constexpr uint8_t kBrightnessMax = 100;
constexpr uint8_t kBrightnessDefault = 70;

constexpr uint16_t kScreenTimeoutMaxS = 3600;  // 0 = never dim
constexpr uint16_t kScreenTimeoutDefaultS = 60;

constexpr uint8_t kSettingsVersion = 1;

// Stored as one fixed-size record. Every text field is NUL-terminated.
struct Settings {
    uint8_t version;
    Mode mode;
    uint8_t startVolume;     // percent, <= kStartVolumeMax
    uint8_t brightness;      // percent
    uint16_t screenTimeoutS;
    float primaryHz;
    float beatHz;
    char hostname[24];       // mDNS / DHCP name, RFC 1123 label
    char wifiSsid[33];
    char wifiPass[65];
    char apPass[12];         // password of the set-up access point; empty = not generated yet
};

// Fills `s` with the factory defaults.
void setDefaults(Settings& s);

// Brings every field into its valid range. Returns true when something had to
// be changed. A record of another version is replaced by the defaults.
bool sanitize(Settings& s);

// Clamps to [lo, hi], rounds to the 0.1 Hz step, and returns `fallback` for a
// value that is not finite.
float sanitizeFrequency(float hz, float lo, float hi, float fallback);

// One DNS label: 1-23 characters, letters, digits and '-', not starting or
// ending with '-'.
bool isValidHostname(const char* name);

// True when `text` holds a NUL within `capacity` bytes and only printable
// ASCII before it.
bool isPrintableField(const char* text, size_t capacity);

}  // namespace rc
