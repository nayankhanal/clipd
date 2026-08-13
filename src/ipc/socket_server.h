#pragma once

#include <atomic>
#include <functional>
#include <thread>

namespace clipd {

// Background control socket the daemon listens on so clipdctl can
// pause/resume/query/undo without restarting the process.
class ControlServer {
public:
    // `on_undo` is invoked when an "undo" command arrives; it returns
    // true if something was restored. May block until the watch thread
    // processes the request.
    ControlServer(std::atomic<bool>& paused, std::function<bool()> on_undo);
    ~ControlServer();

    ControlServer(const ControlServer&) = delete;
    ControlServer& operator=(const ControlServer&) = delete;

    // Binds the socket and starts accepting connections on a background thread.
    void start();

    // Stops the accept loop and joins the background thread.
    void stop();

private:
    void run();
    void handle_client(int client_fd);

    std::atomic<bool>& paused_;
    std::function<bool()> on_undo_;
    int listen_fd_ = -1;
    std::thread thread_;
    std::atomic<bool> running_{false};
};

} // namespace clipd
