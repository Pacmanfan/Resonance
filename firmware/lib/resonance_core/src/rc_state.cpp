#include "rc_state.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rc_audio.h"
#include "rc_settings.h"

namespace rc {

static_assert(kStateLayers == kLayerCount, "the record keeps one switch for each mixer layer");

namespace {

const char kHeader[] = "RESONANCE STATE";

bool fits(int written, size_t capacity) { return written >= 0 && static_cast<size_t>(written) < capacity; }

// A whole number, with an optional minus sign, of at most five digits and
// with nothing after it, within low..high.
bool readInt(const char* text, long low, long high, long& out) {
    const char* digits = *text == '-' ? text + 1 : text;
    if (*digits < '0' || *digits > '9') return false;
    char* end = nullptr;
    const long v = strtol(text, &end, 10);
    if (*end != '\0' || end - digits > 5 || v < low || v > high) return false;
    out = v;
    return true;
}

bool readFlag(const char* text, bool& out) {
    long v = 0;
    if (!readInt(text, 0, 1, v)) return false;
    out = v != 0;
    return true;
}

template <typename T>
bool readNumber(const char* text, long low, long high, T& out) {
    long v = 0;
    if (!readInt(text, low, high, v)) return false;
    out = static_cast<T>(v);
    return true;
}

// A frequency: 0 (not set) or within low..high.
bool readHz(const char* text, float low, float high, float& out) {
    if ((*text < '0' || *text > '9') && *text != '.') return false;  // no sign, no "nan", no "inf"
    char* end = nullptr;
    const float v = strtof(text, &end);
    if (*end != '\0' || !isfinite(v)) return false;
    if (v != 0.0f && (v < low || v > high)) return false;
    out = v;
    return true;
}

// "<on>/<level>": a layer's switch and its level in percent.
bool readLayer(char* text, bool& on, uint8_t& level) {
    char* slash = strchr(text, '/');
    if (slash == nullptr) return false;
    *slash = '\0';
    return readFlag(text, on) && readNumber(slash + 1, 0, 100, level);
}

bool validHz(float hz, float low, float high) { return hz == 0.0f || (hz >= low && hz <= high); }  // NaN fails

bool validSide(const LabSide& side) {
    LabSettings lab = side.lab;
    return validHz(side.primaryHz, kPrimaryMinHz, kPrimaryMaxHz) && validHz(side.beatHz, kBeatMinHz, kBeatMaxHz) &&
           side.partial[0] <= 100 && side.partial[1] <= 100 && side.phaseL >= -kLabPhaseMaxDegrees &&
           side.phaseL <= kLabPhaseMaxDegrees && side.phaseR >= -kLabPhaseMaxDegrees &&
           side.phaseR <= kLabPhaseMaxDegrees && !sanitizeLab(lab);
}

bool sameSide(const LabSide& a, const LabSide& b) {
    return a.primaryHz == b.primaryHz && a.beatHz == b.beatHz && a.partial[0] == b.partial[0] &&
           a.partial[1] == b.partial[1] && a.phaseL == b.phaseL && a.phaseR == b.phaseR && a.invertL == b.invertL &&
           a.invertR == b.invertR && a.lab.widthPercent == b.lab.widthPercent && a.lab.delayUs == b.lab.delayUs &&
           a.lab.bilateralPeriodDs == b.lab.bilateralPeriodDs && a.lab.bilateralDepth == b.lab.bilateralDepth &&
           a.lab.bilateralGolden == b.lab.bilateralGolden;
}

}  // namespace

bool isValidState(const AdvancedState& s) {
    return s.toneLevel <= 100 && s.partialsLevel <= 100 && s.partial[0] <= 100 && s.partial[1] <= 100 &&
           s.fadeChoice < kMixerFadeChoices && isValidBreath(s.breath) && s.breathDepth <= kBreathDepthMax &&
           s.labSlot < 2 && validSide(s.lab[0]) && validSide(s.lab[1]);
}

bool sameState(const AdvancedState& a, const AdvancedState& b) {
    for (uint8_t i = 0; i < kStateLayers; ++i) {
        if (a.layerOn[i] != b.layerOn[i]) return false;
    }
    return a.toneLevel == b.toneLevel && a.partialsLevel == b.partialsLevel && a.partial[0] == b.partial[0] &&
           a.partial[1] == b.partial[1] && a.fadeChoice == b.fadeChoice && a.breath.inhaleDs == b.breath.inhaleDs &&
           a.breath.holdInDs == b.breath.holdInDs && a.breath.exhaleDs == b.breath.exhaleDs &&
           a.breath.holdOutDs == b.breath.holdOutDs && a.breathDepth == b.breathDepth && a.breathOn == b.breathOn &&
           sameSide(a.lab[0], b.lab[0]) && sameSide(a.lab[1], b.lab[1]) && a.labSlot == b.labSlot &&
           a.labMatch == b.labMatch;
}

bool stateLine(const AdvancedState& s, uint8_t index, char* out, size_t capacity) {
    if (capacity < 24) return false;
    switch (index) {
        case 0:
            return fits(snprintf(out, capacity, "%s 1", kHeader), capacity);
        case 1:
            return fits(snprintf(out, capacity, "mixer tone=%d/%u gold=%d/%u noise=%d sound=%d p1=%u p2=%u fade=%u",
                                 s.layerOn[0], static_cast<unsigned>(s.toneLevel), s.layerOn[1],
                                 static_cast<unsigned>(s.partialsLevel), s.layerOn[2], s.layerOn[3],
                                 static_cast<unsigned>(s.partial[0]), static_cast<unsigned>(s.partial[1]),
                                 static_cast<unsigned>(s.fadeChoice)),
                        capacity);
        case 2:
            return fits(snprintf(out, capacity, "breath on=%d in=%u hold=%u out=%u rest=%u depth=%u", s.breathOn,
                                 static_cast<unsigned>(s.breath.inhaleDs), static_cast<unsigned>(s.breath.holdInDs),
                                 static_cast<unsigned>(s.breath.exhaleDs), static_cast<unsigned>(s.breath.holdOutDs),
                                 static_cast<unsigned>(s.breathDepth)),
                        capacity);
        case 3:
            return fits(snprintf(out, capacity, "lab side=%c match=%d", s.labSlot == 1 ? 'b' : 'a', s.labMatch), capacity);
        case 4:
        case 5: {
            const LabSide& side = s.lab[index - 4];
            return fits(snprintf(out, capacity,
                                 "%c hz=%.2f beat=%.2f p1=%u p2=%u phl=%d phr=%d invl=%d invr=%d width=%u delay=%d "
                                 "period=%u depth=%u golden=%d",
                                 index == 4 ? 'a' : 'b', static_cast<double>(side.primaryHz),
                                 static_cast<double>(side.beatHz), static_cast<unsigned>(side.partial[0]),
                                 static_cast<unsigned>(side.partial[1]), static_cast<int>(side.phaseL),
                                 static_cast<int>(side.phaseR), side.invertL, side.invertR,
                                 static_cast<unsigned>(side.lab.widthPercent), static_cast<int>(side.lab.delayUs),
                                 static_cast<unsigned>(side.lab.bilateralPeriodDs),
                                 static_cast<unsigned>(side.lab.bilateralDepth), side.lab.bilateralGolden),
                        capacity);
        }
        default:
            return false;
    }
}

void StateParser::begin() {
    _state = AdvancedState{};
    _header = false;
    _error = "";
}

bool StateParser::fail(const char* why) {
    if (_error[0] == '\0') _error = why;
    return false;
}

bool StateParser::line(const char* text) {
    if (_error[0] != '\0') return false;
    char buffer[kStateLineMax];
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
        long version = 0;
        if (strncmp(buffer, kHeader, n) != 0 || buffer[n] != ' ' || !readInt(buffer + n + 1, 0, 99999, version)) {
            return fail("not a settings file");
        }
        if (version != 1) return fail("written by a later version");
        _header = true;
        return true;
    }

    char* save = nullptr;
    char* kind = strtok_r(buffer, " \t", &save);
    if (kind == nullptr) return true;
    enum { Mixer, Breath, Lab, SideA, SideB } what;
    if (strcmp(kind, "mixer") == 0) what = Mixer;
    else if (strcmp(kind, "breath") == 0) what = Breath;
    else if (strcmp(kind, "lab") == 0) what = Lab;
    else if (strcmp(kind, "a") == 0) what = SideA;
    else if (strcmp(kind, "b") == 0) what = SideB;
    else return true;  // a line of a later version

    AdvancedState& s = _state;
    for (char* token = strtok_r(nullptr, " \t", &save); token != nullptr; token = strtok_r(nullptr, " \t", &save)) {
        char* value = strchr(token, '=');
        if (value == nullptr) return fail("entry without a value");
        *value++ = '\0';
        bool ok = true;
        if (what == Mixer) {
            if (strcmp(token, "tone") == 0) ok = readLayer(value, s.layerOn[0], s.toneLevel);
            else if (strcmp(token, "gold") == 0) ok = readLayer(value, s.layerOn[1], s.partialsLevel);
            else if (strcmp(token, "noise") == 0) ok = readFlag(value, s.layerOn[2]);
            else if (strcmp(token, "sound") == 0) ok = readFlag(value, s.layerOn[3]);
            else if (strcmp(token, "p1") == 0) ok = readNumber(value, 0, 100, s.partial[0]);
            else if (strcmp(token, "p2") == 0) ok = readNumber(value, 0, 100, s.partial[1]);
            else if (strcmp(token, "fade") == 0) ok = readNumber(value, 0, kMixerFadeChoices - 1, s.fadeChoice);
            if (!ok) return fail("bad mixer value");
        } else if (what == Breath) {
            if (strcmp(token, "on") == 0) ok = readFlag(value, s.breathOn);
            else if (strcmp(token, "in") == 0) ok = readNumber(value, 0, 255, s.breath.inhaleDs);
            else if (strcmp(token, "hold") == 0) ok = readNumber(value, 0, 255, s.breath.holdInDs);
            else if (strcmp(token, "out") == 0) ok = readNumber(value, 0, 255, s.breath.exhaleDs);
            else if (strcmp(token, "rest") == 0) ok = readNumber(value, 0, 255, s.breath.holdOutDs);
            else if (strcmp(token, "depth") == 0) ok = readNumber(value, 0, kBreathDepthMax, s.breathDepth);
            if (!ok) return fail("bad breathing value");
        } else if (what == Lab) {
            if (strcmp(token, "side") == 0) {
                ok = (value[0] == 'a' || value[0] == 'b') && value[1] == '\0';
                if (ok) s.labSlot = value[0] == 'b' ? 1 : 0;
            } else if (strcmp(token, "match") == 0) {
                ok = readFlag(value, s.labMatch);
            }
            if (!ok) return fail("bad lab value");
        } else {
            LabSide& side = s.lab[what == SideB ? 1 : 0];
            if (strcmp(token, "hz") == 0) ok = readHz(value, kPrimaryMinHz, kPrimaryMaxHz, side.primaryHz);
            else if (strcmp(token, "beat") == 0) ok = readHz(value, kBeatMinHz, kBeatMaxHz, side.beatHz);
            else if (strcmp(token, "p1") == 0) ok = readNumber(value, 0, 100, side.partial[0]);
            else if (strcmp(token, "p2") == 0) ok = readNumber(value, 0, 100, side.partial[1]);
            else if (strcmp(token, "phl") == 0) ok = readNumber(value, -kLabPhaseMaxDegrees, kLabPhaseMaxDegrees, side.phaseL);
            else if (strcmp(token, "phr") == 0) ok = readNumber(value, -kLabPhaseMaxDegrees, kLabPhaseMaxDegrees, side.phaseR);
            else if (strcmp(token, "invl") == 0) ok = readFlag(value, side.invertL);
            else if (strcmp(token, "invr") == 0) ok = readFlag(value, side.invertR);
            else if (strcmp(token, "width") == 0) ok = readNumber(value, 0, 100, side.lab.widthPercent);
            else if (strcmp(token, "delay") == 0) ok = readNumber(value, -kLabDelayMaxUs, kLabDelayMaxUs, side.lab.delayUs);
            else if (strcmp(token, "period") == 0) ok = readNumber(value, 0, kBilateralMaxDs, side.lab.bilateralPeriodDs);
            else if (strcmp(token, "depth") == 0) ok = readNumber(value, 0, 100, side.lab.bilateralDepth);
            else if (strcmp(token, "golden") == 0) ok = readFlag(value, side.lab.bilateralGolden);
            if (!ok) return fail("bad lab value");
        }
        // Any other key belongs to a later version.
    }
    return true;
}

bool StateParser::finish(AdvancedState& out) const {
    if (_error[0] != '\0' || !_header || !isValidState(_state)) return false;
    out = _state;
    return true;
}

}  // namespace rc
