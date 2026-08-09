#pragma once

#include <functional>
#include <memory>
#include <string>

namespace clipd {

// OS-specific clipboard access. v1 implements this for Linux/X11 only
// (see linux_x11.cpp). Wayland and macOS backends are future work.
class ClipboardBackend {
public:
    virtual ~ClipboardBackend() = default;

    // Blocks, watching for clipboard-owner changes, and invokes `on_change`
    // with the new clipboard text each time it changes. Does not return
    // until the process is signaled to stop.
    virtual void watch(const std::function<void(const std::string&)>& on_change) = 0;

    // Replaces the clipboard content and becomes the new selection owner.
    virtual void set_content(const std::string& text) = 0;
};

// Implemented per-OS (see linux_x11.cpp for v1).
std::unique_ptr<ClipboardBackend> make_platform_backend();

} // namespace clipd
