// Resonance portable core: frequency presets for the Frequency Studio
// (RES-AUDIO-006). A preset sets a starting value and never restricts what
// the user does next.
//
// The names say which frequency or tradition a value comes from. They make
// no statement about effects on a listener (RES-PROC-006).
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stdint.h>

namespace rc {

struct FrequencyPreset {
    const char* name;  // at most 14 characters
    float hz;
};

// Primary (carrier) presets and beat presets, each in rising order.
uint8_t primaryPresetCount();
const FrequencyPreset& primaryPreset(uint8_t index);
uint8_t beatPresetCount();
const FrequencyPreset& beatPreset(uint8_t index);

// The preset that follows `currentHz`: the first one above it, or the lowest
// when `currentHz` is at or above the highest. Cycling therefore always
// moves, wherever the value was left by hand.
const FrequencyPreset& nextPrimaryPreset(float currentHz);
const FrequencyPreset& nextBeatPreset(float currentHz);

// Name of the preset `hz` is on (within half a step), or nullptr.
const char* primaryPresetName(float hz);
const char* beatPresetName(float hz);

}  // namespace rc
