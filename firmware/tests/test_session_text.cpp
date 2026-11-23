// The text form of a session (RES-SESS-002, RES-CFG-002).
#include "host_test.h"
#include "rc_session_text.h"

#include <initializer_list>
#include <string>
#include <vector>

using namespace rc;

namespace {

std::vector<std::string> linesOf(const Session& s) {
    std::vector<std::string> lines;
    char line[kSessionLineMax];
    for (uint8_t i = 0; sessionLine(s, i, line, sizeof(line)); ++i) lines.push_back(line);
    return lines;
}

bool parse(std::initializer_list<const char*> lines, Session& out, std::string* error = nullptr) {
    SessionParser parser;
    parser.begin();
    for (const char* line : lines) parser.line(line);
    if (error != nullptr) *error = parser.error();
    return parser.finish(out);
}

Session everything() {
    Session s = {};
    strcpy(s.name, "Evening descent");
    s.endFadeS = 90;
    s.stageCount = 3;
    s.stages[0] = {120, 200.0f, 10.0f, Sweep::Hold};
    s.stages[1] = {600, 174.5f, 6.25f, Sweep::Exponential, 210.0f, 12.0f};
    s.stages[1].transitionS = 300;
    s.stages[1].flags = kStageSetsPartials | kStageSetsNoise;
    s.stages[1].partial[0] = 40;
    s.stages[1].partial[1] = 15;
    s.stages[1].noiseType = 2;
    s.stages[1].noiseVolume = 35;
    s.stages[2] = {65535, 0, 4.0f, Sweep::Linear};
    s.stages[2].flags = kStageSetsNoise;
    return s;
}

bool sameStage(const Stage& a, const Stage& b) {
    return a.durationS == b.durationS && a.primaryHz == b.primaryHz && a.beatHz == b.beatHz && a.sweep == b.sweep &&
           a.fromPrimaryHz == b.fromPrimaryHz && a.fromBeatHz == b.fromBeatHz && a.transitionS == b.transitionS &&
           a.flags == b.flags && a.partial[0] == b.partial[0] && a.partial[1] == b.partial[1] &&
           a.noiseType == b.noiseType && a.noiseVolume == b.noiseVolume;
}

}  // namespace

TEST(a_session_survives_being_written_and_read) {
    const Session s = everything();
    CHECK(isValidSession(s));
    const auto lines = linesOf(s);
    CHECK(lines.size() == 6);
    CHECK(lines[0] == "RESONANCE SESSION 1");
    CHECK(lines[1] == "name=Evening descent");
    CHECK(lines[2] == "endfade=90");
    CHECK(lines[3] == "stage d=120 sweep=hold p=200.00 b=10.00");
    CHECK(lines[4] == "stage d=600 t=300 sweep=exp p=174.50 b=6.25 fp=210.00 fb=12.00 phi=40 phi2=15 noise=pink nv=35");
    CHECK(lines[5] == "stage d=65535 sweep=lin b=4.00 noise=off nv=0");
    SessionParser parser;
    parser.begin();
    for (const auto& line : lines) CHECK(parser.line((line + "\r\n").c_str()));
    Session back = {};
    CHECK(parser.finish(back));
    CHECK(strcmp(back.name, s.name) == 0);
    CHECK(back.endFadeS == 90);
    CHECK(back.stageCount == 3);
    for (uint8_t i = 0; i < 3; ++i) CHECK(sameStage(back.stages[i], s.stages[i]));
}

TEST(every_factory_program_and_the_longest_session_fit_their_lines) {
    for (uint8_t i = 0; i < factorySessionCount(); ++i) {
        const auto lines = linesOf(factorySession(i));
        CHECK(lines.size() == 3u + factorySession(i).stageCount);
        CHECK(lines[2] == "endfade=device");
        SessionParser parser;
        parser.begin();
        for (const auto& line : lines) CHECK(parser.line(line.c_str()));
        Session back = {};
        CHECK(parser.finish(back));
        CHECK(back.endFadeS == kFadeDeviceChoice);
        for (uint8_t n = 0; n < back.stageCount; ++n) CHECK(sameStage(back.stages[n], factorySession(i).stages[n]));
    }
    // The widest line a valid stage can give still fits, with room to spare.
    Session s = everything();
    s.stages[1] = {65535, 1000.0f, 39.99f, Sweep::Exponential, 999.99f, 39.99f};
    s.stages[1].transitionS = 65535;
    s.stages[1].flags = kStageKnownFlags;
    s.stages[1].partial[0] = s.stages[1].partial[1] = 100;
    s.stages[1].noiseType = 3;
    s.stages[1].noiseVolume = 100;
    char line[kSessionLineMax];
    CHECK(sessionLine(s, 4, line, sizeof(line)));
    CHECK(strlen(line) < kSessionLineMax - 16);
    CHECK(!sessionLine(s, 4, line, 40));    // too small a buffer is reported, not overrun
    CHECK(!sessionLine(s, 6, line, sizeof(line)));
}

TEST(a_file_made_by_hand_is_read_leniently) {
    Session s = {};
    CHECK(parse({"# my session", "", "RESONANCE SESSION 1", "  name=Hand made  ", "stage d=60 b=10",
                 "stage   d=300\tsweep=lin  b=6", "colour=blue", "stage d=30 sweep=hold b=6 breath=2 colour=green"},
                s));
    CHECK(strcmp(s.name, "Hand made") == 0);
    CHECK(s.endFadeS == kFadeDeviceChoice);          // not given
    CHECK(s.stageCount == 3);
    CHECK(s.stages[0].sweep == Sweep::Hold);         // not given
    CHECK(s.stages[1].sweep == Sweep::Linear);
    CHECK_NEAR(s.stages[1].beatHz, 6.0, 1e-6);
    CHECK(s.stages[2].flags == 0);                   // keys of a later version are skipped
    // No name: a default one.
    CHECK(parse({"RESONANCE SESSION 1", "stage d=60 b=10"}, s));
    CHECK(strcmp(s.name, "Session") == 0);
    // A long name is cut, not refused.
    CHECK(parse({"RESONANCE SESSION 1", "name=A name that is far too long to show", "stage d=60 b=10"}, s));
    CHECK(strlen(s.name) == sizeof(s.name) - 1);
}

TEST(damaged_and_hostile_files_are_refused) {
    Session s = {};
    std::string why;
    CHECK(!parse({}, s));                                                       // empty
    CHECK(!parse({"stage d=60 b=10"}, s, &why));                                // no header
    CHECK(why == "not a session file");
    CHECK(!parse({"RESONANCE SESSION 2", "stage d=60 b=10"}, s, &why));
    CHECK(why == "written by a later version");
    CHECK(!parse({"RESONANCE SESSION 1"}, s));                                  // no stage
    CHECK(!parse({"RESONANCE SESSION 1", "stage b=10"}, s, &why));
    CHECK(why == "stage without a duration");
    CHECK(!parse({"RESONANCE SESSION 1", "stage d=60 b=nan"}, s));
    CHECK(!parse({"RESONANCE SESSION 1", "stage d=60 b=inf"}, s));
    CHECK(!parse({"RESONANCE SESSION 1", "stage d=60 b=-4"}, s));
    CHECK(!parse({"RESONANCE SESSION 1", "stage d=60 b=4x"}, s));
    CHECK(!parse({"RESONANCE SESSION 1", "stage d=60 b=1e9"}, s));              // read, then out of range
    CHECK(!parse({"RESONANCE SESSION 1", "stage d=70000 b=4"}, s));
    CHECK(!parse({"RESONANCE SESSION 1", "stage d=600000000000 b=4"}, s));
    CHECK(!parse({"RESONANCE SESSION 1", "stage d=60 sweep=zigzag b=4"}, s));
    CHECK(!parse({"RESONANCE SESSION 1", "stage d=60 b=4 phi=101"}, s));
    CHECK(!parse({"RESONANCE SESSION 1", "stage d=60 b=4 noise=purple"}, s));
    CHECK(!parse({"RESONANCE SESSION 1", "stage d=60 b=4 t=10"}, s));           // transition on a hold stage
    CHECK(!parse({"RESONANCE SESSION 1", "stage d=60 b=4 d"}, s, &why));
    CHECK(why == "stage entry without a value");
    CHECK(!parse({"RESONANCE SESSION 1", "endfade=9999", "stage d=60 b=4"}, s));
    CHECK(!parse({"RESONANCE SESSION 1", "name=", "stage d=60 b=4"}, s));
    CHECK(!parse({"RESONANCE SESSION 1", "name=bad\x01name", "stage d=60 b=4"}, s));
    CHECK(!parse({"RESONANCE SESSION 1", "stage d=0 b=4", "stage d=60 b=4"}, s));  // endless stage among others
    const std::string longLine = "stage d=60 b=4 " + std::string(200, 'x');
    CHECK(!parse({"RESONANCE SESSION 1", longLine.c_str()}, s, &why));
    CHECK(why == "line too long");
    // Too many stages.
    SessionParser parser;
    parser.begin();
    CHECK(parser.line("RESONANCE SESSION 1"));
    for (int i = 0; i < kMaxStages; ++i) CHECK(parser.line("stage d=10 b=5"));
    CHECK(!parser.line("stage d=10 b=5"));
    CHECK(std::string(parser.error()) == "too many stages");
    CHECK(!parser.line("stage d=10 b=5"));     // stays failed
    CHECK(!parser.finish(s));
    // After begin() the parser is clean again.
    parser.begin();
    CHECK(parser.line("RESONANCE SESSION 1"));
    CHECK(parser.line("stage d=10 b=5"));
    CHECK(parser.finish(s));
    CHECK(s.stageCount == 1);
}

TEST(a_stage_recording_survives_the_text_form_spaces_included) {
    Session s = everything();
    s.stages[0].flags = kStageSetsVoice;
    s.stages[0].voiceVolume = 55;
    strcpy(s.stages[0].voice, "VOICE/Body scan part 1.mp3");
    s.stages[2].flags |= kStageSetsVoice;   // names none: stop
    // The longest line a stage can make still fits.
    s.stages[1].flags |= kStageSetsVoice;
    s.stages[1].voiceVolume = 100;
    s.stages[1].durationS = 65535;
    s.stages[1].transitionS = 65535;
    s.stages[1].primaryHz = 1000.0f;
    s.stages[1].fromPrimaryHz = 999.99f;
    strcpy(s.stages[1].voice, "VOICE/");
    memset(s.stages[1].voice + 6, 'a', kStageVoiceLength - 11);
    strcpy(s.stages[1].voice + kStageVoiceLength - 5, ".mp3");
    CHECK(isValidSession(s));
    const auto lines = linesOf(s);
    CHECK(lines.size() == 6);
    CHECK(lines[3].find(" vv=55 voice=VOICE/Body scan part 1.mp3") != std::string::npos);
    CHECK(lines[4].size() < kSessionLineMax - 1);
    SessionParser parser;
    parser.begin();
    for (const auto& line : lines) parser.line(line.c_str());
    Session back = {};
    CHECK(parser.finish(back));
    for (uint8_t i = 0; i < 3; ++i) {
        CHECK(sameStage(s.stages[i], back.stages[i]));
        CHECK(back.stages[i].voiceVolume == s.stages[i].voiceVolume);
        CHECK(strcmp(back.stages[i].voice, s.stages[i].voice) == 0);
    }
}

TEST(a_bad_stage_recording_is_refused) {
    Session out = {};
    std::string error;
    CHECK(parse({"RESONANCE SESSION 1", "stage d=60 sweep=hold vv=40 voice=rain.wav"}, out));
    CHECK(strcmp(out.stages[0].voice, "rain.wav") == 0 && out.stages[0].voiceVolume == 40);
    CHECK(!parse({"RESONANCE SESSION 1", "stage d=60 sweep=hold vv=40 voice=../../etc/passwd.mp3"}, out));
    CHECK(!parse({"RESONANCE SESSION 1", "stage d=60 sweep=hold vv=400 voice=rain.wav"}, out, &error));
    CHECK(error == "bad recording volume");
    CHECK(!parse({"RESONANCE SESSION 1", "stage d=60 sweep=hold voice=notes.txt"}, out));
    const std::string longName = "stage d=60 sweep=hold voice=" + std::string(kStageVoiceLength, 'a') + ".mp3";
    CHECK(!parse({"RESONANCE SESSION 1", longName.c_str()}, out, &error));
    CHECK(error == "recording name too long");
    // An empty name stops playback and is valid.
    CHECK(parse({"RESONANCE SESSION 1", "stage d=60 sweep=hold vv=0 voice="}, out));
    CHECK((out.stages[0].flags & kStageSetsVoice) != 0 && out.stages[0].voice[0] == '\0');
}

int main() { return host_test::runAll(); }
