#include "platform/clipboard_backend.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xfixes.h>

#include <cstring>
#include <memory>
#include <stdexcept>

namespace clipd {

namespace {

// Reads the CLIPBOARD selection's current content by asking the current
// owner to convert it to UTF8_STRING, then reading the resulting property
// off our own window. Returns empty string if the conversion fails/times out.
std::string read_clipboard_text(Display* display, Window window, Atom clipboard,
                                 Atom utf8_string, Atom xsel_data) {
    XConvertSelection(display, clipboard, utf8_string, xsel_data, window, CurrentTime);
    XFlush(display);

    // Wait (bounded) for the SelectionNotify reply.
    for (int i = 0; i < 1000; ++i) {
        if (XPending(display) == 0) {
            struct timespec ts{0, 1'000'000}; // 1ms
            nanosleep(&ts, nullptr);
            continue;
        }
        XEvent event;
        XNextEvent(display, &event);
        if (event.type == SelectionNotify) {
            if (event.xselection.property == None) {
                return "";
            }
            Atom actual_type;
            int actual_format;
            unsigned long item_count, bytes_after;
            unsigned char* data = nullptr;
            XGetWindowProperty(display, window, xsel_data, 0, ~0L, False, AnyPropertyType,
                                &actual_type, &actual_format, &item_count, &bytes_after, &data);
            std::string result;
            if (data) {
                result.assign(reinterpret_cast<char*>(data), item_count);
                XFree(data);
            }
            XDeleteProperty(display, window, xsel_data);
            return result;
        }
    }
    return "";
}

} // namespace

class LinuxX11Backend : public ClipboardBackend {
public:
    LinuxX11Backend() {
        display_ = XOpenDisplay(nullptr);
        if (!display_) {
            throw std::runtime_error("clipd: could not open X display (is X11 running?)");
        }

        int fixes_event_base, fixes_error_base;
        if (!XFixesQueryExtension(display_, &fixes_event_base, &fixes_error_base)) {
            throw std::runtime_error("clipd: XFixes extension not available");
        }
        fixes_event_base_ = fixes_event_base;

        window_ = XCreateSimpleWindow(display_, DefaultRootWindow(display_), 0, 0, 1, 1, 0, 0, 0);

        clipboard_atom_ = XInternAtom(display_, "CLIPBOARD", False);
        utf8_string_atom_ = XInternAtom(display_, "UTF8_STRING", False);
        xsel_data_atom_ = XInternAtom(display_, "CLIPD_SELECTION", False);
        targets_atom_ = XInternAtom(display_, "TARGETS", False);

        XFixesSelectSelectionInput(display_, DefaultRootWindow(display_), clipboard_atom_,
                                    XFixesSetSelectionOwnerNotifyMask);
    }

    ~LinuxX11Backend() override {
        if (display_) {
            XDestroyWindow(display_, window_);
            XCloseDisplay(display_);
        }
    }

    void watch(const std::function<void(const std::string&)>& on_change) override {
        while (true) {
            XEvent event;
            XNextEvent(display_, &event);

            if (event.type == fixes_event_base_ + XFixesSelectionNotify) {
                auto* sel_event = reinterpret_cast<XFixesSelectionNotifyEvent*>(&event);
                if (sel_event->selection != clipboard_atom_) continue;

                // Skip changes we caused ourselves (we became the owner via set_content).
                if (sel_event->owner == window_) continue;

                std::string text = read_clipboard_text(display_, window_, clipboard_atom_,
                                                         utf8_string_atom_, xsel_data_atom_);
                if (!text.empty()) {
                    on_change(text);
                }
            } else if (event.type == SelectionRequest) {
                handle_selection_request(event.xselectionrequest);
            }
        }
    }

    void set_content(const std::string& text) override {
        pending_content_ = text;
        XSetSelectionOwner(display_, clipboard_atom_, window_, CurrentTime);
        XFlush(display_);
    }

private:
    void handle_selection_request(const XSelectionRequestEvent& request) {
        XSelectionEvent response{};
        response.type = SelectionNotify;
        response.display = request.display;
        response.requestor = request.requestor;
        response.selection = request.selection;
        response.time = request.time;
        response.target = request.target;
        response.property = None;

        if (request.target == targets_atom_) {
            Atom supported[] = {utf8_string_atom_, XA_STRING, targets_atom_};
            XChangeProperty(display_, request.requestor, request.property, XA_ATOM, 32,
                             PropModeReplace, reinterpret_cast<unsigned char*>(supported), 3);
            response.property = request.property;
        } else if (request.target == utf8_string_atom_ || request.target == XA_STRING) {
            XChangeProperty(display_, request.requestor, request.property, request.target, 8,
                             PropModeReplace,
                             reinterpret_cast<const unsigned char*>(pending_content_.data()),
                             static_cast<int>(pending_content_.size()));
            response.property = request.property;
        }

        XSendEvent(display_, request.requestor, False, NoEventMask,
                   reinterpret_cast<XEvent*>(&response));
        XFlush(display_);
    }

    Display* display_ = nullptr;
    Window window_ = 0;
    int fixes_event_base_ = 0;
    Atom clipboard_atom_ = 0;
    Atom utf8_string_atom_ = 0;
    Atom xsel_data_atom_ = 0;
    Atom targets_atom_ = 0;
    std::string pending_content_;
};

std::unique_ptr<ClipboardBackend> make_platform_backend() {
    return std::make_unique<LinuxX11Backend>();
}

} // namespace clipd
