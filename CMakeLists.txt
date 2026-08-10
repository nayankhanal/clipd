cmake_minimum_required(VERSION 3.16)
project(clipd CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

find_package(Threads REQUIRED)

# ---- core (OS-agnostic, unit-testable) ----
add_library(clipd_core
    src/core/config.cpp
    src/core/content_type.cpp
    src/core/detector.cpp
    src/core/cleaners/json_formatter.cpp
    src/core/cleaners/text_normalizer.cpp
    src/core/cleaners/indent_fixer.cpp
)
target_include_directories(clipd_core PUBLIC src)

# ---- platform backend ----
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    add_library(clipd_platform src/platform/linux_x11.cpp)
    target_link_libraries(clipd_platform PUBLIC ${X11_LIBRARIES} Xfixes)
    target_include_directories(clipd_platform PUBLIC src ${X11_INCLUDE_DIR})
else()
    message(WARNING "No platform clipboard backend for this OS yet (v1 targets Linux/X11). "
                     "clipd_core will still build and its tests will still run.")
    add_library(clipd_platform INTERFACE)
endif()

# ---- control-socket IPC (plain POSIX sockets, builds on any Unix) ----
add_library(clipd_ipc
    src/ipc/socket_path.cpp
    src/ipc/socket_server.cpp
)
target_include_directories(clipd_ipc PUBLIC src)
target_link_libraries(clipd_ipc PUBLIC Threads::Threads)

# ---- daemon ----
if(UNIX AND NOT APPLE)
    add_executable(clipd src/main.cpp)
    target_link_libraries(clipd PRIVATE clipd_core clipd_platform clipd_ipc)
endif()

# ---- clipdctl (control CLI, needs only the socket path, builds anywhere) ----
add_executable(clipdctl cli/clipdctl.cpp)
target_link_libraries(clipdctl PRIVATE clipd_ipc)

enable_testing()
add_subdirectory(tests)
