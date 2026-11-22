#include "rc_session_text.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace rc {

namespace {

const char kHeader[] = "RESONANCE SESSION";
const char* const kSweepNames[] = {"hold", "lin", "exp"};
const char* const kNoiseNames[] = {"off", "white", "pink", "brown"};

// Appends formatted text; false when it did not fit.
bool append(char* out, size_t capacity, size_t& used, const char* format, double value) {
    const int n = snprintf(out + used, capacity - used, format, value);
    if (n < 0 || static_cast<size_t>(n) >= capacity - used) return false;
    used += static_cast<size_t>(n);
    return true;
}

bool appendText(char* out, size_t capacity, size_t& used, const char* a, const char* b) {
    const int n = snprintf(out + used, capacity - used, "%s%s", a, b);
    if (n < 0 || static_cast<size_t>(n) >= capacity - used) return false;
    used += static_cast<size_t>(n);
    return true;
}

// A whole number of at most five digits, with nothing after it.
bool readCount(const char* text, uint32_t limit, uint32_t& out) {
    if (*text < '0' || *text > '9') return false;
    char* end = nullptr;
    const unsigned long v = strtoul(text, &end, 10);
    if (*end != '\0' || end - text > 5 || v > limit) return false;
    out = static_cast<uint32_t>(v);
    return true;
}

bool readHz(const char* text, float& out) {
    if ((*text < '0' || *text > '9') && *text != '.') return false;  // no sign, no "nan", no "inf"
    char* end = nullptr;
    const float v = strtof(text, &end);
    if (*end != '\0' || !isfinite(v)) return false;
    out = v;
    return true;
}

int indexOf(const char* text, const char* const* names, int count) {
    for (int i = 0; i < count; ++i) {
        if (strcmp(text, names[i]) == 0) return i;
    }
    return -1;
}

}  // namespace

bool sessionLine(const Session& session, uint8_t index, char* out, size_t capacity) {
    if (capacity < 24) return false;
    size_t used = 0;
    out[0] = '\0';
    if (index == 0) return appendText(out, capacity, used, kHeader, " 1");
    if (index == 1) {
        char name[sizeof(session.name)];
        memcpy(name, session.name, sizeof(name));
        name[sizeof(name) - 1] = '\0';
        return appendText(out, capacity, used, "name=", name);
    }
    if (index == 2) {
        if (session.endFadeS == kFadeDeviceChoice) return appendText(out, capacity, used, "endfade=", "device");
        return append(out, capacity, used, "endfade=%.0f", session.endFadeS);
    }
    const uint8_t n = static_cast<uint8_t>(index - 3);
    if (n >= session.stageCount || n >= kMaxStages) return false;
    const Stage& stage = session.stages[n];
    const uint8_t sweep = static_cast<uint8_t>(stage.sweep);
    bool ok = append(out, capacity, used, "stage d=%.0f", stage.durationS);
    if (stage.transitionS != 0) ok = ok && append(out, capacity, used, " t=%.0f", stage.transitionS);
    ok = ok && appendText(out, capacity, used, " sweep=", sweep < 3 ? kSweepNames[sweep] : "hold");
    if (stage.primaryHz != 0.0f) ok = ok && append(out, capacity, used, " p=%.2f", stage.primaryHz);
    if (stage.beatHz != 0.0f) ok = ok && append(out, capacity, used, " b=%.2f", stage.beatHz);
    if (stage.fromPrimaryHz != 0.0f) ok = ok && append(out, capacity, used, " fp=%.2f", stage.fromPrimaryHz);
    if (stage.fromBeatHz != 0.0f) ok = ok && append(out, capacity, used, " fb=%.2f", stage.fromBeatHz);
    if ((stage.flags & kStageSetsPartials) != 0) {
        ok = ok && append(out, capacity, used, " phi=%.0f", stage.partial[0]);
        ok = ok && append(out, capacity, used, " phi2=%.0f", stage.partial[1]);
    }
    if ((stage.flags & kStageSetsNoise) != 0) {
        ok = ok && appendText(out, capacity, used, " noise=", stage.noiseType < 4 ? kNoiseNames[stage.noiseType] : "off");
        ok = ok && append(out, capacity, used, " nv=%.0f", stage.noiseVolume);
    }
    return ok;
}

void SessionParser::begin() {
    _session = Session{};
    strcpy(_session.name, "Session");
    _header = false;
    _error = "";
}

bool SessionParser::fail(const char* why) {
    if (_error[0] == '\0') _error = why;
    return false;
}

bool SessionParser::line(const char* text) {
    if (_error[0] != '\0') return false;
    char buffer[kSessionLineMax];
    while (*text == ' ' || *text == '\t') ++text;
    size_t length = strlen(text);
    while (length > 0 && (text[length - 1] == '\r' || text[length - 1] == '\n' || text[length - 1] == ' ' ||
                          text[length - 1] == '\t')) {
        --length;
    }
    if (length == 0 || text[0] == '#') return true;
    if (length >= sizeof(buffer)) return fail("line too long");
    memcpy(buffer, text, length);
    buffer[length] = '\0';

    if (!_header) {
        const size_t n = sizeof(kHeader) - 1;
        uint32_t version = 0;
        if (strncmp(buffer, kHeader, n) != 0 || buffer[n] != ' ' || !readCount(buffer + n + 1, 99999, version)) {
            return fail("not a session file");
        }
        if (version != 1) return fail("written by a later version");
        _header = true;
        return true;
    }
    if (strncmp(buffer, "name=", 5) == 0) {
        const char* name = buffer + 5;
        if (*name == '\0') return fail("empty name");
        for (const char* c = name; *c != '\0'; ++c) {
            if (*c < 0x20 || *c > 0x7E) return fail("name has a character that cannot be shown");
        }
        strncpy(_session.name, name, sizeof(_session.name) - 1);
        _session.name[sizeof(_session.name) - 1] = '\0';
        return true;
    }
    if (strncmp(buffer, "endfade=", 8) == 0) {
        uint32_t seconds = 0;
        if (strcmp(buffer + 8, "device") == 0) _session.endFadeS = kFadeDeviceChoice;
        else if (readCount(buffer + 8, kEndFadeMaxS, seconds)) _session.endFadeS = static_cast<uint16_t>(seconds);
        else return fail("bad end fade");
        return true;
    }
    if (strncmp(buffer, "stage", 5) == 0 && (buffer[5] == ' ' || buffer[5] == '\0')) return stageLine(buffer + 5);
    return true;  // a line of a later version
}

bool SessionParser::stageLine(char* text) {
    if (_session.stageCount >= kMaxStages) return fail("too many stages");
    Stage stage = {};
    bool haveDuration = false;
    char* save = nullptr;
    for (char* token = strtok_r(text, " \t", &save); token != nullptr; token = strtok_r(nullptr, " \t", &save)) {
        char* value = strchr(token, '=');
        if (value == nullptr) return fail("stage entry without a value");
        *value++ = '\0';
        uint32_t count = 0;
        if (strcmp(token, "d") == 0) {
            if (!readCount(value, 65535, count)) return fail("bad duration");
            stage.durationS = static_cast<uint16_t>(count);
            haveDuration = true;
        } else if (strcmp(token, "t") == 0) {
            if (!readCount(value, 65535, count)) return fail("bad transition");
            stage.transitionS = static_cast<uint16_t>(count);
        } else if (strcmp(token, "sweep") == 0) {
            const int i = indexOf(value, kSweepNames, 3);
            if (i < 0) return fail("unknown sweep");
            stage.sweep = static_cast<Sweep>(i);
        } else if (strcmp(token, "p") == 0) {
            if (!readHz(value, stage.primaryHz)) return fail("bad primary frequency");
        } else if (strcmp(token, "b") == 0) {
            if (!readHz(value, stage.beatHz)) return fail("bad beat frequency");
        } else if (strcmp(token, "fp") == 0) {
            if (!readHz(value, stage.fromPrimaryHz)) return fail("bad primary start");
        } else if (strcmp(token, "fb") == 0) {
            if (!readHz(value, stage.fromBeatHz)) return fail("bad beat start");
        } else if (strcmp(token, "phi") == 0 || strcmp(token, "phi2") == 0) {
            if (!readCount(value, 100, count)) return fail("bad partial level");
            stage.partial[token[3] == '2' ? 1 : 0] = static_cast<uint8_t>(count);
            stage.flags |= kStageSetsPartials;
        } else if (strcmp(token, "noise") == 0) {
            const int i = indexOf(value, kNoiseNames, 4);
            if (i < 0) return fail("unknown noise");
            stage.noiseType = static_cast<uint8_t>(i);
            stage.flags |= kStageSetsNoise;
        } else if (strcmp(token, "nv") == 0) {
            if (!readCount(value, 100, count)) return fail("bad noise volume");
            stage.noiseVolume = static_cast<uint8_t>(count);
            stage.flags |= kStageSetsNoise;
        }
        // Any other key belongs to a later version.
    }
    if (!haveDuration) return fail("stage without a duration");
    _session.stages[_session.stageCount++] = stage;
    return true;
}

bool SessionParser::finish(Session& out) const {
    if (_error[0] != '\0' || !_header || !isValidSession(_session)) return false;
    out = _session;
    return true;
}

}  // namespace rc
