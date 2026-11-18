#include "rc_tone.h"

#include <math.h>

#include "rc_settings.h"

namespace rc {

namespace {

constexpr int kTableBits = 10;
constexpr int kTableSize = 1 << kTableBits;
constexpr int kFractionBits = 32 - kTableBits;
constexpr float kFractionScale = 1.0f / static_cast<float>(1u << kFractionBits);

// One period of a sine, plus one extra entry so that interpolation needs no
// wrap. Built on first use: 4 kB of RAM, cheaper than keeping it in flash and
// reading it through the cache on every sample.
const float* sineTable() {
    static float table[kTableSize + 1];
    static bool built = false;
    if (!built) {
        for (int i = 0; i <= kTableSize; ++i) {
            table[i] = static_cast<float>(sin(2.0 * M_PI * i / kTableSize));
        }
        built = true;
    }
    return table;
}

uint32_t samplesFor(float seconds) {
    return static_cast<uint32_t>(seconds * static_cast<float>(kSampleRate));
}

}  // namespace

void Oscillator::setFrequency(float hz) {
    // increment = hz / sampleRate * 2^32, computed in double: a float has too
    // few bits to hold the result exactly.
    _increment = static_cast<uint32_t>(static_cast<double>(hz) * (4294967296.0 / kSampleRate));
}

float Oscillator::next() {
    const float* table = sineTable();
    const uint32_t index = _phase >> kFractionBits;
    const float fraction = static_cast<float>(_phase & ((1u << kFractionBits) - 1)) * kFractionScale;
    const float a = table[index];
    const float b = table[index + 1];
    _phase += _increment;
    return a + (b - a) * fraction;
}

void Ramp::moveTo(float target, uint32_t samples) {
    if (target == _target && _left != 0) return;  // already on the way there
    _target = target;
    if (samples == 0 || target == _value) {
        _value = target;
        _left = 0;
        _step = 0.0f;
        return;
    }
    _left = samples;
    _step = (target - _value) / static_cast<float>(samples);
}

float Ramp::next() {
    if (_left == 0) return _value;
    _value += _step;
    if (--_left == 0) _value = _target;  // land exactly, without rounding drift
    return _value;
}

ToneEngine::ToneEngine()
    : _primaryHz(kPrimaryDefaultHz), _beatHz(kBeatDefaultHz), _amplitude(0.0f), _enabled(false) {
    sineTable();  // built here, not inside the first render()
    _leftHz.jump(kPrimaryDefaultHz);
    _rightHz.jump(kPrimaryDefaultHz + kBeatDefaultHz);
    _level.jump(0.0f);
    _gate.jump(0.0f);
}

void ToneEngine::setFrequencies(float primaryHz, float beatHz) {
    if (!isfinite(primaryHz) || !isfinite(beatHz)) return;
    _primaryHz.store(sanitizeFrequency(primaryHz, kPrimaryMinHz, kPrimaryMaxHz, kPrimaryDefaultHz),
                     std::memory_order_relaxed);
    _beatHz.store(sanitizeFrequency(beatHz, kBeatMinHz, kBeatMaxHz, kBeatDefaultHz), std::memory_order_relaxed);
}

void ToneEngine::setVolume(uint8_t percent) {
    if (percent > kVolumeMax) percent = kVolumeMax;
    const float fraction = static_cast<float>(percent) / static_cast<float>(kVolumeMax);
    _amplitude.store(fraction * fraction * kToneCeiling, std::memory_order_relaxed);
}

void ToneEngine::setEnabled(bool on) { _enabled.store(on, std::memory_order_relaxed); }

bool ToneEngine::render(int16_t* out, size_t frames) {
    // Pick up the targets once per block.
    const float primary = _primaryHz.load(std::memory_order_relaxed);
    const float beat = _beatHz.load(std::memory_order_relaxed);
    _leftHz.moveTo(primary, samplesFor(kFrequencyGlideS));
    _rightHz.moveTo(primary + beat, samplesFor(kFrequencyGlideS));
    _level.moveTo(_amplitude.load(std::memory_order_relaxed), samplesFor(kVolumeRampS));
    const bool on = _enabled.load(std::memory_order_relaxed);
    if (on != _gateOn) {
        _gateOn = on;
        _gate.moveTo(on ? 1.0f : 0.0f, samplesFor(kStartStopFadeS));
    }

    // Fully off: silence, and the oscillators rest at phase zero so that the
    // next start rises from a zero crossing.
    if (!_gateOn && _gate.settled() && _gate.value() == 0.0f) {
        for (size_t i = 0; i < frames * 2; ++i) out[i] = 0;
        _left.reset();
        _right.reset();
        _leftHz.jump(primary);
        _rightHz.jump(primary + beat);
        return true;
    }

    const bool gliding = !_leftHz.settled() || !_rightHz.settled();
    if (!gliding) {
        _left.setFrequency(_leftHz.value());
        _right.setFrequency(_rightHz.value());
    }
    for (size_t i = 0; i < frames; ++i) {
        if (gliding) {
            _left.setFrequency(_leftHz.next());
            _right.setFrequency(_rightHz.next());
        }
        const float gain = _level.next() * _gate.next() * 32767.0f;
        // Triangular dither of one least-significant bit, so that a quiet
        // sine does not turn into quantisation tones.
        _noise = _noise * 1664525u + 1013904223u;
        const float d1 = static_cast<float>(_noise >> 16) * (1.0f / 65536.0f);
        _noise = _noise * 1664525u + 1013904223u;
        const float d2 = static_cast<float>(_noise >> 16) * (1.0f / 65536.0f);
        const float dither = d1 - d2;
        // The two channels are never summed or cross-fed.
        float l = _left.next() * gain + dither;
        float r = _right.next() * gain + dither;
        if (l > 32767.0f) l = 32767.0f; else if (l < -32768.0f) l = -32768.0f;
        if (r > 32767.0f) r = 32767.0f; else if (r < -32768.0f) r = -32768.0f;
        out[2 * i] = static_cast<int16_t>(lrintf(l));
        out[2 * i + 1] = static_cast<int16_t>(lrintf(r));
    }
    return false;
}

}  // namespace rc
