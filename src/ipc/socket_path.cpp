#include "ipc/socket_path.h"

#include <cstdlib>
#include <sstream>
#include <unistd.h>

namespace clipd::ipc {

std::string socket_path() {
    if (const char* runtime_dir = std::getenv("XDG_RUNTIME_DIR")) {
        return std::string(runtime_dir) + "/clipd.sock";
    }
    std::ostringstream oss;
    oss << "/tmp/clipd-" << getuid() << ".sock";
    return oss.str();
}

} // namespace clipd::ipc
