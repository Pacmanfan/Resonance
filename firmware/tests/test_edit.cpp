// The edits the touchscreen makes to sessions and sequences (RES-GUI-006).
#include "host_test.h"
#include "rc_edit.h"
#include "rc_settings.h"

#include <math.h>
#include <string.h>

using namespace rc;

TEST(a_new_session_is_one_valid_stage) {
    const Session s = newSession();
    CHECK(isValidSession(s));
    CHECK(s.stageCount == 1 && s.stages[0].durationS == 300 && s.stages[0].sweep == Sweep::Hold);
    CHECK(s.stages[0].primaryHz == 0.0f && s.stages[0].beatHz == 0.0f);  // holds what is set
}

TEST(stages_are_added_removed_and_reordered) {
    Session s = newSession();
    s.stages[0].beatHz = 10.0f;
    CHECK(addStage(s, 0));
    CHECK(s.stageCount == 2 && s.stages[1].beatHz == 10.0f);  // a copy of the one before
    s.stages[1].beatHz = 8.0f;
    CHECK(addStage(s, 0));                                     // in the middle
    s.stages[1].beatHz = 9.0f;
    CHECK(s.stages[0].beatHz == 10.0f && s.stages[1].beatHz == 9.0f && s.stages[2].beatHz == 8.0f);
    CHECK(!addStage(s, 3));                                    // no such stage
    CHECK(moveStage(s, 2, -1));
    CHECK(s.stages[1].beatHz == 8.0f && s.stages[2].beatHz == 9.0f);
    CHECK(moveStage(s, 0, 1));
    CHECK(s.stages[0].beatHz == 8.0f && s.stages[1].beatHz == 10.0f);
    CHECK(!moveStage(s, 0, -1));                               // already first
    CHECK(!moveStage(s, 2, 1));                                // already last
    CHECK(!moveStage(s, 1, 0));
    CHECK(!moveStage(s, 7, 1));
    CHECK(removeStage(s, 1));
    CHECK(s.stageCount == 2 && s.stages[0].beatHz == 8.0f && s.stages[1].beatHz == 9.0f);
    CHECK(!removeStage(s, 2));
    CHECK(removeStage(s, 0));
    CHECK(!removeStage(s, 0));                                 // the last one stays
    CHECK(isValidSession(s));
    while (addStage(s, 0)) {}
    CHECK(s.stageCount == kMaxStages);
    CHECK(isValidSession(s));
    // A session that ran until stopped gets a length when it gets a second stage.
    Session timer = timerSession(0);
    CHECK(timer.stages[0].durationS == 0);
    CHECK(addStage(timer, 0));
    CHECK(isValidSession(timer));
}

TEST(times_step_coarser_as_they_grow_and_stop_at_their_limits) {
    CHECK(timeStep(10) == 10 && timeStep(110) == 10 && timeStep(120) == 30 && timeStep(570) == 30 && timeStep(600) == 60);
    Stage stage = {};
    stage.durationS = 100;
    stepStageDuration(stage, 1);
    CHECK(stage.durationS == 110);
    stepStageDuration(stage, 1);
    CHECK(stage.durationS == 120);
    stepStageDuration(stage, 1);
    CHECK(stage.durationS == 150);
    stepStageDuration(stage, -1);
    CHECK(stage.durationS == 120);
    stepStageDuration(stage, -1);
    CHECK(stage.durationS == 110);   // back the way it came
    for (int i = 0; i < 100; ++i) stepStageDuration(stage, -1);
    CHECK(stage.durationS == kEditMinStageS);
    for (int i = 0; i < 500; ++i) stepStageDuration(stage, 1);
    CHECK(stage.durationS == kEditMaxStageS);
    // An odd value from a file settles onto the grid.
    stage.durationS = 137;
    stepStageDuration(stage, 1);
    CHECK(stage.durationS == 150);
    stage.durationS = 137;
    stepStageDuration(stage, -1);
    CHECK(stage.durationS == 90 || stage.durationS == 120);
    stage.durationS = 300;
    stepStageDuration(stage, 0);
    CHECK(stage.durationS == 300);
}

TEST(the_transition_stays_inside_the_stage) {
    Stage stage = {};
    stage.durationS = 300;
    stage.sweep = Sweep::Hold;
    stepStageTransition(stage, -1);
    CHECK(stage.transitionS == 0);           // nothing on a hold
    cycleStageSweep(stage);
    CHECK(stage.sweep == Sweep::Linear && stage.transitionS == 0);  // the whole stage
    stepStageTransition(stage, 1);
    CHECK(stage.transitionS == 0);           // cannot be longer than the stage
    stepStageTransition(stage, -1);
    CHECK(stage.transitionS == 270);
    for (int i = 0; i < 100; ++i) stepStageTransition(stage, -1);
    CHECK(stage.transitionS == kEditMinStageS);
    for (int i = 0; i < 100; ++i) stepStageTransition(stage, 1);
    CHECK(stage.transitionS == 0);           // the whole stage again
    stepStageTransition(stage, -1);
    stepStageTransition(stage, -1);
    CHECK(stage.transitionS == 240);
    // Shortening the stage shortens the transition with it.
    while (stage.durationS > 120) stepStageDuration(stage, -1);
    CHECK(stage.transitionS <= stage.durationS);
    cycleStageSweep(stage);
    CHECK(stage.sweep == Sweep::Exponential);
    stage.fromBeatHz = 12.0f;
    stage.beatHz = 6.0f;
    cycleStageSweep(stage);
    CHECK(stage.sweep == Sweep::Hold && stage.transitionS == 0 && stage.fromBeatHz == 0.0f);
}

TEST(frequencies_step_within_range_and_zero_means_leave_alone) {
    Stage stage = {};
    stepStageBeat(stage, -0.5f, 10.0f);
    CHECK(stage.beatHz == 0.0f);             // still "leave alone"
    stepStageBeat(stage, 0.5f, 10.0f);
    CHECK(stage.beatHz == 10.0f);            // starts at the current value
    stepStageBeat(stage, 0.5f, 10.0f);
    CHECK_NEAR(stage.beatHz, 10.5, 1e-4);
    for (int i = 0; i < 200; ++i) stepStageBeat(stage, 0.5f, 10.0f);
    CHECK(stage.beatHz == kBeatMaxHz);
    for (int i = 0; i < 200; ++i) {
        stepStageBeat(stage, -0.5f, 10.0f);
        if (stage.beatHz == kBeatMinHz) break;
    }
    CHECK(stage.beatHz == kBeatMinHz);
    stepStageBeat(stage, -0.5f, 10.0f);
    CHECK(stage.beatHz == 0.0f);             // one more step down: leave alone
    stepStagePrimary(stage, 10.0f, 5000.0f); // a current value out of range is brought into it
    CHECK(stage.primaryHz == kPrimaryMaxHz);
    stepStagePrimary(stage, -10.0f, 200.0f);
    CHECK_NEAR(stage.primaryHz, 990.0, 1e-3);
    stepStagePrimary(stage, NAN, 200.0f);
    CHECK_NEAR(stage.primaryHz, 990.0, 1e-3);
    stage.primaryHz = 25.0f;
    stepStagePrimary(stage, -10.0f, 200.0f);
    CHECK(stage.primaryHz == kPrimaryMinHz);
    stage.fromPrimaryHz = 100.0f;
    stepStagePrimary(stage, -10.0f, 200.0f);
    CHECK(stage.primaryHz == 0.0f && stage.fromPrimaryHz == 0.0f);
}

TEST(no_run_of_edits_makes_a_session_that_is_refused) {
    Session s = newSession();
    uint32_t seed = 12345;
    auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return seed >> 8; };
    for (int i = 0; i < 20000; ++i) {
        const uint8_t index = static_cast<uint8_t>(next() % (s.stageCount + 1));  // sometimes one too far
        Stage scratch = {};
        scratch.durationS = 60;
        Stage& stage = index < s.stageCount ? s.stages[index] : scratch;
        const int direction = static_cast<int>(next() % 3) - 1;
        switch (next() % 9) {
            case 0: addStage(s, index); break;
            case 1: removeStage(s, index); break;
            case 2: moveStage(s, index, direction); break;
            case 3: stepStageDuration(stage, direction); break;
            case 4: stepStageTransition(stage, direction); break;
            case 5: cycleStageSweep(stage); break;
            case 6: stepStagePrimary(stage, 10.0f * direction, 200.0f); break;
            case 7: stepStageBeat(stage, 0.5f * direction, 10.0f); break;
            default: stepStagePrimary(stage, 1.0f * direction, 963.0f); break;
        }
        if (!isValidSession(s)) {
            CHECK(isValidSession(s));
            break;
        }
    }
    CHECK(isValidSession(s));
}

TEST(sequence_steps_are_added_removed_and_changed_within_limits) {
    Sequence s = defaultSequence();
    CHECK(addStep(s, 8));
    CHECK(s.stepCount == 10 && s.steps[9].hz == 963.0f);
    CHECK(!addStep(s, 0));                   // full
    CHECK(removeStep(s, 0));
    CHECK(s.steps[0].hz == 285.0f && s.stepCount == 9);
    CHECK(!removeStep(s, 9));
    stepStepHz(s.steps[0], 10.0f);
    CHECK(s.steps[0].hz == 295.0f);
    stepStepHz(s.steps[0], -5000.0f);
    CHECK(s.steps[0].hz == kPrimaryMinHz);
    stepStepHz(s.steps[0], 5000.0f);
    CHECK(s.steps[0].hz == kPrimaryMaxHz);
    stepStepHz(s.steps[0], NAN);
    CHECK(s.steps[0].hz == kPrimaryMaxHz);
    stepStepSeconds(s.steps[0], 1);
    CHECK(s.steps[0].seconds == 150);
    for (int i = 0; i < 200; ++i) stepStepSeconds(s.steps[0], -1);
    CHECK(s.steps[0].seconds == kEditMinStageS);
    stepGlide(s, -1);
    CHECK(s.glideS == 0);
    stepGlide(s, 1);
    CHECK(s.glideS == 5);
    for (int i = 0; i < 500; ++i) stepGlide(s, 1);
    CHECK(s.glideS == kSequenceGlideMaxS);
    while (removeStep(s, 0)) {}
    CHECK(s.stepCount == 1);
    CHECK(isValidSequence(s));
    CHECK(isValidSession(sequenceSession(s)));
}

TEST(a_stage_is_described_in_one_short_line) {
    Stage stage = {};
    stage.durationS = 330;
    stage.sweep = Sweep::Exponential;
    stage.primaryHz = 174.0f;
    stage.beatHz = 6.5f;
    char text[32];
    describeStage(stage, text, sizeof(text));
    CHECK(strcmp(text, "5:30 exp 174 / 6.5 Hz") == 0);
    stage = Stage{};
    stage.durationS = 60;
    describeStage(stage, text, sizeof(text));
    CHECK(strcmp(text, "1:00 hold - / - Hz") == 0);
    char tiny[6];
    describeStage(stage, tiny, sizeof(tiny));   // cut, not overrun
    CHECK(strlen(tiny) == 5);
}

int main() { return host_test::runAll(); }
