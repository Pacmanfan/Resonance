// The advanced screens (RES-GUI-006): layer mixer, breathing pacer, session
// builder with its stage editor and slot list, and frequency sequence. They
// are opened over the SET or SESS tab from one entry, use the same buttons
// and helpers as the tabs, and change the Advanced record through the edit
// functions of rc_edit.h, which cannot produce an invalid session.
#include <Arduino.h>

#include "rc_audio.h"
#include "rc_noise.h"
#include "theme.h"
#include "ui.h"

namespace gui {

namespace {

using namespace theme;

const lgfx::IFont* const kFontSmall = &fonts::DejaVu12;
const lgfx::IFont* const kFontText = &fonts::DejaVu18;

// Button ids. 100..149 repeat while held (Ui::repeats); 150.. do not.
constexpr uint8_t kAdjust = 100;  // adjusters of a screen, in the order of its rows: minus, plus
constexpr uint8_t kAction = 150;  // the other buttons of a screen, in the order they are added

constexpr int16_t kTop = kContentTop;
constexpr int16_t kMinusX = 88, kPlusX = 194, kStepW = 38, kValueCx = 160, kValueW = 66;

const char* const kNoiseNames[] = {"off", "white", "pink", "brown"};
const char* const kSweepNames[] = {"Hold", "Straight", "Exponential"};

void seconds(char* out, size_t capacity, uint16_t s) {
    snprintf(out, capacity, "%u:%02u", static_cast<unsigned>(s / 60), static_cast<unsigned>(s % 60));
}

void tenths(char* out, size_t capacity, uint8_t ds) {
    snprintf(out, capacity, "%u.%u s", static_cast<unsigned>(ds / 10), static_cast<unsigned>(ds % 10));
}

uint8_t stepped(uint8_t value, int delta, uint8_t lo, uint8_t hi) {
    const int next = static_cast<int>(value) + delta;
    return static_cast<uint8_t>(next < lo ? lo : (next > hi ? hi : next));
}

}  // namespace

// ---- navigation ------------------------------------------------------------

void Ui::openSub(Sub sub) {
    _helpOpen = false;
    if (sub == Sub::MindResult && _sub != Sub::MindResult) _resultPage = 0;
    _sub = sub;
    _slotArmed = 0xFF;
    if (_adv.stageIndex >= _adv.session.stageCount) _adv.stageIndex = 0;
    if (_step >= _adv.sequence.stepCount) _step = 0;
    if (sub != Sub::Builder && sub != Sub::Sequence) _adv.message = "";
    if (sub == Sub::MindSource || sub == Sub::MindSetup) _adv.mind.message = "";
    drawHeader();
    drawTabBar();
    drawContent();
}

// One level back: the stage editor and the session slots to the builder,
// the sequence slots to the sequence, the rest to the menu, the menu to the
// tab it was opened from.
void Ui::backSub() {
    // Mindfield: every screen back to its home, the home to the tab. Leaving
    // the active screen does not stop the experiment.
    if (_sub == Sub::Mind) openSub(Sub::None);
    else if (mindSub()) openSub(Sub::Mind);
    else if (_sub == Sub::Stage) openSub(Sub::Builder);
    else if (_sub == Sub::Slots) openSub(_slotKind == 0 ? Sub::Builder : Sub::Sequence);
    else if (_sub == Sub::Menu || (_sub == Sub::Builder && _tab == Tab::Sessions)) openSub(Sub::None);
    else openSub(Sub::Menu);
}

const char* Ui::subTitle() const {
    switch (_sub) {
        case Sub::Mixer: return "LAYER MIXER";
        case Sub::Breath: return "BREATHING";
        case Sub::Builder: return "SESSION BUILDER";
        case Sub::Stage: return "STAGE";
        case Sub::Slots: return _slotSave ? "SAVE TO" : "LOAD FROM";
        case Sub::Sequence: return "SEQUENCE";
        case Sub::Lab: return "RESONANCE LAB";
        case Sub::Mind: return "MINDFIELD";
        case Sub::MindSource: return "RNG SOURCE";
        case Sub::MindSetup: return "EXPERIMENT";
        case Sub::MindRun: return _adv.mind.replaying ? "REPLAY" : rc::mindVizName(_adv.mind.shownViz);
        case Sub::MindResult: return "RESULT";
        case Sub::MindHistory: return "HISTORY";
        default: return "ADVANCED";
    }
}

void Ui::refreshAdvanced() {
    if (_sub != Sub::None && !_dimmed && !_helpOpen) drawContent();
}

void Ui::drawAdvanced() {
    if (mindSub()) {
        drawMind();
        return;
    }
    switch (_sub) {
        case Sub::Menu: drawMenu(); break;
        case Sub::Mixer: drawMixer(); break;
        case Sub::Breath: drawBreath(); break;
        case Sub::Builder: drawBuilder(); break;
        case Sub::Stage: drawStage(); break;
        case Sub::Slots: drawSlots(); break;
        case Sub::Sequence: drawSequence(); break;
        case Sub::Lab: drawLab(); break;
        default: break;
    }
}

// Renews the values of the open screen and leaves its buttons alone, so that
// a held adjuster does not make the screen flicker.
void Ui::redrawValues() {
    _valuesOnly = true;
    const uint8_t count = _buttonCount;
    drawAdvanced();
    _buttonCount = count;
    _valuesOnly = false;
}

// A value between the two adjusters of a row, or wherever a screen puts it.
void Ui::valueAt(int16_t cx, int16_t y, int16_t w, const char* value) {
    _lcd.fillRect(cx - w / 2, y, w, 28, kBackground);
    _lcd.setFont(kFontText);
    centered(value, cx, y + 14, kText, kBackground, _lcd.textWidth(value) > w ? kFontSmall : kFontText);
}

// One row: a name, a minus button, the value, a plus button.
void Ui::row(int16_t y, const char* name, const char* value, uint8_t idMinus, uint8_t idPlus) {
    if (!_valuesOnly) {
        if (name != nullptr) label(name, 8, y + 8, kTextDim, kFontSmall);
        addButton(kMinusX, y, kStepW, 28, "-", idMinus);
        addButton(kPlusX, y, kStepW, 28, "+", idPlus);
    }
    valueAt(kValueCx, y, kValueW, value);
}

// ---- screens ---------------------------------------------------------------

void Ui::drawMenu() {
    static const char* const kEntries[] = {"LAYER MIXER", "BREATHING", "SESSION BUILDER", "SEQUENCE", "RESONANCE LAB"};
    for (uint8_t i = 0; i < 5; ++i) {
        addButton(8, static_cast<int16_t>(kTop + 8 + i * 46), 224, 40, kEntries[i], static_cast<uint8_t>(kAction + i));
    }
}

void Ui::drawMixer() {
    static const char* const kNames[] = {"TONE", "GOLD", "NOISE", "SOUND"};
    char text[16];
    const uint8_t levels[4] = {_adv.toneLevel, _adv.partialsLevel, _status.noiseVolume, _status.backgroundVolume};
    for (uint8_t i = 0; i < 4; ++i) {
        const int16_t y = static_cast<int16_t>(kTop + 2 + i * 34);
        // The name is the switch: lit while the layer is on.
        snprintf(_advLabel[i], sizeof(_advLabel[i]), "%s %s", kNames[i], _adv.layerOn[i] ? "on" : "off");
        if (!_valuesOnly) addButton(4, y, 80, 28, _advLabel[i], static_cast<uint8_t>(kAction + i));
        snprintf(text, sizeof(text), "%u%%", static_cast<unsigned>(levels[i]));
        row(y, nullptr, text, static_cast<uint8_t>(kAdjust + 2 * i), static_cast<uint8_t>(kAdjust + 2 * i + 1));
    }
    for (uint8_t p = 0; p < 2; ++p) {
        if (_adv.partial[p] == 0) snprintf(text, sizeof(text), "off");
        else snprintf(text, sizeof(text), "%u%%", static_cast<unsigned>(_adv.partial[p]));
        row(static_cast<int16_t>(kTop + 138 + p * 34), p == 0 ? "Golden 1" : "Golden 2", text,
            static_cast<uint8_t>(kAdjust + 8 + 2 * p), static_cast<uint8_t>(kAdjust + 9 + 2 * p));
    }
    const float fade = kFadeChoices[_adv.fadeChoice < kFadeChoiceCount ? _adv.fadeChoice : 0];
    if (fade < 0.1f) snprintf(_advLabel[4], sizeof(_advLabel[4]), "quick");
    else snprintf(_advLabel[4], sizeof(_advLabel[4]), "%.1f s", static_cast<double>(fade));
    if (!_valuesOnly) {
        label("Fade", 8, kTop + 214, kTextDim, kFontSmall);
        addButton(kMinusX, kTop + 206, 144, 28, _advLabel[4], kAction + 4);
    }
}

void Ui::drawBreath() {
    static const char* const kNames[] = {"Inhale", "Hold", "Exhale", "Hold"};
    const uint8_t parts[4] = {_adv.breath.inhaleDs, _adv.breath.holdInDs, _adv.breath.exhaleDs, _adv.breath.holdOutDs};
    char text[16];
    for (uint8_t i = 0; i < 4; ++i) {
        if (parts[i] == 0) snprintf(text, sizeof(text), "none");
        else tenths(text, sizeof(text), parts[i]);
        row(static_cast<int16_t>(kTop + 2 + i * 34), kNames[i], text, static_cast<uint8_t>(kAdjust + 2 * i),
            static_cast<uint8_t>(kAdjust + 2 * i + 1));
    }
    // The depth of the audio modulation; without it the pacer is seen only.
    if (_adv.breathDepth == 0) snprintf(text, sizeof(text), "none");
    else snprintf(text, sizeof(text), "%u%%", static_cast<unsigned>(_adv.breathDepth));
    row(kTop + 138, "In sound", text, kAdjust + 8, kAdjust + 9);
    snprintf(_advLabel[0], sizeof(_advLabel[0]), "%s", _adv.breathOn ? "STOP" : "START");
    if (!_valuesOnly) {
        addButton(8, kTop + 176, 224, 36, _advLabel[0], kAction);
        _breathBar = 0;
        _lcd.drawRect(19, kTop + 221, 202, 10, kOutline);
    }
}

void Ui::drawBuilder() {
    char text[40];
    const rc::Session& s = _adv.session;
    const rc::Stage& stage = s.stages[_adv.stageIndex];
    if (!_valuesOnly) {
        addButton(8, kTop + 2, 40, 30, "<", kAdjust);
        addButton(192, kTop + 2, 40, 30, ">", kAdjust + 1);
        addButton(8, kTop + 90, 72, 30, "EDIT", kAction);
        addButton(84, kTop + 90, 72, 30, "ADD", kAction + 1);
        addButton(160, kTop + 90, 72, 30, "REMOVE", kAction + 2);
        addButton(8, kTop + 124, 110, 30, "EARLIER", kAction + 3);
        addButton(122, kTop + 124, 110, 30, "LATER", kAction + 4);
        addButton(8, kTop + 158, 72, 30, "NEW", kAction + 5);
        addButton(84, kTop + 158, 72, 30, "LOAD", kAction + 6);
        addButton(160, kTop + 158, 72, 30, "SAVE", kAction + 7);
        addButton(8, kTop + 194, 224, 40, "START", kAction + 8);
    }
    snprintf(text, sizeof(text), "Stage %u of %u", static_cast<unsigned>(_adv.stageIndex + 1), static_cast<unsigned>(s.stageCount));
    valueAt(120, kTop + 3, 140, text);
    _lcd.fillRect(0, kTop + 36, kScreenW, 52, kBackground);
    rc::describeStage(stage, text, sizeof(text));
    centered(text, kScreenW / 2, kTop + 46, kText, kBackground, kFontSmall);
    // What else the stage sets, or the outcome of the last store or load.
    if (_adv.message[0] != '\0') {
        centered(_adv.message, kScreenW / 2, kTop + 64, kWarning, kBackground, kFontSmall);
    } else {
        snprintf(text, sizeof(text), "%s%s%s%s", (stage.flags & rc::kStageSetsPartials) ? "golden " : "",
                 (stage.flags & rc::kStageSetsNoise) ? "noise " : "", (stage.flags & rc::kStageSetsBreath) ? "breath " : "",
                 (stage.flags & rc::kStageSetsVoice) ? "sound" : "");
        centered(text[0] ? text : "frequencies only", kScreenW / 2, kTop + 64, kTeal, kBackground, kFontSmall);
    }
    const uint32_t total = rc::sessionSeconds(s);
    snprintf(text, sizeof(text), "%s  %lu:%02lu", s.name, static_cast<unsigned long>(total / 60),
             static_cast<unsigned long>(total % 60));
    centered(text, kScreenW / 2, kTop + 80, kTextDim, kBackground, kFontSmall);
}

void Ui::drawStage() {
    char text[20];
    rc::Stage& stage = _adv.session.stages[_adv.stageIndex];
    seconds(text, sizeof(text), stage.durationS);
    row(kTop + 2, "Length", text, kAdjust, kAdjust + 1);
    // How long the move to the stage's values takes; the rest is held.
    if (stage.sweep == rc::Sweep::Hold) snprintf(text, sizeof(text), "-");
    else if (stage.transitionS == 0) snprintf(text, sizeof(text), "all");
    else seconds(text, sizeof(text), stage.transitionS);
    row(kTop + 32, "Move in", text, kAdjust + 2, kAdjust + 3);
    const uint8_t sweep = static_cast<uint8_t>(stage.sweep);
    snprintf(_advLabel[0], sizeof(_advLabel[0]), "%s", kSweepNames[sweep < 3 ? sweep : 0]);
    if (!_valuesOnly) {
        label("Shape", 8, kTop + 70, kTextDim, kFontSmall);
        addButton(kMinusX, kTop + 62, 144, 28, _advLabel[0], kAction);
    }
    if (stage.primaryHz == 0.0f) snprintf(text, sizeof(text), "as is");
    else snprintf(text, sizeof(text), "%.0f Hz", static_cast<double>(stage.primaryHz));
    row(kTop + 92, "Primary", text, kAdjust + 4, kAdjust + 5);
    if (stage.beatHz == 0.0f) snprintf(text, sizeof(text), "as is");
    else snprintf(text, sizeof(text), "%.1f Hz", static_cast<double>(stage.beatHz));
    row(kTop + 122, "Beat", text, kAdjust + 6, kAdjust + 7);

    // The layers: each button takes what is set on the device now, or lets go of it.
    if (stage.flags & rc::kStageSetsPartials) {
        snprintf(_advLabel[1], sizeof(_advLabel[1]), "Golden %u/%u", static_cast<unsigned>(stage.partial[0]),
                 static_cast<unsigned>(stage.partial[1]));
    } else {
        snprintf(_advLabel[1], sizeof(_advLabel[1]), "Golden: as is");
    }
    if (stage.flags & rc::kStageSetsNoise) {
        if (stage.noiseType == 0) snprintf(_advLabel[2], sizeof(_advLabel[2]), "Noise off");
        else snprintf(_advLabel[2], sizeof(_advLabel[2]), "%s %u%%", kNoiseNames[stage.noiseType < 4 ? stage.noiseType : 0],
                      static_cast<unsigned>(stage.noiseVolume));
    } else {
        snprintf(_advLabel[2], sizeof(_advLabel[2]), "Noise: as is");
    }
    if (stage.flags & rc::kStageSetsBreath) {
        if (stage.breath.inhaleDs == 0) snprintf(_advLabel[3], sizeof(_advLabel[3]), "Breath off");
        else snprintf(_advLabel[3], sizeof(_advLabel[3]), "Breath %u-%u-%u-%u", stage.breath.inhaleDs / 10u,
                      stage.breath.holdInDs / 10u, stage.breath.exhaleDs / 10u, stage.breath.holdOutDs / 10u);
    } else {
        snprintf(_advLabel[3], sizeof(_advLabel[3]), "Breath: as is");
    }
    if (stage.flags & rc::kStageSetsVoice) {
        const char* slash = strrchr(stage.voice, '/');
        if (stage.voice[0] == '\0') snprintf(_advLabel[4], sizeof(_advLabel[4]), "Sound stops");
        else snprintf(_advLabel[4], sizeof(_advLabel[4]), "%.14s", slash != nullptr ? slash + 1 : stage.voice);
    } else {
        snprintf(_advLabel[4], sizeof(_advLabel[4]), "Sound: as is");
    }
    if (_valuesOnly) {
        for (uint8_t i = 0; i < 5; ++i) redrawButton(static_cast<uint8_t>(kAction + i));
        return;
    }
    addButton(8, kTop + 156, 110, 28, _advLabel[1], kAction + 1);
    addButton(122, kTop + 156, 110, 28, _advLabel[2], kAction + 2);
    addButton(8, kTop + 188, 110, 28, _advLabel[3], kAction + 3);
    addButton(122, kTop + 188, 110, 28, _advLabel[4], kAction + 4);
    label("Tap to take what is set now.", 12, kTop + 222, kTextDim, kFontSmall);
}

void Ui::drawSlots() {
    if (!_adv.slotsReady) {
        label("Reading the card...", 12, kTop + 12, kTextDim, kFontSmall);
        return;
    }
    const uint8_t count = _slotKind == 0 ? kSlotListMax : 8;
    for (uint8_t i = 0; i < count; ++i) {
        if (i == _slotArmed) snprintf(_slotLabel[i], sizeof(_slotLabel[i]), "Replace?");
        else if (_adv.slotUsed[i]) snprintf(_slotLabel[i], sizeof(_slotLabel[i]), "%u %.11s", static_cast<unsigned>(i + 1), _adv.slotName[i]);
        else snprintf(_slotLabel[i], sizeof(_slotLabel[i]), "%u  -", static_cast<unsigned>(i + 1));
        addButton(static_cast<int16_t>(8 + (i % 2) * 114), static_cast<int16_t>(kTop + 4 + (i / 2) * 36), 110, 32, _slotLabel[i],
                  static_cast<uint8_t>(kAction + i));
    }
    label(_adv.message[0] ? _adv.message : (_slotSave ? "Tap a slot to store there." : "Tap a slot to load it."), 12, kTop + 222,
          _adv.message[0] ? kWarning : kTextDim, kFontSmall);
}

void Ui::drawSequence() {
    char text[32];
    rc::Sequence& q = _adv.sequence;
    if (!_valuesOnly) {
        addButton(8, kTop + 2, 40, 28, "<", kAdjust);
        addButton(192, kTop + 2, 40, 28, ">", kAdjust + 1);
        addButton(4, kTop + 34, 40, 28, "-10", kAdjust + 2);
        addButton(46, kTop + 34, 36, 28, "-1", kAdjust + 3);
        addButton(158, kTop + 34, 36, 28, "+1", kAdjust + 4);
        addButton(196, kTop + 34, 40, 28, "+10", kAdjust + 5);
        addButton(8, kTop + 132, 110, 28, "ADD", kAction);
        addButton(122, kTop + 132, 110, 28, "REMOVE", kAction + 1);
        addButton(8, kTop + 164, 72, 28, "RESET", kAction + 2);
        addButton(84, kTop + 164, 72, 28, "LOAD", kAction + 3);
        addButton(160, kTop + 164, 72, 28, "SAVE", kAction + 4);
        addButton(8, kTop + 198, 224, 36, "USE AS SESSION", kAction + 5);
    }
    snprintf(text, sizeof(text), "Step %u of %u", static_cast<unsigned>(_step + 1), static_cast<unsigned>(q.stepCount));
    valueAt(120, kTop + 2, 140, text);
    snprintf(text, sizeof(text), "%.0f Hz", static_cast<double>(q.steps[_step].hz));
    valueAt(120, kTop + 34, 72, text);
    seconds(text, sizeof(text), q.steps[_step].seconds);
    row(kTop + 66, "Length", text, kAdjust + 6, kAdjust + 7);
    if (q.glideS == 0) snprintf(text, sizeof(text), "none");
    else snprintf(text, sizeof(text), "%u s", static_cast<unsigned>(q.glideS));
    row(kTop + 98, "Glide", text, kAdjust + 8, kAdjust + 9);
}

// One screen: the two sides and their matching on top, then width, delay,
// the phase of each ear, inversion and sweep, and the alternation.
void Ui::drawLab() {
    char text[20];
    LabSide& side = _adv.lab[_adv.labSlot < 2 ? _adv.labSlot : 0];
    snprintf(_advLabel[0], sizeof(_advLabel[0]), "%s", _adv.labSlot == 0 ? "(A)" : "A");
    snprintf(_advLabel[1], sizeof(_advLabel[1]), "%s", _adv.labSlot == 1 ? "(B)" : "B");
    snprintf(_advLabel[2], sizeof(_advLabel[2]), "MATCH %s", _adv.labMatch ? "on" : "off");
    snprintf(_advLabel[3], sizeof(_advLabel[3]), "INV L %s", side.invertL ? "on" : "off");
    snprintf(_advLabel[4], sizeof(_advLabel[4]), "INV R %s", side.invertR ? "on" : "off");
    snprintf(_advLabel[5], sizeof(_advLabel[5]), "%s", side.lab.bilateralGolden ? "GOLDEN" : "EVEN");
    if (!_valuesOnly) {
        addButton(8, kTop + 1, 52, 28, _advLabel[0], kAction);
        addButton(64, kTop + 1, 52, 28, _advLabel[1], kAction + 1);
        addButton(120, kTop + 1, 112, 28, _advLabel[2], kAction + 2);
        addButton(8, kTop + 151, 72, 28, _advLabel[3], kAction + 3);
        addButton(84, kTop + 151, 72, 28, _advLabel[4], kAction + 4);
        addButton(160, kTop + 151, 72, 28, "SWEEP", kAction + 5);
        addButton(4, kTop + 181, 80, 28, _advLabel[5], kAction + 6);
    } else {
        for (uint8_t i = 0; i < 7; ++i) {
            if (i != 5) redrawButton(static_cast<uint8_t>(kAction + i));
        }
    }
    snprintf(text, sizeof(text), "%u%%", static_cast<unsigned>(side.lab.widthPercent));
    row(kTop + 31, "Width", text, kAdjust, kAdjust + 1);
    // Which ear hears later, and by how many microseconds.
    if (side.lab.delayUs == 0) snprintf(text, sizeof(text), "none");
    else snprintf(text, sizeof(text), "%c %d", side.lab.delayUs > 0 ? 'R' : 'L', abs(side.lab.delayUs));
    row(kTop + 61, "Delay us", text, kAdjust + 2, kAdjust + 3);
    snprintf(text, sizeof(text), "%d", static_cast<int>(side.phaseL));
    row(kTop + 91, "Phase L", text, kAdjust + 4, kAdjust + 5);
    snprintf(text, sizeof(text), "%d", static_cast<int>(side.phaseR));
    row(kTop + 121, "Phase R", text, kAdjust + 6, kAdjust + 7);
    // The alternation between the ears: its period, and below it its depth.
    if (side.lab.bilateralPeriodDs == 0) snprintf(text, sizeof(text), "off");
    else snprintf(text, sizeof(text), "%u.%u s", side.lab.bilateralPeriodDs / 10u, side.lab.bilateralPeriodDs % 10u);
    row(kTop + 181, nullptr, text, kAdjust + 8, kAdjust + 9);
    snprintf(text, sizeof(text), "%u%%", static_cast<unsigned>(side.lab.bilateralDepth));
    row(kTop + 211, "Depth", text, kAdjust + 10, kAdjust + 11);
}

// ---- input -----------------------------------------------------------------

void Ui::onAdvancedButton(uint8_t id) {
    if (mindSub()) {
        onMindButton(id);
        return;
    }
    const bool adjust = id >= kAdjust && id < kAction;
    const uint8_t n = static_cast<uint8_t>(adjust ? id - kAdjust : id - kAction);
    const int direction = (n % 2) ? 1 : -1;  // adjusters come in pairs: minus, plus
    switch (_sub) {
        case Sub::Menu:
            if (n == 0) openSub(Sub::Mixer);
            else if (n == 1) openSub(Sub::Breath);
            else if (n == 2) openSub(Sub::Builder);
            else if (n == 3) openSub(Sub::Sequence);
            else if (n == 4) openSub(Sub::Lab);
            return;

        case Sub::Mixer:
            if (!adjust) {
                if (n < 4) _adv.layerOn[n] = !_adv.layerOn[n];
                else _adv.fadeChoice = static_cast<uint8_t>((_adv.fadeChoice + 1) % kFadeChoiceCount);
                _pending = Event::MixerChanged;
                redrawButton(id);
                redrawValues();
                return;
            }
            // Noise and recording levels belong to the tabs: the same events.
            if (n / 2 == 2) { _pending = direction > 0 ? Event::NoiseVolumeUp : Event::NoiseVolumeDown; return; }
            if (n / 2 == 3) { _pending = direction > 0 ? Event::SoundVolumeUp : Event::SoundVolumeDown; return; }
            if (n / 2 == 0) _adv.toneLevel = stepped(_adv.toneLevel, 5 * direction, 0, 100);
            else if (n / 2 == 1) _adv.partialsLevel = stepped(_adv.partialsLevel, 5 * direction, 0, 100);
            else _adv.partial[n / 2 - 4] = stepped(_adv.partial[n / 2 - 4], 5 * direction, 0, 100);
            _pending = Event::MixerChanged;
            redrawValues();
            return;

        case Sub::Breath:
            if (!adjust) {
                _adv.breathOn = !_adv.breathOn;
                _pending = Event::BreathChanged;
                redrawValues();
                redrawButton(id);
                return;
            }
            if (n / 2 == 0) _adv.breath.inhaleDs = stepped(_adv.breath.inhaleDs, 5 * direction, rc::kBreathMinDs, rc::kBreathMaxDs);
            else if (n / 2 == 1) _adv.breath.holdInDs = stepped(_adv.breath.holdInDs, 5 * direction, 0, rc::kBreathMaxDs);
            else if (n / 2 == 2) _adv.breath.exhaleDs = stepped(_adv.breath.exhaleDs, 5 * direction, rc::kBreathMinDs, rc::kBreathMaxDs);
            else if (n / 2 == 3) _adv.breath.holdOutDs = stepped(_adv.breath.holdOutDs, 5 * direction, 0, rc::kBreathMaxDs);
            else _adv.breathDepth = stepped(_adv.breathDepth, 5 * direction, 0, rc::kBreathDepthMax);
            if (_adv.breathOn) _pending = Event::BreathChanged;
            redrawValues();
            return;

        case Sub::Builder: {
            rc::Session& s = _adv.session;
            _adv.message = "";
            if (adjust) {
                const int next = static_cast<int>(_adv.stageIndex) + direction;
                if (next >= 0 && next < s.stageCount) _adv.stageIndex = static_cast<uint8_t>(next);
            } else if (n == 0) {
                openSub(Sub::Stage);
                return;
            } else if (n == 1) {
                if (rc::addStage(s, _adv.stageIndex)) ++_adv.stageIndex;
                else _adv.message = "No room for another stage";
            } else if (n == 2) {
                if (!rc::removeStage(s, _adv.stageIndex)) _adv.message = "The last stage stays";
                else if (_adv.stageIndex >= s.stageCount) _adv.stageIndex = static_cast<uint8_t>(s.stageCount - 1);
            } else if (n == 3 || n == 4) {
                const int way = n == 3 ? -1 : 1;
                if (rc::moveStage(s, _adv.stageIndex, way)) _adv.stageIndex = static_cast<uint8_t>(_adv.stageIndex + way);
            } else if (n == 5) {
                s = rc::newSession();
                _adv.stageIndex = 0;
            } else if (n == 6 || n == 7) {
                _slotKind = 0;
                _slotSave = n == 7;
                _adv.slotsReady = false;
                _eventArg = 0;
                _pending = Event::SlotsRefresh;
                openSub(Sub::Slots);
                return;
            } else if (n == 8) {
                // The running session is watched on the SESS tab.
                _pending = Event::CustomStart;
                _sub = Sub::None;
                _tab = Tab::Sessions;
                drawHeader();
                drawTabBar();
                drawContent();
                return;
            }
            redrawValues();
            return;
        }

        case Sub::Stage: {
            rc::Stage& stage = _adv.session.stages[_adv.stageIndex];
            if (!adjust) {
                if (n == 0) {
                    rc::cycleStageSweep(stage);
                } else {
                    _eventArg = static_cast<uint8_t>(n - 1);
                    _pending = Event::StageCapture;  // the application knows what is set now
                    return;
                }
            } else if (n / 2 == 0) {
                // A session of one stage may have no length; here it gets one.
                if (stage.durationS == 0) stage.durationS = 300;
                else rc::stepStageDuration(stage, direction);
            } else if (n / 2 == 1) {
                rc::stepStageTransition(stage, direction);
            } else if (n / 2 == 2) {
                rc::stepStagePrimary(stage, static_cast<float>(direction), _settings.primaryHz);
            } else {
                rc::stepStageBeat(stage, 0.5f * static_cast<float>(direction), _settings.beatHz);
            }
            redrawValues();
            return;
        }

        case Sub::Slots: {
            if (adjust || !_adv.slotsReady) return;
            if (_slotSave && _adv.slotUsed[n] && _slotArmed != n) {
                _slotArmed = n;  // a second tap replaces what is there
                drawContent();
                return;
            }
            if (!_slotSave && !_adv.slotUsed[n]) return;
            _eventArg = static_cast<uint8_t>(n + 1);
            if (_slotKind == 0) _pending = _slotSave ? Event::SessionSave : Event::SessionLoad;
            else _pending = _slotSave ? Event::SequenceSave : Event::SequenceLoad;
            // Back to where the slots were opened from; the application
            // draws it again once the card has answered.
            _sub = _slotKind == 0 ? Sub::Builder : Sub::Sequence;
            _slotArmed = 0xFF;
            drawHeader();
            return;
        }

        case Sub::Lab: {
            LabSide& side = _adv.lab[_adv.labSlot < 2 ? _adv.labSlot : 0];
            rc::LabSettings& lab = side.lab;
            _pending = Event::LabChanged;
            if (!adjust) {
                if (n < 2) {
                    if (n == _adv.labSlot) { _pending = Event::None; return; }
                    _eventArg = n;
                    _pending = Event::LabSwitch;  // the application redraws once the side is in place
                    return;
                }
                if (n == 2) _adv.labMatch = !_adv.labMatch;
                else if (n == 3) side.invertL = !side.invertL;
                else if (n == 4) side.invertR = !side.invertR;
                else if (n == 5) _pending = Event::LabSweep;
                else lab.bilateralGolden = !lab.bilateralGolden;
            } else if (n / 2 == 0) {
                lab.widthPercent = stepped(lab.widthPercent, 5 * direction, 0, 100);
            } else if (n / 2 == 1) {
                int next = lab.delayUs + direction * rc::kLabDelayStepUs;
                if (next > rc::kLabDelayMaxUs) next = rc::kLabDelayMaxUs;
                if (next < -rc::kLabDelayMaxUs) next = -rc::kLabDelayMaxUs;
                lab.delayUs = static_cast<int16_t>(next);
            } else if (n / 2 == 2 || n / 2 == 3) {
                int16_t& phase = n / 2 == 2 ? side.phaseL : side.phaseR;
                int next = phase + 15 * direction;
                if (next > 180) next -= 360;
                if (next < -180) next += 360;
                phase = static_cast<int16_t>(next);
            } else if (n / 2 == 4) {
                // Off, then half-second steps, coarser as the period grows.
                const uint16_t now = lab.bilateralPeriodDs;
                const uint16_t step = now < 50 ? 5 : (now < 200 ? 10 : 50);
                int next = direction > 0 ? now + step : static_cast<int>(now) - (now <= 50 ? 5 : (now <= 200 ? 10 : 50));
                if (next < rc::kBilateralMinDs) next = direction > 0 ? rc::kBilateralMinDs : 0;
                if (next > rc::kBilateralMaxDs) next = rc::kBilateralMaxDs;
                lab.bilateralPeriodDs = static_cast<uint16_t>(next);
            } else {
                lab.bilateralDepth = stepped(lab.bilateralDepth, 5 * direction, 0, 100);
            }
            redrawValues();
            return;
        }

        case Sub::Sequence: {
            rc::Sequence& q = _adv.sequence;
            _adv.message = "";
            if (adjust) {
                if (n < 2) {
                    const int next = static_cast<int>(_step) + direction;
                    if (next >= 0 && next < q.stepCount) _step = static_cast<uint8_t>(next);
                } else if (n < 6) {
                    static const float kDelta[] = {-10.0f, -1.0f, 1.0f, 10.0f};
                    rc::stepStepHz(q.steps[_step], kDelta[n - 2]);
                } else if (n < 8) {
                    rc::stepStepSeconds(q.steps[_step], direction);
                } else {
                    rc::stepGlide(q, direction);
                }
            } else if (n == 0) {
                if (rc::addStep(q, _step)) ++_step;
            } else if (n == 1) {
                if (rc::removeStep(q, _step) && _step >= q.stepCount) _step = static_cast<uint8_t>(q.stepCount - 1);
            } else if (n == 2) {
                q = rc::defaultSequence();
                _step = 0;
            } else if (n == 3 || n == 4) {
                _slotKind = 1;
                _slotSave = n == 4;
                _adv.slotsReady = false;
                _eventArg = 1;
                _pending = Event::SlotsRefresh;
                openSub(Sub::Slots);
                return;
            } else if (n == 5) {
                _adv.session = rc::sequenceSession(q);
                _adv.stageIndex = 0;
                openSub(Sub::Builder);
                return;
            }
            redrawValues();
            return;
        }

        default:
            return;
    }
}

// ---- development aids ------------------------------------------------------

void Ui::press(int16_t x, int16_t y) {
    if (_dimmed) wake();
    _lastActivityMs = millis();
    onPress(x, y);
    _repeat.release();
    _heldId = 0xFF;
}

void Ui::describe(Print& out) {
    out.printf("UI tab %u sub %u, %u button(s)\n", static_cast<unsigned>(_tab), static_cast<unsigned>(_sub),
               static_cast<unsigned>(_buttonCount));
    describeTitle(out);
    for (uint8_t i = 0; i < _buttonCount; ++i) {
        const Button& b = _buttons[i];
        _lcd.setFont(kFontText);
        const int wide = _lcd.textWidth(b.label);
        _lcd.setFont(kFontSmall);
        const int narrow = _lcd.textWidth(b.label);
        bool overlap = false;
        for (uint8_t j = 0; j < _buttonCount; ++j) {
            const Button& o = _buttons[j];
            if (j != i && b.x < o.x + o.w && o.x < b.x + b.w && b.y < o.y + o.h && o.y < b.y + b.h) overlap = true;
        }
        const bool inside = b.x >= 0 && b.x + b.w <= kScreenW && b.y >= kContentTop && b.y + b.h <= kContentBottom;
        out.printf("UI %3u  %3d,%3d %3dx%2d  '%s' text %d/%d%s%s%s\n", static_cast<unsigned>(b.id), b.x, b.y, b.w, b.h, b.label,
                   wide, narrow, narrow > b.w - 4 ? " TOO WIDE" : (wide > b.w - 4 ? " small" : ""), overlap ? " OVERLAP" : "",
                   inside ? "" : " OUTSIDE");
    }
    // Text drawn past the left or right edge since the last call.
    for (uint8_t i = 0; i < _overflowCount && i < 4; ++i) {
        out.printf("UI text OFF SCREEN (to x=%d): '%s'\n", _overflowEdge[i], _overflow[i]);
    }
    if (_overflowCount > 4) out.printf("UI text OFF SCREEN: %u more\n", static_cast<unsigned>(_overflowCount - 4));
    _overflowCount = 0;
}

// ---- the breathing bar -----------------------------------------------------

// Fills the bar to where the level stands: only the pixels that changed.
void Ui::drawBreathBar(int16_t x, int16_t y, int16_t w, int16_t h, int16_t& shown) {
    float level = _adv.breathOn ? _adv.breathLevel : 0.0f;
    if (!(level >= 0.0f)) level = 0.0f;
    if (level > 1.0f) level = 1.0f;
    const int16_t want = static_cast<int16_t>(level * static_cast<float>(w) + 0.5f);
    if (want == shown) return;
    if (want > shown) _lcd.fillRect(x + shown, y, want - shown, h, kTeal);
    else _lcd.fillRect(x + want, y, shown - want, h, kBackground);
    shown = want;
}

// A thin bar on Home while the pacer runs (it covers nothing and moves
// nothing), and a larger one on the breathing screen.
void Ui::animateBreath() {
    if (_dimmed || _helpOpen) return;
    if (_sub == Sub::Breath) drawBreathBar(20, kTop + 222, 200, 8, _breathBar);
    else if (_sub == Sub::None && _tab == Tab::Home && !_btScreen) drawBreathBar(40, kTop + 136, 160, 3, _homeBar);
}

}  // namespace gui
