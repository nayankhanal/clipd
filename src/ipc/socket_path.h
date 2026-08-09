#pragma once

#include <string>

namespace clipd::ipc {

// Where the daemon's control socket lives. Shared between the daemon
// (socket_server.cpp) and the clipdctl CLI so they always agree.
std::string socket_path();

} // namespace clipd::ipc
