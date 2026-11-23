// The breathing pacer (RES-SESS-011).
#include "host_test.h"
#include "rc_breath.h"

#include <math.h>

using namespace rc;

TEST(a_pattern_is_checked) {
    CHECK(isValidBreath({40, 0, 60, 0}));
    CHECK(isValidBreath({40, 70, 80, 0}));
    CHECK(isValidBreath({10, 250, 250, 250}));
    CHECK(!isValidBreath({9, 0, 60, 0}));     // inhale under a second
    CHECK(!isValidBreath({40, 0, 0, 0}));     // no exhale
    CHECK(!isValidBreath({0, 0, 0, 0}));
    CHECK(!isValidBreath({251, 0, 60, 0}));
    CHECK(!isValidBreath({40, 251, 60, 0}));
    CHECK(breathCycleMs({40, 70, 80, 0}) == 19000);
    BreathPacer p;
    CHECK(!p.start({0, 0, 60, 0}));
    CHECK(!p.running());
    CHECK(p.phase() == BreathPhase::Off);
    CHECK(p.level() == 0.0f);
    CHECK(p.phaseLeftDs() == 0);
}

TEST(it_goes_through_inhale_hold_exhale_hold_in_time) {
    BreathPacer p;
    CHECK(p.start({40, 20, 60, 10}));  // 4 s in, 2 s hold, 6 s out, 1 s hold
    CHECK(p.phase() == BreathPhase::Inhale);
    CHECK(p.level() == 0.0f);
    CHECK(p.phaseLeftDs() == 40);
    p.advance(2000);
    CHECK(p.phase() == BreathPhase::Inhale);
    CHECK_NEAR(p.level(), 0.5, 1e-4);
    CHECK(p.phaseLeftDs() == 20);
    p.advance(1999);
    CHECK(p.phase() == BreathPhase::Inhale);
    CHECK(p.phaseLeftDs() == 1);
    p.advance(1);
    CHECK(p.phase() == BreathPhase::HoldIn);
    CHECK(p.level() == 1.0f);
    CHECK(p.phaseLeftDs() == 20);
    p.advance(2000);
    CHECK(p.phase() == BreathPhase::Exhale);
    CHECK(p.level() == 1.0f);
    p.advance(3000);
    CHECK_NEAR(p.level(), 0.5, 1e-4);
    p.advance(3000);
    CHECK(p.phase() == BreathPhase::HoldOut);
    CHECK(p.level() == 0.0f);
    CHECK(p.breaths() == 0);
    p.advance(1000);
    CHECK(p.phase() == BreathPhase::Inhale);
    CHECK(p.breaths() == 1);
    CHECK(p.level() == 0.0f);
}

TEST(without_holds_it_is_inhale_and_exhale_only) {
    BreathPacer p;
    CHECK(p.start({40, 0, 60, 0}));
    int holds = 0;
    for (int i = 0; i < 3000; ++i) {
        p.advance(10);
        if (p.phase() == BreathPhase::HoldIn || p.phase() == BreathPhase::HoldOut) ++holds;
    }
    CHECK(holds == 0);
    CHECK(p.breaths() == 3);
}

TEST(the_level_moves_smoothly_and_stays_in_range) {
    BreathPacer p;
    CHECK(p.start({15, 5, 25, 5}));
    float last = p.level(), biggest = 0.0f;
    for (int i = 0; i < 20000; ++i) {
        p.advance(15);  // one pass of the main loop
        const float now = p.level();
        CHECK(now >= 0.0f && now <= 1.0f);
        biggest = fabsf(now - last) > biggest ? fabsf(now - last) : biggest;
        last = now;
    }
    // The steepest part of a 1.5 s inhale, in 15 ms: pi/2 * 15/1500.
    CHECK(biggest < 0.017f);
}

TEST(a_long_gap_and_an_endless_run_neither_jump_nor_hang) {
    BreathPacer p;
    CHECK(p.start({40, 0, 60, 0}));
    p.advance(2000);
    p.advance(0xFFFFFFFFu);  // seven weeks without a call
    CHECK(p.running());
    CHECK(p.level() >= 0.0f && p.level() <= 1.0f);
    CHECK(p.breaths() == 0xFFFFFFFFu / 10000u + ((2000u + 0xFFFFFFFFu % 10000u) >= 10000u ? 1u : 0u));
    // The same place as if the time had passed in small pieces.
    BreathPacer q;
    q.start({40, 0, 60, 0});
    q.advance(2000);
    q.advance(0xFFFFFFFFu % 10000u);
    CHECK_NEAR(p.level(), q.level(), 1e-6);
    CHECK(p.phase() == q.phase());
    // A new start begins a new inhale; stop is off.
    CHECK(p.start({20, 0, 20, 0}));
    CHECK(p.breaths() == 0 && p.phase() == BreathPhase::Inhale && p.level() == 0.0f);
    p.stop();
    CHECK(!p.running() && p.phase() == BreathPhase::Off && p.level() == 0.0f);
    p.advance(5000);
    CHECK(p.breaths() == 0);
}

TEST(the_modulation_follows_the_level_by_the_depth) {
    CHECK(breathGain(0.0f, 0) == 1.0f);   // off is exactly one
    CHECK(breathGain(0.37f, 0) == 1.0f);
    CHECK(breathGain(1.0f, 50) == 1.0f);  // breathed in: full level
    CHECK_NEAR(breathGain(0.0f, 50), 0.5, 1e-6);
    CHECK_NEAR(breathGain(0.5f, 50), 0.75, 1e-6);
    CHECK_NEAR(breathGain(0.0f, 80), 0.2, 1e-6);
    CHECK_NEAR(breathGain(0.0f, 100), 0.2, 1e-6);  // capped: never silent
    CHECK_NEAR(breathGain(NAN, 50), 0.5, 1e-6);
    CHECK(breathGain(7.0f, 50) == 1.0f);
    CHECK_NEAR(breathGain(-3.0f, 50), 0.5, 1e-6);
}

int main() { return host_test::runAll(); }
