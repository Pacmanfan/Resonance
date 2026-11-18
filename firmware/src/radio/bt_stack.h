// Bluetooth memory. The stack itself is started by the audio output
// (audio/bt_audio) in Play mode; in Setup mode its reserved memory is handed
// back to the heap instead (ADR-000204).
#pragma once

namespace btstack {

// Gives the memory reserved for Bluetooth back to the heap. One-way: after
// this call Bluetooth cannot be started until the next restart.
void releaseMemory();

}  // namespace btstack
