// Wi-Fi for Setup mode (ADR-000204): station with the saved network, or the
// set-up access point when none is saved or the connection fails.
#pragma once

#include <stddef.h>

#include "rc_settings.h"

namespace wifilink {

enum class State : uint8_t { Off, Connecting, Station, AccessPoint };

// Starts Wi-Fi according to the settings. Does not block.
void begin(const rc::Settings& s);

// Advances the connection state; call about once a second.
void tick();

State state();

// Writes the address to type into a browser ("192.168.4.1") into `out`.
void address(char* out, size_t capacity);

// Name of the network: the station's SSID or the access point's.
const char* networkName();

// Scans for networks and prints them to the serial port. Starts the radio in
// station mode when it is off. Blocks for a few seconds. Returns the count.
int scanToSerial();

}  // namespace wifilink
