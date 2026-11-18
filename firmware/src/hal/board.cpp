#include "board.h"

#include <Arduino.h>
#include <esp_system.h>

#include "board_pins.h"

namespace board {

void begin() {
    // The onboard amplifier is always enabled and amplifies whatever is on
    // this pin, so it is driven to a steady low level.
    pinMode(pins::kSpeaker, OUTPUT);
    digitalWrite(pins::kSpeaker, LOW);

    pinMode(pins::kLedRed, OUTPUT);
    pinMode(pins::kLedGreen, OUTPUT);
    pinMode(pins::kLedBlue, OUTPUT);
    setLed(false, false, false);
}

void setLed(bool red, bool green, bool blue) {
    digitalWrite(pins::kLedRed, red ? LOW : HIGH);
    digitalWrite(pins::kLedGreen, green ? LOW : HIGH);
    digitalWrite(pins::kLedBlue, blue ? LOW : HIGH);
}

const char* resetReason() {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON: return "power-on";
        case ESP_RST_EXT: return "reset pin";
        case ESP_RST_SW: return "software restart";
        case ESP_RST_PANIC: return "panic";
        case ESP_RST_INT_WDT: return "interrupt watchdog";
        case ESP_RST_TASK_WDT: return "task watchdog";
        case ESP_RST_WDT: return "watchdog";
        case ESP_RST_DEEPSLEEP: return "deep sleep";
        case ESP_RST_BROWNOUT: return "brown-out";
        default: return "unknown";
    }
}

bool resetWasUnexpected() {
    switch (esp_reset_reason()) {
        case ESP_RST_PANIC:
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT:
        case ESP_RST_BROWNOUT:
            return true;
        default:
            return false;
    }
}

}  // namespace board
