// Mindfield experiment files (RES-MIND-002).
#include "host_test.h"
#include "rc_mindfile.h"

#include <initializer_list>
#include <string>
#include <vector>

using namespace rc;

namespace {

MindHeader header() {
    MindHeader h;
    h.experimentId = 42;
    h.source = RngId::Software;
    h.sourceClass = RngClass::SoftwareControl;
    h.condition = MindCondition::Meditation;
    h.mode = MindMode::Directed;
    h.target = MindTarget::MoreOnes;
    h.viz = MindViz::Orb;
    h.audioOn = true;
    h.rate = 250;
    h.windowSamples = 250;
    h.plannedSeconds = 600;
    h.seed = 777;
    h.startUptimeMs = 123456;
    strcpy(h.firmware, "0.11.0");
    strcpy(h.audio, "Trance 10-4.5 Hz");
    h.primaryHz = 136.1f;
    h.beatHz = 7.8f;
    return h;
}

struct Collect : MindFileVisitor {
    std::vector<uint32_t> samples;
    std::vector<uint32_t> gaps;
    std::vector<bool> pauses;
    void onSample(uint32_t v) override { samples.push_back(v); }
    void onGap(uint32_t lost) override { gaps.push_back(lost); }
    void onPause(bool resumed) override { pauses.push_back(resumed); }
};

// A whole file: `count` samples of the software source in blocks, with a gap
// and a pause in the middle.
std::vector<uint8_t> file(uint32_t count, std::vector<uint32_t>* values = nullptr, bool withEnd = true) {
    std::vector<uint8_t> bytes(kMindHeaderBytes);
    MindFileWriter writer;
    writer.begin();
    CHECK(writer.header(header(), bytes.data(), bytes.size()) == kMindHeaderBytes);
    SoftwareRng rng(777);
    rng.begin();
    uint8_t block[kMindRecordHead + kMindBlockSamples * 4];
    uint32_t done = 0;
    while (done < count) {
        uint32_t chunk[100];
        const uint16_t n = static_cast<uint16_t>(count - done < 100 ? count - done : 100);
        rng.collect(chunk, n);
        if (values != nullptr) values->insert(values->end(), chunk, chunk + n);
        size_t wrote = writer.samples(chunk, n, block, sizeof(block));
        CHECK(wrote == kMindRecordHead + n * 4u);
        bytes.insert(bytes.end(), block, block + wrote);
        done += n;
        if (done == 200) {
            wrote = writer.gap(17, block, sizeof(block));
            bytes.insert(bytes.end(), block, block + wrote);
            wrote = writer.pause(false, block, sizeof(block));
            bytes.insert(bytes.end(), block, block + wrote);
            wrote = writer.pause(true, block, sizeof(block));
            bytes.insert(bytes.end(), block, block + wrote);
        }
    }
    if (withEnd) {
        const size_t wrote = writer.end(MindEnd::Completed, block, sizeof(block));
        CHECK(wrote == kMindEndBytes);
        bytes.insert(bytes.end(), block, block + wrote);
    }
    return bytes;
}

}  // namespace

TEST(crc32_is_the_usual_one) {
    CHECK(crc32(reinterpret_cast<const uint8_t*>("123456789"), 9) == 0xCBF43926u);
    CHECK(crc32(nullptr, 0) == 0);
    // In pieces as in one.
    const uint8_t* text = reinterpret_cast<const uint8_t*>("123456789");
    CHECK(crc32(text + 4, 5, crc32(text, 4)) == 0xCBF43926u);
}

TEST(a_file_gives_back_every_sample_and_its_description) {
    std::vector<uint32_t> written;
    const std::vector<uint8_t> bytes = file(1000, &written);
    Collect got;
    MindFileReader reader;
    reader.begin(&got);
    CHECK(reader.feed(bytes.data(), bytes.size()));
    CHECK(reader.headerValid() && reader.ended() && reader.intact() && !reader.refused());
    CHECK(reader.endReason() == MindEnd::Completed);
    CHECK(got.samples == written);
    CHECK(reader.sampleCount() == 1000);
    CHECK(got.gaps.size() == 1 && got.gaps[0] == 17 && reader.gaps() == 1 && reader.lost() == 17);
    CHECK(got.pauses.size() == 2 && !got.pauses[0] && got.pauses[1]);
    const MindHeader& h = reader.header();
    CHECK(h.experimentId == 42 && h.source == RngId::Software && h.sourceClass == RngClass::SoftwareControl);
    CHECK(!h.physicalEntropy && h.condition == MindCondition::Meditation && h.mode == MindMode::Directed);
    CHECK(h.target == MindTarget::MoreOnes && h.viz == MindViz::Orb && h.audioOn);
    CHECK(h.rate == 250 && h.windowSamples == 250 && h.plannedSeconds == 600 && h.seed == 777);
    CHECK(h.startUptimeMs == 123456 && h.startUnix == 0);
    CHECK(strcmp(h.firmware, "0.11.0") == 0 && strcmp(h.audio, "Trance 10-4.5 Hz") == 0);
    CHECK_NEAR(h.primaryHz, 136.1, 0.01);
    CHECK_NEAR(h.beatHz, 7.8, 0.01);
}

TEST(the_file_can_arrive_in_pieces_of_any_size) {
    std::vector<uint32_t> written;
    const std::vector<uint8_t> bytes = file(700, &written);
    for (size_t piece : {1u, 3u, 7u, 97u, 512u}) {
        Collect got;
        MindFileReader reader;
        reader.begin(&got);
        for (size_t at = 0; at < bytes.size(); at += piece) {
            CHECK(reader.feed(bytes.data() + at, at + piece <= bytes.size() ? piece : bytes.size() - at));
        }
        CHECK(reader.intact());
        CHECK(got.samples == written);
    }
}

TEST(a_replay_gives_the_same_statistics_as_the_run) {
    std::vector<uint32_t> written;
    const std::vector<uint8_t> bytes = file(5000, &written);
    MindStats live;
    live.begin(250, MindTarget::MoreOnes);
    for (uint32_t v : written) live.add(v);
    struct Replay : MindFileVisitor {
        MindStats stats;
        void onSample(uint32_t v) override { stats.add(v); }
    } replay;
    MindFileReader reader;
    reader.begin(&replay);
    // The reader gives the window and the target; nothing else is needed.
    CHECK(reader.feed(bytes.data(), kMindHeaderBytes));
    replay.stats.begin(reader.header().windowSamples, reader.header().target);
    CHECK(reader.feed(bytes.data() + kMindHeaderBytes, bytes.size() - kMindHeaderBytes));
    MindSummary a, b;
    live.summary(a);
    replay.stats.summary(b);
    CHECK(a.ones == b.ones && a.z == b.z && a.runs == b.runs && a.chi2Bytes == b.chi2Bytes);
    CHECK(a.windows == b.windows && a.windowMaxAbsZ == b.windowMaxAbsZ && a.directedZ == b.directedZ);
    CHECK(reader.ones() == a.ones);
}

TEST(an_interrupted_file_keeps_what_was_written) {
    std::vector<uint32_t> written;
    std::vector<uint8_t> bytes = file(450, &written, false);
    // Power lost in the middle of a block: 30 bytes of the last record gone.
    bytes.resize(bytes.size() - 30);
    Collect got;
    MindFileReader reader;
    reader.begin(&got);
    CHECK(reader.feed(bytes.data(), bytes.size()));
    CHECK(reader.headerValid() && !reader.ended() && !reader.intact() && !reader.refused());
    CHECK(got.samples.size() == 450 - 8);  // the last seven and a half samples are missing
    CHECK(reader.danglingBytes() == 2);
    for (size_t i = 0; i < got.samples.size(); ++i) CHECK(got.samples[i] == written[i]);
    // Cut exactly after a record: nothing dangling, still not ended.
    bytes = file(400, nullptr, false);
    reader.begin(nullptr);
    CHECK(reader.feed(bytes.data(), bytes.size()));
    CHECK(reader.sampleCount() == 400 && !reader.ended() && reader.danglingBytes() == 0);
}

TEST(a_changed_sample_is_noticed) {
    std::vector<uint8_t> bytes = file(300);
    bytes[kMindHeaderBytes + kMindRecordHead + 40] ^= 0x10;  // one bit of one sample
    MindFileReader reader;
    reader.begin();
    CHECK(reader.feed(bytes.data(), bytes.size()));
    CHECK(reader.ended() && !reader.intact());
    CHECK(reader.sampleCount() == 300);
}

TEST(what_is_not_an_experiment_file_is_refused) {
    std::vector<uint8_t> bytes = file(100);
    MindFileReader reader;
    // Wrong magic.
    std::vector<uint8_t> bad = bytes;
    bad[0] = 'X';
    reader.begin();
    CHECK(!reader.feed(bad.data(), bad.size()));
    CHECK(reader.refused() && !reader.headerValid() && reader.sampleCount() == 0);
    // A changed header field: the header's checksum no longer fits.
    bad = bytes;
    bad[17] = 2;
    reader.begin();
    CHECK(!reader.feed(bad.data(), bad.size()));
    // A later format version.
    bad = bytes;
    bad[4] = 2;
    reader.begin();
    CHECK(!reader.feed(bad.data(), bad.size()));
    // An unknown record after valid samples: refused there, samples kept.
    bad = bytes;
    bad[kMindHeaderBytes + kMindRecordHead + 400] = 'Q';
    reader.begin();
    CHECK(!reader.feed(bad.data(), bad.size()));
    CHECK(reader.headerValid() && reader.sampleCount() == 100 && reader.refused());
    CHECK(!reader.feed(bad.data(), 1));  // and stays refused
    // Text.
    const char* text = "RESONANCE SESSION 1\nname=x\n";
    reader.begin();
    reader.feed(reinterpret_cast<const uint8_t*>(text), strlen(text));
    CHECK(!reader.headerValid() && !reader.ended());
}

TEST(only_an_honest_header_is_written) {
    MindFileWriter writer;
    writer.begin();
    uint8_t out[kMindHeaderBytes];
    MindHeader h = header();
    CHECK(isValidMindHeader(h));
    // Software can never be recorded as physical entropy.
    h.physicalEntropy = true;
    CHECK(!isValidMindHeader(h));
    CHECK(writer.header(h, out, sizeof(out)) == 0);
    h = header();
    h.source = RngId::Esp32Hardware;
    h.sourceClass = RngClass::HardwareEntropy;
    h.physicalEntropy = true;
    CHECK(isValidMindHeader(h));
    h.sourceClass = RngClass::ExperimentalNoise;
    CHECK(!isValidMindHeader(h));
    // A directed experiment needs its target; an open one has none.
    h = header();
    h.target = MindTarget::None;
    CHECK(!isValidMindHeader(h));
    h.mode = MindMode::Open;
    CHECK(isValidMindHeader(h));
    h.target = MindTarget::MoreZeros;
    CHECK(!isValidMindHeader(h));
    // Ranges.
    h = header();
    h.source = RngId::None;
    CHECK(!isValidMindHeader(h));
    h = header();
    h.rate = 0;
    CHECK(!isValidMindHeader(h));
    h.rate = 1001;
    CHECK(!isValidMindHeader(h));
    h = header();
    h.windowSamples = 0;
    CHECK(!isValidMindHeader(h));
    h = header();
    memset(h.audio, 'a', sizeof(h.audio));
    CHECK(!isValidMindHeader(h));
    // Too small a buffer.
    CHECK(writer.header(header(), out, sizeof(out) - 1) == 0);
}

TEST(blocks_have_limits) {
    MindFileWriter writer;
    writer.begin();
    uint32_t values[kMindBlockSamples + 1] = {};
    uint8_t out[kMindRecordHead + (kMindBlockSamples + 1) * 4];
    CHECK(writer.samples(values, 0, out, sizeof(out)) == 0);
    CHECK(writer.samples(values, kMindBlockSamples + 1, out, sizeof(out)) == 0);
    CHECK(writer.samples(values, 10, out, kMindRecordHead + 39) == 0);
    CHECK(writer.sampleCount() == 0);  // a refused block counts nothing
    CHECK(writer.samples(values, kMindBlockSamples, out, sizeof(out)) == kMindRecordHead + kMindBlockSamples * 4);
    CHECK(writer.sampleCount() == kMindBlockSamples && writer.ones() == 0);
    CHECK(writer.end(MindEnd::Stopped, out, kMindEndBytes - 1) == 0);
}

TEST(the_summary_text_names_everything_and_reads_back) {
    const MindHeader h = header();
    MindStats stats;
    stats.begin(h.windowSamples, h.target);
    SoftwareRng rng(h.seed);
    rng.begin();
    for (int i = 0; i < 10000; ++i) stats.add(rng.next());
    MindSummary s;
    stats.summary(s);
    std::vector<std::string> lines;
    char line[kMindSummaryLineMax];
    for (uint8_t i = 0; mindSummaryLine(h, s, MindEnd::Stopped, 3, i, line, sizeof(line)); ++i) lines.push_back(line);
    CHECK(lines.size() == 36);
    CHECK(lines[0] == "RESONANCE MINDFIELD 1");
    std::string all;
    for (const auto& l : lines) all += l + "\n";
    for (const char* needle : {"id=42", "source=6 Software PRNG", "class=Software control", "physical_entropy=no",
                               "condition=1 Meditation", "mode=1 Directed", "target=1 More ones", "visualization=1 Orb",
                               "rate=250", "samples=10000", "bits=320000", "gaps=3", "ended=1 Stopped",
                               "audio=Trance 10-4.5 Hz, 136.1 + 7.8 Hz, on", "not evidence of a cause"}) {
        if (all.find(needle) == std::string::npos) {
            printf("  missing: %s\n", needle);
            CHECK(false);
        }
    }
    MindRecordParser parser;
    parser.begin();
    for (const auto& l : lines) CHECK(parser.line(l.c_str()));
    MindRecord r;
    CHECK(parser.finish(r));
    CHECK(r.complete && r.experimentId == 42 && r.source == RngId::Software && r.rate == 250);
    CHECK(r.condition == MindCondition::Meditation && r.mode == MindMode::Directed && r.target == MindTarget::MoreOnes);
    CHECK(r.viz == MindViz::Orb && r.samples == 10000 && r.ones == s.ones && r.reason == MindEnd::Stopped);
    CHECK_NEAR(r.z, s.z, 1e-4);
    CHECK_NEAR(r.directedZ, s.directedZ, 1e-4);
    // As a table row.
    char row[200], head[200];
    CHECK(mindCsvHeader(head, sizeof(head)) && mindCsvRow(r, row, sizeof(row)));
    CHECK(std::string(row).find("42,Software PRNG,Meditation,Directed,More ones,Orb,250,10000,320000,") == 0);
    int commasHead = 0, commasRow = 0;
    for (const char* c = head; *c; ++c) commasHead += *c == ',';
    for (const char* c = row; *c; ++c) commasRow += *c == ',';
    CHECK(commasHead == commasRow);
    CHECK(!mindCsvRow(r, row, 20));
}

TEST(a_summary_that_is_not_one_is_refused) {
    MindRecord r;
    r.experimentId = 9;
    MindRecordParser parser;
    parser.begin();
    CHECK(!parser.finish(r));
    CHECK(!parser.line("RESONANCE SESSION 1"));
    CHECK(!parser.finish(r));
    parser.begin();
    CHECK(parser.line("RESONANCE MINDFIELD 1"));
    CHECK(parser.line("source=6 Software PRNG"));
    CHECK(!parser.finish(r));  // no counts: an experiment that was never summarised
    CHECK(parser.line("samples=10"));
    CHECK(parser.line("something_new=5"));
    CHECK(parser.finish(r));
    CHECK(parser.line("ones=321"));  // more ones than bits
    CHECK(!parser.finish(r));
    for (const char* bad : {"source=7", "source=0", "condition=4", "mode=2", "target=3", "rate=1001", "samples=x",
                            "ended=9", "id=-1"}) {
        parser.begin();
        parser.line("RESONANCE MINDFIELD 1");
        parser.line("source=1");
        parser.line("samples=10");
        parser.line(bad);
        CHECK(!parser.finish(r));
    }
    CHECK(r.experimentId == 0 || r.experimentId == 9);
}

TEST(sixty_four_bit_counts_are_written_in_full) {
    MindHeader h = header();
    MindSummary s;
    s.samples = 5000000000ull;  // more than 32 bits hold
    s.bits = s.samples * 32u;
    s.ones = 80000000001ull;
    char line[kMindSummaryLineMax];
    CHECK(mindSummaryLine(h, s, MindEnd::Completed, 0, 15, line, sizeof(line)));
    CHECK(strcmp(line, "samples=5000000000") == 0);
    CHECK(mindSummaryLine(h, s, MindEnd::Completed, 0, 16, line, sizeof(line)));
    CHECK(strcmp(line, "bits=160000000000") == 0);
    CHECK(mindSummaryLine(h, s, MindEnd::Completed, 0, 17, line, sizeof(line)));
    CHECK(strcmp(line, "ones=80000000001") == 0);
    CHECK(!mindSummaryLine(h, s, MindEnd::Completed, 0, 14, line, 10));
}

TEST(names_for_everything_stored) {
    CHECK(strcmp(mindVizName(MindViz::Pendulum), "Pendulum") == 0);
    CHECK(strcmp(mindVizName(static_cast<MindViz>(99)), "?") == 0);
    CHECK(strcmp(rngIdName(RngId::Esp32Hardware), "ESP32 hardware RNG") == 0);
    CHECK(strcmp(mindConditionName(MindCondition::Sham), "Sham") == 0);
    CHECK(strcmp(mindEndName(MindEnd::CardFailed), "Card failed") == 0);
}

int main() { return host_test::runAll(); }
