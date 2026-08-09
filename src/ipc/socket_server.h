#pragma once

#include <atomic>
#include <thread>

namespace clipd {

// Background control socket the daemon listens on so clipdctl can
// pause/resume/query it without restarting the process.
class ControlServer {
public:
    explicit ControlServer(std::atomic<bool>& paused);
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
    int listen_fd_ = -1;
    std::thread thread_;
    std::atomic<bool> running_{false};
};

} // namespace clipd
