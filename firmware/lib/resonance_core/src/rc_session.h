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

// Room for a stage's recording, with its closing NUL.
constexpr size_t kStageVoiceLength = 64;

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
    // Time the move to the stage's values takes (RES-SESS-007). 0 = as
    // before: a sweep takes the whole stage. Given a value, a sweep takes
    // that long and the stage holds its values for the rest: a transition
    // followed by a hold. Not allowed on a Hold stage.
    uint16_t transitionS = 0;
    // Layers the stage sets (kStageSets...); a layer whose bit is clear is
    // left exactly as the user has it, which is what every factory program
    // does.
    uint8_t flags = 0;
    uint8_t partial[2] = {0, 0};  // golden partials phi and phi squared, percent
    uint8_t noiseType = 0;        // rc::NoiseType
    uint8_t noiseVolume = 0;      // percent
    // The recording the stage plays (RES-SESS-010): a path below the sound
    // library, such as "VOICE/Body scan.mp3". Empty with the flag set stops
    // what is playing.
    uint8_t voiceVolume = 0;      // percent
    char voice[kStageVoiceLength] = "";
};

constexpr uint8_t kStageSetsPartials = 0x01;
constexpr uint8_t kStageSetsNoise = 0x02;
constexpr uint8_t kStageSetsVoice = 0x04;
constexpr uint8_t kStageKnownFlags = kStageSetsPartials | kStageSetsNoise | kStageSetsVoice;

// Session::endFadeS when the fade is left to whoever starts the session.
constexpr uint16_t kFadeDeviceChoice = 0xFFFF;
constexpr uint16_t kEndFadeMaxS = 600;

// A session of one stage that sweeps from one pair of frequencies to
// another in `seconds` (1..65535): the custom sweep. A frequency of 0 on
// both ends leaves that frequency alone.
struct Session;
Session sweepSession(Sweep sweep, float fromPrimaryHz, float toPrimaryHz, float fromBeatHz, float toBeatHz,
                     uint16_t seconds);

constexpr uint8_t kMaxStages = 10;

struct Session {
    char name[20];
    uint8_t stageCount;
    Stage stages[kMaxStages];
    // Fade at the end, in seconds; kFadeDeviceChoice = the length given to
    // SessionRunner::start(), as for the factory programs.
    uint16_t endFadeS = kFadeDeviceChoice;
};

// Progressive descent (RES-SESS-008): from a starting pair of frequencies
// through up to kMaxDescentTargets further pairs, each reached in
// `transitionS` and then held for `holdS`.
constexpr uint8_t kMaxDescentTargets = 5;
struct DescentTarget {
    float primaryHz;  // 0 = leave the primary frequency alone
    float beatHz;     // 0 = leave the beat frequency alone
};
struct Descent {
    float startPrimaryHz;  // 0 = start from the current value
    float startBeatHz;
    uint16_t startHoldS;   // time at the start values before the first move
    Sweep sweep;           // Linear or Exponential
    uint16_t transitionS;  // at least 1
    uint16_t holdS;
    uint8_t targetCount;
    DescentTarget targets[kMaxDescentTargets];
};
// The session a descent describes. For a descent that makes no sense the
// result has no stages, which isValidSession() and start() refuse.
Session descentSession(const Descent& descent);

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
    // Where the layers stand before the session, in percent, so that the
    // first stage that sets them moves from there instead of jumping. Call
    // after start().
    void setLayersNow(uint8_t partial0, uint8_t partial1, uint8_t noiseVolume);
    // Layers the current stage sets, and their values now. While a stage
    // sweeps, its levels move with it in a straight line.
    bool setsPartials() const { return (_flags & kStageSetsPartials) != 0; }
    bool setsNoise() const { return (_flags & kStageSetsNoise) != 0; }
    uint8_t partialPercent(uint8_t index) const { return toPercent(_layerNow[index < 2 ? index : 0]); }
    uint8_t noiseVolume() const { return toPercent(_layerNow[2]); }
    uint8_t noiseType() const { return _noiseType; }
    // True once after a stage that sets the recording has been entered; its
    // file (empty = stop) and level are then those of stageVoice().
    bool takeVoiceChange() {
        const bool pending = _voicePending;
        _voicePending = false;
        return pending;
    }
    const char* stageVoice() const { return _session.stages[_stage].voice; }
    uint8_t stageVoiceVolume() const { return _session.stages[_stage].voiceVolume; }

    uint8_t stageIndex() const { return _stage; }
    uint8_t stageCount() const { return _session.stageCount; }

private:
    void enterStage(uint8_t index);
    static uint8_t toPercent(float v) { return static_cast<uint8_t>(v < 0.0f ? 0.0f : (v > 100.0f ? 100.0f : v + 0.5f)); }
    float _layerFrom[3] = {0, 0, 0};  // partial 0, partial 1, noise volume
    float _layerNow[3] = {0, 0, 0};
    uint8_t _flags = 0;
    uint8_t _noiseType = 0;
    bool _voicePending = false;
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
