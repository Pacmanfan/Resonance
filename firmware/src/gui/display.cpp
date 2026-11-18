#include "display.h"

#include "../hal/board_pins.h"

namespace gui {

namespace {
constexpr int kTouchI2cPort = 1;
constexpr uint32_t kTouchI2cHz = 400000;

// True when a device acknowledges `address` on the touch bus.
bool touchAnswers(uint8_t address) {
    const bool began = lgfx::i2c::beginTransaction(kTouchI2cPort, address, kTouchI2cHz, false).has_value();
    const bool ended = lgfx::i2c::endTransaction(kTouchI2cPort).has_value();
    return began && ended;
}
}  // namespace

Display::Display() {
    {
        auto cfg = _bus.config();
        cfg.spi_host = HSPI_HOST;
        cfg.spi_mode = 0;
        // 24 MHz is what the board-definition files use; a higher clock is
        // not adopted before it has been tested on the panel.
        cfg.freq_write = 24000000;
        cfg.freq_read = 8000000;
        cfg.spi_3wire = false;
        cfg.use_lock = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        cfg.pin_sclk = pins::kLcdSclk;
        cfg.pin_mosi = pins::kLcdMosi;
        cfg.pin_miso = pins::kLcdMiso;
        cfg.pin_dc = pins::kLcdDc;
        _bus.config(cfg);
        _panel.setBus(&_bus);
    }
    {
        auto cfg = _panel.config();
        cfg.pin_cs = pins::kLcdCs;
        cfg.pin_rst = -1;  // tied to EN on the board
        cfg.pin_busy = -1;
        cfg.panel_width = 240;
        cfg.panel_height = 320;
        cfg.readable = true;
        cfg.invert = true;      // IPS panel
        cfg.rgb_order = false;
        cfg.bus_shared = false;  // the SD card has its own SPI bus
        _panel.config(cfg);
    }
    {
        auto cfg = _light.config();
        cfg.pin_bl = pins::kLcdBacklight;
        cfg.invert = false;
        cfg.freq = 12000;
        cfg.pwm_channel = 7;
        _light.config(cfg);
        _panel.setLight(&_light);
    }
    {
        auto cfg = _touch.config();
        cfg.x_min = 0;
        cfg.x_max = 239;
        cfg.y_min = 0;
        cfg.y_max = 319;
        cfg.pin_int = pins::kTouchInt;
        cfg.pin_rst = pins::kTouchRst;
        cfg.bus_shared = false;
        cfg.offset_rotation = 0;
        cfg.i2c_port = kTouchI2cPort;
        cfg.i2c_addr = 0x5D;
        cfg.pin_sda = pins::kTouchSda;
        cfg.pin_scl = pins::kTouchScl;
        cfg.freq = kTouchI2cHz;
        _touch.config(cfg);
        _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
}

bool Display::begin() {
    if (!init()) return false;
    setRotation(0);  // portrait
    // Read Display ID (0x04): one dummy byte, then three identification bytes,
    // packed first byte lowest.
    _panelId = _panel.readCommand(0x04, 1, 3) & 0x00FFFFFF;
    // The GT911 takes its address from the level of INT at reset, so both
    // possible addresses are probed rather than assumed.
    if (touchAnswers(0x5D)) _touchAddress = 0x5D;
    else if (touchAnswers(0x14)) _touchAddress = 0x14;
    return true;
}

void Display::setBacklightPercent(uint8_t percent) {
    if (percent > 100) percent = 100;
    setBrightness(static_cast<uint8_t>((percent * 255u) / 100u));
}

}  // namespace gui
