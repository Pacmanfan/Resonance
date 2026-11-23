#include "ui.h"

#include <Arduino.h>

#include "../version.h"
#include "rc_presets.h"
#include "theme.h"

namespace gui {

namespace {

using namespace theme;

const char* const kTabNames[] = {"HOME", "FREQ", "SND", "SESS", "SET"};
const char* const kTabTitles[] = {"RESONANCE", "FREQUENCY STUDIO", "SOUND LIBRARY", "SESSIONS", "SETTINGS"};
constexpr int16_t kTabW = kScreenW / 5;

const lgfx::IFont* const kFontSmall = &fonts::DejaVu12;
const lgfx::IFont* const kFontText = &fonts::DejaVu18;
const lgfx::IFont* const kFontTitle = &fonts::DejaVu18;
const lgfx::IFont* const kFontValue = &fonts::DejaVu24;

constexpr uint8_t kDimPercent = 5;

// Button ids. Frequency screen: 0-3 primary, 4-7 beat. Settings screen: 10+.
constexpr uint8_t kIdBrightnessDown = 10;
constexpr uint8_t kIdBrightnessUp = 11;
constexpr uint8_t kIdAdvanced = 14;
constexpr uint8_t kIdBuilder = 15;
constexpr uint8_t kIdAudio = 20;
constexpr uint8_t kIdVolumeDown = 21;
constexpr uint8_t kIdVolumeUp = 22;
constexpr uint8_t kIdBluetooth = 23;
constexpr uint8_t kIdBtDevice = 30;      // 30..34: found devices
constexpr uint8_t kIdBtDisconnect = 40;
constexpr uint8_t kIdSoundEntry = 50;    // 50..53: listed entries
constexpr uint8_t kIdSoundUp = 60;
constexpr uint8_t kIdSoundPrev = 61;
constexpr uint8_t kIdSoundNext = 62;
constexpr uint8_t kIdSoundPlay = 63;
constexpr uint8_t kIdSoundVolDown = 64;
constexpr uint8_t kIdSoundVolUp = 65;
constexpr uint8_t kIdNoise = 70;
constexpr uint8_t kIdNoiseDown = 71;
constexpr uint8_t kIdNoiseUp = 72;
constexpr uint8_t kIdPrimaryPreset = 90;  // the value itself: tap to step through the presets
constexpr uint8_t kIdBeatPreset = 91;
constexpr uint8_t kIdPrimaryPrev = 92;
constexpr uint8_t kIdPrimaryNext = 93;
constexpr uint8_t kIdBeatPrev = 94;
constexpr uint8_t kIdBeatNext = 95;
constexpr uint8_t kIdSessionProgram = 80;
constexpr uint8_t kIdSessionDuration = 81;
constexpr uint8_t kIdSessionFade = 82;
constexpr uint8_t kIdSessionStart = 83;
constexpr uint8_t kIdSessionPause = 84;
constexpr uint8_t kIdSessionStop = 85;

// Values of rc::SessionState.
constexpr uint8_t kSessionIdle = 0;
constexpr uint8_t kSessionRunning = 1;
constexpr uint8_t kSessionPaused = 2;
constexpr uint8_t kSessionFading = 3;

// Values of player::State.
constexpr uint8_t kPlayerPlaying = 1;
constexpr uint8_t kPlayerPaused = 2;
constexpr uint8_t kPlayerError = 3;

const char* const kNoiseNames[] = {"OFF", "WHITE", "PINK", "BROWN"};

// Values of btaudio::State, which this file does not include.
constexpr uint8_t kBtScanning = 1;
constexpr uint8_t kBtConnecting = 2;
constexpr uint8_t kBtConnected = 3;

const float kPrimarySteps[] = {-10.0f, -1.0f, 1.0f, 10.0f};
const float kBeatSteps[] = {-1.0f, -0.1f, 0.1f, 1.0f};

}  // namespace

Ui::Ui(Display& display, rc::Settings& settings, Advanced& advanced)
    : _lcd(display), _settings(settings), _adv(advanced) {}

void Ui::begin() {
    _lastActivityMs = millis();
    drawFrame();
}

void Ui::setStatus(const Status& status) {
    // Nothing is redrawn unless something shown has changed, and then only
    // the lines that show it: a periodic full redraw is visible as flicker.
    const bool sdChanged = status.sdMounted != _status.sdMounted || status.sdFreeMb != _status.sdFreeMb ||
                           status.sdTotalMb != _status.sdTotalMb;
    const bool audioChanged = status.audioOn != _status.audioOn || status.volume != _status.volume ||
                              status.btState != _status.btState || status.btCount != _status.btCount ||
                              strcmp(status.btPeer, _status.btPeer) != 0;
    const bool soundChanged = status.listVersion != _status.listVersion || status.playerState != _status.playerState ||
                              status.backgroundVolume != _status.backgroundVolume ||
                              strcmp(status.trackName, _status.trackName) != 0;
    const bool positionChanged = status.trackPositionS != _status.trackPositionS;
    const bool noiseChanged = status.noiseType != _status.noiseType || status.noiseVolume != _status.noiseVolume;
    const bool sessionChanged = status.sessionState != _status.sessionState ||
                                strcmp(status.sessionName, _status.sessionName) != 0 ||
                                status.sessionMinutes != _status.sessionMinutes ||
                                status.sessionFadeS != _status.sessionFadeS;
    const bool sessionTick = status.sessionRemainingS != _status.sessionRemainingS ||
                             status.sessionStage != _status.sessionStage;
    const bool tonePushed = status.sessionState != kSessionIdle && sessionTick;
    const bool radioChanged = status.bluetoothRunning != _status.bluetoothRunning ||
                              status.radioBlocked != _status.radioBlocked;
    const bool mixerChanged = noiseChanged || status.backgroundVolume != _status.backgroundVolume;
    _status = status;
    if (_dimmed) return;
    if (_sub != Sub::None) {
        // Of the advanced screens only the mixer shows something of the status.
        if (_sub == Sub::Mixer && mixerChanged) redrawValues();
        return;
    }
    if (_tab == Tab::Home && audioChanged) drawContent();
    else if (_tab == Tab::Home && !_btScreen && (sdChanged || radioChanged)) drawHomeStatus();
    else if (_tab == Tab::Sound && (sdChanged || soundChanged)) drawContent();
    else if (_tab == Tab::Sound && positionChanged) drawSoundStatus();
    else if (_tab == Tab::Frequency && noiseChanged) drawContent();
    else if (_tab == Tab::Frequency && tonePushed) drawFrequencyValues();  // a session is moving the frequencies
    else if (_tab == Tab::Sessions && sessionChanged) drawContent();
    else if (_tab == Tab::Sessions && sessionTick) drawSessionStatus();
    else if (_tab == Tab::Home && !_btScreen && (sessionChanged || sessionTick)) drawHomeStatus();
    // The free-memory figure on Settings is as of opening the screen; it
    // jitters, and following it would bring the flicker back.
    else if (_tab == Tab::Settings && radioChanged) drawSettingsValues();
}

Event Ui::poll() {
    int32_t x = 0, y = 0;
    const bool down = _lcd.getTouch(&x, &y) > 0;
    if (down && !_touching) {
        ++_touchCount;
        _lastX = static_cast<int16_t>(x);
        _lastY = static_cast<int16_t>(y);
#if RESONANCE_TOUCH_DEBUG
        // Shows where the press was registered, on the screen and on serial.
        int32_t rawX = -1, rawY = -1;
        _lcd.rawTouch(&rawX, &rawY);
        Serial.printf("TOUCH %d,%d (raw %ld,%ld)\n", _lastX, _lastY, static_cast<long>(rawX), static_cast<long>(rawY));
        _lcd.fillCircle(_lastX, _lastY, 4, kWarning);
#endif
        if (_dimmed) {
            wake();  // the first touch only wakes the screen
        } else {
            _lastActivityMs = millis();
            onPress(_lastX, _lastY);
        }
    }
    // Tap-and-hold: while the finger stays on an adjustment button, its
    // action repeats after a pause (rc_repeat.h).
    if (!down) {
        _repeat.release();
        _heldId = 0xFF;
    } else if (_touching && _heldId != 0xFF) {
        bool stillOn = false;
        for (uint8_t i = 0; i < _buttonCount; ++i) {
            const Button& b = _buttons[i];
            if (b.id == _heldId && x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h) stillOn = true;
        }
        if (!stillOn) {
            _repeat.release();  // the finger slid off: no more repeats for this press
            _heldId = 0xFF;
        } else if (_repeat.due(millis())) {
            _lastActivityMs = millis();
            onButton(_heldId);
        }
    }
    _touching = down;
    animateHome();
    animateBreath();

    const uint32_t timeoutMs = static_cast<uint32_t>(_settings.screenTimeoutS) * 1000u;
    if (!_dimmed && timeoutMs != 0 && millis() - _lastActivityMs > timeoutMs) {
        _dimmed = true;
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
    _lcd.drawRoundRect(b.x + 1, b.y + 1, b.w - 2, b.h - 2, 5, kOutline);
    // A label too wide for its button is drawn in the small font.
    _lcd.setFont(kFontText);
    // Two pixels of air each side are enough inside the outline; a wider
    // margin made "+0.1" alone fall back to the small font in its row.
    const lgfx::IFont* font = _lcd.textWidth(b.label) > b.w - 4 ? kFontSmall : kFontText;
    centered(b.label, b.x + b.w / 2, b.y + b.h / 2, pressed ? kBackground : kText, fill, font);
}

void Ui::drawFrame() {
    _lcd.fillScreen(kBackground);
    drawHeader();
    drawTabBar();
    drawContent();
}

void Ui::drawHeader() {
    _lcd.fillRect(0, 0, kScreenW, kHeaderH, kBackground);
    // Every screen's title is centred, in the same font.
    const char* title = _btScreen ? "BLUETOOTH" : (_sub != Sub::None ? subTitle() : kTabTitles[static_cast<uint8_t>(_tab)]);
    centered(title, kScreenW / 2, kHeaderH / 2 - 1, kCyan, kBackground, kFontTitle);
    // An advanced screen: the header leads back one level.
    if (_sub != Sub::None) label("<", 8, 6, kText, kFontTitle);
    _lcd.fillRect(0, kHeaderH - 2, kScreenW, 2, kOutline);
}

void Ui::drawTabBar() {
    // Each tab is drawn as a button: a filled, outlined rounded rectangle.
    // The open tab is filled with the highlight colour and dark text.
    const int16_t top = kContentBottom;
    _lcd.fillRect(0, top, kScreenW, kTabBarH, kBackground);
    for (uint8_t i = 0; i < static_cast<uint8_t>(Tab::Count); ++i) {
        const bool active = i == static_cast<uint8_t>(_tab) && !_btScreen && _sub == Sub::None;
        const int16_t x = static_cast<int16_t>(i * kTabW + 2);
        const int16_t y = static_cast<int16_t>(top + 4);
        const int16_t w = kTabW - 4;
        const int16_t h = kTabBarH - 7;
        const uint16_t fill = active ? kCyan : kPanel;
        _lcd.fillRoundRect(x, y, w, h, 7, fill);
        _lcd.drawRoundRect(x, y, w, h, 7, kOutline);
        _lcd.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 6, kOutline);
        centered(kTabNames[i], x + w / 2, y + h / 2, active ? kBackground : kText, fill, kFontSmall);
    }
}

void Ui::drawContent() {
    _buttonCount = 0;
    _lcd.fillRect(0, kContentTop, kScreenW, kContentBottom - kContentTop, kBackground);
    if (_sub != Sub::None) {
        drawAdvanced();
        return;
    }
    switch (_tab) {
        case Tab::Home:
            if (_btScreen) drawBluetooth(); else drawHome();
            break;
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
    _homeBar = 0;  // the content was cleared: the breathing bar is drawn afresh
    const int16_t cx = kScreenW / 2, cy = kContentTop + 62;
    // Concentric rings. They rest while the audio is off and pulse at the
    // beat frequency while it is on (animateHome).
    if (!_status.audioOn) {
        _pulse.reset();
        for (uint8_t i = 0; i < rc::kPulseRings; ++i) _ringOffset[i] = 0;
    }
    for (uint8_t i = 0; i < rc::kPulseRings; ++i) drawRing(i, _ringOffset[i], false);
    snprintf(text, sizeof(text), "%.1f", static_cast<double>(_settings.beatHz));
    centered(text, cx, cy - 4, kText, kBackground, kFontValue);
    centered("Hz beat", cx, cy + 18, kTextDim, kBackground, kFontSmall);

    int16_t y = kContentTop + 126;
    snprintf(text, sizeof(text), "L %.1f Hz", static_cast<double>(_settings.primaryHz));
    centered(text, 60, y, kCyan, kBackground, kFontSmall);
    snprintf(text, sizeof(text), "R %.1f Hz", static_cast<double>(_settings.primaryHz + _settings.beatHz));
    centered(text, 180, y, kViolet, kBackground, kFontSmall);

    // Audio is off until the user turns it on (RES-CFG-003, RES-GUI-003).
    // A level is shown only while the audio is on.
    if (_status.audioOn) snprintf(_audioLabel, sizeof(_audioLabel), "ON %u%%", static_cast<unsigned>(_status.volume));
    else snprintf(_audioLabel, sizeof(_audioLabel), "OFF");
    addButton(8, kContentTop + 142, 46, 36, "-", kIdVolumeDown);
    addButton(58, kContentTop + 142, 124, 36, _audioLabel, kIdAudio);
    addButton(186, kContentTop + 142, 46, 36, "+", kIdVolumeUp);
    if (_status.btState == kBtConnected) {
        snprintf(_btLabel, sizeof(_btLabel), "BT: %s", _status.btPeer[0] ? _status.btPeer : "connected");
    } else if (_status.btState == kBtConnecting) {
        snprintf(_btLabel, sizeof(_btLabel), "BT: connecting...");
    } else if (_status.btState == kBtScanning) {
        snprintf(_btLabel, sizeof(_btLabel), "BT: choose device (%u)", static_cast<unsigned>(_status.btCount));
    } else {
        snprintf(_btLabel, sizeof(_btLabel), "BT: off");
    }
    addButton(8, kContentTop + 184, 224, 32, _btLabel, kIdBluetooth);
    drawHomeStatus();
}

// One ring of the home screen, `offset` pixels outside its resting radius;
// with `erase`, in the background colour. The resting radii leave room for
// kPulseTravel between the rings, the number inside and the line below.
void Ui::drawRing(uint8_t ring, uint8_t offset, bool erase) {
    static const int16_t kRadius[rc::kPulseRings] = {34, 42, 50};
    static const uint8_t kWidth[rc::kPulseRings] = {1, 2, 2};
    static const uint16_t kColor[rc::kPulseRings] = {kCyan, kTeal, kViolet};
    const int16_t cx = kScreenW / 2, cy = kContentTop + 62;
    for (uint8_t w = 0; w < kWidth[ring]; ++w) {
        _lcd.drawCircle(cx, cy, kRadius[ring] + offset + w, erase ? kBackground : kColor[ring]);
    }
}

// Moves the rings to where the pulse stands now. Nothing is buffered: a
// ring that moved is erased where it was and drawn where it is, and a ring
// that did not move is left alone, so most passes draw nothing at all.
void Ui::animateHome() {
    if (_dimmed || _tab != Tab::Home || _btScreen || _sub != Sub::None || !_status.audioOn) return;
    if (!_pulse.advance(millis(), _settings.beatHz)) return;
    bool open = false;
    for (uint8_t i = 0; i < rc::kPulseRings; ++i) {
        const uint8_t o = _pulse.offset(i);
        if (o == _ringOffset[i]) continue;
        if (!open) {
            _lcd.startWrite();  // one bus transaction for the whole frame
            open = true;
        }
        drawRing(i, _ringOffset[i], true);
        drawRing(i, o, false);
        _ringOffset[i] = o;
    }
    if (open) _lcd.endWrite();
}

void Ui::drawHomeStatus() {
    char text[40];
    const int16_t y = kContentTop + 220;
    _lcd.fillRect(0, y, kScreenW, kContentBottom - y, kBackground);
    if (_status.radioBlocked) {
        label("Radio off: power dropped", 12, y, kWarning, kFontSmall);
        return;
    }
    if (_status.unexpectedReset) {
        snprintf(text, sizeof(text), "Restarted: %s", _status.resetReason);
        label(text, 12, y, kWarning, kFontSmall);
        return;
    }
    if (_status.sessionState != kSessionIdle) {
        const uint32_t r = _status.sessionRemainingS;
        if (_status.sessionState == kSessionFading) snprintf(text, sizeof(text), "Session: fading out");
        else if (_status.sessionMinutes == 0) snprintf(text, sizeof(text), "Session: running");
        else snprintf(text, sizeof(text), "Session: %lu:%02lu left", static_cast<unsigned long>(r / 60),
                      static_cast<unsigned long>(r % 60));
        label(text, 12, y, kTeal, kFontSmall);
        return;
    }
    if (_status.sdMounted) snprintf(text, sizeof(text), "SD: %lu MB free", static_cast<unsigned long>(_status.sdFreeMb));
    else snprintf(text, sizeof(text), "SD: no card");
    label(text, 12, y, kTextDim, kFontSmall);
}

void Ui::drawBluetooth() {
    int16_t y = kContentTop + 8;
    if (_status.btState == kBtConnected) {
        label("Connected to", 12, y, kTextDim, kFontSmall);
        label(_status.btPeer[0] ? _status.btPeer : "a remembered device", 12, y + 16, kTeal, kFontText);
        addButton(8, y + 48, 224, 38, "Disconnect", kIdBtDisconnect);
        label("Disconnect to choose another", 12, y + 96, kTextDim, kFontSmall);
        label("device from the list.", 12, y + 112, kTextDim, kFontSmall);
        return;
    }
    if (_status.btState == kBtConnecting) {
        label("Connecting to", 12, y, kTextDim, kFontSmall);
        label(_status.btPeer, 12, y + 16, kCyan, kFontText);
        label("Keep it in pairing mode.", 12, y + 44, kTextDim, kFontSmall);
        addButton(8, y + 66, 224, 38, "Cancel", kIdBtDisconnect);
        return;
    }
    if (_status.btState != kBtScanning) {
        label("Bluetooth is off.", 12, y, kWarning, kFontSmall);
        return;
    }
    label(_status.btCount ? "Tap a device to connect" : "Searching... put the headset", 12, y, kTextDim, kFontSmall);
    if (_status.btCount == 0) label("or speaker in pairing mode.", 12, y + 16, kTextDim, kFontSmall);
    y += 22;
    for (uint8_t i = 0; i < _status.btCount && i < 5; ++i) {
        addButton(8, static_cast<int16_t>(y + i * 42), 224, 38, _status.btDevices[i], static_cast<uint8_t>(kIdBtDevice + i));
    }
}

void Ui::drawFrequency() {
    static const char* const kPrimaryLabels[] = {"-10", "-1", "+1", "+10"};
    static const char* const kBeatLabels[] = {"-1", "-0.1", "+0.1", "+1"};
    // Presets: "<" and ">" step back and forward; a tap on the value itself
    // also steps forward.
    addButton(8, kContentTop + 18, 42, 28, "<", kIdPrimaryPrev);
    addButton(54, kContentTop + 18, 132, 28, _primaryLabel, kIdPrimaryPreset);
    addButton(190, kContentTop + 18, 42, 28, ">", kIdPrimaryNext);
    addButton(8, kContentTop + 102, 42, 28, "<", kIdBeatPrev);
    addButton(54, kContentTop + 102, 132, 28, _beatLabel, kIdBeatPreset);
    addButton(190, kContentTop + 102, 42, 28, ">", kIdBeatNext);
    for (uint8_t i = 0; i < 4; ++i) {
        const int16_t x = 8 + i * 57;
        addButton(x, kContentTop + 48, 53, 34, kPrimaryLabels[i], i);
        addButton(x, kContentTop + 132, 53, 34, kBeatLabels[i], static_cast<uint8_t>(4 + i));
    }
    // The volume is shown beside the word, not inside the button: with it
    // the longer type names did not fit and were drawn in a smaller font
    // than the others.
    char noiseLine[20];
    // A level is shown only for something that is on.
    if (_status.noiseType == 0) snprintf(noiseLine, sizeof(noiseLine), "Noise");
    else snprintf(noiseLine, sizeof(noiseLine), "Noise  %u%%", static_cast<unsigned>(_status.noiseVolume));
    label(noiseLine, 12, kContentTop + 174, kTextDim, kFontSmall);
    snprintf(_noiseLabel, sizeof(_noiseLabel), "%s", kNoiseNames[_status.noiseType & 3]);
    addButton(8, kContentTop + 192, 46, 34, "-", kIdNoiseDown);
    addButton(58, kContentTop + 192, 124, 34, _noiseLabel, kIdNoise);
    addButton(186, kContentTop + 192, 46, 34, "+", kIdNoiseUp);
    drawFrequencyValues();
}

void Ui::redrawButton(uint8_t id) {
    for (uint8_t i = 0; i < _buttonCount; ++i) {
        if (_buttons[i].id == id) drawButton(_buttons[i], false);
    }
}

// The two values and, above each, the name of the preset it is on.
void Ui::drawFrequencyValues() {
    char text[32];
    snprintf(_primaryLabel, sizeof(_primaryLabel), "%.1f Hz", static_cast<double>(_settings.primaryHz));
    snprintf(_beatLabel, sizeof(_beatLabel), "%.1f Hz", static_cast<double>(_settings.beatHz));
    redrawButton(kIdPrimaryPreset);
    redrawButton(kIdBeatPreset);

    _lcd.fillRect(0, kContentTop + 2, kScreenW, 15, kBackground);
    const char* primaryName = rc::primaryPresetName(_settings.primaryHz);
    if (primaryName != nullptr) snprintf(text, sizeof(text), "Primary: %s", primaryName);
    else snprintf(text, sizeof(text), "Primary (left ear)");
    label(text, 12, kContentTop + 3, primaryName != nullptr ? kCyan : kTextDim, kFontSmall);

    _lcd.fillRect(0, kContentTop + 86, kScreenW, 15, kBackground);
    const char* beatName = rc::beatPresetName(_settings.beatHz);
    if (beatName != nullptr) snprintf(text, sizeof(text), "Beat: %s", beatName);
    else snprintf(text, sizeof(text), "Binaural beat");
    label(text, 12, kContentTop + 87, beatName != nullptr ? kViolet : kTextDim, kFontSmall);
}

void Ui::drawSound() {
    if (!_status.sdMounted) {
        label("No microSD card", 12, kContentTop + 12, kWarning, kFontText);
        label("Insert a card for sound files.", 12, kContentTop + 38, kTextDim, kFontSmall);
        return;
    }
    // The folder, then up to four entries of it.
    label(_status.soundDir, 8, kContentTop + 3, kTeal, kFontSmall);
    if (_status.soundCount == 0) {
        label("No sound files here.", 12, kContentTop + 28, kTextDim, kFontSmall);
        label("Copy .wav or .mp3 files to", 12, kContentTop + 46, kTextDim, kFontSmall);
        label("RESONANCE/AUDIO on the card.", 12, kContentTop + 62, kTextDim, kFontSmall);
    }
    for (uint8_t i = 0; i < _status.soundCount && i < 4; ++i) {
        snprintf(_entryLabels[i], sizeof(_entryLabels[i]), "%s%s", _status.soundIsDir[i] ? "> " : "",
                 _status.soundEntries[i]);
        // Every entry is drawn in the same font. A file is shown without
        // its extension, and a name still too wide for the button is cut
        // and ends in two dots. (Names used to be cut to a fixed number of
        // characters; those with wide letters then fell back to the small
        // font, one entry in a list of otherwise equal ones.)
        char* text = _entryLabels[i];
        if (!_status.soundIsDir[i]) {
            char* dot = strrchr(text, '.');
            if (dot != nullptr && dot != text) *dot = '\0';
        }
        _lcd.setFont(kFontText);
        size_t length = strlen(text);
        while (length > 3 && _lcd.textWidth(text) > 232 - 8) {
            --length;
            text[length] = '\0';
            text[length - 1] = '.';
            text[length - 2] = '.';
        }
        addButton(4, static_cast<int16_t>(kContentTop + 19 + i * 35), 232, 32, _entryLabels[i],
                  static_cast<uint8_t>(kIdSoundEntry + i));
    }
    const int16_t navY = kContentTop + 161;
    // Previous page on the left, next page on the right, parent folder
    // between them: each keeps its place whichever of the others is shown.
    if (_status.soundHasPrev) addButton(4, navY, 74, 30, "<", kIdSoundPrev);
    if (!_status.soundAtRoot) addButton(83, navY, 74, 30, "UP", kIdSoundUp);
    if (_status.soundHasMore) addButton(162, navY, 74, 30, ">", kIdSoundNext);
    // Transport and the background layer's volume.
    const char* action = _status.playerState == kPlayerPlaying ? "PAUSE" : "PLAY";
    snprintf(_playLabel, sizeof(_playLabel), "%s %u%%", action, static_cast<unsigned>(_status.backgroundVolume));
    const int16_t playY = kContentTop + 194;
    addButton(4, playY, 46, 30, "-", kIdSoundVolDown);
    addButton(54, playY, 132, 30, _playLabel, kIdSoundPlay);
    addButton(190, playY, 46, 30, "+", kIdSoundVolUp);
    drawSoundStatus();
}

void Ui::drawSoundStatus() {
    char text[40];
    const int16_t y = kContentTop + 226;
    _lcd.fillRect(0, y, kScreenW, kContentBottom - y, kBackground);
    if (_status.playerState == kPlayerError) {
        label("Cannot play this file", 8, y, kWarning, kFontSmall);
    } else if (_status.trackName[0] != '\0') {
        snprintf(text, sizeof(text), "%u:%02u %s", static_cast<unsigned>(_status.trackPositionS / 60),
                 static_cast<unsigned>(_status.trackPositionS % 60), _status.trackName);
        label(text, 8, y, _status.playerState == kPlayerPaused ? kTextDim : kText, kFontSmall);
    }
}

void Ui::drawSessions() {
    const bool idle = _status.sessionState == kSessionIdle;
    if (idle) {
        // Choose the program, its length (timer only) and the fade at its end.
        label("Program", 12, kContentTop + 4, kTextDim, kFontSmall);
        snprintf(_programLabel, sizeof(_programLabel), "%s", _status.sessionName);
        addButton(8, kContentTop + 20, 224, 34, _programLabel, kIdSessionProgram);
        label("Length", 12, kContentTop + 60, kTextDim, kFontSmall);
        if (_status.sessionMinutes == 0) snprintf(_durationLabel, sizeof(_durationLabel), "Continuous");
        else snprintf(_durationLabel, sizeof(_durationLabel), "%u min", static_cast<unsigned>(_status.sessionMinutes));
        addButton(8, kContentTop + 76, 224, 34, _durationLabel, kIdSessionDuration);
        label("Fade out at the end", 12, kContentTop + 116, kTextDim, kFontSmall);
        if (_status.sessionFadeS == 0) snprintf(_fadeLabel, sizeof(_fadeLabel), "None");
        else if (_status.sessionFadeS < 60) snprintf(_fadeLabel, sizeof(_fadeLabel), "%u s", static_cast<unsigned>(_status.sessionFadeS));
        else snprintf(_fadeLabel, sizeof(_fadeLabel), "%u min", static_cast<unsigned>(_status.sessionFadeS / 60));
        addButton(8, kContentTop + 132, 224, 34, _fadeLabel, kIdSessionFade);
        addButton(8, kContentTop + 186, 146, 44, "START", kIdSessionStart);
        addButton(160, kContentTop + 186, 72, 44, "BUILD", kIdBuilder);
        return;
    }
    label(_status.sessionName, 12, kContentTop + 6, kTeal, kFontText);
    if (_status.sessionState != kSessionFading) {
        addButton(8, kContentTop + 150, 108, 44, _status.sessionState == kSessionPaused ? "RESUME" : "PAUSE",
                  kIdSessionPause);
        addButton(124, kContentTop + 150, 108, 44, "STOP", kIdSessionStop);
    }
    drawSessionStatus();
}

void Ui::drawSessionStatus() {
    if (_status.sessionState == kSessionIdle) return;
    char text[32];
    _lcd.fillRect(0, kContentTop + 36, kScreenW, 108, kBackground);
    const uint32_t r = _status.sessionRemainingS;
    if (_status.sessionState == kSessionFading) snprintf(text, sizeof(text), "Fading");
    else if (_status.sessionMinutes == 0) snprintf(text, sizeof(text), "Running");
    else snprintf(text, sizeof(text), "%lu:%02lu", static_cast<unsigned long>(r / 60), static_cast<unsigned long>(r % 60));
    centered(text, kScreenW / 2, kContentTop + 66, _status.sessionState == kSessionPaused ? kTextDim : kCyan,
             kBackground, &fonts::DejaVu40);
    if (_status.sessionStages > 1) {
        snprintf(text, sizeof(text), "Stage %u of %u", static_cast<unsigned>(_status.sessionStage + 1),
                 static_cast<unsigned>(_status.sessionStages));
        centered(text, kScreenW / 2, kContentTop + 104, kTextDim, kBackground, kFontSmall);
    }
    snprintf(text, sizeof(text), "%.1f Hz beat", static_cast<double>(_settings.beatHz));
    centered(text, kScreenW / 2, kContentTop + 124, kViolet, kBackground, kFontSmall);
}

void Ui::drawSettings() {
    label("Brightness", 12, kContentTop + 10, kTextDim, kFontSmall);
    addButton(8, kContentTop + 26, 56, 38, "-", kIdBrightnessDown);
    addButton(176, kContentTop + 26, 56, 38, "+", kIdBrightnessUp);

    addButton(8, kContentTop + 84, 224, 38, "ADVANCED", kIdAdvanced);
    drawSettingsValues();
}

void Ui::drawSettingsValues() {
    char text[48];
    snprintf(text, sizeof(text), "  %u%%  ", static_cast<unsigned>(_settings.brightness));
    centered(text, kScreenW / 2, kContentTop + 45, kText, kBackground, kFontTitle);

    const int16_t y = kContentTop + 136;
    _lcd.fillRect(0, y, kScreenW, kContentBottom - y, kBackground);
    snprintf(text, sizeof(text), "Firmware %s", RESONANCE_VERSION);
    label(text, 12, y, kTextDim, kFontSmall);
    snprintf(text, sizeof(text), "Free memory %lu kB", static_cast<unsigned long>(_status.freeHeap / 1024));
    label(text, 12, y + 15, kTextDim, kFontSmall);
}

// ---- input -----------------------------------------------------------------

void Ui::onPress(int16_t x, int16_t y) {
    if (y < kHeaderH && _sub != Sub::None) {
        backSub();
        return;
    }
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
            if (repeats(id)) {
                _heldId = id;
                _repeat.press(millis());
            }
            onButton(id);
            return;
        }
    }
}

// Buttons that change a value by a step repeat when held. Buttons that
// switch, start, open or cycle something do not: holding "START" or a tab
// must never act twice.
bool Ui::repeats(uint8_t id) {
    return (id >= 100 && id < 150) ||                            // adjusters of the advanced screens
           id < 8 ||                                             // frequency steps
           id == kIdBrightnessDown || id == kIdBrightnessUp ||
           id == kIdVolumeDown || id == kIdVolumeUp ||
           id == kIdSoundVolDown || id == kIdSoundVolUp ||
           id == kIdNoiseDown || id == kIdNoiseUp ||
           (id >= kIdPrimaryPrev && id <= kIdBeatNext);           // preset "<" and ">"
}

void Ui::onButton(uint8_t id) {
    if (_sub != Sub::None) {
        onAdvancedButton(id);
    } else if (id == kIdAdvanced) {
        openSub(Sub::Menu);
    } else if (id == kIdBuilder) {
        openSub(Sub::Builder);
    } else if (id < 4) {
        _settings.primaryHz = rc::sanitizeFrequency(_settings.primaryHz + kPrimarySteps[id], rc::kPrimaryMinHz,
                                                    rc::kPrimaryMaxHz, rc::kPrimaryDefaultHz);
        _settingsDirty = true;
        _pending = Event::ToneChanged;
        drawFrequencyValues();
    } else if (id < 8) {
        _settings.beatHz = rc::sanitizeFrequency(_settings.beatHz + kBeatSteps[id - 4], rc::kBeatMinHz,
                                                 rc::kBeatMaxHz, rc::kBeatDefaultHz);
        _settingsDirty = true;
        _pending = Event::ToneChanged;
        drawFrequencyValues();
    } else if (id == kIdPrimaryPrev || id == kIdBeatPrev) {
        if (id == kIdPrimaryPrev) _settings.primaryHz = rc::previousPrimaryPreset(_settings.primaryHz).hz;
        else _settings.beatHz = rc::previousBeatPreset(_settings.beatHz).hz;
        _settingsDirty = true;
        _pending = Event::ToneChanged;
        drawFrequencyValues();
    } else if (id == kIdPrimaryPreset || id == kIdPrimaryNext) {
        _settings.primaryHz = rc::nextPrimaryPreset(_settings.primaryHz).hz;
        _settingsDirty = true;
        _pending = Event::ToneChanged;
        drawFrequencyValues();
    } else if (id == kIdBeatPreset || id == kIdBeatNext) {
        _settings.beatHz = rc::nextBeatPreset(_settings.beatHz).hz;
        _settingsDirty = true;
        _pending = Event::ToneChanged;
        drawFrequencyValues();
    } else if (id == kIdBrightnessDown || id == kIdBrightnessUp) {
        int value = _settings.brightness + (id == kIdBrightnessUp ? 10 : -10);
        if (value < rc::kBrightnessMin) value = rc::kBrightnessMin;
        if (value > rc::kBrightnessMax) value = rc::kBrightnessMax;
        _settings.brightness = static_cast<uint8_t>(value);
        _settingsDirty = true;
        _lcd.setBacklightPercent(_settings.brightness);
        drawSettingsValues();
    } else if (id == kIdAudio) {
        _pending = Event::AudioToggle;
    } else if (id == kIdVolumeDown) {
        _pending = Event::VolumeDown;
    } else if (id == kIdVolumeUp) {
        _pending = Event::VolumeUp;
    } else if (id == kIdBluetooth) {
        _btScreen = true;
        drawHeader();
        drawTabBar();
        drawContent();
    } else if (id >= kIdBtDevice && id < kIdBtDevice + 5) {
        _eventArg = static_cast<uint8_t>(id - kIdBtDevice);
        _pending = Event::BluetoothConnect;
    } else if (id == kIdBtDisconnect) {
        _pending = Event::BluetoothDisconnect;
    } else if (id >= kIdSoundEntry && id < kIdSoundEntry + 4) {
        _eventArg = static_cast<uint8_t>(id - kIdSoundEntry);
        _pending = Event::SoundOpen;
    } else if (id == kIdSoundUp) {
        _pending = Event::SoundUp;
    } else if (id == kIdSoundPrev) {
        _pending = Event::SoundPagePrev;
    } else if (id == kIdSoundNext) {
        _pending = Event::SoundPageNext;
    } else if (id == kIdSoundPlay) {
        _pending = Event::SoundPlayPause;
    } else if (id == kIdSoundVolDown) {
        _pending = Event::SoundVolumeDown;
    } else if (id == kIdSoundVolUp) {
        _pending = Event::SoundVolumeUp;
    } else if (id == kIdSessionProgram) {
        _pending = Event::SessionProgramNext;
    } else if (id == kIdSessionDuration) {
        _pending = Event::SessionDurationNext;
    } else if (id == kIdSessionFade) {
        _pending = Event::SessionFadeNext;
    } else if (id == kIdSessionStart) {
        _pending = Event::SessionStart;
    } else if (id == kIdSessionPause) {
        _pending = Event::SessionPauseResume;
    } else if (id == kIdSessionStop) {
        _pending = Event::SessionStop;
    } else if (id == kIdNoise) {
        _pending = Event::NoiseCycle;
    } else if (id == kIdNoiseDown) {
        _pending = Event::NoiseVolumeDown;
    } else if (id == kIdNoiseUp) {
        _pending = Event::NoiseVolumeUp;
    }
}

void Ui::selectTab(Tab tab) {
    if (tab == _tab && !_btScreen && _sub == Sub::None) return;
    _btScreen = false;
    _sub = Sub::None;
    _tab = tab;
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
