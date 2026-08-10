# clipd

A background daemon that watches your clipboard, detects what you copied
(JSON, a stack trace, code, or plain text), and cleans it up in place —
so by the time you paste, smart quotes are fixed, invisible characters
are stripped, JSON is pretty-printed, and indentation is sane.

v1 targets Linux/X11 only. Wayland and macOS support are future work.

## Status

Working v1. Core detection/cleaning logic is unit tested; the X11
clipboard backend, pause/resume control, config loading, and the
systemd service have all been verified running live on real hardware
(Ubuntu 24.04). Detector heuristics are still basic and will misclassify
some edge cases — see Known limitations.

## Known limitations

- **Linux/X11 only.** No Wayland or macOS backend yet. If your session
  runs Wayland (check with `echo $XDG_SESSION_TYPE`), this won't see
  clipboard changes.
- **Detector heuristics are basic.** Content-type detection (JSON vs.
  stack trace vs. code vs. plain) is a small set of hand-written rules,
  not a real parser — it will misfire on some inputs (e.g. prose that
  happens to contain code-like punctuation). Expect to hit false
  positives/negatives during real use; these get fixed as they're found,
  not guessed at upfront.
- **No packaging yet.** Build from source only — no AUR/`.deb`/etc. yet.
- **Text-only.** Non-text clipboard content (images, files) is expected
  to no-op safely (the X11 backend only reads `UTF8_STRING` conversions),
  but this hasn't been extensively tested across clipboard producers.

## Layout

- `src/core/` — content-type detection, text cleaners, and config
  loading. Pure logic, no OS dependency, unit tested in `tests/`.
- `src/platform/` — OS-specific clipboard access (X11 via XFixes for v1).
- `src/ipc/` — Unix domain socket control server the daemon listens on.
- `src/main.cpp` — daemon entrypoint: watch clipboard -> detect -> clean -> write back.
- `cli/clipdctl.cpp` — control CLI (pause/resume/toggle/status).
- `systemd/clipd.service` — user service unit.
- `scripts/install.sh` — build + install `clipd`/`clipdctl` as a systemd --user service.
- `config/clipd.example.toml` — example config; copy to `~/.config/clipd/config.toml`.

## Prerequisites (Debian/Ubuntu naming)

```bash
sudo apt install build-essential cmake libx11-dev libxfixes-dev
```

The core/test targets have no X11 dependency and will build without
`libx11-dev`/`libxfixes-dev`, but the `clipd` daemon itself needs them.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/tests/clipd_tests   # run unit tests (no X11 needed)
```

## Install as a service

```bash
./scripts/install.sh
```

Installs `clipd` and `clipdctl` to `/usr/local/bin`, sets up
`clipd.service` as a `systemd --user` unit, and starts it. Re-running
this script after pulling new changes rebuilds and restarts the service.

```bash
systemctl --user status clipd     # check it's running
clipdctl status                   # pause/resume/toggle/status
journalctl --user -u clipd -f     # watch it live
```

## Configuration

Copy `config/clipd.example.toml` to `~/.config/clipd/config.toml` to
toggle detectors on/off or change indentation settings. Missing file or
unrecognized keys fall back to defaults — nothing is required.

## Development notes

- The `core/` layer is intentionally OS-agnostic so it can be built and
  tested on any machine, including macOS during development.
- `platform/linux_x11.cpp` is the only file that talks to X11 directly.
