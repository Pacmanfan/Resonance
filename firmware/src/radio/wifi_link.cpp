#include "wifi_link.h"

#include <ESPmDNS.h>
#include <WiFi.h>

namespace wifilink {

namespace {
const char kApName[] = "Resonance-Setup";
constexpr uint32_t kStationTimeoutMs = 15000;

State current = State::Off;
uint32_t startedMs = 0;
const rc::Settings* settings = nullptr;

void startAccessPoint() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(kApName, settings->apPass);
    current = State::AccessPoint;
    MDNS.begin(settings->hostname);
}
}  // namespace

void begin(const rc::Settings& s) {
    settings = &s;
    WiFi.persistent(false);  // credentials live in our own settings record
    WiFi.setHostname(s.hostname);
    if (s.wifiSsid[0] == '\0') {
        startAccessPoint();
        return;
    }
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(s.wifiSsid, s.wifiPass);
    startedMs = millis();
    current = State::Connecting;
}

void tick() {
    if (settings == nullptr) return;
    if (current == State::Connecting) {
        if (WiFi.status() == WL_CONNECTED) {
            current = State::Station;
            MDNS.begin(settings->hostname);
        } else if (millis() - startedMs > kStationTimeoutMs) {
            // Fallback access point (RES-NET-002).
            WiFi.disconnect(true);
            startAccessPoint();
        }
    }
}

State state() { return current; }

void address(char* out, size_t capacity) {
    IPAddress ip;
    if (current == State::Station) ip = WiFi.localIP();
    else if (current == State::AccessPoint) ip = WiFi.softAPIP();
    snprintf(out, capacity, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

const char* networkName() {
    if (current == State::AccessPoint) return kApName;
    if (settings != nullptr && current != State::Off) return settings->wifiSsid;
    return "";
}

int scanToSerial() {
    if (WiFi.getMode() == WIFI_OFF) WiFi.mode(WIFI_STA);
    const int n = WiFi.scanNetworks();
    for (int i = 0; i < n; ++i) {
        Serial.printf("  %-32s %4d dBm ch %d\n", WiFi.SSID(i).c_str(), (int)WiFi.RSSI(i), (int)WiFi.channel(i));
    }
    WiFi.scanDelete();
    return n;
}

}  // namespace wifilink
