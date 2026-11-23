// Resonance portable core: the breathing pacer (RES-SESS-011).
//
// Turns elapsed time into the part of the breath, the time left in it and a
// level between 0 (breathed out) and 1 (breathed in). It knows nothing of
// screens or audio; whoever owns it draws the level and, if wanted, hands
// breathGain() to the audio engine.
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stdint.h>

namespace rc {

// Times in tenths of a second.
constexpr uint8_t kBreathMinDs = 10;   // shortest inhale or exhale: 1 s
constexpr uint8_t kBreathMaxDs = 250;  // longest part: 25 s
constexpr uint8_t kBreathDepthMax = 80;  // percent; never down to silence

struct BreathPattern {
    uint8_t inhaleDs;
    uint8_t holdInDs;   // hold after the inhale; 0 = none
    uint8_t exhaleDs;
    uint8_t holdOutDs;  // hold after the exhale; 0 = none
};

bool isValidBreath(const BreathPattern& pattern);
// Length of one whole breath in milliseconds.
uint32_t breathCycleMs(const BreathPattern& pattern);

enum class BreathPhase : uint8_t { Off, Inhale, HoldIn, Exhale, HoldOut };

class BreathPacer {
public:
    // Starts at the beginning of an inhale. Returns false, and stops, when
    // the pattern is not valid.
    bool start(const BreathPattern& pattern);
    void stop();
    // Advances by `ms` of real time. A long gap is taken as it is: the pacer
    // lands where the breath would be by then.
    void advance(uint32_t ms);

    bool running() const { return _running; }
    const BreathPattern& pattern() const { return _pattern; }
    BreathPhase phase() const;
    // 0 breathed out .. 1 breathed in. Eased at both ends of the inhale and
    // of the exhale, so that it never changes direction abruptly. 0 when off.
    float level() const;
    // Tenths of a second left in the current part, rounded up.
    uint16_t phaseLeftDs() const;
    // Whole breaths completed since start().
    uint32_t breaths() const { return _breaths; }

private:
    // Where in the cycle `_ms` falls: the part, and how far into it.
    BreathPhase locate(uint32_t& into, uint32_t& length) const;

    BreathPattern _pattern = {0, 0, 0, 0};
    bool _running = false;
    uint32_t _ms = 0;  // time into the current breath
    uint32_t _breaths = 0;
};

// The gain the optional audio modulation gives at `level`: 1 breathed in,
// 1 - depth breathed out. A depth of 0 is exactly 1.
float breathGain(float level, uint8_t depthPercent);

}  // namespace rc
