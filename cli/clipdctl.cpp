#include "ipc/socket_path.h"

#include <cstring>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace {

void print_usage() {
    std::cerr << "usage: clipdctl <pause|resume|toggle|status>\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        print_usage();
        return 1;
    }

    std::string command = argv[1];
    if (command != "pause" && command != "resume" && command != "toggle" && command != "status") {
        print_usage();
        return 1;
    }

    std::string path = clipd::ipc::socket_path();
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "clipdctl: failed to create socket\n";
        return 1;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        std::cerr << "clipdctl: could not connect to clipd at " << path
                   << " (is the daemon running?)\n";
        close(fd);
        return 1;
    }

    std::string request = command + "\n";
    ssize_t written = write(fd, request.data(), request.size());
    (void)written;

    char buffer[256] = {0};
    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (n > 0) {
        std::cout.write(buffer, n);
    }
    return 0;
}
