// Mindfield: collecting an experiment (RES-MIND-002, RES-MIND-004).
//
// A sampling task takes one sample from the chosen source on a fixed
// schedule - so many a second, counted in system ticks, whatever the screen
// or the user are doing - and hands it over through a ring. The main loop
// (tick()) feeds the statistics, writes the raw file and ends the
// experiment. Nothing here runs in, or is waited for by, the audio path.
//
// The target, the source and every other setting are fixed by start() and
// cannot be changed until the experiment has ended.
#pragma once

#include <stdint.h>

#include "rc_mindfile.h"
#include "rc_mindstats.h"

namespace mindfield {

enum class State : uint8_t { Idle, Running, Paused, Finished };

struct Setup {
    uint8_t source = 0;  // index for rngsources::at()
    rc::MindCondition condition = rc::MindCondition::Baseline;
    rc::MindMode mode = rc::MindMode::Open;
    rc::MindTarget target = rc::MindTarget::None;
    rc::MindViz viz = rc::MindViz::Orb;
    uint16_t rate = 100;      // samples a second: 100, 250, 500 or 1000
    uint16_t seconds = 300;   // 0 = until stopped
    uint32_t seed = 1;        // software source only
};

// What else was going on, for the record.
struct Context {
    const char* firmware = "";
    const char* audio = "";  // name of the running session, or empty
    float primaryHz = 0.0f;
    float beatHz = 0.0f;
    bool audioOn = false;
};

struct Live {
    State state = State::Idle;
    uint32_t id = 0;
    bool recorded = false;       // a raw file is being, or was, written
    uint32_t samples = 0;
    uint32_t plannedSamples = 0; // 0 = until stopped
    uint32_t elapsedS = 0;
    uint32_t windows = 0;
    uint32_t gaps = 0;
    float z = 0.0f;              // bit balance so far
    float directedZ = 0.0f;      // towards the target; 0 without one
    float windowZ = 0.0f;        // last completed window
    uint32_t windowSample = 0;   // the sample that completed it
    bool replaying = false;      // a recorded file is being played again: nothing is collected
};

constexpr uint16_t kRates[] = {100, 250, 500, 1000};
constexpr uint8_t kRateCount = sizeof(kRates) / sizeof(kRates[0]);
constexpr uint32_t kWindowSeconds = 1;  // a measurement window is one second of samples

// Starts an experiment. False when it cannot start; message() says why.
bool start(const Setup& setup, const Context& context);
void pause();
void resume();
// Ends a running experiment now; the data so far is kept and summarised.
void stop();
// From the main loop.
void tick();
// Forgets a finished experiment's result (not its files).
void clear();

const Live& live();
const rc::MindHeader& header();
// Valid once the state is Finished.
const rc::MindSummary& summary();
rc::MindEnd endReason();
const char* message();

// Plays raw file `id` again, a window at each tick(), as if it were running:
// live() and the pictures follow it, nothing is collected or written, and it
// ends in the Finished state with the statistics recomputed from the file.
// stop() ends it early. False when the file is missing or refused, or an
// experiment is running.
bool replayStart(uint32_t id);

// The statistics of the running experiment, for a picture to read. Null
// when none is running.
const rc::MindStats* stats();

// ---- experiments on the card -------------------------------------------------

constexpr uint8_t kHistoryMax = 48;  // the newest this many are listed

// The summaries on the card, newest first: fills `out` from position `skip`.
uint8_t list(uint8_t skip, rc::MindRecord* out, uint8_t capacity, uint8_t* total);
// Writes summaries for raw files that have none (an experiment that was
// interrupted), marked as interrupted. Returns how many it made.
uint8_t recover();
// Writes every summary as one line of EXPORTS/SUMMARY.CSV.
bool exportCsv(uint8_t* rows);
// Reads raw file `id` through `visitor`, recomputing its statistics into
// `out`. False when the file is missing or refused.
bool replay(uint32_t id, rc::MindFileVisitor* visitor, rc::MindHeader& headerOut, rc::MindSummary& out, bool* intact);

}  // namespace mindfield
