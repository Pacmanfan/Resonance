#include "bt_stack.h"

#include <Arduino.h>
#include <esp_bt.h>

// The Arduino start-up code releases all Bluetooth memory unless this hook
// says that Bluetooth is in use. The decision belongs to the operating mode,
// which is known only after the settings are loaded, so the memory is kept
// here and released by releaseMemory() in Setup mode.
extern "C" bool btInUse(void) { return true; }

namespace btstack {

void releaseMemory() {
    static bool released = false;
    if (released) return;
    esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);
    released = true;
}

}  // namespace btstack
