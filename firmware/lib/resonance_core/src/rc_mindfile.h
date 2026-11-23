// Resonance portable core: the files of a Mindfield experiment
// (RES-MIND-002).
//
// RAW FILE (Ennnnn.RMF) - every sample as it was collected, never changed
// afterwards. Little-endian throughout.
//
//   Header, 96 bytes:
//     0  char[4]  "RMF1"
//     4  u16      format version (1)
//     6  u16      header length (96)
//     8  u32      experiment identifier
//    12  u8       source (rc::RngId)
//    13  u8       source class (rc::RngClass)
//    14  u8       1 = the source reported established physical entropy at the start
//    15  u8       condition (rc::MindCondition)
//    16  u8       mode (rc::MindMode)
//    17  u8       target (rc::MindTarget), fixed before collection
//    18  u8       visualization shown (rc::MindViz)
//    19  u8       flags (bit 0: audio was on at the start)
//    20  u16      samples a second
//    22  u16      bits in a sample (32)
//    24  u32      samples in a measurement window
//    28  u32      planned length in seconds (0 = until stopped)
//    32  u32      seed of the software source, else 0
//    36  u32      milliseconds since the device started, at the first sample
//    40  u32      start time, seconds since 1970, or 0: the device has no clock
//    44  char[12] firmware version, NUL-padded
//    56  char[24] name of the session running at the start, NUL-padded, or empty
//    80  u16      primary frequency x 10 Hz at the start
//    82  u16      beat frequency x 10 Hz at the start
//    84  u8[8]    reserved, zero
//    92  u32      CRC-32 of bytes 0..91
//
//   Then records, each: u8 tag, u8 zero, u16 count, payload.
//     'S'  count samples of 4 bytes each
//     'G'  gap: the source could not deliver. count 1, payload u32 = samples lost
//     'P'  paused (count 1, payload u32 0) or resumed (payload u32 1) by the user
//     'E'  end. count 0, payload 24 bytes: u64 samples, u64 ones,
//          u32 CRC-32 of all sample bytes in file order, u32 reason (rc::MindEnd)
//
//   A file without an 'E' record was interrupted; everything up to its last
//   complete record is valid data, and the reader says so.
//
// SUMMARY FILE (Ennnnn.TXT) - text, one "key=value" a line, written when the
// experiment ends. It is derived from the raw file and can be made again
// from it.
//
// Portable: no Arduino, ESP-IDF or display header.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "rc_mindstats.h"
#include "rc_rng.h"

namespace rc {

constexpr size_t kMindHeaderBytes = 96;
constexpr size_t kMindRecordHead = 4;
constexpr size_t kMindEndBytes = kMindRecordHead + 24;
constexpr uint16_t kMindBlockSamples = 256;  // most samples in one 'S' record
constexpr size_t kMindSummaryLineMax = 96;

// Stored in files: never renumber.
enum class MindCondition : uint8_t { Baseline = 0, Meditation = 1, Control = 2, Sham = 3 };
enum class MindMode : uint8_t { Open = 0, Directed = 1 };
enum class MindViz : uint8_t {
    None = 0, Orb = 1, Balance = 2, Tree = 3, Mandala = 4, River = 5, Star = 6, Flame = 7,
    Lotus = 8, Labyrinth = 9, Waterfall = 10, Vortex = 11, Pendulum = 12,
};
// Interrupted is never written to a raw file: it is what a summary made
// afterwards says of a raw file that has no end record.
enum class MindEnd : uint32_t { Completed = 0, Stopped = 1, SourceFailed = 2, CardFailed = 3, Interrupted = 4 };

const char* mindConditionName(MindCondition c);
const char* mindModeName(MindMode m);
const char* mindTargetName(MindTarget t);
const char* mindVizName(MindViz v);
const char* mindEndName(MindEnd e);
const char* rngIdName(RngId id);

struct MindHeader {
    uint32_t experimentId = 0;
    RngId source = RngId::None;
    RngClass sourceClass = RngClass::SoftwareControl;
    bool physicalEntropy = false;
    MindCondition condition = MindCondition::Baseline;
    MindMode mode = MindMode::Open;
    MindTarget target = MindTarget::None;
    MindViz viz = MindViz::None;
    bool audioOn = false;
    uint16_t rate = 100;
    uint32_t windowSamples = 100;
    uint32_t plannedSeconds = 0;
    uint32_t seed = 0;
    uint32_t startUptimeMs = 0;
    uint32_t startUnix = 0;
    char firmware[12] = "";
    char audio[24] = "";
    float primaryHz = 0.0f;
    float beatHz = 0.0f;
};

// True when the header describes an experiment that may be run and stored:
// known source, rate and window in range, and a target exactly when the mode
// is directed.
bool isValidMindHeader(const MindHeader& h);

uint32_t crc32(const uint8_t* data, size_t length, uint32_t crc = 0);

// Builds the bytes of a raw file. The caller writes them to the card; this
// class keeps the totals and the checksum that the end record needs.
class MindFileWriter {
public:
    void begin();
    // Each returns the number of bytes put into `out`, 0 when it does not fit
    // or the arguments are refused.
    size_t header(const MindHeader& h, uint8_t* out, size_t capacity);
    size_t samples(const uint32_t* values, uint16_t count, uint8_t* out, size_t capacity);
    size_t gap(uint32_t lost, uint8_t* out, size_t capacity);
    size_t pause(bool resumed, uint8_t* out, size_t capacity);
    size_t end(MindEnd reason, uint8_t* out, size_t capacity);

    uint64_t sampleCount() const { return _samples; }
    uint64_t ones() const { return _ones; }

private:
    uint64_t _samples = 0;
    uint64_t _ones = 0;
    uint32_t _crc = 0;
};

// Receives what a raw file holds, in file order.
class MindFileVisitor {
public:
    virtual ~MindFileVisitor() = default;
    virtual void onSample(uint32_t) {}
    virtual void onGap(uint32_t /*lost*/) {}
    virtual void onPause(bool /*resumed*/) {}
};

// Reads a raw file fed to it in pieces of any size.
class MindFileReader {
public:
    void begin(MindFileVisitor* visitor = nullptr);
    // False once the data is refused (bad header or an unknown record);
    // what was read before stays valid.
    bool feed(const uint8_t* data, size_t length);

    bool headerValid() const { return _headerValid; }
    const MindHeader& header() const { return _header; }
    uint64_t sampleCount() const { return _samples; }
    uint64_t ones() const { return _ones; }
    uint32_t gaps() const { return _gaps; }
    uint64_t lost() const { return _lost; }
    // An end record was found.
    bool ended() const { return _ended; }
    MindEnd endReason() const { return _reason; }
    // The end record's totals and checksum agree with the samples read.
    bool intact() const { return _ended && _footerMatches; }
    // Bytes of an unfinished record at the end of the data (an interrupted
    // write), and whether the data was refused outright.
    size_t danglingBytes() const { return _ended ? 0 : _fill; }
    bool refused() const { return _refused; }

private:
    bool record();
    MindFileVisitor* _visitor = nullptr;
    MindHeader _header;
    bool _headerValid = false, _ended = false, _footerMatches = false, _refused = false;
    MindEnd _reason = MindEnd::Completed;
    uint64_t _samples = 0, _ones = 0, _lost = 0;
    uint32_t _gaps = 0, _crc = 0;
    // The piece being assembled: the header, a record head, or a payload
    // unit (one sample, or the end record's 24 bytes).
    uint8_t _buffer[kMindHeaderBytes];
    size_t _fill = 0, _need = kMindHeaderBytes;
    enum class State : uint8_t { Header, Head, Sample, Gap, Pause, End, Done } _state = State::Header;
    uint16_t _left = 0;
};

// Line `index` of the summary file, false past the last line.
bool mindSummaryLine(const MindHeader& h, const MindSummary& s, MindEnd reason, uint32_t gaps, uint8_t index,
                     char* out, size_t capacity);

// What a list of experiments shows, read back from a summary file.
struct MindRecord {
    uint32_t experimentId = 0;
    RngId source = RngId::None;
    MindCondition condition = MindCondition::Baseline;
    MindMode mode = MindMode::Open;
    MindTarget target = MindTarget::None;
    MindViz viz = MindViz::None;
    uint16_t rate = 0;
    uint64_t samples = 0;
    uint64_t ones = 0;
    double z = 0.0;
    double directedZ = 0.0;
    MindEnd reason = MindEnd::Completed;
    bool complete = false;  // the header line and the counts were found
};

class MindRecordParser {
public:
    void begin();
    bool line(const char* text);
    bool finish(MindRecord& out) const;

private:
    MindRecord _record;
    bool _header = false, _counts = false, _bad = false;
};

// One line of a table of experiments, and the line of column names above it.
bool mindCsvHeader(char* out, size_t capacity);
bool mindCsvRow(const MindRecord& r, char* out, size_t capacity);

}  // namespace rc
