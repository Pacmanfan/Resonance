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
        // The board-definition files use 24 MHz. 40 MHz halves every
        // drawing time (docs/audits/display-refresh-audit-2026-11-18.md);
        // go back to 24 MHz if the picture shows noise or wrong colours.
        cfg.freq_write = 40000000;
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
        // Polled, not interrupt-driven: with an interrupt pin configured the
        // driver reads the controller only while that line is low, and on
        // this board IO21 idles high (10 k pull-up), so no touch was ever
        // read. IO21 is left alone.
        cfg.pin_int = -1;
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

Display::TouchInfo Display::touchInfo() {
    TouchInfo info = {};
    if (_touchAddress == 0) return info;
    uint8_t reg[2];
    uint8_t buf[6];
    reg[0] = 0x81; reg[1] = 0x40;  // product id, 4 characters
    bool ok = lgfx::i2c::transactionWriteRead(kTouchI2cPort, _touchAddress, reg, 2, buf, 4, kTouchI2cHz).has_value();
    memcpy(info.product, buf, 4);
    info.product[4] = '\0';
    for (char& c : info.product) {
        if (c != '\0' && (c < 0x20 || c > 0x7e)) c = '?';
    }
    reg[0] = 0x80; reg[1] = 0x48;  // X resolution, Y resolution (little endian), touch number
    ok = ok && lgfx::i2c::transactionWriteRead(kTouchI2cPort, _touchAddress, reg, 2, buf, 5, kTouchI2cHz).has_value();
    info.width = static_cast<uint16_t>(buf[0] | (buf[1] << 8));
    info.height = static_cast<uint16_t>(buf[2] | (buf[3] << 8));
    info.maxPoints = buf[4] & 0x0F;
    reg[0] = 0x81; reg[1] = 0x4E;
    ok = ok && lgfx::i2c::transactionWriteRead(kTouchI2cPort, _touchAddress, reg, 2, buf, 1, kTouchI2cHz).has_value();
    info.status = buf[0];
    info.ok = ok;
    return info;
}

bool Display::rawTouch(int32_t* x, int32_t* y) {
    lgfx::touch_point_t point;
    if (getTouchRaw(&point, 1) == 0) return false;
    *x = point.x;
    *y = point.y;
    return true;
}

void Display::setWriteClock(uint32_t hz) {
    auto cfg = _bus.config();
    cfg.freq_write = hz;
    _bus.config(cfg);
}

void Display::setBacklightPercent(uint8_t percent) {
    if (percent > 100) percent = 100;
    setBrightness(static_cast<uint8_t>((percent * 255u) / 100u));
}

}  // namespace gui
