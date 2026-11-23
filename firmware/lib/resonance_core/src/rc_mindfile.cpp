#include "rc_mindfile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace rc {

namespace {

const char kMagic[4] = {'R', 'M', 'F', '1'};
const char kSummaryHeader[] = "RESONANCE MINDFIELD";

void put16(uint8_t* p, uint16_t v) { p[0] = static_cast<uint8_t>(v); p[1] = static_cast<uint8_t>(v >> 8); }
void put32(uint8_t* p, uint32_t v) { put16(p, static_cast<uint16_t>(v)); put16(p + 2, static_cast<uint16_t>(v >> 16)); }
void put64(uint8_t* p, uint64_t v) { put32(p, static_cast<uint32_t>(v)); put32(p + 4, static_cast<uint32_t>(v >> 32)); }
uint16_t get16(const uint8_t* p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
uint32_t get32(const uint8_t* p) { return static_cast<uint32_t>(get16(p)) | (static_cast<uint32_t>(get16(p + 2)) << 16); }
uint64_t get64(const uint8_t* p) { return static_cast<uint64_t>(get32(p)) | (static_cast<uint64_t>(get32(p + 4)) << 32); }

uint32_t popcount(uint32_t x) {
    x = x - ((x >> 1) & 0x55555555u);
    x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
    x = (x + (x >> 4)) & 0x0F0F0F0Fu;
    return (x * 0x01010101u) >> 24;
}

bool fits(int written, size_t capacity) { return written >= 0 && static_cast<size_t>(written) < capacity; }

// 64-bit counts as decimal text: not every printf on a small device has %llu.
const char* u64text(uint64_t v, char* buffer, size_t capacity) {
    char* p = buffer + capacity - 1;
    *p = '\0';
    do {
        *--p = static_cast<char>('0' + v % 10u);
        v /= 10u;
    } while (v != 0 && p > buffer);
    return p;
}

bool parseU64(const char* text, uint64_t& out) {
    if (*text < '0' || *text > '9') return false;
    uint64_t v = 0;
    int digits = 0;
    for (; *text >= '0' && *text <= '9'; ++text) {
        if (++digits > 19) return false;
        v = v * 10u + static_cast<uint64_t>(*text - '0');
    }
    if (*text != '\0' && *text != ' ') return false;
    out = v;
    return true;
}

uint16_t tenths(float hz) {
    if (!(hz > 0.0f)) return 0;
    const float t = hz * 10.0f + 0.5f;
    return t >= 65535.0f ? 65535 : static_cast<uint16_t>(t);
}

void copyText(uint8_t* to, const char* from, size_t capacity) {
    size_t i = 0;
    for (; i + 1 < capacity && from[i] != '\0'; ++i) to[i] = static_cast<uint8_t>(from[i]);
    for (; i < capacity; ++i) to[i] = 0;
}

}  // namespace

const char* mindConditionName(MindCondition c) {
    switch (c) {
        case MindCondition::Baseline: return "Baseline";
        case MindCondition::Meditation: return "Meditation";
        case MindCondition::Control: return "Control";
        case MindCondition::Sham: return "Sham";
    }
    return "?";
}

const char* mindModeName(MindMode m) { return m == MindMode::Directed ? "Directed" : "Open"; }

const char* mindTargetName(MindTarget t) {
    switch (t) {
        case MindTarget::None: return "None";
        case MindTarget::MoreOnes: return "More ones";
        case MindTarget::MoreZeros: return "More zeros";
    }
    return "?";
}

const char* mindVizName(MindViz v) {
    static const char* const kNames[] = {"None", "Orb", "Balance", "Tree", "Mandala", "River", "Star",
                                         "Flame", "Lotus", "Labyrinth", "Waterfall", "Vortex", "Pendulum"};
    const uint8_t i = static_cast<uint8_t>(v);
    return i < sizeof(kNames) / sizeof(kNames[0]) ? kNames[i] : "?";
}

const char* mindEndName(MindEnd e) {
    switch (e) {
        case MindEnd::Completed: return "Completed";
        case MindEnd::Stopped: return "Stopped";
        case MindEnd::SourceFailed: return "Source failed";
        case MindEnd::CardFailed: return "Card failed";
        case MindEnd::Interrupted: return "Interrupted";
    }
    return "?";
}

const char* rngIdName(RngId id) {
    switch (id) {
        case RngId::None: return "None";
        case RngId::Esp32Hardware: return "ESP32 hardware RNG";
        case RngId::InternalAdc: return "Internal ADC";
        case RngId::AdcNoise: return "ADC noise";
        case RngId::TimingJitter: return "Timing jitter";
        case RngId::External: return "External RNG";
        case RngId::Software: return "Software PRNG";
    }
    return "?";
}

bool isValidMindHeader(const MindHeader& h) {
    const uint8_t source = static_cast<uint8_t>(h.source);
    if (source < 1 || source > 6) return false;
    if (static_cast<uint8_t>(h.sourceClass) > 4) return false;
    // Only a hardware-entropy source may be recorded as physical entropy.
    if (h.physicalEntropy && h.sourceClass != RngClass::HardwareEntropy) return false;
    if (static_cast<uint8_t>(h.condition) > 3 || static_cast<uint8_t>(h.mode) > 1) return false;
    if (static_cast<uint8_t>(h.target) > 2 || static_cast<uint8_t>(h.viz) > 12) return false;
    // A directed experiment has a target; an open one has none.
    if ((h.mode == MindMode::Directed) != (h.target != MindTarget::None)) return false;
    if (h.rate < 1 || h.rate > 1000) return false;
    if (h.windowSamples < 1 || h.windowSamples > 600000u) return false;
    if (memchr(h.firmware, '\0', sizeof(h.firmware)) == nullptr) return false;
    if (memchr(h.audio, '\0', sizeof(h.audio)) == nullptr) return false;
    return true;
}

uint32_t crc32(const uint8_t* data, size_t length, uint32_t crc) {
    static const uint32_t kTable[16] = {0x00000000u, 0x1DB71064u, 0x3B6E20C8u, 0x26D930ACu, 0x76DC4190u, 0x6B6B51F4u,
                                        0x4DB26158u, 0x5005713Cu, 0xEDB88320u, 0xF00F9344u, 0xD6D6A3E8u, 0xCB61B38Cu,
                                        0x9B64C2B0u, 0x86D3D2D4u, 0xA00AE278u, 0xBDBDF21Cu};
    crc = ~crc;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        crc = kTable[crc & 0x0Fu] ^ (crc >> 4);
        crc = kTable[crc & 0x0Fu] ^ (crc >> 4);
    }
    return ~crc;
}

// ---- writer ------------------------------------------------------------------

void MindFileWriter::begin() {
    _samples = 0;
    _ones = 0;
    _crc = 0;
}

size_t MindFileWriter::header(const MindHeader& h, uint8_t* out, size_t capacity) {
    if (capacity < kMindHeaderBytes || !isValidMindHeader(h)) return 0;
    memset(out, 0, kMindHeaderBytes);
    memcpy(out, kMagic, 4);
    put16(out + 4, 1);
    put16(out + 6, static_cast<uint16_t>(kMindHeaderBytes));
    put32(out + 8, h.experimentId);
    out[12] = static_cast<uint8_t>(h.source);
    out[13] = static_cast<uint8_t>(h.sourceClass);
    out[14] = h.physicalEntropy ? 1 : 0;
    out[15] = static_cast<uint8_t>(h.condition);
    out[16] = static_cast<uint8_t>(h.mode);
    out[17] = static_cast<uint8_t>(h.target);
    out[18] = static_cast<uint8_t>(h.viz);
    out[19] = h.audioOn ? 1 : 0;
    put16(out + 20, h.rate);
    put16(out + 22, 32);
    put32(out + 24, h.windowSamples);
    put32(out + 28, h.plannedSeconds);
    put32(out + 32, h.seed);
    put32(out + 36, h.startUptimeMs);
    put32(out + 40, h.startUnix);
    copyText(out + 44, h.firmware, 12);
    copyText(out + 56, h.audio, 24);
    put16(out + 80, tenths(h.primaryHz));
    put16(out + 82, tenths(h.beatHz));
    put32(out + 92, crc32(out, 92));
    return kMindHeaderBytes;
}

size_t MindFileWriter::samples(const uint32_t* values, uint16_t count, uint8_t* out, size_t capacity) {
    const size_t bytes = kMindRecordHead + static_cast<size_t>(count) * 4u;
    if (count == 0 || count > kMindBlockSamples || capacity < bytes) return 0;
    out[0] = 'S';
    out[1] = 0;
    put16(out + 2, count);
    for (uint16_t i = 0; i < count; ++i) {
        put32(out + kMindRecordHead + i * 4u, values[i]);
        _ones += popcount(values[i]);
    }
    _crc = crc32(out + kMindRecordHead, static_cast<size_t>(count) * 4u, _crc);
    _samples += count;
    return bytes;
}

size_t MindFileWriter::gap(uint32_t lost, uint8_t* out, size_t capacity) {
    if (capacity < kMindRecordHead + 4) return 0;
    out[0] = 'G';
    out[1] = 0;
    put16(out + 2, 1);
    put32(out + 4, lost);
    return kMindRecordHead + 4;
}

size_t MindFileWriter::pause(bool resumed, uint8_t* out, size_t capacity) {
    if (capacity < kMindRecordHead + 4) return 0;
    out[0] = 'P';
    out[1] = 0;
    put16(out + 2, 1);
    put32(out + 4, resumed ? 1u : 0u);
    return kMindRecordHead + 4;
}

size_t MindFileWriter::end(MindEnd reason, uint8_t* out, size_t capacity) {
    if (capacity < kMindEndBytes) return 0;
    out[0] = 'E';
    out[1] = 0;
    put16(out + 2, 0);
    put64(out + 4, _samples);
    put64(out + 12, _ones);
    put32(out + 20, _crc);
    put32(out + 24, static_cast<uint32_t>(reason));
    return kMindEndBytes;
}

// ---- reader ------------------------------------------------------------------

void MindFileReader::begin(MindFileVisitor* visitor) {
    *this = MindFileReader();
    _visitor = visitor;
}

// Called when `_need` bytes are in `_buffer`.
bool MindFileReader::record() {
    switch (_state) {
        case State::Header: {
            if (memcmp(_buffer, kMagic, 4) != 0 || get16(_buffer + 4) != 1 ||
                get16(_buffer + 6) != kMindHeaderBytes || get16(_buffer + 22) != 32 ||
                get32(_buffer + 92) != crc32(_buffer, 92)) {
                return false;
            }
            MindHeader h;
            h.experimentId = get32(_buffer + 8);
            h.source = static_cast<RngId>(_buffer[12]);
            h.sourceClass = static_cast<RngClass>(_buffer[13]);
            h.physicalEntropy = _buffer[14] != 0;
            h.condition = static_cast<MindCondition>(_buffer[15]);
            h.mode = static_cast<MindMode>(_buffer[16]);
            h.target = static_cast<MindTarget>(_buffer[17]);
            h.viz = static_cast<MindViz>(_buffer[18]);
            h.audioOn = (_buffer[19] & 1u) != 0;
            h.rate = get16(_buffer + 20);
            h.windowSamples = get32(_buffer + 24);
            h.plannedSeconds = get32(_buffer + 28);
            h.seed = get32(_buffer + 32);
            h.startUptimeMs = get32(_buffer + 36);
            h.startUnix = get32(_buffer + 40);
            memcpy(h.firmware, _buffer + 44, 12);
            h.firmware[11] = '\0';
            memcpy(h.audio, _buffer + 56, 24);
            h.audio[23] = '\0';
            h.primaryHz = static_cast<float>(get16(_buffer + 80)) * 0.1f;
            h.beatHz = static_cast<float>(get16(_buffer + 82)) * 0.1f;
            if (!isValidMindHeader(h)) return false;
            _header = h;
            _headerValid = true;
            _state = State::Head;
            _need = kMindRecordHead;
            return true;
        }
        case State::Head: {
            const uint8_t tag = _buffer[0];
            const uint16_t count = get16(_buffer + 2);
            if (_buffer[1] != 0) return false;
            if (tag == 'S' && count >= 1 && count <= kMindBlockSamples) {
                _state = State::Sample;
                _left = count;
                _need = 4;
            } else if (tag == 'G' && count == 1) {
                _state = State::Gap;
                _need = 4;
            } else if (tag == 'P' && count == 1) {
                _state = State::Pause;
                _need = 4;
            } else if (tag == 'E' && count == 0) {
                _state = State::End;
                _need = 24;
            } else {
                return false;
            }
            return true;
        }
        case State::Sample: {
            const uint32_t value = get32(_buffer);
            _crc = crc32(_buffer, 4, _crc);
            _ones += popcount(value);
            ++_samples;
            if (_visitor != nullptr) _visitor->onSample(value);
            if (--_left == 0) {
                _state = State::Head;
                _need = kMindRecordHead;
            }
            return true;
        }
        case State::Gap: {
            const uint32_t lost = get32(_buffer);
            ++_gaps;
            _lost += lost;
            if (_visitor != nullptr) _visitor->onGap(lost);
            _state = State::Head;
            _need = kMindRecordHead;
            return true;
        }
        case State::Pause:
            if (_visitor != nullptr) _visitor->onPause(get32(_buffer) != 0);
            _state = State::Head;
            _need = kMindRecordHead;
            return true;
        case State::End: {
            const uint32_t reason = get32(_buffer + 20);
            _ended = true;
            _reason = reason <= 3 ? static_cast<MindEnd>(reason) : MindEnd::Stopped;
            _footerMatches = get64(_buffer) == _samples && get64(_buffer + 8) == _ones && get32(_buffer + 16) == _crc;
            _state = State::Done;
            _need = 0;
            return true;
        }
        case State::Done:
            return true;
    }
    return false;
}

bool MindFileReader::feed(const uint8_t* data, size_t length) {
    if (_refused) return false;
    for (size_t i = 0; i < length; ++i) {
        if (_state == State::Done) return true;  // anything after the end record is not ours
        _buffer[_fill++] = data[i];
        // A sample in the middle of a record is not "dangling": only what
        // has not completed a unit counts, and the record head is kept in
        // mind through _left.
        if (_fill == _need) {
            _fill = 0;
            if (!record()) {
                _refused = true;
                return false;
            }
        }
    }
    return true;
}

// ---- summary -----------------------------------------------------------------

bool mindSummaryLine(const MindHeader& h, const MindSummary& s, MindEnd reason, uint32_t gaps, uint8_t index,
                     char* out, size_t capacity) {
    char a[24], b[24];
    int n = -1;
    switch (index) {
        case 0: n = snprintf(out, capacity, "%s 1", kSummaryHeader); break;
        case 1: n = snprintf(out, capacity, "id=%lu", static_cast<unsigned long>(h.experimentId)); break;
        case 2: n = snprintf(out, capacity, "source=%u %s", static_cast<unsigned>(h.source), rngIdName(h.source)); break;
        case 3: n = snprintf(out, capacity, "class=%s", rngClassName(h.sourceClass)); break;
        case 4: n = snprintf(out, capacity, "physical_entropy=%s", h.physicalEntropy ? "yes" : "no"); break;
        case 5: n = snprintf(out, capacity, "condition=%u %s", static_cast<unsigned>(h.condition), mindConditionName(h.condition)); break;
        case 6: n = snprintf(out, capacity, "mode=%u %s", static_cast<unsigned>(h.mode), mindModeName(h.mode)); break;
        case 7: n = snprintf(out, capacity, "target=%u %s", static_cast<unsigned>(h.target), mindTargetName(h.target)); break;
        case 8: n = snprintf(out, capacity, "visualization=%u %s", static_cast<unsigned>(h.viz), mindVizName(h.viz)); break;
        case 9: n = snprintf(out, capacity, "rate=%u", static_cast<unsigned>(h.rate)); break;
        case 10: n = snprintf(out, capacity, "window_samples=%lu", static_cast<unsigned long>(h.windowSamples)); break;
        case 11: n = snprintf(out, capacity, "planned_seconds=%lu", static_cast<unsigned long>(h.plannedSeconds)); break;
        case 12: n = snprintf(out, capacity, "seed=%lu", static_cast<unsigned long>(h.seed)); break;
        case 13: n = snprintf(out, capacity, "firmware=%s", h.firmware); break;
        case 14:
            n = snprintf(out, capacity, "audio=%s, %.1f + %.1f Hz, %s", h.audio[0] ? h.audio : "no session",
                         static_cast<double>(h.primaryHz), static_cast<double>(h.beatHz), h.audioOn ? "on" : "off");
            break;
        case 15: n = snprintf(out, capacity, "samples=%s", u64text(s.samples, a, sizeof(a))); break;
        case 16: n = snprintf(out, capacity, "bits=%s", u64text(s.bits, a, sizeof(a))); break;
        case 17: n = snprintf(out, capacity, "ones=%s", u64text(s.ones, a, sizeof(a))); break;
        case 18: n = snprintf(out, capacity, "proportion=%.7f", s.proportion); break;
        case 19: n = snprintf(out, capacity, "z=%.4f", s.z); break;
        case 20: n = snprintf(out, capacity, "ci95=%.7f..%.7f", s.ciLow, s.ciHigh); break;
        case 21: n = snprintf(out, capacity, "p_two_sided=%.6g", s.pTwoSided); break;
        case 22: n = snprintf(out, capacity, "directed_z=%.4f", s.directedZ); break;
        case 23: n = snprintf(out, capacity, "p_directed=%.6g", s.pDirected); break;
        case 24:
            n = snprintf(out, capacity, "runs=%s expected=%.1f z=%.4f", u64text(s.runs, a, sizeof(a)), s.runsExpected, s.runsZ);
            break;
        case 25: n = snprintf(out, capacity, "chi2_bytes=%.2f z=%.4f", s.chi2Bytes, s.chi2Z); break;
        case 26:
            n = snprintf(out, capacity, "autocorr lag1=%.6f lag2=%.6f lag8=%.6f lag32=%.6f", s.autocorr[0], s.autocorr[1],
                         s.autocorr[2], s.autocorr[3]);
            break;
        case 27:
            n = snprintf(out, capacity, "autocorr_z lag1=%.3f lag2=%.3f lag8=%.3f lag32=%.3f", s.autocorrZ[0], s.autocorrZ[1],
                         s.autocorrZ[2], s.autocorrZ[3]);
            break;
        case 28:
            n = snprintf(out, capacity, "windows=%lu mean_z=%.4f var_z=%.4f", static_cast<unsigned long>(s.windows),
                         s.windowMeanZ, s.windowVarZ);
            break;
        case 29:
            n = snprintf(out, capacity, "window_max_abs_z=%.4f corrected_threshold=%.4f exceeded=%s", s.windowMaxAbsZ,
                         s.windowCritZ, s.windowExceedsCrit ? "yes" : "no");
            break;
        case 30:
            n = snprintf(out, capacity, "windows_beyond_1.96=%lu expected_by_chance=%.1f",
                         static_cast<unsigned long>(s.windowsBeyond95), 0.05 * static_cast<double>(s.windows));
            break;
        case 31: n = snprintf(out, capacity, "window_lag1=%.4f", s.windowLag1); break;
        case 32: n = snprintf(out, capacity, "gaps=%lu", static_cast<unsigned long>(gaps)); break;
        case 33: n = snprintf(out, capacity, "ended=%lu %s", static_cast<unsigned long>(reason), mindEndName(reason)); break;
        case 34: n = snprintf(out, capacity, "note=Scores are against ideal fair bits. Compare with a baseline"); break;
        case 35: n = snprintf(out, capacity, "note=from the same source. A small p is not evidence of a cause."); break;
        default: return false;
    }
    (void)b;
    return fits(n, capacity);
}

void MindRecordParser::begin() { *this = MindRecordParser(); }

bool MindRecordParser::line(const char* text) {
    if (_bad) return false;
    size_t length = strlen(text);
    while (length > 0 && (text[length - 1] == '\r' || text[length - 1] == '\n' || text[length - 1] == ' ')) --length;
    if (length == 0) return true;
    char buffer[kMindSummaryLineMax];
    if (length >= sizeof(buffer)) return true;  // a longer line is not one this parser needs
    memcpy(buffer, text, length);
    buffer[length] = '\0';
    if (!_header) {
        const size_t n = sizeof(kSummaryHeader) - 1;
        if (strncmp(buffer, kSummaryHeader, n) != 0 || strcmp(buffer + n, " 1") != 0) return !(_bad = true);
        _header = true;
        return true;
    }
    char* value = strchr(buffer, '=');
    if (value == nullptr) return true;
    *value++ = '\0';
    uint64_t v = 0;
    auto small = [&](uint64_t limit) { return parseU64(value, v) && v <= limit; };
    if (strcmp(buffer, "id") == 0) {
        if (!small(0xFFFFFFFFu)) return !(_bad = true);
        _record.experimentId = static_cast<uint32_t>(v);
    } else if (strcmp(buffer, "source") == 0) {
        if (!small(6) || v == 0) return !(_bad = true);
        _record.source = static_cast<RngId>(v);
    } else if (strcmp(buffer, "condition") == 0) {
        if (!small(3)) return !(_bad = true);
        _record.condition = static_cast<MindCondition>(v);
    } else if (strcmp(buffer, "mode") == 0) {
        if (!small(1)) return !(_bad = true);
        _record.mode = static_cast<MindMode>(v);
    } else if (strcmp(buffer, "target") == 0) {
        if (!small(2)) return !(_bad = true);
        _record.target = static_cast<MindTarget>(v);
    } else if (strcmp(buffer, "visualization") == 0) {
        if (!small(12)) return !(_bad = true);
        _record.viz = static_cast<MindViz>(v);
    } else if (strcmp(buffer, "rate") == 0) {
        if (!small(1000)) return !(_bad = true);
        _record.rate = static_cast<uint16_t>(v);
    } else if (strcmp(buffer, "samples") == 0) {
        if (!parseU64(value, v)) return !(_bad = true);
        _record.samples = v;
        _counts = true;
    } else if (strcmp(buffer, "ones") == 0) {
        if (!parseU64(value, v)) return !(_bad = true);
        _record.ones = v;
    } else if (strcmp(buffer, "z") == 0) {
        _record.z = strtod(value, nullptr);
    } else if (strcmp(buffer, "directed_z") == 0) {
        _record.directedZ = strtod(value, nullptr);
    } else if (strcmp(buffer, "ended") == 0) {
        if (!small(4)) return !(_bad = true);
        _record.reason = static_cast<MindEnd>(v);
    }
    return true;
}

bool MindRecordParser::finish(MindRecord& out) const {
    if (_bad || !_header || !_counts || _record.source == RngId::None) return false;
    if (_record.ones > _record.samples * 32u) return false;
    out = _record;
    out.complete = true;
    return true;
}

bool mindCsvHeader(char* out, size_t capacity) {
    return fits(snprintf(out, capacity, "id,source,condition,mode,target,visualization,rate,samples,bits,ones,z,directed_z,ended"),
                capacity);
}

bool mindCsvRow(const MindRecord& r, char* out, size_t capacity) {
    char a[24], b[24], c[24];
    return fits(snprintf(out, capacity, "%lu,%s,%s,%s,%s,%s,%u,%s,%s,%s,%.4f,%.4f,%s",
                         static_cast<unsigned long>(r.experimentId), rngIdName(r.source), mindConditionName(r.condition),
                         mindModeName(r.mode), mindTargetName(r.target), mindVizName(r.viz), static_cast<unsigned>(r.rate),
                         u64text(r.samples, a, sizeof(a)), u64text(r.samples * 32u, b, sizeof(b)),
                         u64text(r.ones, c, sizeof(c)), r.z, r.directedZ, mindEndName(r.reason)),
                capacity);
}

}  // namespace rc
