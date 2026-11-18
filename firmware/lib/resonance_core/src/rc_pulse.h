// Resonance portable core: the pulse of the home-screen rings.
//
// The rings swell and settle at the beat frequency of the tone. A display
// drawn about thirty times a second cannot show a fast beat: above a few
// cycles per second the motion would alias into something unrelated. So the
// shown rate is the beat frequency halved until it is slow enough to draw
// about six frames per cycle; a 10 Hz beat is shown at 5 Hz, a 40 Hz beat
// also at 5 Hz, a 6 Hz beat at 3 Hz. Up to kPulseMaxHz the rings move at
// the beat itself.
//
// Each ring moves outwards from its resting radius by 0..kPulseTravel
// pixels, and each ring lags the one inside it, so that the pulse is seen
// travelling outwards.
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <math.h>
#include <stdint.h>

namespace rc {

constexpr float kPulseMaxHz = 5.0f;      // fastest motion drawn
constexpr uint8_t kPulseTravel = 4;      // pixels a ring moves outwards
constexpr uint8_t kPulseRings = 3;
constexpr float kPulseRingLag = 0.9f;    // radians between neighbouring rings
constexpr uint32_t kPulseFrameMs = 33;   // at most about thirty frames a second
constexpr uint32_t kPulseMaxStepMs = 250;  // a stalled interface does not jump ahead

// The rate at which a beat of `beatHz` is shown.
inline float pulseShownHz(float beatHz) {
    if (!(beatHz > 0.0f) || !isfinite(beatHz)) return 0.0f;
    while (beatHz > kPulseMaxHz) beatHz *= 0.5f;
    return beatHz;
}

class Pulse {
public:
    // Back to rest: every ring at its resting radius.
    void reset() {
        _phase = 0.0f;
        _travelled = 0.0f;
        _started = false;
    }

    // Advances the pulse to `nowMs` for a beat of `beatHz`. Returns true
    // when a new frame is due; false when the last one is younger than
    // kPulseFrameMs.
    bool advance(uint32_t nowMs, float beatHz) {
        if (!_started) {
            _started = true;
            _lastMs = nowMs;
            return true;
        }
        uint32_t dt = nowMs - _lastMs;
        if (dt < kPulseFrameMs) return false;
        _lastMs = nowMs;
        if (dt > kPulseMaxStepMs) dt = kPulseMaxStepMs;
        const float step = kTwoPi * pulseShownHz(beatHz) * static_cast<float>(dt) * 0.001f;
        _phase += step;
        if (_travelled < kTwoPi) _travelled += step;  // only the first cycle matters
        while (_phase >= kTwoPi) _phase -= kTwoPi;
        return true;
    }

    // How far ring `ring` (0 = innermost) stands outside its resting radius
    // now, in pixels: 0..kPulseTravel.
    uint8_t offset(uint8_t ring) const {
        const float lag = kPulseRingLag * static_cast<float>(ring);
        // From rest, a ring stays still until the first pulse has reached it.
        if (_travelled < lag) return 0;
        const float a = _phase - lag;
        const float v = 0.5f - 0.5f * cosf(a);  // 0 at rest, 1 at full swell
        const int px = static_cast<int>(v * static_cast<float>(kPulseTravel) + 0.5f);
        return static_cast<uint8_t>(px < 0 ? 0 : (px > kPulseTravel ? kPulseTravel : px));
    }

    float phase() const { return _phase; }

private:
    static constexpr float kTwoPi = 6.28318530718f;
    float _phase = 0.0f;
    float _travelled = 0.0f;  // phase covered since rest, up to one cycle
    uint32_t _lastMs = 0;
    bool _started = false;
};

}  // namespace rc
