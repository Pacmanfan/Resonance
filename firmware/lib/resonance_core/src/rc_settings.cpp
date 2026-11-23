#include "rc_settings.h"

#include <math.h>
#include <string.h>

namespace rc {

namespace {

// The record of settings version 3, kept only to read it once.
struct SettingsV3 {
    uint8_t version;
    uint8_t mode;
    uint8_t startVolume;
    uint8_t brightness;
    uint16_t screenTimeoutS;
    float primaryHz;
    float beatHz;
    char hostname[24];
    char wifiSsid[33];
    char wifiPass[65];
    char apPass[12];
    uint8_t backgroundVolume;
    uint8_t noiseVolume;
    uint8_t sessionProgram;
    uint8_t timerChoice;
    uint8_t programLength;
    uint8_t fadeChoice;
};
static_assert(sizeof(SettingsV3) == kSettingsV3Size, "the version 3 record as it was stored");

template <typename T>
bool clampInto(T& value, T lo, T hi) {
    if (value < lo) { value = lo; return true; }
    if (value > hi) { value = hi; return true; }
    return false;
}

}  // namespace

void setDefaults(Settings& s) {
    memset(&s, 0, sizeof(s));
    s.version = kSettingsVersion;
    s.startVolume = kStartVolumeDefault;
    s.brightness = kBrightnessDefault;
    s.screenTimeoutS = kScreenTimeoutDefaultS;
    s.primaryHz = kPrimaryDefaultHz;
    s.beatHz = kBeatDefaultHz;
    s.backgroundVolume = kBackgroundVolumeDefault;
    s.noiseVolume = kNoiseVolumeDefault;
    s.sessionProgram = 0;
    s.timerChoice = 2;  // 20 minutes
    s.programLength = 0;
    s.fadeChoice = 1;   // 30 seconds
}

float sanitizeFrequency(float hz, float lo, float hi, float fallback) {
    if (!isfinite(hz)) return fallback;
    if (hz < lo) hz = lo;
    if (hz > hi) hz = hi;
    // Round to the step; computed in double so that 0.1 steps do not drift.
    const double steps = round(static_cast<double>(hz) / kFrequencyStepHz);
    float rounded = static_cast<float>(steps * kFrequencyStepHz);
    if (rounded < lo) rounded = lo;
    if (rounded > hi) rounded = hi;
    return rounded;
}

bool sanitize(Settings& s) {
    if (s.version != kSettingsVersion) {
        setDefaults(s);
        return true;
    }
    bool changed = false;

    changed |= clampInto<uint8_t>(s.startVolume, 0, kStartVolumeMax);
    changed |= clampInto<uint8_t>(s.brightness, kBrightnessMin, kBrightnessMax);
    changed |= clampInto<uint16_t>(s.screenTimeoutS, 0, kScreenTimeoutMaxS);
    changed |= clampInto<uint8_t>(s.backgroundVolume, 0, kVolumeMax);
    changed |= clampInto<uint8_t>(s.noiseVolume, 0, kVolumeMax);
    // The four choices index tables that belong to the application; here
    // they are only kept from being absurd.
    changed |= clampInto<uint8_t>(s.sessionProgram, 0, 31);
    changed |= clampInto<uint8_t>(s.timerChoice, 0, 31);
    changed |= clampInto<uint8_t>(s.programLength, 0, 31);
    changed |= clampInto<uint8_t>(s.fadeChoice, 0, 31);

    const float primary = sanitizeFrequency(s.primaryHz, kPrimaryMinHz, kPrimaryMaxHz, kPrimaryDefaultHz);
    const float beat = sanitizeFrequency(s.beatHz, kBeatMinHz, kBeatMaxHz, kBeatDefaultHz);
    // Compared as bit patterns so that a NaN in the record counts as a change.
    if (memcmp(&primary, &s.primaryHz, sizeof(float)) != 0) { s.primaryHz = primary; changed = true; }
    if (memcmp(&beat, &s.beatHz, sizeof(float)) != 0) { s.beatHz = beat; changed = true; }
    return changed;
}

bool migrateFromV3(const void* bytes, size_t size, Settings& out) {
    if (bytes == nullptr || size != kSettingsV3Size) return false;
    SettingsV3 old;
    memcpy(&old, bytes, sizeof(old));
    if (old.version != 3) return false;
    Settings s;
    setDefaults(s);
    s.startVolume = old.startVolume;
    s.brightness = old.brightness;
    s.screenTimeoutS = old.screenTimeoutS;
    s.primaryHz = old.primaryHz;
    s.beatHz = old.beatHz;
    s.backgroundVolume = old.backgroundVolume;
    s.noiseVolume = old.noiseVolume;
    s.sessionProgram = old.sessionProgram;
    s.timerChoice = old.timerChoice;
    s.programLength = old.programLength;
    s.fadeChoice = old.fadeChoice;
    sanitize(s);
    out = s;
    return true;
}

}  // namespace rc
