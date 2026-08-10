#pragma once

#include <string>

namespace clipd {

struct Config {
    bool detect_json = true;
    bool detect_stack_trace = true;
    bool detect_code = true;

    bool fix_indentation = true;
    int spaces_per_tab = 4;
};

// Parses a flat TOML-like config: [section] headers plus key = value
// lines. No nested tables/arrays. Unknown keys/sections and malformed
// lines are silently ignored rather than rejected, so a partially
// wrong config still loads with sane defaults for the rest.
Config parse_config(const std::string& text);

// Reads `path`; returns a default-constructed Config if the file is
// missing or unreadable (config is optional, not required to run).
Config load_config(const std::string& path);

// ~/.config/clipd/config.toml
std::string default_config_path();

} // namespace clipd
