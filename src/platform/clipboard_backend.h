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

    // Records `original` as the value to restore if undo is requested.
    // Called on the watch thread right before a clean overwrites the
    // clipboard. Only one level of history is kept.
    virtual void save_undo(const std::string& original) { (void)original; }

    // Restores the last saved pre-clean value to the clipboard. May be
    // called from another thread; the implementation is responsible for
    // performing the actual write safely. Returns true if something was
    // restored, false if there was nothing to undo.
    virtual bool undo_last() { return false; }
};

// Chooses and constructs the right backend for the current session
// (Wayland vs X11). Throws std::runtime_error if no backend is usable.
std::unique_ptr<ClipboardBackend> make_platform_backend();

// Per-backend constructors, implemented in their platform files.
std::unique_ptr<ClipboardBackend> make_x11_backend();
std::unique_ptr<ClipboardBackend> make_wayland_backend();

} // namespace clipd
