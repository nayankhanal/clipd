#include "core/cleaners/json_formatter.h"

#include <sstream>
#include <string>

namespace clipd::cleaners {

namespace {

void write_indent(std::ostringstream& out, int depth, int indent_width) {
    out << '\n' << std::string(static_cast<size_t>(depth) * indent_width, ' ');
}

} // namespace

std::string pretty_print_json(const std::string& text, int indent_width) {
    std::ostringstream out;
    int depth = 0;
    bool in_string = false;
    bool escaped = false;
    char last_char = '\0';

    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];

        if (in_string) {
            out << c;
            last_char = c;
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                in_string = false;
            }
            continue;
        }

        switch (c) {
            case '"':
                in_string = true;
                out << c;
                last_char = c;
                break;
            case '{':
            case '[':
                out << c;
                last_char = c;
                // Peek ahead: collapse empty {} / [] onto one line.
                if (i + 1 < text.size()) {
                    char next = text[i + 1];
                    if (next == '}' || next == ']') {
                        break;
                    }
                }
                depth++;
                write_indent(out, depth, indent_width);
                break;
            case '}':
            case ']':
                if (last_char == '{' || last_char == '[') {
                    out << c;
                    last_char = c;
                    break;
                }
                depth--;
                write_indent(out, depth, indent_width);
                out << c;
                last_char = c;
                break;
            case ',':
                out << c;
                last_char = c;
                write_indent(out, depth, indent_width);
                break;
            case ':':
                out << c << ' ';
                last_char = ' ';
                break;
            default:
                if (!std::isspace(static_cast<unsigned char>(c))) {
                    out << c;
                    last_char = c;
                }
                break;
        }
    }

    return out.str();
}

} // namespace clipd::cleaners
