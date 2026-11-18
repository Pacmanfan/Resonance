// The pulse of the home-screen rings.
#include "host_test.h"
#include "rc_pulse.h"

#include <math.h>

using namespace rc;

namespace {
// Runs the pulse for `ms` in passes of 15 ms and counts the times the
// innermost ring comes back inside half its travel after being outside it.
int cyclesIn(float beatHz, uint32_t ms) {
    Pulse p;
    int cycles = 0;
    bool swollen = false;
    for (uint32_t t = 0; t <= ms; t += 15) {
        if (!p.advance(t, beatHz)) continue;
        const uint8_t o = p.offset(0);
        if (o > kPulseTravel / 2) swollen = true;
        if (o < kPulseTravel / 2 && swollen) {
            swollen = false;
            ++cycles;
        }
    }
    return cycles;
}
}  // namespace

TEST(a_slow_beat_is_shown_at_its_own_rate) {
    CHECK(pulseShownHz(1.0f) == 1.0f);
    CHECK(pulseShownHz(4.5f) == 4.5f);
    CHECK(pulseShownHz(5.0f) == 5.0f);
}

TEST(a_fast_beat_is_halved_until_it_can_be_drawn) {
    CHECK(fabsf(pulseShownHz(6.0f) - 3.0f) < 1e-4f);
    CHECK(fabsf(pulseShownHz(7.8f) - 3.9f) < 1e-4f);
    CHECK(fabsf(pulseShownHz(10.0f) - 5.0f) < 1e-4f);
    CHECK(fabsf(pulseShownHz(14.0f) - 3.5f) < 1e-4f);
    CHECK(fabsf(pulseShownHz(40.0f) - 5.0f) < 1e-4f);
    for (float hz = 0.1f; hz <= 100.0f; hz += 0.1f) {
        const float shown = pulseShownHz(hz);
        CHECK(shown > 0.0f && shown <= kPulseMaxHz);
        // At least six frames for every cycle shown.
        CHECK(shown * static_cast<float>(kPulseFrameMs) * 0.001f <= 1.0f / 6.0f);
    }
}

TEST(no_beat_and_bad_values_give_no_motion) {
    CHECK(pulseShownHz(0.0f) == 0.0f);
    CHECK(pulseShownHz(-3.0f) == 0.0f);
    CHECK(pulseShownHz(NAN) == 0.0f);
    CHECK(pulseShownHz(INFINITY) == 0.0f);
    Pulse p;
    for (uint32_t t = 0; t < 5000; t += 15) {
        p.advance(t, 0.0f);
        for (uint8_t r = 0; r < kPulseRings; ++r) CHECK(p.offset(r) == 0);
    }
}

TEST(the_rings_pulse_as_often_as_the_beat) {
    CHECK(cyclesIn(1.0f, 10000) >= 9 && cyclesIn(1.0f, 10000) <= 10);
    CHECK(cyclesIn(2.5f, 10000) >= 24 && cyclesIn(2.5f, 10000) <= 25);
    CHECK(cyclesIn(5.0f, 10000) >= 49 && cyclesIn(5.0f, 10000) <= 50);
    CHECK(cyclesIn(6.0f, 10000) >= 29 && cyclesIn(6.0f, 10000) <= 30);    // shown at 3 Hz
    CHECK(cyclesIn(10.0f, 10000) >= 49 && cyclesIn(10.0f, 10000) <= 50);  // shown at 5 Hz
}

TEST(frames_are_limited_and_offsets_stay_in_range) {
    Pulse p;
    int frames = 0;
    for (uint32_t t = 0; t < 1000; ++t) {
        if (!p.advance(t, 4.5f)) continue;
        ++frames;
        for (uint8_t r = 0; r < kPulseRings; ++r) CHECK(p.offset(r) <= kPulseTravel);
    }
    CHECK(frames >= 29 && frames <= 31);
}

TEST(the_pulse_travels_outwards) {
    // Each ring reaches full swell after the one inside it.
    Pulse p;
    uint32_t peakAt[kPulseRings] = {0, 0, 0};
    for (uint32_t t = 0; t <= 1000; t += 5) {
        p.advance(t, 1.0f);
        const float a = p.phase();
        for (uint8_t r = 0; r < kPulseRings; ++r) {
            const float want = 3.14159265f + kPulseRingLag * r;  // phase of this ring's peak
            if (peakAt[r] == 0 && a >= want) peakAt[r] = t;
        }
    }
    CHECK(peakAt[0] > 0 && peakAt[0] < peakAt[1] && peakAt[1] < peakAt[2]);
    // And at the innermost ring's peak the outer rings are still behind.
    Pulse q;
    q.advance(0, 1.0f);
    q.advance(250, 1.0f);
    q.advance(500, 1.0f);  // half a cycle
    CHECK(q.offset(0) == kPulseTravel);
    CHECK(q.offset(2) < kPulseTravel);
}

TEST(a_stall_does_not_jump_and_reset_returns_to_rest) {
    Pulse p;
    p.advance(0, 1.0f);
    p.advance(60000, 1.0f);  // a minute without a call counts as a quarter second
    CHECK(fabsf(p.phase() - 6.2831853f * 0.25f) < 1e-3f);
    p.reset();
    for (uint8_t r = 0; r < kPulseRings; ++r) CHECK(p.offset(r) == 0);
}

int main() { return host_test::runAll(); }
