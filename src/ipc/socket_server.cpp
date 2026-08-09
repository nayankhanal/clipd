#include "ipc/socket_server.h"
#include "ipc/socket_path.h"

#include <cstring>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace clipd {

ControlServer::ControlServer(std::atomic<bool>& paused) : paused_(paused) {}

ControlServer::~ControlServer() {
    stop();
}

void ControlServer::start() {
    std::string path = ipc::socket_path();
    unlink(path.c_str());

    listen_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        throw std::runtime_error("clipd: failed to create control socket");
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        close(listen_fd_);
        throw std::runtime_error("clipd: failed to bind control socket at " + path);
    }
    if (listen(listen_fd_, 4) != 0) {
        close(listen_fd_);
        throw std::runtime_error("clipd: failed to listen on control socket");
    }

    running_ = true;
    thread_ = std::thread(&ControlServer::run, this);
}

void ControlServer::stop() {
    if (!running_) return;
    running_ = false;
    if (listen_fd_ >= 0) {
        shutdown(listen_fd_, SHUT_RDWR);
        close(listen_fd_);
        listen_fd_ = -1;
    }
    if (thread_.joinable()) thread_.join();
    unlink(ipc::socket_path().c_str());
}

void ControlServer::run() {
    while (running_) {
        int client_fd = accept(listen_fd_, nullptr, nullptr);
        if (client_fd < 0) {
            break; // listen_fd_ was closed by stop(), or a real error
        }
        handle_client(client_fd);
        close(client_fd);
    }
}

void ControlServer::handle_client(int client_fd) {
    char buffer[256] = {0};
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0) return;

    std::string command(buffer, static_cast<size_t>(n));
    while (!command.empty() && (command.back() == '\n' || command.back() == '\r')) {
        command.pop_back();
    }

    std::string response;
    if (command == "pause") {
        paused_ = true;
        response = "ok paused\n";
    } else if (command == "resume") {
        paused_ = false;
        response = "ok resumed\n";
    } else if (command == "toggle") {
        paused_ = !paused_;
        response = paused_ ? "ok paused\n" : "ok resumed\n";
    } else if (command == "status") {
        response = paused_ ? "paused\n" : "running\n";
    } else {
        response = "error unknown command\n";
    }

    write(client_fd, response.data(), response.size());
}

} // namespace clipd
