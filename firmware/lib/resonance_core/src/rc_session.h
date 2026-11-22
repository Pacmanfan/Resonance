// Resonance portable core: sessions (RES-SESS-001..005).
//
// A session is a list of stages. Each stage lasts a given time and either
// holds its frequencies or moves to them in a straight line from where the
// previous stage ended. The runner turns elapsed time into target values; it
// knows nothing of oscillators, screens or files, and whoever owns it applies
// the values (RES-SESS-001: automation separate from the oscillator).
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace rc {

enum class Sweep : uint8_t {
    Hold,    // jump to the stage's values at its start and stay there
    Linear,  // glide from the previous values to the stage's values over the stage
    Exponential,  // the same, in equal ratios per unit of time (RES-SESS-006)
};

// The value of a sweep of shape `sweep` from `from` to `to` at `fraction`
// (0..1) of its time. Hold gives `to` throughout. Both ends must be above
// zero for an exponential sweep; otherwise it falls back to a straight line.
float sweepValue(Sweep sweep, float from, float to, float fraction);

struct Stage {
    uint16_t durationS;  // 0 only in a one-stage session: runs until stopped
    float primaryHz;     // 0 = leave the primary frequency as it is
    float beatHz;        // 0 = leave the beat frequency as it is
    Sweep sweep;
    // Where a sweep starts (RES-SESS-006). 0 = from wherever the previous
    // stage ended, which is the behaviour of every factory program. Given a
    // value, the stage jumps there at its start (the tone engine glides the
    // jump) and sweeps on from it. Not used by a Hold stage.
    float fromPrimaryHz = 0.0f;
    float fromBeatHz = 0.0f;
};

// A session of one stage that sweeps from one pair of frequencies to
// another in `seconds` (1..65535): the custom sweep. A frequency of 0 on
// both ends leaves that frequency alone.
struct Session;
Session sweepSession(Sweep sweep, float fromPrimaryHz, float toPrimaryHz, float fromBeatHz, float toBeatHz,
                     uint16_t seconds);

constexpr uint8_t kMaxStages = 6;

struct Session {
    char name[20];
    uint8_t stageCount;
    Stage stages[kMaxStages];
};

// A session that holds the current frequencies for `minutes` (0 = until
// stopped): the plain timer of RES-SESS-003.
Session timerSession(uint16_t minutes);

// The same session stretched or shortened to last `minutes` in all: every
// stage keeps its share of the whole, and no stage becomes shorter than one
// second. A session that runs until stopped, and a length of zero, are
// returned unchanged.
Session scaledSession(const Session& session, uint16_t minutes);

// Total length in seconds; 0 for a session that runs until stopped.
uint32_t sessionSeconds(const Session& session);

// True when the session is well formed: 1..kMaxStages stages, frequencies in
// range or zero, a zero duration only as the single stage.
bool isValidSession(const Session& session);

// Sessions compiled into the firmware.
uint8_t factorySessionCount();
const Session& factorySession(uint8_t index);

enum class SessionState : uint8_t {
    Idle,       // nothing running
    Running,
    Paused,
    FadingOut,  // the stages are over (or stop was asked); the level is going down
    Finished,   // the fade is complete: turn the audio off, then call reset()
};

class SessionRunner {
public:
    // Starts `session` from the given current frequencies. `fadeOutS` is the
    // length of the fade at the end (0 = immediate). Returns false, and stays
    // idle, when the session is not valid.
    bool start(const Session& session, float currentPrimaryHz, float currentBeatHz, uint16_t fadeOutS);

    // Advances the session by `ms` of real time. Has no effect while paused.
    void advance(uint32_t ms);

    void pause();
    void resume();

    // Ends the session early, through the same fade as a normal end.
    void stop();

    // Back to idle, from any state.
    void reset();

    SessionState state() const { return _state; }

    // Values to apply now. The frequencies are 0 where the session leaves
    // them alone.
    float primaryHz() const { return _primaryHz; }
    float beatHz() const { return _beatHz; }
    // Level of the whole output, 1 during the stages and falling to 0 in the
    // fade.
    float gain() const { return _gain; }

    // Seconds of stages left; 0 for a session that runs until stopped and
    // during the fade.
    uint32_t remainingS() const;
    bool endless() const { return _totalMs == 0; }
    uint8_t stageIndex() const { return _stage; }
    uint8_t stageCount() const { return _session.stageCount; }

private:
    void enterStage(uint8_t index);
    void beginFade();

    Session _session = {};
    SessionState _state = SessionState::Idle;
    uint8_t _stage = 0;
    uint32_t _stageMs = 0;      // time spent in the current stage
    uint32_t _elapsedMs = 0;    // time spent in all stages
    uint32_t _totalMs = 0;      // 0 = endless
    uint32_t _fadeMs = 0;       // length of the fade
    uint32_t _fadeElapsedMs = 0;
    float _fromPrimary = 0, _fromBeat = 0;  // values at the start of the stage
    float _primaryHz = 0, _beatHz = 0;
    float _gain = 1.0f;
};

}  // namespace rc
