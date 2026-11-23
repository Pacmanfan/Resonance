// Bluetooth memory. The Arduino start-up code hands all memory reserved for
// Bluetooth back to the heap unless this hook says that Bluetooth is in use.
// It always is: Bluetooth audio is the device's output (audio/bt_audio).
extern "C" bool btInUse(void) { return true; }
