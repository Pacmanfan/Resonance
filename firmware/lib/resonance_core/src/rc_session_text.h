// Resonance portable core: the text form of a session, for the user's
// sessions on the card (RES-SESS-002, RES-CFG-002).
//
// A file is a few short lines, written and read one line at a time so that
// neither side needs a buffer for the whole file:
//
//     RESONANCE SESSION 1
//     name=Evening
//     endfade=60
//     stage d=120 sweep=hold b=10
//     stage d=600 t=300 sweep=exp b=6 phi=40 noise=pink nv=30
//
// Stage keys: d duration (s), t transition (s), sweep hold|lin|exp,
// p / b primary and beat (Hz, 0 or absent = leave alone), fp / fb where the
// sweep starts, phi / phi2 partial levels (percent), noise off|white|pink|
// brown, nv noise volume (percent). `endfade` is seconds or `device`.
// Blank lines and lines starting with # are skipped. A key or a line that
// this version does not know is skipped too, so that a file written by a
// later version still loads; a value that cannot be read is an error.
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "rc_session.h"

namespace rc {

// Room for the longest line sessionLine() writes, with its terminator.
constexpr size_t kSessionLineMax = 128;

// Writes line number `index` of the text form of `session` into `out`,
// without a line end. Returns false when `index` is past the last line or
// `capacity` is too small.
bool sessionLine(const Session& session, uint8_t index, char* out, size_t capacity);

class SessionParser {
public:
    void begin();
    // Takes one line, without or with its line end. Returns false from the
    // first bad line on.
    bool line(const char* text);
    // The session read, when every line was good and it is valid.
    bool finish(Session& out) const;
    // What was wrong; empty when nothing was.
    const char* error() const { return _error; }

private:
    bool fail(const char* why);
    bool stageLine(char* text);
    Session _session = {};
    bool _header = false;
    const char* _error = "";
};

}  // namespace rc
