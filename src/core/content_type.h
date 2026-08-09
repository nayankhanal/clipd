#pragma once

namespace clipd {

enum class ContentType {
    Plain,
    Json,
    StackTrace,
    Code,
};

const char* to_string(ContentType type);

} // namespace clipd
