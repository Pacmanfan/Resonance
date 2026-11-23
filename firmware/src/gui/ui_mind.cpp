// The Mindfield screens (RES-MIND-006) and the drawing of its pictures
// (RES-MIND-005): home, source, set-up, the active experiment, the result
// and the history. They are opened from MINDFIELD on the SET tab and use the
// same buttons and helpers as the other screens.
//
// What a picture shows is decided in rc_mindviz.h, from numbers the
// statistics engine has computed. This file only draws. The active screen
// shows no score in figures and no word of success or failure: the numbers
// are on the result screen, with what was expected beside them.
#include <Arduino.h>

#include "theme.h"
#include "ui.h"

namespace gui {

namespace {

using namespace theme;

const lgfx::IFont* const kFontSmall = &fonts::DejaVu12;
const lgfx::IFont* const kFontText = &fonts::DejaVu18;

constexpr uint8_t kAction = 150;  // the buttons of a screen, in the order they are added
constexpr int16_t kTop = kContentTop;

// The picture's field on the active screen.
constexpr int16_t kVizH = 196;
constexpr int16_t kCx = kScreenW / 2;
constexpr int16_t kCy = kTop + kVizH / 2;
constexpr int16_t kFieldR = 88;
constexpr uint16_t kFaint = rgb(0x28, 0x3C, 0x60);
constexpr uint16_t kGlow = rgb(0x9A, 0xF4, 0xFF);

const char* const kStateNames[] = {"idle", "running", "paused", "finished"};

bool scorePicture(rc::MindViz v) {
    return v == rc::MindViz::Orb || v == rc::MindViz::Balance || v == rc::MindViz::Star || v == rc::MindViz::Flame ||
           v == rc::MindViz::Vortex || v == rc::MindViz::Pendulum;
}

void clock(char* out, size_t capacity, uint32_t s) {
    snprintf(out, capacity, "%lu:%02lu", static_cast<unsigned long>(s / 60), static_cast<unsigned long>(s % 60));
}

const char* lengthName(uint16_t seconds, char* buffer, size_t capacity) {
    if (seconds == 0) return "until stopped";
    snprintf(buffer, capacity, "%u min", static_cast<unsigned>(seconds / 60));
    return buffer;
}

}  // namespace

// ---- screens -------------------------------------------------------------------

void Ui::drawMind() {
    switch (_sub) {
        case Sub::Mind: drawMindHome(); break;
        case Sub::MindSource: drawMindSource(); break;
        case Sub::MindSetup: drawMindSetup(); break;
        case Sub::MindRun: drawMindRun(); break;
        case Sub::MindResult: drawMindResult(); break;
        case Sub::MindHistory: drawMindHistory(); break;
        default: break;
    }
}

void Ui::drawMindHome() {
    Mind& m = _adv.mind;
    const MindSourceInfo& source = m.sources[m.source < kMindSources ? m.source : 0];
    const bool busy = m.state == 1 || m.state == 2;
    snprintf(_mindLabel[0], sizeof(_mindLabel[0]), "%s", source.name);
    label("Source", 8, kTop + 4, kTextDim, kFontSmall);
    addButton(8, kTop + 20, 224, 30, _mindLabel[0], kAction);
    label(source.physical ? "Physical entropy: yes" : (source.available ? "Physical entropy: not established" : "Not available"),
          8, kTop + 53, source.physical ? kTeal : kWarning, kFontSmall);
    addButton(8, kTop + 72, 224, 32, "SET UP EXPERIMENT", kAction + 1);
    addButton(8, kTop + 108, 224, 36, busy ? "SHOW EXPERIMENT" : "START", kAction + 2);
    addButton(8, kTop + 150, 110, 30, "RESULT", kAction + 3);
    addButton(122, kTop + 150, 110, 30, "HISTORY", kAction + 4);
    char text[40], length[16];
    snprintf(text, sizeof(text), "%s, %s, %u/s, %s", m.directed ? rc::mindTargetName(m.target) : "Open",
             rc::mindConditionName(m.condition), static_cast<unsigned>(kMindRates[m.rateIndex % kMindRateCount]),
             lengthName(kMindSeconds[m.secondsIndex % kMindSecondsCount], length, sizeof(length)));
    label(text, 8, kTop + 186, kTextDim, kFontSmall);
    if (m.message[0] != '\0') label(m.message, 8, kTop + 202, kWarning, kFontSmall);
    label("An instrument, not a proof of anything.", 8, kTop + 220, kTextDim, kFontSmall);
}

// One row a source. An unavailable source is text, not a button: it cannot
// be chosen.
void Ui::drawMindSource() {
    Mind& m = _adv.mind;
    for (uint8_t i = 0; i < kMindSources; ++i) {
        const MindSourceInfo& source = m.sources[i];
        const int16_t y = static_cast<int16_t>(kTop + 2 + i * 31);
        if (source.available) {
            snprintf(_advLabel[i], sizeof(_advLabel[i]), "%s%s", i == m.source ? "> " : "", source.name);
            addButton(4, y, 232, 28, _advLabel[i], static_cast<uint8_t>(kAction + i));
        } else {
            label(source.name, 8, y, kTextDim, kFontSmall);
            label(source.status, 8, y + 14, kWarning, kFontSmall);
        }
    }
    const MindSourceInfo& chosen = m.sources[m.source < kMindSources ? m.source : 0];
    char text[44];
    const int16_t y = kTop + 190;
    snprintf(text, sizeof(text), "%s, up to %u/s", chosen.className, static_cast<unsigned>(chosen.maxRate));
    label(text, 8, y, kText, kFontSmall);
    label(chosen.status, 8, y + 15, chosen.physical ? kTeal : kWarning, kFontSmall);
    label(chosen.suitable ? "Suitable for experiments" : "For studying the source only", 8, y + 30,
          chosen.suitable ? kTextDim : kWarning, kFontSmall);
}

// Each setting is one button that shows its value; a tap takes the next.
void Ui::drawMindSetup() {
    Mind& m = _adv.mind;
    static const char* const kNames[] = {"Mode", "Target", "Condition", "Rate", "Length", "Picture"};
    char length[16];
    snprintf(_advLabel[0], sizeof(_advLabel[0]), "%s", m.directed ? "Directed" : "Open");
    snprintf(_advLabel[1], sizeof(_advLabel[1]), "%s", m.directed ? rc::mindTargetName(m.target) : "none");
    snprintf(_advLabel[2], sizeof(_advLabel[2]), "%s", rc::mindConditionName(m.condition));
    snprintf(_advLabel[3], sizeof(_advLabel[3]), "%u a second", static_cast<unsigned>(kMindRates[m.rateIndex % kMindRateCount]));
    snprintf(_advLabel[4], sizeof(_advLabel[4]), "%s", lengthName(kMindSeconds[m.secondsIndex % kMindSecondsCount], length, sizeof(length)));
    snprintf(_advLabel[5], sizeof(_advLabel[5]), "%s", rc::mindVizName(m.viz));
    for (uint8_t i = 0; i < 6; ++i) {
        const int16_t y = static_cast<int16_t>(kTop + 4 + i * 33);
        label(kNames[i], 8, y + 8, kTextDim, kFontSmall);
        // An open experiment has no target to choose.
        if (i == 1 && !m.directed) label("none (open)", 92, y + 8, kTextDim, kFontSmall);
        else addButton(88, y, 146, 28, _advLabel[i], static_cast<uint8_t>(kAction + i));
    }
    label(scorePicture(m.viz) ? "Picture of the measured score." : "Pattern from samples, not a score.", 8, kTop + 204,
          kTextDim, kFontSmall);
    label("Fixed once the experiment starts.", 8, kTop + 220, kTextDim, kFontSmall);
}

void Ui::drawMindRun() {
    Mind& m = _adv.mind;
    snprintf(_advLabel[0], sizeof(_advLabel[0]), "%s", m.state == 2 ? "RESUME" : "PAUSE");
    if (!m.replaying) addButton(8, kTop + 212, 108, 26, _advLabel[0], kAction);
    addButton(m.replaying ? 8 : 124, kTop + 212, m.replaying ? 224 : 108, 26, m.replaying ? "END REPLAY" : "STOP", kAction + 1);
    vizBegin();
    mindStatusLine();
}

// Time, samples and whether it is being recorded: collection status, and
// nothing about how the numbers are going.
void Ui::mindStatusLine() {
    const Mind& m = _adv.mind;
    char text[48], elapsed[12], planned[12];
    clock(elapsed, sizeof(elapsed), m.elapsedS);
    if (m.plannedSamples != 0 && m.rate != 0) {
        clock(planned, sizeof(planned), m.plannedSamples / m.rate);
        snprintf(text, sizeof(text), "%s / %s  %lu  %s", elapsed, planned, static_cast<unsigned long>(m.samples),
                 m.replaying ? "REPLAY" : (m.state == 2 ? "PAUSED" : (m.recorded ? "REC" : "NOT REC")));
    } else {
        snprintf(text, sizeof(text), "%s  %lu  %s", elapsed, static_cast<unsigned long>(m.samples),
                 m.replaying ? "REPLAY" : (m.state == 2 ? "PAUSED" : (m.recorded ? "REC" : "NOT REC")));
    }
    _lcd.fillRect(0, kTop + kVizH, kScreenW, 15, kBackground);
    centered(text, kCx, kTop + kVizH + 8, m.recorded || m.replaying ? kTextDim : kWarning, kBackground, kFontSmall);
}

void Ui::drawMindResult() {
    const Mind& m = _adv.mind;
    if (m.summary == nullptr || m.header == nullptr) {
        label("No result yet.", 8, kTop + 12, kTextDim, kFontText);
        addButton(8, kTop + 206, 224, 30, "BACK", kAction);
        return;
    }
    const rc::MindHeader& h = *m.header;
    const rc::MindSummary& s = *m.summary;
    char text[48];
    int16_t y = kTop + 2;
    auto line = [&](uint16_t colour) {
        label(text, 6, y, colour, kFontSmall);
        y += 14;
    };
    snprintf(text, sizeof(text), "#%lu %s", static_cast<unsigned long>(h.experimentId), rc::rngIdName(h.source));
    line(kCyan);
    snprintf(text, sizeof(text), "%s", h.physicalEntropy ? "Physical entropy at the start: yes"
                                          : (h.sourceClass == rc::RngClass::SoftwareControl ? "Software control: not random"
                                                                                            : "Entropy not established"));
    line(h.physicalEntropy ? kTeal : kWarning);
    snprintf(text, sizeof(text), "%s, %s%s%s", rc::mindConditionName(h.condition), rc::mindModeName(h.mode),
             h.mode == rc::MindMode::Directed ? ": " : "", h.mode == rc::MindMode::Directed ? rc::mindTargetName(h.target) : "");
    line(kText);
    snprintf(text, sizeof(text), "%lu samples at %u/s, %lu gap(s)", static_cast<unsigned long>(s.samples),
             static_cast<unsigned>(h.rate), static_cast<unsigned long>(m.gaps));
    line(kTextDim);
    snprintf(text, sizeof(text), "Ones %.5f  expected 0.50000", s.proportion);
    line(kText);
    snprintf(text, sizeof(text), "95%%: %.5f .. %.5f", s.ciLow, s.ciHigh);
    line(kTextDim);
    snprintf(text, sizeof(text), "Score z %+.2f   p %.3f", s.z, s.pTwoSided);
    line(kText);
    if (h.mode == rc::MindMode::Directed) {
        snprintf(text, sizeof(text), "To target %+.2f   p %.3f", s.directedZ, s.pDirected);
        line(kText);
    }
    snprintf(text, sizeof(text), "Runs %+.1f  bytes %+.1f  lag1 %+.1f", s.runsZ, s.chi2Z, s.autocorrZ[0]);
    line(kTextDim);
    snprintf(text, sizeof(text), "%lu windows: max %.2f, limit %.2f", static_cast<unsigned long>(s.windows), s.windowMaxAbsZ,
             s.windowCritZ);
    line(kTextDim);
    snprintf(text, sizeof(text), "Beyond 1.96: %lu (%.0f by chance)", static_cast<unsigned long>(s.windowsBeyond95),
             0.05 * static_cast<double>(s.windows));
    line(kTextDim);
    if (m.baseline.valid) {
        snprintf(text, sizeof(text), "vs baseline #%lu: z %+.2f p %.3f", static_cast<unsigned long>(m.baselineId),
                 m.baseline.z, m.baseline.pTwoSided);
        line(kText);
    } else {
        snprintf(text, sizeof(text), "No baseline from this source yet");
        line(kWarning);
    }
    snprintf(text, sizeof(text), "%s%s%s", rc::mindEndName(m.reason), m.recorded ? ", on the card" : ", NOT recorded",
             m.message[0] ? "" : "");
    line(m.recorded ? kTextDim : kWarning);
    label("Chance alone gives scores like these.", 6, y, kTextDim, kFontSmall);
    addButton(8, kTop + 208, 108, 30, "DONE", kAction);
    if (m.recorded) addButton(124, kTop + 208, 108, 30, "REPLAY", kAction + 1);
}

void Ui::drawMindHistory() {
    Mind& m = _adv.mind;
    for (uint8_t i = 0; i < m.historyCount && i < kMindPage; ++i) {
        const rc::MindRecord& r = m.history[i];
        if (r.complete) {
            snprintf(_mindLabel[i], sizeof(_mindLabel[i]), "#%lu %.4s %.5s %+.2f", static_cast<unsigned long>(r.experimentId),
                     r.source == rc::RngId::Esp32Hardware ? "HW" : (r.source == rc::RngId::Software ? "SW" : "EXP"),
                     rc::mindConditionName(r.condition), r.z);
        } else {
            snprintf(_mindLabel[i], sizeof(_mindLabel[i]), "#%lu unreadable", static_cast<unsigned long>(r.experimentId));
        }
        addButton(8, static_cast<int16_t>(kTop + 4 + i * 34), 224, 30, _mindLabel[i], static_cast<uint8_t>(kAction + i));
    }
    char text[40];
    if (m.historyTotal == 0) snprintf(text, sizeof(text), "No experiments on the card");
    else snprintf(text, sizeof(text), "%u to %u of %u, newest first", static_cast<unsigned>(m.historySkip + 1),
                  static_cast<unsigned>(m.historySkip + m.historyCount), static_cast<unsigned>(m.historyTotal));
    label(text, 8, kTop + 142, kTextDim, kFontSmall);
    if (m.message[0] != '\0') label(m.message, 8, kTop + 157, kWarning, kFontSmall);
    addButton(8, kTop + 174, 108, 28, "NEWER", kAction + 4);
    addButton(124, kTop + 174, 108, 28, "OLDER", kAction + 5);
    addButton(8, kTop + 208, 108, 28, "EXPORT", kAction + 6);
    addButton(124, kTop + 208, 108, 28, "RECOVER", kAction + 7);
}

// ---- input ---------------------------------------------------------------------

void Ui::onMindButton(uint8_t id) {
    Mind& m = _adv.mind;
    const uint8_t n = static_cast<uint8_t>(id - kAction);
    const bool busy = m.state == 1 || m.state == 2;
    switch (_sub) {
        case Sub::Mind:
            if (n == 0 && !busy) {
                _pending = Event::MindRefresh;
                openSub(Sub::MindSource);
            } else if (n == 1 && !busy) {
                openSub(Sub::MindSetup);
            } else if (n == 2) {
                if (busy) openSub(Sub::MindRun);
                else _pending = Event::MindStart;  // the application opens the active screen when it has started
            } else if (n == 3) {
                openSub(Sub::MindResult);
            } else if (n == 4) {
                m.historySkip = 0;
                _pending = Event::MindHistory;
                openSub(Sub::MindHistory);
            } else if (busy) {
                m.message = "Fixed while an experiment runs";
                drawContent();
            }
            return;
        case Sub::MindSource:
            if (n < kMindSources && m.sources[n].available) {
                m.source = n;
                // A rate the source is not validated for is brought down.
                while (m.rateIndex > 0 && kMindRates[m.rateIndex] > m.sources[n].maxRate) --m.rateIndex;
                drawContent();
            }
            return;
        case Sub::MindSetup: {
            const uint16_t maxRate = m.sources[m.source < kMindSources ? m.source : 0].maxRate;
            if (n == 0) m.directed = !m.directed;
            else if (n == 1) m.target = m.target == rc::MindTarget::MoreOnes ? rc::MindTarget::MoreZeros : rc::MindTarget::MoreOnes;
            else if (n == 2) m.condition = static_cast<rc::MindCondition>((static_cast<uint8_t>(m.condition) + 1) % 4);
            else if (n == 3) {
                m.rateIndex = static_cast<uint8_t>((m.rateIndex + 1) % kMindRateCount);
                if (kMindRates[m.rateIndex] > maxRate) m.rateIndex = 0;
            } else if (n == 4) m.secondsIndex = static_cast<uint8_t>((m.secondsIndex + 1) % kMindSecondsCount);
            else if (n == 5) m.viz = static_cast<rc::MindViz>(static_cast<uint8_t>(m.viz) % 12 + 1);
            drawContent();
            return;
        }
        case Sub::MindRun:
            if (n == 0 && !m.replaying) _pending = m.state == 2 ? Event::MindResume : Event::MindPause;
            else if (n == 1) _pending = Event::MindStop;
            return;
        case Sub::MindResult:
            if (n == 0) {
                _pending = Event::MindDone;
                openSub(Sub::Mind);
            } else if (n == 1 && m.recorded) {
                _pending = Event::MindReplay;
            }
            return;
        case Sub::MindHistory:
            if (n < kMindPage && n < m.historyCount) {
                _eventArg = n;
                _pending = Event::MindOpen;
            } else if (n == 4 && m.historySkip >= kMindPage) {
                m.historySkip = static_cast<uint8_t>(m.historySkip - kMindPage);
                _pending = Event::MindHistory;
            } else if (n == 5 && m.historySkip + kMindPage < m.historyTotal) {
                m.historySkip = static_cast<uint8_t>(m.historySkip + kMindPage);
                _pending = Event::MindHistory;
            } else if (n == 6) {
                _pending = Event::MindExport;
            } else if (n == 7) {
                _pending = Event::MindRecover;
            }
            return;
        default:
            return;
    }
}

void Ui::showMindRun() {
    if (_dimmed) wake();
    openSub(Sub::MindRun);
}

void Ui::showMindResult() {
    if (mindSub()) openSub(Sub::MindResult);
}

void Ui::mindPauseChanged() {
    if (_sub != Sub::MindRun) return;
    snprintf(_advLabel[0], sizeof(_advLabel[0]), "%s", _adv.mind.state == 2 ? "RESUME" : "PAUSE");
    redrawButton(kAction);
    mindStatusLine();
}

// ---- pictures ------------------------------------------------------------------

// Draws what does not move, and puts every picture in its neutral state.
void Ui::vizBegin() {
    const Mind& m = _adv.mind;
    _lcd.fillRect(0, kTop, kScreenW, kVizH, kBackground);
    _vizShown = 0.0f;
    _vizA = _vizB = 0;
    _vizPhase = 0;
    _vizWindows = 0;
    _vizFrameMs = millis();
    _tree.begin();
    _mandala.begin();
    _lotus.begin();
    _flow.begin();
    _maze.begin();
    const char* plus = m.input.directed ? "target" : "ones";
    const char* minus = m.input.directed ? "" : "zeros";
    switch (m.shownViz) {
        case rc::MindViz::Orb:
        case rc::MindViz::Vortex:
            _lcd.drawCircle(kCx, kCy, kFieldR, kOutline);
            _lcd.drawCircle(kCx, kCy, kFieldR + 1, kFaint);
            if (m.shownViz == rc::MindViz::Vortex) _lcd.drawCircle(kCx, kCy, kFieldR / 2, kFaint);  // the neutral orbit
            label(m.shownViz == rc::MindViz::Orb ? plus : "out", kCx + kFieldR - 40, kTop + 2, kTextDim, kFontSmall);
            label(m.shownViz == rc::MindViz::Orb ? minus : "in", 4, kTop + 2, kTextDim, kFontSmall);
            break;
        case rc::MindViz::Balance:
            _lcd.fillTriangle(kCx, kCy, kCx - 12, kCy + 60, kCx + 12, kCy + 60, kFaint);
            label(plus, kScreenW - 46, kTop + 2, kTextDim, kFontSmall);
            label(minus, 4, kTop + 2, kTextDim, kFontSmall);
            break;
        case rc::MindViz::Pendulum:
            _lcd.drawFastVLine(kCx, kTop + 10, kVizH - 30, kFaint);  // where it hangs when nothing deviates
            label(plus, kScreenW - 46, kTop + 2, kTextDim, kFontSmall);
            label(minus, 4, kTop + 2, kTextDim, kFontSmall);
            break;
        case rc::MindViz::River:
        case rc::MindViz::Waterfall:
        case rc::MindViz::Labyrinth:
        case rc::MindViz::Tree:
        case rc::MindViz::Mandala:
        case rc::MindViz::Lotus:
            label("Pattern from samples - not a score", 8, kTop + 2, kTextDim, kFontSmall);
            break;
        default:
            break;
    }
    if (m.shownViz == rc::MindViz::Labyrinth) {
        // Walls of the 7 x 7 maze, 24 pixels a cell.
        const int16_t cell = 24, x0 = kCx - cell * rc::kMazeSize / 2, y0 = kTop + 22;
        _lcd.drawRect(x0, y0, cell * rc::kMazeSize + 1, cell * rc::kMazeSize + 1, kOutline);
        for (uint8_t y = 0; y < rc::kMazeSize; ++y) {
            for (uint8_t x = 0; x < rc::kMazeSize; ++x) {
                const uint8_t open = rc::Labyrinth::openings(x, y);
                if ((open & 2u) == 0) _lcd.drawFastVLine(x0 + (x + 1) * cell, y0 + y * cell, cell + 1, kOutline);
                if ((open & 4u) == 0) _lcd.drawFastHLine(x0 + x * cell, y0 + (y + 1) * cell, cell + 1, kOutline);
            }
        }
        _lcd.fillRect(x0 + (rc::kMazeSize - 1) * cell + 6, y0 + (rc::kMazeSize - 1) * cell + 6, cell - 11, cell - 11, kTeal);
    } else if (m.shownViz == rc::MindViz::River) {
        // Three levels of forks, drawn once; the path of a particle lights up over them.
        for (uint8_t level = 0; level < rc::kFlowLevels; ++level) {
            const uint8_t forks = static_cast<uint8_t>(1u << level);
            const int16_t y = static_cast<int16_t>(kTop + 24 + level * 36);
            for (uint8_t f = 0; f < forks; ++f) {
                const int16_t span = static_cast<int16_t>(224 / forks);
                const int16_t x = static_cast<int16_t>(8 + span * f + span / 2);
                _lcd.drawLine(x, y, x - span / 4, y + 36, kFaint);
                _lcd.drawLine(x, y, x + span / 4, y + 36, kFaint);
            }
        }
    }
    vizFrame(true);
}

// One frame. `stepped` is true when the pattern pictures are to take the
// windows that have completed since the last frame.
void Ui::vizFrame(bool stepped) {
    const Mind& m = _adv.mind;
    const uint32_t now = millis();
    const uint32_t elapsed = now - _vizFrameMs;
    _vizFrameMs = now;
    const float target = rc::vizUnit(rc::vizScore(m.input));
    _vizShown = rc::vizApproach(_vizShown, target, elapsed);
    const float u = _vizShown;
    const bool newWindow = stepped && m.input.windows != _vizWindows && m.input.windows != 0;
    if (newWindow) _vizWindows = m.input.windows;
    const uint32_t sample = m.input.windowSample;

    switch (m.shownViz) {
        case rc::MindViz::Orb: {
            const int16_t x = static_cast<int16_t>(kCx + lrintf(u * (kFieldR - 12)));
            if (x == _vizA && _vizB != 0) break;
            if (_vizB != 0) _lcd.fillCircle(_vizA, kCy, 11, kBackground);
            _lcd.drawFastHLine(kCx - kFieldR + 2, kCy, 2 * kFieldR - 3, kFaint);
            _lcd.drawFastVLine(kCx, kCy - 6, 13, kOutline);  // the centre: what chance expects
            _lcd.fillCircle(x, kCy, 10, kCyan);
            _lcd.fillCircle(x - 3, kCy - 3, 3, kGlow);
            _vizA = x;
            _vizB = 1;
            break;
        }
        case rc::MindViz::Balance: {
            // Tilt of up to 20 degrees; the right pan sinks towards the target.
            const float angle = u * 0.35f;
            const int16_t dx = static_cast<int16_t>(lrintf(cosf(angle) * 84.0f)), dy = static_cast<int16_t>(lrintf(sinf(angle) * 84.0f));
            if (dy == _vizA && _vizB != 0) break;
            if (_vizB != 0) {
                const int16_t ox = static_cast<int16_t>(lrintf(sqrtf(84.0f * 84.0f - static_cast<float>(_vizA) * _vizA)));
                _lcd.drawLine(kCx - ox, kCy - _vizA, kCx + ox, kCy + _vizA, kBackground);
                _lcd.fillCircle(kCx - ox, kCy - _vizA + 14, 12, kBackground);
                _lcd.fillCircle(kCx + ox, kCy + _vizA + 14, 12, kBackground);
            }
            _lcd.drawFastHLine(kCx - 92, kCy, 184, kFaint);  // level: what chance expects
            _lcd.drawLine(kCx - dx, kCy - dy, kCx + dx, kCy + dy, kText);
            _lcd.fillCircle(kCx - dx, kCy - dy + 14, 11, kViolet);
            _lcd.fillCircle(kCx + dx, kCy + dy + 14, 11, kCyan);
            _lcd.fillCircle(kCx, kCy, 4, kOutline);
            _vizA = dy;
            _vizB = 1;
            break;
        }
        case rc::MindViz::Star: {
            // Size 14..80; 47 is the neutral size, marked by a ring.
            const int16_t r = static_cast<int16_t>(47 + lrintf(u * 33.0f));
            if (r == _vizA) break;
            if (_vizA != 0) {
                _lcd.fillTriangle(kCx, kCy - _vizA, kCx - _vizA / 4, kCy, kCx + _vizA / 4, kCy, kBackground);
                _lcd.fillTriangle(kCx, kCy + _vizA, kCx - _vizA / 4, kCy, kCx + _vizA / 4, kCy, kBackground);
                _lcd.fillTriangle(kCx - _vizA, kCy, kCx, kCy - _vizA / 4, kCx, kCy + _vizA / 4, kBackground);
                _lcd.fillTriangle(kCx + _vizA, kCy, kCx, kCy - _vizA / 4, kCx, kCy + _vizA / 4, kBackground);
            }
            const uint8_t bright = static_cast<uint8_t>(150 + lrintf(u * 100.0f));
            const uint16_t colour = rgb(bright, bright, 255);
            _lcd.fillTriangle(kCx, kCy - r, kCx - r / 4, kCy, kCx + r / 4, kCy, colour);
            _lcd.fillTriangle(kCx, kCy + r, kCx - r / 4, kCy, kCx + r / 4, kCy, colour);
            _lcd.fillTriangle(kCx - r, kCy, kCx, kCy - r / 4, kCx, kCy + r / 4, colour);
            _lcd.fillTriangle(kCx + r, kCy, kCx, kCy - r / 4, kCx, kCy + r / 4, colour);
            _lcd.drawCircle(kCx, kCy, 47, kFaint);
            _vizA = r;
            break;
        }
        case rc::MindViz::Flame: {
            // Height 30..150; 90 is neutral, marked at both sides.
            const int16_t h = static_cast<int16_t>(90 + lrintf(u * 60.0f));
            if (h == _vizA) break;
            const int16_t base = kTop + kVizH - 14;
            if (_vizA != 0) {
                _lcd.fillTriangle(kCx, base - _vizA, kCx - 26, base - 18, kCx + 26, base - 18, kBackground);
                _lcd.fillCircle(kCx, base - 18, 26, kBackground);
            }
            const uint8_t heat = static_cast<uint8_t>(170 + lrintf(u * 80.0f));
            _lcd.fillCircle(kCx, base - 18, 26, rgb(255, heat / 2, 20));
            _lcd.fillTriangle(kCx, base - h, kCx - 26, base - 18, kCx + 26, base - 18, rgb(255, heat / 2, 20));
            _lcd.fillCircle(kCx, base - 16, 14, rgb(255, heat, 90));
            _lcd.fillTriangle(kCx, base - h / 2 - 10, kCx - 13, base - 16, kCx + 13, base - 16, rgb(255, heat, 90));
            _lcd.drawFastHLine(kCx - 60, base - 90, 22, kOutline);
            _lcd.drawFastHLine(kCx + 38, base - 90, 22, kOutline);
            _vizA = h;
            break;
        }
        case rc::MindViz::Pendulum: {
            // A steady swing of 12 degrees about an equilibrium that leans up
            // to 25 degrees with the score. The swing itself means nothing.
            _vizPhase = static_cast<uint16_t>(_vizPhase + elapsed);
            const float angle = u * 0.44f + 0.21f * sinf(static_cast<float>(_vizPhase % 3000) * (6.2831853f / 3000.0f));
            const int16_t x = static_cast<int16_t>(kCx + lrintf(sinf(angle) * 150.0f));
            const int16_t y = static_cast<int16_t>(kTop + 10 + lrintf(cosf(angle) * 150.0f));
            if (_vizB != 0) {
                _lcd.drawLine(kCx, kTop + 10, _vizA, _vizB, kBackground);
                _lcd.fillCircle(_vizA, _vizB, 10, kBackground);
            }
            _lcd.drawFastVLine(kCx, kTop + 10, kVizH - 30, kFaint);
            _lcd.drawLine(kCx, kTop + 10, x, y, kTextDim);
            _lcd.fillCircle(x, y, 9, kCyan);
            _vizA = x;
            _vizB = y;
            break;
        }
        case rc::MindViz::Vortex: {
            // A point circling steadily; its distance from the centre is the
            // score: the faint ring is the neutral orbit.
            _vizPhase = static_cast<uint16_t>(_vizPhase + elapsed);
            const float angle = static_cast<float>(_vizPhase % 6000) * (6.2831853f / 6000.0f);
            const float radius = (kFieldR / 2) + u * (kFieldR / 2 - 10);
            const int16_t x = static_cast<int16_t>(kCx + lrintf(cosf(angle) * radius));
            const int16_t y = static_cast<int16_t>(kCy + lrintf(sinf(angle) * radius));
            if (_vizB != 0) _lcd.fillCircle(_vizA, _vizB, 7, kBackground);
            _lcd.drawCircle(kCx, kCy, kFieldR / 2, kFaint);
            _lcd.fillCircle(kCx, kCy, 2, kOutline);
            _lcd.fillCircle(x, y, 6, kViolet);
            _vizA = x;
            _vizB = y;
            break;
        }
        case rc::MindViz::Tree: {
            rc::TreeSegment segment;
            if (newWindow && _tree.step(sample, segment)) {
                const int16_t rootY = kTop + kVizH - 6;
                const uint16_t colour = segment.depth < 3 ? kTextDim : (segment.depth < 8 ? kTeal : kCyan);
                _lcd.drawLine(kCx + segment.x0, rootY - segment.y0, kCx + segment.x1, rootY - segment.y1, colour);
                if (segment.depth < 2) _lcd.drawLine(kCx + segment.x0 + 1, rootY - segment.y0, kCx + segment.x1 + 1, rootY - segment.y1, colour);
            }
            break;
        }
        case rc::MindViz::Mandala: {
            rc::MandalaElement e;
            if (newWindow && _mandala.step(sample, e)) {
                const uint8_t folds = rc::kMandalaFolds[1];
                const float radius = 16.0f + e.ring * 13.0f;
                const uint16_t colour = e.ring % 3 == 0 ? kCyan : (e.ring % 3 == 1 ? kViolet : kTeal);
                for (uint8_t f = 0; f < folds; ++f) {
                    // Each element and its mirror image within the fold.
                    for (int8_t side = -1; side <= 1; side += 2) {
                        const float a = (static_cast<float>(f) + side * static_cast<float>(e.angle) / 512.0f) * (6.2831853f / folds);
                        const int16_t x = static_cast<int16_t>(kCx + lrintf(cosf(a) * radius));
                        const int16_t y = static_cast<int16_t>(kCy + 4 + lrintf(sinf(a) * radius));
                        if (e.kind == 0) _lcd.fillCircle(x, y, 1 + e.size / 2, colour);
                        else if (e.kind == 1) _lcd.drawCircle(x, y, 2 + e.size, colour);
                        else if (e.kind == 2) _lcd.drawLine(x, y, static_cast<int16_t>(kCx + lrintf(cosf(a) * (radius + 5 + 2 * e.size))),
                                                           static_cast<int16_t>(kCy + 4 + lrintf(sinf(a) * (radius + 5 + 2 * e.size))), colour);
                        else _lcd.fillRect(x - 1, y - 1, 2 + e.size / 2, 2 + e.size / 2, colour);
                    }
                }
            }
            break;
        }
        case rc::MindViz::Lotus: {
            rc::LotusPetal p;
            if (newWindow && _lotus.step(sample, p)) {
                // Outer layer first; each petal a triangle with a round tip.
                const float reach = 78.0f - p.layer * 22.0f + p.size * 2.0f;
                const float a = (static_cast<float>(p.petal) + 0.5f * p.layer) * (6.2831853f / rc::kLotusPetals);
                const float half = 0.30f;
                const int16_t cy = kCy + 6;
                const uint16_t colour = p.layer == 0 ? kViolet : (p.layer == 1 ? rgb(0xE0, 0xC0, 0xFF) : kText);
                const int16_t tx = static_cast<int16_t>(kCx + lrintf(cosf(a) * reach)), ty = static_cast<int16_t>(cy + lrintf(sinf(a) * reach));
                const float inner = reach * 0.45f;
                _lcd.fillTriangle(tx, ty, static_cast<int16_t>(kCx + lrintf(cosf(a - half) * inner)),
                                  static_cast<int16_t>(cy + lrintf(sinf(a - half) * inner)),
                                  static_cast<int16_t>(kCx + lrintf(cosf(a + half) * inner)),
                                  static_cast<int16_t>(cy + lrintf(sinf(a + half) * inner)), colour);
                _lcd.fillTriangle(kCx, cy, static_cast<int16_t>(kCx + lrintf(cosf(a - half) * inner)),
                                  static_cast<int16_t>(cy + lrintf(sinf(a - half) * inner)),
                                  static_cast<int16_t>(kCx + lrintf(cosf(a + half) * inner)),
                                  static_cast<int16_t>(cy + lrintf(sinf(a + half) * inner)), colour);
                _lcd.drawLine(kCx, cy, tx, ty, kBackground);
                _lcd.fillCircle(kCx, cy, 5, kWarning);
            }
            break;
        }
        case rc::MindViz::River:
        case rc::MindViz::Waterfall: {
            if (!newWindow) break;
            const uint8_t channel = _flow.add(sample);
            const int16_t barBase = kTop + kVizH - 4;
            const int16_t barMax = m.shownViz == rc::MindViz::River ? 56 : 150;
            if (m.shownViz == rc::MindViz::River) {
                // The particle's path: the forks it took, lit; the last one fades back.
                int16_t x = 120, span = 224;
                for (uint8_t level = 0; level < rc::kFlowLevels; ++level) {
                    const int16_t y = static_cast<int16_t>(kTop + 24 + level * 36);
                    const int16_t next = static_cast<int16_t>(x + (rc::flowTurn(sample, level) ? span / 4 : -span / 4));
                    _lcd.drawLine(x, y, next, y + 36, kCyan);
                    x = next;
                    span /= 2;
                }
            }
            // The counts, as columns scaled to the fullest; a line marks the
            // equal share that chance expects.
            uint32_t most = 1;
            for (uint8_t c = 0; c < rc::kFlowChannels; ++c) most = _flow.count(c) > most ? _flow.count(c) : most;
            _lcd.fillRect(0, barBase - barMax - 2, kScreenW, barMax + 3, kBackground);
            for (uint8_t c = 0; c < rc::kFlowChannels; ++c) {
                const int16_t h = static_cast<int16_t>(_flow.count(c) * static_cast<uint32_t>(barMax) / most);
                _lcd.fillRect(static_cast<int16_t>(10 + c * 28), barBase - h, 24, h, c == channel ? kCyan : kPanel);
                _lcd.drawRect(static_cast<int16_t>(10 + c * 28), barBase - h, 24, h, kOutline);
            }
            const int16_t even = static_cast<int16_t>((_flow.total() / rc::kFlowChannels) * static_cast<uint32_t>(barMax) / most);
            _lcd.drawFastHLine(4, barBase - even, kScreenW - 8, kWarning);
            break;
        }
        case rc::MindViz::Labyrinth: {
            if (!newWindow) break;
            const int16_t cell = 24, x0 = kCx - cell * rc::kMazeSize / 2, y0 = kTop + 22;
            _lcd.fillCircle(x0 + _maze.x() * cell + cell / 2, y0 + _maze.y() * cell + cell / 2, 6, kBackground);
            const uint32_t exits = _maze.exits();
            _maze.step(sample);
            _lcd.fillRect(x0 + (rc::kMazeSize - 1) * cell + 6, y0 + (rc::kMazeSize - 1) * cell + 6, cell - 11, cell - 11, kTeal);
            _lcd.fillCircle(x0 + _maze.x() * cell + cell / 2, y0 + _maze.y() * cell + cell / 2, 6, kCyan);
            if (_maze.exits() != exits || _maze.steps() == 1) {
                char text[24];
                snprintf(text, sizeof(text), "exits %lu  ", static_cast<unsigned long>(_maze.exits()));
                _lcd.fillRect(150, kTop + 2, 90, 14, kBackground);
                label(text, 170, kTop + 2, kTextDim, kFontSmall);
            }
            break;
        }
        default:
            break;
    }
}

// Called from poll(): the picture moves about twelve times a second,
// whatever the sampling is doing, and the status line once a second.
void Ui::animateMind() {
    // Also while the backlight is dimmed: a pattern picture must take every
    // window, or it would not be the picture a replay gives.
    if (_sub != Sub::MindRun) return;
    const uint32_t now = millis();
    const bool newWindow = _adv.mind.input.windows != _vizWindows;
    if (now - _vizFrameMs < 80 && !newWindow) return;
    vizFrame(true);
    if (now - _vizLineMs >= 1000) {
        _vizLineMs = now;
        mindStatusLine();
    }
}

}  // namespace gui
