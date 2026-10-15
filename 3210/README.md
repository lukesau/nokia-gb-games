# Nokia 3210 games for Game Boy and Game Boy Advance

![The first screen, the list of games, and Rotation, Snake and Memory being played, on the Game Boy and the Game Boy Advance, each in the phone-sized mode and in full screen](docs/banner.png)

Ports of the Nokia 3210 (NSE-8/9 v6.00) built-in games to two cartridges
built from one portable C core:

- a Game Boy ROM (`.gb`), and
- a Game Boy Advance ROM (`.gba`).

Targets: Snake, Memory and Rotation as shipped. The two games the firmware
carries but never offers, React (a shooting gallery in its plugin table)
and the Mastermind-style Logic, were re-implemented and tried in October
2026 and dropped again: they are not fun to play. The fork's
`docs/games_applications.md` keeps everything learned about them.

The games are re-implemented from a function-level map of the firmware, not
copied from it. The map, the tracing tools and the evidence live in a fork of
the upstream Nokia DCT3 MAME project:
<https://github.com/lukesau/nokia-dct3-re>, branch `games/re`, document
`docs/games_applications.md`.

## Why a re-implementation

The original plan was to run the firmware's own game code directly on the
GBA behind a thin wrapper, since the 3210 and the GBA share the same ARM7TDMI
core. Endianness foiled it: the 3210 runs its ARM7TDMI big-endian and the
game code depends on that (it packs bytes into words and pulls fields out by
shifting, and reads 16-bit values from byte records), while the GBA is
little-endian. Re-implementing the games in C was the way out, and it also
made a Game Boy build possible.

## Firmware policy

No Nokia firmware, no extracted code or data from it, and nothing derived
from a firmware image is committed here. Graphics and strings are read from
your own legally obtained dump at build time into an ignored directory.

That being said I got my firmware at firmware center
<https://firmware.center/firmware/Nokia/3210%20(NSE-8-9)/Flash%20Files/NSE-8%20v.06.00%203210%20NSE-9.rar>

## Layout

The games and menus are shared with the 5110 port, whose games are the
same, and live in `../common/classic/`; this directory holds what is the
3210's own.

- `core/`: Rotation, and `phone.h`, the 3210's list of games and what it
  has (the Games icon, the Top score animation).
- `Makefile`: names the phone and its dump, then includes
  `../common/classic/classic.mk`.
- `../common/classic/`: Snake, Memory and Logic, the phone's menus, `rand`,
  the sounds, the event and platform interface in `game.h`;
  `extract_assets.py`, which reads the pictures, fonts and text from your
  dump into C arrays under the ignored `build/assets/`; and the layers:
  - `platform/host/`: host layer used for verification; writes LCD frames
    as PGM in the layout of the fork's MAME frames.
  - `platform/gb/`: Game Boy layer (84x48 drawn 1:1 as background tiles).
  - `scripts/setup-sameboy.sh`: clones upstream SameBoy at a pinned commit
    into the repository's ignored `tools/SameBoy/` and builds its headless
    tester; `scripts/copy-to-cards.sh` is `make cards`.
  - `tests/`: host checks that need no firmware.
- `../common/`: what every port shares (the framebuffer and drawing, fonts,
  the full-screen menus' tiles, `gbafix.py`, `check_gb_frame.py`), and the
  GBA layer all four ports use, `../common/platform/gba/` (bitmap mode 3;
  the phone's screen and the full-screen board are shown at 2x, Rotation
  and Memory in the full-screen mode at 3x by the hardware's scaling, the
  full-screen menus as they are), with its startup code.
- `scripts/make-banner.sh`: regenerates the banner above from headless
  emulator runs of both ROMs (needs ImageMagick).
- `tests/test_boards.c`: Memory's and Rotation's incremental drawing.

## Toolchains

On macOS with Homebrew:

```
brew install sdcc rgbds arm-none-eabi-gcc
../common/classic/scripts/setup-sameboy.sh
```

- Game Boy: SDCC's `sm83` port with its stock startup code, and `makebin`
  for the cartridge header. No GBDK.
- GBA: bare `arm-none-eabi-gcc` with no C library; `../common/platform/gba/`
  supplies the startup code, the linker script and `memset`/`memcpy`.
- Emulators: SameBoy's tester for headless Game Boy frames and mGBA's core
  library for headless GBA frames (`../common/scripts/setup-mgba.sh`, needs cmake);
  `make run-gb` and `make run-gba` open the ROMs in SameBoy.app and
  mGBA.app.

## Version

The first screen shows the port's version in its bottom right corner, and
the full-screen variant's list of games ends with an About entry that
shows it with this repository's address. Both come from `GAME_VERSION` in
`../common/core/version.h`, one number for the three ports, set by hand;
change the string there when a release deserves a new number.

## Building

```
make test                       # host checks, no firmware needed
make gb gba                     # build/nokia3210.gb and build/nokia3210.gba
make check-gb                   # run the .gb headlessly, compare with the host frame
make check-gba                  # the same for the .gba
make shot-gb KEYS=sd            # screenshot after scripted keys (u, d, l, r, s select, e Select, b back,
                                # a the full-screen key or Start, t one game move)
make check-golden               # compare host frames with MAME frames in golden/
make test-snake test-boards     # incremental drawing against full redraws, all three games
make assets DUMP=/path/to/3210f600a.fls
make sheet                      # draw the extracted assets to build/sheet_*.pgm
make fonts                      # the phone's four fonts as ASCII-art sheets in build/fonts/
```

`DUMP` defaults to `../../../roms/3210f600a.fls`, the MAME fork's, in
whose `ports/` directory this repository is kept. The dump must be
NSE-8/9 v6.00 (SHA-256 `7bf29b96…0d8a` raw, or the fork's `_swap16.bin`
form); the extractor refuses anything else.

A real console's BIOS checks the GBA ROM's boot logo, which is not in
this repository: the build copies it from a GBA ROM it finds, the ones it
built with the logo before (kept in `roms/gba-logo/`, never committed),
mGBA's test ROMs (`make setup`) or the Omega card's, or from
`make gba GBA_LOGO_FROM=/path/to/some.gba` (see
`common/tools/gbafix.py`).

`make cards` (`../common/classic/scripts/copy-to-cards.sh`) copies the ROMs to the root of
the flash carts' SD cards: the `.gb` to `/Volumes/EZGB_FW4` and
`/Volumes/EZGB_FW5`, the `.gba` to `/Volumes/OMEGADE`. It skips a card
that is not mounted, refuses a `.gba` without the boot logo, checks each
copy and ejects nothing. The checks and screenshots run separate test
ROMs (`build/nokia3210-keys.*`), which it leaves alone.

## Status

In progress. Both cartridges build and run the same core: they open on the
main menu's Games entry, whose icon animates as the phone's does (the first
picture for 1.08 s, then the other twelve at 0.19 s each and the first
again, as measured in MAME), and walk the Games list and each game's menu with
its Level, Top score and Instructions pages, drawn with the phone's own
fonts and text. All three of the phone's games play, with the pause menu's
Continue, the Game over page and Last view:

- Snake: steering, food, scoring, nine levels.
- Memory: five board sizes from 2x2 to 10x6, the firmware's deal, the
  blinking cursor, and the score counted from the tries used.
- Rotation: seven levels (a 2x2 frame on boards of 3x3 to 6x6, then a 3x3
  frame), the opening turns the game makes itself, the sliding animation,
  the clock, and the score counted from the time taken.

The Top score page has the phone's animation of stars gathering into a
cup. Menus and gameplay match frames captured from the firmware in MAME. Level
and top score are kept per game in battery-backed cartridge RAM. The games
have their buzzer sounds.

| | Snake | Memory | Rotation |
|---|---|---|---|
| D-pad | steer | move the cursor | move the frame |
| A | | turn a card | turn with the clock |
| Start | | jump to the next card face down | turn against the clock |
| Select | | jump to the previous card face down | turn with the clock |
| B | pause | pause | pause |

Start on the first screen picks a
full-screen mode instead, which is the port's own design and not the
phone's: menus laid out for the console's whole screen in the phone's large
font with every entry visible and a cursor beside the selection, and Snake
on a bigger board of the same 4-pixel cells (38x34 on the Game Boy; 28x18
on the GBA, which shows the board at 2x), with its own level, top score and
speeds. Rotation and Memory are the same games in both modes: from the
full-screen menus they are played on the phone's screen at 2x on the Game
Boy and at 3x on the GBA, where the display hardware does the scaling, and
share their level and top score with the phone-sized mode. Either way the
phone's 84 columns are four too many for the screen's width, so the last
four are left off; Memory's biggest board still fits whole. [`../docs/authenticity.md`](../docs/authenticity.md)
says where each part of the three ports comes from and where the code's
structure still differs from the firmware's. See
[`docs/handoff.md`](docs/handoff.md) for the decisions taken, the reference
material and the work items.
