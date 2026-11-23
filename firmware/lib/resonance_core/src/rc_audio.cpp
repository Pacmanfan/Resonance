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
    : _ring(background), _master(0.0f), _sessionGain(1.0f), _bgGain(0.0f), _bgSeconds(kRampS), _bgPlaying(false),
      _bgGateSeconds(kPlayFadeS), _noiseGain(0.0f), _noiseSeconds(kRampS * 4),
      _noiseType(static_cast<uint8_t>(NoiseType::Off)), _underruns(0), _bgFrames(0), _bgIdle(true) {
    _tone.setVolume(kVolumeMax);
    _masterRamp.jump(0.0f);
    _sessionRamp.jump(1.0f);
    _breathRamp.jump(1.0f);
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

void AudioEngine::setBreathGain(float gain) {
    if (!(gain >= 0.2f)) gain = 0.2f;  // also catches NaN
    if (gain > 1.0f) gain = 1.0f;
    _breathGain.store(gain, std::memory_order_relaxed);
}

void AudioEngine::setLayerEnabled(Layer layer, bool on) {
    const uint8_t i = static_cast<uint8_t>(layer);
    if (i >= kLayerCount || _layerOn[i] == on) return;
    _layerOn[i] = on;
    applyLayer(layer, _layerFadeS[i]);
}

void AudioEngine::setLayerLevel(Layer layer, uint8_t percent) {
    const uint8_t i = static_cast<uint8_t>(layer);
    if (i >= kLayerCount) return;
    _layerLevel[i] = percent > kVolumeMax ? kVolumeMax : percent;
    // A level step is quick; the noise keeps the longer ramp it always had.
    applyLayer(layer, layer == Layer::Noise ? kRampS * 4 : kRampS);
}

void AudioEngine::setLayerFade(Layer layer, float seconds) {
    const uint8_t i = static_cast<uint8_t>(layer);
    if (i >= kLayerCount || !(seconds >= 0.0f)) return;  // also refuses NaN
    _layerFadeS[i] = seconds > kLayerFadeMaxS ? kLayerFadeMaxS : seconds;
}

// Hands the layer's gain to whoever renders it, to be reached in `seconds`.
void AudioEngine::applyLayer(Layer layer, float seconds) {
    const uint8_t i = static_cast<uint8_t>(layer);
    const float gain = _layerOn[i] ? squareLaw(_layerLevel[i]) : 0.0f;
    switch (layer) {
        case Layer::Tone:
            _tone.setFundamentalGain(gain, seconds);
            break;
        case Layer::Partials:
            _tone.setPartialsGain(gain, seconds);
            break;
        case Layer::Noise:
            _noiseGain.store(_noiseWanted == NoiseType::Off ? 0.0f : gain * kNoiseCeiling, std::memory_order_relaxed);
            _noiseSeconds.store(seconds, std::memory_order_relaxed);
            _noiseVersion.fetch_add(1, std::memory_order_release);
            break;
        case Layer::Voice:
            _bgGain.store(gain * kBackgroundCeiling, std::memory_order_relaxed);
            _bgSeconds.store(seconds, std::memory_order_relaxed);
            _bgVersion.fetch_add(1, std::memory_order_release);
            break;
    }
}

void AudioEngine::setBackgroundPlaying(bool playing, float fadeS) {
    if (!(fadeS >= 0.0f)) fadeS = 0.0f;
    if (fadeS > kLayerFadeMaxS) fadeS = kLayerFadeMaxS;
    if (playing) _bgIdle.store(false, std::memory_order_relaxed);
    else _bgEnding.store(false, std::memory_order_relaxed);
    _bgGateSeconds.store(fadeS, std::memory_order_relaxed);
    _bgPlaying.store(playing, std::memory_order_release);
}

void AudioEngine::setNoise(NoiseType type, uint8_t percent) {
    _noiseWanted = type;
    _noiseType.store(static_cast<uint8_t>(type), std::memory_order_relaxed);
    setLayerLevel(Layer::Noise, percent);
}

void AudioEngine::render(int16_t* out, size_t frames) {
    _masterRamp.moveTo(_master.load(std::memory_order_relaxed), samplesFor(kRampS));
    // The session level is updated about ten times a second; a ramp of twice
    // that interval joins the updates into a smooth line.
    _sessionRamp.moveTo(_sessionGain.load(std::memory_order_relaxed), samplesFor(0.2f));
    _breathRamp.moveTo(_breathGain.load(std::memory_order_relaxed), samplesFor(0.2f));
    uint32_t version = _bgVersion.load(std::memory_order_acquire);
    if (version != _bgSeen) {
        _bgSeen = version;
        _bgRamp.moveTo(_bgGain.load(std::memory_order_relaxed), samplesFor(_bgSeconds.load(std::memory_order_relaxed)));
    }
    version = _noiseVersion.load(std::memory_order_acquire);
    if (version != _noiseSeen) {
        _noiseSeen = version;
        _noiseRamp.moveTo(_noiseGain.load(std::memory_order_relaxed),
                          samplesFor(_noiseSeconds.load(std::memory_order_relaxed)));
    }
    const bool playing = _bgPlaying.load(std::memory_order_acquire);
    if (playing != _bgGateOn) {
        _bgGateOn = playing;
        _bgGate.moveTo(playing ? 1.0f : 0.0f, samplesFor(_bgGateSeconds.load(std::memory_order_relaxed)));
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
        const bool lab = _lab.begin(n);
        for (size_t i = 0; i < n; ++i) {
            const float bg = _bgRamp.next() * _bgGate.next() * (1.0f / 32768.0f);
            const float noise = noiseOn ? _noise.next() * _noiseRamp.next() : (_noiseRamp.next(), 0.0f);
            const float master = _masterRamp.next() * _sessionRamp.next();
            // The breathing modulation: exactly 1 unless it is in use, and a
            // multiplication by 1 changes no sample.
            const float breath = _breathRamp.next();
            // Noise is the same in both ears; tone and background keep their
            // own channels.
            // The order of the sum is the one it always had.
            const float hush = noise * breath;
            float l, r;
            if (lab) {
                // The Lab works on what is generated; the recording joins after it.
                float gl = static_cast<float>(out[2 * i]) * (1.0f / 32768.0f) * breath + hush;
                float gr = static_cast<float>(out[2 * i + 1]) * (1.0f / 32768.0f) * breath + hush;
                _lab.process(gl, gr);
                l = (gl + static_cast<float>(background[2 * i]) * bg) * master;
                r = (gr + static_cast<float>(background[2 * i + 1]) * bg) * master;
            } else {
                l = (static_cast<float>(out[2 * i]) * (1.0f / 32768.0f) * breath +
                     static_cast<float>(background[2 * i]) * bg + hush) * master;
                r = (static_cast<float>(out[2 * i + 1]) * (1.0f / 32768.0f) * breath +
                     static_cast<float>(background[2 * i + 1]) * bg + hush) * master;
            }
            out[2 * i] = static_cast<int16_t>(lrintf(limit(l) * 32767.0f));
            out[2 * i + 1] = static_cast<int16_t>(lrintf(limit(r) * 32767.0f));
        }
        out += n * 2;
        frames -= n;
    }
}

}  // namespace rc
