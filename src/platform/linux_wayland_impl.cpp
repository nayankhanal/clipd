#include "platform/clipboard_backend.h"

#include <wayland-client.h>
#include "wlr-data-control-unstable-v1-client-protocol.h"

#include <condition_variable>
#include <cstring>
#include <fcntl.h>
#include <map>
#include <memory>
#include <mutex>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

// NOTE: This backend targets the wlr-data-control protocol, which is
// implemented by wlroots-based compositors (Sway, Hyprland, river, ...).
// It is NOT supported by GNOME/Mutter or KDE/KWin. It has been written
// against the protocol spec but, unlike the X11 backend, has not yet
// been verified live on real hardware — test on a wlroots compositor.

namespace clipd {

namespace {

// Preference order for text MIME types when reading a selection.
const char* kTextMimes[] = {
    "text/plain;charset=utf-8",
    "text/plain",
    "UTF8_STRING",
    "STRING",
};

std::string read_all(int fd) {
    std::string out;
    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        out.append(buf, static_cast<size_t>(n));
    }
    return out;
}

} // namespace

class WaylandBackend : public ClipboardBackend {
public:
    WaylandBackend() {
        display_ = wl_display_connect(nullptr);
        if (!display_) {
            throw std::runtime_error("clipd: could not connect to Wayland display");
        }

        registry_ = wl_display_get_registry(display_);
        wl_registry_add_listener(registry_, &registry_listener_, this);
        wl_display_roundtrip(display_); // populate globals

        if (!manager_ || !seat_) {
            throw std::runtime_error(
                "clipd: compositor does not support wlr-data-control "
                "(needed for clipboard access; supported on wlroots "
                "compositors like Sway/Hyprland, not GNOME/KDE)");
        }

        device_ = zwlr_data_control_manager_v1_get_data_device(manager_, seat_);
        zwlr_data_control_device_v1_add_listener(device_, &device_listener_, this);
        wl_display_roundtrip(display_); // deliver the initial selection event

        int pipe_fds[2];
        if (pipe(pipe_fds) != 0) {
            throw std::runtime_error("clipd: failed to create wake pipe");
        }
        wake_read_fd_ = pipe_fds[0];
        wake_write_fd_ = pipe_fds[1];
        fcntl(wake_read_fd_, F_SETFL, O_NONBLOCK);
        fcntl(wake_write_fd_, F_SETFL, O_NONBLOCK);
    }

    ~WaylandBackend() override {
        if (wake_read_fd_ >= 0) close(wake_read_fd_);
        if (wake_write_fd_ >= 0) close(wake_write_fd_);
        if (source_) zwlr_data_control_source_v1_destroy(source_);
        if (device_) zwlr_data_control_device_v1_destroy(device_);
        if (manager_) zwlr_data_control_manager_v1_destroy(manager_);
        if (registry_) wl_registry_destroy(registry_);
        if (display_) wl_display_disconnect(display_);
    }

    void watch(const std::function<void(const std::string&)>& on_change) override {
        on_change_ = on_change;
        int wl_fd = wl_display_get_fd(display_);

        while (true) {
            // libwayland's thread-safe read preparation.
            while (wl_display_prepare_read(display_) != 0) {
                wl_display_dispatch_pending(display_);
            }
            wl_display_flush(display_);

            struct pollfd fds[2];
            fds[0].fd = wl_fd;
            fds[0].events = POLLIN;
            fds[1].fd = wake_read_fd_;
            fds[1].events = POLLIN;

            if (poll(fds, 2, -1) < 0) {
                wl_display_cancel_read(display_);
                continue;
            }

            if (fds[0].revents & POLLIN) {
                wl_display_read_events(display_);
            } else {
                wl_display_cancel_read(display_);
            }
            wl_display_dispatch_pending(display_);

            if (fds[1].revents & POLLIN) {
                drain_wake_pipe();
                process_undo_request();
            }
        }
    }

    void set_content(const std::string& text) override {
        current_content_ = text;

        if (source_) {
            zwlr_data_control_source_v1_destroy(source_);
            source_ = nullptr;
        }
        source_ = zwlr_data_control_manager_v1_create_data_source(manager_);
        zwlr_data_control_source_v1_add_listener(source_, &source_listener_, this);
        for (const char* mime : kTextMimes) {
            zwlr_data_control_source_v1_offer(source_, mime);
        }
        zwlr_data_control_device_v1_set_selection(device_, source_);
        self_owned_ = true;
        wl_display_flush(display_);
    }

    void save_undo(const std::string& original) override {
        undo_buffer_ = original;
        has_undo_ = true;
    }

    bool undo_last() override {
        std::unique_lock<std::mutex> lock(req_mutex_);
        undo_requested_ = true;
        undo_done_ = false;

        char signal = 1;
        ssize_t w = write(wake_write_fd_, &signal, 1);
        (void)w;

        req_cv_.wait(lock, [this] { return undo_done_; });
        return undo_restored_;
    }

    // ---- registry ----
    void on_global(wl_registry* registry, uint32_t name, const char* interface, uint32_t version) {
        if (std::strcmp(interface, zwlr_data_control_manager_v1_interface.name) == 0) {
            uint32_t bind_version = version < 2 ? version : 2;
            manager_ = static_cast<zwlr_data_control_manager_v1*>(
                wl_registry_bind(registry, name, &zwlr_data_control_manager_v1_interface, bind_version));
        } else if (std::strcmp(interface, wl_seat_interface.name) == 0) {
            seat_ = static_cast<wl_seat*>(
                wl_registry_bind(registry, name, &wl_seat_interface, 1));
        }
    }

    // ---- data offer ----
    void on_offer_mime(zwlr_data_control_offer_v1* offer, const char* mime_type) {
        offer_mimes_[offer].emplace_back(mime_type);
    }

    void on_data_offer(zwlr_data_control_offer_v1* offer) {
        zwlr_data_control_offer_v1_add_listener(offer, &offer_listener_, this);
        offer_mimes_[offer]; // ensure an entry exists
    }

    void on_selection(zwlr_data_control_offer_v1* offer) {
        // An echo of our own selection: ignore until a foreign client takes
        // over (signalled by our source's `cancelled` event).
        if (self_owned_) {
            cleanup_offer(offer);
            return;
        }
        if (!offer) return;

        std::string mime = pick_text_mime(offer);
        if (!mime.empty()) {
            std::string text = receive_offer(offer, mime);
            if (!text.empty() && on_change_) {
                on_change_(text);
            }
        }
        cleanup_offer(offer);
    }

    // ---- source ----
    void on_source_send(const char* /*mime_type*/, int fd) {
        // Serve any requested text MIME with the same content.
        const std::string& data = current_content_;
        size_t written = 0;
        while (written < data.size()) {
            ssize_t n = write(fd, data.data() + written, data.size() - written);
            if (n <= 0) break;
            written += static_cast<size_t>(n);
        }
        close(fd);
    }

    void on_source_cancelled(zwlr_data_control_source_v1* source) {
        // A different client now owns the selection.
        self_owned_ = false;
        if (source == source_) {
            zwlr_data_control_source_v1_destroy(source_);
            source_ = nullptr;
        } else {
            zwlr_data_control_source_v1_destroy(source);
        }
    }

private:
    std::string pick_text_mime(zwlr_data_control_offer_v1* offer) {
        auto it = offer_mimes_.find(offer);
        if (it == offer_mimes_.end()) return "";
        for (const char* preferred : kTextMimes) {
            for (const std::string& available : it->second) {
                if (available == preferred) return available;
            }
        }
        return "";
    }

    std::string receive_offer(zwlr_data_control_offer_v1* offer, const std::string& mime) {
        int pipe_fds[2];
        if (pipe(pipe_fds) != 0) return "";
        zwlr_data_control_offer_v1_receive(offer, mime.c_str(), pipe_fds[1]);
        wl_display_flush(display_);
        close(pipe_fds[1]);
        std::string text = read_all(pipe_fds[0]);
        close(pipe_fds[0]);
        return text;
    }

    void cleanup_offer(zwlr_data_control_offer_v1* offer) {
        if (!offer) return;
        offer_mimes_.erase(offer);
        zwlr_data_control_offer_v1_destroy(offer);
    }

    void drain_wake_pipe() {
        char buffer[64];
        while (read(wake_read_fd_, buffer, sizeof(buffer)) > 0) {
        }
    }

    void process_undo_request() {
        std::unique_lock<std::mutex> lock(req_mutex_);
        if (!undo_requested_) return;
        undo_requested_ = false;

        bool restored = false;
        if (has_undo_) {
            set_content(undo_buffer_);
            has_undo_ = false;
            restored = true;
        }
        undo_restored_ = restored;
        undo_done_ = true;
        lock.unlock();
        req_cv_.notify_all();
    }

    // ---- static listener trampolines ----
    static void registry_global(void* data, wl_registry* r, uint32_t name,
                                 const char* interface, uint32_t version) {
        static_cast<WaylandBackend*>(data)->on_global(r, name, interface, version);
    }
    static void registry_global_remove(void*, wl_registry*, uint32_t) {}

    static void offer_offer(void* data, zwlr_data_control_offer_v1* offer, const char* mime) {
        static_cast<WaylandBackend*>(data)->on_offer_mime(offer, mime);
    }

    static void device_data_offer(void* data, zwlr_data_control_device_v1*,
                                    zwlr_data_control_offer_v1* offer) {
        static_cast<WaylandBackend*>(data)->on_data_offer(offer);
    }
    static void device_selection(void* data, zwlr_data_control_device_v1*,
                                  zwlr_data_control_offer_v1* offer) {
        static_cast<WaylandBackend*>(data)->on_selection(offer);
    }
    static void device_finished(void*, zwlr_data_control_device_v1*) {}
    static void device_primary_selection(void* data, zwlr_data_control_device_v1*,
                                          zwlr_data_control_offer_v1* offer) {
        // We only manage the regular selection; drop primary offers.
        static_cast<WaylandBackend*>(data)->cleanup_offer(offer);
    }

    static void source_send(void* data, zwlr_data_control_source_v1*,
                             const char* mime, int fd) {
        static_cast<WaylandBackend*>(data)->on_source_send(mime, fd);
    }
    static void source_cancelled(void* data, zwlr_data_control_source_v1* source) {
        static_cast<WaylandBackend*>(data)->on_source_cancelled(source);
    }

    // ---- state ----
    wl_display* display_ = nullptr;
    wl_registry* registry_ = nullptr;
    zwlr_data_control_manager_v1* manager_ = nullptr;
    wl_seat* seat_ = nullptr;
    zwlr_data_control_device_v1* device_ = nullptr;
    zwlr_data_control_source_v1* source_ = nullptr;

    std::function<void(const std::string&)> on_change_;
    std::map<zwlr_data_control_offer_v1*, std::vector<std::string>> offer_mimes_;
    std::string current_content_;
    bool self_owned_ = false;

    std::string undo_buffer_;
    bool has_undo_ = false;

    int wake_read_fd_ = -1;
    int wake_write_fd_ = -1;
    std::mutex req_mutex_;
    std::condition_variable req_cv_;
    bool undo_requested_ = false;
    bool undo_done_ = false;
    bool undo_restored_ = false;

    // ---- listener vtables ----
    const wl_registry_listener registry_listener_{registry_global, registry_global_remove};
    const zwlr_data_control_offer_v1_listener offer_listener_{offer_offer};
    const zwlr_data_control_device_v1_listener device_listener_{
        device_data_offer, device_selection, device_finished, device_primary_selection};
    const zwlr_data_control_source_v1_listener source_listener_{source_send, source_cancelled};
};

std::unique_ptr<ClipboardBackend> make_wayland_backend() {
    return std::make_unique<WaylandBackend>();
}

} // namespace clipd
