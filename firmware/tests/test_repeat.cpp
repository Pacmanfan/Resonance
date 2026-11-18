// Tap-and-hold repeat timing.
#include "host_test.h"
#include "rc_repeat.h"

using namespace rc;

namespace {
// Polls every `stepMs` from `fromMs` to `toMs` and counts the repeats.
int repeatsBetween(HoldRepeat& r, uint32_t fromMs, uint32_t toMs, uint32_t stepMs = 15) {
    int n = 0;
    for (uint32_t t = fromMs; t <= toMs; t += stepMs) n += r.due(t) ? 1 : 0;
    return n;
}
}  // namespace

TEST(an_ordinary_tap_never_repeats) {
    HoldRepeat r;
    r.press(1000);
    CHECK(repeatsBetween(r, 1000, 1000 + 300) == 0);  // a tap lasts well under 300 ms
    r.release();
    CHECK(repeatsBetween(r, 1300, 5000) == 0);        // and nothing follows after release
}

TEST(a_hold_repeats_after_the_pause_at_a_steady_rate) {
    HoldRepeat r;
    r.press(0);
    CHECK(repeatsBetween(r, 0, kRepeatDelayMs - 15) == 0);
    CHECK(r.due(kRepeatDelayMs));
    // One second of steady repeating: about seven.
    const int n = repeatsBetween(r, kRepeatDelayMs + 15, kRepeatDelayMs + 1000);
    CHECK(n >= 6 && n <= 8);
}

TEST(a_long_hold_speeds_up) {
    HoldRepeat r;
    r.press(0);
    repeatsBetween(r, 0, kRepeatFastAfterMs);
    const int fast = repeatsBetween(r, kRepeatFastAfterMs + 15, kRepeatFastAfterMs + 1000);
    CHECK(fast >= 13 && fast <= 17);
}

TEST(a_stalled_interface_does_not_burst) {
    HoldRepeat r;
    r.press(0);
    CHECK(r.due(5000));    // one repeat for the first call after a long stall ...
    CHECK(!r.due(5001));   // ... not a queue of missed ones
}

TEST(each_press_starts_its_own_pause_and_time_wrap_is_harmless) {
    HoldRepeat r;
    r.press(0);
    repeatsBetween(r, 0, 4000);
    r.release();
    r.press(10000);
    CHECK(repeatsBetween(r, 10000, 10000 + kRepeatDelayMs - 15) == 0);  // slow again, not fast
    // A press just before the millisecond counter wraps.
    HoldRepeat w;
    const uint32_t start = 0xFFFFFF00u;
    w.press(start);
    CHECK(!w.due(start + 100));
    CHECK(w.due(start + kRepeatDelayMs));  // past the wrap
}

int main() { return host_test::runAll(); }
