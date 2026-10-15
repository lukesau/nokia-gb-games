# Next release: the 3410 and the 5110

The next release adds the Nokia 5110 (NSE-1 v5.28) and finishes the 3410
(NHM-2 v5.46). Both build on the shared code in `common/` wherever the
phones agree, so a fix lands once.

## Nokia 5110

The 5110's games are the 3210's: the same game framework and event codes,
the same tile, card, cursor, board-size, speed and food tables byte for
byte, the same four fonts and the same English text (at other indices).
It offers Memory, Snake and Logic, in that order; Rotation and React are
not on it. Logic is the game that is in the 3210's firmware but never
reached; on the 5110 it is in the menu, so it is in the port.

| | Where in v5.28 |
|---|---|
| game table | `0x2a79cc`: Memory, Snake, Logic |
| tiles, card back, cursor | `0x2a79ec`, `0x2a7bf4`, `0x2a7bfc` |
| Snake food, speeds; Memory boards | `0x2a7c20`, `0x2a7c3c`; `0x2a7c48` |
| FONT chunk | `0x2b0414` (the 3210's fonts) |
| TEXT chunk, ENGL block | `0x2b4f28`, `+0x288` |

The fork's Unicorn harness (`tools/dct3_5110_game_harness.py`, branch
`games/re-5110`) runs the three games natively and gives frames to check
against. MAME stops at CONTACT SERVICE on v5.28, so the menus have to be
mapped from the firmware rather than measured.

Steps:

1. `common/classic/`: Snake, Memory, Logic, their rand and sounds, and the
   3210's menus, with a small table per phone for what differs (the games,
   their order, the Games icon). The 3210 builds from it and stays equal
   to its goldens.
2. `5110/`: the extractor's table for v5.28, the game list, a GB and GBA
   build.
3. Logic: the 3210 port's implementation of its dormant Logic, checked
   against the 5110 running it in the harness.
4. The 5110's own menus: what differs from the 3210's (main menu entry and
   icon, the Top score page), from the firmware.

## Nokia 3410

Done: Snake II and Space Impact, the menus, sounds and rumble, on the Game
Boy. Left:

1. Bantumi, shared with the 3310's: the 3410's is the 3310's game on a
   96x65 screen (pit table `0x4beccc`, title pictures unchanged).
2. What the 3310 and 3410 share moved into `common/`: rand, sounds,
   sprite, the GB start-up, cart RAM saves, the GBA start-up.
3. The GBA build at 2x.
4. Link5, then Bumper: new games, mapped from the firmware first.
5. Leftovers: the Games icon, the Chapters note's tick, the lists' sliding
   highlight.

Munkiki's Castles is Java and stays out of this release.
