// Resonance firmware entry point (docs/architecture/ARCHITECTURE.md).
//
// Stage 2: board foundation. The device starts in the stored operating mode
// (ADR-000204): Play starts Bluetooth and never Wi-Fi, Setup starts Wi-Fi and
// never Bluetooth. No audio exists yet, so nothing can start playing.
#include <Arduino.h>

#include "gui/display.h"
#include "gui/ui.h"
#include "hal/board.h"
#include "audio/bt_audio.h"
#include "audio/player.h"
#include "radio/bt_stack.h"
#include "radio/wifi_link.h"
#include "rc_settings.h"
#include "rc_audio.h"
#include "rc_session.h"
#include "storage/sdcard.h"
#include "storage/settings_store.h"
#include "version.h"

#include <SD.h>

namespace {

rc::Settings settings;
// The engine and the ring that carries decoded sound to it. 4096 frames are
// 93 ms of audio and 16 kB.
constexpr uint32_t kRingFrames = 4096;
int16_t ringStorage[kRingFrames * 2];
rc::FrameRing ring(ringStorage, kRingFrames);
rc::AudioEngine audio(ring);

bool audioOn = false;         // never on at start (RES-CFG-003)
uint8_t volume = 0;           // master, percent; starts at the settings' start-up volume
uint8_t backgroundVolume = 70;
rc::NoiseType noiseType = rc::NoiseType::Off;
uint8_t noiseVolume = 40;

// Sessions. Program 0 is the plain timer; 1.. are the factory sessions.
rc::SessionRunner session;
uint8_t sessionProgram = 0;
uint8_t timerChoice = 2;  // index into kTimerMinutes
uint8_t fadeChoice = 1;   // index into kFadeSeconds
const uint16_t kTimerMinutes[] = {10, 15, 20, 30, 45, 60, 0};  // 0 = continuous
const uint16_t kFadeSeconds[] = {0, 30, 60, 300};
uint32_t sessionTickMs = 0;

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
// Display and interface are built on first use, inside setup(): objects that
// configure drivers must not be constructed before the scheduler runs.
gui::Display& lcd() {
    static gui::Display instance;
    return instance;
}
gui::Ui& screen() {
    static gui::Ui instance(lcd(), settings);
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
char lineBuffer[120];
uint8_t lineLength = 0;

rc::Session chosenSession();  // defined with the session control below

void printHeap(const char* stage) {
    Serial.printf("HEAP %-22s free %6lu  min %6lu  largest %6lu\n", stage,
                  static_cast<unsigned long>(ESP.getFreeHeap()), static_cast<unsigned long>(ESP.getMinFreeHeap()),
                  static_cast<unsigned long>(ESP.getMaxAllocHeap()));
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
    } else {
        audio.tone().setFrequencies(settings.primaryHz, settings.beatHz);
        audio.setMasterVolume(volume);
        audio.setBackgroundVolume(backgroundVolume);
        player::begin(audio, ring);
        btOk = btaudio::begin("Resonance", audio);
        printHeap("bluetooth started");
    }
}

void switchMode(rc::Mode mode) {
    settings.mode = mode;
    storage::save(settings);
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
    status.freeHeap = ESP.getFreeHeap();
    status.resetReason = board::resetReason();
    status.unexpectedReset = board::resetWasUnexpected();
    status.radioBlocked = radioBlocked;
    status.audioOn = audioOn;
    status.volume = volume;
    btaudio::updateList();
    status.btState = static_cast<uint8_t>(btaudio::state());
    strlcpy(status.btPeer, btaudio::peerName(), sizeof(status.btPeer));
    // The folder is shown relative to the library root.
    const char* shown = soundDir + strlen(sdcard::kAudioRoot);
    const size_t shownLength = strlen(shown);
    const size_t fits = sizeof(status.soundDir) - 1;
    strlcpy(status.soundDir, shownLength == 0 ? "/" : (shownLength > fits ? shown + shownLength - fits : shown),
            sizeof(status.soundDir));
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
    return rc::factorySession(static_cast<uint8_t>(sessionProgram - 1));
}

void startSession() {
    if (settings.mode != rc::Mode::Play) return;
    if (!session.start(chosenSession(), settings.primaryHz, settings.beatHz, kFadeSeconds[fadeChoice])) return;
    sessionTickMs = millis();
    audio.setSessionGain(1.0f);
    setAudio(true);  // starting a session is the user turning the audio on
    Serial.printf("SESSION started: %s\n", chosenSession().name);
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
    audio.setSessionGain(session.gain());
    if (session.state() == rc::SessionState::Finished) {
        // Everything stops, then the level is restored for the next start.
        setAudio(false);
        if (player::state() == player::State::Playing) player::pause();
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

// Serial diagnostics, for development (RES-17 "serial diagnostic output").
//   report | heap | scan | radio | mktest | put <path> <bytes> | play <path> | player | stop | session | session start <n> | session stop | bt | bt connect <n> | audio on | audio off | mode play | mode setup | wifi <ssid> <password> | wifi clear
void runCommand(char* line) {
    if (strcmp(line, "report") == 0) {
        printReport();
    } else if (strcmp(line, "heap") == 0) {
        printHeap("now");
    } else if (strcmp(line, "scan") == 0) {
        const int n = wifilink::scanToSerial();
        Serial.printf("SCAN %d network(s)\n", n);
        printHeap("after wifi scan");
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
                      static_cast<unsigned>(session.state()), chosenSession().name,
                      static_cast<unsigned>(session.stageIndex() + 1), static_cast<unsigned>(session.stageCount()),
                      static_cast<unsigned long>(session.remainingS()), static_cast<double>(settings.beatHz),
                      static_cast<double>(session.gain()));
    } else if (strncmp(line, "session start ", 14) == 0) {
        sessionProgram = static_cast<uint8_t>(atoi(line + 14)) % (rc::factorySessionCount() + 1);
        startSession();
    } else if (strcmp(line, "session stop") == 0) {
        session.stop();
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
        Serial.println("commands: report | heap | scan | radio | mktest | put <path> <bytes> | play <path> | player | stop | session | session start <n> | session stop | bt | bt connect <n> | audio on | audio off | mode play | mode setup | wifi <ssid> <password> | wifi clear");
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
    volume = settings.startVolume;

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

    refreshStatus();
    printReport();
}

void loop() {
    const gui::Event event = screen().poll();
    switch (event) {
        case gui::Event::SettingsChanged:
            storage::save(settings);
            break;
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
            refreshStatus();
            break;
        case gui::Event::SessionDurationNext:
            timerChoice = static_cast<uint8_t>((timerChoice + 1) % (sizeof(kTimerMinutes) / sizeof(kTimerMinutes[0])));
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

    const uint32_t now = millis();
    if (radioPending && now - radioStartedMs > 3000) {
        radioPending = false;
        storage::markRadioStable();
        Serial.println("RADIO stable");
    }
    tickSession();
    // The next track of the folder follows when one has played to its end.
    if (player::takeEnded()) {
        char next[sdcard::kNameLength];
        if (sdcard::nextFile(trackDir, trackName, next, sizeof(next))) playTrack(trackDir, next);
    }
    if (now - lastStatusMs >= 1000) {
        lastStatusMs = now;
        wifilink::tick();
        // Card sizes are re-read now and then, but never while a track plays.
        if (++statusTicks % 10 == 0 && player::state() != player::State::Playing) sdcard::refresh();
        refreshStatus();
    }
    delay(15);
}
