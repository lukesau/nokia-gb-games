# Nokia 5110 games for Game Boy and Game Boy Advance

The Nokia 5110 (NSE-1 v5.28) built-in games: Memory, Snake and Logic, as a
Game Boy ROM (`.gb`) and a Game Boy Advance ROM (`.gba`).

The 5110's games are the 3210's. Its firmware has the same game framework,
the same tiles, speeds and boards byte for byte, the same fonts and the
same English text, so this port has no code of its own: it builds the
games and menus in `../common/classic` that the 3210 port builds too, with
the 5110's list of games (`core/phone.h`) and its data read from your
dump. Logic, which the 3210 carries but never offers, is in the 5110's
menu, and so it is here.

Not yet: the 5110's own menus where they differ from the 3210's. MAME does
not reach them on v5.28, so they are being mapped from the firmware; until
then the main menu and the Top score page are the 3210's without its
animations.

## Checked against the phone

MAME does not run v5.28, so Logic is checked against the 5110's own Logic
run natively in the fork's Unicorn harness (branch `games/re-5110`,
`tools/dct3_5110_game_harness.py`). `make golden-logic` records its frames
into the ignored `golden/`, 356 of them over all eight levels: games of
random keys past the ten tries' scroll, and a game per level that types the
secret in (the firmware ends it, posting its game-over status). `make
check-golden` replays them on the port, which draws the same to the pixel.
`DCT3_RE` names the fork's checkout and `HARNESS_PYTHON` a Python with
unicorn.

## Building

```sh
make gb    # build/nokia5110.gb
make gba   # build/nokia5110.gba
```

`DUMP` defaults to `../../../roms/noki5110/5110f528a.fls`. The other
targets are the 3210's (`make help`), plus `make golden-logic`.

## Firmware

No firmware or anything derived from it is committed here. I got mine from
the Internet Archive's Nokia DCT3 collection,
<https://archive.org/download/Nokia_DCT3_firmwares/Nse-1_v5.27-5.28.exe>,
a self-extracting zip holding a Wintesla package; the fork's
`tools/extract_dct3_wintesla.py` turns it into the flash image
(`--mcu nse1nx05.280 --ppm nse1nx05.28a`, SHA-1 `a623b59d…b602`).
