#include "rc_sequence.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rc_settings.h"

namespace rc {

namespace {

const char kHeader[] = "RESONANCE SEQUENCE";

// A whole number of at most five digits with nothing after it.
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

bool fits(int written, size_t capacity) { return written >= 0 && static_cast<size_t>(written) < capacity; }

}  // namespace

bool isValidSequence(const Sequence& s) {
    if (s.stepCount < 1 || s.stepCount > kMaxSequenceSteps) return false;
    if (memchr(s.name, '\0', sizeof(s.name)) == nullptr) return false;
    if (s.glideS > kSequenceGlideMaxS) return false;
    if (!(s.beatHz == 0.0f || (s.beatHz >= kBeatMinHz && s.beatHz <= kBeatMaxHz))) return false;  // NaN fails
    for (uint8_t i = 0; i < s.stepCount; ++i) {
        if (!(s.steps[i].hz >= kPrimaryMinHz && s.steps[i].hz <= kPrimaryMaxHz)) return false;
        if (s.steps[i].seconds == 0) return false;
    }
    return true;
}

Sequence defaultSequence() {
    static const float kSet[] = {174.0f, 285.0f, 396.0f, 417.0f, 528.0f, 639.0f, 741.0f, 852.0f, 963.0f};
    Sequence s = {};
    strcpy(s.name, "Solfeggio");
    s.stepCount = sizeof(kSet) / sizeof(kSet[0]);
    for (uint8_t i = 0; i < s.stepCount; ++i) s.steps[i] = {kSet[i], 120};
    s.glideS = 0;
    s.beatHz = 0.0f;
    return s;
}

Session sequenceSession(const Sequence& sequence) {
    Session session = {};
    if (!isValidSequence(sequence)) return session;
    memcpy(session.name, sequence.name, sizeof(session.name));
    session.stageCount = sequence.stepCount;
    for (uint8_t i = 0; i < sequence.stepCount; ++i) {
        Stage& stage = session.stages[i];
        stage.durationS = sequence.steps[i].seconds;
        stage.primaryHz = sequence.steps[i].hz;
        stage.beatHz = sequence.beatHz;
        // The first step has nothing to glide from.
        if (i == 0 || sequence.glideS == 0) {
            stage.sweep = Sweep::Hold;
        } else {
            stage.sweep = Sweep::Linear;
            // A glide as long as the step, or longer, is the whole step.
            stage.transitionS = sequence.glideS < stage.durationS ? sequence.glideS : 0;
        }
    }
    return session;
}

bool sequenceLine(const Sequence& s, uint8_t index, char* out, size_t capacity) {
    if (capacity < 24) return false;
    if (index == 0) return fits(snprintf(out, capacity, "%s 1", kHeader), capacity);
    if (index == 1) {
        char name[sizeof(s.name)];
        memcpy(name, s.name, sizeof(name));
        name[sizeof(name) - 1] = '\0';
        return fits(snprintf(out, capacity, "name=%s", name), capacity);
    }
    if (index == 2) return fits(snprintf(out, capacity, "glide=%u", static_cast<unsigned>(s.glideS)), capacity);
    if (index == 3) return fits(snprintf(out, capacity, "beat=%.2f", static_cast<double>(s.beatHz)), capacity);
    const uint8_t n = static_cast<uint8_t>(index - 4);
    if (n >= s.stepCount || n >= kMaxSequenceSteps) return false;
    return fits(snprintf(out, capacity, "step hz=%.2f d=%u", static_cast<double>(s.steps[n].hz),
                         static_cast<unsigned>(s.steps[n].seconds)),
                capacity);
}

void SequenceParser::begin() {
    _sequence = Sequence{};
    strcpy(_sequence.name, "Sequence");
    _header = false;
    _error = "";
}

bool SequenceParser::fail(const char* why) {
    if (_error[0] == '\0') _error = why;
    return false;
}

bool SequenceParser::line(const char* text) {
    if (_error[0] != '\0') return false;
    char buffer[kSequenceLineMax];
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

    uint32_t count = 0;
    if (!_header) {
        const size_t n = sizeof(kHeader) - 1;
        if (strncmp(buffer, kHeader, n) != 0 || buffer[n] != ' ' || !readCount(buffer + n + 1, 99999, count)) {
            return fail("not a sequence file");
        }
        if (count != 1) return fail("written by a later version");
        _header = true;
        return true;
    }
    if (strncmp(buffer, "name=", 5) == 0) {
        const char* name = buffer + 5;
        if (*name == '\0') return fail("empty name");
        for (const char* c = name; *c != '\0'; ++c) {
            if (*c < 0x20 || *c > 0x7E) return fail("name has a character that cannot be shown");
        }
        strncpy(_sequence.name, name, sizeof(_sequence.name) - 1);
        _sequence.name[sizeof(_sequence.name) - 1] = '\0';
        return true;
    }
    if (strncmp(buffer, "glide=", 6) == 0) {
        if (!readCount(buffer + 6, kSequenceGlideMaxS, count)) return fail("bad glide");
        _sequence.glideS = static_cast<uint16_t>(count);
        return true;
    }
    if (strncmp(buffer, "beat=", 5) == 0) {
        if (!readHz(buffer + 5, _sequence.beatHz)) return fail("bad beat frequency");
        return true;
    }
    if (strncmp(buffer, "step", 4) == 0 && (buffer[4] == ' ' || buffer[4] == '\0')) {
        if (_sequence.stepCount >= kMaxSequenceSteps) return fail("too many steps");
        SequenceStep step = {0.0f, 0};
        char* save = nullptr;
        for (char* token = strtok_r(buffer + 4, " \t", &save); token != nullptr; token = strtok_r(nullptr, " \t", &save)) {
            char* value = strchr(token, '=');
            if (value == nullptr) return fail("step entry without a value");
            *value++ = '\0';
            if (strcmp(token, "hz") == 0) {
                if (!readHz(value, step.hz)) return fail("bad frequency");
            } else if (strcmp(token, "d") == 0) {
                if (!readCount(value, 65535, count)) return fail("bad duration");
                step.seconds = static_cast<uint16_t>(count);
            }
            // Any other key belongs to a later version.
        }
        _sequence.steps[_sequence.stepCount++] = step;
        return true;
    }
    return true;  // a line of a later version
}

bool SequenceParser::finish(Sequence& out) const {
    if (_error[0] != '\0' || !_header || !isValidSequence(_sequence)) return false;
    out = _sequence;
    return true;
}

}  // namespace rc
