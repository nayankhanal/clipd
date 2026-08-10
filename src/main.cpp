#include <atomic>
#include <iostream>

#include "core/config.h"
#include "core/content_type.h"
#include "core/detector.h"
#include "core/cleaners/indent_fixer.h"
#include "core/cleaners/json_formatter.h"
#include "core/cleaners/text_normalizer.h"
#include "ipc/socket_server.h"
#include "platform/clipboard_backend.h"

namespace {

clipd::ContentType apply_detector_toggles(clipd::ContentType type, const clipd::Config& config) {
    switch (type) {
        case clipd::ContentType::Json:
            return config.detect_json ? type : clipd::ContentType::Plain;
        case clipd::ContentType::StackTrace:
            return config.detect_stack_trace ? type : clipd::ContentType::Plain;
        case clipd::ContentType::Code:
            return config.detect_code ? type : clipd::ContentType::Plain;
        case clipd::ContentType::Plain:
            return type;
    }
    return type;
}

std::string clean(const std::string& text, clipd::ContentType type, const clipd::Config& config) {
    using namespace clipd::cleaners;
    switch (type) {
        case clipd::ContentType::Json:
            return pretty_print_json(normalize_text(text));
        case clipd::ContentType::Code:
        case clipd::ContentType::StackTrace: {
            std::string result = normalize_text(text);
            if (config.fix_indentation) result = fix_indentation(result, config.spaces_per_tab);
            return result;
        }
        case clipd::ContentType::Plain:
            return normalize_text(text);
    }
    return text;
}

} // namespace

int main() {
    std::cout << "clipd starting up" << std::endl;

    clipd::Config config = clipd::load_config(clipd::default_config_path());

    std::atomic<bool> paused{false};
    clipd::ControlServer control(paused);
    control.start();

    auto backend = clipd::make_platform_backend();

    backend->watch([&](const std::string& raw) {
        if (paused) {
            std::cout << "clipd: paused, skipping" << std::endl;
            return;
        }

        clipd::ContentType type = apply_detector_toggles(clipd::detect_content_type(raw), config);
        std::string cleaned = clean(raw, type, config);

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
