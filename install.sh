#!/usr/bin/env bash
# PopkaStudio installer for macOS and Linux. Everything goes into PopkaStudio's own folder (.venv, .tools) — nothing
# system-wide, no administrator password (on Linux: only to let you use the USB port, once).
#
#   curl -fsSL https://raw.githubusercontent.com/PopaSpinka/PopkaStudio/main/install.sh | bash
#   or, in a copy you cloned:  ./install.sh          (run it again any time to update)
#
# It installs: uv (Python and its libraries numpy, Pillow, pyserial), arduino-cli and the ESP32 core 3.3.11 (to build
# and upload the firmware), the fonts, and the command `popkastudio`.
set -euo pipefail

REPO="https://github.com/PopaSpinka/PopkaStudio.git"
CORE="esp32:esp32@3.3.11"
ESP_URL="https://espressif.github.io/arduino-esp32/package_esp32_index.json"

say() { printf '\n\033[1m▸ %s\033[0m\n' "$*"; }
note() { printf '  %s\n' "$*"; }
die() { printf '\n\033[31m✗ %s\033[0m\n' "$*" >&2; exit 1; }

OS="$(uname -s)"
case "$OS" in
  Darwin | Linux) ;;
  *) die "PopkaStudio runs on macOS and Linux." ;;
esac
command -v curl >/dev/null || die "curl is needed (Linux: sudo apt install curl)."

# ---------- 1. The code: this folder (a clone), or a fresh copy in ~/PopkaStudio ----------
HERE="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" 2>/dev/null && pwd || true)"
if [ -n "$HERE" ] && [ -f "$HERE/PopkaStudio/studio.py" ]; then
  DIR="$HERE"
  if [ -d "$DIR/.git" ] && command -v git >/dev/null; then
    say "Updating PopkaStudio"
    git -C "$DIR" pull --ff-only || note "(not updated: $DIR has changes of its own — kept as it is)"
  fi
else
  DIR="${POPKASTUDIO_DIR:-$HOME/PopkaStudio}"
  command -v git >/dev/null || die "git is needed. macOS: xcode-select --install · Linux: sudo apt install git"
  if [ -d "$DIR/.git" ]; then
    say "Updating PopkaStudio in $DIR"
    git -C "$DIR" pull --ff-only
  else
    say "Downloading PopkaStudio into $DIR"
    git clone --depth 1 "$REPO" "$DIR"
  fi
fi
cd "$DIR"
mkdir -p .tools/bin
export PATH="$DIR/.tools/bin:$PATH"

# ---------- 2. Python and its libraries (uv: its own Python, in .venv) ----------
if ! command -v uv >/dev/null; then
  say "Installing uv (a Python manager) into .tools/bin"
  curl -LsSf https://astral.sh/uv/install.sh | env UV_INSTALL_DIR="$DIR/.tools/bin" UV_NO_MODIFY_PATH=1 sh >/dev/null
fi
say "Python and its libraries"
uv venv --quiet --python 3.12 --allow-existing .venv
uv pip install --quiet --python .venv/bin/python numpy pillow pyserial
PY="$DIR/.venv/bin/python"

# ---------- 3. arduino-cli and the ESP32 core (Load: build and upload the firmware) ----------
if ! command -v arduino-cli >/dev/null; then
  say "Installing arduino-cli into .tools/bin"
  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR="$DIR/.tools/bin" sh >/dev/null
fi
CLI="$(command -v arduino-cli)"
if ! "$CLI" core list 2>/dev/null | grep -Eq '^esp32:esp32 +3\.3\.11 '; then
  say "The ESP32 tools — the longest step (1.5–3 GB, 5–20 minutes)"
  "$CLI" core update-index --additional-urls "$ESP_URL"
  "$CLI" core install "$CORE" --additional-urls "$ESP_URL"
fi

# ---------- 4. The fonts (Google Sans Flex, Font Awesome Free: from their own repositories) ----------
say "Fonts"
"$PY" - <<'EOF'
import sys
sys.path.insert(0, "tools")
import halo_assets as h
for name in ("GoogleSansFlex.ttf", "fa-solid-900.ttf"):
    h.font_path(name)
EOF

# ---------- 5. Linux: the USB port (the board is /dev/ttyACM…, its group is dialout or uucp) ----------
RELOGIN=""
if [ "$OS" = Linux ]; then
  GROUP=dialout
  getent group dialout >/dev/null || GROUP=uucp
  if getent group "$GROUP" >/dev/null && ! id -nG "$USER" | tr ' ' '\n' | grep -qx "$GROUP"; then
    say "USB access: adding you to the '$GROUP' group (asks for your password once)"
    if sudo usermod -aG "$GROUP" "$USER"; then
      RELOGIN=1
    else
      note "Not done — run later: sudo usermod -aG $GROUP $USER"
    fi
  fi
fi

# ---------- 6. The command `popkastudio` (and a menu entry on Linux) ----------
chmod +x "$DIR/popkastudio"
mkdir -p "$HOME/.local/bin"
ln -sf "$DIR/popkastudio" "$HOME/.local/bin/popkastudio"
if [ "$OS" = Linux ]; then
  mkdir -p "$HOME/.local/share/applications"
  cat > "$HOME/.local/share/applications/popkastudio.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=PopkaStudio
Comment=Watch screens for ESP32 boards
Exec=$DIR/popkastudio
Icon=$DIR/PopkaStudio/app/icon.png
Terminal=false
Categories=Development;
EOF
fi

say "PopkaStudio is ready"
case ":$PATH:" in
  *":$HOME/.local/bin:"*) note "Start it:  popkastudio" ;;
  *) note "Start it:  $DIR/popkastudio      (or add ~/.local/bin to your PATH and type popkastudio)" ;;
esac
note "It opens http://localhost:8466 in your browser. Connect the board with a USB-C cable and press Load."
[ -n "$RELOGIN" ] && note "Linux: log out and back in once, so the board's USB port opens for you."
exit 0
