#include "test_framework.h"
#include "core/cleaners/indent_fixer.h"
#include "core/cleaners/json_formatter.h"
#include "core/cleaners/text_normalizer.h"

using namespace clipd::cleaners;

CLIPD_TEST(normalizes_smart_quotes) {
    std::string input = "\xE2\x80\x9Chello\xE2\x80\x9D";
    CLIPD_CHECK_EQ(normalize_text(input), "\"hello\"");
}

CLIPD_TEST(strips_zero_width_space) {
    std::string input = "hello\xE2\x80\x8Bworld";
    CLIPD_CHECK_EQ(normalize_text(input), "helloworld");
}

CLIPD_TEST(strips_trailing_whitespace_per_line) {
    std::string input = "line one   \nline two\t\n";
    std::string result = normalize_text(input);
    CLIPD_CHECK(result.find("one   ") == std::string::npos);
    CLIPD_CHECK(result.find("two\t") == std::string::npos);
}

CLIPD_TEST(converts_leading_tabs_to_spaces) {
    std::string input = "\tfoo();";
    CLIPD_CHECK_EQ(fix_indentation(input, 4), "    foo();");
}

CLIPD_TEST(pretty_prints_minified_json) {
    std::string input = R"({"a":1,"b":[1,2]})";
    std::string result = pretty_print_json(input);
    CLIPD_CHECK(result.find('\n') != std::string::npos);
    CLIPD_CHECK(result.find("\"a\": 1") != std::string::npos);
}

CLIPD_TEST(pretty_print_json_collapses_empty_containers) {
    std::string input = R"({"a":{},"b":[]})";
    std::string result = pretty_print_json(input);
    CLIPD_CHECK(result.find("{}") != std::string::npos);
    CLIPD_CHECK(result.find("[]") != std::string::npos);
}
