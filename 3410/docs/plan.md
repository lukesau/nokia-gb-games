# Plan

Where the 3410 port starts, from the static survey in the RE fork
(`docs/games_survey_3410.md`, reproduced by `tools/games_compare_3410.py`),
and the maps that followed it (`docs/games_applications_3410.md`,
`docs/games_snake2_3410.md`).

## Games

| Game | Kind | Starting point |
|---|---|---|
| Space Impact | native, adapted from the 3310 | 3310 port's code; new y-paths, level data and title art from the 3410 image; logic to be checked function by function |
| Snake II | native, adapted from the 3310 | ported; see README.md |
| Bantumi | native, adapted from the 3310 | 3310 mapping; pit table at `0x4beccc`; title pictures unchanged |
| Bumper | native, new (pinball) | needs a full map; handler `0x2d6d1c` |
| Link5 | native, new | needs a full map; handler `0x32ae6e` |
| Munkiki's Castles | Java (MIDP, Nokia UI API) | ported: decompiled and re-implemented in C, checked against the JAR in a harness; see README.md and docs/munkiki.md |

Pairs II is not on the 3410. Handlers and how the games are reached are in the fork's `docs/games_applications_3410.md`.

## Screen

96 x 65. The Game Boy (160 x 144) shows it whole at 1x; at 2x 16 columns
do not fit. The GBA (240 x 160) shows it whole at 2x (192 x 130).

## Steps

1. Done: the games' handlers and how they are reached (MAME and Ghidra).
2. Done: the core and the Game Boy layer at 96 x 65, the 3410's menus,
   and Snake II, checked against the phone frame for frame.
3. The main menu's Games icon; the sounds checked against the phone.
4. The GBA layer at 2x.
5. Done: Bantumi, Space Impact and Munkiki's Castles. Then Link5 and
   Bumper.

## Known setup issue

The RE fork's local MAME tree is behind its patch list, so `make overlay`
and anything that rebuilds MAME fail. Runs work with `make run-prebuilt`
after copying `roms/noki3410/` into `mame/roms/noki3410/`; the fork's
`make run-keys GAMES_PRODUCT=3410` does both.
