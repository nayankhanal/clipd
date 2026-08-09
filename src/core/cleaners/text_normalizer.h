#pragma once

#include <string>

namespace clipd::cleaners {

// Fixes smart quotes -> straight quotes, strips invisible/zero-width
// characters, and trims trailing whitespace from each line.
std::string normalize_text(const std::string& text);

} // namespace clipd::cleaners
