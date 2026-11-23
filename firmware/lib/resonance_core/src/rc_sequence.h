// Resonance portable core: frequency sequences (RES-SESS-012).
//
// A sequence is a named list of primary frequencies, each held for its own
// time, with an optional glide between them. It is a short way to write a
// session: sequenceSession() makes the session, which then runs, is changed
// and is stored like any other. The Solfeggio set is offered as a default
// to start from, not as a fixed program; nothing is claimed for any of its
// frequencies (RES-PROC-006).
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "rc_session.h"

namespace rc {

constexpr uint8_t kMaxSequenceSteps = kMaxStages;
constexpr uint16_t kSequenceGlideMaxS = 600;
constexpr size_t kSequenceLineMax = 64;

struct SequenceStep {
    float hz;          // primary frequency, kPrimaryMinHz..kPrimaryMaxHz
    uint16_t seconds;  // 1..65535
};

struct Sequence {
    char name[20];
    uint8_t stepCount;
    SequenceStep steps[kMaxSequenceSteps];
    uint16_t glideS;  // 0 = each step starts at once; otherwise the move to it takes this long
    float beatHz;     // 0 = leave the beat frequency as it is
};

bool isValidSequence(const Sequence& sequence);

// The Solfeggio set, two minutes a step, as a sequence to change.
Sequence defaultSequence();

// The session that plays `sequence`; no stages when it is not valid.
Session sequenceSession(const Sequence& sequence);

// Text form, as for sessions (rc_session_text.h): line `index` of the
// sequence, false past the last line or when it does not fit.
//   RESONANCE SEQUENCE 1 / name=... / glide=<s> / beat=<Hz> / step hz=<Hz> d=<s>
bool sequenceLine(const Sequence& sequence, uint8_t index, char* out, size_t capacity);

class SequenceParser {
public:
    void begin();
    // Takes one line. Returns false once the text is refused.
    bool line(const char* text);
    // The sequence, when the text was complete and valid.
    bool finish(Sequence& out) const;
    const char* error() const { return _error; }

private:
    bool fail(const char* why);
    Sequence _sequence = {};
    bool _header = false;
    const char* _error = "";
};

}  // namespace rc
