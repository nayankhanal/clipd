#include "core/detector.h"

#include <algorithm>
#include <cctype>

namespace clipd {

namespace {

std::string trimmed(const std::string& text) {
    size_t start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

bool looks_like_json(const std::string& text) {
    if (text.empty()) return false;
    char first = text.front();
    char last = text.back();
    if (!((first == '{' && last == '}') || (first == '[' && last == ']'))) {
        return false;
    }
    // Cheap structural check: braces/brackets roughly balanced.
    int depth = 0;
    for (char c : text) {
        if (c == '{' || c == '[') depth++;
        else if (c == '}' || c == ']') depth--;
        if (depth < 0) return false;
    }
    return depth == 0;
}

bool looks_like_stack_trace(const std::string& text) {
    static const char* markers[] = {
        "Traceback (most recent call last)",
        "\tat ",
        "\n\tat ",
        "Exception in thread",
        "Caused by:",
        "panic:",
        "goroutine ",
    };
    for (const char* marker : markers) {
        if (text.find(marker) != std::string::npos) return true;
    }
    return false;
}

bool looks_like_code(const std::string& text) {
    // Weak heuristic: multi-line, and contains common code punctuation density.
    if (text.find('\n') == std::string::npos) return false;
    int hits = 0;
    for (const char* tok : {"{", "}", ";", "=>", "function ", "def ", "class ", "const ", "let ", "var ", "#include", "import "}) {
        if (text.find(tok) != std::string::npos) hits++;
    }
    return hits >= 2;
}

} // namespace

ContentType detect_content_type(const std::string& raw) {
    std::string text = trimmed(raw);
    if (text.empty()) return ContentType::Plain;

    if (looks_like_json(text)) return ContentType::Json;
    if (looks_like_stack_trace(text)) return ContentType::StackTrace;
    if (looks_like_code(text)) return ContentType::Code;
    return ContentType::Plain;
}

} // namespace clipd
