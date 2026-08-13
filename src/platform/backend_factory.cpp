#include "platform/clipboard_backend.h"

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>

namespace clipd {

namespace {

// Returns lowercased $XDG_SESSION_TYPE, or "" if unset.
std::string session_type() {
    const char* value = std::getenv("XDG_SESSION_TYPE");
    if (!value) return "";
    std::string result(value);
    for (char& c : result) c = static_cast<char>(std::tolower(c));
    return result;
}

bool wayland_available() {
    // WAYLAND_DISPLAY is set inside a Wayland session; XDG_SESSION_TYPE
    // is a secondary hint (some setups leave WAYLAND_DISPLAY unset in
    // certain contexts).
    if (std::getenv("WAYLAND_DISPLAY")) return true;
    return session_type() == "wayland";
}

bool x11_available() {
    if (std::getenv("DISPLAY")) return true;
    return session_type() == "x11";
}

} // namespace

std::unique_ptr<ClipboardBackend> make_platform_backend() {
    // Prefer Wayland when we're clearly in a Wayland session, else X11.
    // Note: XWayland sets DISPLAY too, so check Wayland first.
    if (wayland_available()) {
        return make_wayland_backend();
    }
    if (x11_available()) {
        return make_x11_backend();
    }
    throw std::runtime_error(
        "clipd: no supported display session found "
        "(need a Wayland or X11 session; neither WAYLAND_DISPLAY nor DISPLAY is set)");
}

} // namespace clipd
