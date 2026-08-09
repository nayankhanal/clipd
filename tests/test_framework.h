#pragma once

// Minimal dependency-free test harness. Not meant to grow into a full
// framework — swap for Catch2/GoogleTest later if tests outgrow this.

#include <iostream>
#include <string>
#include <vector>

namespace clipd::test {

struct Registry {
    inline static std::vector<std::pair<std::string, void (*)()>> tests;
    inline static int failures = 0;
};

struct Registrar {
    Registrar(const std::string& name, void (*fn)()) {
        Registry::tests.emplace_back(name, fn);
    }
};

inline void check(bool condition, const char* expr, const char* file, int line) {
    if (!condition) {
        std::cerr << "FAIL: " << expr << " at " << file << ":" << line << "\n";
        Registry::failures++;
    }
}

inline int run_all() {
    for (auto& [name, fn] : Registry::tests) {
        std::cout << "-- " << name << "\n";
        fn();
    }
    if (Registry::failures == 0) {
        std::cout << "All tests passed.\n";
    } else {
        std::cout << Registry::failures << " assertion(s) failed.\n";
    }
    return Registry::failures == 0 ? 0 : 1;
}

} // namespace clipd::test

#define CLIPD_TEST(name) \
    void name(); \
    static clipd::test::Registrar registrar_##name(#name, &name); \
    void name()

#define CLIPD_CHECK(expr) clipd::test::check((expr), #expr, __FILE__, __LINE__)
#define CLIPD_CHECK_EQ(a, b) clipd::test::check((a) == (b), #a " == " #b, __FILE__, __LINE__)
