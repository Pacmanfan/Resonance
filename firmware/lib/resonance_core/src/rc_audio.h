// Resonance portable core: the audio engine. Mixes three layers - the
// binaural tone, background audio from a ring buffer, and noise - under a
// master volume and a limiter (RES-AUDIO-007, RES-AUDIO-008).
//
// Portable: no Arduino, ESP-IDF or display header.
//
// Threads: setters from the interface task, render() from the audio output's
// task; values cross as single atomics, picked up once per block.
#pragma once

#include <atomic>
#include <stddef.h>
#include <stdint.h>

#include "rc_noise.h"
#include "rc_ring.h"
#include "rc_tone.h"

namespace rc {

// Layer ceilings as fractions of full scale. Tone 0.5 (rc_tone.h), background
// 0.6, noise 0.3: all three at maximum sum to 1.4, which the limiter folds
// back under full scale.
constexpr float kBackgroundCeiling = 0.6f;
constexpr float kNoiseCeiling = 0.3f;
// The limiter is transparent below this level and bends smoothly above it.
constexpr float kLimiterKnee = 0.7f;
// Hard ceiling of the output, as a fraction of full scale.
constexpr float kOutputCeiling = 0.98f;

class AudioEngine {
public:
    explicit AudioEngine(FrameRing& background);

    // The tone layer: frequencies, its own volume, on/off.
    ToneEngine& tone() { return _tone; }

    // Master volume in percent; square law.
    void setMasterVolume(uint8_t percent);

    // Level of the whole output as set by a running session: 1 normally,
    // falling to 0 during the fade at a session's end. Ramped, so it may be
    // updated a few times a second without steps.
    void setSessionGain(float gain);

    // Background layer: volume in percent, and whether it is being played.
    // While not playing the ring is not read, so a paused track keeps its
    // place. Both changes are faded.
    void setBackgroundVolume(uint8_t percent);
    void setBackgroundPlaying(bool playing);

    // Tells the engine that the track is ending: the ring running empty is
    // then expected and is not counted as an underrun. Cleared by the next
    // setBackgroundPlaying().
    void setBackgroundEnding() { _bgEnding.store(true, std::memory_order_relaxed); }

    // Noise layer.
    void setNoise(NoiseType type, uint8_t percent);

    // Writes `frames` stereo 16-bit frames. Never blocks.
    void render(int16_t* interleaved, size_t frames);

    // Blocks in which the background layer was playing but its ring ran dry.
    uint32_t underruns() const { return _underruns.load(std::memory_order_relaxed); }

    // Background frames consumed since the last resetPosition().
    uint32_t backgroundFrames() const { return _bgFrames.load(std::memory_order_relaxed); }
    void resetPosition() { _bgFrames.store(0, std::memory_order_relaxed); }

    // True when the background fade-out after setBackgroundPlaying(false) is
    // complete, so the ring may be refilled from another file.
    bool backgroundIdle() const { return _bgIdle.load(std::memory_order_relaxed); }

private:
    ToneEngine _tone;
    FrameRing& _ring;
    NoiseGenerator _noise;

    std::atomic<float> _master;
    std::atomic<float> _sessionGain;
    std::atomic<float> _bgGain;
    std::atomic<bool> _bgPlaying;
    std::atomic<float> _noiseGain;
    std::atomic<uint8_t> _noiseType;
    std::atomic<uint32_t> _underruns;
    std::atomic<uint32_t> _bgFrames;
    std::atomic<bool> _bgIdle;
    std::atomic<bool> _bgEnding{false};

    // Owned by render().
    Ramp _masterRamp;
    Ramp _sessionRamp;
    Ramp _bgRamp;
    Ramp _bgGate;
    Ramp _noiseRamp;
    bool _bgGateOn = false;
};

// The limiter's transfer curve, for one sample in units of full scale.
float limit(float x);

}  // namespace rc
