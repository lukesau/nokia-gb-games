#!/usr/bin/env python3
"""Record the 5110's own Games menus as golden frames, from the fork's menu harness.

Usage: golden_menus.py DCT3_RE DUMP OUT_DIR

MAME does not run v5.28, so the pages come from the phone's own menu
engine, games applications and fonts run in
DCT3_RE/tools/dct3_5110_menu_harness.py (the fork's
docs/games_menus_5110.md). Each page is written under the name of the host
frame that must equal it: the port's key presses from its first screen to
that page. Derived from the firmware: keep OUT_DIR ignored.
"""
import sys
from pathlib import Path

W, H = 84, 48
MEMORY, SNAKE, LOGIC = 0, 1, 2
SEED = 0x5110

# Logic's secret for SEED at level 0, from the runtime's rand; the game
# harness's goldens check the same.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from golden_logic import secret  # noqa: E402


def pages():
    """(port keys, harness method, arguments) for every page."""
    out = [("", "main", ())]
    for g in (MEMORY, SNAKE, LOGIC):
        games = "s" + "d" * g
        menu = games + "s"
        has_top = g != LOGIC
        out += [
            (games, "games", (g,)),
            (menu, "menu", (g,)),
            # The harness selects by position: Level 1, New game 2, ...
            (menu + "u", "menu", (g, "none", 1)),
            (menu + "d" * (2 if has_top else 1), "menu", (g, "none", 4 if has_top else 3)),
            # Back during play, back to the list and the game again: its
            # menu opens with Continue.
            (menu + "sccs", "menu", (g, "continue")),
            (menu + "d" * (2 if has_top else 1) + "s", "instructions", (g,)),
        ]
        if has_top:
            out += [
                (menu + "d", "menu", (g, "none", 3)),
                (menu + "ds", "top_score", (g, 0)),
            ]
        levels = (5, 9, 8)[g]
        for level in range(levels):
            out.append((menu + "us" + "u" * level, "level", (g, level)))
    # Snake run into the wall at score 0, and a solved Logic game.
    out.append(("sdss" + "t" * 40, "game_over", (SNAKE, 0, 0, 0x11)))
    code, _ = secret(SEED, 0)
    solve = "sddss" + "".join("s" * f + "d" for f in code) + "a"
    out.append((solve, "game_over", (LOGIC, 0, 0, 0x13)))
    out.append((solve + "w", "menu", (LOGIC, "lastview")))
    return out


def write_pgm(path, fb):
    pixels = bytes(0 if fb[y][x] else 255 for y in range(H) for x in range(W))
    path.write_bytes(b"P5\n%d %d\n255\n" % (W, H) + pixels)


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    dct3_re, dump, out = Path(sys.argv[1]), sys.argv[2], Path(sys.argv[3])
    sys.path.insert(0, str(dct3_re / "tools"))
    from dct3_5110_menu_harness import MenuHarness

    out.mkdir(parents=True, exist_ok=True)
    for keys, page, args in pages():
        h = MenuHarness(dump)
        getattr(h, page)(*args)
        h.paint()
        write_pgm(out / f"seed{SEED:08x}-menu-{keys}.pgm", h.frames[-1][1])
    print(f"wrote {len(pages())} pages to {out}")


if __name__ == "__main__":
    main()
