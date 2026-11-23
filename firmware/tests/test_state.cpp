// The advanced settings kept on the card (RES-CFG-004).
#include "host_test.h"
#include "rc_state.h"

#include <string.h>
#include <string>
#include <vector>

using namespace rc;

namespace {

std::vector<std::string> linesOf(const AdvancedState& s) {
    std::vector<std::string> lines;
    char line[kStateLineMax];
    for (uint8_t i = 0; stateLine(s, i, line, sizeof(line)); ++i) lines.push_back(line);
    return lines;
}

bool parse(const std::vector<std::string>& lines, AdvancedState& out, std::string* error = nullptr) {
    StateParser parser;
    parser.begin();
    for (const auto& line : lines) parser.line(line.c_str());
    if (error != nullptr) *error = parser.error();
    return parser.finish(out);
}

// A record with every field away from its default, at or near its limits.
AdvancedState busy() {
    AdvancedState s;
    s.layerOn[0] = false;
    s.layerOn[2] = false;
    s.toneLevel = 35;
    s.partialsLevel = 0;
    s.partial[0] = 100;
    s.partial[1] = 15;
    s.fadeChoice = kMixerFadeChoices - 1;
    s.breath = {250, 250, 250, 250};
    s.breathDepth = kBreathDepthMax;
    s.breathOn = true;
    s.labSlot = 1;
    s.labMatch = true;
    s.lab[0].primaryHz = 1000.0f;
    s.lab[0].beatHz = 40.0f;
    s.lab[0].partial[0] = 100;
    s.lab[0].partial[1] = 100;
    s.lab[0].phaseL = -180;
    s.lab[0].phaseR = -180;
    s.lab[0].invertL = true;
    s.lab[0].invertR = true;
    s.lab[0].lab = {100, -2000, 600, 100, true};
    s.lab[1].primaryHz = 136.1f;
    s.lab[1].beatHz = 7.83f;
    s.lab[1].phaseR = 90;
    s.lab[1].invertR = true;
    s.lab[1].lab = {0, 350, 5, 0, false};
    return s;
}

}  // namespace

TEST(a_new_record_has_every_control_at_rest) {
    const AdvancedState s;
    CHECK(isValidState(s));
    for (bool on : s.layerOn) CHECK(on);
    CHECK(s.toneLevel == 100 && s.partialsLevel == 100 && s.partial[0] == 0 && s.partial[1] == 0);
    CHECK(!s.breathOn && s.breathDepth == 0);
    CHECK(labAtRest(s.lab[0].lab) && labAtRest(s.lab[1].lab));
    CHECK(s.lab[0].primaryHz == 0.0f && s.lab[1].primaryHz == 0.0f);
    CHECK(!s.lab[0].invertL && !s.lab[0].invertR && s.lab[0].phaseL == 0 && s.lab[0].phaseR == 0);
    CHECK(s.labSlot == 0 && !s.labMatch);
}

TEST(the_text_form_gives_back_the_same_record) {
    for (const AdvancedState& s : {AdvancedState{}, busy()}) {
        const auto lines = linesOf(s);
        CHECK(lines.size() == 6);
        CHECK(lines[0] == "RESONANCE STATE 1");
        for (const auto& line : lines) CHECK(line.size() < kStateLineMax);
        AdvancedState back;
        back.toneLevel = 1;  // not what is read
        CHECK(parse(lines, back));
        CHECK(sameState(s, back));
    }
    CHECK(!sameState(AdvancedState{}, busy()));
}

TEST(every_field_counts_in_the_comparison) {
    const AdvancedState base;
    AdvancedState s;
    s.layerOn[3] = false;
    CHECK(!sameState(base, s));
    s = base, s.fadeChoice = 3;
    CHECK(!sameState(base, s));
    s = base, s.breath.holdOutDs = 10;
    CHECK(!sameState(base, s));
    s = base, s.breathOn = true;
    CHECK(!sameState(base, s));
    s = base, s.lab[1].lab.delayUs = 50;
    CHECK(!sameState(base, s));
    s = base, s.lab[0].invertL = true;
    CHECK(!sameState(base, s));
    s = base, s.lab[1].beatHz = 4.0f;
    CHECK(!sameState(base, s));
    s = base, s.labMatch = true;
    CHECK(!sameState(base, s));
}

TEST(lines_and_keys_of_a_later_version_are_left_alone) {
    AdvancedState out;
    CHECK(parse({"RESONANCE STATE 1", "# written by hand", "", "mixer tone=1/80 tint=4 p1=20",
                 "spatial room=3", "breath in=50 out=70 shape=sine", "b width=40 colour=2"},
                out));
    CHECK(out.toneLevel == 80 && out.partial[0] == 20);
    CHECK(out.breath.inhaleDs == 50 && out.breath.exhaleDs == 70 && !out.breathOn);
    CHECK(out.lab[1].lab.widthPercent == 40 && out.lab[0].lab.widthPercent == 100);
    // What a line does not mention keeps its default.
    CHECK(out.partialsLevel == 100 && out.fadeChoice == 2 && out.lab[1].lab.bilateralGolden);
}

TEST(a_header_alone_is_the_record_at_rest) {
    AdvancedState out = busy();
    CHECK(parse({"RESONANCE STATE 1"}, out));
    CHECK(sameState(out, AdvancedState{}));
}

TEST(text_that_is_not_a_valid_record_is_refused_whole) {
    const AdvancedState before = busy();
    AdvancedState out = before;
    std::string error;
    CHECK(!parse({}, out));
    CHECK(!parse({"mixer tone=1/80"}, out, &error));
    CHECK(error == "not a settings file");
    CHECK(!parse({"RESONANCE STATE 2"}, out, &error));
    CHECK(error == "written by a later version");
    CHECK(!parse({"RESONANCE STATE 1", std::string(300, 'x')}, out, &error));
    CHECK(error == "line too long");
    CHECK(!parse({"RESONANCE STATE 1", "mixer tone"}, out, &error));
    CHECK(error == "entry without a value");
    const char* const bad[] = {
        "mixer tone=1/101", "mixer tone=2/50", "mixer tone=50", "mixer gold=1/-1", "mixer noise=yes", "mixer p1=101",
        "mixer fade=6", "mixer p2=1e2", "mixer p2=", "breath on=2", "breath in=256", "breath depth=81",
        "breath in=0", "breath in=5", "breath hold=251", "lab side=c", "lab side=ab", "lab match=2", "a hz=19",
        "a hz=1001", "a hz=nan", "a hz=-200", "a beat=0.4", "a beat=41", "b phl=181", "b phr=-181", "b invl=2",
        "b width=101", "b delay=2001", "b delay=-2001", "b period=601", "b period=4", "b depth=101", "b golden=7",
        "b delay=--5", "b delay=000050",
    };
    for (const char* line : bad) {
        if (parse({"RESONANCE STATE 1", "mixer tone=1/60", line}, out)) {
            printf("  accepted: %s\n", line);
            CHECK(false);
        }
    }
    // A good line before a bad one is not used either.
    CHECK(sameState(out, before));
}

TEST(records_out_of_range_are_not_valid) {
    AdvancedState s;
    s.toneLevel = 101;
    CHECK(!isValidState(s));
    s = AdvancedState{}, s.fadeChoice = kMixerFadeChoices;
    CHECK(!isValidState(s));
    s = AdvancedState{}, s.breath.inhaleDs = 0;
    CHECK(!isValidState(s));
    s = AdvancedState{}, s.breathDepth = kBreathDepthMax + 1;
    CHECK(!isValidState(s));
    s = AdvancedState{}, s.labSlot = 2;
    CHECK(!isValidState(s));
    s = AdvancedState{}, s.lab[1].lab.delayUs = 2050;
    CHECK(!isValidState(s));
    s = AdvancedState{}, s.lab[0].primaryHz = 5.0f;
    CHECK(!isValidState(s));
    s = AdvancedState{}, s.lab[0].phaseR = 200;
    CHECK(!isValidState(s));
}

TEST(a_line_is_refused_when_the_buffer_is_too_small) {
    char small[40];
    CHECK(stateLine(busy(), 0, small, sizeof(small)));
    CHECK(!stateLine(busy(), 4, small, sizeof(small)));
    CHECK(!stateLine(busy(), 6, small, sizeof(small)));
}
int main() { return host_test::runAll(); }
