// Resonance portable core: the Resonance Lab (RES-AUDIO-017).
//
// Experiments on the generated sound - tone, partials and noise, never a
// recording: stereo width, an interaural delay, a bilateral alternation of
// level between the ears, and a trim used for comparing two settings at the
// same loudness. With every control at rest the stage is not in the signal
// path at all.
//
// Portable: no Arduino, ESP-IDF or display header.
//
// Threads: set() and setTrim() from the interface task, begin() and
// process() from the audio output's task.
#pragma once

#include <atomic>
#include <stddef.h>
#include <stdint.h>

#include "rc_tone.h"

namespace rc {

constexpr int16_t kLabDelayMaxUs = 2000;
constexpr int16_t kLabDelayStepUs = 50;
constexpr uint16_t kBilateralMinDs = 5;    // period in tenths of a second: 0.5 s ...
constexpr uint16_t kBilateralMaxDs = 600;  // ... to a minute
// Share of the period spent on the left when the timing is golden: 1/phi.
constexpr float kGoldenShare = 0.6180339887f;
// Each crossing from one ear to the other takes this share of the period.
constexpr float kBilateralCrossing = 0.1f;

struct LabSettings {
    uint8_t widthPercent = 100;     // 0 = both ears the same, 100 = as generated
    int16_t delayUs = 0;            // > 0: the right ear later; < 0: the left ear later
    uint16_t bilateralPeriodDs = 0; // 0 = off
    uint8_t bilateralDepth = 100;   // percent the far ear is turned down by
    bool bilateralGolden = true;    // time left : right in the golden ratio, else equal
};

// Brings every field into range. Returns true when something was changed.
bool sanitizeLab(LabSettings& settings);
bool labAtRest(const LabSettings& settings);

// Where the alternation stands at `position` (0..1 of its period): -1 fully
// left, +1 fully right, moving smoothly in between.
float bilateralSide(float position, bool golden);

class Lab {
public:
    Lab();

    void set(const LabSettings& settings);
    // A gain on the generated sound, 0..1, reached in 50 ms (loudness matching).
    void setTrim(float gain);

    // Once per block of at most `frames`. Returns false when the stage is at
    // rest and process() must not be called: the caller then uses the
    // samples as they are.
    bool begin(size_t frames);
    // One frame of the generated sound, in place.
    void process(float& left, float& right);

private:
    static constexpr uint32_t kDelayLength = 128;  // samples; 2000 us are 88.2

    std::atomic<float> _widthTarget;
    std::atomic<float> _delayTarget;      // in samples, signed as LabSettings::delayUs
    std::atomic<float> _periodSamples;    // 0 = off
    std::atomic<float> _depthTarget;
    std::atomic<bool> _golden;
    std::atomic<float> _trimTarget;

    // Owned by the audio task.
    Ramp _width;
    Ramp _delay[2];  // left, right, in samples
    Ramp _side[2];   // bilateral gain of each ear
    Ramp _trim;
    Ramp _depth;     // of the alternation; comes in and goes out over a third of a second
    float _position = 0.0f;
    bool _active = false;
    bool _delaying = false;
    float _line[2][kDelayLength] = {};
    uint32_t _write = 0;
};

// What decides how loud a setting is, for matching two of them.
struct LabVoice {
    float primaryHz = 200.0f;
    float beatHz = 10.0f;
    float toneGain = 1.0f;      // the tone layer, 0..1
    float partialsGain = 1.0f;  // the partials layer, 0..1
    uint8_t partial[kPartialCount] = {0, 0};  // percent
    LabSettings lab;
};

// The A-weighting of a frequency as an amplitude factor (1 at 1 kHz).
float aWeight(float hz);
// The A-weighted power of the tone layer of `voice`, averaged over the ears,
// in arbitrary units. Noise is not counted: it is the same in both settings.
float labLoudness(const LabVoice& voice);
// The trims that make `a` and `b` equally loud: 1 for the quieter, below 1
// for the louder. Both 1 when either is silent.
void matchLoudness(const LabVoice& a, const LabVoice& b, float& trimA, float& trimB);

}  // namespace rc
