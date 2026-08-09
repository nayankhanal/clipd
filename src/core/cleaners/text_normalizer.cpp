#include "core/cleaners/text_normalizer.h"

#include <sstream>
#include <string>
#include <unordered_map>

namespace clipd::cleaners {

namespace {

// UTF-8 byte sequences for characters that silently break code when pasted.
const std::unordered_map<std::string, std::string>& replacement_map() {
    static const std::unordered_map<std::string, std::string> map = {
        {"\xE2\x80\x9C", "\""}, // “ left double quote
        {"\xE2\x80\x9D", "\""}, // ” right double quote
        {"\xE2\x80\x98", "'"},  // ‘ left single quote
        {"\xE2\x80\x99", "'"},  // ’ right single quote
        {"\xC2\xA0", " "},      // non-breaking space
        {"\xE2\x80\x8B", ""},   // zero-width space
        {"\xE2\x80\x8C", ""},   // zero-width non-joiner
        {"\xE2\x80\x8D", ""},   // zero-width joiner
        {"\xEF\xBB\xBF", ""},   // BOM
    };
    return map;
}

std::string replace_known_sequences(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    size_t i = 0;
    const auto& map = replacement_map();
    while (i < text.size()) {
        bool matched = false;
        for (const auto& [from, to] : map) {
            if (text.compare(i, from.size(), from) == 0) {
                out += to;
                i += from.size();
                matched = true;
                break;
            }
        }
        if (!matched) {
            out += text[i];
            i++;
        }
    }
    return out;
}

std::string rstrip(const std::string& line) {
    size_t end = line.find_last_not_of(" \t");
    return end == std::string::npos ? "" : line.substr(0, end + 1);
}

std::string strip_trailing_whitespace_per_line(const std::string& text) {
    std::istringstream stream(text);
    std::ostringstream out;
    std::string line;
    bool first = true;
    while (std::getline(stream, line)) {
        if (!first) out << "\n";
        first = false;
        out << rstrip(line);
    }
    return out.str();
}

} // namespace

std::string normalize_text(const std::string& text) {
    std::string cleaned = replace_known_sequences(text);
    cleaned = strip_trailing_whitespace_per_line(cleaned);
    return cleaned;
}

} // namespace clipd::cleaners
