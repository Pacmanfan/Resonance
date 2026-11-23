// Resonance firmware entry point (docs/architecture/ARCHITECTURE.md).
//
// Stage 2: board foundation. The device starts in the stored operating mode
// (ADR-000204): Play starts Bluetooth and never Wi-Fi, Setup starts Wi-Fi and
// never Bluetooth. No audio exists yet, so nothing can start playing.
#include <Arduino.h>
#include <esp_heap_caps.h>

#include "gui/display.h"
#include "gui/ui.h"
#include "hal/board.h"
#include "net/webui.h"
#include "audio/bt_audio.h"
#include "audio/player.h"
#include "radio/bt_stack.h"
#include "radio/wifi_link.h"
#include "rc_settings.h"
#include "rc_audio.h"
#include "rc_session.h"
#include "storage/sdcard.h"
#include "storage/session_store.h"
#include "rc_session_text.h"
#include "storage/settings_store.h"
#include "version.h"

#include <SD.h>

namespace {

rc::Settings settings;
// The engine and the ring that carries decoded sound to it. 4096 frames are
// 93 ms of audio and 16 kB. The storage is taken from the region that only
// allows 32-bit access (about 29 kB that nothing else can use), so that it
// does not take data memory from the Bluetooth stack: with an MP3 decoder
// loaded there were only 8 kB of data memory left, the stack could not get
// a buffer for every packet, and a third of the sound was dropped.
constexpr uint32_t kRingFrames = 4096;
uint32_t* allocateRing() {
    void* p = heap_caps_malloc(kRingFrames * sizeof(uint32_t), MALLOC_CAP_EXEC | MALLOC_CAP_32BIT);
    if (p == nullptr) p = malloc(kRingFrames * sizeof(uint32_t));
    return static_cast<uint32_t*>(p);
}
uint32_t* ringStorage = allocateRing();
rc::FrameRing ring(ringStorage, kRingFrames);
rc::AudioEngine audio(ring);

bool audioOn = false;         // never on at start (RES-CFG-003)
uint8_t volume = 0;           // master, percent; starts at the settings' start-up volume
uint8_t backgroundVolume = rc::kBackgroundVolumeDefault;
rc::NoiseType noiseType = rc::NoiseType::Off;  // not restored: nothing sounds at start
uint8_t noiseVolume = rc::kNoiseVolumeDefault;

// Everything the user adjusts is written to storage a few seconds after the
// last change, not at once, to spare the flash while a button is tapped
// repeatedly.
bool settingsDirty = false;
uint32_t settingsChangedMs = 0;
constexpr uint32_t kSaveDelayMs = 4000;

// Sessions. Program 0 is the plain timer; 1.. are the factory sessions.
rc::SessionRunner session;
uint8_t sessionProgram = 0;
uint8_t timerChoice = 2;  // index into kTimerMinutes
uint8_t programLength = 0;  // index into kProgramMinutes; 0 = the program's own length
uint8_t fadeChoice = 1;   // index into kFadeSeconds
const uint16_t kTimerMinutes[] = {10, 15, 20, 30, 45, 60, 0};  // 0 = continuous
const uint16_t kProgramMinutes[] = {0, 10, 15, 20, 30, 45, 60};  // 0 = as written
const uint16_t kFadeSeconds[] = {0, 30, 60, 300};
uint32_t sessionTickMs = 0;
// A session that is not one of the programs above: a custom sweep, a
// descent, or a user session read from the card. It is the one the serial
// commands (and later the editor) work on.
// What the advanced screens edit (gui::Advanced); the serial commands work
// on the same session, sequence and partial levels.
gui::Advanced advanced;
rc::Session& customSession = advanced.session;
rc::Sequence& sequence = advanced.sequence;  // the sequence being worked on
uint8_t (&partialPercent)[2] = advanced.partial;
rc::BreathPacer breath;
uint8_t breathDepth = 0;                 // percent of audio modulation; 0 = the pacer is seen only
uint32_t breathTickMs = 0;
bool breathFromSession = false;          // a session stage started the pacer: it stops with the session
char runningName[20] = "";    // name of the session that is running
// Golden partials, percent: part of the advanced settings kept on the card.

// Sound library browsing. One page of four entries is kept; the directory is
// read again for every page.
constexpr uint8_t kPageSize = 4;
char soundDir[128];
uint16_t soundOffset = 0;
sdcard::Entry soundPage[kPageSize];
uint8_t soundCount = 0;
bool soundMore = false;
uint32_t listVersion = 1;
char trackDir[128] = "";                 // folder of the track being played
char trackName[sdcard::kNameLength] = "";
bool trackOnce = false;                  // started by a session stage: the folder's next file does not follow
// Display and interface are built on first use, inside setup(): objects that
// configure drivers must not be constructed before the scheduler runs.
gui::Display& lcd() {
    static gui::Display instance;
    return instance;
}
gui::Ui& screen() {
    static gui::Ui instance(lcd(), settings, advanced);
    return instance;
}
storage::LoadResult loadResult;
bool displayOk = false;
bool sdOk = false;
bool btOk = false;
bool radioBlocked = false;   // the radio was skipped at this start (see setup)
bool radioPending = false;   // started, not yet confirmed stable
uint32_t radioStartedMs = 0;
uint32_t lastStatusMs = 0;
uint32_t statusTicks = 0;
uint32_t loopsThisSecond = 0;   // passes of loop(), for the benchmark
uint32_t loopsLastSecond = 0;
char lineBuffer[120];
uint8_t lineLength = 0;

rc::Session chosenSession();  // defined with the session control below

void markSettingsChanged() {
    settingsDirty = true;
    settingsChangedMs = millis();
}

// Data memory: what malloc() can give. ESP.getFreeHeap() also counts the
// region that only allows 32-bit access, which ordinary data cannot use,
// and so reads about 29 kB too high (that hid the cause of a stutter).
void printHeap(const char* stage) {
    Serial.printf("HEAP %-22s free %6lu  min %6lu  largest %6lu\n", stage,
                  static_cast<unsigned long>(heap_caps_get_free_size(MALLOC_CAP_DEFAULT)),
                  static_cast<unsigned long>(heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT)),
                  static_cast<unsigned long>(heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT)));
}

void printReport() {
    const sdcard::Info sd = sdcard::info();
    Serial.printf("REPORT firmware %s\n", RESONANCE_VERSION);
    Serial.printf("REPORT reset %s%s\n", board::resetReason(), board::resetWasUnexpected() ? " (unexpected)" : "");
    Serial.printf("REPORT mode %s\n", settings.mode == rc::Mode::Setup ? "setup" : "play");
    Serial.printf("REPORT settings %s\n", loadResult == storage::LoadResult::Ok ? "loaded"
                                          : loadResult == storage::LoadResult::Repaired ? "repaired" : "defaults");
    Serial.printf("REPORT display %s id 0x%06lX\n", displayOk ? "ok" : "FAILED",
                  static_cast<unsigned long>(lcd().panelId()));
    Serial.printf("REPORT touch address 0x%02X, %lu press(es), last %d,%d\n", lcd().touchAddress(),
                  static_cast<unsigned long>(screen().touchCount()), screen().lastTouchX(), screen().lastTouchY());
    const gui::Display::TouchInfo touch = lcd().touchInfo();
    Serial.printf("REPORT touch chip '%s' configured %ux%u, %u point(s), status 0x%02X%s\n", touch.product,
                  static_cast<unsigned>(touch.width), static_cast<unsigned>(touch.height),
                  static_cast<unsigned>(touch.maxPoints), static_cast<unsigned>(touch.status),
                  touch.ok ? "" : " (READ FAILED)");
    Serial.printf("REPORT sd %s total %lu MB free %lu MB\n", sd.mounted ? "mounted" : "none",
                  static_cast<unsigned long>(sd.totalMb), static_cast<unsigned long>(sd.freeMb));
    Serial.printf("REPORT bluetooth %s%s\n", (btaudio::state() != btaudio::State::Off) ? "running" : "off",
                  radioBlocked ? " (radio skipped at this start)" : "");
    char address[20];
    wifilink::address(address, sizeof(address));
    Serial.printf("REPORT wifi state %u network '%s' address %s\n", static_cast<unsigned>(wifilink::state()),
                  wifilink::networkName(), address);
    Serial.printf("REPORT user values: volume %u, background %u, noise %u, program %u, timer %u, length %u, fade %u, %.1f / %.1f Hz\n",
                  static_cast<unsigned>(volume), static_cast<unsigned>(backgroundVolume), static_cast<unsigned>(noiseVolume),
                  static_cast<unsigned>(sessionProgram), static_cast<unsigned>(timerChoice),
                  static_cast<unsigned>(programLength), static_cast<unsigned>(fadeChoice),
                  static_cast<double>(settings.primaryHz), static_cast<double>(settings.beatHz));
    Serial.printf("REPORT web %lu request(s), %lu upload(s)\n", static_cast<unsigned long>(webui::requestCount()),
                  static_cast<unsigned long>(webui::uploadCount()));
    Serial.printf("REPORT flash image %lu bytes\n", static_cast<unsigned long>(ESP.getSketchSize()));
    // Raw identification reads, to settle which controller the panel carries.
    Serial.printf("REPORT panel RDDID 0x%06lX RDDST 0x%08lX\n", static_cast<unsigned long>(lcd().panelId()),
                  static_cast<unsigned long>(lcd().readStatus()));
    printHeap("now");
}

// Starts the radio of the current mode. The guard mark is cleared from loop()
// once the device has stayed up.
void startRadio() {
    storage::markRadioStarting();
    radioStartedMs = millis();
    radioPending = true;
    Serial.println("RADIO starting");
    Serial.flush();
    if (settings.mode == rc::Mode::Setup) {
        btstack::releaseMemory();
        printHeap("bluetooth released");
        wifilink::begin(settings);
        printHeap("wifi started");
        webui::begin(settings);
        printHeap("web server started");
    } else {
        audio.tone().setFrequencies(settings.primaryHz, settings.beatHz);
        audio.setMasterVolume(volume);
        audio.setBackgroundVolume(backgroundVolume);
        player::begin(audio, ring);
        btOk = btaudio::begin("Resonance", audio);
        printHeap("bluetooth started");
    }
}

// Writes everything the user has adjusted to storage.
void storeUserValues() {
    settingsDirty = false;
    settings.startVolume = volume;  // capped by sanitize() inside save(); the running volume is untouched
    settings.backgroundVolume = backgroundVolume;
    settings.noiseVolume = noiseVolume;
    settings.sessionProgram = sessionProgram;
    settings.timerChoice = timerChoice;
    settings.programLength = programLength;
    settings.fadeChoice = fadeChoice;
    storage::save(settings);
}

void switchMode(rc::Mode mode) {
    settings.mode = mode;
    storeUserValues();
    Serial.printf("MODE restarting in %s\n", mode == rc::Mode::Setup ? "setup" : "play");
    Serial.flush();
    delay(100);
    ESP.restart();
}

void refreshStatus() {
    gui::Status status;
    const sdcard::Info sd = sdcard::info();
    status.sdMounted = sd.mounted;
    status.sdTotalMb = sd.totalMb;
    status.sdFreeMb = sd.freeMb;
    status.bluetoothRunning = (btaudio::state() != btaudio::State::Off);
    status.wifiState = static_cast<uint8_t>(wifilink::state());
    wifilink::address(status.address, sizeof(status.address));
    status.network = wifilink::networkName();
    status.freeHeap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    status.resetReason = board::resetReason();
    status.unexpectedReset = board::resetWasUnexpected();
    status.radioBlocked = radioBlocked;
    status.audioOn = audioOn;
    status.volume = volume;
    btaudio::updateList();
    status.btState = static_cast<uint8_t>(btaudio::state());
    strlcpy(status.btPeer, btaudio::peerName(), sizeof(status.btPeer));
    // The folder is shown relative to the library root, without the leading
    // slash; at the root the line is empty.
    const char* shown = soundDir + strlen(sdcard::kAudioRoot);
    if (shown[0] == '/') ++shown;
    const size_t shownLength = strlen(shown);
    const size_t fits = sizeof(status.soundDir) - 1;
    strlcpy(status.soundDir, shownLength > fits ? shown + shownLength - fits : shown, sizeof(status.soundDir));
    status.soundAtRoot = shownLength == 0;
    status.soundHasPrev = soundOffset > 0;
    status.soundHasMore = soundMore;
    status.soundCount = soundCount;
    for (uint8_t i = 0; i < soundCount; ++i) {
        strlcpy(status.soundEntries[i], soundPage[i].name, sizeof(status.soundEntries[i]));
        status.soundIsDir[i] = soundPage[i].directory;
    }
    status.playerState = static_cast<uint8_t>(player::state());
    strlcpy(status.trackName, trackName, sizeof(status.trackName));
    status.trackPositionS = static_cast<uint16_t>(player::positionS());
    status.backgroundVolume = backgroundVolume;
    status.noiseType = static_cast<uint8_t>(noiseType);
    status.noiseVolume = noiseVolume;
    status.listVersion = listVersion;
    const rc::Session chosen = chosenSession();
    strlcpy(status.sessionName, chosen.name, sizeof(status.sessionName));
    status.sessionIsTimer = sessionProgram == 0;
    status.sessionMinutes = static_cast<uint16_t>(rc::sessionSeconds(chosen) / 60u);
    status.sessionFadeS = kFadeSeconds[fadeChoice];
    status.sessionState = static_cast<uint8_t>(session.state());
    status.sessionRemainingS = session.remainingS();
    status.sessionStage = session.stageIndex();
    status.sessionStages = session.stageCount();
    status.btCount = btaudio::deviceCount();
    for (uint8_t i = 0; i < status.btCount && i < 5; ++i) {
        strlcpy(status.btDevices[i], btaudio::deviceName(i), sizeof(status.btDevices[i]));
    }
    screen().setStatus(status);
}

void setAudio(bool on) {
    audioOn = on;
    audio.tone().setEnabled(on);
    Serial.printf("AUDIO %s\n", on ? "on" : "off");
    refreshStatus();
}

void setVolume(int percent) {
    if (percent < 0) percent = 0;
    if (percent > rc::kVolumeMax) percent = rc::kVolumeMax;
    volume = static_cast<uint8_t>(percent);
    audio.setMasterVolume(volume);
    refreshStatus();
}

int clampPercent(int percent) { return percent < 0 ? 0 : (percent > rc::kVolumeMax ? rc::kVolumeMax : percent); }

// Reads the current page of the current folder from the card.
void loadSoundPage() {
    soundCount = sdcard::list(soundDir, soundOffset, soundPage, kPageSize, &soundMore);
    ++listVersion;
}

void playTrack(const char* directory, const char* name) {
    char path[200];
    snprintf(path, sizeof(path), "%s/%s", directory, name);
    strlcpy(trackDir, directory, sizeof(trackDir));
    strlcpy(trackName, name, sizeof(trackName));
    trackOnce = false;
    player::play(path);
}

void openSoundEntry(uint8_t index) {
    if (index >= soundCount) return;
    const sdcard::Entry& entry = soundPage[index];
    if (!entry.directory) {
        playTrack(soundDir, entry.name);
        return;
    }
    if (strlen(soundDir) + 1 + strlen(entry.name) >= sizeof(soundDir)) return;  // too deep to name
    strlcat(soundDir, "/", sizeof(soundDir));
    strlcat(soundDir, entry.name, sizeof(soundDir));
    soundOffset = 0;
    loadSoundPage();
}

void soundUp() {
    if (strcmp(soundDir, sdcard::kAudioRoot) == 0) return;
    char* slash = strrchr(soundDir, '/');
    if (slash != nullptr) *slash = '\0';
    soundOffset = 0;
    loadSoundPage();
}

void applyNoise() { audio.setNoise(noiseType, noiseVolume); }

rc::Session chosenSession() {
    if (sessionProgram == 0) return rc::timerSession(kTimerMinutes[timerChoice]);
    return rc::scaledSession(rc::factorySession(static_cast<uint8_t>(sessionProgram - 1)), kProgramMinutes[programLength]);
}

void startSession() {
    if (settings.mode != rc::Mode::Play) return;
    if (!session.start(chosenSession(), settings.primaryHz, settings.beatHz, kFadeSeconds[fadeChoice])) return;
    strlcpy(runningName, chosenSession().name, sizeof(runningName));
    session.setLayersNow(partialPercent[0], partialPercent[1], noiseType == rc::NoiseType::Off ? 0 : noiseVolume);
    sessionTickMs = millis();
    audio.setSessionGain(1.0f);
    setAudio(true);  // starting a session is the user turning the audio on
    Serial.printf("SESSION started: %s\n", chosenSession().name);
}

// Starts a session that is not one of the chosen programs (a custom sweep).
void startCustomSession(const rc::Session& custom) {
    if (settings.mode != rc::Mode::Play) return;
    if (!session.start(custom, settings.primaryHz, settings.beatHz, kFadeSeconds[fadeChoice])) {
        Serial.println("SESSION refused: values out of range");
        return;
    }
    strlcpy(runningName, custom.name, sizeof(runningName));
    session.setLayersNow(partialPercent[0], partialPercent[1], noiseType == rc::NoiseType::Off ? 0 : noiseVolume);
    sessionTickMs = millis();
    audio.setSessionGain(1.0f);
    setAudio(true);
    Serial.printf("SESSION started: %s, %lu s\n", custom.name, static_cast<unsigned long>(rc::sessionSeconds(custom)));
}

// Times the audio render as it is set up now: one second of audio, in the
// blocks the Bluetooth stack asks for. Only without a connected device, since
// the stack would otherwise be rendering at the same time.
void runAudioBenchmark() {
    if (btaudio::state() == btaudio::State::Connected) {
        Serial.println("BENCH audio: disconnect the Bluetooth device first");
        return;
    }
    static int16_t block[2 * 128];
    const bool wasOn = audio.tone().enabled();
    audio.tone().setEnabled(true);
    for (int i = 0; i < 200; ++i) audio.render(block, 128);  // past the start fade
    uint32_t worst = 0;
    const uint32_t start = micros();
    for (uint32_t done = 0; done < rc::kSampleRate; done += 128) {
        const uint32_t t0 = micros();
        audio.render(block, 128);
        const uint32_t took = micros() - t0;
        if (took > worst) worst = took;
    }
    const uint32_t total = micros() - start;
    audio.tone().setEnabled(wasOn);
    Serial.printf("BENCH audio: 1 s of audio rendered in %lu us (%lu.%lu %% of one core), worst block of 128 frames %lu us\n",
                  static_cast<unsigned long>(total), static_cast<unsigned long>(total / 10000),
                  static_cast<unsigned long>((total / 1000) % 10), static_cast<unsigned long>(worst));
}

// Starts the breathing pacer, or stops it when the pattern is all zero.
void setBreath(const rc::BreathPattern& pattern, uint8_t depth) {
    breathDepth = depth > rc::kBreathDepthMax ? rc::kBreathDepthMax : depth;
    if (pattern.inhaleDs == 0 || !breath.start(pattern)) {
        breath.stop();
        breathDepth = 0;
    }
    breathTickMs = millis();
    audio.setBreathGain(rc::breathGain(breath.level(), breathDepth));
    // The breathing screen shows what runs, whoever started it.
    advanced.breathOn = breath.running();
    advanced.breathLevel = breath.level();
    if (breath.running()) {
        advanced.breath = breath.pattern();
        advanced.breathDepth = breathDepth;
    }
}

// Applies the mixer screen's record to the audio engine and the player.
void applyMixer() {
    static const rc::Layer kLayers[rc::kLayerCount] = {rc::Layer::Tone, rc::Layer::Partials, rc::Layer::Noise, rc::Layer::Voice};
    const float fade = gui::kFadeChoices[advanced.fadeChoice < gui::kFadeChoiceCount ? advanced.fadeChoice : 0];
    for (uint8_t i = 0; i < rc::kLayerCount; ++i) {
        audio.setLayerFade(kLayers[i], fade);
        audio.setLayerEnabled(kLayers[i], advanced.layerOn[i]);
    }
    if (audio.layerLevel(rc::Layer::Tone) != advanced.toneLevel) audio.setLayerLevel(rc::Layer::Tone, advanced.toneLevel);
    if (audio.layerLevel(rc::Layer::Partials) != advanced.partialsLevel) {
        audio.setLayerLevel(rc::Layer::Partials, advanced.partialsLevel);
    }
    for (uint8_t i = 0; i < 2; ++i) audio.tone().setPartial(i, advanced.partial[i]);
    player::setFades(fade, fade);
}

// The level of a mixer layer as a gain, for the loudness estimate.
float layerGain(bool on, uint8_t percent) {
    const float f = static_cast<float>(percent) * 0.01f;
    return on ? f * f : 0.0f;
}

rc::LabVoice labVoice(const gui::LabSide& side) {
    rc::LabVoice voice;
    voice.primaryHz = side.primaryHz;
    voice.beatHz = side.beatHz;
    voice.toneGain = layerGain(advanced.layerOn[0], advanced.toneLevel);
    voice.partialsGain = layerGain(advanced.layerOn[1], advanced.partialsLevel);
    voice.partial[0] = side.partial[0];
    voice.partial[1] = side.partial[1];
    voice.lab = side.lab;
    return voice;
}

bool labSweepTurn = false;  // the right ear's phase stands a full turn ahead after a sweep

// Applies the Lab side being heard: phase, inversion, the Lab controls, and
// - with matching on - the trim that makes it as loud as the other side.
void applyLab(float phaseSeconds = rc::kPhaseGlideS) {
    gui::LabSide& side = advanced.lab[advanced.labSlot < 2 ? advanced.labSlot : 0];
    // What is heard now is this side's frequencies and partials.
    side.primaryHz = settings.primaryHz;
    side.beatHz = settings.beatHz;
    side.partial[0] = partialPercent[0];
    side.partial[1] = partialPercent[1];
    audio.tone().setPhase(side.phaseL, static_cast<float>(side.phaseR) + (labSweepTurn ? 360.0f : 0.0f), phaseSeconds);
    audio.tone().setInverted(side.invertL, side.invertR);
    audio.lab().set(side.lab);
    float trim = 1.0f;
    const gui::LabSide& other = advanced.lab[advanced.labSlot == 0 ? 1 : 0];
    if (advanced.labMatch && other.primaryHz != 0.0f) {
        float mine = 1.0f, theirs = 1.0f;
        rc::matchLoudness(labVoice(side), labVoice(other), mine, theirs);
        trim = mine;
    }
    audio.lab().setTrim(trim);
}

// Leaves the side being heard and brings in the other, through the engine's
// glides and ramps: no step. A side not used yet starts as a copy.
void switchLab(uint8_t slot) {
    if (slot > 1 || slot == advanced.labSlot) return;
    gui::LabSide& from = advanced.lab[advanced.labSlot];
    from.primaryHz = settings.primaryHz;
    from.beatHz = settings.beatHz;
    from.partial[0] = partialPercent[0];
    from.partial[1] = partialPercent[1];
    gui::LabSide& to = advanced.lab[slot];
    if (to.primaryHz == 0.0f) to = from;
    advanced.labSlot = slot;
    settings.primaryHz = to.primaryHz;
    settings.beatHz = to.beatHz;
    audio.tone().setFrequencies(settings.primaryHz, settings.beatHz);
    for (uint8_t i = 0; i < 2; ++i) {
        partialPercent[i] = to.partial[i];
        audio.tone().setPartial(i, partialPercent[i]);
    }
    applyLab();
}

// The advanced settings on the card (RES-CFG-004): mixer, breathing and Lab.
// `keptState` is the record last handed to the card; the settings are
// compared with it once a second and written when they have stood changed
// for a few seconds, so that holding a button down is one write.
rc::AdvancedState keptState;
rc::AdvancedState pendingState;
bool stateDirty = false;
uint32_t stateChangedMs = 0;
const char* stateOutcome = "not read";

rc::AdvancedState stateNow() {
    rc::AdvancedState s;
    for (uint8_t i = 0; i < rc::kStateLayers; ++i) s.layerOn[i] = advanced.layerOn[i];
    s.toneLevel = advanced.toneLevel;
    s.partialsLevel = advanced.partialsLevel;
    s.partial[0] = advanced.partial[0];
    s.partial[1] = advanced.partial[1];
    s.fadeChoice = advanced.fadeChoice;
    if (breathFromSession) {
        // A pacer that a session started is the session's, not a setting.
        s.breath = keptState.breath;
        s.breathDepth = keptState.breathDepth;
        s.breathOn = keptState.breathOn;
    } else {
        s.breath = advanced.breath;
        s.breathDepth = advanced.breathDepth;
        s.breathOn = advanced.breathOn;
    }
    s.lab[0] = advanced.lab[0];
    s.lab[1] = advanced.lab[1];
    s.labSlot = advanced.labSlot;
    s.labMatch = advanced.labMatch;
    return s;
}

// Hands the settings to the card. Whatever the outcome they are not offered
// again until they change: a device without a card does not keep trying.
void storeState() {
    stateDirty = false;
    keptState = stateNow();
    if (!sdcard::info().mounted) stateOutcome = "not stored: no card";
    else if (statestore::save(keptState)) stateOutcome = "stored";
    else stateOutcome = statestore::error();
    Serial.printf("STATE %s\n", stateOutcome);
}

void tickState() {
    const rc::AdvancedState now = stateNow();
    if (rc::sameState(now, keptState)) {
        stateDirty = false;
    } else if (!stateDirty || !rc::sameState(now, pendingState)) {
        pendingState = now;
        stateDirty = true;
        stateChangedMs = millis();
    } else if (millis() - stateChangedMs > kSaveDelayMs) {
        storeState();
    }
}

// Brings back what the card holds. Without a valid file nothing is touched:
// the device starts as it does without these settings.
void restoreState() {
    rc::AdvancedState s;
    if (!sdcard::info().mounted) {
        stateOutcome = "no card";
    } else if (!statestore::used()) {
        stateOutcome = "nothing stored";
    } else if (!statestore::load(s)) {
        stateOutcome = statestore::error();
    } else {
        stateOutcome = "restored";
        for (uint8_t i = 0; i < rc::kStateLayers; ++i) advanced.layerOn[i] = s.layerOn[i];
        advanced.toneLevel = s.toneLevel;
        advanced.partialsLevel = s.partialsLevel;
        advanced.partial[0] = s.partial[0];
        advanced.partial[1] = s.partial[1];
        advanced.fadeChoice = s.fadeChoice;
        advanced.breath = s.breath;
        advanced.breathDepth = s.breathDepth;
        advanced.lab[0] = s.lab[0];
        advanced.lab[1] = s.lab[1];
        advanced.labSlot = s.labSlot;
        advanced.labMatch = s.labMatch;
        applyMixer();
        if (s.breathOn) setBreath(s.breath, s.breathDepth);
        // The side being heard takes the device's frequencies, as it does
        // whenever the Lab is applied.
        applyLab();
    }
    Serial.printf("STATE %s\n", stateOutcome);
    keptState = stateNow();
}

// Fills the slot list of the advanced screens: 0 = sessions, 1 = sequences.
void readSlots(uint8_t kind) {
    for (uint8_t i = 0; i < gui::kSlotListMax; ++i) {
        const uint8_t slot = static_cast<uint8_t>(i + 1);
        advanced.slotName[i][0] = '\0';
        if (kind == 0) {
            advanced.slotUsed[i] = sessionstore::used(slot);
            if (advanced.slotUsed[i] && !sessionstore::name(slot, advanced.slotName[i], sizeof(advanced.slotName[i]))) {
                strlcpy(advanced.slotName[i], "?", sizeof(advanced.slotName[i]));
            }
        } else {
            advanced.slotUsed[i] = slot <= sequencestore::kSlots && sequencestore::used(slot);
            if (advanced.slotUsed[i] && !sequencestore::name(slot, advanced.slotName[i], sizeof(advanced.slotName[i]))) {
                strlcpy(advanced.slotName[i], "?", sizeof(advanced.slotName[i]));
            }
        }
    }
    advanced.slotsReady = true;
}

// The stage being edited takes what is set on the device now for one of its
// layers, or - when it already sets that layer - lets go of it.
void captureStage(uint8_t kind) {
    if (advanced.stageIndex >= customSession.stageCount) return;
    rc::Session edited = customSession;
    rc::Stage& stage = edited.stages[advanced.stageIndex];
    static const uint8_t kFlags[4] = {rc::kStageSetsPartials, rc::kStageSetsNoise, rc::kStageSetsBreath, rc::kStageSetsVoice};
    if (kind >= 4) return;
    const bool had = (stage.flags & kFlags[kind]) != 0;
    stage.flags = static_cast<uint8_t>(had ? stage.flags & ~kFlags[kind] : stage.flags | kFlags[kind]);
    if (kind == 0) {
        stage.partial[0] = had ? 0 : partialPercent[0];
        stage.partial[1] = had ? 0 : partialPercent[1];
    } else if (kind == 1) {
        stage.noiseType = had ? 0 : static_cast<uint8_t>(noiseType);
        stage.noiseVolume = had || noiseType == rc::NoiseType::Off ? 0 : noiseVolume;
    } else if (kind == 2) {
        const bool on = !had && breath.running();
        stage.breath = on ? breath.pattern() : rc::BreathPattern{0, 0, 0, 0};
        stage.breathDepth = on ? breathDepth : 0;
    } else {
        // The recording that plays or is paused now; none = the stage stops playback.
        stage.voice[0] = '\0';
        stage.voiceVolume = 0;
        const bool playing = player::state() == player::State::Playing || player::state() == player::State::Paused;
        if (!had && playing && trackName[0] != '\0') {
            const size_t root = strlen(sdcard::kAudioRoot);
            char relative[200];
            if (strlen(trackDir) > root) snprintf(relative, sizeof(relative), "%s/%s", trackDir + root + 1, trackName);
            else strlcpy(relative, trackName, sizeof(relative));
            if (strlen(relative) < sizeof(stage.voice)) {
                strlcpy(stage.voice, relative, sizeof(stage.voice));
                stage.voiceVolume = backgroundVolume;
            } else {
                advanced.message = "That name is too long";
            }
        }
    }
    if (rc::isValidSession(edited)) customSession = edited;
}

// Advances the pacer and, when the modulation is on, hands its level to the
// audio engine. Called from loop() whatever screen is shown.
void tickBreath() {
    if (!breath.running()) return;
    const uint32_t now = millis();
    breath.advance(now - breathTickMs);
    breathTickMs = now;
    advanced.breathLevel = breath.level();
    // The engine ramps the gain, so the pacer starting or stopping in the
    // middle of a sound is a short glide, not a step.
    if (breathDepth != 0) audio.setBreathGain(rc::breathGain(breath.level(), breathDepth));
}

// Advances the running session and applies its values. Called from loop()
// whatever screen is shown (RES-SESS-005).
void tickSession() {
    if (session.state() == rc::SessionState::Idle) return;
    const uint32_t now = millis();
    if (now - sessionTickMs < 100) return;
    session.advance(now - sessionTickMs);
    sessionTickMs = now;
    // The session's frequencies become the device's: the screens show them
    // and they stay when the session ends. They are not written to storage.
    if (session.primaryHz() != 0.0f) settings.primaryHz = session.primaryHz();
    if (session.beatHz() != 0.0f) settings.beatHz = session.beatHz();
    audio.tone().setFrequencies(settings.primaryHz, settings.beatHz);
    // Layers the current stage sets become the device's, like the frequencies.
    if (session.setsPartials()) {
        for (uint8_t i = 0; i < 2; ++i) {
            partialPercent[i] = session.partialPercent(i);
            audio.tone().setPartial(i, partialPercent[i]);
        }
    }
    if (session.setsNoise()) {
        const rc::NoiseType type = static_cast<rc::NoiseType>(session.noiseType());
        if (type != noiseType || session.noiseVolume() != noiseVolume) {
            noiseType = type;
            noiseVolume = session.noiseVolume();
            audio.setNoise(noiseType, noiseVolume);
        }
    }
    if (session.takeBreathChange()) {
        setBreath(session.stageBreath(), session.stageBreathDepth());
        breathFromSession = breath.running();
    }
    // A stage that names a recording starts it; one that names none stops
    // what plays. The player fades the old one out before the new comes in.
    if (session.takeVoiceChange()) {
        const char* voice = session.stageVoice();
        if (voice[0] == '\0') {
            player::stop();
        } else {
            char directory[sizeof(trackDir)];
            const char* slash = strrchr(voice, '/');
            if (slash != nullptr) {
                snprintf(directory, sizeof(directory), "%s/%.*s", sdcard::kAudioRoot, static_cast<int>(slash - voice), voice);
            } else {
                strlcpy(directory, sdcard::kAudioRoot, sizeof(directory));
            }
            backgroundVolume = session.stageVoiceVolume();
            audio.setBackgroundVolume(backgroundVolume);
            playTrack(directory, slash != nullptr ? slash + 1 : voice);
            trackOnce = true;
        }
    }
    audio.setSessionGain(session.gain());
    if (session.state() == rc::SessionState::Finished) {
        // Everything stops, then the level is restored for the next start.
        setAudio(false);
        if (player::state() == player::State::Playing) player::pause();
        if (breathFromSession) setBreath({0, 0, 0, 0}, 0);
        breathFromSession = false;
        session.reset();
        delay(250);  // let the engine reach silence before the level returns
        audio.setSessionGain(1.0f);
        Serial.println("SESSION finished");
        refreshStatus();
    }
}

// Writes a five-second 440 Hz test tone as a WAV file (22.05 kHz, 16-bit,
// mono) to the card, so that the player can be exercised without a computer.
void makeTestFile() {
    const char path[] = "/RESONANCE/AUDIO/CUSTOM/test.wav";
    const uint32_t rate = 22050, frames = rate * 5, bytes = frames * 2;
    File f = SD.open(path, FILE_WRITE);
    if (!f) {
        Serial.println("MKTEST cannot create the file");
        return;
    }
    uint8_t header[44] = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0, 0, 0, 1, 0, 1, 0};
    auto put32 = [&](int at, uint32_t v) { for (int i = 0; i < 4; ++i) header[at + i] = static_cast<uint8_t>(v >> (8 * i)); };
    put32(4, 36 + bytes);
    put32(24, rate);
    put32(28, rate * 2);
    header[32] = 2; header[34] = 16;
    memcpy(header + 36, "data", 4);
    put32(40, bytes);
    f.write(header, sizeof(header));
    int16_t block[256];
    for (uint32_t done = 0; done < frames;) {
        uint32_t n = frames - done < 256 ? frames - done : 256;
        for (uint32_t i = 0; i < n; ++i) block[i] = static_cast<int16_t>(12000.0f * sinf(6.2831853f * 440.0f * (done + i) / rate));
        f.write(reinterpret_cast<const uint8_t*>(block), n * 2);
        done += n;
    }
    f.close();
    loadSoundPage();
    Serial.printf("MKTEST wrote %s (%lu bytes)\n", path, static_cast<unsigned long>(bytes + 44));
}

// Development aid: receives a file over the serial port and stores it below
// the library root, so that sound files can be put on the card without
// taking it out. Usage: "put <path> <bytes>", then exactly that many raw
// bytes. Used by firmware/tools/serial_put.py.
void receiveFile(char* arguments) {
    char* space = strrchr(arguments, ' ');
    if (space == nullptr) {
        Serial.println("PUT usage: put <path> <bytes>");
        return;
    }
    *space = '\0';
    const uint32_t size = strtoul(space + 1, nullptr, 10);
    char path[200];
    snprintf(path, sizeof(path), "%s/%s", sdcard::kAudioRoot, arguments);
    if (strstr(arguments, "..") != nullptr || size == 0 || !sdcard::isSoundFile(path)) {
        Serial.println("PUT refused");
        return;
    }
    File f = SD.open(path, FILE_WRITE);
    if (!f) {
        Serial.println("PUT cannot create the file");
        return;
    }
    Serial.println("PUT ready");
    static uint8_t block[512];
    uint32_t received = 0, lastByteMs = millis();
    while (received < size && millis() - lastByteMs < 3000) {
        const int available = Serial.available();
        if (available <= 0) {
            delay(1);
            continue;
        }
        size_t want = size - received < sizeof(block) ? size - received : sizeof(block);
        if (static_cast<size_t>(available) < want) want = static_cast<size_t>(available);
        const size_t got = Serial.readBytes(block, want);
        f.write(block, got);
        received += got;
        lastByteMs = millis();
        // Flow control: the sender waits for this after every 4096 bytes.
        if (received % 4096 == 0 || received == size) Serial.printf("PUT %lu\n", static_cast<unsigned long>(received));
    }
    f.close();
    if (received != size) {
        SD.remove(path);
        Serial.printf("PUT failed after %lu of %lu bytes\n", static_cast<unsigned long>(received), static_cast<unsigned long>(size));
        return;
    }
    loadSoundPage();
    Serial.printf("PUT stored %s (%lu bytes)\n", path, static_cast<unsigned long>(size));
}

// Display benchmark: times the drawing operations an animated screen would
// be built from, at the working SPI clock and at higher ones, and the cost
// of one pass of loop(). Prints BENCH lines; the screen shows test patterns
// meanwhile and is redrawn at the end.
void runDisplayBenchmark() {
    gui::Display& d = lcd();
    const uint32_t workingClock = d.writeClock();
    auto timeUs = [&](void (*work)(gui::Display&), int repeats) -> uint32_t {
        const uint32_t start = micros();
        for (int i = 0; i < repeats; ++i) work(d);
        return (micros() - start) / static_cast<uint32_t>(repeats);
    };
    const uint32_t clocks[] = {workingClock, 40000000u, 80000000u};
    for (uint32_t clock : clocks) {
        d.setWriteClock(clock);
        const uint32_t fill = timeUs([](gui::Display& x) { x.fillScreen(x.color565(0, 0, 40)); x.fillScreen(0); }, 2) / 2;
        const uint32_t region = timeUs([](gui::Display& x) { x.fillRect(60, 40, 120, 120, x.color565(0, 40, 40)); }, 8);
        const uint32_t rings = timeUs([](gui::Display& x) {
            for (int r = 34; r <= 54; r += 10) { x.drawCircle(120, 100, r, 0xFFFF); x.drawCircle(120, 100, r - 1, 0xFFFF); }
        }, 8);
        const uint32_t arcs = timeUs([](gui::Display& x) {
            x.fillArc(120, 100, 54, 50, 0, 360, 0x07FF);
            x.fillArc(120, 100, 44, 40, 0, 360, 0x07E0);
            x.fillArc(120, 100, 34, 30, 0, 360, 0xF81F);
        }, 4);
        const uint32_t text = timeUs([](gui::Display& x) {
            x.setFont(&fonts::DejaVu18);
            x.setTextColor(0xFFFF, 0);
            x.setTextDatum(textdatum_t::top_left);
            for (int i = 0; i < 5; ++i) x.drawString("200.0 Hz  10.0 Hz", 10, 180 + i * 22);
        }, 4);
        Serial.printf("BENCH clock %lu MHz: full screen %lu us, 120x120 fill %lu us, 6 circles %lu us, 3 thick rings %lu us, 5 text lines %lu us\n",
                      static_cast<unsigned long>(clock / 1000000u), static_cast<unsigned long>(fill),
                      static_cast<unsigned long>(region), static_cast<unsigned long>(rings),
                      static_cast<unsigned long>(arcs), static_cast<unsigned long>(text));
    }
    d.setWriteClock(workingClock);

    // An off-screen buffer pushed in one transfer: the flicker-free way to
    // animate a region. 8 bits per pixel keeps it to 14 kB for 120 x 120.
    {
        lgfx::LGFX_Sprite sprite(&d);
        sprite.setColorDepth(8);
        const uint32_t before = ESP.getFreeHeap();
        if (sprite.createSprite(120, 120) != nullptr) {
            const uint32_t used = before - ESP.getFreeHeap();
            uint32_t draw = 0, push = 0;
            for (int i = 0; i < 8; ++i) {
                uint32_t t = micros();
                sprite.fillScreen(0);
                sprite.fillArc(60, 60, 54, 50, 0, 360, 0x07FF);
                sprite.fillArc(60, 60, 44, 40, i * 20, i * 20 + 300, 0x07E0);
                sprite.fillArc(60, 60, 34, 30, 0, 360, 0xF81F);
                draw += micros() - t;
                t = micros();
                sprite.pushSprite(60, 40);
                push += micros() - t;
            }
            Serial.printf("BENCH buffered 120x120 frame at %lu MHz: draw %lu us, push %lu us, buffer %lu bytes\n",
                          static_cast<unsigned long>(workingClock / 1000000u), static_cast<unsigned long>(draw / 8),
                          static_cast<unsigned long>(push / 8), static_cast<unsigned long>(used));
            sprite.deleteSprite();
        } else {
            Serial.println("BENCH buffered frame: not enough memory for the buffer");
        }
    }

    // The interface itself.
    uint32_t t = micros();
    screen().redraw();
    Serial.printf("BENCH whole current screen redraw %lu us\n", static_cast<unsigned long>(micros() - t));
    t = micros();
    for (int i = 0; i < 20; ++i) { int32_t x, y; d.getTouch(&x, &y); }
    Serial.printf("BENCH touch read %lu us each\n", static_cast<unsigned long>((micros() - t) / 20));
    Serial.printf("BENCH loop: %lu passes in the last full second, fixed delay 15 ms per pass\n",
                  static_cast<unsigned long>(loopsLastSecond));
    printHeap("after benchmark");
}

// Serial diagnostics, for development (RES-17 "serial diagnostic output").
//   report | heap | caps | bench | scan | radio | mktest | put <path> <bytes> | play <path> | pause | resume | player | stop | session | session start <n> | session stop | bt | bt connect <n> | volume <n> | audio on | audio off | mode play | mode setup | wifi <ssid> <password> | wifi clear
void runCommand(char* line) {
    if (strcmp(line, "report") == 0) {
        printReport();
    } else if (strcmp(line, "bench") == 0) {
        runDisplayBenchmark();
    } else if (strcmp(line, "bench audio") == 0) {
        runAudioBenchmark();
    } else if (strcmp(line, "layer") == 0) {
        static const char* const names[rc::kLayerCount] = {"tone", "partials", "noise", "voice"};
        for (uint8_t i = 0; i < rc::kLayerCount; ++i) {
            const rc::Layer layer = static_cast<rc::Layer>(i);
            Serial.printf("LAYER %-8s %s level %3u fade %.1f s\n", names[i], audio.layerEnabled(layer) ? "on " : "off",
                          static_cast<unsigned>(audio.layerLevel(layer)), static_cast<double>(audio.layerFade(layer)));
        }
        Serial.printf("LAYER recording fades: in %.1f s, out %.1f s\n", static_cast<double>(player::fadeInS()),
                      static_cast<double>(player::fadeOutS()));
    } else if (strncmp(line, "layer ", 6) == 0) {
        // layer <tone|partials|noise|voice> on|off | level <percent> | fade <seconds>
        char name[10] = "", what[8] = "";
        float value = 0;
        const int got = sscanf(line + 6, "%9s %7s %f", name, what, &value);
        int index = -1;
        if (strcmp(name, "tone") == 0) index = 0;
        else if (strcmp(name, "partials") == 0) index = 1;
        else if (strcmp(name, "noise") == 0) index = 2;
        else if (strcmp(name, "voice") == 0) index = 3;
        const rc::Layer layer = static_cast<rc::Layer>(index < 0 ? 0 : index);
        if (index < 0 || got < 2) {
            Serial.println("LAYER ?");
        } else if (strcmp(what, "on") == 0 || strcmp(what, "off") == 0) {
            audio.setLayerEnabled(layer, what[1] == 'n');
            advanced.layerOn[index] = what[1] == 'n';  // what the mixer screen shows and the card keeps
        } else if (strcmp(what, "level") == 0 && got == 3) {
            const uint8_t percent = static_cast<uint8_t>(clampPercent(static_cast<int>(value)));
            // Noise and recording levels are the ones the screens show.
            if (layer == rc::Layer::Noise) noiseVolume = percent;
            if (layer == rc::Layer::Voice) backgroundVolume = percent;
            if (layer == rc::Layer::Tone) advanced.toneLevel = percent;
            if (layer == rc::Layer::Partials) advanced.partialsLevel = percent;
            if (layer == rc::Layer::Noise) applyNoise();
            else audio.setLayerLevel(layer, percent);
        } else if (strcmp(what, "fade") == 0 && got == 3) {
            audio.setLayerFade(layer, value);
        }
    } else if (strncmp(line, "voice fade ", 11) == 0) {
        // voice fade <in seconds> <out seconds>: how a recording starts, stops and gives way to the next
        float in = 0, out = 0;
        if (sscanf(line + 11, "%f %f", &in, &out) == 2) player::setFades(in, out);
    } else if (strncmp(line, "noise ", 6) == 0) {
        // noise off|white|pink|brown [percent]
        char type[8] = "";
        int percent = noiseVolume;
        if (sscanf(line + 6, "%7s %d", type, &percent) >= 1) {
            noiseType = strcmp(type, "white") == 0   ? rc::NoiseType::White
                        : strcmp(type, "pink") == 0  ? rc::NoiseType::Pink
                        : strcmp(type, "brown") == 0 ? rc::NoiseType::Brown
                                                     : rc::NoiseType::Off;
            noiseVolume = static_cast<uint8_t>(clampPercent(percent));
            applyNoise();
        }
    } else if (strcmp(line, "tone") == 0) {
        Serial.printf("TONE %s, %.1f Hz + %.1f Hz, phase L %.1f R %.1f deg%s\n", audio.tone().enabled() ? "on" : "off",
                      settings.primaryHz, settings.beatHz, audio.tone().phaseDegrees(rc::Channel::Left),
                      audio.tone().phaseDegrees(rc::Channel::Right), audio.tone().phaseMoving() ? " (moving)" : "");
    } else if (strncmp(line, "tone phase ", 11) == 0) {
        // tone phase <left degrees> <right degrees> [seconds]
        float left = 0, right = 0, seconds = rc::kPhaseGlideS;
        if (sscanf(line + 11, "%f %f %f", &left, &right, &seconds) >= 2) audio.tone().setPhase(left, right, seconds);
    } else if (strncmp(line, "tone invert ", 12) == 0) {
        // tone invert <left 0|1> <right 0|1>
        int left = 0, right = 0;
        if (sscanf(line + 12, "%d %d", &left, &right) == 2) audio.tone().setInverted(left != 0, right != 0);
    } else if (strncmp(line, "tone partial ", 13) == 0) {
        // tone partial <1|2> <percent>: 1 = phi, 2 = phi squared
        int index = 0, percent = 0;
        if (sscanf(line + 13, "%d %d", &index, &percent) == 2 && index >= 1 && percent >= 0) {
            if (index <= 2) partialPercent[index - 1] = static_cast<uint8_t>(percent > 100 ? 100 : percent);
            audio.tone().setPartial(static_cast<uint8_t>(index - 1), static_cast<uint8_t>(percent > 100 ? 100 : percent));
        }
    } else if (strncmp(line, "descent ", 8) == 0) {
        // descent lin|exp <start beat> <transition s> <hold s> <beat 1> [<beat 2> ...]: builds the custom session
        rc::Descent d = {};
        char shape[8] = "";
        int transition = 0, hold = 0, used = 0;
        if (sscanf(line + 8, "%7s %f %d %d%n", shape, &d.startBeatHz, &transition, &hold, &used) == 4 && transition > 0 &&
            transition <= 65535 && hold >= 0 && hold <= 65535) {
            d.sweep = strcmp(shape, "exp") == 0 ? rc::Sweep::Exponential : rc::Sweep::Linear;
            d.transitionS = static_cast<uint16_t>(transition);
            d.holdS = static_cast<uint16_t>(hold);
            const char* rest = line + 8 + used;
            float beat = 0;
            int step = 0;
            while (d.targetCount < rc::kMaxDescentTargets && sscanf(rest, "%f%n", &beat, &step) == 1) {
                d.targets[d.targetCount++] = {0.0f, beat};
                rest += step;
            }
            customSession = rc::descentSession(d);
            Serial.printf("DESCENT %s: %u stage(s), %lu s\n", rc::isValidSession(customSession) ? "ready" : "REFUSED",
                          static_cast<unsigned>(customSession.stageCount),
                          static_cast<unsigned long>(rc::sessionSeconds(customSession)));
        }
    } else if (strcmp(line, "usession") == 0) {
        // The custom session as it would be stored, then the slots on the card.
        char text[rc::kSessionLineMax];
        for (uint8_t i = 0; rc::sessionLine(customSession, i, text, sizeof(text)); ++i) Serial.printf("USESSION | %s\n", text);
        for (uint8_t slot = 1; slot <= sessionstore::kSlots; ++slot) {
            if (!sessionstore::used(slot)) continue;
            char name[20] = "?";
            sessionstore::name(slot, name, sizeof(name));
            Serial.printf("USESSION slot %u: %s\n", static_cast<unsigned>(slot), name);
        }
    } else if (strncmp(line, "state", 5) == 0 && (line[5] == '\0' || line[5] == ' ')) {
        // state | state save | state clear: the advanced settings kept on the card
        if (strcmp(line, "state save") == 0) {
            storeState();
        } else if (strcmp(line, "state clear") == 0) {
            const bool removed = statestore::remove();
            keptState = stateNow();
            stateDirty = false;
            stateOutcome = removed ? "file removed" : "nothing to remove";
        }
        char text[rc::kStateLineMax];
        const rc::AdvancedState now = stateNow();
        for (uint8_t i = 0; rc::stateLine(now, i, text, sizeof(text)); ++i) Serial.printf("STATE | %s\n", text);
        Serial.printf("STATE %s%s, file %s\n", stateOutcome, stateDirty ? ", a change is waiting" : "",
                      statestore::used() ? "on the card" : "not on the card");
    } else if (strcmp(line, "lab") == 0) {
        for (uint8_t i = 0; i < 2; ++i) {
            const gui::LabSide& side = advanced.lab[i];
            Serial.printf("LAB %c%s %.1f + %.1f Hz, golden %u/%u, phase %d/%d, invert %d/%d, width %u, delay %d us, bilateral %u ds depth %u %s, loudness %.4f\n",
                          'A' + i, i == advanced.labSlot ? "*" : " ", static_cast<double>(side.primaryHz),
                          static_cast<double>(side.beatHz), side.partial[0], side.partial[1], side.phaseL, side.phaseR,
                          side.invertL, side.invertR, side.lab.widthPercent, side.lab.delayUs, side.lab.bilateralPeriodDs,
                          side.lab.bilateralDepth, side.lab.bilateralGolden ? "golden" : "even",
                          static_cast<double>(rc::labLoudness(labVoice(side))));
        }
        Serial.printf("LAB match %s, phase now L %.1f R %.1f\n", advanced.labMatch ? "on" : "off",
                      static_cast<double>(audio.tone().phaseDegrees(rc::Channel::Left)),
                      static_cast<double>(audio.tone().phaseDegrees(rc::Channel::Right)));
    } else if (strncmp(line, "tap ", 4) == 0) {
        // tap <x> <y>: as if the screen had been touched there
        int x = 0, y = 0;
        if (sscanf(line + 4, "%d %d", &x, &y) == 2) screen().press(static_cast<int16_t>(x), static_cast<int16_t>(y));
    } else if (strcmp(line, "ui") == 0) {
        screen().describe(Serial);
    } else if (strcmp(line, "breath") == 0) {
        static const char* const names[] = {"off", "inhale", "hold in", "exhale", "hold out"};
        const rc::BreathPattern& p = breath.pattern();
        Serial.printf("BREATH %s, %u.%u s left, level %.2f, %lu breath(s), pattern %u/%u/%u/%u ds, depth %u %%\n",
                      names[static_cast<uint8_t>(breath.phase())], breath.phaseLeftDs() / 10u, breath.phaseLeftDs() % 10u,
                      static_cast<double>(breath.level()), static_cast<unsigned long>(breath.breaths()), p.inhaleDs,
                      p.holdInDs, p.exhaleDs, p.holdOutDs, static_cast<unsigned>(breathDepth));
    } else if (strcmp(line, "breath off") == 0) {
        setBreath({0, 0, 0, 0}, 0);
    } else if (strncmp(line, "breath ", 7) == 0) {
        // breath <inhale s> <hold s> <exhale s> <hold s> [depth percent]
        float in = 0, holdIn = 0, out = 0, holdOut = 0;
        int depth = 0;
        if (sscanf(line + 7, "%f %f %f %f %d", &in, &holdIn, &out, &holdOut, &depth) >= 4 && in >= 0 && in <= 25 &&
            holdIn >= 0 && holdIn <= 25 && out >= 0 && out <= 25 && holdOut >= 0 && holdOut <= 25) {
            const rc::BreathPattern pattern = {static_cast<uint8_t>(in * 10 + 0.5f), static_cast<uint8_t>(holdIn * 10 + 0.5f),
                                               static_cast<uint8_t>(out * 10 + 0.5f), static_cast<uint8_t>(holdOut * 10 + 0.5f)};
            if (rc::isValidBreath(pattern)) setBreath(pattern, static_cast<uint8_t>(clampPercent(depth)));
            breathFromSession = false;
            Serial.printf("BREATH %s\n", rc::isValidBreath(pattern) ? "started" : "REFUSED");
        }
    } else if (strncmp(line, "usession breath ", 16) == 0) {
        // usession breath <stage> <inhale ds> <hold ds> <exhale ds> <hold ds> <depth>: all zero = the stage stops the pacer
        int stage = 0, v[5] = {0, 0, 0, 0, 0};
        if (sscanf(line + 16, "%d %d %d %d %d %d", &stage, &v[0], &v[1], &v[2], &v[3], &v[4]) == 6 && stage >= 1 &&
            stage <= customSession.stageCount) {
            rc::Session edited = customSession;
            rc::Stage& target = edited.stages[stage - 1];
            bool ok = true;
            for (int value : v) ok = ok && value >= 0 && value <= 255;
            target.flags |= rc::kStageSetsBreath;
            target.breath = {static_cast<uint8_t>(v[0]), static_cast<uint8_t>(v[1]), static_cast<uint8_t>(v[2]),
                             static_cast<uint8_t>(v[3])};
            target.breathDepth = static_cast<uint8_t>(v[4]);
            ok = ok && rc::isValidSession(edited);
            if (ok) customSession = edited;
            Serial.printf("USESSION breath %s\n", ok ? "set" : "REFUSED");
        }
    } else if (strcmp(line, "seq") == 0) {
        // The sequence being worked on as it would be stored, then the slots on the card.
        char text[rc::kSequenceLineMax];
        for (uint8_t i = 0; rc::sequenceLine(sequence, i, text, sizeof(text)); ++i) Serial.printf("SEQ | %s\n", text);
        for (uint8_t slot = 1; slot <= sequencestore::kSlots; ++slot) {
            if (!sequencestore::used(slot)) continue;
            char name[20] = "?";
            sequencestore::name(slot, name, sizeof(name));
            Serial.printf("SEQ slot %u: %s\n", static_cast<unsigned>(slot), name);
        }
    } else if (strcmp(line, "seq default") == 0) {
        sequence = rc::defaultSequence();
    } else if (strncmp(line, "seq step ", 9) == 0) {
        // seq step <n> <Hz> <seconds>: changes step n, or adds it when n is one past the last
        int n = 0, seconds = 0;
        float hz = 0;
        rc::Sequence edited = sequence;
        bool ok = sscanf(line + 9, "%d %f %d", &n, &hz, &seconds) == 3 && n >= 1 && n <= edited.stepCount + 1 &&
                  n <= rc::kMaxSequenceSteps && seconds >= 1 && seconds <= 65535;
        if (ok) {
            edited.steps[n - 1] = {hz, static_cast<uint16_t>(seconds)};
            if (n > edited.stepCount) edited.stepCount = static_cast<uint8_t>(n);
            ok = rc::isValidSequence(edited);
        }
        if (ok) sequence = edited;
        Serial.printf("SEQ step %s\n", ok ? "set" : "REFUSED");
    } else if (strncmp(line, "seq remove ", 11) == 0) {
        const int n = atoi(line + 11);
        const bool ok = n >= 1 && n <= sequence.stepCount && sequence.stepCount > 1;
        if (ok) {
            for (int i = n; i < sequence.stepCount; ++i) sequence.steps[i - 1] = sequence.steps[i];
            --sequence.stepCount;
        }
        Serial.printf("SEQ remove %s\n", ok ? "done" : "REFUSED");
    } else if (strncmp(line, "seq glide ", 10) == 0) {
        const int seconds = atoi(line + 10);
        if (seconds >= 0 && seconds <= rc::kSequenceGlideMaxS) sequence.glideS = static_cast<uint16_t>(seconds);
    } else if (strncmp(line, "seq beat ", 9) == 0) {
        rc::Sequence edited = sequence;
        edited.beatHz = static_cast<float>(atof(line + 9));
        const bool ok = rc::isValidSequence(edited);
        if (ok) sequence = edited;
        Serial.printf("SEQ beat %s\n", ok ? "set" : "REFUSED");
    } else if (strncmp(line, "seq save ", 9) == 0) {
        const bool ok = sequencestore::save(static_cast<uint8_t>(atoi(line + 9)), sequence);
        Serial.printf("SEQ save %s %s\n", ok ? "ok" : "FAILED:", sequencestore::error());
    } else if (strncmp(line, "seq load ", 9) == 0) {
        const bool ok = sequencestore::load(static_cast<uint8_t>(atoi(line + 9)), sequence);
        Serial.printf("SEQ load %s %s\n", ok ? "ok" : "FAILED:", sequencestore::error());
    } else if (strncmp(line, "seq delete ", 11) == 0) {
        Serial.printf("SEQ delete %s\n", sequencestore::remove(static_cast<uint8_t>(atoi(line + 11))) ? "ok" : "FAILED");
    } else if (strcmp(line, "seq use") == 0) {
        // The sequence becomes the custom session, to run, change or store.
        customSession = rc::sequenceSession(sequence);
        Serial.printf("SEQ %s: %u stage(s), %lu s\n", rc::isValidSession(customSession) ? "ready as the custom session" : "REFUSED",
                      static_cast<unsigned>(customSession.stageCount),
                      static_cast<unsigned long>(rc::sessionSeconds(customSession)));
    } else if (strncmp(line, "usession voice ", 15) == 0) {
        // usession voice <stage> <percent> [<path below the sound library>]: no path = the stage stops playback
        int stage = 0, percent = 0, used = 0;
        if (sscanf(line + 15, "%d %d%n", &stage, &percent, &used) == 2 && stage >= 1 && stage <= customSession.stageCount) {
            rc::Session edited = customSession;
            rc::Stage& target = edited.stages[stage - 1];
            const char* name = line + 15 + used;
            while (*name == ' ') ++name;
            target.flags |= rc::kStageSetsVoice;
            target.voiceVolume = static_cast<uint8_t>(clampPercent(percent));
            strlcpy(target.voice, name, sizeof(target.voice));
            const bool ok = strlen(name) < sizeof(target.voice) && rc::isValidSession(edited);
            if (ok) customSession = edited;
            Serial.printf("USESSION voice %s\n", ok ? "set" : "REFUSED");
        }
    } else if (strcmp(line, "usession run") == 0) {
        startCustomSession(customSession);
    } else if (strncmp(line, "usession save ", 14) == 0) {
        const bool ok = sessionstore::save(static_cast<uint8_t>(atoi(line + 14)), customSession);
        Serial.printf("USESSION save %s %s\n", ok ? "ok" : "FAILED:", sessionstore::error());
    } else if (strncmp(line, "usession load ", 14) == 0) {
        const bool ok = sessionstore::load(static_cast<uint8_t>(atoi(line + 14)), customSession);
        Serial.printf("USESSION load %s %s\n", ok ? "ok" : "FAILED:", sessionstore::error());
    } else if (strncmp(line, "usession delete ", 16) == 0) {
        Serial.printf("USESSION delete %s\n", sessionstore::remove(static_cast<uint8_t>(atoi(line + 16))) ? "ok" : "FAILED");
    } else if (strncmp(line, "sweep ", 6) == 0) {
        // sweep lin|exp <from primary> <to primary> <from beat> <to beat> <seconds>; 0 0 leaves a frequency alone
        char shape[8] = "";
        float fromPrimary = 0, toPrimary = 0, fromBeat = 0, toBeat = 0;
        int seconds = 0;
        if (sscanf(line + 6, "%7s %f %f %f %f %d", shape, &fromPrimary, &toPrimary, &fromBeat, &toBeat, &seconds) == 6 &&
            seconds > 0 && seconds <= 65535) {
            customSession = (rc::sweepSession(strcmp(shape, "exp") == 0 ? rc::Sweep::Exponential : rc::Sweep::Linear,
                                                fromPrimary, toPrimary, fromBeat, toBeat, static_cast<uint16_t>(seconds)));
            startCustomSession(customSession);
        }
    } else if (strcmp(line, "caps") == 0) {
        // Every memory region, for diagnostics.
        heap_caps_print_heap_info(MALLOC_CAP_DEFAULT);
    } else if (strcmp(line, "heap") == 0) {
        printHeap("now");
    } else if (strcmp(line, "scan") == 0) {
        const int n = wifilink::scanToSerial();
        Serial.printf("SCAN %d network(s)\n", n);
        printHeap("after wifi scan");
    } else if (strncmp(line, "volume ", 7) == 0) {
        setVolume(atoi(line + 7));
        markSettingsChanged();
    } else if (strcmp(line, "audio on") == 0) {
        setAudio(true);
    } else if (strcmp(line, "audio off") == 0) {
        setAudio(false);
    } else if (strncmp(line, "play ", 5) == 0) {
        // A path below the library root, for example: play CUSTOM/test.wav
        const char* slash = strrchr(line + 5, '/');
        char directory[128];
        if (slash != nullptr) {
            snprintf(directory, sizeof(directory), "%s/%.*s", sdcard::kAudioRoot, static_cast<int>(slash - (line + 5)), line + 5);
            playTrack(directory, slash + 1);
        } else {
            playTrack(sdcard::kAudioRoot, line + 5);
        }
    } else if (strncmp(line, "put ", 4) == 0) {
        receiveFile(line + 4);
    } else if (strcmp(line, "player") == 0) {
        Serial.printf("PLAYER state %u track '%s' position %lu s of %lu s, ring %lu of %lu frames, underruns %lu, error '%s'\n",
                      static_cast<unsigned>(player::state()), trackName, static_cast<unsigned long>(player::positionS()),
                      static_cast<unsigned long>(player::durationS()), static_cast<unsigned long>(ring.available()),
                      static_cast<unsigned long>(ring.capacity()), static_cast<unsigned long>(audio.underruns()),
                      player::error());
    } else if (strcmp(line, "session") == 0) {
        Serial.printf("SESSION state %u '%s' stage %u of %u, %lu s left, beat %.2f Hz, level %.2f\n",
                      static_cast<unsigned>(session.state()), runningName,
                      static_cast<unsigned>(session.stageIndex() + 1), static_cast<unsigned>(session.stageCount()),
                      static_cast<unsigned long>(session.remainingS()), static_cast<double>(settings.beatHz),
                      static_cast<double>(session.gain()));
    } else if (strncmp(line, "session start ", 14) == 0) {
        sessionProgram = static_cast<uint8_t>(atoi(line + 14)) % (rc::factorySessionCount() + 1);
        startSession();
    } else if (strcmp(line, "session stop") == 0) {
        session.stop();
    } else if (strcmp(line, "pause") == 0) {
        player::pause();
    } else if (strcmp(line, "resume") == 0) {
        player::resume();
    } else if (strcmp(line, "stop") == 0) {
        player::stop();
    } else if (strcmp(line, "mktest") == 0) {
        makeTestFile();
    } else if (strcmp(line, "bt") == 0) {
        Serial.printf("BT state %u peer '%s', %u device(s) found\n", static_cast<unsigned>(btaudio::state()),
                      btaudio::peerName(), static_cast<unsigned>(btaudio::deviceCount()));
        for (uint8_t i = 0; i < btaudio::deviceCount(); ++i) Serial.printf("  %u: %s\n", i, btaudio::deviceName(i));
        const btaudio::StreamStats stats = btaudio::takeStreamStats();
        Serial.printf("STREAM worst gap %lu ms, worst render %lu us, %lu long gap(s), %lu underrun(s) since last asked\n",
                      static_cast<unsigned long>(stats.worstGapMs), static_cast<unsigned long>(stats.worstRenderUs),
                      static_cast<unsigned long>(stats.longGaps), static_cast<unsigned long>(audio.underruns()));
        Serial.printf("STREAM %lu tick(s), task tick %lu ms, clock %lu ms\n", static_cast<unsigned long>(stats.bursts),
                      static_cast<unsigned long>(xTaskGetTickCount() * portTICK_PERIOD_MS), static_cast<unsigned long>(millis()));
        Serial.printf("STREAM %lu frames/s in %lu request(s) of %lu..%lu frames\n",
                      static_cast<unsigned long>(stats.framesPerSecond), static_cast<unsigned long>(stats.calls),
                      static_cast<unsigned long>(stats.smallestCall), static_cast<unsigned long>(stats.largestCall));
    } else if (strncmp(line, "bt connect ", 11) == 0) {
        btaudio::connect(static_cast<uint8_t>(atoi(line + 11)));
    } else if (strcmp(line, "radio") == 0) {
        startRadio();
    } else if (strcmp(line, "mode play") == 0) {
        switchMode(rc::Mode::Play);
    } else if (strcmp(line, "mode setup") == 0) {
        switchMode(rc::Mode::Setup);
    } else if (strcmp(line, "wifi clear") == 0) {
        settings.wifiSsid[0] = '\0';
        settings.wifiPass[0] = '\0';
        Serial.printf("WIFI cleared, stored %s\n", storage::save(settings) ? "ok" : "FAILED");
    } else if (strncmp(line, "wifi ", 5) == 0) {
        char* ssid = line + 5;
        char* pass = strchr(ssid, ' ');
        if (pass != nullptr) *pass++ = '\0';
        strlcpy(settings.wifiSsid, ssid, sizeof(settings.wifiSsid));
        strlcpy(settings.wifiPass, pass != nullptr ? pass : "", sizeof(settings.wifiPass));
        // The password is never echoed (RES-NET-004).
        Serial.printf("WIFI network '%s' stored %s; applies at the next start in setup mode\n", settings.wifiSsid,
                      storage::save(settings) ? "ok" : "FAILED");
    } else if (line[0] != '\0') {
        Serial.println("commands: report | heap | tap <x> <y> | ui | lab | state | state save | state clear | caps | bench | bench audio | tone | tone phase <L> <R> [s] | tone invert <L> <R> | tone partial <1|2> <percent> | sweep lin|exp <fromP> <toP> <fromB> <toB> <s> | descent lin|exp <startB> <trans s> <hold s> <B1> [B2..] | breath | breath <in s> <hold s> <out s> <hold s> [depth] | breath off | seq | seq default | seq step <n> <Hz> <s> | seq remove <n> | seq glide <s> | seq beat <Hz> | seq save|load|delete <slot> | seq use | usession | usession breath <stage> <in ds> <hold ds> <out ds> <hold ds> <depth> | usession voice <stage> <percent> [path] | usession run | layer | layer <tone|partials|noise|voice> on|off|level <n>|fade <s> | voice fade <in s> <out s> | noise off|white|pink|brown [percent] | usession save|load|delete <slot> | scan | radio | mktest | put <path> <bytes> | play <path> | pause | resume | player | stop | session | session start <n> | session stop | bt | bt connect <n> | volume <n> | audio on | audio off | mode play | mode setup | wifi <ssid> <password> | wifi clear");
    }
}

void pollSerial() {
    while (Serial.available() > 0) {
        const char c = static_cast<char>(Serial.read());
        if (c == '\n' || c == '\r') {
            lineBuffer[lineLength] = '\0';
            lineLength = 0;
            runCommand(lineBuffer);
        } else if (lineLength < sizeof(lineBuffer) - 1) {
            lineBuffer[lineLength++] = c;
        }
    }
}

}  // namespace

void setup() {
    Serial.begin(115200);
    Serial.printf("\nRESONANCE %s\n", RESONANCE_VERSION);
    board::begin();
    printHeap("boot");

    loadResult = storage::load(settings);
    // The device comes back as it was left. The master volume was stored
    // capped, and the audio itself always starts off.
    volume = settings.startVolume;
    backgroundVolume = settings.backgroundVolume;
    noiseVolume = settings.noiseVolume;
    sessionProgram = settings.sessionProgram <= rc::factorySessionCount() ? settings.sessionProgram : 0;
    timerChoice = settings.timerChoice < sizeof(kTimerMinutes) / sizeof(kTimerMinutes[0]) ? settings.timerChoice : 2;
    programLength = settings.programLength < sizeof(kProgramMinutes) / sizeof(kProgramMinutes[0]) ? settings.programLength : 0;
    fadeChoice = settings.fadeChoice < sizeof(kFadeSeconds) / sizeof(kFadeSeconds[0]) ? settings.fadeChoice : 1;

    displayOk = lcd().begin();
    lcd().setBacklightPercent(settings.brightness);
    screen().begin();
    printHeap("display + ui");

    sdOk = sdcard::begin();
    strlcpy(soundDir, sdcard::kAudioRoot, sizeof(soundDir));
    loadSoundPage();
    printHeap("sd");

    // One radio per mode. The other one is never started, so it costs no RAM.
    // If the last starts all died while a radio was coming up, no radio is
    // started this time: the device stays usable and says why.
    radioBlocked = storage::radioStartKeepsFailing();
    if (radioBlocked) {
        Serial.println("RADIO not started: the last starts lost power while the radio came up");
        storage::markRadioStable();  // one skipped start, then try again
    } else {
        startRadio();
    }
    restoreState();
    printHeap("advanced settings");

    refreshStatus();
    printReport();
}

void loop() {
    const gui::Event event = screen().poll();
    switch (event) {
        case gui::Event::SettingsChanged:
        case gui::Event::VolumeUp:
        case gui::Event::VolumeDown:
        case gui::Event::SoundVolumeUp:
        case gui::Event::SoundVolumeDown:
        case gui::Event::NoiseVolumeUp:
        case gui::Event::NoiseVolumeDown:
        case gui::Event::ToneChanged:
        case gui::Event::SessionProgramNext:
        case gui::Event::SessionDurationNext:
        case gui::Event::SessionFadeNext:
            markSettingsChanged();
            break;
        default:
            break;
    }
    switch (event) {
        case gui::Event::SwitchMode:
            switchMode(settings.mode == rc::Mode::Setup ? rc::Mode::Play : rc::Mode::Setup);
            break;
        case gui::Event::ToneChanged:
            audio.tone().setFrequencies(settings.primaryHz, settings.beatHz);
            break;
        case gui::Event::AudioToggle:
            setAudio(!audioOn);
            break;
        case gui::Event::VolumeUp:
            setVolume(volume + 5);
            break;
        case gui::Event::VolumeDown:
            setVolume(volume - 5);
            break;
        case gui::Event::BluetoothConnect:
            btaudio::connect(screen().eventArg());
            refreshStatus();
            break;
        case gui::Event::SoundOpen:
            openSoundEntry(screen().eventArg());
            refreshStatus();
            break;
        case gui::Event::SoundUp:
            soundUp();
            refreshStatus();
            break;
        case gui::Event::SoundPagePrev:
            soundOffset = soundOffset >= kPageSize ? soundOffset - kPageSize : 0;
            loadSoundPage();
            refreshStatus();
            break;
        case gui::Event::SoundPageNext:
            if (soundMore) soundOffset += kPageSize;
            loadSoundPage();
            refreshStatus();
            break;
        case gui::Event::SoundPlayPause:
            if (player::state() == player::State::Playing) player::pause();
            else if (player::state() == player::State::Paused) player::resume();
            else if (trackName[0] != '\0') playTrack(trackDir, trackName);
            else if (soundCount > 0 && !soundPage[0].directory) playTrack(soundDir, soundPage[0].name);
            refreshStatus();
            break;
        case gui::Event::SoundVolumeUp:
        case gui::Event::SoundVolumeDown:
            backgroundVolume = static_cast<uint8_t>(
                clampPercent(backgroundVolume + (event == gui::Event::SoundVolumeUp ? 5 : -5)));
            audio.setBackgroundVolume(backgroundVolume);
            refreshStatus();
            break;
        case gui::Event::NoiseCycle:
            noiseType = static_cast<rc::NoiseType>((static_cast<uint8_t>(noiseType) + 1) & 3);
            applyNoise();
            refreshStatus();
            break;
        case gui::Event::NoiseVolumeUp:
        case gui::Event::NoiseVolumeDown:
            noiseVolume = static_cast<uint8_t>(clampPercent(noiseVolume + (event == gui::Event::NoiseVolumeUp ? 5 : -5)));
            applyNoise();
            refreshStatus();
            break;
        case gui::Event::SessionProgramNext:
            sessionProgram = static_cast<uint8_t>((sessionProgram + 1) % (rc::factorySessionCount() + 1));
            programLength = 0;  // a newly chosen program starts at its own length
            refreshStatus();
            break;
        case gui::Event::SessionDurationNext:
            if (sessionProgram == 0) {
                timerChoice = static_cast<uint8_t>((timerChoice + 1) % (sizeof(kTimerMinutes) / sizeof(kTimerMinutes[0])));
            } else {
                // Skip a choice equal to the program's own length, which is
                // already the first entry.
                const uint32_t own = rc::sessionSeconds(rc::factorySession(static_cast<uint8_t>(sessionProgram - 1))) / 60u;
                do {
                    programLength = static_cast<uint8_t>((programLength + 1) % (sizeof(kProgramMinutes) / sizeof(kProgramMinutes[0])));
                } while (programLength != 0 && kProgramMinutes[programLength] == own);
            }
            refreshStatus();
            break;
        case gui::Event::SessionFadeNext:
            fadeChoice = static_cast<uint8_t>((fadeChoice + 1) % (sizeof(kFadeSeconds) / sizeof(kFadeSeconds[0])));
            refreshStatus();
            break;
        case gui::Event::SessionStart:
            startSession();
            refreshStatus();
            break;
        case gui::Event::MixerChanged:
            applyMixer();
            break;
        case gui::Event::BreathChanged:
            if (advanced.breathOn) setBreath(advanced.breath, advanced.breathDepth);
            else setBreath({0, 0, 0, 0}, 0);
            breathFromSession = false;
            break;
        case gui::Event::CustomStart:
            startCustomSession(customSession);
            refreshStatus();
            break;
        case gui::Event::SlotsRefresh:
            readSlots(screen().eventArg());
            screen().refreshAdvanced();
            break;
        case gui::Event::SessionSave:
        case gui::Event::SessionLoad: {
            const uint8_t slot = screen().eventArg();
            const bool ok = event == gui::Event::SessionSave ? sessionstore::save(slot, customSession)
                                                              : sessionstore::load(slot, customSession);
            advanced.message = ok ? (event == gui::Event::SessionSave ? "Stored" : "Loaded") : sessionstore::error();
            advanced.stageIndex = 0;
            screen().refreshAdvanced();
            break;
        }
        case gui::Event::SequenceSave:
        case gui::Event::SequenceLoad: {
            const uint8_t slot = screen().eventArg();
            const bool ok = event == gui::Event::SequenceSave ? sequencestore::save(slot, sequence)
                                                               : sequencestore::load(slot, sequence);
            advanced.message = ok ? "" : sequencestore::error();
            screen().refreshAdvanced();
            break;
        }
        case gui::Event::LabChanged:
            applyLab();
            break;
        case gui::Event::LabSwitch:
            switchLab(screen().eventArg());
            screen().refreshAdvanced();
            refreshStatus();
            break;
        case gui::Event::LabSweep:
            // A full turn of the right ear's phase in thirty seconds, forwards
            // and, the next time, back: the offset never piles up.
            labSweepTurn = !labSweepTurn;
            applyLab(30.0f);
            break;
        case gui::Event::StageCapture:
            captureStage(screen().eventArg());
            screen().refreshAdvanced();
            break;
        case gui::Event::SessionPauseResume:
            if (session.state() == rc::SessionState::Paused) {
                session.resume();
                sessionTickMs = millis();
                setAudio(true);
            } else if (session.state() == rc::SessionState::Running) {
                session.pause();
                setAudio(false);
            }
            refreshStatus();
            break;
        case gui::Event::SessionStop:
            // A paused session has nothing to fade: it ends at once.
            if (session.state() == rc::SessionState::Paused) {
                session.reset();
                audio.setSessionGain(1.0f);
            } else {
                session.stop();
            }
            refreshStatus();
            break;
        case gui::Event::BluetoothDisconnect:
            setAudio(false);
            btaudio::disconnect();
            refreshStatus();
            break;
        default:
            break;
    }
    pollSerial();
    if (settings.mode == rc::Mode::Setup) {
        webui::tick();
        bool toSetup = false;
        if (webui::takeRestartRequest(&toSetup)) {
            delay(400);  // let the answer reach the browser
            switchMode(toSetup ? rc::Mode::Setup : rc::Mode::Play);
        }
    }

    const uint32_t now = millis();
    if (radioPending && now - radioStartedMs > 3000) {
        radioPending = false;
        storage::markRadioStable();
        Serial.println("RADIO stable");
    }
    if (settingsDirty && millis() - settingsChangedMs > kSaveDelayMs) {
        storeUserValues();
    }
    tickSession();
    tickBreath();
    // The next track of the folder follows when one has played to its end.
    if (player::takeEnded()) {
        char next[sdcard::kNameLength];
        if (!trackOnce && sdcard::nextFile(trackDir, trackName, next, sizeof(next))) playTrack(trackDir, next);
    }
    ++loopsThisSecond;
    if (now - lastStatusMs >= 1000) {
        lastStatusMs = now;
        loopsLastSecond = loopsThisSecond;
        loopsThisSecond = 0;
        wifilink::tick();
        tickState();
        // Card sizes are re-read now and then, but never while a track plays.
        if (++statusTicks % 10 == 0 && player::state() != player::State::Playing) sdcard::refresh();
        refreshStatus();
    }
    delay(15);
}
