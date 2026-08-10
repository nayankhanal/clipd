#include "core/config.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace clipd {

namespace {

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

bool parse_bool(const std::string& value, bool fallback) {
    std::string v = value;
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return std::tolower(c); });
    if (v == "true") return true;
    if (v == "false") return false;
    return fallback;
}

} // namespace

Config parse_config(const std::string& text) {
    Config config;
    std::istringstream stream(text);
    std::string line;
    std::string section;

    while (std::getline(stream, line)) {
        std::string trimmed_line = trim(line);
        if (trimmed_line.empty() || trimmed_line[0] == '#') continue;

        if (trimmed_line.front() == '[' && trimmed_line.back() == ']') {
            section = trimmed_line.substr(1, trimmed_line.size() - 2);
            continue;
        }

        size_t eq = trimmed_line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(trimmed_line.substr(0, eq));
        std::string value = trim(trimmed_line.substr(eq + 1));

        if (section == "detectors") {
            if (key == "json") config.detect_json = parse_bool(value, config.detect_json);
            else if (key == "stack_trace") config.detect_stack_trace = parse_bool(value, config.detect_stack_trace);
            else if (key == "code") config.detect_code = parse_bool(value, config.detect_code);
        } else if (section == "cleaners") {
            if (key == "fix_indentation") {
                config.fix_indentation = parse_bool(value, config.fix_indentation);
            } else if (key == "spaces_per_tab") {
                try {
                    config.spaces_per_tab = std::stoi(value);
                } catch (...) {
                    // leave default on malformed value
                }
            }
        }
    }

    return config;
}

Config load_config(const std::string& path) {
    std::ifstream file(path);
    if (!file) return Config{};
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return parse_config(buffer.str());
}

std::string default_config_path() {
    if (const char* home = std::getenv("HOME")) {
        return std::string(home) + "/.config/clipd/config.toml";
    }
    return "config.toml";
}

} // namespace clipd
