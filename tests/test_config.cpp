#include "test_framework.h"
#include "core/config.h"

using clipd::Config;
using clipd::parse_config;

CLIPD_TEST(defaults_when_empty) {
    Config config = parse_config("");
    CLIPD_CHECK_EQ(config.detect_json, true);
    CLIPD_CHECK_EQ(config.detect_stack_trace, true);
    CLIPD_CHECK_EQ(config.detect_code, true);
    CLIPD_CHECK_EQ(config.fix_indentation, true);
    CLIPD_CHECK_EQ(config.spaces_per_tab, 4);
}

CLIPD_TEST(parses_detector_toggles) {
    std::string text =
        "[detectors]\n"
        "json = true\n"
        "stack_trace = false\n"
        "code = false\n";
    Config config = parse_config(text);
    CLIPD_CHECK_EQ(config.detect_json, true);
    CLIPD_CHECK_EQ(config.detect_stack_trace, false);
    CLIPD_CHECK_EQ(config.detect_code, false);
}

CLIPD_TEST(parses_cleaner_settings) {
    std::string text =
        "[cleaners]\n"
        "fix_indentation = false\n"
        "spaces_per_tab = 2\n";
    Config config = parse_config(text);
    CLIPD_CHECK_EQ(config.fix_indentation, false);
    CLIPD_CHECK_EQ(config.spaces_per_tab, 2);
}

CLIPD_TEST(ignores_unknown_keys_and_comments) {
    std::string text =
        "# a comment\n"
        "[detectors]\n"
        "made_up_key = true\n"
        "json = false\n"
        "\n"
        "[unknown_section]\n"
        "code = false\n"; // should NOT affect [detectors].code since it's in the wrong section
    Config config = parse_config(text);
    CLIPD_CHECK_EQ(config.detect_json, false);
    CLIPD_CHECK_EQ(config.detect_code, true);
}

CLIPD_TEST(malformed_spaces_per_tab_keeps_default) {
    std::string text =
        "[cleaners]\n"
        "spaces_per_tab = not_a_number\n";
    Config config = parse_config(text);
    CLIPD_CHECK_EQ(config.spaces_per_tab, 4);
}
