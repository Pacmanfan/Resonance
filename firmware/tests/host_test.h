// Minimal test harness for the portable core, built with the host compiler
// (firmware/tools/host_tests.py). No framework, so the tests need nothing
// downloaded.
#pragma once

#include <math.h>
#include <stdio.h>
#include <string.h>

namespace host_test {

struct Case {
    const char* name;
    void (*run)();
    Case* next;
};

inline Case*& head() { static Case* h = nullptr; return h; }
inline int& failures() { static int f = 0; return f; }

struct Registrar {
    Registrar(Case* c) { c->next = head(); head() = c; }
};

inline int runAll() {
    int cases = 0, failed = 0;
    for (Case* c = head(); c != nullptr; c = c->next) {
        const int before = failures();
        c->run();
        ++cases;
        const bool ok = failures() == before;
        if (!ok) ++failed;
        printf("%s %s\n", ok ? "PASS" : "FAIL", c->name);
    }
    printf("%d case(s), %d failed\n", cases, failed);
    return failed == 0 ? 0 : 1;
}

}  // namespace host_test

#define TEST(name)                                                        \
    static void name();                                                   \
    static host_test::Case case_##name = {#name, name, nullptr};          \
    static host_test::Registrar registrar_##name(&case_##name);           \
    static void name()

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            printf("  %s:%d: CHECK(%s)\n", __FILE__, __LINE__, #cond);    \
            ++host_test::failures();                                      \
        }                                                                 \
    } while (0)

#define CHECK_NEAR(a, b, tol)                                             \
    do {                                                                  \
        const double va = (a), vb = (b);                                  \
        if (!(fabs(va - vb) <= (tol))) {                                  \
            printf("  %s:%d: %s = %g, expected %g\n", __FILE__, __LINE__, #a, va, vb); \
            ++host_test::failures();                                      \
        }                                                                 \
    } while (0)
