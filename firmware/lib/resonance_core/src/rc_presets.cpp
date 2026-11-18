#include "rc_presets.h"

#include "rc_settings.h"

namespace rc {

namespace {

// Carriers: round numbers, two tuning references, and the "Solfeggio" set.
const FrequencyPreset kPrimary[] = {
    {"100 Hz", 100.0f},
    {"Om 136.1", 136.1f},
    {"Solfeggio 174", 174.0f},
    {"200 Hz", 200.0f},
    {"Solfeggio 285", 285.0f},
    {"Solfeggio 396", 396.0f},
    {"Solfeggio 417", 417.0f},
    {"Verdi 432", 432.0f},
    {"Concert 440", 440.0f},
    {"Solfeggio 528", 528.0f},
    {"Solfeggio 639", 639.0f},
    {"Solfeggio 741", 741.0f},
    {"Solfeggio 852", 852.0f},
    {"Solfeggio 963", 963.0f},
};

// Beats: one or two per band of RES-AUDIO-006, and the Schumann value
// (7.83 Hz, held here at the device's 0.1 Hz step).
const FrequencyPreset kBeat[] = {
    {"Delta 1", 1.0f},
    {"Delta 2.5", 2.5f},
    {"Theta 4.5", 4.5f},
    {"Theta 6", 6.0f},
    {"Schumann 7.8", 7.8f},
    {"Alpha 10", 10.0f},
    {"SMR 14", 14.0f},
    {"Beta 20", 20.0f},
    {"Gamma 40", 40.0f},
};

constexpr float kHalfStep = kFrequencyStepHz / 2.0f;

template <uint8_t N>
const FrequencyPreset& next(const FrequencyPreset (&list)[N], float currentHz) {
    for (uint8_t i = 0; i < N; ++i) {
        if (list[i].hz > currentHz + kHalfStep) return list[i];
    }
    return list[0];
}

template <uint8_t N>
const char* nameOf(const FrequencyPreset (&list)[N], float hz) {
    for (uint8_t i = 0; i < N; ++i) {
        const float d = list[i].hz - hz;
        if (d < kHalfStep && d > -kHalfStep) return list[i].name;
    }
    return nullptr;
}

}  // namespace

uint8_t primaryPresetCount() { return sizeof(kPrimary) / sizeof(kPrimary[0]); }
const FrequencyPreset& primaryPreset(uint8_t index) { return kPrimary[index < primaryPresetCount() ? index : 0]; }
uint8_t beatPresetCount() { return sizeof(kBeat) / sizeof(kBeat[0]); }
const FrequencyPreset& beatPreset(uint8_t index) { return kBeat[index < beatPresetCount() ? index : 0]; }

const FrequencyPreset& nextPrimaryPreset(float currentHz) { return next(kPrimary, currentHz); }
const FrequencyPreset& nextBeatPreset(float currentHz) { return next(kBeat, currentHz); }

const char* primaryPresetName(float hz) { return nameOf(kPrimary, hz); }
const char* beatPresetName(float hz) { return nameOf(kBeat, hz); }

}  // namespace rc
