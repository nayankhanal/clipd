#!/usr/bin/env bash
set -euo pipefail

# Builds clipd and installs it as a systemd --user service.
# Run from the repo root: ./scripts/install.sh

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j

sudo install -m 755 "$BUILD_DIR/clipd" /usr/local/bin/clipd
sudo install -m 755 "$BUILD_DIR/clipdctl" /usr/local/bin/clipdctl

mkdir -p "$HOME/.config/systemd/user"
cp "$ROOT_DIR/systemd/clipd.service" "$HOME/.config/systemd/user/clipd.service"

systemctl --user daemon-reload
systemctl --user enable clipd
systemctl --user restart clipd

echo "clipd installed and running. Check status with: systemctl --user status clipd"
echo "control it with: clipdctl <pause|resume|toggle|status>"
