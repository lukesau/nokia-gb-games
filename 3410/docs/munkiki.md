# Munkiki's Castles

The Nokia 3410's one Java game, re-implemented in C for the Game Boy and
the GBA from a decompile of its JAR. On the phone it is a MIDlet (MIDP 1.0
with the Nokia UI and M3D APIs) in the PMM, reached from Games, More
games, as "Munkiki", and run by the phone's KVM; the port lists it sixth
in Select game, as a game got for the phone, since there is no More games
here. That is a deliberate difference from the phone (`../../docs/authenticity.md`).

## What it is

A 3D puzzle on a 16 by 8 by 16 grid of blocks: Munkiki, a monkey, walks
the castle walls collecting the musical notes of a level, jumping gaps,
climbing steps, pushing blocks, breaking them with his stick, riding
travelling blocks and bouncing off bouncy ones; a magic key or a warp ends
a level early. 30 levels in five castles, the Instructions with six demo
levels, a High scores page, and Options for the game's own sounds and
lights. The view is from behind him, looking down a little, or through his
eyes.

## How the port is built

- `tools/extract_munkiki.py` reads the JAR (`roms/munkikis_castles.jar`,
  see README.md) at build time: the levels, the pictures, Munkiki's
  frames, the 3D geometry, the animation tables, the English text and the
  twelve ringing tones, written as C arrays into `build/assets/`, split
  by the bank that reads each on the Game Boy. Nothing from the JAR is
  committed.
- `core/munkiki.c` is the world: the JAR's class `d`, method for method.
  Munkiki's position is kept in the JAR's own 16.16 fixed point, since
  his moves divide a step by an animation's length; blocks and things
  in sixteenths of a cell, which is exact.
- `core/munkiki_game.c` is the flow of screens (class `KKM` and the
  canvas class `b`), `core/munkiki_pages.c` their drawing.
- `core/munkiki_3d.c` is the port's own rasterizer (the phone's M3D was
  not read): fixed-point transforms, clipping against the frustum,
  faces sorted back to front (every box is axis-aligned, so one is in
  front of another when the eye is on its side of a plane that separates
  them), and scanline fills in the grays the JAR asks for, dithered with
  the 4x4 Bayer pattern the phone's frames show. `core/munkiki_scene.c`
  orders and draws the world through it, then Munkiki and the effects.
- `core/munkiki_sprite.c` draws Munkiki's frames, which are in the Nokia
  API's vertical format and trimmed to the bands their masks touch.

The JAR's names are obfuscated; each function and field here says which
of them it is, so the Java can be read beside the C.

## How it is checked

`tools/munkiki_harness/` runs the JAR's own classes on a desktop JVM
against stubs of the APIs they use, driven by a script and logging the
game's whole state after every tick. `make check-munkiki` runs the
scripts in `tests/munkiki/` through the JAR there and through the port
(`build/munkiki_replay`) and compares every field
(`tools/munkiki_compare.py`): the first level played to its end with the
bonus count and the map, all six Instructions demos, a walk with a turn,
a jump, a fall into the water, a look around, the pause list, the Options
and the High scores page.

The picture has no such check: it is compared by eye with the phone's
frames captured in MAME (the RE fork's `make run-keys GAMES_PRODUCT=3410`
with the keys in `../../docs/authenticity-audit.md`). What was measured
from those: the camera (the JAR's, confirmed: every floor edge of the
level's start falls on its row once the rows are scaled by 49 instead
of the 48 the frustum gives, which is where the phone's M3D rounds), the
dither patterns and their phase, the horizon's row, the fonts the KVM
uses (the phone's small plain, medium bold and large bold, a glyph a row
under the line) and the list style the KVM draws the game's menus in
(a tiny-font title between lines, rows of eleven pixels in the medium
bold font, the chosen one on a bar, a scrollbar, the soft keys).

The sounds are the JAR's ringing tones decoded by the Smart Messaging
format (no tempo is set in any, so 63 bpm is assumed) and played on the
buzzer; the phone's own rendering of them was not heard, since MAME's
DSP does not play them.

## Keys

| | Lists and pages | In the game |
|---|---|---|
| D-pad | up and down | forward (2), back (8), turn left and right (4, 6) |
| A | select | push (5) |
| B | back | hit with the stick (0) |
| Start | select | the C key: the pause list (Return to game, Restart level, Game menu) |
| Select | select | look around (#); again to stop |

The phone's * (restart the level) is not on a button: Restart level is in
the pause list. The game ticks every 100 ms, as the JAR's timer does; the
phone itself draws fewer frames than that, being slower.

## What differs from the phone

- Where it is in the menus, as above.
- Continue, which the JAR keeps in a record store across power cycles,
  goes on with the game started in this session only, and goes on from
  where it was rather than from the JAR's snapshot, which snaps moving
  blocks to their cells; the level reached, the two top scores and the
  Options are kept in cartridge RAM.
- The Instructions' text pages and the Options page are drawn in the
  list style; the phone's Form pages for them were not captured.
- The warp's texture is drawn once over its square; the JAR's texture
  coordinates run 0 to 4, which may tile it.
- The Game Boy keeps the phone's pace by drawing fewer pictures of the
  3D view when a tick's picture takes longer than a tick: a picture of
  the first level's start takes it about twenty frames, so it shows some
  three a second there and fewer where more blocks are in view, while
  the game's ticks go on at ten a second. Its rasterizer keeps to 8-bit
  products and 16-bit divisions, with the row filler, the multiply, the
  divider and the box vertices in assembly (`core/munkiki_3d.c`); SDCC's
  own 32-bit division costs some 17000 cycles there.
