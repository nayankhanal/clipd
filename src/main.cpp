#include <iostream>

#include "core/content_type.h"
#include "core/detector.h"
#include "core/cleaners/indent_fixer.h"
#include "core/cleaners/json_formatter.h"
#include "core/cleaners/text_normalizer.h"
#include "platform/clipboard_backend.h"

namespace {

std::string clean(const std::string& text, clipd::ContentType type) {
    using namespace clipd::cleaners;
    switch (type) {
        case clipd::ContentType::Json:
            return pretty_print_json(normalize_text(text));
        case clipd::ContentType::Code:
        case clipd::ContentType::StackTrace:
            return fix_indentation(normalize_text(text));
        case clipd::ContentType::Plain:
            return normalize_text(text);
    }
    return text;
}

} // namespace

int main() {
    std::cout << "clipd starting up" << std::endl;

    auto backend = clipd::make_platform_backend();

    backend->watch([&](const std::string& raw) {
        clipd::ContentType type = clipd::detect_content_type(raw);
        std::string cleaned = clean(raw, type);

        if (cleaned == raw) {
            std::cout << "clipd: [" << clipd::to_string(type) << "] no change needed" << std::endl;
            return;
        }

        std::cout << "clipd: [" << clipd::to_string(type) << "] cleaned "
                   << raw.size() << " -> " << cleaned.size() << " bytes" << std::endl;
        backend->set_content(cleaned);
    });

    return 0;
}
