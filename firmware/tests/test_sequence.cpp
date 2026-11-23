// Frequency sequences (RES-SESS-012).
#include "host_test.h"
#include "rc_sequence.h"
#include "rc_settings.h"

#include <math.h>
#include <string.h>
#include <string>
#include <vector>

using namespace rc;

namespace {

std::vector<std::string> linesOf(const Sequence& s) {
    std::vector<std::string> lines;
    char line[kSequenceLineMax];
    for (uint8_t i = 0; sequenceLine(s, i, line, sizeof(line)); ++i) lines.push_back(line);
    return lines;
}

bool parse(const std::vector<std::string>& lines, Sequence& out, std::string* error = nullptr) {
    SequenceParser parser;
    parser.begin();
    for (const auto& line : lines) parser.line(line.c_str());
    if (error != nullptr) *error = parser.error();
    return parser.finish(out);
}

}  // namespace

TEST(the_default_is_the_solfeggio_set_and_only_a_starting_point) {
    Sequence s = defaultSequence();
    CHECK(isValidSequence(s));
    CHECK(s.stepCount == 9);
    CHECK(s.steps[0].hz == 174.0f && s.steps[4].hz == 528.0f && s.steps[8].hz == 963.0f);
    for (uint8_t i = 0; i < s.stepCount; ++i) CHECK(s.steps[i].seconds == 120);
    // It can be changed like any other: a step altered, one removed, one added.
    s.steps[4].hz = 432.0f;
    s.steps[4].seconds = 300;
    --s.stepCount;
    CHECK(isValidSequence(s));
    s.stepCount = 10;
    s.steps[8] = {852.0f, 60};
    s.steps[9] = {111.0f, 60};
    CHECK(isValidSequence(s));
}

TEST(a_sequence_is_checked) {
    Sequence s = defaultSequence();
    s.stepCount = 0;
    CHECK(!isValidSequence(s));
    s.stepCount = kMaxSequenceSteps + 1;
    CHECK(!isValidSequence(s));
    s = defaultSequence();
    s.steps[2].hz = 19.9f;
    CHECK(!isValidSequence(s));
    s.steps[2].hz = 1000.1f;
    CHECK(!isValidSequence(s));
    s.steps[2].hz = NAN;
    CHECK(!isValidSequence(s));
    s.steps[2].hz = 396.0f;
    s.steps[3].seconds = 0;
    CHECK(!isValidSequence(s));
    s.steps[3].seconds = 1;
    s.glideS = kSequenceGlideMaxS + 1;
    CHECK(!isValidSequence(s));
    s.glideS = 0;
    s.beatHz = 0.4f;
    CHECK(!isValidSequence(s));
    s.beatHz = NAN;
    CHECK(!isValidSequence(s));
    s.beatHz = 7.0f;
    CHECK(isValidSequence(s));
    memset(s.name, 'x', sizeof(s.name));
    CHECK(!isValidSequence(s));
}

TEST(a_sequence_becomes_a_session_that_plays_its_steps) {
    Sequence s = {};
    strcpy(s.name, "Three");
    s.stepCount = 3;
    s.steps[0] = {396.0f, 60};
    s.steps[1] = {528.0f, 90};
    s.steps[2] = {639.0f, 30};
    s.glideS = 0;
    s.beatHz = 0.0f;
    Session session = sequenceSession(s);
    CHECK(isValidSession(session));
    CHECK(strcmp(session.name, "Three") == 0);
    CHECK(sessionSeconds(session) == 180);
    SessionRunner r;
    CHECK(r.start(session, 200.0f, 10.0f, 0));
    r.advance(1000);
    CHECK(r.primaryHz() == 396.0f);
    CHECK(r.beatHz() == 10.0f);      // the beat is left alone
    r.advance(60000);
    CHECK(r.primaryHz() == 528.0f);
    r.advance(90000);
    CHECK(r.primaryHz() == 639.0f);
    r.advance(29000);
    CHECK(r.state() == SessionState::Finished);

    // With a glide and a beat: each later step is reached in the glide time and then held.
    s.glideS = 20;
    s.beatHz = 6.0f;
    session = sequenceSession(s);
    CHECK(isValidSession(session));
    CHECK(session.stages[0].sweep == Sweep::Hold);
    CHECK(session.stages[1].sweep == Sweep::Linear && session.stages[1].transitionS == 20);
    CHECK(r.start(session, 200.0f, 10.0f, 0));
    r.advance(60000 + 10000);
    CHECK_NEAR(r.primaryHz(), (396.0 + 528.0) / 2.0, 0.01);
    CHECK_NEAR(r.beatHz(), 6.0, 1e-4);
    r.advance(10000);
    CHECK_NEAR(r.primaryHz(), 528.0, 0.01);
    r.advance(60000);
    CHECK_NEAR(r.primaryHz(), 528.0, 0.01);  // held
    // A glide as long as the step, or longer, is the whole step.
    s.glideS = 30;
    session = sequenceSession(s);
    CHECK(isValidSession(session));
    CHECK(session.stages[2].transitionS == 0 && session.stages[2].sweep == Sweep::Linear);
    // A sequence that is not valid makes no session.
    s.steps[1].hz = 5.0f;
    CHECK(!isValidSession(sequenceSession(s)));
}

TEST(the_text_form_gives_the_same_sequence_back) {
    Sequence s = defaultSequence();
    strcpy(s.name, "My set");
    s.steps[1].hz = 285.5f;
    s.steps[1].seconds = 65535;
    s.glideS = 15;
    s.beatHz = 7.83f;
    const auto lines = linesOf(s);
    CHECK(lines.size() == 4u + s.stepCount);
    CHECK(lines[0] == "RESONANCE SEQUENCE 1");
    Sequence back = {};
    CHECK(parse(lines, back));
    CHECK(strcmp(back.name, "My set") == 0);
    CHECK(back.stepCount == s.stepCount && back.glideS == 15);
    CHECK_NEAR(back.beatHz, 7.83, 1e-5);
    for (uint8_t i = 0; i < s.stepCount; ++i) {
        CHECK(back.steps[i].hz == s.steps[i].hz);
        CHECK(back.steps[i].seconds == s.steps[i].seconds);
    }
    char small[30];
    CHECK(!sequenceLine(s, 1, small, 10));   // too small a buffer is reported, not overrun
    CHECK(!sequenceLine(s, 4 + s.stepCount, small, sizeof(small)));
}

TEST(a_damaged_or_foreign_file_is_refused_and_a_later_version_is_read) {
    Sequence out = {};
    std::string error;
    CHECK(parse({"# mine", "RESONANCE SEQUENCE 1", "name=Two", "colour=blue", "step hz=396 d=60 tint=3", "step d=30 hz=417.5"}, out));
    CHECK(out.stepCount == 2 && out.steps[1].hz == 417.5f && out.steps[1].seconds == 30);
    CHECK(out.glideS == 0 && out.beatHz == 0.0f);
    CHECK(!parse({"RESONANCE SESSION 1", "step hz=396 d=60"}, out, &error));
    CHECK(error == "not a sequence file");
    CHECK(!parse({"RESONANCE SEQUENCE 2", "step hz=396 d=60"}, out, &error));
    CHECK(error == "written by a later version");
    CHECK(!parse({"step hz=396 d=60"}, out));
    CHECK(!parse({"RESONANCE SEQUENCE 1"}, out));                       // no step
    CHECK(!parse({"RESONANCE SEQUENCE 1", "step hz=396"}, out));        // no duration
    CHECK(!parse({"RESONANCE SEQUENCE 1", "step hz=nan d=60"}, out));
    CHECK(!parse({"RESONANCE SEQUENCE 1", "step hz=-396 d=60"}, out));
    CHECK(!parse({"RESONANCE SEQUENCE 1", "step hz=5000 d=60"}, out));
    CHECK(!parse({"RESONANCE SEQUENCE 1", "step hz=396 d=70000"}, out));
    CHECK(!parse({"RESONANCE SEQUENCE 1", "glide=601", "step hz=396 d=60"}, out, &error));
    CHECK(error == "bad glide");
    CHECK(!parse({"RESONANCE SEQUENCE 1", "beat=99", "step hz=396 d=60"}, out));
    CHECK(!parse({"RESONANCE SEQUENCE 1", std::string(200, 'x')}, out, &error));
    CHECK(error == "line too long");
    std::vector<std::string> many = {"RESONANCE SEQUENCE 1"};
    for (int i = 0; i < kMaxSequenceSteps + 1; ++i) many.push_back("step hz=396 d=60");
    CHECK(!parse(many, out, &error));
    CHECK(error == "too many steps");
}

int main() { return host_test::runAll(); }
