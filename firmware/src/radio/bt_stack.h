// The Bluetooth Classic stack. Started only in Play mode (ADR-000204); in
// Setup mode its reserved memory is handed back to the heap instead.
#pragma once

namespace btstack {

// Starts the controller in Classic-only mode and the host stack. BLE is not
// started and its memory is released. Returns false on failure.
bool begin(const char* deviceName);

// Gives the memory reserved for Bluetooth back to the heap. One-way: after
// this call Bluetooth cannot be started until the next restart.
void releaseMemory();

bool running();

}  // namespace btstack
