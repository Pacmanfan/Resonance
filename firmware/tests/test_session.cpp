// The session runner (RES-SESS-001..005).
#include "host_test.h"
#include "rc_session.h"
#include "rc_settings.h"

#include <initializer_list>
#include <limits>
#include <math.h>
#include <stdlib.h>

using namespace rc;

namespace {
Session sweepSession() {
    Session s = {};
    strcpy(s.name, "test");
    s.stageCount = 3;
    s.stages[0] = {60, 0, 10.0f, Sweep::Hold};
    s.stages[1] = {100, 300.0f, 6.0f, Sweep::Linear};
    s.stages[2] = {40, 0, 6.0f, Sweep::Hold};
    return s;
}
}  // namespace

TEST(a_timer_holds_the_current_frequencies_and_counts_down) {
    SessionRunner r;
    CHECK(r.start(timerSession(10), 200.0f, 7.5f, 0));
    CHECK(r.state() == SessionState::Running);
    CHECK(r.remainingS() == 600);
    r.advance(90500);
    CHECK(r.remainingS() == 510);  // rounded up to the second
    CHECK_NEAR(r.primaryHz(), 200.0, 1e-6);
    CHECK_NEAR(r.beatHz(), 7.5, 1e-6);
    CHECK_NEAR(r.gain(), 1.0, 1e-6);
    r.advance(509500);
    CHECK(r.state() == SessionState::Finished);  // no fade asked: ends at once
    CHECK_NEAR(r.gain(), 0.0, 1e-6);
}

TEST(a_continuous_timer_runs_until_stopped) {
    SessionRunner r;
    CHECK(r.start(timerSession(0), 200.0f, 10.0f, 30));
    CHECK(r.endless());
    r.advance(10u * 3600u * 1000u);  // ten hours
    CHECK(r.state() == SessionState::Running);
    CHECK(r.remainingS() == 0);
    r.stop();
    CHECK(r.state() == SessionState::FadingOut);
}

TEST(a_hold_stage_jumps_and_a_linear_stage_glides) {
    SessionRunner r;
    CHECK(r.start(sweepSession(), 200.0f, 14.0f, 0));
    CHECK_NEAR(r.beatHz(), 10.0, 1e-6);     // hold: at once
    CHECK_NEAR(r.primaryHz(), 200.0, 1e-6); // 0 = left alone
    r.advance(60000);
    CHECK(r.stageIndex() == 1);
    CHECK_NEAR(r.beatHz(), 10.0, 1e-6);     // the glide starts from here
    r.advance(25000);
    CHECK_NEAR(r.beatHz(), 9.0, 1e-4);      // a quarter of the way from 10 to 6
    CHECK_NEAR(r.primaryHz(), 225.0, 1e-3); // and from 200 to 300
    r.advance(25000);
    CHECK_NEAR(r.beatHz(), 8.0, 1e-4);
    r.advance(50000);
    CHECK(r.stageIndex() == 2);
    CHECK_NEAR(r.beatHz(), 6.0, 1e-4);
    CHECK_NEAR(r.primaryHz(), 300.0, 1e-3);
    CHECK(r.remainingS() == 40);
}

TEST(one_long_step_crosses_several_stages_correctly) {
    SessionRunner a, b;
    a.start(sweepSession(), 200.0f, 14.0f, 0);
    b.start(sweepSession(), 200.0f, 14.0f, 0);
    a.advance(110000);
    for (int i = 0; i < 1100; ++i) b.advance(100);
    CHECK(a.stageIndex() == b.stageIndex());
    CHECK_NEAR(a.beatHz(), b.beatHz(), 1e-3);
    CHECK_NEAR(a.primaryHz(), b.primaryHz(), 1e-2);
    CHECK(a.remainingS() == b.remainingS());
}

TEST(pause_freezes_everything_exactly) {
    SessionRunner r;
    r.start(sweepSession(), 200.0f, 14.0f, 0);
    r.advance(85000);
    const float beat = r.beatHz();
    const uint32_t remaining = r.remainingS();
    r.pause();
    CHECK(r.state() == SessionState::Paused);
    r.advance(3600000);
    CHECK(r.beatHz() == beat && r.remainingS() == remaining);
    r.resume();
    r.advance(25000);
    CHECK_NEAR(r.beatHz(), 8.0, 1e-4);  // 50 s into the glide, as if never paused
}

TEST(the_end_fades_out_in_a_straight_line) {
    SessionRunner r;
    r.start(timerSession(1), 200.0f, 10.0f, 60);
    r.advance(60000);
    CHECK(r.state() == SessionState::FadingOut);
    CHECK_NEAR(r.gain(), 1.0, 1e-6);
    r.advance(15000);
    CHECK_NEAR(r.gain(), 0.75, 1e-4);
    r.advance(30000);
    CHECK_NEAR(r.gain(), 0.25, 1e-4);
    CHECK_NEAR(r.beatHz(), 10.0, 1e-6);  // the frequencies stay during the fade
    r.advance(15000);
    CHECK(r.state() == SessionState::Finished);
    CHECK_NEAR(r.gain(), 0.0, 1e-6);
    r.reset();
    CHECK(r.state() == SessionState::Idle);
    CHECK_NEAR(r.gain(), 1.0, 1e-6);
}

TEST(stop_goes_through_the_same_fade_and_works_from_pause) {
    SessionRunner r;
    r.start(sweepSession(), 200.0f, 14.0f, 30);
    r.advance(20000);
    r.pause();
    r.stop();
    CHECK(r.state() == SessionState::FadingOut);
    r.advance(15000);
    CHECK_NEAR(r.gain(), 0.5, 1e-4);
    r.advance(15000);
    CHECK(r.state() == SessionState::Finished);
}

TEST(an_idle_runner_ignores_everything) {
    SessionRunner r;
    r.advance(1000);
    r.pause();
    r.resume();
    r.stop();
    CHECK(r.state() == SessionState::Idle);
    CHECK_NEAR(r.gain(), 1.0, 1e-6);
    CHECK(r.remainingS() == 0);
}

TEST(invalid_sessions_are_refused) {
    SessionRunner r;
    Session s = sweepSession();
    CHECK(isValidSession(s));
    s.stageCount = 0;
    CHECK(!isValidSession(s) && !r.start(s, 200.0f, 10.0f, 0));
    CHECK(r.state() == SessionState::Idle);
    s = sweepSession(); s.stageCount = kMaxStages + 1; CHECK(!isValidSession(s));
    s = sweepSession(); s.stages[1].beatHz = 100.0f; CHECK(!isValidSession(s));
    s = sweepSession(); s.stages[1].primaryHz = 5.0f; CHECK(!isValidSession(s));
    s = sweepSession(); s.stages[1].beatHz = std::numeric_limits<float>::quiet_NaN(); CHECK(!isValidSession(s));
    s = sweepSession(); s.stages[1].durationS = 0; CHECK(!isValidSession(s));  // endless only alone
    s = sweepSession(); s.stages[0].sweep = static_cast<Sweep>(9); CHECK(!isValidSession(s));
}

TEST(factory_sessions_are_valid) {
    CHECK(factorySessionCount() >= 3);
    for (uint8_t i = 0; i < factorySessionCount(); ++i) {
        const Session& s = factorySession(i);
        CHECK(isValidSession(s));
        CHECK(strlen(s.name) < sizeof(s.name));
        CHECK(sessionSeconds(s) % 60 == 0);  // whole minutes, so the screen can show them
    }
    CHECK(sessionSeconds(timerSession(45)) == 2700);
    CHECK(sessionSeconds(timerSession(0)) == 0);
}

TEST(the_practice_programs_descend_and_hold_and_promise_nothing) {
    // Find them by name, so that the test does not depend on their order.
    const char* const wanted[] = {"Trance", "Deep trance", "Astral"};
    const uint32_t minutes[] = {30, 40, 45};
    for (int w = 0; w < 3; ++w) {
        const Session* found = nullptr;
        for (uint8_t i = 0; i < factorySessionCount(); ++i) {
            if (strncmp(factorySession(i).name, wanted[w], strlen(wanted[w])) == 0 &&
                factorySession(i).name[strlen(wanted[w])] == ' ') {
                found = &factorySession(i);
            }
        }
        CHECK(found != nullptr);
        if (found == nullptr) continue;
        CHECK(sessionSeconds(*found) == minutes[w] * 60u);
        // The beat only ever goes down, ends in the theta/delta border, and
        // the last stage is a hold of at least a quarter of the program.
        float previous = 100.0f;
        for (uint8_t k = 0; k < found->stageCount; ++k) {
            CHECK(found->stages[k].beatHz <= previous);
            CHECK(found->stages[k].primaryHz == 0.0f);  // the carrier is the user's
            previous = found->stages[k].beatHz;
        }
        const Stage& last = found->stages[found->stageCount - 1];
        CHECK(last.sweep == Sweep::Hold && last.beatHz >= 3.0f && last.beatHz <= 5.0f);
        CHECK(last.durationS * 4u >= sessionSeconds(*found));
    }
    // RES-PROC-006: no program name promises an effect.
    const char* const forbidden[] = {"heal", "cure", "guarantee", "therapy", "treat", "pain", "anxiety", "miracle"};
    for (uint8_t i = 0; i < factorySessionCount(); ++i) {
        char lower[24];
        size_t n = 0;
        for (const char* c = factorySession(i).name; *c != '\0' && n < sizeof(lower) - 1; ++c, ++n) {
            lower[n] = static_cast<char>(*c >= 'A' && *c <= 'Z' ? *c + 32 : *c);
        }
        lower[n] = '\0';
        for (const char* word : forbidden) CHECK(strstr(lower, word) == nullptr);
    }
}

TEST(a_program_can_be_scaled_to_any_length) {
    for (uint8_t i = 0; i < factorySessionCount(); ++i) {
        const Session& original = factorySession(i);
        for (uint16_t minutes : {1, 10, 15, 20, 30, 45, 60, 240}) {
            const Session scaled = scaledSession(original, minutes);
            CHECK(isValidSession(scaled));
            CHECK(sessionSeconds(scaled) == minutes * 60u);  // exact, whatever the rounding
            CHECK(scaled.stageCount == original.stageCount);
            for (uint8_t k = 0; k < scaled.stageCount; ++k) {
                CHECK(scaled.stages[k].durationS >= 1);
                CHECK(scaled.stages[k].beatHz == original.stages[k].beatHz);
                CHECK(scaled.stages[k].sweep == original.stages[k].sweep);
                // Each stage keeps its share of the whole, within a second or two.
                const double share = static_cast<double>(original.stages[k].durationS) / sessionSeconds(original);
                CHECK_NEAR(scaled.stages[k].durationS, share * minutes * 60.0, 2.5);
            }
        }
    }
}

TEST(scaling_leaves_endless_and_zero_alone_and_shapes_the_sweep) {
    const Session endless = timerSession(0);
    CHECK(sessionSeconds(scaledSession(endless, 30)) == 0);
    const Session same = scaledSession(sweepSession(), 0);
    CHECK(sessionSeconds(same) == sessionSeconds(sweepSession()));

    // The 200-second test session at three times its length: the glide is at
    // its midpoint after 180 + 150 seconds instead of 60 + 50.
    SessionRunner r;
    const Session twice = scaledSession(sweepSession(), 10);  // 200 s -> 600 s: three times
    CHECK(r.start(twice, 200.0f, 14.0f, 0));
    r.advance(180000 + 150000);        // first stage (3 x 60 s) and half of the glide (3 x 100 s)
    CHECK(r.stageIndex() == 1);
    CHECK_NEAR(r.beatHz(), 8.0, 1e-3);
}

// ---- RES-SESS-006: sweep shapes and custom sweeps ------------------------------

TEST(sweep_shapes_at_their_ends_and_in_the_middle) {
    CHECK_NEAR(sweepValue(Sweep::Linear, 8.0f, 2.0f, 0.0f), 8.0, 1e-6);
    CHECK_NEAR(sweepValue(Sweep::Linear, 8.0f, 2.0f, 0.5f), 5.0, 1e-6);
    CHECK_NEAR(sweepValue(Sweep::Linear, 8.0f, 2.0f, 1.0f), 2.0, 1e-6);
    CHECK_NEAR(sweepValue(Sweep::Exponential, 8.0f, 2.0f, 0.0f), 8.0, 1e-6);
    CHECK_NEAR(sweepValue(Sweep::Exponential, 8.0f, 2.0f, 0.5f), 4.0, 1e-5);   // the geometric middle
    CHECK_NEAR(sweepValue(Sweep::Exponential, 8.0f, 2.0f, 1.0f), 2.0, 1e-6);
    CHECK_NEAR(sweepValue(Sweep::Exponential, 100.0f, 400.0f, 0.25f), 141.4214, 1e-3);
    CHECK_NEAR(sweepValue(Sweep::Hold, 8.0f, 2.0f, 0.3f), 2.0, 1e-6);
    // Outside 0..1, and a fraction that is not a number, stay on the ends.
    CHECK_NEAR(sweepValue(Sweep::Exponential, 8.0f, 2.0f, -1.0f), 8.0, 1e-6);
    CHECK_NEAR(sweepValue(Sweep::Exponential, 8.0f, 2.0f, 7.0f), 2.0, 1e-6);
    CHECK_NEAR(sweepValue(Sweep::Linear, 8.0f, 2.0f, std::numeric_limits<float>::quiet_NaN()), 8.0, 1e-6);
    // An exponential sweep from or to zero cannot exist: a straight line instead.
    CHECK_NEAR(sweepValue(Sweep::Exponential, 0.0f, 2.0f, 0.5f), 1.0, 1e-6);
}

TEST(an_exponential_stage_moves_in_equal_ratios) {
    Session s = {};
    strcpy(s.name, "exp");
    s.stageCount = 2;
    s.stages[0] = {10, 0, 8.0f, Sweep::Hold};
    s.stages[1] = {100, 400.0f, 2.0f, Sweep::Exponential};
    CHECK(isValidSession(s));
    SessionRunner r;
    CHECK(r.start(s, 100.0f, 10.0f, 0));
    r.advance(10000);
    CHECK_NEAR(r.beatHz(), 8.0, 1e-5);
    CHECK_NEAR(r.primaryHz(), 100.0, 1e-4);
    float previous = r.beatHz();
    for (int i = 0; i < 4; ++i) {
        r.advance(25000);
        CHECK_NEAR(r.beatHz() / previous, pow(2.0 / 8.0, 0.25), 1e-4);  // the same ratio every quarter
        previous = r.beatHz();
    }
    CHECK_NEAR(r.beatHz(), 2.0, 1e-5);
    CHECK_NEAR(r.primaryHz(), 400.0, 1e-3);
}

TEST(a_stage_can_name_where_its_sweep_starts) {
    Session s = {};
    strcpy(s.name, "custom");
    s.stageCount = 2;
    s.stages[0] = {20, 0, 12.0f, Sweep::Hold};
    s.stages[1] = {60, 300.0f, 4.0f, Sweep::Linear, 150.0f, 10.0f};
    CHECK(isValidSession(s));
    SessionRunner r;
    CHECK(r.start(s, 200.0f, 7.0f, 0));
    r.advance(19999);
    CHECK_NEAR(r.beatHz(), 12.0, 1e-5);
    CHECK_NEAR(r.primaryHz(), 200.0, 1e-4);
    r.advance(1);                           // the stage begins at its own start
    CHECK_NEAR(r.primaryHz(), 150.0, 1e-4);
    CHECK_NEAR(r.beatHz(), 10.0, 1e-5);
    r.advance(30000);
    CHECK_NEAR(r.primaryHz(), 225.0, 1e-3);
    CHECK_NEAR(r.beatHz(), 7.0, 1e-4);
    r.pause();
    r.advance(100000);
    CHECK_NEAR(r.primaryHz(), 225.0, 1e-3);
    r.resume();
    r.advance(29999);
    CHECK(r.state() == SessionState::Running);
    r.advance(1);
    CHECK(r.state() == SessionState::Finished);
    CHECK_NEAR(r.primaryHz(), 300.0, 1e-3);
    CHECK_NEAR(r.beatHz(), 4.0, 1e-5);
}

TEST(a_custom_sweep_is_a_session_of_one_stage) {
    const Session s = sweepSession(Sweep::Exponential, 100.0f, 800.0f, 0.0f, 0.0f, 90);
    CHECK(isValidSession(s));
    CHECK(sessionSeconds(s) == 90);
    SessionRunner r;
    CHECK(r.start(s, 432.0f, 6.0f, 0));
    CHECK_NEAR(r.primaryHz(), 100.0, 1e-4);   // starts where it was told to
    r.advance(30000);
    CHECK_NEAR(r.primaryHz(), 200.0, 1e-2);   // an octave every 30 s
    CHECK_NEAR(r.beatHz(), 6.0, 1e-6);        // the beat was left alone
    r.advance(30000);
    CHECK_NEAR(r.primaryHz(), 400.0, 1e-2);
    r.stop();                                 // interrupted: no fade asked, ends at once
    CHECK(r.state() == SessionState::Finished);
    CHECK(isValidSession(sweepSession(Sweep::Linear, 0.0f, 0.0f, 10.0f, 4.0f, 0)));  // 0 s becomes 1 s
    CHECK(sessionSeconds(sweepSession(Sweep::Linear, 0.0f, 0.0f, 10.0f, 4.0f, 0)) == 1);
}

TEST(bad_sweeps_are_refused) {
    Session s = sweepSession(Sweep::Linear, 100.0f, 200.0f, 10.0f, 4.0f, 60);
    CHECK(isValidSession(s));
    Session bad = s;
    bad.stages[0].sweep = static_cast<Sweep>(3);
    CHECK(!isValidSession(bad));
    bad = s;
    bad.stages[0].fromPrimaryHz = 5.0f;        // below the range
    CHECK(!isValidSession(bad));
    bad = s;
    bad.stages[0].fromBeatHz = 41.0f;          // above the range
    CHECK(!isValidSession(bad));
    bad = s;
    bad.stages[0].fromBeatHz = std::numeric_limits<float>::quiet_NaN();
    CHECK(!isValidSession(bad));
    bad = s;
    bad.stages[0].primaryHz = 0.0f;            // a start without an end
    CHECK(!isValidSession(bad));
    SessionRunner r;
    CHECK(!r.start(bad, 200.0f, 10.0f, 0));
    CHECK(r.state() == SessionState::Idle);
}

int main() { return host_test::runAll(); }
