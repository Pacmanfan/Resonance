#include "rc_audio.h"

#include <math.h>

#include "rc_settings.h"

namespace rc {

namespace {

constexpr size_t kBlock = 128;  // frames rendered per inner pass; sets the stack use
constexpr float kRampS = 0.05f;

uint32_t samplesFor(float seconds) { return static_cast<uint32_t>(seconds * static_cast<float>(kSampleRate)); }

float squareLaw(uint8_t percent) {
    if (percent > kVolumeMax) percent = kVolumeMax;
    const float f = static_cast<float>(percent) / static_cast<float>(kVolumeMax);
    return f * f;
}

}  // namespace

float limit(float x) {
    const float magnitude = x < 0.0f ? -x : x;
    if (magnitude <= kLimiterKnee) return x;
    // Above the knee the curve approaches the ceiling asymptotically, with a
    // slope of one at the knee, so there is no corner in the waveform.
    const float range = kOutputCeiling - kLimiterKnee;
    const float bent = kLimiterKnee + range * tanhf((magnitude - kLimiterKnee) / range);
    return x < 0.0f ? -bent : bent;
}

AudioEngine::AudioEngine(FrameRing& background)
    : _ring(background), _master(0.0f), _sessionGain(1.0f), _bgGain(0.0f), _bgPlaying(false), _noiseGain(0.0f),
      _noiseType(static_cast<uint8_t>(NoiseType::Off)), _underruns(0), _bgFrames(0), _bgIdle(true) {
    _tone.setVolume(kVolumeMax);
    _masterRamp.jump(0.0f);
    _sessionRamp.jump(1.0f);
    _bgRamp.jump(0.0f);
    _bgGate.jump(0.0f);
    _noiseRamp.jump(0.0f);
}

void AudioEngine::setMasterVolume(uint8_t percent) { _master.store(squareLaw(percent), std::memory_order_relaxed); }

void AudioEngine::setSessionGain(float gain) {
    if (!(gain >= 0.0f)) gain = 0.0f;  // also catches NaN
    if (gain > 1.0f) gain = 1.0f;
    _sessionGain.store(gain, std::memory_order_relaxed);
}

void AudioEngine::setBackgroundVolume(uint8_t percent) {
    _bgGain.store(squareLaw(percent) * kBackgroundCeiling, std::memory_order_relaxed);
}

void AudioEngine::setBackgroundPlaying(bool playing) {
    if (playing) _bgIdle.store(false, std::memory_order_relaxed);
    else _bgEnding.store(false, std::memory_order_relaxed);
    _bgPlaying.store(playing, std::memory_order_relaxed);
}

void AudioEngine::setNoise(NoiseType type, uint8_t percent) {
    _noiseType.store(static_cast<uint8_t>(type), std::memory_order_relaxed);
    _noiseGain.store(type == NoiseType::Off ? 0.0f : squareLaw(percent) * kNoiseCeiling, std::memory_order_relaxed);
}

void AudioEngine::render(int16_t* out, size_t frames) {
    _masterRamp.moveTo(_master.load(std::memory_order_relaxed), samplesFor(kRampS));
    // The session level is updated about ten times a second; a ramp of twice
    // that interval joins the updates into a smooth line.
    _sessionRamp.moveTo(_sessionGain.load(std::memory_order_relaxed), samplesFor(0.2f));
    _bgRamp.moveTo(_bgGain.load(std::memory_order_relaxed), samplesFor(kRampS));
    _noiseRamp.moveTo(_noiseGain.load(std::memory_order_relaxed), samplesFor(kRampS * 4));
    const bool playing = _bgPlaying.load(std::memory_order_relaxed);
    if (playing != _bgGateOn) {
        _bgGateOn = playing;
        _bgGate.moveTo(playing ? 1.0f : 0.0f, samplesFor(kRampS));
    }
    const NoiseType wanted = static_cast<NoiseType>(_noiseType.load(std::memory_order_relaxed));
    // The noise type changes only while its gain is at zero, so a change of
    // type is never heard as a jump.
    if (wanted != _noise.type() && (_noiseRamp.value() == 0.0f || _noise.type() == NoiseType::Off)) {
        _noise.setType(wanted);
    }

    int16_t background[kBlock * 2];
    while (frames > 0) {
        const size_t n = frames < kBlock ? frames : kBlock;
        _tone.render(out, n);  // the tone layer, already at its own level

        // The ring is read only while the background is audible, so a pause
        // keeps the track's place.
        const bool bgAudible = _bgGateOn || !_bgGate.settled();
        size_t got = 0;
        if (bgAudible) {
            got = _ring.read(background, static_cast<uint32_t>(n));
            if (got < n && _bgGateOn && !_bgEnding.load(std::memory_order_relaxed)) {
                _underruns.fetch_add(1, std::memory_order_relaxed);
            }
            _bgFrames.fetch_add(static_cast<uint32_t>(got), std::memory_order_relaxed);
        } else if (!_bgIdle.load(std::memory_order_relaxed)) {
            _bgIdle.store(true, std::memory_order_relaxed);
        }
        for (size_t i = got * 2; i < n * 2; ++i) background[i] = 0;

        const bool noiseOn = _noise.type() != NoiseType::Off;
        for (size_t i = 0; i < n; ++i) {
            const float bg = _bgRamp.next() * _bgGate.next() * (1.0f / 32768.0f);
            const float noise = noiseOn ? _noise.next() * _noiseRamp.next() : (_noiseRamp.next(), 0.0f);
            const float master = _masterRamp.next() * _sessionRamp.next();
            // Noise is the same in both ears; tone and background keep their
            // own channels.
            const float l = (static_cast<float>(out[2 * i]) * (1.0f / 32768.0f) +
                             static_cast<float>(background[2 * i]) * bg + noise) * master;
            const float r = (static_cast<float>(out[2 * i + 1]) * (1.0f / 32768.0f) +
                             static_cast<float>(background[2 * i + 1]) * bg + noise) * master;
            out[2 * i] = static_cast<int16_t>(lrintf(limit(l) * 32767.0f));
            out[2 * i + 1] = static_cast<int16_t>(lrintf(limit(r) * 32767.0f));
        }
        out += n * 2;
        frames -= n;
    }
}

}  // namespace rc
