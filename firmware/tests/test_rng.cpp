// Random-number sources (RES-MIND-001).
#include "host_test.h"
#include "rc_rng.h"

using namespace rc;

TEST(the_software_source_says_what_it_is) {
    SoftwareRng rng(7);
    CHECK(rng.id() == RngId::Software);
    CHECK(rng.rngClass() == RngClass::SoftwareControl);
    CHECK(rng.available());
    CHECK(!rng.physicalEntropy());  // never, whatever it produces
    CHECK(rng.suitable());          // as a control
    CHECK(strstr(rng.status(), "not random") != nullptr);
    CHECK(strcmp(rngClassName(rng.rngClass()), "Software control") == 0);
}

TEST(the_same_seed_gives_the_same_samples) {
    SoftwareRng a(12345), b(12345), c(12346);
    CHECK(a.begin() && b.begin() && c.begin());
    uint32_t x[64], y[64], z[64];
    CHECK(a.collect(x, 64) && b.collect(y, 64) && c.collect(z, 64));
    CHECK(memcmp(x, y, sizeof(x)) == 0);
    CHECK(memcmp(x, z, sizeof(x)) != 0);
    // begin() starts the same run again.
    CHECK(a.begin());
    CHECK(a.collect(y, 64));
    CHECK(memcmp(x, y, sizeof(x)) == 0);
}

TEST(the_sequence_is_fixed_for_all_time) {
    // Recorded experiments name a seed; these values must never change.
    SoftwareRng rng(1);
    rng.begin();
    uint32_t first[4];
    rng.collect(first, 4);
    SoftwareRng again(1);
    again.begin();
    for (uint32_t v : first) CHECK(again.next() == v);
    // A zero seed is as good as any other.
    SoftwareRng zero(0);
    zero.begin();
    uint32_t any = 0;
    for (int i = 0; i < 8; ++i) any |= zero.next();
    CHECK(any != 0);
}

TEST(software_samples_look_balanced) {
    SoftwareRng rng(99);
    rng.begin();
    uint64_t ones = 0;
    const int n = 20000;
    for (int i = 0; i < n; ++i) ones += static_cast<uint64_t>(__builtin_popcount(rng.next()));
    const double z = (2.0 * static_cast<double>(ones) - n * 32.0) / sqrt(n * 32.0);
    CHECK(fabs(z) < 4.0);
}

TEST(hardware_that_is_not_fitted_is_not_offered) {
    ExternalRngPlaceholder external;
    CHECK(external.id() == RngId::External);
    CHECK(!external.available());
    CHECK(!external.begin());
    CHECK(!external.physicalEntropy() && !external.suitable());
    uint32_t sample = 0x1234;
    CHECK(!external.collect(&sample, 1));
    CHECK(sample == 0x1234);  // nothing is made up in its place
    CHECK(external.maxRate() == 0);
}

TEST(every_class_has_a_name_that_does_not_say_true_random) {
    for (int c = 0; c <= 4; ++c) {
        const char* name = rngClassName(static_cast<RngClass>(c));
        CHECK(name[0] != '?' && name[0] != '\0');
        CHECK(strstr(name, "rue random") == nullptr);
    }
}

int main() { return host_test::runAll(); }
