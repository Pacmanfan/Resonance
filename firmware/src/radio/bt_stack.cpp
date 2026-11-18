#include "bt_stack.h"

#include <Arduino.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gap_bt_api.h>

// The Arduino start-up code releases all Bluetooth memory unless this hook
// says that Bluetooth is in use. The decision belongs to the operating mode,
// which is known only after the settings are loaded, so the memory is kept
// here and released by releaseMemory() in Setup mode.
extern "C" bool btInUse(void) { return true; }

namespace btstack {

namespace {
bool started = false;
bool released = false;
}  // namespace

bool begin(const char* deviceName) {
    if (started) return true;
    if (released) return false;

    // Resonance uses Bluetooth Classic only (A2DP).
    esp_bt_controller_mem_release(ESP_BT_MODE_BLE);

    // The controller must be initialised in the mode it is then enabled in;
    // the default configuration is dual mode.
    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    cfg.mode = ESP_BT_MODE_CLASSIC_BT;
    esp_err_t err = esp_bt_controller_init(&cfg);
    const char* step = "controller init";
    if (err == ESP_OK) { step = "controller enable"; err = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT); }
    if (err == ESP_OK) { step = "host init"; err = esp_bluedroid_init(); }
    if (err == ESP_OK) { step = "host enable"; err = esp_bluedroid_enable(); }
    if (err != ESP_OK) {
        Serial.printf("BT %s failed: %s\n", step, esp_err_to_name(err));
        return false;
    }
    esp_bt_gap_set_device_name(deviceName);
    started = true;
    return true;
}

void releaseMemory() {
    if (started || released) return;
    esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);
    released = true;
}

bool running() { return started; }

}  // namespace btstack
