# Nokia 3310 games for Game Boy and Game Boy Advance

![The first screen, the list of games, and Snake II, Space Impact, Bantumi and Pairs II being played, on the Game Boy and the Game Boy Advance, each in the phone-sized mode and in full screen, where Snake II has its bigger board](docs/banner.png)

The Nokia 3310 (NHM-5 v6.39) follow-up to the [3210's](../3210/README.md): the phone's four games,
Space Impact, Snake II, Bantumi and Pairs II, re-implemented in C from a map of the firmware, behind the phone's
own Games menus, with the levels, mazes, boards, sprites, fonts and text read from your
own dump at build time.
One portable core runs on the host, as a Game Boy ROM (`.gb`) and as a
Game Boy Advance ROM (`.gba`).

What is there so far:

- the phone's menus: the main menu's Games entry with its animated icon
  (the first picture for 0.84 s, then four pictures three times round at
  0.155 s each, as measured in MAME), the list of games,
  Space Impact's menu with Continue while a game is paused, its Top score
  page with the animation, its Instructions and the Game over page, drawn
  with the phone's fonts and text;
- the sprite and scrolling-terrain layer the 3310's games draw with;
- the player, shots, the three special weapons, the level scripts, the
  movement patterns, collisions, scoring, lives and continues;
- the bosses of all eight levels, so the game can be played to its end;
- the top score, kept in battery-backed cartridge RAM. It starts at 0;
- the game's five buzzer sounds: the shot, the missile or wall, the beam,
  a bonus collected and the ship destroyed;
- the phone's vibrator, as a rumble motor: half a second when the ship is
  hit and when a boss explodes;
- the Games menu's Settings: the phone's four pages, Sounds, Lights, Shakes
  and Club Nokia ID, with their Off and On lists and the Done note. Sounds
  and Shakes do what they say; Lights is kept but does nothing, and the
  score ID has no ID. All are kept in cartridge RAM with the top score;
- Snake II: its menu with Level (the nine bars), Mazes (No maze and the
  five mazes, with the note that one was chosen), Top score and
  Instructions; the game on the 20 by 9 board with its speeds, mazes,
  food, the small creatures and their countdown, the fat swallowed piece,
  the one short tick a turn can save the snake in, the blinking death and
  its three sounds, and the same rumble on every meal and at the end. The
  level and the maze are kept with its top score;
- Pairs II: its list of the two modes, Time trial and Puzzle, each with its
  own menu, Level (seven bars), Top score and Instructions, and its own
  record of level and top score, as on the phone. Time trial's nine boards
  with their shapes and time limits, the cards dealt one pixel a tick from
  the middle, the saloon door and the wipe, the dynamite's fuse burning
  down, the explosion when time runs out and the bonus of half the time
  left; Puzzle's grid by level, each pair found taken away to uncover the
  picture behind. Level + 4 points a pair and one off a miss, and the
  phone's two sounds; the phone does not vibrate in Pairs II.
- Bantumi: its menu with Level (five bars) and Instructions, and no Top
  score, as on the phone. The board sliding in, the hand that picks up a
  pit's beans and drops them one by one, extra turns, captures, the
  phone's own search for its move (a hundred steps a tick, from one ply at
  level 1 to eight at level 5, with its thinking picture, and with the two
  mistakes of the firmware's that change which moves it picks), the hint
  on `*` at level 1, the bean sound, the winner's store blinking, and the
  phone's three Game over pages: won, lost and a draw.

Start on the first screen picks a full-screen mode instead, as in the 3210
project: the port's own menus laid out for the console's whole screen in
the phone's large font. In that mode Snake II, as the 3210 project's Snake,
is played on a bigger board: the phone's own rule for the board's size
applied to the console's screen, 39 by 33 cells on the Game Boy and 29 by
17 on the GBA (which shows it at 2x), with the snake, the food, the
creatures and the score at the phone's size and speed, the mazes moved
out in proportion to fill it, and a top score of its own. Space Impact,
Bantumi and Pairs II are shown at 2x on the Game Boy and at 3x on the GBA,
where the display hardware does the scaling. Either way the phone's 84
columns are four too many for the screen's width, so the last four are
left off: nothing of Space Impact's score, which ends at column 75, but
enemies come on four columns late and the ship can fly its nose out of
sight; Bantumi loses the right edge of its board.

| | Menus | Space Impact | Snake II | Bantumi | Pairs II |
|---|---|---|---|---|---|
| D-pad | up and down | move the ship | steer (2, 4, 6, 8) | move the hand (4, 6; up and down as the scroll key) | move the cursor (2, 4, 6, 8) |
| A | select | fire | turn clockwise (#) | sow (5) | open a card (5) |
| B | back | special weapon | turn anticlockwise (*) | hint at level 1 (*) | open a card (5) |
| Start, Select | select | pause | pause | pause | pause |

Choosing a game from the list first plays its title animation, as on the
phone, and any button skips it. All four games' titles are there: Snake
II's, Space Impact's stars and closing logo, Bantumi's and Pairs II's
cards turning to spell its name.

A game that ends with a new top score, and a game of Bantumi won, first
plays the phone's fireworks: its six full-screen pictures from the dump,
twice over, 233 ms each, with a sound as they start and again as the
Game over page follows, as in MAME.

What is not: a paused Space Impact continues
exactly where it stopped, where the phone gives the ship a second and a
half of shield; a paused Snake II, Bantumi or Pairs II waits for a key, as
on the phone. Bantumi's losing store
blinks on the phone through the LCD's blink plane; here it is just drawn,
as Space Impact's blinking things are. Snake
II's large animated creature never comes: the phone lets it come only when
a setting the port does not have is on, and in MAME it never does.

## Firmware policy

No Nokia firmware, no extracted code or data from it, and nothing derived
from a firmware image is committed here. `roms/`, run output and build
output are ignored.

That being said I got my firmware at firmware center
<https://firmware.center/firmware/Nokia/3310%20(NHM-5)/Flash%20Files/NHM-5%20v.06.39%203310.rar>

## Dump

Put the Wintesla flash files of NHM-5 v6.39 in `roms/3310-nhm5-v639/`:

| file | SHA-256 |
|---|---|
| `NHM5NY06.390` (MCU) | `c3710f27f68472a70cca0b2af1f8f5048fa435d40d99a36b0f69ea3ab598ea5f` |
| `NHM5NY06.39E` (PPM E) | `3cbdb4e75289016cc4e4ccf7013bba9f7e591d5a27b381f7ad98d90bcbdaa5d9` |
| `v.2.pmm` | `b825400f99a9bd3eba161fc97915084cef8286215796f8792f0523d06f36945f` |

`make dump` strips their record headers and lays the three regions out at
their flash addresses (MCU `0x200000`, PPM `0x340000`, PMM `0x3d0000`):

| output in `roms/noki3310/` | size | SHA-256 |
|---|---:|---|
| `3310f639e.fls` | `0x200000` | `975ec791205f026d647254ee772d7fa32691fa50c72a68eecdaff7c8a5921442` |
| `3310 v2 pmm.bin` | `0x30000` | `dcb2212579f2a2a7059ed85ef81174d337003566ce2f83f284f20bc70aef8bf4` |

These are the files the `noki3310` driver of the MAME fork
<https://github.com/lukesau/nokia-dct3-re> declares as BIOS `639`. The
extractor and the emulator are the fork's; this repository is expected in
the fork's `ports/` directory (`DCT3_RE=../../..` from here), with its
MAME already built.

## Running the phone

```
make phone                                   # headless, 15 emulated seconds
make phone PHONE_SECONDS=13 PHONE_KEYS=enter,wait1000,enter
make phone-window                            # a MAME window
```

`make phone` leaves every LCD frame as PGM in the ignored `run_phone/` and
the last one as `run_phone/latest.png`. PPM E starts in Russian.

## Toolchains

On macOS with Homebrew:

```
brew install sdcc arm-none-eabi-gcc imagemagick
```

- Game Boy: SDCC's `sm83` port and `makebin`, with this project's own
  startup code. No GBDK.
- GBA: bare `arm-none-eabi-gcc` with no C library.
- Emulators, for the headless checks: SameBoy's core as a library
  (`scripts/setup-sameboy.sh`, needs rgbds) and mGBA's
  (`../common/scripts/setup-mgba.sh`, needs cmake). Both go into the repository's
  shared, ignored `tools/`; `SAMEBOY=` and `MGBA=` name builds elsewhere.

## Version

The first screen shows the port's version in its bottom right corner, and
the full-screen variant's list of games ends with an About entry that
shows it with this repository's address. Both come from `GAME_VERSION` in
`../common/core/version.h`, one number for the three ports, set by hand;
change the string there when a release deserves a new number.

## Building

```
make test          # core checks, no firmware needed
make assets        # extract the game data, fonts and text from the dump into build/assets/
make sheet         # the extracted sprites and tiles as build/sheet_*.pgm
make gb gba        # build/nokia3310.gb and build/nokia3310.gba
make run-gb        # open the .gb in SameBoy
make run-gba       # open the .gba in mGBA
```

A real console's BIOS checks the GBA ROM's boot logo, which is not in
this repository: the build copies it from a GBA ROM it finds, the ones it
built with the logo before (kept in `roms/gba-logo/`, never committed),
mGBA's test ROMs (`make setup`) or the Omega card's, or from
`make gba GBA_LOGO_FROM=/path/to/some.gba` (see
`common/tools/gbafix.py`).

`make cards` (`scripts/copy-to-cards.sh`) copies the ROMs to the root of
the flash carts' SD cards: the `.gb` to `/Volumes/EZGB_FW4` and
`/Volumes/EZGB_FW5`, the `.gba` to `/Volumes/OMEGADE`. It skips a card
that is not mounted, refuses a `.gba` without the boot logo, checks each
copy and ejects nothing.

The phone moves the ship with 8, 0, * and #, fires with 1 or 3 and uses
the special with 4 or 6, one key at a time; the pad is mapped onto those
keys, so only the button pressed last counts.

## Checks

```
make check-menus                 # the menu pages against the phone's own, to the pixel
make check-golden                # the games against recorded runs of the firmware (below)
make check-gb                    # the .gb in SameBoy against the host's frames, and its speed
make check-gb KEYS=a3dsss        # the same in the full-screen mode's 2x, starting at the fourth level
make check-gb KEYS=asssss        # Snake II in the full-screen mode
make check-gb KEYS=sdddsssswwwwwwwwsrsrsdsls  # Pairs II's Time trial, cards opened after the deal
make check-gb KEYS=zsddssdsuuuususwwwwwsww GB_FRAMES="600 3000"  # Bantumi at level 5, the phone's first move
make check-gba KEYS=sds SHOT_FRAMES=70   # Space Impact's title, part-way
make check-gba                   # the .gba in mGBA against the host's frames
make shot-gb KEYS=sdsss          # a screenshot after scripted keys
```

`KEYS` are pressed by a test ROM at power-on (`menu_script` in
`core/menu.c` lists them); the ROMs `make gb` and `make gba` build press
none. `check-gb` also reports how fast the ROM ran the game, 100% being
the phone's pace.

`golden/menus/` holds frames of the phone's menus captured in MAME with
the phone switched to English (Menu, 6, 2, 1, up, Select), each named
after the host frame it must equal; those that start a game begin with
`z`, the seed the phone has after power-on, which it never reseeds in
MAME since the clock is not set; the port then does not reseed either.
They include every distinct picture of the four title animations, Space
Impact's stars drawn from that seed. Space Impact's Top score page's two
frames show the 4075 the PMM dump holds and differ from the port's 0 by
design. The other frames, Snake II's menus and Level, Mazes and
Instructions pages, Pairs II's list of modes, menu and Level page, and
Bantumi's menu, Level page, Instructions and the start of its board's
slide among them, are equal to the pixel.

The rumble is checked the same way: `build/gb_run ... rumble 1400` prints
each frame the motor was on for, and `build/gba_shot` prints every switch
of the motor. In a scripted game left to be hit, both show a pulse of
about 15 frames, half the phone's (see Rumble).

## Game Boy

The Game Boy has a slow processor for this game, and the things the game
does every tick that the compiler makes too slow are the Game Boy's own,
in assembly (`platform/gb/draw.s`): putting a sprite into the picture,
finding what a shot has hit, and turning the picture into background
tiles, of which only those that changed are made again.

At 2x the terrain, which moves a column every tick from the second level
on, is not made into tiles at all (`platform/gb/strip.c`). Its rows of
the tile map hold tiles made once per level and the LCD scrolls just
those rows, between two cuts across the screen. Where anything else is
over the terrain, a ship or a shot, that cell gets a tile of its own made
from the picture, so what is shown is still the phone's picture to the
pixel; `make check-gb` holds it to that.

Snake II, Bantumi and Pairs II have no terrain, and at 2x all six bands
of their picture are made into twelve rows of tiles, of which only those
that changed are made again.

Bantumi's search for the phone's move, a hundred steps of a depth-first
search every tick, takes the compiler's code some 1.6 times the phone's
117 ms tick at level 5; the Game Boy has its own version of it in
assembly (`platform/gb/search.s`), which does the same steps on the same
nodes and keeps the phone's pace. The phone asks for a redraw on every
tick it thinks, when as a rule nothing has moved; the picture is drawn
again only when a sprite has changed.

When a tick still takes longer to show than the 93 ms between ticks, the
game keeps the phone's pace and shows fewer pictures: every tick is
played, not every one is drawn.

The ROM is 16 KiB banks on an MBC5 with 8 KiB of battery-backed RAM, seven
of eight banks used (`platform/gb/far.h` says what is where); MBC5 for
its rumble pin, see below. Space Impact's 7 KiB of data is
copied from the ROM to cartridge RAM at power-on, because both the game
and the sprite code, which are in different banks, read it. The save file
therefore holds a copy of that data next to the top score. Pairs II's
state, nearly 600 bytes for up to 60 cards, does not fit in the Game Boy's work
RAM either and is kept in cartridge RAM after that data, where Bantumi's
goes too, with its search's nodes.

## Sound

The sounds are read from the dump with the rest: each is a short run of
notes for the phone's buzzer, a pitch and a length in the units of the
phone's timers, 7.8 ms, with rests and repeats in Snake II's death. The notes are too short to be timed by screen
frames, two units for most, so both consoles keep a timer that interrupts
once a unit, and play the notes as a square wave on their second pulse
channel. A sound that starts while another plays takes its place, as on
the phone.

The pitches are the phone's, 440 Hz to 4186 Hz, as near as the pulse
channel's frequency register comes: within 3 Hz up to 1 kHz and within
1.2% above, 4228 Hz for the highest. The notes were compared with a trace
of the firmware's writes to the buzzer in MAME (the fork's
`docs/games_applications_3310.md`, Sounds) by recording the ROMs:

```
build/gb_run build/nokia3310-keys.gb BOOT_ROM 400 audio:shot.raw +a 4 -a 40
GBA_SHOT_AUDIO=shot.raw build/gba_shot build/nokia3310-keys.gba shot.bmp 360 0x1 300 304
```

Both write the left channel as raw signed 16-bit samples at 32768 Hz.
The phone plays no sound until Sounds in the games' settings and Warning
and game tones in the profile are both on; here Sounds starts on and is
enough.

## Rumble

Space Impact pulses the phone's vibrator when the ship is hit and when a
boss explodes, and Snake II at every meal and when the snake dies, for 62
of the phone's timer units, half a second, when Shakes in the games'
settings is on; a pulse that starts during another starts the half second
over. The port does the same with whatever motor the cartridge has:

- on the GBA, through the cartridge's general-purpose port as the rumble
  cartridges (Drill Dozer, WarioWare: Twisted!) use it: pin 3 of the
  registers at `0x80000c4`, left clear of code in the ROM header's tail.
  mGBA gives a ROM that port when its game code is a known rumble
  cartridge's, or through an override, `[override.N33E]` with
  `hardware=2` in its `config.ini`; `build/gba_shot` sets it up itself. The EZ-Flash Omega Definitive Edition has a motor and a Mode B
  setting of Rumble in its menu for cartridges that drive it this way;
  that is how the ROM is meant to rumble there, and has not been tried
  on the cart yet;
- on the Game Boy, through the MBC5's rumble pin, bit 3 of the RAM bank
  register, which is why the header now says MBC5. SameBoy's cartridge
  rumble follows it; the EZ-Flash Junior has no motor.

Shakes starts on, as on the phone.

The phone's motor is gentler than a rumble cartridge's, so the port
shortens every pulse: by default to half its length, the motor on
throughout. The knobs, shared by all the ports, are in
`../common/core/rumble.h`: RUMBLE_LENGTH_NUM / RUMBLE_LENGTH_DEN to
shorten every pulse, RUMBLE_ON_FRAMES and RUMBLE_OFF_FRAMES to run the
motor only some of the frames of one. On the Game Boy a game frame
sometimes takes two screen frames, so the latter is uneven there.

## Golden run

The core is checked against the firmware itself. `make golden` plays a
scripted game in MAME (`GOLDEN_KEYS` in the `Makefile`), keeps every LCD
frame and logs every event the firmware hands its Space Impact
(`tools/mame_event_log.lua`). `make check-golden` feeds the same events to
the core and requires every picture it draws to appear, in order, among
MAME's.

The recorded run covers the first 20 seconds of level 1: the shielded
start, the first waves, shots, a missile, kills and score. It reaches no
terrain, enemy fire, boss or level change, so those parts of the core
follow the firmware's code but have not been compared with it running.
The bosses were only checked by a bot that plays each level through on the
host.

Snake II is checked the same way against two games the firmware played
in MAME (`make golden-snake`, `SNAKE_GOLDENS` in the `Makefile`), steered
by the MAME fork's `mame_nokia_3310_snake2_probe.lua`, which takes the
snake to the food and the creatures and, after so many meals, into itself.
It steers by writing the game's next direction, and logs each write; the
replay hands the game the key that would have done it. The three games:

- level 5, no maze, 30 meals and a death after the snake's ring of
  segments has gone round, when the phone's dead snake does not blink;
- level 9 on Maze 2, 14 meals;
- level 2, no maze, creatures left alone, and a crash the snake is turned
  out of in its one short tick before it crashes for good.

Every picture of all three appears in order among MAME's: growth, the fat
swallowed piece, creatures and their countdown, eating one and letting
one run out, the wrap at the edges, walls, the saving tick, the death and
the blinking.

Pairs II is checked against four games (`make golden-pairs`,
`PAIRS_GOLDENS`) played by the MAME fork's `mame_nokia_3310_pairs2_bot.lua`,
which reads the board from RAM, steers the cursor the shortest way to the
other card of a pair, and opens a wrong card every so many pairs. The
phone draws a tick's change on the LCD up to 170 ms after the tick, so the
autopilot presses a key only once that change is shown and at least
130 ms before the next tick, as a key's change takes up to 100 ms; MAME's LCD then shows each change on its own:

- Time trial at level 1, all nine boards to the end of the game;
- Time trial at level 3, the time left to run out on the second board;
- Time trial at level 5, paused between boards and continued, which deals
  the next board;
- Puzzle at level 5, paused and continued, to the last pair.

Level 7's 100 ms tick leaves the autopilot no time between ticks, so it is
not among them.

Bantumi is checked against four games (`make golden-bantumi`,
`BANTUMI_GOLDENS`) played by the MAME fork's
`mame_nokia_3310_bantumi_bot.lua`, which picks pits with a seeded
generator of its own and walks the hand there with 4 and 6 before sowing
with 5:

- level 1 to the end of the game, asking for a hint every third turn and
  sowing where it puts the hand;
- level 2 to the end of the game;
- level 3, paused and continued;
- level 5, the phone's longest searches.

Every picture of all four appears in order among MAME's: the slide, the
hand's walks, sowing round the board and past the stores, captures,
extra turns, the hint, the phone's thinking and the moves it chose, the
end of the game with the rows swept into the stores and the winner's
digits flashing.

The frames and the event logs are derived from the firmware and stay in
the ignored `golden/`.

## Layout

- `core/` is portable C: `lcd` (framebuffer), `font`, `menu` (the phone's
  menus, its Settings pages and the full-screen ones), `title` (the games'
  title animations, drawn with the sprite layer's drawing but none of its
  sprites, so that a paused game's are kept), `sprite` (sprite
  list and tile layer), `si`, `si_setup` and `si_base` (Space Impact),
  `snake2` (Snake II, which draws straight into the sprite layer's
  picture), `pairs2` (Pairs II, both modes), `bantumi` (Bantumi and the
  phone's search), `game` (what the games and
  the menus share), `games` (keys and timers to game events, the settings
  the games follow and the vibrator's timer), `sound` (the notes of the
  sound in progress), `rand`.
- `platform/host/` has the tools above; `platform/gb/` is the Game Boy
  cartridge, and `../common/platform/gba/` the GBA's, which the 3410 shares.
- `tools/extract_assets.py` copies the games' data regions, their sounds, the
  fonts, the English text and the menus' pictures, the Top score page's
  and the Done note's animations among them, out of the dump; the game
  reads its data by firmware address.
- `tools/gb_run.c` runs a Game Boy ROM headlessly with scripted buttons,
  screenshots, sound capture, memory peeks and a profiler
  (`../common/tools/gb_profile.py`).
- The firmware maps the core follows are `docs/games_applications_3310.md`,
  `docs/games_snake2_3310.md`, `docs/games_pairs2_3310.md` and
  `docs/games_bantumi_3310.md` in the MAME fork.

`tools/export_fonts.py` and `tools/gb_audio.c` are unchanged copies from
the 3210 project and are not used yet.
