// Resonance portable core: the binaural tone engine.
// RES-AUDIO-002 (two independent oscillators: left f0, right f0 + fb),
// RES-AUDIO-003 (phase-continuous frequency changes, faded level changes),
// RES-AUDIO-005 (invalid values rejected), RES-AUDIO-008 (bounded output),
// RES-AUDIO-012 (render never blocks, allocates or logs).
// RES-AUDIO-013 (phase offset and inversion of each channel), RES-AUDIO-014
// (golden partials), RES-AUDIO-015 (phase sweep). All three are off after
// construction, and the output is then exactly that of the plain engine.
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
constexpr float kPhaseGlideS = 0.05f;      // a phase offset step, unless a sweep time is given
constexpr float kInvertFadeS = 0.05f;      // an inversion passes through silence in this time

// Golden partials (RES-AUDIO-014): two per channel, at phi and phi squared
// times the primary frequency.
constexpr uint8_t kPartialCount = 2;
constexpr float kPartialRatio[kPartialCount] = {1.6180339887f, 2.6180339887f};
// Amplitude of a partial at 100 %, as a fraction of the fundamental's.
constexpr float kPartialCeiling = 0.5f;

// Limits of a phase offset and of the time a move to it may take.
constexpr float kPhaseMaxDegrees = 3600.0f;  // ten turns either way
constexpr float kPhaseSweepMaxS = 3600.0f;

enum class Channel : uint8_t { Left = 0, Right = 1 };

// One sine oscillator: a 32-bit phase accumulator read through a table with
// linear interpolation. Changing the frequency changes only the increment,
// never the phase, so the waveform has no discontinuity.
class Oscillator {
public:
    void setFrequency(float hz);
    // The same in single precision: accurate to about 0.0001 Hz, for use on
    // every sample while a frequency is gliding.
    void setFrequencyFast(float hz);
    float next() { return next(0); }
    // The value `offset` ahead of the phase (2^32 = one turn); the phase
    // itself advances as usual.
    float next(uint32_t offset);
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

// A phase offset that moves to its target in a straight line. Kept as an
// integer with 16 fractional bits below the oscillator's phase unit, so
// that a sweep lasting an hour neither stalls nor drifts.
class PhaseRamp {
public:
    void jump(float degrees);
    void moveTo(float degrees, uint32_t samples);
    // The offset to add to an oscillator's phase for this sample.
    uint32_t next();
    float inDegrees() const;  // not "degrees": Arduino defines a macro of that name
    bool settled() const { return _left == 0; }

private:
    int64_t _value = 0;
    int64_t _target = 0;
    int64_t _step = 0;
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

    // Phase offset of each channel's fundamental, in degrees (positive =
    // ahead), reached in `seconds`: a short glide by default, a phase sweep
    // when a longer time is given. Values out of range are brought into it;
    // a value that is not finite is ignored. While the audio is off the
    // offsets take their targets at once.
    void setPhase(float leftDegrees, float rightDegrees, float seconds = kPhaseGlideS);
    // Where the offset of `channel` is now, and whether one is still moving.
    float phaseDegrees(Channel channel) const {
        return _phaseNow[static_cast<uint8_t>(channel)].load(std::memory_order_relaxed);
    }
    bool phaseMoving() const { return _phaseMoving.load(std::memory_order_relaxed); }

    // Inverts a channel (fundamental and partials). The change fades through
    // silence, so there is no step.
    void setInverted(bool left, bool right);

    // Level of partial `index` (0 = phi, 1 = phi squared) in percent of
    // kPartialCeiling; 0 switches it off and costs nothing in render().
    void setPartial(uint8_t index, uint8_t percent);

    // Writes `frames` stereo frames of signed 16-bit samples, left first.
    // Never blocks. Returns true when the block contains only silence.
    bool render(int16_t* interleaved, size_t frames);

private:
    std::atomic<float> _primaryHz;
    std::atomic<float> _beatHz;
    std::atomic<float> _amplitude;  // target, 0..kToneCeiling
    std::atomic<bool> _enabled;
    std::atomic<float> _phaseTarget[2];
    std::atomic<float> _phaseSeconds;
    std::atomic<uint32_t> _phaseVersion;  // bumped after the three values above are written
    std::atomic<float> _phaseNow[2];
    std::atomic<bool> _phaseMoving;
    std::atomic<bool> _inverted[2];
    std::atomic<float> _partialGain[kPartialCount];  // target, 0..kPartialCeiling

    // Owned by render().
    Oscillator _left;
    Oscillator _right;
    Ramp _leftHz;
    Ramp _rightHz;
    Ramp _level;  // volume
    Ramp _gate;   // 0 = off, 1 = on
    bool _gateOn = false;
    uint32_t _phaseSeen = 0;
    PhaseRamp _phase[2];
    Ramp _sign[2];  // +1 normal, -1 inverted
    Oscillator _partialLeft[kPartialCount];
    Oscillator _partialRight[kPartialCount];
    Ramp _partialLevel[kPartialCount];
    uint32_t _noise = 0x1234567u;  // dither generator state
};

}  // namespace rc
