#include "rc_breath.h"

#include <math.h>

namespace rc {

bool isValidBreath(const BreathPattern& p) {
    return p.inhaleDs >= kBreathMinDs && p.inhaleDs <= kBreathMaxDs && p.exhaleDs >= kBreathMinDs &&
           p.exhaleDs <= kBreathMaxDs && p.holdInDs <= kBreathMaxDs && p.holdOutDs <= kBreathMaxDs;
}

uint32_t breathCycleMs(const BreathPattern& p) {
    return (static_cast<uint32_t>(p.inhaleDs) + p.holdInDs + p.exhaleDs + p.holdOutDs) * 100u;
}

bool BreathPacer::start(const BreathPattern& pattern) {
    _ms = 0;
    _breaths = 0;
    _running = isValidBreath(pattern);
    if (_running) _pattern = pattern;
    return _running;
}

void BreathPacer::stop() {
    _running = false;
    _ms = 0;
}

void BreathPacer::advance(uint32_t ms) {
    if (!_running) return;
    const uint32_t cycle = breathCycleMs(_pattern);
    // Whole breaths first, so that no sum can overflow.
    _breaths += ms / cycle;
    _ms += ms % cycle;
    if (_ms >= cycle) {
        _ms -= cycle;
        ++_breaths;
    }
}

BreathPhase BreathPacer::locate(uint32_t& into, uint32_t& length) const {
    const uint32_t parts[4] = {_pattern.inhaleDs * 100u, _pattern.holdInDs * 100u, _pattern.exhaleDs * 100u,
                               _pattern.holdOutDs * 100u};
    static const BreathPhase kPhases[4] = {BreathPhase::Inhale, BreathPhase::HoldIn, BreathPhase::Exhale,
                                           BreathPhase::HoldOut};
    uint32_t at = _ms;
    for (uint8_t i = 0; i < 4; ++i) {
        if (at < parts[i]) {
            into = at;
            length = parts[i];
            return kPhases[i];
        }
        at -= parts[i];
    }
    into = 0;
    length = parts[0];
    return BreathPhase::Inhale;  // not reached: advance() keeps _ms inside the cycle
}

BreathPhase BreathPacer::phase() const {
    if (!_running) return BreathPhase::Off;
    uint32_t into = 0, length = 0;
    return locate(into, length);
}

float BreathPacer::level() const {
    if (!_running) return 0.0f;
    uint32_t into = 0, length = 0;
    const BreathPhase phase = locate(into, length);
    if (phase == BreathPhase::HoldIn) return 1.0f;
    if (phase == BreathPhase::HoldOut) return 0.0f;
    const float fraction = static_cast<float>(into) / static_cast<float>(length);
    const float eased = 0.5f - 0.5f * cosf(3.14159265f * fraction);
    return phase == BreathPhase::Inhale ? eased : 1.0f - eased;
}

uint16_t BreathPacer::phaseLeftDs() const {
    if (!_running) return 0;
    uint32_t into = 0, length = 0;
    locate(into, length);
    return static_cast<uint16_t>((length - into + 99) / 100);
}

float breathGain(float level, uint8_t depthPercent) {
    if (depthPercent == 0) return 1.0f;
    if (depthPercent > kBreathDepthMax) depthPercent = kBreathDepthMax;
    if (!(level >= 0.0f)) level = 0.0f;  // also catches NaN
    if (level > 1.0f) level = 1.0f;
    return 1.0f - static_cast<float>(depthPercent) * 0.01f * (1.0f - level);
}

}  // namespace rc
