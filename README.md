# clipd

A background daemon that watches your clipboard, detects what you copied
(JSON, a stack trace, code, or plain text), and cleans it up in place —
so by the time you paste, smart quotes are fixed, invisible characters
are stripped, JSON is pretty-printed, and indentation is sane.

v1 targets Linux/X11 only. Wayland and macOS support are future work.

## Status

Early scaffold. Core detection/cleaning logic is implemented and unit
tested; the X11 clipboard backend is implemented but untested on real
hardware (dev happens on macOS, runtime target is a Linux box/VPS).

## Layout

- `src/core/` — content-type detection + text cleaners. Pure logic, no
  OS dependency, unit tested in `tests/`.
- `src/platform/` — OS-specific clipboard access (X11 via XFixes for v1).
- `src/main.cpp` — daemon entrypoint: watch clipboard -> detect -> clean -> write back.
- `systemd/clipd.service` — user service unit.
- `scripts/install.sh` — build + install as a systemd --user service.

## Build (on Linux)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/clipd_tests   # run unit tests (no X11 needed)
```

Requires `libx11-dev` and `libxfixes-dev` (Debian/Ubuntu naming) to build
the daemon itself; the test binary has no such requirement.

## Install as a service

```bash
./scripts/install.sh
```

## Development notes

- The `core/` layer is intentionally OS-agnostic so it can be built and
  tested on any machine, including macOS during development.
- `platform/linux_x11.cpp` is the only file that talks to X11 directly.
