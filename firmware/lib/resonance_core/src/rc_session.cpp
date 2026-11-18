#include "rc_session.h"

#include <string.h>

#include "rc_settings.h"

namespace rc {

namespace {

// Factory sessions. The names say what the frequencies do and nothing about
// what a listener may experience (RES-PROC-006).
const Session kFactory[] = {
    {"10 to 6 Hz, 20 min", 3, {{120, 0, 10.0f, Sweep::Hold}, {840, 0, 6.0f, Sweep::Linear}, {240, 0, 6.0f, Sweep::Hold}}},
    {"8 to 2 Hz, 30 min", 3, {{180, 0, 8.0f, Sweep::Hold}, {1200, 0, 2.0f, Sweep::Linear}, {420, 0, 2.0f, Sweep::Hold}}},
    {"14 Hz hold, 25 min", 1, {{1500, 0, 14.0f, Sweep::Hold}}},
    {"6-10-6 Hz, 15 min", 4, {{180, 0, 6.0f, Sweep::Hold}, {300, 0, 10.0f, Sweep::Linear}, {120, 0, 10.0f, Sweep::Hold}, {300, 0, 6.0f, Sweep::Linear}}},
};

bool frequencyOk(float hz, float lo, float hi) { return hz == 0.0f || (hz >= lo && hz <= hi); }

}  // namespace

Session timerSession(uint16_t minutes) {
    Session s = {};
    strcpy(s.name, "Timer");
    s.stageCount = 1;
    // 1092 minutes is the most that fits the 16-bit seconds of a stage.
    if (minutes > 1092) minutes = 1092;
    s.stages[0] = {static_cast<uint16_t>(minutes * 60u), 0.0f, 0.0f, Sweep::Hold};
    return s;
}

uint32_t sessionSeconds(const Session& session) {
    uint32_t total = 0;
    for (uint8_t i = 0; i < session.stageCount && i < kMaxStages; ++i) total += session.stages[i].durationS;
    return total;
}

bool isValidSession(const Session& session) {
    if (session.stageCount < 1 || session.stageCount > kMaxStages) return false;
    for (uint8_t i = 0; i < session.stageCount; ++i) {
        const Stage& stage = session.stages[i];
        if (stage.durationS == 0 && session.stageCount != 1) return false;
        if (stage.sweep != Sweep::Hold && stage.sweep != Sweep::Linear) return false;
        // NaN fails both comparisons and is refused here.
        if (!frequencyOk(stage.primaryHz, kPrimaryMinHz, kPrimaryMaxHz)) return false;
        if (!frequencyOk(stage.beatHz, kBeatMinHz, kBeatMaxHz)) return false;
    }
    return true;
}

uint8_t factorySessionCount() { return sizeof(kFactory) / sizeof(kFactory[0]); }

const Session& factorySession(uint8_t index) { return kFactory[index < factorySessionCount() ? index : 0]; }

bool SessionRunner::start(const Session& session, float currentPrimaryHz, float currentBeatHz, uint16_t fadeOutS) {
    if (!isValidSession(session)) return false;
    _session = session;
    _totalMs = sessionSeconds(session) * 1000u;
    _fadeMs = static_cast<uint32_t>(fadeOutS) * 1000u;
    _elapsedMs = 0;
    _fadeElapsedMs = 0;
    _gain = 1.0f;
    _primaryHz = currentPrimaryHz;
    _beatHz = currentBeatHz;
    _state = SessionState::Running;
    enterStage(0);
    return true;
}

void SessionRunner::enterStage(uint8_t index) {
    _stage = index;
    _stageMs = 0;
    // The stage starts from wherever the values are now.
    _fromPrimary = _primaryHz;
    _fromBeat = _beatHz;
    const Stage& stage = _session.stages[index];
    if (stage.sweep == Sweep::Hold) {
        if (stage.primaryHz != 0.0f) _primaryHz = stage.primaryHz;
        if (stage.beatHz != 0.0f) _beatHz = stage.beatHz;
    }
}

void SessionRunner::beginFade() {
    _state = SessionState::FadingOut;
    _fadeElapsedMs = 0;
    if (_fadeMs == 0) {
        _gain = 0.0f;
        _state = SessionState::Finished;
    }
}

void SessionRunner::advance(uint32_t ms) {
    if (_state == SessionState::FadingOut) {
        _fadeElapsedMs += ms;
        if (_fadeElapsedMs >= _fadeMs) {
            _gain = 0.0f;
            _state = SessionState::Finished;
        } else {
            _gain = 1.0f - static_cast<float>(_fadeElapsedMs) / static_cast<float>(_fadeMs);
        }
        return;
    }
    if (_state != SessionState::Running) return;

    // A long step may cross several stage boundaries.
    while (ms > 0 && _state == SessionState::Running) {
        const Stage& stage = _session.stages[_stage];
        const uint32_t stageMs = static_cast<uint32_t>(stage.durationS) * 1000u;
        if (stageMs == 0) {  // endless
            _stageMs += ms;
            _elapsedMs += ms;
            break;
        }
        const uint32_t left = stageMs - _stageMs;
        const uint32_t step = ms < left ? ms : left;
        _stageMs += step;
        _elapsedMs += step;
        ms -= step;
        if (stage.sweep == Sweep::Linear) {
            const float t = static_cast<float>(_stageMs) / static_cast<float>(stageMs);
            if (stage.primaryHz != 0.0f) _primaryHz = _fromPrimary + (stage.primaryHz - _fromPrimary) * t;
            if (stage.beatHz != 0.0f) _beatHz = _fromBeat + (stage.beatHz - _fromBeat) * t;
        }
        if (_stageMs >= stageMs) {
            if (_stage + 1 < _session.stageCount) enterStage(static_cast<uint8_t>(_stage + 1));
            else beginFade();
        }
    }
}

void SessionRunner::pause() {
    if (_state == SessionState::Running) _state = SessionState::Paused;
}

void SessionRunner::resume() {
    if (_state == SessionState::Paused) _state = SessionState::Running;
}

void SessionRunner::stop() {
    if (_state == SessionState::Running || _state == SessionState::Paused) beginFade();
}

void SessionRunner::reset() {
    _state = SessionState::Idle;
    _gain = 1.0f;
    _elapsedMs = 0;
    _stage = 0;
}

uint32_t SessionRunner::remainingS() const {
    if (_totalMs == 0 || _state == SessionState::Idle || _state == SessionState::FadingOut ||
        _state == SessionState::Finished) {
        return 0;
    }
    return (_totalMs - _elapsedMs + 999u) / 1000u;
}

}  // namespace rc
