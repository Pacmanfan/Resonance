// The background player (AmbientAudioPlayer): reads a sound file from the SD
// card in its own task, decodes it (WAV, MP3), converts it to 44.1 kHz stereo
// and fills the engine's ring buffer (RES-PLAY-001..003). The audio render
// never touches the card; when the ring runs dry only the background layer
// falls silent.
#pragma once

#include <stdint.h>

#include "rc_audio.h"
#include "rc_ring.h"

namespace player {

enum class State : uint8_t { Stopped, Playing, Paused, Error };

// Creates the decoder task. `engine` and `ring` must outlive it.
void begin(rc::AudioEngine& engine, rc::FrameRing& ring);

// Starts `path` (absolute path on the card) from its beginning.
void play(const char* path);
void pause();
void resume();
void stop();

// How long a track takes to come in when it starts and to go out when it is
// stopped or replaced, in seconds (RES-PLAY-007). A track being replaced
// keeps playing while it goes out. Pause and resume are always quick.
void setFades(float inS, float outS);
float fadeInS();
float fadeOutS();

State state();

// True once after a track has played to its end.
bool takeEnded();

// Why the last track could not be played; empty when there was no error.
const char* error();

// Seconds of the current track that have been heard.
uint32_t positionS();

// Length of the current track in seconds; 0 when unknown (MP3).
uint32_t durationS();

}  // namespace player
