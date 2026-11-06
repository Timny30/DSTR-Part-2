#pragma once
#include <iostream>

// Minimal test helper. CHECK records a failure and carries on, so one run
// reports every failing check.
namespace TestUtil {
    inline int& checks() { static int value = 0; return value; }
    inline int& failures() { static int value = 0; return value; }

    inline int finish(const char* suite) {
        std::cout << suite << ": " << (checks() - failures()) << "/" << checks() << " checks passed\n";
        return failures() == 0 ? 0 : 1;
    }

    // Counts live objects, so tests can prove that every node was freed.
    struct Counted {
        static int& live() { static int value = 0; return value; }
        int value;
        Counted(int v = 0) : value(v) { live()++; }
        Counted(const Counted& other) : value(other.value) { live()++; }
        Counted& operator=(const Counted& other) { value = other.value; return *this; }
        ~Counted() { live()--; }
    };
}

#define CHECK(condition)                                                                   \
    do {                                                                                   \
        TestUtil::checks()++;                                                              \
        if (!(condition)) {                                                                \
            TestUtil::failures()++;                                                        \
            std::cout << "FAILED " << __FILE__ << ":" << __LINE__ << ": " #condition "\n"; \
        }                                                                                  \
    } while (false)
