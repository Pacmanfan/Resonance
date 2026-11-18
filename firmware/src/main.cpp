// Resonance firmware entry point (docs/architecture/ARCHITECTURE.md).
//
// Stage 2: board foundation. The device starts in the stored operating mode
// (ADR-000204): Play starts Bluetooth and never Wi-Fi, Setup starts Wi-Fi and
// never Bluetooth. No audio exists yet, so nothing can start playing.
#include <Arduino.h>

#include "gui/display.h"
#include "gui/ui.h"
#include "hal/board.h"
#include "radio/bt_stack.h"
#include "radio/wifi_link.h"
#include "rc_settings.h"
#include "storage/sdcard.h"
#include "storage/settings_store.h"
#include "version.h"

namespace {

rc::Settings settings;
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
char lineBuffer[120];
uint8_t lineLength = 0;

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
    Serial.printf("REPORT sd %s total %lu MB free %lu MB\n", sd.mounted ? "mounted" : "none",
                  static_cast<unsigned long>(sd.totalMb), static_cast<unsigned long>(sd.freeMb));
    Serial.printf("REPORT bluetooth %s%s\n", btstack::running() ? "running" : "off",
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
        btOk = btstack::begin("Resonance");
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
    status.bluetoothRunning = btstack::running();
    status.wifiState = static_cast<uint8_t>(wifilink::state());
    wifilink::address(status.address, sizeof(status.address));
    status.network = wifilink::networkName();
    status.freeHeap = ESP.getFreeHeap();
    status.resetReason = board::resetReason();
    status.unexpectedReset = board::resetWasUnexpected();
    status.radioBlocked = radioBlocked;
    screen().setStatus(status);
}

// Serial diagnostics, for development (RES-17 "serial diagnostic output").
//   report | heap | scan | radio | mode play | mode setup | wifi <ssid> <password> | wifi clear
void runCommand(char* line) {
    if (strcmp(line, "report") == 0) {
        printReport();
    } else if (strcmp(line, "heap") == 0) {
        printHeap("now");
    } else if (strcmp(line, "scan") == 0) {
        const int n = wifilink::scanToSerial();
        Serial.printf("SCAN %d network(s)\n", n);
        printHeap("after wifi scan");
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
        Serial.println("commands: report | heap | scan | radio | mode play | mode setup | wifi <ssid> <password> | wifi clear");
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

    displayOk = lcd().begin();
    lcd().setBacklightPercent(settings.brightness);
    screen().begin();
    printHeap("display + ui");

    sdOk = sdcard::begin();
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
    switch (screen().poll()) {
        case gui::Event::SettingsChanged:
            storage::save(settings);
            break;
        case gui::Event::SwitchMode:
            switchMode(settings.mode == rc::Mode::Setup ? rc::Mode::Play : rc::Mode::Setup);
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
    if (now - lastStatusMs >= 1000) {
        lastStatusMs = now;
        wifilink::tick();
        refreshStatus();
    }
    delay(15);
}
