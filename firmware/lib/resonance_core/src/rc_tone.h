// Resonance portable core: the binaural tone engine.
// RES-AUDIO-002 (two independent oscillators: left f0, right f0 + fb),
// RES-AUDIO-003 (phase-continuous frequency changes, faded level changes),
// RES-AUDIO-005 (invalid values rejected), RES-AUDIO-008 (bounded output),
// RES-AUDIO-012 (render never blocks, allocates or logs).
//
// Portable: this file must not include an Arduino, ESP-IDF or display header.
//
// Threads: the setters are called from the interface task, render() from the
// audio output's task. Each setter stores one 32-bit value atomically and
// render() picks the values up at the start of a block, so no lock is needed.
#pragma once

#include <atomic>
#include <stddef.h>
#include <stdint.h>

namespace rc {

constexpr uint32_t kSampleRate = 44100;  // fixed by Bluetooth A2DP

// Full-scale ceiling of the tone, as a fraction of 16-bit full scale. Leaves
// headroom for the layers that will be mixed in later.
constexpr float kToneCeiling = 0.5f;

// Fade times.
constexpr float kFrequencyGlideS = 0.05f;  // a frequency step is spread over this
constexpr float kVolumeRampS = 0.05f;      // a volume step
constexpr float kStartStopFadeS = 0.5f;    // audio on / off

// One sine oscillator: a 32-bit phase accumulator read through a table with
// linear interpolation. Changing the frequency changes only the increment,
// never the phase, so the waveform has no discontinuity.
class Oscillator {
public:
    void setFrequency(float hz);
    float next();
    void reset() { _phase = 0; }

private:
    uint32_t _phase = 0;
    uint32_t _increment = 0;
};

// A value that moves to its target in a straight line over a given number of
// samples.
class Ramp {
public:
    void jump(float value) { _value = _target = value; _step = 0.0f; _left = 0; }
    void moveTo(float target, uint32_t samples);
    float next();
    float value() const { return _value; }
    bool settled() const { return _left == 0; }

private:
    float _value = 0.0f;
    float _target = 0.0f;
    float _step = 0.0f;
    uint32_t _left = 0;
};

class ToneEngine {
public:
    ToneEngine();

    // Primary frequency (left ear) and beat frequency; the right ear gets
    // their sum. Values are brought into range; a value that is not finite
    // is ignored.
    void setFrequencies(float primaryHz, float beatHz);

    // Volume in percent, 0-100. The amplitude follows the square of it.
    void setVolume(uint8_t percent);

    // Audio on or off, with a fade. Off is the state after construction.
    void setEnabled(bool on);
    bool enabled() const { return _enabled.load(std::memory_order_relaxed); }

    // Writes `frames` stereo frames of signed 16-bit samples, left first.
    // Never blocks. Returns true when the block contains only silence.
    bool render(int16_t* interleaved, size_t frames);

private:
    std::atomic<float> _primaryHz;
    std::atomic<float> _beatHz;
    std::atomic<float> _amplitude;  // target, 0..kToneCeiling
    std::atomic<bool> _enabled;

    // Owned by render().
    Oscillator _left;
    Oscillator _right;
    Ramp _leftHz;
    Ramp _rightHz;
    Ramp _level;  // volume
    Ramp _gate;   // 0 = off, 1 = on
    bool _gateOn = false;
    uint32_t _noise = 0x1234567u;  // dither generator state
};

}  // namespace rc
