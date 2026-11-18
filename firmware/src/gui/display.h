// Display and touch of the ESP32-2432S032C: ST7789 over SPI, GT911 over I2C,
// backlight by PWM. DisplayManager and TouchManager of the architecture are
// this one object, because LovyanGFX drives panel, light and touch together.
#pragma once

#include <LovyanGFX.hpp>

namespace gui {

class Display : public lgfx::LGFX_Device {
public:
    Display();

    // Initialises panel, backlight and touch. Returns false when the panel
    // does not answer.
    bool begin();

    // Backlight in percent (0-100).
    void setBacklightPercent(uint8_t percent);

    // Identification bytes read back from the panel (command 0x04). A value
    // of 0 or 0xFFFFFF means nothing answered on the bus.
    uint32_t panelId() const { return _panelId; }

    // Display status word (command 0x09), read now. Diagnostics only.
    uint32_t readStatus() { return _panel.readCommand(0x09, 1, 4); }

    // I2C address at which the touch controller answered, 0 when none did.
    uint8_t touchAddress() const { return _touchAddress; }

    // What the touch controller says about itself. Diagnostics only.
    struct TouchInfo {
        bool ok;             // the registers could be read
        char product[5];     // product id text, "911" for a GT911
        uint16_t width;      // resolution the controller is configured for
        uint16_t height;
        uint8_t maxPoints;
        uint8_t status;      // status register 0x814E: bit 7 = data ready, low bits = touch count
    };
    TouchInfo touchInfo();

    // The touch point as the controller reports it, before any scaling or
    // rotation. Returns false when nothing is touched.
    bool rawTouch(int32_t* x, int32_t* y);

private:
    lgfx::Panel_ST7789 _panel;
    lgfx::Bus_SPI _bus;
    lgfx::Light_PWM _light;
    lgfx::Touch_GT911 _touch;
    uint32_t _panelId = 0;
    uint8_t _touchAddress = 0;
};

}  // namespace gui
