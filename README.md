# clipd

A background daemon that watches your clipboard, detects what you copied
(JSON, a stack trace, code, or plain text), and cleans it up in place —
so by the time you paste, smart quotes are fixed, invisible characters
are stripped, JSON is pretty-printed, and indentation is sane.

Linux only. Works on X11, and on Wayland compositors that implement the
`wlr-data-control` protocol (Sway, Hyprland, river, ...). macOS support
is future work.

## Status

Working. Core detection/cleaning logic is unit tested; the X11 and
Wayland backends, pause/resume control, undo, config loading, and the
systemd service have all been verified running live on real hardware
(Ubuntu 24.04; Wayland verified on Sway). Detector heuristics are still
basic and will misclassify some edge cases — see Known limitations.

## Known limitations

- **No macOS backend yet.** Linux only.
- **Wayland support is wlroots-only.** It uses the `wlr-data-control`
  protocol, which Sway, Hyprland, river, etc. implement — but GNOME
  (Mutter) and KDE (KWin) do not, so clipd can't watch the clipboard
  there yet. Check your session with `echo $XDG_SESSION_TYPE`; the
  backend is chosen automatically at startup.
- **Detector heuristics are basic.** Content-type detection (JSON vs.
  stack trace vs. code vs. plain) is a small set of hand-written rules,
  not a real parser — it will misfire on some inputs (e.g. prose that
  happens to contain code-like punctuation). Expect to hit false
  positives/negatives during real use; these get fixed as they're found,
  not guessed at upfront.
- **No packaging yet.** Build from source only — no AUR/`.deb`/etc. yet.
- **Text-only.** Non-text clipboard content (images, files) is expected
  to no-op safely (the backends only read text MIME types / `UTF8_STRING`
  conversions), but this hasn't been extensively tested across clipboard
  producers.

## Layout

- `src/core/` — content-type detection, text cleaners, and config
  loading. Pure logic, no OS dependency, unit tested in `tests/`.
- `src/platform/` — OS-specific clipboard access: X11 via XFixes
  (`linux_x11.cpp`), Wayland via wlr-data-control (`linux_wayland_impl.cpp`),
  and `backend_factory.cpp` which picks one at runtime.
- `protocol/` — the `wlr-data-control` protocol XML (Wayland bindings are
  generated from it at build time by `wayland-scanner`).
- `src/ipc/` — Unix domain socket control server the daemon listens on.
- `src/main.cpp` — daemon entrypoint: watch clipboard -> detect -> clean -> write back.
- `cli/clipdctl.cpp` — control CLI (pause/resume/toggle/status/undo).
- `systemd/clipd.service` — user service unit.
- `scripts/install.sh` — build + install `clipd`/`clipdctl` as a systemd --user service.
- `config/clipd.example.toml` — example config; copy to `~/.config/clipd/config.toml`.

## Prerequisites (Debian/Ubuntu naming)

```bash
# X11 support (required)
sudo apt install build-essential cmake pkg-config libx11-dev libxfixes-dev

# Wayland support (optional; enables the wlr-data-control backend)
sudo apt install libwayland-dev wayland-protocols
```

The Wayland backend is built automatically when `libwayland-client` and
`wayland-scanner` (needs `pkg-config`) are found; otherwise it's skipped
and the build is X11-only — no configuration flag needed either way. The
core/test targets have no display dependency and build without any of
these.

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
clipdctl status                   # pause/resume/toggle/status/undo
clipdctl undo                     # restore the clipboard to its pre-clean value
journalctl --user -u clipd -f     # watch it live
```

`clipdctl undo` reverses the most recent clean by restoring the original
copied value to the clipboard — one level of history, so if a detector
misfires you can get the untouched text back without re-copying it.

## Configuration

Copy `config/clipd.example.toml` to `~/.config/clipd/config.toml` to
toggle detectors on/off or change indentation settings. Missing file or
unrecognized keys fall back to defaults — nothing is required.

## Development notes

- The `core/` layer is intentionally OS-agnostic so it can be built and
  tested on any machine, including macOS during development.
- The `platform/` backends are the only code that talks to X11 or Wayland
  directly; everything above them works on plain strings.
- To runtime-test the Wayland backend on a headless or X11-only box, run a
  nested headless Sway session and drive it with wl-clipboard:
  `WLR_BACKENDS=headless sway -c /dev/null &`, then
  `export WAYLAND_DISPLAY=wayland-1` and use `wl-copy`/`wl-paste`.
