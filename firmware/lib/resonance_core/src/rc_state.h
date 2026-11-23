// Resonance portable core: the advanced settings that are kept on the card
// (RES-CFG-004) - the layer mixer, the breathing pacer and the Resonance Lab
// with its two sides.
//
// They are kept apart from the settings record of rc_settings.h, which lives
// in the device's own storage: this record is a text file on the card, in
// the manner of sessions (rc_session_text.h), so that it can be read, kept
// and removed with the card. A device without a card, or with a file that is
// not valid, starts with every one of these controls at rest.
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "rc_breath.h"
#include "rc_lab.h"

namespace rc {

constexpr uint8_t kStateLayers = 4;        // rc::Layer order: tone, partials, noise, recording
constexpr uint8_t kMixerFadeChoices = 6;   // the fade times the mixer offers
constexpr int16_t kLabPhaseMaxDegrees = 180;
constexpr size_t kStateLineMax = 160;

// One of the two settings the Lab compares. Frequencies and partial levels
// are taken from the device when the side is left, and put back when it is
// returned to; a primary frequency of 0 marks a side not used yet.
struct LabSide {
    float primaryHz = 0.0f;
    float beatHz = 0.0f;
    uint8_t partial[2] = {0, 0};
    int16_t phaseL = 0;  // degrees, -180..180
    int16_t phaseR = 0;
    bool invertL = false;
    bool invertR = false;
    LabSettings lab;
};

struct AdvancedState {
    // Layer mixer. The noise and recording levels are in the settings record.
    bool layerOn[kStateLayers] = {true, true, true, true};
    uint8_t toneLevel = 100;
    uint8_t partialsLevel = 100;
    uint8_t partial[2] = {0, 0};  // phi, phi squared; percent
    uint8_t fadeChoice = 2;
    // Breathing pacer.
    BreathPattern breath = {40, 0, 60, 0};
    uint8_t breathDepth = 0;
    bool breathOn = false;
    // Resonance Lab.
    LabSide lab[2];
    uint8_t labSlot = 0;
    bool labMatch = false;
};

bool isValidState(const AdvancedState& state);
bool sameState(const AdvancedState& a, const AdvancedState& b);

// Text form: line `index` of the record, false past the last line or when it
// does not fit.
//   RESONANCE STATE 1
//   mixer tone=<on>/<level> gold=<on>/<level> noise=<on> sound=<on> p1=<%> p2=<%> fade=<choice>
//   breath on=<0|1> in=<ds> hold=<ds> out=<ds> rest=<ds> depth=<%>
//   lab side=<a|b> match=<0|1>
//   a hz=<Hz> beat=<Hz> p1= p2= phl= phr= invl= invr= width= delay= period= depth= golden=
//   b ...
bool stateLine(const AdvancedState& state, uint8_t index, char* out, size_t capacity);

// Reads the text form a line at a time. A line or key it does not know is
// left alone (a later version's); a value it cannot read or that is out of
// range refuses the whole text, so that half a record is never used.
class StateParser {
public:
    void begin();
    // Takes one line. Returns false once the text is refused.
    bool line(const char* text);
    // The record, when the text was a valid one.
    bool finish(AdvancedState& out) const;
    const char* error() const { return _error; }

private:
    bool fail(const char* why);
    AdvancedState _state;
    bool _header = false;
    const char* _error = "";
};

}  // namespace rc
