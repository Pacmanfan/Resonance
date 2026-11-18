// Resonance portable core: tap-and-hold repeat for adjustment buttons.
//
// The timing follows the long-standing convention of keyboards and of
// stepper controls: a press acts once at once; if it is held, nothing more
// happens for a pause long enough that an ordinary tap never repeats
// (operating systems default to 400-500 ms; touch interfaces count a press
// as "long" from about 500 ms); after that the action repeats at a steady
// rate, and a hold that goes on speeds up, so that a far value can be
// reached without the first steps running away.
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stdint.h>

namespace rc {

constexpr uint32_t kRepeatDelayMs = 450;     // pause before the first repeat
constexpr uint32_t kRepeatIntervalMs = 140;  // then about seven a second
constexpr uint32_t kRepeatFastAfterMs = 2500;  // after this long held ...
constexpr uint32_t kRepeatFastIntervalMs = 60; // ... about sixteen a second

class HoldRepeat {
public:
    // The button went down at `nowMs`. The caller performs the first action
    // itself; this object only schedules the repeats.
    void press(uint32_t nowMs) {
        _held = true;
        _pressedMs = nowMs;
        _nextMs = nowMs + kRepeatDelayMs;
    }

    void release() { _held = false; }
    bool held() const { return _held; }

    // Call on every pass while the button is held. Returns true when the
    // action should be performed again. At most one repeat per call, so a
    // stalled interface catches up at its own pace and does not burst.
    bool due(uint32_t nowMs) {
        if (!_held || static_cast<int32_t>(nowMs - _nextMs) < 0) return false;
        const uint32_t heldFor = nowMs - _pressedMs;
        _nextMs = nowMs + (heldFor >= kRepeatFastAfterMs ? kRepeatFastIntervalMs : kRepeatIntervalMs);
        return true;
    }

private:
    bool _held = false;
    uint32_t _pressedMs = 0;
    uint32_t _nextMs = 0;
};

}  // namespace rc
