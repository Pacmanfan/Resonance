// Resonance portable core: device settings and their validation.
// RES-CFG-002 (validated on load, safe defaults), RES-CFG-003 (never loud at
// start), RES-AUDIO-002 / RES-AUDIO-005 (frequency ranges, invalid values
// rejected).
//
// Portable: this file must not include an Arduino, ESP-IDF or display header.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace rc {

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
constexpr uint8_t kBrightnessDefault = 100;

constexpr uint16_t kScreenTimeoutMaxS = 3600;  // 0 = never dim
constexpr uint16_t kScreenTimeoutDefaultS = 60;

// 2: brightness default 100; 3: layer volumes and session choices; 4: the
// operating mode, device name and Wi-Fi fields are gone with the Setup mode
// (ADR-000205).
constexpr uint8_t kSettingsVersion = 4;

constexpr uint8_t kBackgroundVolumeDefault = 70;
constexpr uint8_t kNoiseVolumeDefault = 40;

// Stored as one fixed-size record.
struct Settings {
    uint8_t version;
    uint8_t startVolume;     // percent, <= kStartVolumeMax
    uint8_t brightness;      // percent
    uint16_t screenTimeoutS;
    float primaryHz;
    float beatHz;
    // What the user last set, so that the device comes back as it was left.
    // The master volume is kept in startVolume, and so can never come back
    // above kStartVolumeMax. The noise type is deliberately not kept:
    // nothing may sound at start (RES-CFG-003).
    uint8_t backgroundVolume;  // percent
    uint8_t noiseVolume;       // percent
    uint8_t sessionProgram;    // 0 = timer, 1.. = factory programs; checked against their number by the caller
    uint8_t timerChoice;       // index into the timer lengths; checked by the caller
    uint8_t programLength;     // index into the program lengths; checked by the caller
    uint8_t fadeChoice;        // index into the fade lengths; checked by the caller
};

// Fills `s` with the factory defaults.
void setDefaults(Settings& s);

// Brings every field into its valid range. Returns true when something had to
// be changed. A record of another version is replaced by the defaults.
bool sanitize(Settings& s);

// Clamps to [lo, hi], rounds to the 0.1 Hz step, and returns `fallback` for a
// value that is not finite.
float sanitizeFrequency(float hz, float lo, float hi, float fallback);

// Size of the version 3 record, which also held the operating mode, a
// device name and Wi-Fi credentials.
constexpr size_t kSettingsV3Size = 156;

// Carries what the user had set in a version 3 record over into `out`, and
// validates it. The mode, the name and the credentials are not carried.
// Returns false, with `out` untouched, when `bytes` is not such a record.
bool migrateFromV3(const void* bytes, size_t size, Settings& out);

}  // namespace rc
