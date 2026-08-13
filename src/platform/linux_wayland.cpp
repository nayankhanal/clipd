#include "platform/clipboard_backend.h"

#include <stdexcept>

namespace clipd {

// Placeholder until the wlr-data-control backend lands. Selecting Wayland
// today fails loudly with a clear message rather than silently doing
// nothing, so users on Wayland know why clipd isn't cleaning yet.
std::unique_ptr<ClipboardBackend> make_wayland_backend() {
    throw std::runtime_error(
        "clipd: Wayland backend not implemented yet. "
        "Run under an X11 session for now (see XDG_SESSION_TYPE), or track "
        "Wayland support on the project's issue tracker.");
}

} // namespace clipd
