#include "mindfield.h"

#include <Arduino.h>
#include <atomic>
#include <new>

#include "rng_sources.h"
#include "storage/sdcard.h"
#include "storage/settings_store.h"

namespace mindfield {

namespace {

static_assert(portTICK_PERIOD_MS == 1, "the sampling schedule counts 1 ms ticks");

const char kRawFormat[] = "/RESONANCE/MINDFIELD/EXPERIMENTS/E%05lu.RMF";
const char kResultFormat[] = "/RESONANCE/MINDFIELD/RESULTS/E%05lu.TXT";
const char kRawDir[] = "/RESONANCE/MINDFIELD/EXPERIMENTS";
const char kResultDir[] = "/RESONANCE/MINDFIELD/RESULTS";
const char kCsvPath[] = "/RESONANCE/MINDFIELD/EXPORTS/SUMMARY.CSV";

constexpr uint32_t kRing = 512;      // samples between the sampler and the main loop
constexpr uint16_t kBlock = 128;     // samples in one record of the raw file
constexpr uint32_t kRecoverMaxBytes = 4u * 1024u * 1024u;

// Everything one experiment needs, taken from the heap when it starts and
// given back when it ends: none of it costs memory while Mindfield is idle.
struct Run {
    // Sampler -> main loop. The sampler writes `head`, the main loop `tail`.
    uint32_t ring[kRing];
    std::atomic<uint32_t> head{0};
    std::atomic<uint32_t> tail{0};
    std::atomic<uint32_t> lost{0};        // samples that could not be taken or stored
    std::atomic<uint32_t> scheduled{0};   // sampling slots that have come due
    std::atomic<bool> paused{false};
    std::atomic<bool> quit{false};
    std::atomic<bool> exited{false};
    std::atomic<bool> complete{false};    // the planned number of slots has passed
    std::atomic<bool> sourceFailed{false};
    rc::RngSource* source = nullptr;
    uint16_t rate = 100;
    uint32_t planned = 0;
    // Main loop only.
    uint32_t block[kBlock];
    uint16_t blockFill = 0;
    uint8_t bytes[rc::kMindRecordHead + kBlock * 4];
    rc::MindStats stats;
    rc::MindFileWriter writer;
    fs::File file;
    bool recording = false;
    bool cardFailed = false;
    uint32_t lastBlockMs = 0;
    uint32_t lastFlushMs = 0;
};

Run* run = nullptr;

// A recorded file being played again.
struct Playback final : rc::MindFileVisitor {
    fs::File file;
    rc::MindFileReader reader;
    rc::MindStats stats;
    uint32_t lastSample = 0;
    uint32_t gaps = 0;
    void onSample(uint32_t value) override {
        stats.add(value);
        lastSample = value;
    }
    void onGap(uint32_t) override { ++gaps; }
};
Playback* playback = nullptr;

Live liveNow;
rc::MindHeader headerNow;
rc::MindSummary summaryNow;
rc::MindEnd reasonNow = rc::MindEnd::Completed;
const char* messageNow = "";

void sampler(void* argument) {
    Run* r = static_cast<Run*>(argument);
    TickType_t wake = xTaskGetTickCount();
    uint32_t due = 0;       // thousandths of a sample
    uint32_t failures = 0;  // in a row
    while (!r->quit.load(std::memory_order_relaxed)) {
        // One tick is one millisecond. If this task is kept from running the
        // ticks are made up one after another, so the count of slots follows
        // the clock, not the load.
        vTaskDelayUntil(&wake, 1);
        if (r->paused.load(std::memory_order_relaxed) || r->complete.load(std::memory_order_relaxed)) continue;
        due += r->rate;
        while (due >= 1000) {
            due -= 1000;
            const uint32_t slot = r->scheduled.fetch_add(1, std::memory_order_relaxed) + 1;
            uint32_t value = 0;
            if (!r->source->collect(&value, 1)) {
                r->lost.fetch_add(1, std::memory_order_relaxed);
                // A second without a single sample: the source is gone.
                if (++failures >= r->rate) r->sourceFailed.store(true, std::memory_order_relaxed);
            } else {
                failures = 0;
                const uint32_t head = r->head.load(std::memory_order_relaxed);
                const uint32_t next = (head + 1) % kRing;
                if (next == r->tail.load(std::memory_order_acquire)) {
                    r->lost.fetch_add(1, std::memory_order_relaxed);  // the main loop fell behind
                } else {
                    r->ring[head] = value;
                    r->head.store(next, std::memory_order_release);
                }
            }
            if (r->planned != 0 && slot >= r->planned) {
                r->complete.store(true, std::memory_order_relaxed);
                break;
            }
        }
    }
    r->exited.store(true, std::memory_order_release);
    vTaskDelete(nullptr);
}

void pathOf(const char* format, uint32_t id, char* out, size_t capacity) {
    snprintf(out, capacity, format, static_cast<unsigned long>(id));
}

bool writeBytes(size_t length) {
    if (!run->recording || length == 0) return true;
    if (run->file.write(run->bytes, length) == length) return true;
    run->recording = false;
    run->cardFailed = true;
    return false;
}

void flushBlock() {
    if (run->blockFill == 0) return;
    writeBytes(run->writer.samples(run->block, run->blockFill, run->bytes, sizeof(run->bytes)));
    run->blockFill = 0;
    run->lastBlockMs = millis();
}

void writeGap(uint32_t lost) {
    flushBlock();
    writeBytes(run->writer.gap(lost, run->bytes, sizeof(run->bytes)));
    ++liveNow.gaps;
}

void drain() {
    uint32_t tail = run->tail.load(std::memory_order_relaxed);
    const uint32_t head = run->head.load(std::memory_order_acquire);
    while (tail != head) {
        const uint32_t value = run->ring[tail];
        tail = (tail + 1) % kRing;
        const uint32_t windows = run->stats.windows();
        run->stats.add(value);
        if (run->stats.windows() != windows) liveNow.windowSample = value;
        run->block[run->blockFill++] = value;
        if (run->blockFill == kBlock) flushBlock();
    }
    run->tail.store(tail, std::memory_order_release);
    const uint32_t lost = run->lost.exchange(0, std::memory_order_relaxed);
    if (lost != 0) writeGap(lost);
}

void refreshLive() {
    const uint64_t samples = run->stats.samples();
    liveNow.samples = samples > 0xFFFFFFFFull ? 0xFFFFFFFFu : static_cast<uint32_t>(samples);
    liveNow.elapsedS = run->scheduled.load(std::memory_order_relaxed) / run->rate;
    liveNow.windows = run->stats.windows();
    liveNow.z = static_cast<float>(run->stats.z());
    liveNow.directedZ = static_cast<float>(run->stats.directedZ());
    liveNow.windowZ = static_cast<float>(run->stats.lastWindowZ());
    liveNow.recorded = run->recording;
}

bool writeSummaryFile(uint32_t id, const rc::MindHeader& h, const rc::MindSummary& s, rc::MindEnd reason, uint32_t gaps) {
    char path[56], temporary[56];
    pathOf(kResultFormat, id, path, sizeof(path));
    snprintf(temporary, sizeof(temporary), "/RESONANCE/MINDFIELD/RESULTS/E%05lu.TMP", static_cast<unsigned long>(id));
    fs::File file = sdcard::openForWrite(temporary);
    if (!file) return false;
    char line[rc::kMindSummaryLineMax];
    bool ok = true;
    for (uint8_t i = 0; ok && rc::mindSummaryLine(h, s, reason, gaps, i, line, sizeof(line)); ++i) {
        const size_t length = strlen(line);
        ok = file.write(reinterpret_cast<const uint8_t*>(line), length) == length && file.write('\n') == 1;
    }
    file.close();
    if (!ok) {
        sdcard::remove(temporary);
        return false;
    }
    if (sdcard::exists(path)) sdcard::remove(path);
    return sdcard::rename(temporary, path);
}

void finish(rc::MindEnd reason) {
    // The sampler first: after it has gone nothing more arrives.
    run->quit.store(true, std::memory_order_relaxed);
    for (int i = 0; i < 200 && !run->exited.load(std::memory_order_acquire); ++i) delay(1);
    drain();
    flushBlock();
    if (run->cardFailed) reason = rc::MindEnd::CardFailed;
    const bool wasRecording = run->recording;
    if (run->recording) writeBytes(run->writer.end(reason, run->bytes, sizeof(run->bytes)));
    if (run->file) run->file.close();
    run->stats.summary(summaryNow);
    refreshLive();
    reasonNow = reason;
    liveNow.recorded = wasRecording;
    if (wasRecording && !writeSummaryFile(headerNow.experimentId, headerNow, summaryNow, reason, liveNow.gaps)) {
        messageNow = "Raw data stored; summary not written";
    } else if (run->cardFailed) {
        messageNow = "The card stopped taking data";
    } else if (reason == rc::MindEnd::SourceFailed) {
        messageNow = "The source stopped delivering";
    } else {
        messageNow = wasRecording ? "" : "Not recorded: no card";
    }
    run->source->end();
    run->~Run();
    free(run);
    run = nullptr;
    liveNow.state = State::Finished;
}

bool readLine(fs::File& file, char* out, size_t capacity) {
    size_t n = 0;
    bool any = false;
    while (file.available()) {
        const int c = file.read();
        if (c < 0) break;
        any = true;
        if (c == '\n') break;
        if (n + 1 < capacity) out[n++] = static_cast<char>(c);
    }
    out[n] = '\0';
    return any;
}

// "E00042.TXT" -> 42; 0 for any other name.
uint32_t idOfName(const char* name, const char* extension) {
    if (name[0] != 'E' && name[0] != 'e') return 0;
    char* end = nullptr;
    const unsigned long id = strtoul(name + 1, &end, 10);
    if (end == name + 1 || strcasecmp(end, extension) != 0) return 0;
    return static_cast<uint32_t>(id);
}

// The newest ids of a directory, largest first.
struct IdList {
    uint32_t ids[kHistoryMax];
    uint8_t count;
    const char* extension;
};

bool collectId(const char* name, uint32_t, void* context) {
    IdList* list = static_cast<IdList*>(context);
    const uint32_t id = idOfName(name, list->extension);
    if (id == 0) return true;
    // Insertion into a list kept sorted, largest first; the smallest falls off.
    uint8_t at = 0;
    while (at < list->count && list->ids[at] > id) ++at;
    if (at >= kHistoryMax) return true;
    if (list->count < kHistoryMax) ++list->count;
    for (uint8_t i = list->count - 1; i > at; --i) list->ids[i] = list->ids[i - 1];
    list->ids[at] = id;
    return true;
}

bool readRecord(uint32_t id, rc::MindRecord& out) {
    char path[56];
    pathOf(kResultFormat, id, path, sizeof(path));
    fs::File file = sdcard::openForRead(path);
    if (!file) return false;
    rc::MindRecordParser parser;
    parser.begin();
    char line[rc::kMindSummaryLineMax];
    for (int lines = 0; lines < 64 && readLine(file, line, sizeof(line)); ++lines) {
        if (!parser.line(line)) break;
    }
    file.close();
    return parser.finish(out);
}

struct StatsVisitor final : rc::MindFileVisitor {
    rc::MindStats* stats = nullptr;
    rc::MindFileVisitor* also = nullptr;
    void onSample(uint32_t value) override {
        stats->add(value);
        if (also != nullptr) also->onSample(value);
    }
    void onGap(uint32_t lost) override {
        if (also != nullptr) also->onGap(lost);
    }
    void onPause(bool resumed) override {
        if (also != nullptr) also->onPause(resumed);
    }
};

}  // namespace

bool start(const Setup& setup, const Context& context) {
    if (run != nullptr) return (messageNow = "An experiment is running"), false;
    if (playback != nullptr) return (messageNow = "A replay is running"), false;
    if (setup.source >= rngsources::kCount) return (messageNow = "No such source"), false;
    rc::RngSource& source = rngsources::at(setup.source);
    if (!source.available()) return (messageNow = source.status()), false;
    bool rateKnown = false;
    for (uint16_t rate : kRates) rateKnown = rateKnown || rate == setup.rate;
    if (!rateKnown || setup.rate > source.maxRate()) return (messageNow = "This source is not validated at that rate"), false;

    rc::MindHeader h;
    h.source = source.id();
    h.sourceClass = source.rngClass();
    h.condition = setup.condition;
    h.mode = setup.mode;
    h.target = setup.target;
    h.viz = setup.viz;
    h.audioOn = context.audioOn;
    h.rate = setup.rate;
    h.windowSamples = static_cast<uint32_t>(setup.rate) * kWindowSeconds;
    h.plannedSeconds = setup.seconds;
    h.startUnix = 0;  // the device has no clock
    strlcpy(h.firmware, context.firmware, sizeof(h.firmware));
    strlcpy(h.audio, context.audio, sizeof(h.audio));
    h.primaryHz = context.primaryHz;
    h.beatHz = context.beatHz;
    if (source.id() == rc::RngId::Software) {
        rngsources::software().setSeed(setup.seed);
        h.seed = setup.seed;
    }
    if (!rc::isValidMindHeader(h)) return (messageNow = "A directed experiment needs a target; an open one has none"), false;

    // Room on the card for the whole run, and some to spare.
    const sdcard::Info card = sdcard::info();
    const uint32_t seconds = setup.seconds != 0 ? setup.seconds : 3600;
    const uint32_t needMb = static_cast<uint32_t>((static_cast<uint64_t>(setup.rate) * 4u * seconds) >> 20) + 2;
    if (card.mounted && card.freeMb < needMb) return (messageNow = "Not enough room on the card"), false;

    void* memory = malloc(sizeof(Run));
    if (memory == nullptr) return (messageNow = "Not enough memory to collect now"), false;
    Run* r = new (memory) Run();
    if (!source.begin()) {
        messageNow = source.status();
        r->~Run();
        free(r);
        return false;
    }
    h.physicalEntropy = source.physicalEntropy();
    h.experimentId = storage::takeExperimentId();
    r->source = &source;
    r->rate = setup.rate;
    r->planned = static_cast<uint32_t>(setup.rate) * setup.seconds;
    r->stats.begin(h.windowSamples, h.target);
    r->writer.begin();
    h.startUptimeMs = millis();
    if (card.mounted) {
        char path[56];
        pathOf(kRawFormat, h.experimentId, path, sizeof(path));
        // A number already used on this card (it came from another device):
        // go on to the next free one rather than replace anything.
        for (int tries = 0; tries < 1000 && sdcard::exists(path); ++tries) {
            h.experimentId = storage::takeExperimentId();
            pathOf(kRawFormat, h.experimentId, path, sizeof(path));
        }
        if (!sdcard::exists(path)) r->file = sdcard::openForWrite(path);
        r->recording = static_cast<bool>(r->file);
    }
    run = r;
    headerNow = h;
    summaryNow = rc::MindSummary();
    liveNow = Live();
    liveNow.id = h.experimentId;
    liveNow.plannedSamples = r->planned;
    liveNow.recorded = r->recording;
    messageNow = r->recording ? "" : "Not recorded: no card";
    writeBytes(r->writer.header(h, r->bytes, sizeof(r->bytes)));
    r->lastBlockMs = r->lastFlushMs = millis();

    // Above the interface (1), below the file player (4): the pictures can
    // never hold a sample back, and collecting can never hold the sound back.
    if (xTaskCreatePinnedToCore(sampler, "mindfield", 3072, r, 2, nullptr, 1) != pdPASS) {
        if (r->file) r->file.close();
        source.end();
        r->~Run();
        free(r);
        run = nullptr;
        return (messageNow = "Not enough memory to collect now"), false;
    }
    liveNow.state = State::Running;
    return true;
}

void pause() {
    if (run == nullptr || liveNow.state != State::Running) return;
    run->paused.store(true, std::memory_order_relaxed);
    drain();
    flushBlock();
    writeBytes(run->writer.pause(false, run->bytes, sizeof(run->bytes)));
    liveNow.state = State::Paused;
}

void resume() {
    if (run == nullptr || liveNow.state != State::Paused) return;
    writeBytes(run->writer.pause(true, run->bytes, sizeof(run->bytes)));
    run->paused.store(false, std::memory_order_relaxed);
    liveNow.state = State::Running;
}

namespace {
void endPlayback() {
    playback->stats.summary(summaryNow);
    headerNow = playback->reader.header();
    reasonNow = playback->reader.ended() ? playback->reader.endReason() : rc::MindEnd::Interrupted;
    liveNow.gaps = playback->gaps;
    liveNow.recorded = true;
    liveNow.state = State::Finished;
    messageNow = playback->reader.intact() ? "" : "Raw file interrupted or changed";
    playback->file.close();
    delete playback;
    playback = nullptr;
}

// One window of the recorded file a call.
void tickPlayback() {
    const uint32_t windows = playback->stats.windows();
    uint8_t buffer[64];
    bool more = true;
    while (more && playback->stats.windows() == windows) {
        const int got = playback->file.read(buffer, sizeof(buffer));
        more = got > 0 && playback->reader.feed(buffer, static_cast<size_t>(got)) && !playback->reader.ended();
    }
    const uint64_t samples = playback->stats.samples();
    liveNow.samples = samples > 0xFFFFFFFFull ? 0xFFFFFFFFu : static_cast<uint32_t>(samples);
    liveNow.elapsedS = headerNow.rate != 0 ? liveNow.samples / headerNow.rate : 0;
    liveNow.windows = playback->stats.windows();
    liveNow.z = static_cast<float>(playback->stats.z());
    liveNow.directedZ = static_cast<float>(playback->stats.directedZ());
    liveNow.windowZ = static_cast<float>(playback->stats.lastWindowZ());
    liveNow.windowSample = playback->lastSample;
    if (!more) endPlayback();
}
}  // namespace

bool replayStart(uint32_t id) {
    if (run != nullptr || playback != nullptr) return (messageNow = "An experiment is running"), false;
    char path[56];
    pathOf(kRawFormat, id, path, sizeof(path));
    Playback* p = new (std::nothrow) Playback();
    if (p == nullptr) return (messageNow = "Not enough memory to replay now"), false;
    p->file = sdcard::openForRead(path);
    uint8_t head[rc::kMindHeaderBytes];
    p->reader.begin(p);
    if (!p->file || p->file.read(head, sizeof(head)) != static_cast<int>(sizeof(head)) || !p->reader.feed(head, sizeof(head))) {
        if (p->file) p->file.close();
        delete p;
        return (messageNow = "No raw file for this experiment"), false;
    }
    headerNow = p->reader.header();
    p->stats.begin(headerNow.windowSamples, headerNow.target);
    playback = p;
    liveNow = Live();
    liveNow.id = id;
    liveNow.replaying = true;
    liveNow.recorded = true;
    liveNow.plannedSamples = headerNow.plannedSeconds * headerNow.rate;
    liveNow.state = State::Running;
    messageNow = "";
    return true;
}

void stop() {
    if (run != nullptr) finish(rc::MindEnd::Stopped);
    else if (playback != nullptr) endPlayback();
}

void tick() {
    if (playback != nullptr) {
        tickPlayback();
        return;
    }
    if (run == nullptr) return;
    drain();
    const uint32_t now = millis();
    // What has been collected reaches the card within a second, and the
    // card's own bookkeeping within five: an interruption loses little.
    if (run->blockFill != 0 && now - run->lastBlockMs >= 1000) flushBlock();
    if (run->recording && now - run->lastFlushMs >= 5000) {
        run->file.flush();
        run->lastFlushMs = now;
    }
    refreshLive();
    if (run->cardFailed) {
        finish(rc::MindEnd::CardFailed);
    } else if (run->sourceFailed.load(std::memory_order_relaxed)) {
        finish(rc::MindEnd::SourceFailed);
    } else if (run->complete.load(std::memory_order_relaxed)) {
        finish(rc::MindEnd::Completed);
    }
}

void clear() {
    if (run == nullptr && playback == nullptr) liveNow = Live();
}

const Live& live() { return liveNow; }
const rc::MindHeader& header() { return headerNow; }
const rc::MindSummary& summary() { return summaryNow; }
rc::MindEnd endReason() { return reasonNow; }
const char* message() { return messageNow; }
const rc::MindStats* stats() { return run != nullptr ? &run->stats : nullptr; }

uint8_t list(uint8_t skip, rc::MindRecord* out, uint8_t capacity, uint8_t* total) {
    IdList ids = {{}, 0, ".TXT"};
    sdcard::visitFiles(kResultDir, collectId, &ids);
    if (total != nullptr) *total = ids.count;
    uint8_t written = 0;
    for (uint8_t i = skip; i < ids.count && written < capacity; ++i) {
        rc::MindRecord record;
        if (!readRecord(ids.ids[i], record)) {
            // A file that cannot be read is still listed: nothing is hidden.
            record = rc::MindRecord();
            record.experimentId = ids.ids[i];
        }
        out[written++] = record;
    }
    return written;
}

bool replay(uint32_t id, rc::MindFileVisitor* visitor, rc::MindHeader& headerOut, rc::MindSummary& out, bool* intact) {
    char path[56];
    pathOf(kRawFormat, id, path, sizeof(path));
    fs::File file = sdcard::openForRead(path);
    if (!file) return false;
    rc::MindStats* stats = new (std::nothrow) rc::MindStats();
    if (stats == nullptr) {
        file.close();
        return false;
    }
    StatsVisitor feed;
    feed.stats = stats;
    feed.also = visitor;
    rc::MindFileReader reader;
    reader.begin(&feed);
    uint8_t buffer[256];
    // The header first: it gives the window and the target the statistics need.
    bool ok = file.read(buffer, rc::kMindHeaderBytes) == static_cast<int>(rc::kMindHeaderBytes) &&
              reader.feed(buffer, rc::kMindHeaderBytes);
    if (ok) {
        stats->begin(reader.header().windowSamples, reader.header().target);
        for (;;) {
            const int got = file.read(buffer, sizeof(buffer));
            if (got <= 0) break;
            if (!reader.feed(buffer, static_cast<size_t>(got))) break;  // what came before stays
        }
        headerOut = reader.header();
        stats->summary(out);
        if (intact != nullptr) *intact = reader.intact();
    }
    file.close();
    delete stats;
    return ok;
}

namespace {
struct RecoverState {
    uint8_t made;
};

bool recoverOne(const char* name, uint32_t size, void* context) {
    const uint32_t id = idOfName(name, ".RMF");
    if (id == 0 || size > kRecoverMaxBytes || size < rc::kMindHeaderBytes) return true;
    if (run != nullptr && id == headerNow.experimentId) return true;  // the one being written
    if (playback != nullptr) return true;
    char result[56];
    pathOf(kResultFormat, id, result, sizeof(result));
    if (sdcard::exists(result)) return true;
    rc::MindHeader h;
    rc::MindSummary s;
    bool intact = false;
    if (!replay(id, nullptr, h, s, &intact)) return true;
    if (writeSummaryFile(id, h, s, rc::MindEnd::Interrupted, 0)) ++static_cast<RecoverState*>(context)->made;
    return true;
}
}  // namespace

uint8_t recover() {
    RecoverState state = {0};
    sdcard::visitFiles(kRawDir, recoverOne, &state);
    return state.made;
}

bool exportCsv(uint8_t* rows) {
    IdList ids = {{}, 0, ".TXT"};
    if (!sdcard::visitFiles(kResultDir, collectId, &ids)) return false;
    fs::File file = sdcard::openForWrite(kCsvPath);
    if (!file) return false;
    char line[200];
    bool ok = rc::mindCsvHeader(line, sizeof(line)) && file.println(line) > 0;
    uint8_t written = 0;
    // Oldest first, as a table is read.
    for (int i = static_cast<int>(ids.count) - 1; ok && i >= 0; --i) {
        rc::MindRecord record;
        if (!readRecord(ids.ids[i], record)) continue;
        ok = rc::mindCsvRow(record, line, sizeof(line)) && file.println(line) > 0;
        if (ok) ++written;
    }
    file.close();
    if (rows != nullptr) *rows = written;
    return ok;
}

}  // namespace mindfield
