// Resonance portable core: the audio engine. Mixes four layers - the
// binaural tone, its golden partials, a recording from a ring buffer, and
// noise - under a master volume and a limiter (RES-AUDIO-007, RES-AUDIO-008,
// RES-AUDIO-016).
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

// The mixer's layers (RES-AUDIO-016). "Voice" is whatever recording is
// played from the card: a guided voice or an ambient sound.
enum class Layer : uint8_t { Tone = 0, Partials = 1, Noise = 2, Voice = 3 };
constexpr uint8_t kLayerCount = 4;
// Time a layer takes to come in or go out when it is switched.
constexpr float kLayerFadeDefaultS = 1.0f;
constexpr float kLayerFadeMaxS = 30.0f;
// Time the recording takes to come in or go out when playback starts,
// pauses or stops, unless another is given.
constexpr float kPlayFadeS = 0.05f;

class AudioEngine {
public:
    explicit AudioEngine(FrameRing& background);

    // The tone layer: frequencies, its own volume, on/off.
    ToneEngine& tone() { return _tone; }

    // A layer on or off, over the layer's fade time. A layer switched off
    // keeps its level, and - for the recording - goes on playing unheard.
    void setLayerEnabled(Layer layer, bool on);
    bool layerEnabled(Layer layer) const { return _layerOn[static_cast<uint8_t>(layer)]; }
    // A layer's level in percent; square law. Tone and partials start at
    // 100, so that the tone volume and the partials' own levels alone decide.
    void setLayerLevel(Layer layer, uint8_t percent);
    uint8_t layerLevel(Layer layer) const { return _layerLevel[static_cast<uint8_t>(layer)]; }
    // The layer's fade time in seconds, 0..kLayerFadeMaxS.
    void setLayerFade(Layer layer, float seconds);
    float layerFade(Layer layer) const { return _layerFadeS[static_cast<uint8_t>(layer)]; }

    // Master volume in percent; square law.
    void setMasterVolume(uint8_t percent);

    // Level of the whole output as set by a running session: 1 normally,
    // falling to 0 during the fade at a session's end. Ramped, so it may be
    // updated a few times a second without steps.
    void setSessionGain(float gain);

    // The recording: volume in percent (the Voice layer's level), and
    // whether it is being played. While not playing the ring is not read, so
    // a paused track keeps its place. Both changes are faded; playback comes
    // in or goes out over `fadeS` (RES-PLAY-007).
    void setBackgroundVolume(uint8_t percent) { setLayerLevel(Layer::Voice, percent); }
    void setBackgroundPlaying(bool playing, float fadeS = kPlayFadeS);

    // Tells the engine that the track is ending: the ring running empty is
    // then expected and is not counted as an underrun. Cleared by the next
    // setBackgroundPlaying().
    void setBackgroundEnding() { _bgEnding.store(true, std::memory_order_relaxed); }

    // Noise: its type and the Noise layer's level.
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
    void applyLayer(Layer layer, float seconds);

    // Written and read by the interface task only.
    bool _layerOn[kLayerCount] = {true, true, true, true};
    uint8_t _layerLevel[kLayerCount] = {100, 100, 0, 0};
    float _layerFadeS[kLayerCount] = {kLayerFadeDefaultS, kLayerFadeDefaultS, kLayerFadeDefaultS, kLayerFadeDefaultS};
    NoiseType _noiseWanted = NoiseType::Off;

    ToneEngine _tone;
    FrameRing& _ring;
    NoiseGenerator _noise;

    std::atomic<float> _master;
    std::atomic<float> _sessionGain;
    // A gain crosses as value, time and a version bumped after both.
    std::atomic<float> _bgGain;
    std::atomic<float> _bgSeconds;
    std::atomic<uint32_t> _bgVersion{0};
    std::atomic<bool> _bgPlaying;
    std::atomic<float> _bgGateSeconds;
    std::atomic<float> _noiseGain;
    std::atomic<float> _noiseSeconds;
    std::atomic<uint32_t> _noiseVersion{0};
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
    uint32_t _bgSeen = 0;
    uint32_t _noiseSeen = 0;
};

// The limiter's transfer curve, for one sample in units of full scale.
float limit(float x);

}  // namespace rc
