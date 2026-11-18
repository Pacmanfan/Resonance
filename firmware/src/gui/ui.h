// The touchscreen interface: five areas behind a tab bar (RES-GUI-001),
// drawn with a small widget layer of our own instead of a GUI framework
// (ADR-000204). The interface reads a status snapshot and reports events;
// it never reaches into another module (RES-ARCH-002).
#pragma once

#include <stdint.h>

#include "display.h"
#include "rc_settings.h"

namespace gui {

enum class Tab : uint8_t { Home, Frequency, Sound, Sessions, Settings, Count };

// What the user asked for; the application acts on it.
enum class Event : uint8_t {
    None,
    SettingsChanged,  // the settings record was edited and should be stored
    SwitchMode,       // restart into the other operating mode
    ToneChanged,      // the frequencies were edited: apply them to the running tone
    AudioToggle,      // audio on / off
    VolumeUp,
    VolumeDown,
    BluetoothConnect,     // connect to the found device number eventArg()
    BluetoothDisconnect,
    SoundOpen,        // open the listed entry number eventArg(): enter a folder or play a file
    SoundUp,          // to the parent folder
    SoundPagePrev,
    SoundPageNext,
    SoundPlayPause,
    SoundVolumeUp,
    SoundVolumeDown,
    NoiseCycle,       // off -> white -> pink -> brown -> off
    NoiseVolumeUp,
    NoiseVolumeDown,
    SessionProgramNext,   // timer -> factory sessions -> timer
    SessionDurationNext,  // 10, 15, 20, 30, 45, 60 minutes, continuous (timer only)
    SessionFadeNext,      // none, 30 s, 1 min, 5 min
    SessionStart,
    SessionPauseResume,
    SessionStop,
};

// Everything the interface shows that it does not own.
struct Status {
    bool sdMounted = false;
    uint32_t sdTotalMb = 0;
    uint32_t sdFreeMb = 0;
    bool bluetoothRunning = false;
    uint8_t wifiState = 0;      // wifilink::State
    char address[20] = "";      // IP address in Setup mode
    const char* network = "";   // Wi-Fi network name in Setup mode
    uint32_t freeHeap = 0;
    const char* resetReason = "";
    bool unexpectedReset = false;
    bool radioBlocked = false;  // the radio was not started: the last start lost power
    // Audio and Bluetooth (Play mode).
    bool audioOn = false;
    uint8_t volume = 0;         // percent
    uint8_t btState = 0;        // btaudio::State
    char btPeer[24] = "";       // connected or chosen device
    uint8_t btCount = 0;        // devices found by the scan
    char btDevices[5][24] = {};
    // Sound library (Play mode).
    char soundDir[28] = "";         // current folder, shortened for display
    bool soundAtRoot = true;
    bool soundHasPrev = false;      // an earlier page exists
    bool soundHasMore = false;      // a later page exists
    uint8_t soundCount = 0;         // entries on this page
    char soundEntries[4][22] = {};  // names, shortened for display
    bool soundIsDir[4] = {};
    uint8_t playerState = 0;        // player::State
    char trackName[22] = "";
    uint16_t trackPositionS = 0;
    uint8_t backgroundVolume = 0;   // percent
    uint8_t noiseType = 0;          // rc::NoiseType
    uint8_t noiseVolume = 0;        // percent
    uint32_t listVersion = 0;       // changes whenever the listing changed
    // Sessions (Play mode).
    char sessionName[20] = "";
    bool sessionIsTimer = true;
    uint16_t sessionMinutes = 0;    // length of the chosen session; 0 = continuous
    uint16_t sessionFadeS = 0;
    uint8_t sessionState = 0;       // rc::SessionState
    uint32_t sessionRemainingS = 0;
    uint8_t sessionStage = 0;
    uint8_t sessionStages = 0;
};

class Ui {
public:
    Ui(Display& display, rc::Settings& settings);

    void begin();

    // Reads the touchscreen and redraws what changed. Call every 10-20 ms.
    Event poll();

    // Replaces the status snapshot; the visible screen is refreshed.
    void setStatus(const Status& status);

    // Argument of the last event returned by poll(), where it has one.
    uint8_t eventArg() const { return _eventArg; }

    // Number of touch presses seen since start (diagnostics).
    uint32_t touchCount() const { return _touchCount; }
    int16_t lastTouchX() const { return _lastX; }
    int16_t lastTouchY() const { return _lastY; }

private:
    struct Button {
        int16_t x, y, w, h;
        const char* label;
        uint8_t id;
    };
    static constexpr uint8_t kMaxButtons = 18;  // the FREQ tab has seventeen

    void drawFrame();
    void drawHeader();
    void drawTabBar();
    void drawContent();
    void drawHome();
    void drawHomeStatus();
    void drawBluetooth();
    void drawFrequency();
    void drawFrequencyValues();
    void redrawButton(uint8_t id);
    void drawSound();
    void drawSoundStatus();
    void drawSessions();
    void drawSessionStatus();
    void drawSettings();
    void drawSettingsValues();
    void addButton(int16_t x, int16_t y, int16_t w, int16_t h, const char* label, uint8_t id);
    void drawButton(const Button& b, bool pressed);
    void label(const char* text, int16_t x, int16_t y, uint16_t color, const lgfx::IFont* font);
    void centered(const char* text, int16_t cx, int16_t cy, uint16_t color, uint16_t bg, const lgfx::IFont* font);
    void onPress(int16_t x, int16_t y);
    void onButton(uint8_t id);
    void selectTab(Tab tab);
    void wake();

    Display& _lcd;
    rc::Settings& _settings;
    Status _status;
    Tab _tab = Tab::Home;
    Button _buttons[kMaxButtons];
    uint8_t _buttonCount = 0;
    bool _touching = false;
    bool _dimmed = false;
    bool _settingsDirty = false;
    bool _modeArmed = false;
    bool _btScreen = false;  // the Bluetooth device list is open over Home
    uint8_t _eventArg = 0;
    char _audioLabel[12] = "";
    char _btLabel[36] = "";
    char _playLabel[14] = "";
    char _noiseLabel[14] = "";
    char _primaryLabel[14] = "";
    char _beatLabel[14] = "";
    char _programLabel[24] = "";
    char _durationLabel[20] = "";
    char _fadeLabel[20] = "";
    char _entryLabels[4][24] = {};
    Event _pending = Event::None;
    uint32_t _lastActivityMs = 0;
    uint32_t _touchCount = 0;
    int16_t _lastX = -1;
    int16_t _lastY = -1;
};

}  // namespace gui
