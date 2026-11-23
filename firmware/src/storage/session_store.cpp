#include "session_store.h"

#include <Arduino.h>

#include "rc_sequence.h"
#include "rc_session_text.h"
#include "rc_state.h"
#include "sdcard.h"

namespace {

const char* lastError = "";

// The two kinds of file kept here.
struct Kind {
    const char* format;  // path, taking the slot and the extension
    uint8_t slots;       // 0: a single file, whose path takes the extension only
    const char* empty;   // what to say of an empty slot
};
const Kind kSessions = {"/RESONANCE/SESSIONS/USER%02u.%s", sessionstore::kSlots, "no session stored here"};
const Kind kSequences = {"/RESONANCE/SEQUENCE/SEQ%02u.%s", sequencestore::kSlots, "no sequence stored here"};
const Kind kState = {"/RESONANCE/ADVANCED.%s", 0, "no settings stored"};

bool pathOf(const Kind& kind, uint8_t slot, const char* extension, char* out, size_t capacity) {
    if (kind.slots == 0) {
        snprintf(out, capacity, kind.format, extension);
        return true;
    }
    if (slot < 1 || slot > kind.slots) return false;
    snprintf(out, capacity, kind.format, static_cast<unsigned>(slot), extension);
    return true;
}

// Reads one line into `out`, without its line end. Returns false at the end
// of the file; sets `tooLong` when the line did not fit (the rest of it is
// skipped).
bool readLine(fs::File& file, char* out, size_t capacity, bool& tooLong) {
    size_t n = 0;
    bool any = false;
    tooLong = false;
    while (file.available()) {
        const int c = file.read();
        if (c < 0) break;
        any = true;
        if (c == '\n') break;
        if (n + 1 < capacity) out[n++] = static_cast<char>(c);
        else tooLong = true;
    }
    out[n] = '\0';
    return any;
}

// Reads and validates the file of `slot` with `Parser`, a line at a time.
template <typename T, typename Parser, size_t LineMax>
bool loadText(const Kind& kind, uint8_t slot, T& out) {
    char path[40];
    if (!pathOf(kind, slot, "TXT", path, sizeof(path))) return (lastError = "no such slot"), false;
    fs::File file = sdcard::openForRead(path);
    if (!file) return (lastError = kind.empty), false;
    Parser parser;
    parser.begin();
    char line[LineMax];
    bool tooLong = false;
    // A file has a handful of lines; allow for comments, and stop a file
    // that is not one of ours early.
    for (int lines = 0; lines < 64 && readLine(file, line, sizeof(line), tooLong); ++lines) {
        if (tooLong) {
            file.close();
            return (lastError = "line too long"), false;
        }
        if (!parser.line(line)) break;
    }
    file.close();
    if (!parser.finish(out)) {
        lastError = parser.error()[0] != '\0' ? parser.error() : "values out of range";
        return false;
    }
    lastError = "";
    return true;
}

// Writes the lines `lineOf` gives under another name, then renames, so that
// an interrupted write never leaves half a file under the slot's name.
template <typename T, size_t LineMax>
bool saveText(const Kind& kind, uint8_t slot, const T& value, bool (*lineOf)(const T&, uint8_t, char*, size_t)) {
    char path[40], temporary[40];
    if (!pathOf(kind, slot, "TXT", path, sizeof(path)) || !pathOf(kind, slot, "TMP", temporary, sizeof(temporary))) {
        return (lastError = "no such slot"), false;
    }
    fs::File file = sdcard::openForWrite(temporary);
    if (!file) return (lastError = "cannot write to the card"), false;
    char line[LineMax];
    bool ok = true;
    for (uint8_t i = 0; ok && lineOf(value, i, line, sizeof(line)); ++i) {
        const size_t length = strlen(line);
        ok = file.write(reinterpret_cast<const uint8_t*>(line), length) == length && file.write('\n') == 1;
    }
    file.close();
    if (!ok) {
        sdcard::remove(temporary);
        return (lastError = "the card did not take the file"), false;
    }
    if (sdcard::exists(path)) sdcard::remove(path);
    if (!sdcard::rename(temporary, path)) return (lastError = "cannot rename the file"), false;
    lastError = "";
    return true;
}

bool usedSlot(const Kind& kind, uint8_t slot) {
    char path[40];
    return pathOf(kind, slot, "TXT", path, sizeof(path)) && sdcard::exists(path);
}

bool removeSlot(const Kind& kind, uint8_t slot) {
    char path[40];
    return pathOf(kind, slot, "TXT", path, sizeof(path)) && sdcard::exists(path) && sdcard::remove(path);
}

// The "name=" line of the file in `slot`, which both kinds have near the top.
bool nameOf(const Kind& kind, uint8_t slot, char* out, size_t capacity) {
    char path[40];
    if (capacity == 0 || !pathOf(kind, slot, "TXT", path, sizeof(path))) return false;
    fs::File file = sdcard::openForRead(path);
    if (!file) return false;
    char line[rc::kSequenceLineMax];
    bool tooLong = false, found = false;
    for (int lines = 0; lines < 8 && !found && readLine(file, line, sizeof(line), tooLong); ++lines) {
        if (!tooLong && strncmp(line, "name=", 5) == 0) {
            size_t length = strlen(line + 5);
            while (length > 0 && (line[5 + length - 1] == '\r' || line[5 + length - 1] == ' ')) --length;
            line[5 + length] = '\0';
            strlcpy(out, line + 5, capacity);
            found = out[0] != '\0';
        }
    }
    file.close();
    return found;
}

}  // namespace

namespace sessionstore {

const char* error() { return lastError; }
bool used(uint8_t slot) { return usedSlot(kSessions, slot); }
bool remove(uint8_t slot) { return removeSlot(kSessions, slot); }
bool name(uint8_t slot, char* out, size_t capacity) { return nameOf(kSessions, slot, out, capacity); }

bool load(uint8_t slot, rc::Session& out) {
    return loadText<rc::Session, rc::SessionParser, rc::kSessionLineMax>(kSessions, slot, out);
}

bool save(uint8_t slot, const rc::Session& session) {
    if (!rc::isValidSession(session)) return (lastError = "values out of range"), false;
    return saveText<rc::Session, rc::kSessionLineMax>(kSessions, slot, session, rc::sessionLine);
}

}  // namespace sessionstore

namespace sequencestore {

const char* error() { return lastError; }
bool used(uint8_t slot) { return usedSlot(kSequences, slot); }
bool remove(uint8_t slot) { return removeSlot(kSequences, slot); }
bool name(uint8_t slot, char* out, size_t capacity) { return nameOf(kSequences, slot, out, capacity); }

bool load(uint8_t slot, rc::Sequence& out) {
    return loadText<rc::Sequence, rc::SequenceParser, rc::kSequenceLineMax>(kSequences, slot, out);
}

bool save(uint8_t slot, const rc::Sequence& sequence) {
    if (!rc::isValidSequence(sequence)) return (lastError = "values out of range"), false;
    return saveText<rc::Sequence, rc::kSequenceLineMax>(kSequences, slot, sequence, rc::sequenceLine);
}

}  // namespace sequencestore

namespace statestore {

const char* error() { return lastError; }
bool used() { return usedSlot(kState, 0); }
bool remove() { return removeSlot(kState, 0); }

bool load(rc::AdvancedState& out) {
    return loadText<rc::AdvancedState, rc::StateParser, rc::kStateLineMax>(kState, 0, out);
}

bool save(const rc::AdvancedState& state) {
    if (!rc::isValidState(state)) return (lastError = "values out of range"), false;
    return saveText<rc::AdvancedState, rc::kStateLineMax>(kState, 0, state, rc::stateLine);
}

}  // namespace statestore
