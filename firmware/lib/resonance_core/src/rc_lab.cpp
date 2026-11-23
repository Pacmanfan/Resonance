#include "rc_lab.h"

#include <math.h>

#include "rc_settings.h"

namespace rc {

namespace {

constexpr float kGlideS = 0.05f;
uint32_t samplesFor(float seconds) { return static_cast<uint32_t>(seconds * static_cast<float>(kSampleRate)); }

// 0..1 with no corner at either end.
float eased(float fraction) {
    if (fraction <= 0.0f) return 0.0f;
    if (fraction >= 1.0f) return 1.0f;
    return 0.5f - 0.5f * cosf(3.14159265f * fraction);
}

}  // namespace

bool sanitizeLab(LabSettings& s) {
    const LabSettings before = s;
    if (s.widthPercent > 100) s.widthPercent = 100;
    if (s.delayUs > kLabDelayMaxUs) s.delayUs = kLabDelayMaxUs;
    if (s.delayUs < -kLabDelayMaxUs) s.delayUs = -kLabDelayMaxUs;
    if (s.bilateralPeriodDs != 0 && s.bilateralPeriodDs < kBilateralMinDs) s.bilateralPeriodDs = kBilateralMinDs;
    if (s.bilateralPeriodDs > kBilateralMaxDs) s.bilateralPeriodDs = kBilateralMaxDs;
    if (s.bilateralDepth > 100) s.bilateralDepth = 100;
    return s.widthPercent != before.widthPercent || s.delayUs != before.delayUs ||
           s.bilateralPeriodDs != before.bilateralPeriodDs || s.bilateralDepth != before.bilateralDepth;
}

bool labAtRest(const LabSettings& s) {
    return s.widthPercent >= 100 && s.delayUs == 0 && (s.bilateralPeriodDs == 0 || s.bilateralDepth == 0);
}

float bilateralSide(float position, bool golden) {
    if (!(position >= 0.0f)) position = 0.0f;  // also catches NaN
    position -= floorf(position);
    const float share = golden ? kGoldenShare : 0.5f;  // the left's part of the period
    const float half = kBilateralCrossing * 0.5f;
    // The crossings are centred on the two boundaries: to the right at
    // `share`, back to the left at the end of the period.
    if (position < half) return -eased(0.5f + position / kBilateralCrossing) * 2.0f + 1.0f;
    if (position < share - half) return -1.0f;
    if (position < share + half) return eased((position - (share - half)) / kBilateralCrossing) * 2.0f - 1.0f;
    if (position < 1.0f - half) return 1.0f;
    return 1.0f - eased((position - (1.0f - half)) / kBilateralCrossing) * 2.0f;
}

Lab::Lab()
    : _widthTarget(1.0f), _delayTarget(0.0f), _periodSamples(0.0f), _depthTarget(0.0f), _golden(true), _trimTarget(1.0f) {
    _width.jump(1.0f);
    _trim.jump(1.0f);
    _depth.jump(0.0f);
    for (uint8_t c = 0; c < 2; ++c) {
        _delay[c].jump(0.0f);
        _side[c].jump(1.0f);
    }
}

void Lab::set(const LabSettings& given) {
    LabSettings s = given;
    sanitizeLab(s);
    _widthTarget.store(static_cast<float>(s.widthPercent) * 0.01f, std::memory_order_relaxed);
    _delayTarget.store(static_cast<float>(s.delayUs) * 1e-6f * static_cast<float>(kSampleRate), std::memory_order_relaxed);
    _depthTarget.store(static_cast<float>(s.bilateralDepth) * 0.01f, std::memory_order_relaxed);
    _golden.store(s.bilateralGolden, std::memory_order_relaxed);
    _periodSamples.store(static_cast<float>(s.bilateralPeriodDs) * 0.1f * static_cast<float>(kSampleRate),
                         std::memory_order_relaxed);
}

void Lab::setTrim(float gain) {
    if (!(gain >= 0.0f)) gain = 0.0f;  // also catches NaN
    if (gain > 1.0f) gain = 1.0f;
    _trimTarget.store(gain, std::memory_order_relaxed);
}

bool Lab::begin(size_t frames) {
    const float width = _widthTarget.load(std::memory_order_relaxed);
    const float delay = _delayTarget.load(std::memory_order_relaxed);
    const float period = _periodSamples.load(std::memory_order_relaxed);
    _depth.moveTo(period > 0.0f ? _depthTarget.load(std::memory_order_relaxed) : 0.0f, samplesFor(0.3f));
    for (size_t i = 0; i < frames; ++i) _depth.next();
    const float depth = _depth.value();
    const float trim = _trimTarget.load(std::memory_order_relaxed);
    _width.moveTo(width, samplesFor(kGlideS));
    _trim.moveTo(trim, samplesFor(kGlideS));
    // A delay is glided: the pitch bends a little for a moment, and nothing steps.
    _delay[0].moveTo(delay < 0.0f ? -delay : 0.0f, samplesFor(kGlideS * 2));
    _delay[1].moveTo(delay > 0.0f ? delay : 0.0f, samplesFor(kGlideS * 2));

    // The alternation: where it stands at the end of this block, reached in a
    // straight line across the block.
    float gainLeft = 1.0f, gainRight = 1.0f;
    if (depth > 0.0f && period > 0.0f) {
        _position += static_cast<float>(frames) / period;
        _position -= floorf(_position);
        const float side = bilateralSide(_position, _golden.load(std::memory_order_relaxed));
        gainLeft = 1.0f - depth * (1.0f + side) * 0.5f;
        gainRight = 1.0f - depth * (1.0f - side) * 0.5f;
    } else if (_side[0].settled() && _side[1].settled() && _side[0].value() == 1.0f && _side[1].value() == 1.0f) {
        _position = 0.0f;  // the next start begins on the left
    }
    const uint32_t reach = depth > 0.0f ? static_cast<uint32_t>(frames) : samplesFor(kGlideS);
    _side[0].moveTo(gainLeft, reach);
    _side[1].moveTo(gainRight, reach);

    const bool delaying = delay != 0.0f || !_delay[0].settled() || !_delay[1].settled() || _delay[0].value() != 0.0f ||
                          _delay[1].value() != 0.0f;
    const bool active = delaying || width != 1.0f || !_width.settled() || _width.value() != 1.0f || trim != 1.0f ||
                        !_trim.settled() || _trim.value() != 1.0f || depth > 0.0f || !_side[0].settled() ||
                        !_side[1].settled() || _side[0].value() != 1.0f || _side[1].value() != 1.0f;
    if (delaying && !_delaying) {
        // The line holds what was heard long ago: start from silence instead.
        // The delay itself starts at zero, so nothing of it is read yet.
        for (uint8_t c = 0; c < 2; ++c) {
            for (uint32_t i = 0; i < kDelayLength; ++i) _line[c][i] = 0.0f;
        }
    }
    _delaying = delaying;
    _active = active;
    return active;
}

void Lab::process(float& left, float& right) {
    float in[2] = {left, right};
    // Width: what the ears share stays, what differs is scaled.
    const float width = _width.next();
    if (width != 1.0f) {
        const float mid = (in[0] + in[1]) * 0.5f;
        const float side = (in[0] - in[1]) * 0.5f * width;
        in[0] = mid + side;
        in[1] = mid - side;
    }
    if (_delaying) {
        for (uint8_t c = 0; c < 2; ++c) {
            _line[c][_write] = in[c];
            const float d = _delay[c].next();
            if (d > 0.0f) {
                // Read `d` samples back, between two stored samples.
                const uint32_t whole = static_cast<uint32_t>(d);
                const float fraction = d - static_cast<float>(whole);
                const float a = _line[c][(_write - whole) & (kDelayLength - 1)];
                const float b = _line[c][(_write - whole - 1) & (kDelayLength - 1)];
                in[c] = a + (b - a) * fraction;
            }
        }
        _write = (_write + 1) & (kDelayLength - 1);
    }
    const float trim = _trim.next();
    left = in[0] * _side[0].next() * trim;
    right = in[1] * _side[1].next() * trim;
}

float aWeight(float hz) {
    if (!(hz > 0.0f)) return 0.0f;
    const float f2 = hz * hz;
    const float ra = (12194.0f * 12194.0f * f2 * f2) /
                     ((f2 + 20.6f * 20.6f) * sqrtf((f2 + 107.7f * 107.7f) * (f2 + 737.9f * 737.9f)) * (f2 + 12194.0f * 12194.0f));
    return ra * 1.2589f;
}

float labLoudness(const LabVoice& v) {
    // Each ear: its fundamental and its partials, as powers that add.
    float ear[2] = {0.0f, 0.0f};
    for (uint8_t c = 0; c < 2; ++c) {
        const float offset = c == 0 ? 0.0f : v.beatHz;
        const float fundamental = v.toneGain * aWeight(v.primaryHz + offset);
        ear[c] = fundamental * fundamental;
        for (uint8_t p = 0; p < kPartialCount; ++p) {
            const float amplitude = v.partialsGain * static_cast<float>(v.partial[p]) * 0.01f * kPartialCeiling *
                                    aWeight(v.primaryHz * kPartialRatio[p] + offset);
            ear[c] += amplitude * amplitude;
        }
    }
    LabSettings lab = v.lab;
    sanitizeLab(lab);
    // Width mixes the ears: each hears its own part and some of the other's.
    const float w = static_cast<float>(lab.widthPercent) * 0.01f;
    const float own = (1.0f + w) * 0.5f, other = (1.0f - w) * 0.5f;
    float power = ((own * own + other * other) * (ear[0] + ear[1])) * 0.5f;
    // The alternation turns one ear down at a time.
    if (lab.bilateralPeriodDs != 0) {
        const float far = 1.0f - static_cast<float>(lab.bilateralDepth) * 0.01f;
        power *= (1.0f + far * far) * 0.5f;
    }
    return power;
}

void matchLoudness(const LabVoice& a, const LabVoice& b, float& trimA, float& trimB) {
    const float la = labLoudness(a), lb = labLoudness(b);
    trimA = trimB = 1.0f;
    if (!(la > 0.0f) || !(lb > 0.0f)) return;  // one is silent, or a value is not a number
    if (la > lb) trimA = sqrtf(lb / la);
    else trimB = sqrtf(la / lb);
}

}  // namespace rc
