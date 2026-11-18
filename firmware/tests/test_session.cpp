// The session runner (RES-SESS-001..005).
#include "host_test.h"
#include "rc_session.h"
#include "rc_settings.h"

#include <initializer_list>
#include <limits>
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

int main() { return host_test::runAll(); }
