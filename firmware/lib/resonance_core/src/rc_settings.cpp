#include "rc_settings.h"

#include <math.h>
#include <string.h>

namespace rc {

namespace {

const char kDefaultHostname[] = "resonance";

// Length of `text` if it is terminated within `capacity`, else `capacity`.
size_t boundedLength(const char* text, size_t capacity) {
    size_t n = 0;
    while (n < capacity && text[n] != '\0') ++n;
    return n;
}

template <typename T>
bool clampInto(T& value, T lo, T hi) {
    if (value < lo) { value = lo; return true; }
    if (value > hi) { value = hi; return true; }
    return false;
}

// Empties a text field that is unterminated or holds non-printable bytes.
bool cleanField(char* text, size_t capacity) {
    if (isPrintableField(text, capacity)) return false;
    memset(text, 0, capacity);
    return true;
}

}  // namespace

void setDefaults(Settings& s) {
    memset(&s, 0, sizeof(s));
    s.version = kSettingsVersion;
    s.mode = Mode::Play;
    s.startVolume = kStartVolumeDefault;
    s.brightness = kBrightnessDefault;
    s.screenTimeoutS = kScreenTimeoutDefaultS;
    s.primaryHz = kPrimaryDefaultHz;
    s.beatHz = kBeatDefaultHz;
    memcpy(s.hostname, kDefaultHostname, sizeof(kDefaultHostname));
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

bool isPrintableField(const char* text, size_t capacity) {
    const size_t n = boundedLength(text, capacity);
    if (n == capacity) return false;  // no terminator
    for (size_t i = 0; i < n; ++i) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if (c < 0x20 || c > 0x7e) return false;
    }
    return true;
}

bool isValidHostname(const char* name) {
    const size_t n = boundedLength(name, 24);
    if (n == 0 || n >= 24) return false;
    if (name[0] == '-' || name[n - 1] == '-') return false;
    for (size_t i = 0; i < n; ++i) {
        const char c = name[i];
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                        (c >= '0' && c <= '9') || c == '-';
        if (!ok) return false;
    }
    return true;
}

bool sanitize(Settings& s) {
    if (s.version != kSettingsVersion) {
        setDefaults(s);
        return true;
    }
    bool changed = false;

    if (s.mode != Mode::Play && s.mode != Mode::Setup) {
        s.mode = Mode::Play;
        changed = true;
    }
    changed |= clampInto<uint8_t>(s.startVolume, 0, kStartVolumeMax);
    changed |= clampInto<uint8_t>(s.brightness, kBrightnessMin, kBrightnessMax);
    changed |= clampInto<uint16_t>(s.screenTimeoutS, 0, kScreenTimeoutMaxS);

    const float primary = sanitizeFrequency(s.primaryHz, kPrimaryMinHz, kPrimaryMaxHz, kPrimaryDefaultHz);
    const float beat = sanitizeFrequency(s.beatHz, kBeatMinHz, kBeatMaxHz, kBeatDefaultHz);
    // Compared as bit patterns so that a NaN in the record counts as a change.
    if (memcmp(&primary, &s.primaryHz, sizeof(float)) != 0) { s.primaryHz = primary; changed = true; }
    if (memcmp(&beat, &s.beatHz, sizeof(float)) != 0) { s.beatHz = beat; changed = true; }

    if (!isValidHostname(s.hostname)) {
        memset(s.hostname, 0, sizeof(s.hostname));
        memcpy(s.hostname, kDefaultHostname, sizeof(kDefaultHostname));
        changed = true;
    }
    changed |= cleanField(s.wifiSsid, sizeof(s.wifiSsid));
    changed |= cleanField(s.wifiPass, sizeof(s.wifiPass));
    changed |= cleanField(s.apPass, sizeof(s.apPass));
    // A WPA2 passphrase needs 8 characters. A shorter one is dropped, and the
    // device generates a new one.
    const size_t apLen = boundedLength(s.apPass, sizeof(s.apPass));
    if (apLen != 0 && apLen < 8) {
        memset(s.apPass, 0, sizeof(s.apPass));
        changed = true;
    }
    // A password without a network name is meaningless.
    if (s.wifiSsid[0] == '\0' && s.wifiPass[0] != '\0') {
        memset(s.wifiPass, 0, sizeof(s.wifiPass));
        changed = true;
    }
    return changed;
}

}  // namespace rc
