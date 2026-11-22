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

void Oscillator::setFrequencyFast(float hz) {
    _increment = static_cast<uint32_t>(hz * (4294967296.0f / static_cast<float>(kSampleRate)));
}

float Oscillator::next(uint32_t offset) {
    const float* table = sineTable();
    const uint32_t phase = _phase + offset;
    const uint32_t index = phase >> kFractionBits;
    const float fraction = static_cast<float>(phase & ((1u << kFractionBits) - 1)) * kFractionScale;
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

namespace {
// One degree in the unit of PhaseRamp: 2^48 a turn.
constexpr double kPhaseUnitsPerDegree = 281474976710656.0 / 360.0;
}  // namespace

void PhaseRamp::jump(float degrees) {
    _value = _target = static_cast<int64_t>(static_cast<double>(degrees) * kPhaseUnitsPerDegree);
    _step = 0;
    _left = 0;
}

void PhaseRamp::moveTo(float degrees, uint32_t samples) {
    const int64_t target = static_cast<int64_t>(static_cast<double>(degrees) * kPhaseUnitsPerDegree);
    _target = target;
    if (samples == 0 || target == _value) {
        _value = target;
        _left = 0;
        _step = 0;
        return;
    }
    _left = samples;
    _step = (target - _value) / static_cast<int64_t>(samples);
}

uint32_t PhaseRamp::next() {
    if (_left != 0) {
        _value += _step;
        if (--_left == 0) _value = _target;
    }
    // Drop the 16 fractional bits; the cast keeps the offset modulo a turn.
    return static_cast<uint32_t>(static_cast<uint64_t>(_value) >> 16);
}

float PhaseRamp::inDegrees() const { return static_cast<float>(static_cast<double>(_value) / kPhaseUnitsPerDegree); }

ToneEngine::ToneEngine()
    : _primaryHz(kPrimaryDefaultHz), _beatHz(kBeatDefaultHz), _amplitude(0.0f), _enabled(false),
      _phaseTarget{{0.0f}, {0.0f}}, _phaseSeconds(kPhaseGlideS), _phaseVersion(0), _phaseNow{{0.0f}, {0.0f}},
      _phaseMoving(false), _inverted{{false}, {false}}, _partialGain{{0.0f}, {0.0f}} {
    sineTable();  // built here, not inside the first render()
    for (uint8_t c = 0; c < 2; ++c) _sign[c].jump(1.0f);
    for (uint8_t p = 0; p < kPartialCount; ++p) _partialLevel[p].jump(0.0f);
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

void ToneEngine::setPhase(float leftDegrees, float rightDegrees, float seconds) {
    if (!isfinite(leftDegrees) || !isfinite(rightDegrees) || !isfinite(seconds)) return;
    const auto clamp = [](float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); };
    _phaseTarget[0].store(clamp(leftDegrees, -kPhaseMaxDegrees, kPhaseMaxDegrees), std::memory_order_relaxed);
    _phaseTarget[1].store(clamp(rightDegrees, -kPhaseMaxDegrees, kPhaseMaxDegrees), std::memory_order_relaxed);
    _phaseSeconds.store(clamp(seconds, 0.0f, kPhaseSweepMaxS), std::memory_order_relaxed);
    // Render reads the three values only after it has seen the version
    // change; a second call while it reads bumps the version again, so a
    // mixed reading is corrected one block later.
    _phaseVersion.fetch_add(1, std::memory_order_release);
}

void ToneEngine::setInverted(bool left, bool right) {
    _inverted[0].store(left, std::memory_order_relaxed);
    _inverted[1].store(right, std::memory_order_relaxed);
}

void ToneEngine::setPartial(uint8_t index, uint8_t percent) {
    if (index >= kPartialCount) return;
    if (percent > 100) percent = 100;
    _partialGain[index].store(static_cast<float>(percent) * 0.01f * kPartialCeiling, std::memory_order_relaxed);
}

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
    const uint32_t version = _phaseVersion.load(std::memory_order_acquire);
    if (version != _phaseSeen) {
        _phaseSeen = version;
        const uint32_t samples = samplesFor(_phaseSeconds.load(std::memory_order_relaxed));
        for (uint8_t c = 0; c < 2; ++c) _phase[c].moveTo(_phaseTarget[c].load(std::memory_order_relaxed), samples);
    }
    for (uint8_t c = 0; c < 2; ++c) {
        _sign[c].moveTo(_inverted[c].load(std::memory_order_relaxed) ? -1.0f : 1.0f, samplesFor(kInvertFadeS));
    }
    bool partialOn[kPartialCount];
    bool anyPartial = false;
    for (uint8_t p = 0; p < kPartialCount; ++p) {
        const float target = _partialGain[p].load(std::memory_order_relaxed);
        _partialLevel[p].moveTo(target, samplesFor(kVolumeRampS));
        partialOn[p] = target > 0.0f || _partialLevel[p].value() > 0.0f;
        anyPartial = anyPartial || partialOn[p];
    }

    // Fully off: silence, and the oscillators rest at phase zero so that the
    // next start rises from a zero crossing.
    if (!_gateOn && _gate.settled() && _gate.value() == 0.0f) {
        for (size_t i = 0; i < frames * 2; ++i) out[i] = 0;
        _left.reset();
        _right.reset();
        _leftHz.jump(primary);
        _rightHz.jump(primary + beat);
        for (uint8_t c = 0; c < 2; ++c) {
            _phase[c].jump(_phaseTarget[c].load(std::memory_order_relaxed));
            _sign[c].jump(_inverted[c].load(std::memory_order_relaxed) ? -1.0f : 1.0f);
            _phaseNow[c].store(_phase[c].inDegrees(), std::memory_order_relaxed);
        }
        for (uint8_t p = 0; p < kPartialCount; ++p) {
            _partialLeft[p].reset();
            _partialRight[p].reset();
            _partialLevel[p].jump(_partialGain[p].load(std::memory_order_relaxed));
        }
        _phaseMoving.store(false, std::memory_order_relaxed);
        return true;
    }

    const bool gliding = !_leftHz.settled() || !_rightHz.settled();
    if (!gliding) {
        _left.setFrequency(_leftHz.value());
        _right.setFrequency(_rightHz.value());
        // A partial pair keeps the beat of the fundamental: the right ear's
        // partial lies above the left ear's by the beat itself.
        const float beatNow = _rightHz.value() - _leftHz.value();
        for (uint8_t p = 0; p < kPartialCount; ++p) {
            if (!partialOn[p]) continue;
            _partialLeft[p].setFrequency(_leftHz.value() * kPartialRatio[p]);
            _partialRight[p].setFrequency(_leftHz.value() * kPartialRatio[p] + beatNow);
        }
    }
    const bool plain = !anyPartial && _phase[0].settled() && _phase[1].settled() && _sign[0].settled() &&
                       _sign[1].settled();
    const uint32_t restLeft = plain ? _phase[0].next() : 0;
    const uint32_t restRight = plain ? _phase[1].next() : 0;
    const float signLeft = _sign[0].value();
    const float signRight = _sign[1].value();
    for (size_t i = 0; i < frames; ++i) {
        if (gliding) {
            const float leftHz = _leftHz.next();
            const float rightHz = _rightHz.next();
            _left.setFrequency(leftHz);
            _right.setFrequency(rightHz);
            for (uint8_t p = 0; p < kPartialCount; ++p) {
                if (!partialOn[p]) continue;
                _partialLeft[p].setFrequencyFast(leftHz * kPartialRatio[p]);
                _partialRight[p].setFrequencyFast(leftHz * kPartialRatio[p] + (rightHz - leftHz));
            }
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
        float l, r;
        if (plain) {
            // Nothing is moving and no partial sounds: the plain engine,
            // with a constant offset and sign.
            l = _left.next(restLeft) * signLeft * gain + dither;
            r = _right.next(restRight) * signRight * gain + dither;
        } else {
            l = _left.next(_phase[0].next());
            r = _right.next(_phase[1].next());
            for (uint8_t p = 0; p < kPartialCount; ++p) {
                if (!partialOn[p]) continue;
                const float level = _partialLevel[p].next();
                l += _partialLeft[p].next() * level;
                r += _partialRight[p].next() * level;
            }
            l = l * _sign[0].next() * gain + dither;
            r = r * _sign[1].next() * gain + dither;
        }
        if (l > 32767.0f) l = 32767.0f; else if (l < -32768.0f) l = -32768.0f;
        if (r > 32767.0f) r = 32767.0f; else if (r < -32768.0f) r = -32768.0f;
        out[2 * i] = static_cast<int16_t>(lrintf(l));
        out[2 * i + 1] = static_cast<int16_t>(lrintf(r));
    }
    for (uint8_t c = 0; c < 2; ++c) _phaseNow[c].store(_phase[c].inDegrees(), std::memory_order_relaxed);
    _phaseMoving.store(!_phase[0].settled() || !_phase[1].settled(), std::memory_order_relaxed);
    return false;
}

}  // namespace rc
