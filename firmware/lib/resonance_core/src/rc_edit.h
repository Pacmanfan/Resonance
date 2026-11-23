// Resonance portable core: the edits the touchscreen makes to a session and
// to a frequency sequence (RES-GUI-006). Every function leaves a valid
// session or sequence valid, whatever is asked: a step past a limit stops at
// the limit, and an edit that cannot be made returns false and changes
// nothing. The screens only call these and draw the result.
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stdint.h>

#include "rc_sequence.h"
#include "rc_session.h"

namespace rc {

constexpr uint16_t kEditMinStageS = 10;
constexpr uint16_t kEditMaxStageS = 3600;

// A session to start from: one stage that holds the current frequencies
// for five minutes.
Session newSession();

// Adds a copy of stage `index` after it. False when the session is full.
bool addStage(Session& session, uint8_t index);
// Removes stage `index`. False for the only stage.
bool removeStage(Session& session, uint8_t index);
// Swaps stage `index` with its neighbour before (-1) or after (+1) it.
bool moveStage(Session& session, uint8_t index, int direction);

// The step a time is changed by at `seconds`: 10 s below two minutes, 30 s
// below ten, a minute above.
uint16_t timeStep(uint16_t seconds);

// The stage's length, one step up or down, within kEditMinStageS and
// kEditMaxStageS. The transition is shortened to fit.
void stepStageDuration(Stage& stage, int direction);
// The time the move to the stage's values takes: 0 (the whole stage) up to
// the stage's length. Nothing on a Hold stage.
void stepStageTransition(Stage& stage, int direction);
// Hold -> Linear -> Exponential -> Hold. Going to Hold drops the transition.
void cycleStageSweep(Stage& stage);
// A frequency of the stage by `delta` Hz. Below its range it becomes 0,
// "leave as it is"; from 0 a step up starts at `current`.
void stepStagePrimary(Stage& stage, float delta, float current);
void stepStageBeat(Stage& stage, float delta, float current);

// The same for a sequence.
bool addStep(Sequence& sequence, uint8_t index);
bool removeStep(Sequence& sequence, uint8_t index);
void stepStepHz(SequenceStep& step, float delta);
void stepStepSeconds(SequenceStep& step, int direction);
void stepGlide(Sequence& sequence, int direction);

// A short description of a stage for a list: "5:00 hold 200 / 10.0 Hz".
void describeStage(const Stage& stage, char* out, size_t capacity);

}  // namespace rc
