#pragma once

#include <string>

namespace clipd::cleaners {

// Converts leading tabs to a consistent number of spaces per line.
// Does not attempt to re-derive logical indent levels — just makes
// mixed tabs/spaces render consistently.
std::string fix_indentation(const std::string& text, int spaces_per_tab = 4);

} // namespace clipd::cleaners
