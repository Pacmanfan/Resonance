#include "ui.h"

#include <Arduino.h>

#include "../version.h"
#include "theme.h"

namespace gui {

namespace {

using namespace theme;

const char* const kTabNames[] = {"HOME", "FREQ", "SOUND", "SESS", "SET"};
const char* const kTabTitles[] = {"RESONANCE", "FREQUENCY STUDIO", "SOUND LIBRARY", "SESSIONS", "SETTINGS"};
constexpr int16_t kTabW = kScreenW / 5;

const lgfx::IFont* const kFontSmall = &fonts::DejaVu9;
const lgfx::IFont* const kFontText = &fonts::DejaVu12;
const lgfx::IFont* const kFontTitle = &fonts::DejaVu18;
const lgfx::IFont* const kFontValue = &fonts::DejaVu24;

constexpr uint8_t kDimPercent = 5;

// Button ids. Frequency screen: 0-3 primary, 4-7 beat. Settings screen: 10+.
constexpr uint8_t kIdBrightnessDown = 10;
constexpr uint8_t kIdBrightnessUp = 11;
constexpr uint8_t kIdMode = 12;
constexpr uint8_t kIdAudio = 20;

const float kPrimarySteps[] = {-10.0f, -1.0f, 1.0f, 10.0f};
const float kBeatSteps[] = {-1.0f, -0.1f, 0.1f, 1.0f};

}  // namespace

Ui::Ui(Display& display, rc::Settings& settings) : _lcd(display), _settings(settings) {}

void Ui::begin() {
    _lastActivityMs = millis();
    drawFrame();
}

void Ui::setStatus(const Status& status) {
    _status = status;
    // Only the screens that show status are redrawn, and only their content.
    if (_dimmed) return;
    if (_tab == Tab::Home || _tab == Tab::Sound) drawContent();
    else if (_tab == Tab::Settings) drawSettingsValues();
}

Event Ui::poll() {
    int32_t x = 0, y = 0;
    const bool down = _lcd.getTouch(&x, &y) > 0;
    if (down && !_touching) {
        ++_touchCount;
        _lastX = static_cast<int16_t>(x);
        _lastY = static_cast<int16_t>(y);
        if (_dimmed) {
            wake();  // the first touch only wakes the screen
        } else {
            _lastActivityMs = millis();
            onPress(_lastX, _lastY);
        }
    }
    _touching = down;

    const uint32_t timeoutMs = static_cast<uint32_t>(_settings.screenTimeoutS) * 1000u;
    if (!_dimmed && timeoutMs != 0 && millis() - _lastActivityMs > timeoutMs) {
        _dimmed = true;
        _modeArmed = false;
        _lcd.setBacklightPercent(kDimPercent);
    }

    const Event e = _pending;
    _pending = Event::None;
    return e;
}

void Ui::wake() {
    _dimmed = false;
    _lastActivityMs = millis();
    _lcd.setBacklightPercent(_settings.brightness);
    drawContent();
}

// ---- drawing helpers -------------------------------------------------------

void Ui::label(const char* text, int16_t x, int16_t y, uint16_t color, const lgfx::IFont* font) {
    _lcd.setFont(font);
    _lcd.setTextDatum(textdatum_t::top_left);
    _lcd.setTextColor(color, kBackground);
    _lcd.drawString(text, x, y);
}

void Ui::centered(const char* text, int16_t cx, int16_t cy, uint16_t color, uint16_t bg, const lgfx::IFont* font) {
    _lcd.setFont(font);
    _lcd.setTextDatum(textdatum_t::middle_center);
    _lcd.setTextColor(color, bg);
    _lcd.drawString(text, cx, cy);
}

void Ui::addButton(int16_t x, int16_t y, int16_t w, int16_t h, const char* text, uint8_t id) {
    if (_buttonCount >= kMaxButtons) return;
    _buttons[_buttonCount] = {x, y, w, h, text, id};
    drawButton(_buttons[_buttonCount], false);
    ++_buttonCount;
}

void Ui::drawButton(const Button& b, bool pressed) {
    const uint16_t fill = pressed ? kPanelPressed : kPanel;
    _lcd.fillRoundRect(b.x, b.y, b.w, b.h, 6, fill);
    _lcd.drawRoundRect(b.x, b.y, b.w, b.h, 6, kOutline);
    centered(b.label, b.x + b.w / 2, b.y + b.h / 2, kText, fill, kFontText);
}

void Ui::drawFrame() {
    _lcd.fillScreen(kBackground);
    drawHeader();
    drawTabBar();
    drawContent();
}

void Ui::drawHeader() {
    _lcd.fillRect(0, 0, kScreenW, kHeaderH, kBackground);
    _lcd.setFont(_tab == Tab::Home ? kFontTitle : kFontText);
    _lcd.setTextDatum(textdatum_t::middle_left);
    _lcd.setTextColor(kCyan, kBackground);
    _lcd.drawString(kTabTitles[static_cast<uint8_t>(_tab)], 10, kHeaderH / 2);
    // The operating mode is always visible (ADR-000204).
    const bool setup = _settings.mode == rc::Mode::Setup;
    _lcd.setFont(kFontSmall);
    _lcd.setTextDatum(textdatum_t::middle_right);
    _lcd.setTextColor(setup ? kWarning : kTeal, kBackground);
    _lcd.drawString(setup ? "SETUP" : "PLAY", kScreenW - 8, kHeaderH / 2);
    _lcd.drawFastHLine(0, kHeaderH - 1, kScreenW, kOutline);
}

void Ui::drawTabBar() {
    const int16_t top = kContentBottom;
    _lcd.fillRect(0, top, kScreenW, kTabBarH, kPanel);
    for (uint8_t i = 0; i < static_cast<uint8_t>(Tab::Count); ++i) {
        const bool active = i == static_cast<uint8_t>(_tab);
        const int16_t x = i * kTabW;
        if (active) _lcd.fillRect(x + 6, top, kTabW - 12, 3, kCyan);
        centered(kTabNames[i], x + kTabW / 2, top + kTabBarH / 2 + 2, active ? kCyan : kTextDim, kPanel, kFontSmall);
    }
}

void Ui::drawContent() {
    _buttonCount = 0;
    _lcd.fillRect(0, kContentTop, kScreenW, kContentBottom - kContentTop, kBackground);
    switch (_tab) {
        case Tab::Home: drawHome(); break;
        case Tab::Frequency: drawFrequency(); break;
        case Tab::Sound: drawSound(); break;
        case Tab::Sessions: drawSessions(); break;
        case Tab::Settings: drawSettings(); break;
        default: break;
    }
}

// ---- screens ---------------------------------------------------------------

void Ui::drawHome() {
    char text[40];
    const int16_t cx = kScreenW / 2, cy = kContentTop + 62;
    // Concentric rings: the still form of the frequency visualisation.
    _lcd.drawCircle(cx, cy, 54, kViolet);
    _lcd.drawCircle(cx, cy, 53, kViolet);
    _lcd.drawCircle(cx, cy, 44, kTeal);
    _lcd.drawCircle(cx, cy, 43, kTeal);
    _lcd.drawCircle(cx, cy, 34, kCyan);
    snprintf(text, sizeof(text), "%.1f", static_cast<double>(_settings.beatHz));
    centered(text, cx, cy - 4, kText, kBackground, kFontValue);
    centered("Hz beat", cx, cy + 16, kTextDim, kBackground, kFontSmall);

    int16_t y = kContentTop + 126;
    snprintf(text, sizeof(text), "L %.1f Hz", static_cast<double>(_settings.primaryHz));
    centered(text, 60, y, kCyan, kBackground, kFontText);
    snprintf(text, sizeof(text), "R %.1f Hz", static_cast<double>(_settings.primaryHz + _settings.beatHz));
    centered(text, 180, y, kViolet, kBackground, kFontText);

    // Audio is off until the user turns it on (RES-CFG-003, RES-GUI-003).
    addButton(40, kContentTop + 142, 160, 36, "AUDIO OFF", kIdAudio);

    y = kContentTop + 188;
    if (_settings.mode == rc::Mode::Play) {
        label(_status.bluetoothRunning ? "Bluetooth: ready" : "Bluetooth: off", 12, y, kTextDim, kFontSmall);
    } else {
        snprintf(text, sizeof(text), "Wi-Fi: %s", _status.address[0] ? _status.address : "starting");
        label(text, 12, y, kTextDim, kFontSmall);
    }
    if (_status.sdMounted) snprintf(text, sizeof(text), "SD: %lu MB free", static_cast<unsigned long>(_status.sdFreeMb));
    else snprintf(text, sizeof(text), "SD: no card");
    label(text, 12, y + 14, _status.sdMounted ? kTextDim : kWarning, kFontSmall);
    if (_status.radioBlocked) {
        label("Radio off: power dropped at start", 12, y + 28, kWarning, kFontSmall);
    } else if (_status.unexpectedReset) {
        snprintf(text, sizeof(text), "Restarted: %s", _status.resetReason);
        label(text, 12, y + 28, kWarning, kFontSmall);
    }
}

void Ui::drawFrequency() {
    static const char* const kPrimaryLabels[] = {"-10", "-1", "+1", "+10"};
    static const char* const kBeatLabels[] = {"-1", "-0.1", "+0.1", "+1"};
    label("Primary (left ear)", 12, kContentTop + 8, kTextDim, kFontSmall);
    label("Binaural beat", 12, kContentTop + 118, kTextDim, kFontSmall);
    for (uint8_t i = 0; i < 4; ++i) {
        const int16_t x = 8 + i * 57;
        addButton(x, kContentTop + 62, 53, 40, kPrimaryLabels[i], i);
        addButton(x, kContentTop + 172, 53, 40, kBeatLabels[i], static_cast<uint8_t>(4 + i));
    }
    drawFrequencyValues();
}

void Ui::drawFrequencyValues() {
    char text[24];
    _lcd.fillRect(0, kContentTop + 22, kScreenW, 36, kBackground);
    _lcd.fillRect(0, kContentTop + 132, kScreenW, 36, kBackground);
    snprintf(text, sizeof(text), "%.1f Hz", static_cast<double>(_settings.primaryHz));
    centered(text, kScreenW / 2, kContentTop + 40, kCyan, kBackground, kFontValue);
    snprintf(text, sizeof(text), "%.1f Hz", static_cast<double>(_settings.beatHz));
    centered(text, kScreenW / 2, kContentTop + 150, kViolet, kBackground, kFontValue);
}

void Ui::drawSound() {
    char text[40];
    if (_status.sdMounted) {
        label("microSD card", 12, kContentTop + 12, kTextDim, kFontSmall);
        snprintf(text, sizeof(text), "%lu MB free of %lu MB", static_cast<unsigned long>(_status.sdFreeMb),
                 static_cast<unsigned long>(_status.sdTotalMb));
        label(text, 12, kContentTop + 28, kText, kFontText);
    } else {
        label("No microSD card", 12, kContentTop + 12, kWarning, kFontText);
        label("Insert a card to play sound files.", 12, kContentTop + 32, kTextDim, kFontSmall);
    }
    label("Playback is not built yet.", 12, kContentTop + 70, kTextDim, kFontSmall);
}

void Ui::drawSessions() {
    label("Sessions are not built yet.", 12, kContentTop + 12, kTextDim, kFontSmall);
}

void Ui::drawSettings() {
    label("Brightness", 12, kContentTop + 10, kTextDim, kFontSmall);
    addButton(8, kContentTop + 26, 56, 38, "-", kIdBrightnessDown);
    addButton(176, kContentTop + 26, 56, 38, "+", kIdBrightnessUp);

    label("Operating mode", 12, kContentTop + 78, kTextDim, kFontSmall);
    const bool setup = _settings.mode == rc::Mode::Setup;
    const char* text = _modeArmed ? "Tap again to restart" : (setup ? "Restart in PLAY mode" : "Restart in SETUP mode");
    addButton(8, kContentTop + 94, 224, 38, text, kIdMode);
    drawSettingsValues();
}

void Ui::drawSettingsValues() {
    char text[48];
    snprintf(text, sizeof(text), " %u%% ", static_cast<unsigned>(_settings.brightness));
    centered(text, kScreenW / 2, kContentTop + 45, kText, kBackground, kFontTitle);

    int16_t y = kContentTop + 142;
    _lcd.fillRect(0, y, kScreenW, kContentBottom - y, kBackground);
    if (_settings.mode == rc::Mode::Setup) {
        snprintf(text, sizeof(text), "Network: %s", _status.network);
        label(text, 12, y, kText, kFontSmall);
        if (_status.wifiState == 3) {  // access point: show how to join it
            snprintf(text, sizeof(text), "Password: %s", _settings.apPass);
            label(text, 12, y + 14, kText, kFontSmall);
        }
        snprintf(text, sizeof(text), "Address: %s", _status.address);
        label(text, 12, y + 28, kText, kFontSmall);
    } else {
        label("Wi-Fi is off in PLAY mode.", 12, y, kTextDim, kFontSmall);
    }
    snprintf(text, sizeof(text), "Firmware %s", RESONANCE_VERSION);
    label(text, 12, y + 48, kTextDim, kFontSmall);
    snprintf(text, sizeof(text), "Free memory %lu kB", static_cast<unsigned long>(_status.freeHeap / 1024));
    label(text, 12, y + 62, kTextDim, kFontSmall);
}

// ---- input -----------------------------------------------------------------

void Ui::onPress(int16_t x, int16_t y) {
    if (y >= kContentBottom) {
        uint8_t index = static_cast<uint8_t>(x / kTabW);
        if (index >= static_cast<uint8_t>(Tab::Count)) index = static_cast<uint8_t>(Tab::Count) - 1;
        selectTab(static_cast<Tab>(index));
        return;
    }
    for (uint8_t i = 0; i < _buttonCount; ++i) {
        const Button& b = _buttons[i];
        if (x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h) {
            const uint8_t id = b.id;
            drawButton(b, true);
            delay(60);  // long enough to be seen as a press
            drawButton(_buttons[i], false);
            onButton(id);
            return;
        }
    }
    if (_modeArmed) {  // a touch elsewhere cancels the pending restart
        _modeArmed = false;
        drawContent();
    }
}

void Ui::onButton(uint8_t id) {
    if (id < 4) {
        _settings.primaryHz = rc::sanitizeFrequency(_settings.primaryHz + kPrimarySteps[id], rc::kPrimaryMinHz,
                                                    rc::kPrimaryMaxHz, rc::kPrimaryDefaultHz);
        _settingsDirty = true;
        drawFrequencyValues();
    } else if (id < 8) {
        _settings.beatHz = rc::sanitizeFrequency(_settings.beatHz + kBeatSteps[id - 4], rc::kBeatMinHz,
                                                 rc::kBeatMaxHz, rc::kBeatDefaultHz);
        _settingsDirty = true;
        drawFrequencyValues();
    } else if (id == kIdBrightnessDown || id == kIdBrightnessUp) {
        int value = _settings.brightness + (id == kIdBrightnessUp ? 10 : -10);
        if (value < rc::kBrightnessMin) value = rc::kBrightnessMin;
        if (value > rc::kBrightnessMax) value = rc::kBrightnessMax;
        _settings.brightness = static_cast<uint8_t>(value);
        _settingsDirty = true;
        _lcd.setBacklightPercent(_settings.brightness);
        drawSettingsValues();
    } else if (id == kIdMode) {
        if (_modeArmed) {
            _pending = Event::SwitchMode;
        } else {
            _modeArmed = true;
            drawContent();
        }
    } else if (id == kIdAudio) {
        // The audio engine is not built yet; the control says so rather than
        // pretending to work.
        centered(" Audio is not built yet ", kScreenW / 2, kContentTop + 160, kWarning, kBackground, kFontSmall);
    }
}

void Ui::selectTab(Tab tab) {
    if (tab == _tab) return;
    _tab = tab;
    _modeArmed = false;
    // Edits are stored when the user leaves the screen, not on every tap, to
    // spare the flash.
    if (_settingsDirty) {
        _settingsDirty = false;
        _pending = Event::SettingsChanged;
    }
    drawHeader();
    drawTabBar();
    drawContent();
}

}  // namespace gui
