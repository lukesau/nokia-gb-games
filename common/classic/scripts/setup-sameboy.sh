#!/usr/bin/env bash
# Clone upstream SameBoy at a pinned commit into the repository's ignored
# tools/SameBoy and build its headless tester, which `make check-gb` uses
# to capture a frame. Needs rgbds (for the boot ROMs) and a C compiler.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
DEST="${SAMEBOY_DIR:-$ROOT/tools/SameBoy}"
REMOTE="${SAMEBOY_REMOTE:-https://github.com/LIJI32/SameBoy.git}"
BASE_COMMIT="213a12ce93d66b105a113debd9396306066a7cfc"

if [[ -d "$DEST/.git" ]]; then
  echo "Updating existing clone at $DEST"
  git -C "$DEST" fetch origin
else
  echo "Cloning SameBoy into $DEST"
  mkdir -p "$(dirname "$DEST")"
  git clone "$REMOTE" "$DEST"
fi
git -C "$DEST" checkout --force "$BASE_COMMIT"

make -C "$DEST" tester
echo "SameBoy tester ready: $DEST/build/bin/tester/sameboy_tester"
