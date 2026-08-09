#include "core/content_type.h"

namespace clipd {

const char* to_string(ContentType type) {
    switch (type) {
        case ContentType::Json:       return "json";
        case ContentType::StackTrace: return "stack_trace";
        case ContentType::Code:       return "code";
        case ContentType::Plain:      return "plain";
    }
    return "plain";
}

} // namespace clipd
