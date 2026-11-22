#include "rc_session.h"

#include <math.h>
#include <string.h>

#include "rc_settings.h"

namespace rc {

namespace {

// Factory sessions. The first four are named for what the frequencies do
// (20, 30, 25 and 15 minutes as written). The last three are named for the
// practice they are meant to accompany and follow the theta-range values
// commonly used for it: a slow descent from alpha, with long holds. A name
// says what a program is for; nothing here or on the screen claims that it
// produces an effect (RES-PROC-006). Lengths can be changed with
// scaledSession().
const Session kFactory[] = {
    {"10 to 6 Hz", 3, {{120, 0, 10.0f, Sweep::Hold}, {840, 0, 6.0f, Sweep::Linear}, {240, 0, 6.0f, Sweep::Hold}}},
    {"8 to 2 Hz", 3, {{180, 0, 8.0f, Sweep::Hold}, {1200, 0, 2.0f, Sweep::Linear}, {420, 0, 2.0f, Sweep::Hold}}},
    {"14 Hz hold", 1, {{1500, 0, 14.0f, Sweep::Hold}}},
    {"6-10-6 Hz", 4, {{180, 0, 6.0f, Sweep::Hold}, {300, 0, 10.0f, Sweep::Linear}, {120, 0, 10.0f, Sweep::Hold}, {300, 0, 6.0f, Sweep::Linear}}},
    // Trance, 30 minutes: 10 Hz, down to 6 Hz, hold, down to 4.5 Hz, hold.
    {"Trance 10-4.5 Hz", 5, {{120, 0, 10.0f, Sweep::Hold}, {480, 0, 6.0f, Sweep::Linear}, {360, 0, 6.0f, Sweep::Hold},
                              {360, 0, 4.5f, Sweep::Linear}, {480, 0, 4.5f, Sweep::Hold}}},
    // Deep trance, 40 minutes: 8 Hz, down to 5 Hz, hold, down to 3.5 Hz, long hold.
    {"Deep trance 3.5 Hz", 5, {{120, 0, 8.0f, Sweep::Hold}, {480, 0, 5.0f, Sweep::Linear}, {360, 0, 5.0f, Sweep::Hold},
                               {480, 0, 3.5f, Sweep::Linear}, {960, 0, 3.5f, Sweep::Hold}}},
    // Astral, 45 minutes: 10 Hz, down to 6.3 Hz, hold, down to 4 Hz, long hold.
    {"Astral 6.3-4 Hz", 5, {{180, 0, 10.0f, Sweep::Hold}, {600, 0, 6.3f, Sweep::Linear}, {600, 0, 6.3f, Sweep::Hold},
                            {420, 0, 4.0f, Sweep::Linear}, {900, 0, 4.0f, Sweep::Hold}}},
};

bool frequencyOk(float hz, float lo, float hi) { return hz == 0.0f || (hz >= lo && hz <= hi); }

}  // namespace

float sweepValue(Sweep sweep, float from, float to, float fraction) {
    if (sweep == Sweep::Hold) return to;
    if (!(fraction > 0.0f)) return from;
    if (fraction >= 1.0f) return to;
    if (sweep == Sweep::Exponential && from > 0.0f && to > 0.0f) return from * powf(to / from, fraction);
    return from + (to - from) * fraction;
}

Session sweepSession(Sweep sweep, float fromPrimaryHz, float toPrimaryHz, float fromBeatHz, float toBeatHz,
                     uint16_t seconds) {
    Session s = {};
    strcpy(s.name, "Sweep");
    s.stageCount = 1;
    s.stages[0] = {static_cast<uint16_t>(seconds == 0 ? 1 : seconds), toPrimaryHz, toBeatHz, sweep, fromPrimaryHz,
                   fromBeatHz};
    return s;
}

Session descentSession(const Descent& d) {
    Session s = {};
    strcpy(s.name, "Descent");
    const uint32_t stageS = static_cast<uint32_t>(d.transitionS) + d.holdS;
    const bool shapeOk = d.sweep == Sweep::Linear || d.sweep == Sweep::Exponential;
    const uint8_t lead = d.startHoldS != 0 ? 1 : 0;
    if (!shapeOk || d.transitionS == 0 || stageS > 65535 || d.targetCount == 0 ||
        d.targetCount > kMaxDescentTargets || d.targetCount + lead > kMaxStages) {
        return s;  // no stages: refused
    }
    uint8_t n = 0;
    if (lead != 0) s.stages[n++] = {d.startHoldS, d.startPrimaryHz, d.startBeatHz, Sweep::Hold};
    for (uint8_t i = 0; i < d.targetCount; ++i) {
        Stage stage = {static_cast<uint16_t>(stageS), d.targets[i].primaryHz, d.targets[i].beatHz, d.sweep};
        stage.transitionS = d.holdS != 0 ? d.transitionS : 0;  // no hold: the sweep fills the stage
        // Without a lead-in the first move starts from the start values.
        if (i == 0 && lead == 0) {
            if (stage.primaryHz != 0.0f) stage.fromPrimaryHz = d.startPrimaryHz;
            if (stage.beatHz != 0.0f) stage.fromBeatHz = d.startBeatHz;
        }
        s.stages[n++] = stage;
    }
    s.stageCount = n;
    return s;
}

Session timerSession(uint16_t minutes) {
    Session s = {};
    strcpy(s.name, "Timer");
    s.stageCount = 1;
    // 1092 minutes is the most that fits the 16-bit seconds of a stage.
    if (minutes > 1092) minutes = 1092;
    s.stages[0] = {static_cast<uint16_t>(minutes * 60u), 0.0f, 0.0f, Sweep::Hold, 0.0f, 0.0f};
    return s;
}

Session scaledSession(const Session& session, uint16_t minutes) {
    const uint32_t from = sessionSeconds(session);
    const uint32_t to = static_cast<uint32_t>(minutes) * 60u;
    if (from == 0 || to == 0 || from == to) return session;
    Session out = session;
    uint32_t given = 0;
    for (uint8_t i = 0; i < out.stageCount && i < kMaxStages; ++i) {
        const bool last = i + 1 == out.stageCount;
        // The last stage takes what is left, so that rounding cannot change
        // the total.
        uint64_t seconds = last ? (to > given ? to - given : 1)
                                : (static_cast<uint64_t>(session.stages[i].durationS) * to + from / 2) / from;
        if (seconds < 1) seconds = 1;
        if (seconds > 65535) seconds = 65535;
        // The transition keeps its share of the stage.
        const uint16_t was = out.stages[i].durationS;
        if (out.stages[i].transitionS != 0 && was != 0) {
            uint64_t t = static_cast<uint64_t>(out.stages[i].transitionS) * seconds / was;
            if (t < 1) t = 1;
            if (t > seconds) t = seconds;
            out.stages[i].transitionS = static_cast<uint16_t>(t);
        }
        out.stages[i].durationS = static_cast<uint16_t>(seconds);
        given += static_cast<uint32_t>(seconds);
    }
    return out;
}

uint32_t sessionSeconds(const Session& session) {
    uint32_t total = 0;
    for (uint8_t i = 0; i < session.stageCount && i < kMaxStages; ++i) total += session.stages[i].durationS;
    return total;
}

bool isValidSession(const Session& session) {
    if (session.stageCount < 1 || session.stageCount > kMaxStages) return false;
    if (session.endFadeS != kFadeDeviceChoice && session.endFadeS > kEndFadeMaxS) return false;
    if (memchr(session.name, '\0', sizeof(session.name)) == nullptr) return false;
    for (uint8_t i = 0; i < session.stageCount; ++i) {
        const Stage& stage = session.stages[i];
        if (stage.durationS == 0 && session.stageCount != 1) return false;
        if (stage.sweep != Sweep::Hold && stage.sweep != Sweep::Linear && stage.sweep != Sweep::Exponential) {
            return false;
        }
        if (!frequencyOk(stage.fromPrimaryHz, kPrimaryMinHz, kPrimaryMaxHz)) return false;
        if (!frequencyOk(stage.fromBeatHz, kBeatMinHz, kBeatMaxHz)) return false;
        if (stage.transitionS != 0 && (stage.sweep == Sweep::Hold || stage.transitionS > stage.durationS)) return false;
        if ((stage.flags & ~kStageKnownFlags) != 0) return false;
        if (stage.partial[0] > 100 || stage.partial[1] > 100 || stage.noiseVolume > 100 || stage.noiseType > 3) {
            return false;
        }
        // A start makes sense only where the stage names an end.
        if (stage.fromPrimaryHz != 0.0f && stage.primaryHz == 0.0f) return false;
        if (stage.fromBeatHz != 0.0f && stage.beatHz == 0.0f) return false;
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
    if (session.endFadeS != kFadeDeviceChoice) fadeOutS = session.endFadeS;
    _fadeMs = static_cast<uint32_t>(fadeOutS) * 1000u;
    for (float& v : _layerFrom) v = 0.0f;
    for (float& v : _layerNow) v = 0.0f;
    _flags = 0;
    _noiseType = 0;
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
    for (uint8_t i = 0; i < 3; ++i) _layerFrom[i] = _layerNow[i];
    _flags = stage.flags;
    if ((stage.flags & kStageSetsNoise) != 0) _noiseType = stage.noiseType;
    if (stage.sweep == Sweep::Hold) {
        if ((stage.flags & kStageSetsPartials) != 0) {
            _layerNow[0] = stage.partial[0];
            _layerNow[1] = stage.partial[1];
        }
        if ((stage.flags & kStageSetsNoise) != 0) _layerNow[2] = stage.noiseVolume;
    }
    if (stage.sweep == Sweep::Hold) {
        if (stage.primaryHz != 0.0f) _primaryHz = stage.primaryHz;
        if (stage.beatHz != 0.0f) _beatHz = stage.beatHz;
    } else {
        // A sweep with a start of its own begins there.
        if (stage.fromPrimaryHz != 0.0f) _primaryHz = _fromPrimary = stage.fromPrimaryHz;
        if (stage.fromBeatHz != 0.0f) _beatHz = _fromBeat = stage.fromBeatHz;
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
        if (stage.sweep != Sweep::Hold) {
            // With a transition time the sweep ends there and the stage holds.
            const uint32_t sweepMs = stage.transitionS != 0 ? static_cast<uint32_t>(stage.transitionS) * 1000u : stageMs;
            const float t = _stageMs >= sweepMs ? 1.0f : static_cast<float>(_stageMs) / static_cast<float>(sweepMs);
            if ((stage.flags & kStageSetsPartials) != 0) {
                _layerNow[0] = _layerFrom[0] + (stage.partial[0] - _layerFrom[0]) * t;
                _layerNow[1] = _layerFrom[1] + (stage.partial[1] - _layerFrom[1]) * t;
            }
            if ((stage.flags & kStageSetsNoise) != 0) _layerNow[2] = _layerFrom[2] + (stage.noiseVolume - _layerFrom[2]) * t;
            if (stage.primaryHz != 0.0f) _primaryHz = sweepValue(stage.sweep, _fromPrimary, stage.primaryHz, t);
            if (stage.beatHz != 0.0f) _beatHz = sweepValue(stage.sweep, _fromBeat, stage.beatHz, t);
        }
        if (_stageMs >= stageMs) {
            if (_stage + 1 < _session.stageCount) enterStage(static_cast<uint8_t>(_stage + 1));
            else beginFade();
        }
    }
}

void SessionRunner::setLayersNow(uint8_t partial0, uint8_t partial1, uint8_t noiseVolume) {
    const float now[3] = {static_cast<float>(partial0), static_cast<float>(partial1), static_cast<float>(noiseVolume)};
    const Stage& stage = _session.stages[_stage];
    for (uint8_t i = 0; i < 3; ++i) {
        _layerFrom[i] = now[i];
        // A first stage that jumps has already taken its own values.
        const bool taken = _state != SessionState::Idle && stage.sweep == Sweep::Hold &&
                           (stage.flags & (i < 2 ? kStageSetsPartials : kStageSetsNoise)) != 0;
        if (!taken) _layerNow[i] = now[i];
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
