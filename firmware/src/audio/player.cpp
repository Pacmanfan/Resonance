#include "player.h"

#include <Arduino.h>
#include <MP3DecoderHelix.h>
#include <SD.h>

#include "../storage/sdcard.h"
#include "rc_resample.h"
#include "rc_wav.h"

namespace player {

namespace {

enum class CommandType : uint8_t { Play, Pause, Resume, Stop };
struct Command {
    CommandType type;
    char path[160];
};

// Input is handled in small pieces so that the buffers stay small whatever
// the file's sample rate: at most kPieceFrames in, and the resampler's
// output for them (up to 5.6 times as many when an 8 kHz file is raised to
// 44.1 kHz) out.
constexpr size_t kPieceFrames = 96;
constexpr size_t kOutputFrames = 560;
constexpr size_t kReadBytes = kPieceFrames * 8;  // 32-bit stereo is the widest frame

rc::AudioEngine* engine = nullptr;
rc::FrameRing* ring = nullptr;
QueueHandle_t commands = nullptr;

volatile State current = State::Stopped;
volatile bool ended = false;
volatile uint32_t trackSeconds = 0;
const char* lastError = "";

// Owned by the task.
File file;
rc::Resampler resampler;
uint8_t readBuffer[kReadBytes];
int16_t pieceBuffer[kPieceFrames * 2];
int16_t outputBuffer[kOutputFrames * 2];
bool abortTrack = false;  // a new command arrived while a track was being fed
bool leaving = false;     // the track is fading out to make way for a command
uint32_t leavingSinceMs = 0;
volatile float fadeIn = rc::kPlayFadeS;
volatile float fadeOut = rc::kPlayFadeS;
int mp3Rate = 0;          // sample rate of the MP3 being decoded

class FileSource : public rc::ByteSource {
public:
    size_t read(uint8_t* out, size_t count) override { return file.read(out, count); }
    bool seek(uint32_t position) override { return file.seek(position); }
};

// A command is waiting: the current track must give way.
bool commandWaiting() { return uxQueueMessagesWaiting(commands) > 0; }

// A command is waiting: the track goes out over the fade-out time, still
// being decoded, and gives way once it is no longer heard (RES-PLAY-007).
// Without an output nothing renders and nothing reports the fade as over, so
// the wait is bounded by the fade's own length.
bool mustLeave() {
    if (!commandWaiting()) return false;
    if (!leaving) {
        leaving = true;
        leavingSinceMs = millis();
        engine->setBackgroundPlaying(false, fadeOut);
    }
    return engine->backgroundIdle() || millis() - leavingSinceMs > static_cast<uint32_t>(fadeOut * 1000.0f) + 300;
}

// Pushes frames into the ring, waiting while it is full. Returns false when
// a command arrived instead.
bool pushFrames(const int16_t* frames, size_t count) {
    while (count > 0) {
        const uint32_t wrote = ring->write(frames, static_cast<uint32_t>(count));
        frames += wrote * 2;
        count -= wrote;
        if (count > 0) {
            if (mustLeave()) return false;
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
    return true;
}

// Resamples one piece (at most kPieceFrames) and pushes it.
bool pushPiece(const int16_t* frames, size_t count) {
    const size_t made = resampler.process(frames, count, outputBuffer);
    return pushFrames(outputBuffer, made);
}

void fail(const char* reason) {
    lastError = reason;
    current = State::Error;
    Serial.printf("PLAYER error: %s\n", reason);
}

// Lets the background layer fade out and empties the ring, so that a new
// track does not start with the tail of the old one.
void quietAndFlush() {
    engine->setBackgroundPlaying(false);
    for (int i = 0; i < 30 && !engine->backgroundIdle(); ++i) vTaskDelay(pdMS_TO_TICKS(5));
    // The render does not read the ring while the layer is idle, so the
    // reader-side discard is safe from here.
    ring->discard();
}

// Starts the layer once enough is buffered to ride out a slow card read.
void startWhenBuffered(bool& started) {
    if (!started && !leaving && ring->available() >= ring->capacity() / 2) {
        engine->setBackgroundPlaying(true, fadeIn);
        started = true;
    }
}

// Waits until the buffered rest of the track has been heard.
void drain() {
    if (!leaving) engine->setBackgroundPlaying(true);
    engine->setBackgroundEnding();
    while (ring->available() > 0 && !mustLeave()) vTaskDelay(pdMS_TO_TICKS(20));
}

bool playWav() {
    FileSource source;
    rc::WavInfo info;
    const rc::WavError error = rc::readWavHeader(source, info);
    if (error == rc::WavError::NotWav) { fail("not a WAV file"); return false; }
    if (error == rc::WavError::Unsupported) { fail("WAV format not supported"); return false; }
    if (error != rc::WavError::Ok) { fail("WAV file is damaged"); return false; }
    resampler.begin(info.sampleRate, rc::kSampleRate);
    trackSeconds = info.frames() / info.sampleRate;

    uint32_t left = info.dataBytes;
    bool started = false;
    const size_t pieceBytes = kPieceFrames * info.frameBytes();
    while (left > 0) {
        const size_t want = left < pieceBytes ? left : pieceBytes;
        const size_t got = file.read(readBuffer, want);
        if (got == 0) break;  // the file is shorter than its header says, or the card is gone
        left -= got;
        const size_t frames = rc::wavToStereo16(readBuffer, got, info, pieceBuffer);
        if (!pushPiece(pieceBuffer, frames) || mustLeave()) return false;
        startWhenBuffered(started);
    }
    return true;
}

// Called by the MP3 decoder for every decoded frame, inside write().
void onMp3Frame(MP3FrameInfo& info, short* pcm, size_t samples, void* /*ref*/) {
    if (abortTrack || info.nChans < 1 || info.nChans > 2 || info.samprate <= 0) return;
    if (info.samprate != mp3Rate) {  // known only once the first frame is decoded
        mp3Rate = info.samprate;
        resampler.begin(static_cast<uint32_t>(info.samprate), rc::kSampleRate);
    }
    size_t frames = samples / static_cast<size_t>(info.nChans);
    const short* in = pcm;
    while (frames > 0) {
        const size_t n = frames < kPieceFrames ? frames : kPieceFrames;
        for (size_t i = 0; i < n; ++i) {
            pieceBuffer[2 * i] = in[0];
            pieceBuffer[2 * i + 1] = info.nChans == 2 ? in[1] : in[0];
            in += info.nChans;
        }
        if (!pushPiece(pieceBuffer, n)) {
            abortTrack = true;
            return;
        }
        frames -= n;
    }
}

bool playMp3() {
    // The decoder is created for the track and freed after it: its buffers
    // (about 30 kB) are not held while nothing plays.
    libhelix::MP3DecoderHelix* decoder = new (std::nothrow) libhelix::MP3DecoderHelix(onMp3Frame);
    if (decoder == nullptr) { fail("not enough memory for MP3"); return false; }
    decoder->begin();
    mp3Rate = 0;
    trackSeconds = 0;
    abortTrack = false;
    bool started = false;
    bool complete = true;
    for (;;) {
        const size_t got = file.read(readBuffer, sizeof(readBuffer));
        if (got == 0) break;
        decoder->write(readBuffer, got);
        if (abortTrack || mustLeave()) {
            complete = false;
            break;
        }
        startWhenBuffered(started);
    }
    decoder->end();
    delete decoder;
    return complete;
}

void playFile(const char* path) {
    quietAndFlush();
    leaving = false;
    if (file) file.close();
    lastError = "";
    ended = false;
    engine->resetPosition();
    file = SD.open(path, FILE_READ);
    if (!file || file.isDirectory()) {
        fail("cannot open the file");
        return;
    }
    current = State::Playing;
    Serial.printf("PLAYER playing %s\n", path);
    const bool complete = sdcard::isMp3(path) ? playMp3() : playWav();
    file.close();
    if (current == State::Error) return;
    if (complete) {
        drain();
        if (!commandWaiting()) {
            engine->setBackgroundPlaying(false);
            current = State::Stopped;
            ended = true;
        }
    }
}

void taskMain(void*) {
    static Command command;  // too large for the task's stack
    for (;;) {
        if (xQueueReceive(commands, &command, portMAX_DELAY) != pdTRUE) continue;
        switch (command.type) {
            case CommandType::Play:
                playFile(command.path);
                break;
            case CommandType::Stop:
                quietAndFlush();
                leaving = false;
                current = State::Stopped;
                break;
            default:
                break;  // pause and resume are applied by the caller, see below
        }
    }
}

void send(CommandType type, const char* path) {
    static Command command;  // built by the interface task only
    command.type = type;
    strlcpy(command.path, path, sizeof(command.path));
    // One place in the queue: a newer command replaces one not yet taken,
    // so that the last thing asked for is what happens after a long fade.
    xQueueOverwrite(commands, &command);
}

}  // namespace

void begin(rc::AudioEngine& audioEngine, rc::FrameRing& frameRing) {
    engine = &audioEngine;
    ring = &frameRing;
    commands = xQueueCreate(1, sizeof(Command));
    // Above the interface (priority 1), below the Bluetooth stack. Core 1,
    // away from the radio tasks.
    xTaskCreatePinnedToCore(taskMain, "player", 6144, nullptr, 4, nullptr, 1);
}

void play(const char* path) { send(CommandType::Play, path); }

void stop() { send(CommandType::Stop, ""); }

// Pause and resume do not interrupt the decoder task: the engine stops
// reading the ring, the ring fills, and the task waits where it is.
void pause() {
    if (current != State::Playing) return;
    engine->setBackgroundPlaying(false);
    current = State::Paused;
}

void resume() {
    if (current != State::Paused) return;
    engine->setBackgroundPlaying(true);
    current = State::Playing;
}

void setFades(float inS, float outS) {
    if (inS >= 0.0f) fadeIn = inS > rc::kLayerFadeMaxS ? rc::kLayerFadeMaxS : inS;
    if (outS >= 0.0f) fadeOut = outS > rc::kLayerFadeMaxS ? rc::kLayerFadeMaxS : outS;
}

float fadeInS() { return fadeIn; }
float fadeOutS() { return fadeOut; }

State state() { return current; }

bool takeEnded() {
    if (!ended) return false;
    ended = false;
    return true;
}

const char* error() { return lastError; }

uint32_t positionS() { return engine != nullptr ? engine->backgroundFrames() / rc::kSampleRate : 0; }

uint32_t durationS() { return trackSeconds; }

}  // namespace player
