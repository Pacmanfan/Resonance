// WAV header reading, PCM conversion and sample-rate conversion
// (RES-PLAY-001).
#include "host_test.h"
#include "rc_resample.h"
#include "rc_wav.h"

#include <vector>

using namespace rc;

namespace {

class MemorySource : public ByteSource {
public:
    explicit MemorySource(std::vector<uint8_t> bytes) : _bytes(std::move(bytes)) {}
    size_t read(uint8_t* out, size_t count) override {
        size_t n = 0;
        while (n < count && _at < _bytes.size()) out[n++] = _bytes[_at++];
        return n;
    }
    bool seek(uint32_t position) override {
        if (position > _bytes.size()) return false;
        _at = position;
        return true;
    }
    size_t position() const { return _at; }

private:
    std::vector<uint8_t> _bytes;
    size_t _at = 0;
};

void put16(std::vector<uint8_t>& v, uint16_t x) { v.push_back(x & 0xFF); v.push_back(x >> 8); }
void put32(std::vector<uint8_t>& v, uint32_t x) { put16(v, x & 0xFFFF); put16(v, x >> 16); }
void putTag(std::vector<uint8_t>& v, const char* tag) { v.insert(v.end(), tag, tag + 4); }

// A WAV file with an optional extra chunk before the data.
std::vector<uint8_t> makeWav(uint16_t format, uint16_t channels, uint32_t rate, uint16_t bits, uint32_t dataBytes,
                             bool listChunk = false, bool extensible = false) {
    std::vector<uint8_t> v;
    putTag(v, "RIFF"); put32(v, 0); putTag(v, "WAVE");
    putTag(v, "fmt "); put32(v, extensible ? 40 : 16);
    put16(v, extensible ? 0xFFFE : format); put16(v, channels); put32(v, rate);
    put32(v, rate * channels * bits / 8); put16(v, channels * bits / 8); put16(v, bits);
    if (extensible) {
        put16(v, 22); put16(v, bits); put32(v, 3);
        put16(v, format);  // sub-format: first two bytes are the format code
        for (int i = 0; i < 14; ++i) v.push_back(0);
    }
    if (listChunk) {
        putTag(v, "LIST"); put32(v, 5);  // odd size: one padding byte follows
        for (int i = 0; i < 6; ++i) v.push_back('x');
    }
    putTag(v, "data"); put32(v, dataBytes);
    for (uint32_t i = 0; i < dataBytes; ++i) v.push_back(static_cast<uint8_t>(i));
    return v;
}

}  // namespace

TEST(a_plain_16_bit_stereo_file_is_read) {
    MemorySource src(makeWav(1, 2, 44100, 16, 400));
    WavInfo info;
    CHECK(readWavHeader(src, info) == WavError::Ok);
    CHECK(info.sampleRate == 44100 && info.channels == 2 && info.bitsPerSample == 16);
    CHECK(info.dataOffset == 44 && info.dataBytes == 400 && info.frames() == 100);
    CHECK(src.position() == 44);  // positioned at the first sample
}

TEST(chunks_before_the_data_are_skipped_including_padding) {
    MemorySource src(makeWav(1, 1, 22050, 8, 64, true));
    WavInfo info;
    CHECK(readWavHeader(src, info) == WavError::Ok);
    CHECK(info.dataOffset == 44 + 8 + 6);
    CHECK(info.channels == 1 && info.bitsPerSample == 8 && info.frames() == 64);
}

TEST(the_extensible_format_with_pcm_inside_is_accepted) {
    MemorySource src(makeWav(1, 2, 48000, 24, 600, false, true));
    WavInfo info;
    CHECK(readWavHeader(src, info) == WavError::Ok);
    CHECK(info.sampleRate == 48000 && info.bitsPerSample == 24 && info.frames() == 100);
}

TEST(unsupported_formats_are_refused) {
    WavInfo info;
    { MemorySource s(makeWav(3, 2, 44100, 32, 64)); CHECK(readWavHeader(s, info) == WavError::Unsupported); }  // float
    { MemorySource s(makeWav(85, 2, 44100, 16, 64)); CHECK(readWavHeader(s, info) == WavError::Unsupported); } // MP3 in WAV
    { MemorySource s(makeWav(1, 6, 44100, 16, 64)); CHECK(readWavHeader(s, info) == WavError::Unsupported); }  // 5.1
    { MemorySource s(makeWav(1, 2, 44100, 12, 64)); CHECK(readWavHeader(s, info) == WavError::Unsupported); }
    { MemorySource s(makeWav(1, 2, 1000, 16, 64)); CHECK(readWavHeader(s, info) == WavError::Unsupported); }
    { MemorySource s(makeWav(3, 2, 44100, 32, 64, false, true)); CHECK(readWavHeader(s, info) == WavError::Unsupported); }
}

TEST(files_that_are_not_wav_or_are_cut_short_are_refused) {
    WavInfo info;
    { MemorySource s(std::vector<uint8_t>{'I', 'D', '3', 4, 0, 0, 0, 0, 0, 0, 0, 0}); CHECK(readWavHeader(s, info) == WavError::NotWav); }
    { MemorySource s(std::vector<uint8_t>{}); CHECK(readWavHeader(s, info) == WavError::NotWav); }
    auto cut = makeWav(1, 2, 44100, 16, 400);
    cut.resize(30);  // ends inside the format chunk
    { MemorySource s(cut); CHECK(readWavHeader(s, info) == WavError::Truncated); }
    auto noData = makeWav(1, 2, 44100, 16, 0);
    noData.resize(36);  // format, then nothing
    { MemorySource s(noData); CHECK(readWavHeader(s, info) == WavError::Truncated); }
}

TEST(a_data_size_that_is_not_whole_frames_is_rounded_down) {
    MemorySource src(makeWav(1, 2, 44100, 16, 403));
    WavInfo info;
    CHECK(readWavHeader(src, info) == WavError::Ok);
    CHECK(info.dataBytes == 400);
}

TEST(pcm_is_converted_to_stereo_16_bit) {
    int16_t out[8];
    WavInfo mono8 = {8000, 1, 8, 0, 0};
    const uint8_t a[] = {128, 255, 0};
    CHECK(wavToStereo16(a, 3, mono8, out) == 3);
    CHECK(out[0] == 0 && out[1] == 0);            // midpoint is silence, copied to both ears
    CHECK(out[2] == 127 * 256 && out[3] == out[2]);
    CHECK(out[4] == -128 * 256);

    WavInfo stereo16 = {44100, 2, 16, 0, 0};
    const uint8_t b[] = {0x34, 0x12, 0x00, 0x80};
    CHECK(wavToStereo16(b, 4, stereo16, out) == 1);
    CHECK(out[0] == 0x1234 && out[1] == -32768);

    WavInfo stereo24 = {44100, 2, 24, 0, 0};
    const uint8_t c[] = {0xFF, 0x34, 0x12, 0x00, 0x00, 0xC0};
    CHECK(wavToStereo16(c, 6, stereo24, out) == 1);
    CHECK(out[0] == 0x1234 && out[1] == -16384);

    WavInfo mono32 = {44100, 1, 32, 0, 0};
    const uint8_t d[] = {0, 0, 0x00, 0x40};
    CHECK(wavToStereo16(d, 4, mono32, out) == 1);
    CHECK(out[0] == 16384 && out[1] == 16384);

    CHECK(wavToStereo16(b, 3, stereo16, out) == 0);  // less than one frame
}

namespace {
// Resamples a sine in awkward block sizes and measures its frequency.
double resampledHz(uint32_t inRate, double hz, double seconds) {
    Resampler r;
    r.begin(inRate, 44100);
    std::vector<int16_t> out;
    const size_t total = static_cast<size_t>(seconds * inRate);
    size_t done = 0;
    std::vector<int16_t> in, block;
    while (done < total) {
        const size_t n = total - done < 251 ? total - done : 251;
        in.resize(n * 2);
        for (size_t i = 0; i < n; ++i) {
            const int16_t s = static_cast<int16_t>(20000.0 * sin(2.0 * M_PI * hz * (done + i) / inRate));
            in[2 * i] = s;
            in[2 * i + 1] = static_cast<int16_t>(-s);
        }
        block.resize(r.maxOutput(n) * 2);
        const size_t got = r.process(in.data(), n, block.data());
        CHECK(got <= r.maxOutput(n));
        out.insert(out.end(), block.begin(), block.begin() + got * 2);
        done += n;
    }
    // Length must match the rate ratio.
    CHECK_NEAR(static_cast<double>(out.size() / 2), seconds * 44100.0, 3.0);
    double first = -1, last = -1;
    int crossings = 0;
    for (size_t i = 1; i < out.size() / 2; ++i) {
        const double a = out[2 * (i - 1)], b = out[2 * i];
        CHECK(out[2 * i + 1] == -out[2 * i] || abs(out[2 * i + 1] + out[2 * i]) <= 1);  // channels stay separate
        if (a < 0 && b >= 0) {
            const double t = (i - 1) + (-a) / (b - a);
            if (first < 0) first = t;
            last = t;
            ++crossings;
        }
    }
    return (crossings - 1) * 44100.0 / (last - first);
}
}  // namespace

TEST(resampling_keeps_the_pitch) {
    CHECK_NEAR(resampledHz(48000, 440.0, 3.0), 440.0, 0.05);
    CHECK_NEAR(resampledHz(22050, 440.0, 3.0), 440.0, 0.05);
    CHECK_NEAR(resampledHz(32000, 1000.0, 3.0), 1000.0, 0.05);
    CHECK_NEAR(resampledHz(8000, 200.0, 3.0), 200.0, 0.05);
}

TEST(the_engine_rate_passes_through_unchanged) {
    Resampler r;
    r.begin(44100, 44100);
    CHECK(r.passthrough());
    const int16_t in[] = {1, -1, 2, -2, 3, -3};
    int16_t out[6];
    CHECK(r.process(in, 3, out) == 3);
    CHECK(memcmp(in, out, sizeof(in)) == 0);
}

TEST(block_boundaries_leave_no_discontinuity) {
    // A slow ramp resampled one frame at a time must stay monotonic.
    Resampler r;
    r.begin(48000, 44100);
    int16_t previous = -32768;
    int16_t out[8];
    for (int i = 0; i < 2000; ++i) {
        const int16_t in[2] = {static_cast<int16_t>(i * 10), static_cast<int16_t>(i * 10)};
        const size_t got = r.process(in, 1, out);
        for (size_t k = 0; k < got; ++k) {
            CHECK(out[2 * k] >= previous);
            previous = out[2 * k];
        }
    }
}

int main() { return host_test::runAll(); }
