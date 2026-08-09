#pragma once

#include <string>

namespace clipd::cleaners {

// Pretty-prints JSON text with consistent indentation.
// Not a validating parser: it re-indents based on bracket/string
// tracking, so malformed JSON is passed through best-effort rather
// than rejected. Good enough for "paste ugly JSON, get readable JSON".
std::string pretty_print_json(const std::string& text, int indent_width = 2);

} // namespace clipd::cleaners
