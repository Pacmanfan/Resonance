// The touchscreen interface: five areas behind a tab bar (RES-GUI-001),
// drawn with a small widget layer of our own instead of a GUI framework
// (ADR-000204). The interface reads a status snapshot and reports events;
// it never reaches into another module (RES-ARCH-002).
#pragma once

#include <stdint.h>

#include "display.h"
#include "rc_edit.h"
#include "rc_lab.h"
#include "rc_pulse.h"
#include "rc_repeat.h"
#include "rc_settings.h"
#include "rc_state.h"

namespace gui {

enum class Tab : uint8_t { Home, Frequency, Sound, Sessions, Settings, Count };

// What the user asked for; the application acts on it.
enum class Event : uint8_t {
    None,
    SettingsChanged,  // the settings record was edited and should be stored
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
    // The advanced screens (RES-GUI-006). They change the Advanced record
    // themselves and say what changed.
    MixerChanged,     // layer switches, tone and partial levels, fade time
    BreathChanged,    // the pacer's pattern, depth or on/off
    CustomStart,      // start Advanced::session
    SlotsRefresh,     // fill the slot list: eventArg() 0 = sessions, 1 = sequences
    SessionSave,      // to / from slot eventArg()
    SessionLoad,
    SequenceSave,
    SequenceLoad,
    LabChanged,       // phase, inversion or a Lab control of the side being heard
    LabSwitch,        // hear side eventArg() (0 = A, 1 = B)
    LabSweep,         // sweep the right ear's phase through a full turn
    StageCapture,     // stage Advanced::stageIndex takes (or drops) what is set now: eventArg() 0 partials, 1 noise, 2 breathing, 3 recording
};

// Fade times offered on the mixer screen, in seconds.
constexpr float kFadeChoices[] = {0.05f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f};
constexpr uint8_t kFadeChoiceCount = sizeof(kFadeChoices) / sizeof(kFadeChoices[0]);
constexpr uint8_t kSlotListMax = 12;

// One of the two settings the Lab compares (kept on the card: rc_state.h).
using LabSide = rc::LabSide;
static_assert(kFadeChoiceCount == rc::kMixerFadeChoices, "the stored fade choice indexes kFadeChoices");

// What the advanced screens edit. Owned by the application; the interface
// changes it and reports an event, as it does with the settings record.
struct Advanced {
    // Layer mixer. Noise and recording levels are the ones of Status.
    bool layerOn[4] = {true, true, true, true};  // rc::Layer order
    uint8_t toneLevel = 100;
    uint8_t partialsLevel = 100;
    uint8_t partial[2] = {0, 0};  // phi, phi squared
    uint8_t fadeChoice = 2;
    // Breathing pacer.
    rc::BreathPattern breath = {40, 0, 60, 0};
    uint8_t breathDepth = 0;
    bool breathOn = false;
    float breathLevel = 0.0f;  // written on every pass of the main loop
    // Resonance Lab.
    LabSide lab[2];
    uint8_t labSlot = 0;    // the side being heard and edited
    bool labMatch = false;  // loudness matching between the sides
    // Session builder and sequence.
    rc::Session session = rc::newSession();
    rc::Sequence sequence = rc::defaultSequence();
    uint8_t stageIndex = 0;
    // Slot list, filled on SlotsRefresh.
    bool slotsReady = false;
    bool slotUsed[kSlotListMax] = {};
    char slotName[kSlotListMax][20] = {};
    const char* message = "";  // outcome of the last store or load
};

// Everything the interface shows that it does not own.
struct Status {
    bool sdMounted = false;
    uint32_t sdTotalMb = 0;
    uint32_t sdFreeMb = 0;
    bool bluetoothRunning = false;
    uint32_t freeHeap = 0;
    const char* resetReason = "";
    bool unexpectedReset = false;
    bool radioBlocked = false;  // the radio was not started: the last start lost power
    // Audio and Bluetooth.
    bool audioOn = false;
    uint8_t volume = 0;         // percent
    uint8_t btState = 0;        // btaudio::State
    char btPeer[24] = "";       // connected or chosen device
    uint8_t btCount = 0;        // devices found by the scan
    char btDevices[5][24] = {};
    // Sound library.
    char soundDir[28] = "";         // current folder, shortened for display
    bool soundAtRoot = true;
    bool soundHasPrev = false;      // an earlier page exists
    bool soundHasMore = false;      // a later page exists
    uint8_t soundCount = 0;         // entries on this page
    char soundEntries[4][48] = {};  // names; the list trims them to the width of its buttons
    bool soundIsDir[4] = {};
    uint8_t playerState = 0;        // player::State
    char trackName[22] = "";
    uint16_t trackPositionS = 0;
    uint8_t backgroundVolume = 0;   // percent
    uint8_t noiseType = 0;          // rc::NoiseType
    uint8_t noiseVolume = 0;        // percent
    uint32_t listVersion = 0;       // changes whenever the listing changed
    // Sessions.
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
    Ui(Display& display, rc::Settings& settings, Advanced& advanced);

    // Draws the open advanced screen again, after the application changed
    // what it shows.
    void refreshAdvanced();

    void begin();

    // Reads the touchscreen and redraws what changed. Call every 10-20 ms.
    Event poll();

    // Draws the whole current screen again (after something else drew on
    // the display, and for timing it).
    void redraw() { drawFrame(); }

    // Replaces the status snapshot; the visible screen is refreshed.
    void setStatus(const Status& status);

    // Development aids, used over the serial port: a press at a point as if
    // the screen had been touched there, and a list of the buttons shown
    // with the width of each label, to check a layout without a camera.
    void press(int16_t x, int16_t y);
    void describe(Print& out);

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
    static constexpr uint8_t kMaxButtons = 22;  // the FREQ tab has seventeen, the mixer screen twenty-one

    // Screens opened over a tab; None = the tab itself.
    enum class Sub : uint8_t { None, Menu, Mixer, Breath, Builder, Stage, Slots, Sequence, Lab };
    void openSub(Sub sub);
    void backSub();
    const char* subTitle() const;
    void drawAdvanced();
    void drawMenu();
    void drawMixer();
    void drawBreath();
    void drawBuilder();
    void drawStage();
    void drawSlots();
    void drawSequence();
    void drawLab();
    void onAdvancedButton(uint8_t id);
    void animateBreath();
    void drawBreathBar(int16_t x, int16_t y, int16_t w, int16_t h, int16_t& shown);
    void row(int16_t y, const char* name, const char* value, uint8_t idMinus, uint8_t idPlus);
    void valueAt(int16_t cx, int16_t y, int16_t w, const char* value);
    void redrawValues();

    void drawFrame();
    void drawHeader();
    void drawTabBar();
    void drawContent();
    void drawHome();
    void drawHomeStatus();
    void drawRing(uint8_t ring, uint8_t offset, bool erase);
    void animateHome();
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
    static bool repeats(uint8_t id);
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
    rc::HoldRepeat _repeat;        // tap-and-hold on an adjustment button
    uint8_t _heldId = 0xFF;        // the button being held; 0xFF = none
    rc::Pulse _pulse;              // the home-screen rings (rc_pulse.h)
    uint8_t _ringOffset[rc::kPulseRings] = {};  // where each ring is drawn now
    bool _btScreen = false;  // the Bluetooth device list is open over Home
    Advanced& _adv;
    Sub _sub = Sub::None;
    bool _valuesOnly = false;      // a draw pass that renews the values and leaves the rest
    uint8_t _step = 0;             // selected step on the sequence screen
    uint8_t _slotKind = 0;         // 0 sessions, 1 sequences
    bool _slotSave = false;
    uint8_t _slotArmed = 0xFF;     // a used slot tapped once for saving
    int16_t _homeBar = 0;          // breathing bar as drawn on Home, in pixels
    int16_t _breathBar = 0;        // and on the breathing screen
    char _advLabel[6][28] = {};
    char _slotLabel[kSlotListMax][24] = {};
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
    char _entryLabels[4][52] = {};
    Event _pending = Event::None;
    uint32_t _lastActivityMs = 0;
    uint32_t _touchCount = 0;
    int16_t _lastX = -1;
    int16_t _lastY = -1;
};

}  // namespace gui
