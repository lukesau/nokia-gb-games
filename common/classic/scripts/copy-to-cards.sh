#!/usr/bin/env bash
# Copy the built ROMs to the root of the flash carts' SD cards: the .gb to
# the two EZ-Flash Junior cards and the .gba to the EZ-Flash Omega card.
#
# Usage: scripts/copy-to-cards.sh [ROM...]
#
# With no arguments it takes every .gb and .gba in build/ except the
# scripted test ROMs (*-keys.*). A card that is not mounted is skipped with
# a note, and the rest are still done. Each copy is compared with its
# source afterwards. Nothing is ejected.
#
# A .gba without the boot logo is not copied: a console refuses it. The
# build copies the logo from a ROM it finds (common/tools/gbafix.py), or
# from `make gba GBA_LOGO_FROM=/path/to/some.gba`.
#
# Exit status: 0 when every ROM reached every mounted card it belongs on
# and at least one card was there; 1 when a copy failed, a ROM was refused,
# or no card was mounted.
set -u

# Run from the port's directory, as its `make cards` does.
ROOT="$(pwd)"
GB_CARDS=(/Volumes/EZGB_FW4 /Volumes/EZGB_FW5)
GBA_CARDS=(/Volumes/OMEGADE)
LOGO_SHA1="17daa0fec02fc33c0f6abb549a8b80b6613b48ee"

if [[ $# -gt 0 ]]; then
  roms=("$@")
else
  roms=()
  for rom in "$ROOT"/build/*.gb "$ROOT"/build/*.gba; do
    [[ -f "$rom" && "$rom" != *-keys.* ]] && roms+=("$rom")
  done
fi
if [[ ${#roms[@]} -eq 0 ]]; then
  echo "No ROMs to copy: build them first (make gb gba)." >&2
  exit 1
fi

copied=0
failed=0
missing=()

has_logo() {
  [[ "$(dd if="$1" bs=1 skip=4 count=156 2>/dev/null | shasum | cut -d' ' -f1)" == "$LOGO_SHA1" ]]
}

copy_to() {
  local rom="$1" card="$2" name dest
  name="$(basename "$rom")"
  dest="$card/$name"
  if [[ ! -d "$card" ]] || ! mount | grep -q " on $card "; then
    [[ " ${missing[*]-} " == *" $card "* ]] || missing+=("$card")
    return
  fi
  # No extended attributes, so macOS leaves no ._ file beside the ROM.
  if COPYFILE_DISABLE=1 cp -X "$rom" "$dest" && rm -f "$card/._$name" && cmp -s "$rom" "$dest"; then
    echo "copied   $name -> $card"
    copied=$((copied + 1))
  else
    echo "FAILED   $name -> $card" >&2
    failed=$((failed + 1))
  fi
}

for rom in "${roms[@]}"; do
  if [[ ! -f "$rom" ]]; then
    echo "FAILED   $rom: no such file" >&2
    failed=$((failed + 1))
    continue
  fi
  case "$rom" in
    *.gb)
      for card in "${GB_CARDS[@]}"; do copy_to "$rom" "$card"; done
      ;;
    *.gba)
      if ! has_logo "$rom"; then
        echo "REFUSED  $(basename "$rom"): no boot logo; see common/tools/gbafix.py" >&2
        failed=$((failed + 1))
        continue
      fi
      for card in "${GBA_CARDS[@]}"; do copy_to "$rom" "$card"; done
      ;;
    *)
      echo "FAILED   $rom: neither a .gb nor a .gba" >&2
      failed=$((failed + 1))
      ;;
  esac
done

sync
for card in "${missing[@]-}"; do
  [[ -n "$card" ]] && echo "skipped  $card: not mounted"
done
echo "$copied copied, $failed failed, ${#missing[@]} card(s) not mounted; nothing ejected"
[[ $failed -eq 0 && $copied -gt 0 ]]
