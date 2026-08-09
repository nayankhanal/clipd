#include "core/cleaners/indent_fixer.h"

#include <sstream>
#include <string>

namespace clipd::cleaners {

namespace {

std::string fix_line_leading_tabs(const std::string& line, int spaces_per_tab) {
    size_t i = 0;
    std::string leading;
    while (i < line.size() && (line[i] == '\t' || line[i] == ' ')) {
        if (line[i] == '\t') {
            leading += std::string(spaces_per_tab, ' ');
        } else {
            leading += ' ';
        }
        i++;
    }
    return leading + line.substr(i);
}

} // namespace

std::string fix_indentation(const std::string& text, int spaces_per_tab) {
    std::istringstream stream(text);
    std::ostringstream out;
    std::string line;
    bool first = true;
    while (std::getline(stream, line)) {
        if (!first) out << "\n";
        first = false;
        out << fix_line_leading_tabs(line, spaces_per_tab);
    }
    return out.str();
}

} // namespace clipd::cleaners
