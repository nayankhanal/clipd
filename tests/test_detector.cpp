#include "test_framework.h"
#include "core/detector.h"

using clipd::ContentType;
using clipd::detect_content_type;

CLIPD_TEST(detects_json_object) {
    CLIPD_CHECK(detect_content_type(R"({"a":1,"b":[1,2,3]})") == ContentType::Json);
}

CLIPD_TEST(detects_json_array) {
    CLIPD_CHECK(detect_content_type("[1, 2, 3]") == ContentType::Json);
}

CLIPD_TEST(detects_python_traceback) {
    std::string trace =
        "Traceback (most recent call last):\n"
        "  File \"a.py\", line 1, in <module>\n"
        "ValueError: bad value";
    CLIPD_CHECK(detect_content_type(trace) == ContentType::StackTrace);
}

CLIPD_TEST(detects_java_stack_trace) {
    std::string trace =
        "Exception in thread \"main\" java.lang.RuntimeException\n"
        "\tat com.example.Main.main(Main.java:10)";
    CLIPD_CHECK(detect_content_type(trace) == ContentType::StackTrace);
}

CLIPD_TEST(detects_code) {
    std::string code =
        "function add(a, b) {\n"
        "  return a + b;\n"
        "}";
    CLIPD_CHECK(detect_content_type(code) == ContentType::Code);
}

CLIPD_TEST(falls_back_to_plain) {
    CLIPD_CHECK(detect_content_type("just some regular text") == ContentType::Plain);
    CLIPD_CHECK(detect_content_type("") == ContentType::Plain);
}
