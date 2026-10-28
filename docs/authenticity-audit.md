# Authenticity audit: the ledger

The working file behind `authenticity.md`: one row per feature with the
firmware address it was checked against, the mismatches found, and what to
trace next. Read `authenticity.md` first; this is for whoever does the next
audit.

Audited 2026-10-05 across the three ports. The question asked of every
feature: was it re-implemented from the firmware's code, read from the
firmware's data, or copied from what the phone does in MAME? The golden
checks cannot answer it, since they only compare pictures, so this file does.

Four sources:

- **traced**: the C follows a firmware routine read in a decompile or the
  disassembly. The routine's address is given; the maps in the fork's
  `docs/games_*.md` describe most of them.
- **data**: read from your dump at build time by the port's
  `tools/extract_assets.py` or `export_fonts.py`: pictures, fonts, text,
  tables, sound scripts.
- **measured**: watched in MAME (LCD frames, event, sound and vibrator logs)
  without the code that does it being traced. Right as far as the runs go.
- **port**: the port's own, with no firmware counterpart, or a deliberate
  difference.

The Golden column says what checks it against the firmware: *frames* (a
recorded MAME run that the port's frames must appear in, in order),
*menus* (`golden/menus/`, frames that must be equal), *sounds* (the
recorded runs' sound and vibrator calls, which the replay's must equal, call
for call), or *no*. None of them compares timing, and only the 3410's Space
Impact compares sounds and rumble with MAME.

## In short

- **The games themselves are traced.** Snake, Memory and Rotation on the
  3210, all four 3310 games and the 3410's Snake II follow their handlers
  rule for rule. The exceptions are a few timings and the sound scripts'
  commands.
- **Everything around the games is measured.** That covers:
  - the menu pages, the Level bars and the Instructions paging;
  - the Settings pages, the Done note and the Games icon animation;
  - the Top score page and its sparkle;
  - the Game over pages, their 385-tick close and the "score > top" rule;
  - the fireworks' sequence.

  The firmware's widget and layout code and its post-game code (message
  `0x5133`/`0x5134` on the 3310) were never traced. The menus were meant to
  be pixel copies, but the post-game rules are guesses that only look right.
- **The pictures, fonts, text and tables are data.** A few come from
  addresses that appear only in the extractors, not in the maps: the 3210's
  Top score sparkle `0x2c83fd`, Memory's board sizes `0x2d9724`, its FONT
  and TEXT chunks; the 3310's sparkle `0x2faa40`; the fireworks `0x2fe6a8`,
  found by searching the image for captured frames.
- **Large traced parts have never been compared with a run**, for example
  Space Impact past tick 240 of level 1 (its terrain, enemy fire, bosses,
  continue screen and level exits), Snake II's large creature, and some
  mazes and levels. They are right by reading, not by a golden.

## 3210

The map is `docs/games_applications.md` in the fork. Its Evidence cells for
Memory's and Rotation's handlers are empty and the Ghidra decompiles are
cut short at the key jump tables, so much of the tracing below was done in
the disassembly (`arm-none-eabi-objdump -EB -M force-thumb`, base
`0x200000`) and is not yet written into the map.

| Feature | Source | Evidence | Golden |
|---|---|---|---|
| `rand` | traced | `rand_2b64d4`, seed `0x11250c` | frames (seeded) |
| The seed when a game starts | port | the phone never seeds it; the port uses uptime. Goldens use `0x7fffffff`, seen in MAME | fixed seed |
| Snake: the start (9 cells at (0, rows-1) heading right), ring size, first food at the middle | traced | disassembly `0x240efe`..`0x2410e2` | menus |
| Snake: speed table and the move delay | traced + data | table `0x2d9738`, constant at `0x24115c` | no |
| Snake: keys 2/4/6/8, a reversal ignored | traced | jump table at `0x240db4` | no |
| Snake: keys 1/3/7/9 | traced, unreachable | `0x2410f0`..`0x241144`; no console button | no |
| Snake: collision and the 20-tick grace move | traced | `snake_check_move_2413dc`; the 20 ticks in disassembly `0x240eac` | menus |
| Snake: head, tail, growing, score level+1 | traced | `snake_move_head_2414dc`, `snake_advance_tail_241620`, `0x240e2e` | head and tail yes; eating no |
| Snake: food placement | traced | `snake_place_food_2416f6` | no |
| Snake: the board full (+100, game over) | traced, **not implemented** | `0x2414dc` | no |
| Snake: drawing | traced + data | `snake_draw_241198`, food `0x2d9744` | menus |
| Memory: board sizes, deal, keys, scoring, mismatched pair turned back | traced + data | `memory_handler_24075c`, `memory_deal_board_240bd8`, `memory_move_cursor_240c88`, sizes `0x2d9724` | frames (`*` key no) |
| Memory: drawing | traced + data | `memory_draw_board_240a4c`, tiles `0x2d94bc`, back `0x2d96c4` | frames |
| Memory: the cursor's blink every 64 ticks | measured; the plane is traced | attr `0x21` in `0x240a4c`; the port inverts one card instead of a plane | phase only |
| Rotation: init, keys, turn phases, opening turns, solved test, score, clock | traced | `rotation_handler_241fd0`, disassembly `0x241fda`..`0x2423e2`, limit `0x24243c` | frames |
| Rotation: drawing | traced | `rotation_draw_board_242494` | frames |
| The 3x5 digits | traced + data | `games_draw_number_2406b6`, glyphs `0x2d96ec` | frames |
| `fill_rect`, `blit_bitmap` | traced | `lcd_fill_rect_25eec0`, `lcd_blit_bitmap_25f3b6` | frames |
| The eat and pair beep (2637 Hz) | traced pitch, measured length | tone `0x10` | no |
| Rotation's solved notes | traced | tone `0x13`; lengths equal the tone's ticks | no |
| Game over sound (3 x 440 Hz) | measured | tone `0x11`'s script is mapped, but the code that posts it is not traced; lengths from MAME | no |
| New top score tune | measured | MAME buzzer trace; no tone id in the map | no |
| Menu pages, scrollbar, path, Level bars, Instructions paging | measured + data | only the path string (`0x26281e`) and the scrollbar formula are traced | menus |
| Games icon animation (140 / 24 ticks) | data + measured | pictures `0x2c50d4`; "code not found" | menus |
| Top score page: sparkle steps of 25 ticks, closes after 768 | data + measured | sparkle `0x2c83fd` | menus |
| Game over page: text, closes after 385 ticks or a key | measured | `game_over_score_screen_29a2a0` not traced | menus |
| New top score when score > top | port | the record's writer not traced | no |
| Last view and how it is dismissed | measured | | no |
| Pause and Continue | measured | `menu_return_263468` only noted | menu yes, the resume no |
| Settings record | traced shape + port | NV `0x074c`; the check byte and signature are the port's | no |
| Full-screen mode, `_draw_changes`, packed framebuffer, test card | port | `make test-snake`, `make test-boards` prove the fast paths | no |

## 3310

The maps are the fork's `docs/games_applications_3310.md` (framework and
Space Impact), `games_snake2_3310.md`, `games_pairs2_3310.md` and
`games_bantumi_3310.md`. Decompiles of every Space Impact routine and of
the sprite and tile engines are in the fork's ignored `run_games_3310/`.

### Framework, menus and settings

| Feature | Source | Evidence | Golden |
|---|---|---|---|
| Events, return codes, timers, key repeat of 12 units | traced | `games_app_handler_2dbdd4` | frames |
| ms to units (x 32/255) | traced | map "Timing" | — |
| The unit's length | **3210's value** | 7.781 ms; MAME measures 7.74 ms on the 3310 and the static value is 7.969 ms. Never settled | no |
| `game_rand16`, ANSI `rand` | traced | `0x2dd7d0`, `0x2f1b44` | frames |
| Seeding from uptime at the title and New game | port | the phone reseeds from the clock only when it is set | fixed seed |
| Sprite list, layers, modes 0-6 | traced | `0x2dda1c`, `0x2dda8c`, `0x2dd9da` | frames |
| Full redraw instead of the dirty-rectangle render | port | `sprite_render_2dfd88` not decompiled | frames |
| Mode 5 (the blink plane) | port, drawn steady | | — |
| Terrain strips and pixel collision | traced | `tilemap_render_2e6e98`, `0x2e7036` | no |
| Main menu, Games list, path numbering | measured + data | | menus |
| Games icon (0.84 s, then 12 steps of 0.155 s) | data + measured | `0x2f6c68`; stepping code "not found" | menus |
| Settings pages, Off/On list | measured + data | | menus |
| Done note pictures and timing (0.70 / 0.92 / 1.47 s) | data + measured | `0x2f9e50` | menus to 1.0 s |
| Sounds switch | traced; default On is port | `0x111505` in `sound_play_2ec8ca`; the phone starts Off | no |
| Shakes switch | traced | `0x111506` in `game_vibrate_2dd70e` | no |
| Lights | port, a no-op | `0x111504` only feeds a trace id | page only |
| Vibrator, 62 units restarted by each call | traced + measured | `game_vibrate_2dd70e`; the phone's vibra/profile/charger checks are left out | no |
| Rumble pulses half as long | port, on purpose | knobs in `common/core/rumble.h` | no |
| Buzzer notes | data | scripts at `sound_table_321e6c` | no |
| Buzzer pitch, note unit, 0x40 rest, 0x05/0x06 repeat | measured | the tone task is not traced | no |
| Instructions paging | data + measured | | menus |
| Top score page, sparkle `{0..9,9,10,10,...}` at 25 ticks, closes at 768 | data + measured | `0x2faa40`; page code `0x298710` not traced | menus to 1 s, digits differ by design |
| Score > top and its save | port | the code after message `0x5133` is not traced | no |
| Game over page, 385 ticks | measured | the map says about 2.95 s (379 ticks) | Snake II menus |
| Fireworks pictures | data | `0x2fe6a8`, found by searching the image for captured frames | no |
| Fireworks sequence (twice, 233 ms, sound `0x23`, keys ignored) | measured | drawing code not traced; the port starts them about 0.1 s early | no |
| Settings, top scores and levels in cartridge RAM | port | | no |

### Space Impact

All traced against decompiles; the port's functions match them in
substance. What is not:

| Feature | Source | Evidence | Golden |
|---|---|---|---|
| Title: stars, logo halves, ship, 210 / 700 ms | traced summary + measured | `0x2d7538`..`0x2d76f6` (no decompile on disk); the three swap positions are measured | menus |
| Menu with Continue | measured | | menus |
| New game, level load, HUD, ship spawn and shield | traced | `0x257b48`, `0x2578de`, `0x257764`, `0x257a18` | frames |
| Ship, fire, missile | traced | `0x257e1c`, `0x258f44` | frames |
| Wall and beam | traced | `0x259534`, `0x2596a0` | no |
| Spawn script, groups, random rows | traced + data | `0x258508`, lists `0x311820`, headers `0x312084` | entries 0-8 of level 1 |
| Movement patterns | traced from the map's table | the decompile of the switch at `0x259718` is cut short | pattern 6, and 1 and 2 entering |
| Enemy fire | traced | `0x25865a`, `0x2586b2` | no |
| Collisions, damage, scoring | traced | `0x259d30`, `0x259a60`..`0x259cfe`, `0x2593a4` | shots on ordinary enemies, +5 and +10 |
| Bonuses, score cap 31500 | traced | `si_award_2592b0` | unclear |
| Lives, continue screen (5 at 800 ms) | traced | `0x258114`, `0x257ce0`; 800 ms never measured | no |
| Bosses of levels 1-7 and the final boss | traced | `0x258b0c`, `0x258882`, `0x258a04`, `0x258808`, `0x258400`, `0x2588c8`, `0x258c68`; the map only summarizes them | no (host bot only) |
| Boss explosions, level exit, end after level 8 | traced | `0x2593a4`, `0x2596a0`, `0x25a200` | no |
| Pause: the port resumes exactly, without the 1.5 s shield | port | `si_resume_2581f8` traced, not ported | no |
| Top score starts at 0 | port | the phone's 4075 is left over in the PMM dump | the 2 failing menus |

### Snake II

| Feature | Source | Evidence | Golden |
|---|---|---|---|
| Board, frame, offsets | traced | `snake2_board_dims_274296`, `snake2_walls_draw_274f0c` | frames |
| Mazes | data + traced | records `0x328108`, walls `0x327fe4`, `snake2_maze_build_2750dc` | No maze and maze 2; 1 and 3 seen in MAME; 4 and 5 never run |
| Speeds | data + traced | `0x3280fc` | levels 1, 2, 5, 9 |
| New game, keys, tick order, collision, edge wrap, grace tick, growth | traced | handler `0x275aa4`, `snake2_new_game_2762e0`, `snake2_blocked_2753f2` | frames |
| Head, neck, corners, fat piece, tail | data + traced | `0x274b98`, `snake2_tail_remove_274844`, pictures `0x327e64`..`0x327fb4` | frames |
| Food placement, small creature spawn, countdown, eating it | traced | `snake2_food_place_27436e`, `snake2_creature_spawn_2744a8`, `snake2_draw_countdown_27421c` | frames |
| A failed creature spawn | traced | never reached in MAME | no |
| Large creature | traced, unreachable | `0x2744a8`, `0x2754b6`; the port's collection mask is 0 | no |
| Death and its blinks | traced + measured | handler event 0 | frames |
| Game over rumble kept as a full pulse | port, on purpose | the phone cuts it after 0.28 ms; what cuts it is not traced | no |
| Sounds `0x1f`, `0x20`, `0x22` | data + measured | | no |
| Resume | traced period + port | the suspend packing (`0x27553c`, `0x275788`) is not ported | no |
| Level, Mazes, the "selected" note, Instructions | measured + data | "described from screenshots only" | menus |

### Pairs II

| Feature | Source | Evidence | Golden |
|---|---|---|---|
| Deal, shuffle, board shapes, times, Puzzle grid | traced + data | `pairs2_deal_2d9c54`, `pairs2_shuffle_2d9a94`, tables `0x32fe74`..`0x32fee4`, `pairs2_layout_puzzle_2c6478` | frames (Puzzle at level 5 only) |
| Cards dealt, saloon door and wipe, cursor, opening, enlarged pictures | traced + measured | `pairs2_deal_step_2d9aea`, `pairs2_tick_2c6740`, `pairs2_show_second_2c6b70` | frames |
| Scoring, fuse, explosion, bonus, Puzzle removal | traced | `pairs2_event_2d9ee0`, `pairs2_fuse_step_2c66ba` | frames |
| Level periods 300..100 ms | traced | `0x2c6ab8`; only 300 and 100 measured | no |
| Resume between boards, and in Puzzle | traced + measured | `pairs2_resume_2d9d68` | frames |
| Resume during play in Time trial | traced only | the port remakes the flame in mode 4 on layer 1, the map says mode 1 on layer 0 | no |
| Mode list, Level page, records per mode | measured | the record's +4 is inferred | menus (Time trial) |
| Text 576 "new top score" after Pairs II | inferred | | no |

### Bantumi

| Feature | Source | Evidence | Golden |
|---|---|---|---|
| New game, slide in, pits, digits, cursor, sowing, captures, extra turn, end | traced + measured | `bantumi_new_game_2b1c10`, `bantumi_set_pit_2b1764`, `bantumi_pick_2b1ad0`, `bantumi_turn_end_2b1898` | frames |
| Hand paths and drawing | traced | `bantumi_hand_path_2b1a04`, `bantumi_draw_hand_2b1520` | frames |
| Hand widths 12 and 11 | measured | the RAM descriptors were not read | frames |
| The phone's search: depth, 100 steps a tick, move order, its two mistakes, the hint | traced + measured | `0x2dd3bc`, `0x2dd5ac`, `0x2dd438`, `0x2dd28e`; 112 of 112 searches modeled | frames (level 4 never run) |
| The search in Game Boy assembly | port | `common/bantumi/search.s` | only against the host |
| Winner's store blinking | traced | | frames |
| Loser's store | port, drawn steady | the phone blinks it through mode 5 | — |
| Game over pages (won, lost, draw) | data + measured | message `0x5134`'s handler not traced | no |
| Level page, Instructions, no Top score | measured + data | | menus |

## 3410

The maps are the fork's `docs/games_snake2_3410.md`,
`games_applications_3410.md` and `games_survey_3410.md`. Space Impact is
ported from `games_si_3410.md`; of it, only the sounds and the vibrator
are listed here.

| Feature | Source | Evidence | Golden |
|---|---|---|---|
| Menu pages, list geometry, scrollbar, Level bars | measured + data | no menu routine in `3410.csv` | menus |
| Main menu's Games icon | missing | | **`menu-` fails by 311 px** |
| Games page, Settings pages, Instructions layout | port, carried over from the 3310 port | the 3410's pages are not mapped | no |
| Title animation | traced + data | `0x24b484`, `0x24eb2c`, `0x4974a0`..`0x498700` | menus |
| Board, offsets | traced | `0x24b82e`, `0x24b874` | frames |
| Mazes | data + traced | `0x497458`, `0x4973b8`, `0x24ba68` | No maze and Tunnel only |
| Speeds | data + traced | `0x4bef64`, `0x24e5dc` | no |
| New game, tick, collision, grace, head, tail, food, creatures, scoring, death | traced | `0x24ce0c`, `0x24e5dc`, `0x24e1d8`, `0x24c6ac`, `0x24ca24`, `0x24cf68`, `0x24e41c`, `0x24c094`, `0x24e9d8` | frames |
| Keys | **3310's tracing** | the 3410's `0x24f8ec` not checked | no (the replays inject directions) |
| Framework events and timer unit | **3310's tracing** | the 3410's own events and `0x3b2546` not traced | indirectly |
| `rand` | measured | `rand_3f903c` named, not decompiled | frames |
| Game over picture and box | traced + data | `0x24b77e`, `0x24b5c8` | menus |
| Game over blink and length (422 units) | measured | the traced code counts 30 x 100 ms, about 0.4 s less | no |
| Game over sounds `0xfa2` / `0xfa4` | traced | `0x24b77e`; codes `0x21` / `0x23` as below | no |
| Top score per maze | traced | `0x24b4ec` | no |
| High scores page | traced + data | `0x24d204`, `0x24f63c`, `0x3b268a` | menus (first box only) |
| Death vibration | traced on/off points, port length | `0x3b25d4`; the port's 141 units are a round figure | no |
| Eat and death sounds | traced + data | `0xfa0`/`0xfa1` are `0x1f`/`0x20`: `0x3f7d0e` maps an id through the halfwords at `0x4c3538`, and the tone task indexes the table from `0x4a9060`, 3 records early | no |
| Space Impact's sounds: shot, wall or missile, beam, bonus, ship lost, game over or new top score | traced + data | `0x3b2510` calls, ids `0xfa5`, `0xfa6`, `0xfa8`, `0xfa0`, `0xfa7`, `0xfa2`/`0xfa4` mapped as above | sounds |
| Space Impact's vibrator, three ticks | traced | `0x3b25d4`; on and off at the phone's ticks, the motor's pulses as in `rumble.h` | sounds |
| Space Impact's Instructions demos: firing, the keys on a keypad, a bonus | traced + data | `0x259a90`, `0x25c6d4`, `0x259d30`; the bonus chapter `0x4ad390`, the keypad `0x4b4a80` and pointer `0x4b4a98` | frames, sounds (`golden/si-i`) |
| Instructions pages around the demos: More or Down past a text's last page starts its demo, a demo's end or any key leads to the next text, C back to the menu, a fourth text with Exit only, Up and Down page | measured | the games framework that sends event 9 and turns keys into event 3 is not traced; Up during a demo was once seen to be ignored | no |
| Full-screen mode, cart-RAM saves, Lights | port | | no |

### Bantumi (3410)

The map is the fork's `games_bantumi_3410.md`; the code is the 3310's
(`common/bantumi/bantumi.c`) with the 3410's drawing.

| Feature | Source | Evidence | Golden |
|---|---|---|---|
| Rules, hand, sowing, captures, the search and its mistakes | traced | `0x2e9340`, `0x2e91c0`, `0x2e92b4`, `0x2e8b84`, `0x3b8180`..`0x3b8438` | frames (`golden/bantumi-a`..`d`, levels 1, 2, 3, 5) |
| Board, digits, hand through its mask, thinking picture, draw order and modes | traced + data | `0x2e899e`, `0x2e8830`, `0x2e879c`, `0x2e8718`; the hands' RAM descriptors read in MAME | frames |
| Intro, game-over pause, blink, closing and slide out | traced | `0x2e9340`; the blink's power-on counter (5) read in MAME | frames |
| Pass on the first turn (#), hint (*) | traced | `0x2e9340`, `0x2e8d54` | frames (`bantumi-a`) |
| Pause and Continue, still until a key | traced + measured | `0x2e9118`, `0x2e8dc4`; the framework's New game before Continue and the held timer seen in MAME | frames (`bantumi-c`) |
| Sounds: bean, game over won or lost each pause tick | traced + data | `0x3b2510`, ids `0xfa9`, `0xfa4`, `0xfa2` | sounds |
| Title | traced + measured | `0x3eb790`, `0x3eb6d4`; the small picture's places read in RAM | frames (`bantumi-t`) |
| Instructions demos | traced + data | `0x2e8fbc`, `0x2e9a10` | frames, sounds (`bantumi-i`) |
| Instructions pages, the first again after the fifth demo | measured | the framework is not traced | menus (first text) |
| Menu, Game options, No. of beans, Level page | measured + data | strings 130..133, 750 | menus |
| Redraw after every event but a closing one, a demo's first and Menu | measured | the framework is not traced | frames |
| The search in Game Boy assembly | port, the 3310's | `common/bantumi/search.s` | only against the host |

## Mismatches found

Places where a comment, README or map claims more than the evidence, or
the C departs from the firmware. None changes play today except where
noted.

**3210**

- The map's row for `0x240d82` and the empty Evidence cells for Memory and
  Rotation don't record the disassembly the port was checked against:
  - the corner keys;
  - the 20-tick grace;
  - the start and first food;
  - Rotation's keys, solved test and score.
- `core/sound.h` says the lengths are "corrected for MAME's fast clock".
  The eat and game over lengths are raw MAME measurements; only Rotation's
  solved notes match the firmware's ticks.
- Memory ends with status `0x13` or `0` on the phone, and the port treats
  them alike; texts 303 and 305 are extracted but unused.

**3310**

- `core/games.h` takes the 3210's 7.781 ms unit; the 3310's is not
  settled.
- `menu.c` calls level 9 and No maze "the phone's own default". It is the
  PMM dump's saved record, and the port's real default is level 1.
- `extract_assets.py` states the 0x40 rest and 0x05/0x06 repeat as facts;
  the map calls them inferred.
- The README says Snake II is checked against "two games"; the Makefile
  has three.
- The README says Snake II has "the same rumble on every meal and at the
  end". The end pulse is the port's choice; the phone cuts it.
- The README says the ship rumbles "when hit and when a boss explodes". It
  also rumbles on each re-roll of the last level's explosions.
- The flame on a mid-play Time trial resume (`pairs2.c:703`) disagrees
  with the map, and no run settles which is right.
- Bantumi's `winner_mode` carries over between games, as the firmware's
  does, but its power-on value is a guess.
- The maps leave out a few things the C traced from decompiles:
  - Snake II's head step removing the tail at head+1 == tail;
  - its first food at (w/2, h/2);
  - the step-by-step bosses of Space Impact.
- `make check-menus` always fails on Space Impact's Top score digits (by
  design), which would hide a real regression there.

**3410**

- `make check-menus` fails on `menu-` (the missing Games icon), and "frame
  for frame" in the README and map overstates the replays: they check
  order, not timing, and inject directions instead of keys.
- `play_over` skips the `rand()` call `0x24b4ec` makes at every game over,
  so a second game under a fixed seed leaves the phone's sequence.
- The last game's record is kept in RAM; the phone stores it in NV.
- The title ends on any key but Back; the map says only Navi.
- A 3310 comment in `way()` about an off-screen first segment does not
  apply on the 3410.
- `export_fonts.py` labels the 3410's fonts as the 3210's and names them
  `nokia3310_*`.
- `sprite.c` and `game_rand16` are compiled in and unused.

## What to trace next

In order of how much of the ports each one would move from measured to
traced:

1. The post-game code on each phone: the 3210's
   `game_over_score_screen_29a2a0`, and the handlers of the 3310's messages
   `0x5133`/`0x5134`. That would settle:
   - the score comparison and save;
   - the Game over pages and their 385 ticks;
   - the fireworks and their sound;
   - the Top score page with its 768 ticks and sparkle;
   - the 3210's Last view.
2. The games menu framework around `0x298xxx` (3310) and `0x26xxxx`
   (3210): the list, Level page, Instructions paging, Settings pages, Done
   note and the Games icon stepper. The pixels are already checked, so this
   only matters for authenticity.
3. The tone task: the script commands and the note unit.
4. The 3310's real timer unit, and the 3410's keys (`0x24f8ec`), events
   and timer.
5. Golden runs for traced code that no run has reached:
   - Space Impact past the first 240 ticks, its bosses above all;
   - the 3210's Snake eating;
   - Snake II mazes 4 and 5 and the 3410's other mazes;
   - Pairs II mid-play resume;
   - Bantumi level 4.

## The 3210's code structure

From the first version of these notes (2026-10-01, 3210 only): where the
3210 core's structure differs from the firmware's, though the output is
the same. The standard aimed at: `core/` mirrors the firmware function for
function, and every console workaround lives in that console's
`platform/`.

1. **Event interface.** The firmware's games are one handler each, taking
   an event code and re-arming their own timer. The port has
   `snake_init`, `snake_key` and `snake_step`, driven by `core/menu.c`.
   `core/game.h` declares a handler interface that nothing uses. Still to
   do: a `snake_handler(event)` that asks the platform for its next tick,
   called through a table by game as the firmware's plugin table does. The
   3310 and 3410 ports already work this way.
2. **Framebuffer layout.** The phone keeps one byte per column in strips
   of eight rows; `common/core/lcd.c` packs rows, 11 bytes each, for the
   Game Boy's sake. Either transpose 8x8 cells in the Game Boy layer or
   say plainly that the layout is the port's.
3. **Whole-board redraw.** The firmware redraws the board every tick
   (`snake_draw_241198`). The Game Boy cannot at level 9, so
   `snake_draw_step` draws only what changed, and `make test-snake` proves
   the two agree. Still to do: the GBA should use the plain redraw;
   `draw_play` uses the fast path everywhere.
4. **Dirty cells.** `lcd_dirty` is the port's LCD driver interface; it
   could move behind a platform hook.
5. **Menus.** Drawn directly as the same pixels, not through widgets.
   Nothing structural to do.
6. **Timing.** The core still assumes 60 frames a second
   (`MENU_TICKS_PER_SECOND`) where both consoles run at 59.73 Hz.
7. **Settings.** The firmware's 4-byte records, with a check byte in the
   unused position; the check byte is the port's.

Memory and Rotation follow their handlers rule for rule, with these
differences:

- Each has a `_draw_changes` beside its `_draw` (`make test-boards`).
- Memory's blinking card is inverted by `memory_draw` instead of drawn
  into a second plane.
- The port seeds `rand` at New game, where the phone's first deal after
  power-on is always the same.
- Rotation's 7, 9 and 5 duplicate 1 and 3 and have no button.

Since 2026-10-01 these have been done:

- the eating beep (pitch traced through tone `0x10`);
- the Game over page's 385-tick close and any-key close (measured).

These are still open:

- the board-full end;
- the seed at New game;
- the Last view's dismissal;
- golden frames of eating.

Drawing speed and screen updates on the Game Boy: `lcd_column_fill` and
`lcd_column_blit` are replaced by assembly in `platform/gb/crt0.s`, and
the 2x view converts only changed tiles. Whole-screen changes are copied
with the LCD off, which blinks; writing into a second tile set over
several vertical blanks would remove it.
