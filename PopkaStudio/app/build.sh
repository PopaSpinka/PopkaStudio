#!/bin/bash
# PopkaStudio.app — the editor as a Mac app (main.js), with its icon (make_icon.py), into /Applications.
# The app holds only its window: the editor's page, studio.py and the firmware stay here, in the repository (home.json
# tells the app where) — a change to the editor needs no new app, only this script when main.js or the icon change.
#
#   PopkaStudio/app/build.sh            → /Applications/PopkaStudio.app (then drag it to the Dock)
set -euo pipefail
cd "$(dirname "$0")"
APP="$PWD"
# Python with numpy, Pillow and pyserial: install.sh's (.venv next to PopkaStudio), else Homebrew's, else python3
ROOT="$(cd ../.. && pwd)"
if [ -z "${PY:-}" ]; then
  for p in "$ROOT/.venv/bin/python" /opt/homebrew/opt/python@3.14/bin/python3.14 "$(command -v python3)"; do
    [ -x "$p" ] && { PY="$p"; break; }
  done
fi
DEST="${DEST:-/Applications}"

[ -d node_modules/electron ] || npm install
[ -f icon.icns ] || "$PY" make_icon.py
printf '{ "studio": "%s", "python": "%s" }\n' "$(cd .. && pwd)" "$PY" > home.json

npx @electron/packager . PopkaStudio --platform=darwin --arch="$(uname -m | sed 's/x86_64/x64/')" \
  --icon=icon.icns --out=dist --overwrite --app-bundle-id=com.popka.studio \
  --app-category-type=public.app-category.developer-tools --darwin-dark-mode-support \
  --ignore='^/dist' --ignore='^/make_icon\.py' --ignore='^/build\.sh' --ignore='^/icon\.(icns|png)$' >/dev/null
BUILT="$(ls -d dist/PopkaStudio-darwin-*/PopkaStudio.app | head -1)"
rm -f home.json

# Signed for this Mac only (no Apple account needed): an app made here runs here
codesign --force --deep --sign - "$BUILT" 2>/dev/null

# Into /Applications: an older PopkaStudio.app there — only ours is replaced
OLD="$DEST/PopkaStudio.app"
if [ -d "$OLD" ]; then
  [ "$(defaults read "$OLD/Contents/Info" CFBundleIdentifier 2>/dev/null)" = com.popka.studio ] || { echo "$OLD is not ours — left as it is" >&2; exit 1; }
  rm -rf "$OLD"
fi
ditto "$BUILT" "$OLD"
touch "$OLD"
/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister -f "$OLD" || true
echo "$OLD — $(du -sh "$OLD" | cut -f1)"
