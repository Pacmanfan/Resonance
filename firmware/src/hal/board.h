// Board-level set-up that belongs to no single peripheral.
#pragma once

#include <stdint.h>

namespace board {

// Puts every pin without an owner into a safe, quiet state. Call first.
void begin();

// Status LED. Each colour is on or off; the LED is active low on the board.
void setLed(bool red, bool green, bool blue);

// Why the processor last started, as short text ("power-on", "panic", ...).
const char* resetReason();

// True when the last start was not requested (brown-out, watchdog, panic).
bool resetWasUnexpected();

}  // namespace board
