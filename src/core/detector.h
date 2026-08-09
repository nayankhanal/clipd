#pragma once

#include <string>
#include "core/content_type.h"

namespace clipd {

// Pure classification: given raw clipboard text, guess what it is.
// No I/O, no OS dependency — safe to unit test anywhere.
ContentType detect_content_type(const std::string& text);

} // namespace clipd
