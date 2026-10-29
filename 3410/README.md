# Nokia 3410 games for Game Boy and GBA

![The first screen, the Select game list and Snake II being played on the Game Boy, at the phone's size and in full screen, where Snake II has its bigger board; Space Impact, Bumper, Bantumi and Link5's panels are blank, as they are not here yet](docs/banner.png)

The Nokia 3410 (NHM-2 v5.46) follow-up to the [3310's](../3310/README.md): the 3410's built-in games
re-implemented in C from a map of the firmware, with their data read from
your own dump at build time, and its Java game from a decompile of its
JAR. One portable core runs on the host and as a
Game Boy ROM (`.gb`) and a GBA ROM (`.gba`).

What is there so far:

- the phone's menus: the main menu's Games entry, Games (Select game and
  Settings; not the phone's Download game and More games), the Select game
  list with all five games, and Snake II's own menu with New game, High
  scores, Options (Game options: the six mazes and the nine levels) and
  Instructions, drawn with the phone's fonts and text. The Select game
  list, Snake II's menu, Game options, the mazes, the Level page, the title,
  the game-over picture and the High scores page equal the phone's to the
  pixel;
- Snake II: the 3410's game, the 3310's on a 23 by 13 board with the mazes
  redrawn, its creatures, scoring, speeds and the dead snake's blinking,
  frame for frame as the phone plays it; its title animation, and the
  game-over picture with the score in its box, blinking for a new top
  score, kept for each maze as on the 3410; and the High scores page with
  its boxes and the snake that eats a creature on its way across;
- Space Impact: the 3410's game, the 3310's rebuilt on the 3410's
  graphics library with its eight chapters in a file, frame for frame as
  the phone plays it through every chapter, its bosses and game over; its
  title, its menu (Continue after a pause, New game, High scores,
  Chapters, Instructions) and its High scores page, all to the pixel. The
  Instructions are the phone's four texts with the three demos between
  them, frame for frame as the phone plays them, and the Chapters note
  "Done" is there without its tick. On the Game Boy the game is
  drawn straight into the tiles' rows from pictures the build makes
  ready, the same to the pixel, and keeps the phone's pace with a picture
  nearly every tick, about nine a second where the phone shows ten;
- Bantumi: the 3410's game, the 3310's on the 3410's graphics library and
  a bigger board, with the 3310's port's code (`../common/bantumi/`), frame
  for frame as the phone plays it at every level, its hints, the pass on
  the first turn (#), a pause and Continue, the board sliding in and out;
  its title, its menu (Continue after a pause, New game, Options with the
  beans and the level, Instructions), all to the pixel, and the
  Instructions' five texts with their five demos, frame for frame. On the
  Game Boy the phone's search at the hardest level runs at about 60% of
  the phone's pace on a DMG and at full pace on a Game Boy Color or
  Advance;
- the games' Settings (Game sounds, Game lights, Shakes, Club Nokia ID),
  the vibrator as a rumble motor (Snake II's from a death to the second
  blink, Space Impact's for three ticks, as on the 3410, but half as
  long, see `../common/core/rumble.h`), and the games' sounds: Snake
  II's eat, death and game over, and every one of Space Impact's and
  Bantumi's, which their replays check against the phone's, call for
  call;
- the top score and the chosen level, maze and beans, kept in
  battery-backed cartridge RAM.

- Munkiki's Castles: the phone's Java game, re-implemented in C from a
  decompile of its JAR, which the build reads for its levels, pictures,
  frames, text and sounds: all 30 levels and the six Instructions demos,
  the rules checked tick for tick against the JAR itself run in a harness
  (`make check-munkiki`); its 3D view drawn by the port's own rasterizer
  from the JAR's geometry and camera, which match the phone's frames in
  where everything falls but not pixel for pixel; its menu, High scores,
  Instructions and Options in the style the phone draws them. On the
  phone it is under Games, More games; the port lists it sixth in Select
  game, as a game got for the phone. See [docs/munkiki.md](docs/munkiki.md);

Not yet: Bumper and Link5, the Chapters note's tick, the main menu's
Games icon and the lists' sliding highlight.

The game is shown at the phone's size, in the middle of the Game Boy's
screen. Start on the first screen picks the full-screen variant, whose
menus fill the screen and in which Snake II is played on a bigger board:
the phone's own rule for the board's size applied to the Game Boy's
screen, 39 by 33 cells, with the snake, the food, the creatures and the
score at the phone's size and speed, the mazes moved out in proportion to
fill it, and top scores of its own.

The GBA uses the GBA layer the 3310 shares (`../common/platform/gba/`):
the phone's LCD at 2x, 192 by 130, in the middle of its screen, and the
full-screen variant's menus at the GBA's size, its Snake II board 29 by 17
cells shown at 2x over the whole screen. The keys are the same; the
vibrator drives a rumble cartridge's motor.

| | Menus | Snake II | Space Impact | Bantumi | Munkiki's Castles |
|---|---|---|---|---|---|
| D-pad | up and down | steer (2, 4, 6, 8) | fly (8, 0, *, #) | left and right move the hand (4, 6), up passes the first move (#) | forward, back, turn (2, 8, 4, 6) |
| A | select | turn clockwise (#) | fire (1) | sow (5) | push (5) |
| B | back | turn anticlockwise (*) | special weapon (4) | hint, at level 1 (*) | hit (0) |
| Start, Select | select | pause | pause | pause | Start the pause list (C), Select look around (#) |

See [docs/plan.md](docs/plan.md) and, in the RE fork,
`docs/games_applications_3410.md`, `docs/games_snake2_3410.md`,
`docs/games_si_3410.md` and `docs/games_bantumi_3410.md`.

## Building

`make dump` makes the flash image from the Wintesla files below (it needs
the RE fork, in whose `ports/` directory this repository is kept, or
`DCT3_RE=` pointing at it; from here it is `../../..`), `make gb` the ROM, `make run-gb` opens it in SameBoy; `make gba` and
`make run-gba` the same for the GBA, in mGBA. `make test` runs the host
checks; `make check-gb` and `make check-gba` run the ROMs headlessly
against the host's frames (`check-gba` with mGBA's core, which
`../common/scripts/setup-mgba.sh` builds).
`make check-munkiki` runs Munkiki's Castles' scripts through the JAR
itself (a JVM is needed) and the port and compares every field of the
game's state after every tick.
`make golden-snake`, `make golden-si` and `make golden-bantumi` record
Snake II, Space Impact and Bantumi games in the fork's MAME
(`golden-si-menus` Space Impact's title and High scores, `golden-si-demos`
and `golden-bantumi-demos` the Instructions' demos, the latter with
Bantumi's title) and `make check-golden` replays them through the core; `make check-menus` compares
the menu pages with the phone's in `golden/menus/`.

## Firmware policy

No Nokia firmware, no extracted code or data from it, and nothing derived
from a firmware image is committed here. `roms/`, run output and build
output are ignored.

That being said I got my firmware at firmware center
<https://firmware.center/firmware/Nokia/3410%20%28NHM-2%29/Flash%20Files/NHM-2%20v.05.46%203410.rar>

## Dump

Put the Wintesla flash files of NHM-2 v5.46 in `roms/3410-nhm2-v546/`:

| file | SHA-256 |
|---|---|
| `NHM2NX05.460` (MCU) | `713c6de148eabd907b3eefd4dd43637279b621eb2c94bed09385e9dc4094693f` |
| `NHM2NX05.46E` (PPM E) | `779b3c13726a5878a117eed9e91d021597bc4d74dedeb1dba26e9335df1ee633` |
| `3410 virgin eeprom.pmm` | `35cf261f9aacae40ad2b95f6646be2b07220e92c8b6c119f958c6bc5e929a632` |

For now the RE fork's `make normalize-3410` lays them out for MAME:

| output in `roms/noki3410/` | size | SHA-256 |
|---|---:|---|
| `3410f546e.fls` | `0x370000` | `4b0e815a07dc18b3b5f1cff1134de55aa514eaa7ce5a0eecc44a270867562b04` |
| `3410 virgin eeprom 005f0000.fls` | `0x90000` | `f00f6974a6c03846ed1109e55d76e336e55767c2954be4ea2ae36687260a5863` |

Munkiki's Castles is a JAR inside the PMM (40888 bytes, SHA-256
`e94f9c942968acfadd13e19c62fc0aa8e48f0758a2ec00f2a9ba42269433caf8`),
kept locally as `roms/munkikis_castles.jar`, which the build reads
(`JAR=` points elsewhere).

## License

GNU Affero General Public License v3.0, see [LICENSE](LICENSE).
