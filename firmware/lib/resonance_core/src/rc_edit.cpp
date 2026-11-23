#include "rc_edit.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "rc_settings.h"

namespace rc {

namespace {

// One step from `value`, kept inside lo..hi.
uint16_t steppedTime(uint16_t value, int direction, uint16_t lo, uint16_t hi) {
    // Going down, the step is the one that led up to this value.
    const uint16_t step = timeStep(direction < 0 && value > 0 ? static_cast<uint16_t>(value - 1) : value);
    int32_t next = static_cast<int32_t>(value) + (direction < 0 ? -static_cast<int32_t>(step) : step);
    // Onto the grid of the step, so that odd values read from a file settle.
    next = (next / step) * step;
    if (direction > 0 && next <= value) next = value + step;
    if (next < lo) next = lo;
    if (next > hi) next = hi;
    return static_cast<uint16_t>(next);
}

float steppedHz(float value, float delta, float current, float lo, float hi) {
    if (!isfinite(delta)) return value;
    if (value == 0.0f) {
        if (delta <= 0.0f) return 0.0f;
        return sanitizeFrequency(current, lo, hi, lo);
    }
    const float next = value + delta;
    if (next < lo) return delta < 0.0f && value <= lo ? 0.0f : lo;
    return sanitizeFrequency(next, lo, hi, value);
}

}  // namespace

Session newSession() {
    Session s = {};
    strcpy(s.name, "My session");
    s.stageCount = 1;
    s.stages[0].durationS = 300;
    s.stages[0].sweep = Sweep::Hold;
    return s;
}

bool addStage(Session& s, uint8_t index) {
    if (s.stageCount >= kMaxStages || index >= s.stageCount) return false;
    for (uint8_t i = s.stageCount; i > index + 1; --i) s.stages[i] = s.stages[i - 1];
    s.stages[index + 1] = s.stages[index];
    // A session that ran until stopped gets a length once it has two stages.
    if (s.stages[index].durationS == 0) s.stages[index].durationS = s.stages[index + 1].durationS = 300;
    ++s.stageCount;
    return true;
}

bool removeStage(Session& s, uint8_t index) {
    if (s.stageCount <= 1 || index >= s.stageCount) return false;
    for (uint8_t i = index; i + 1 < s.stageCount; ++i) s.stages[i] = s.stages[i + 1];
    --s.stageCount;
    s.stages[s.stageCount] = Stage{};
    return true;
}

bool moveStage(Session& s, uint8_t index, int direction) {
    if (index >= s.stageCount) return false;
    if (direction < 0 && index == 0) return false;
    if (direction > 0 && index + 1 >= s.stageCount) return false;
    if (direction == 0) return false;
    const uint8_t other = static_cast<uint8_t>(direction < 0 ? index - 1 : index + 1);
    const Stage keep = s.stages[index];
    s.stages[index] = s.stages[other];
    s.stages[other] = keep;
    return true;
}

uint16_t timeStep(uint16_t seconds) { return seconds < 120 ? 10 : (seconds < 600 ? 30 : 60); }

void stepStageDuration(Stage& stage, int direction) {
    if (direction == 0) return;
    stage.durationS = steppedTime(stage.durationS, direction, kEditMinStageS, kEditMaxStageS);
    if (stage.transitionS > stage.durationS) stage.transitionS = stage.durationS;
}

void stepStageTransition(Stage& stage, int direction) {
    if (direction == 0 || stage.sweep == Sweep::Hold) return;
    const uint16_t limit = stage.durationS;
    // 0 means the whole stage, and so does a transition as long as it.
    if (direction > 0 && stage.transitionS == 0) return;
    uint16_t next = steppedTime(stage.transitionS == 0 ? limit : stage.transitionS, direction, 0, limit);
    if (direction < 0 && stage.transitionS == 0) next = steppedTime(limit, -1, kEditMinStageS, limit);
    if (next < kEditMinStageS) next = kEditMinStageS < limit ? kEditMinStageS : limit;
    stage.transitionS = next >= limit ? 0 : next;
}

void cycleStageSweep(Stage& stage) {
    if (stage.sweep == Sweep::Hold) {
        stage.sweep = Sweep::Linear;
    } else if (stage.sweep == Sweep::Linear) {
        stage.sweep = Sweep::Exponential;
    } else {
        stage.sweep = Sweep::Hold;
        stage.transitionS = 0;
        stage.fromPrimaryHz = 0.0f;
        stage.fromBeatHz = 0.0f;
    }
}

void stepStagePrimary(Stage& stage, float delta, float current) {
    stage.primaryHz = steppedHz(stage.primaryHz, delta, current, kPrimaryMinHz, kPrimaryMaxHz);
    if (stage.primaryHz == 0.0f) stage.fromPrimaryHz = 0.0f;  // a start needs an end
}

void stepStageBeat(Stage& stage, float delta, float current) {
    stage.beatHz = steppedHz(stage.beatHz, delta, current, kBeatMinHz, kBeatMaxHz);
    if (stage.beatHz == 0.0f) stage.fromBeatHz = 0.0f;
}

bool addStep(Sequence& s, uint8_t index) {
    if (s.stepCount >= kMaxSequenceSteps || index >= s.stepCount) return false;
    for (uint8_t i = s.stepCount; i > index + 1; --i) s.steps[i] = s.steps[i - 1];
    s.steps[index + 1] = s.steps[index];
    ++s.stepCount;
    return true;
}

bool removeStep(Sequence& s, uint8_t index) {
    if (s.stepCount <= 1 || index >= s.stepCount) return false;
    for (uint8_t i = index; i + 1 < s.stepCount; ++i) s.steps[i] = s.steps[i + 1];
    --s.stepCount;
    return true;
}

void stepStepHz(SequenceStep& step, float delta) {
    if (!isfinite(delta)) return;
    step.hz = sanitizeFrequency(step.hz + delta, kPrimaryMinHz, kPrimaryMaxHz, step.hz);
}

void stepStepSeconds(SequenceStep& step, int direction) {
    if (direction == 0) return;
    step.seconds = steppedTime(step.seconds, direction, kEditMinStageS, kEditMaxStageS);
}

void stepGlide(Sequence& s, int direction) {
    if (direction == 0) return;
    int32_t next = static_cast<int32_t>(s.glideS) + (direction < 0 ? -5 : 5);
    if (next < 0) next = 0;
    if (next > kSequenceGlideMaxS) next = kSequenceGlideMaxS;
    s.glideS = static_cast<uint16_t>(next);
}

void describeStage(const Stage& stage, char* out, size_t capacity) {
    static const char* const kShape[] = {"hold", "lin", "exp"};
    const uint8_t shape = static_cast<uint8_t>(stage.sweep);
    char primary[12] = "-", beat[12] = "-";
    if (stage.primaryHz != 0.0f) snprintf(primary, sizeof(primary), "%.0f", static_cast<double>(stage.primaryHz));
    if (stage.beatHz != 0.0f) snprintf(beat, sizeof(beat), "%.1f", static_cast<double>(stage.beatHz));
    snprintf(out, capacity, "%u:%02u %s %s / %s Hz", static_cast<unsigned>(stage.durationS / 60),
             static_cast<unsigned>(stage.durationS % 60), shape < 3 ? kShape[shape] : "?", primary, beat);
}

}  // namespace rc
