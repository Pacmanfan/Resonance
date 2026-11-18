// Rules for names, paths and uploaded content (RES-WEB-005).
#include "host_test.h"
#include "rc_paths.h"

#include <string>

using namespace rc;

TEST(ordinary_names_are_accepted) {
    CHECK(isSafeName("rain.mp3"));
    CHECK(isSafeName("Ocean waves - 08m.MP3"));
    CHECK(isSafeName("OCEAN"));
    CHECK(isSafeName("a"));
    CHECK(isSafeName(std::string(kMaxNameLength, 'x').c_str()));
}

TEST(names_that_could_leave_their_folder_or_break_fat_are_refused) {
    CHECK(!isSafeName(nullptr));
    CHECK(!isSafeName(""));
    CHECK(!isSafeName(".."));
    CHECK(!isSafeName("."));
    CHECK(!isSafeName(".hidden"));
    CHECK(!isSafeName("a/b"));
    CHECK(!isSafeName("a\\b"));
    CHECK(!isSafeName("../rain.mp3"));
    CHECK(!isSafeName("C:rain.mp3"));
    CHECK(!isSafeName("rain?.mp3"));
    CHECK(!isSafeName("rain*.mp3"));
    CHECK(!isSafeName("ra\"in.mp3"));
    CHECK(!isSafeName("rain<1>.mp3"));
    CHECK(!isSafeName("a|b"));
    CHECK(!isSafeName("rain.mp3 "));
    CHECK(!isSafeName(" rain.mp3"));
    CHECK(!isSafeName("rain."));
    CHECK(!isSafeName("rain\n.mp3"));
    CHECK(!isSafeName("r\xC3\xA4in.mp3"));  // non-ASCII
    CHECK(!isSafeName(std::string(kMaxNameLength + 1, 'x').c_str()));
}

TEST(relative_paths) {
    CHECK(isSafeRelativePath(""));  // the library itself
    CHECK(isSafeRelativePath("OCEAN"));
    CHECK(isSafeRelativePath("NATURE/Birds"));
    CHECK(isSafeRelativePath("a/b/c"));
    CHECK(!isSafeRelativePath(nullptr));
    CHECK(!isSafeRelativePath("/OCEAN"));
    CHECK(!isSafeRelativePath("OCEAN/"));
    CHECK(!isSafeRelativePath("OCEAN//x"));
    CHECK(!isSafeRelativePath(".."));
    CHECK(!isSafeRelativePath("OCEAN/.."));
    CHECK(!isSafeRelativePath("OCEAN/../../CONFIG"));
    CHECK(!isSafeRelativePath("OCEAN/./x"));
    CHECK(!isSafeRelativePath("OCEAN\\x"));
    CHECK(!isSafeRelativePath(std::string(kMaxRelativePath + 1, 'a').c_str()));
}

TEST(sound_file_names_need_a_known_extension) {
    CHECK(isSoundFileName("rain.mp3"));
    CHECK(isSoundFileName("rain.WAV"));
    CHECK(!isSoundFileName("rain.ogg"));
    CHECK(!isSoundFileName("rain.mp3.exe"));
    CHECK(!isSoundFileName("rain"));
    CHECK(!isSoundFileName(".mp3"));
    CHECK(!isSoundFileName("../rain.mp3"));
}

TEST(content_is_recognised_by_its_first_bytes) {
    const uint8_t wav[] = {'R', 'I', 'F', 'F', 1, 2, 3, 4, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' '};
    const uint8_t id3[] = {'I', 'D', '3', 4, 0, 0, 0, 0, 0, 10, 0, 0};
    const uint8_t frame[] = {0xFF, 0xFB, 0x90, 0x64, 0, 0, 0, 0, 0, 0, 0, 0};       // MPEG-1 layer III, 128 kbit/s
    const uint8_t frame2[] = {0xFF, 0xF3, 0x48, 0xC4, 0, 0, 0, 0, 0, 0, 0, 0};      // MPEG-2 layer III
    CHECK(sniffSound(wav, sizeof(wav)) == SoundKind::Wav);
    CHECK(sniffSound(id3, sizeof(id3)) == SoundKind::Mp3);
    CHECK(sniffSound(frame, sizeof(frame)) == SoundKind::Mp3);
    CHECK(sniffSound(frame2, sizeof(frame2)) == SoundKind::Mp3);
}

TEST(other_content_is_not_mistaken_for_sound) {
    const uint8_t exe[] = {'M', 'Z', 0x90, 0, 3, 0, 0, 0, 4, 0, 0, 0};
    const uint8_t html[] = {'<', 'h', 't', 'm', 'l', '>', '<', 's', 'c', 'r', 'i', 'p'};
    const uint8_t avi[] = {'R', 'I', 'F', 'F', 1, 2, 3, 4, 'A', 'V', 'I', ' '};
    const uint8_t ones[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // erased flash, not MP3
    const uint8_t layer1[] = {0xFF, 0xFF, 0x90, 0x64, 0, 0, 0, 0, 0, 0, 0, 0};       // layer I
    const uint8_t zeros[12] = {};
    CHECK(sniffSound(exe, sizeof(exe)) == SoundKind::Unknown);
    CHECK(sniffSound(html, sizeof(html)) == SoundKind::Unknown);
    CHECK(sniffSound(avi, sizeof(avi)) == SoundKind::Unknown);
    CHECK(sniffSound(ones, sizeof(ones)) == SoundKind::Unknown);
    CHECK(sniffSound(layer1, sizeof(layer1)) == SoundKind::Unknown);
    CHECK(sniffSound(zeros, sizeof(zeros)) == SoundKind::Unknown);
    const uint8_t wav[] = {'R', 'I', 'F', 'F', 1, 2, 3, 4, 'W', 'A', 'V', 'E'};
    CHECK(sniffSound(wav, 11) == SoundKind::Unknown);  // too short to tell
    CHECK(sniffSound(nullptr, 100) == SoundKind::Unknown);
}

TEST(the_extension_must_agree_with_the_content) {
    CHECK(nameMatchesKind("rain.wav", SoundKind::Wav));
    CHECK(nameMatchesKind("rain.MP3", SoundKind::Mp3));
    CHECK(!nameMatchesKind("rain.mp3", SoundKind::Wav));
    CHECK(!nameMatchesKind("rain.wav", SoundKind::Mp3));
    CHECK(!nameMatchesKind("rain.mp3", SoundKind::Unknown));
}

TEST(json_escaping) {
    char out[64];
    CHECK(jsonEscape("plain name.mp3", out, sizeof(out)) && strcmp(out, "plain name.mp3") == 0);
    CHECK(jsonEscape("a\"b\\c", out, sizeof(out)) && strcmp(out, "a\\\"b\\\\c") == 0);
    CHECK(jsonEscape("line\nbreak\t", out, sizeof(out)) && strcmp(out, "line\\u000abreak\\u0009") == 0);
    CHECK(jsonEscape("caf\xC3\xA9", out, sizeof(out)) && strcmp(out, "caf??") == 0);
    CHECK(jsonEscape("</script>", out, sizeof(out)) && strcmp(out, "</script>") == 0);  // harmless inside JSON served as JSON
    char small[6];
    CHECK(!jsonEscape("too long for this", small, sizeof(small)));
    CHECK(strlen(small) < sizeof(small));  // still terminated
}

int main() { return host_test::runAll(); }
