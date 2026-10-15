#!/usr/bin/env python3
"""Record the 5110's own Logic as golden frames, from the fork's Unicorn harness.

Usage: golden_logic.py DCT3_RE DUMP OUT_DIR

MAME does not run v5.28 (CONTACT SERVICE), so the frames come from the
firmware's Logic run natively in DCT3_RE/tools/dct3_5110_game_harness.py.
Each game is seeded through rand's state (0x10b6c0, the runtime's rand at
0x293e4c), started as the games menu's New game does, and given phone keys,
with the LCD cleared before each draw as the framework does. A frame is
written after every few keys, named for the host frame that must equal it:
the seed, the port's menu path into Logic at that level, and the keys as
the port's buttons. Per level: two games of random keys, past the ten
tries' scroll, and one that types the secret in after a wrong try (the
firmware then posts its game-over status, 0x5132 0x13; the frame before
that last check is kept). Derived from the firmware: keep OUT_DIR ignored.
"""
import random
import sys
from pathlib import Path

W, H = 84, 48
RAND_STATE = 0x10B6C0
LOGIC = 2
# Up and down move along the try, left copies, A is 5, Start checks (*).
PORT_KEY = {"2": "u", "8": "d", "4": "l", "6": "r", "5": "s", "*": "a"}
EVERY = 5


def menu_path(level):
    """From the first screen to a Logic game at the level: Games, Logic,
    then for a level other than 0 Level, up to it, OK, and New game."""
    return "sdds" + ("us" + "u" * level + "s" + "ds" if level else "s")


def secret(seed, level):
    length, kinds = 5 - (level < 4), ((level & 3) + 2) * 2
    code = []
    for _ in range(length):
        seed = (seed * 0x41C64E6D + 0x3039) & 0xFFFFFFFF
        code.append(((seed & 0x7FFFFFFF) >> 16) % kinds + 1)
    return code, kinds


def play(harness, image, seed, level, keys):
    """The frames after 0, 1, .. len(keys) keys, and the statuses posted."""
    hs = harness.Harness(image)
    hs.w32(RAND_STATE, seed)
    hs.set_level(LOGIC, level)
    hs.w8(harness.GAME_INDEX, LOGIC)
    hs.w8(harness.GAME_RECORDS + 8 * LOGIC + 6, 1)
    hs.w8(harness.GAME_RECORDS + 8 * LOGIC + 5, 0)
    hs.event(LOGIC, harness.EV_INIT)
    hs.event(LOGIC, harness.EV_RESUME)

    def draw():
        hs.mu.mem_write(harness.FB, bytes(W * H // 8))
        hs.event(LOGIC, harness.EV_DRAW)
        return bytes(hs.mu.mem_read(harness.FB, W * H // 8))

    frames = [draw()]
    for key in keys:
        hs.event(LOGIC, ord(key))
        frames.append(draw())
    return frames, hs.statuses


def write_pgm(path, fb):
    pixels = bytes(0 if fb[(y >> 3) * W + x] >> (y & 7) & 1 else 255 for y in range(H) for x in range(W))
    path.write_bytes(b"P5\n%d %d\n255\n" % (W, H) + pixels)


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    dct3_re, dump, out = Path(sys.argv[1]), sys.argv[2], Path(sys.argv[3])
    sys.path.insert(0, str(dct3_re / "tools"))
    import dct3_5110_game_harness as harness

    out.mkdir(parents=True, exist_ok=True)
    rng = random.Random(5110)
    written = 0
    for level in range(8):
        games = []
        for _ in range(2):
            keys = [rng.choice("2845*" if rng.random() < 0.9 else "6") for _ in range(80)]
            games.append((rng.getrandbits(32), keys, False))
        seed = rng.getrandbits(32)
        code, kinds = secret(seed, level)
        keys = []
        for figure in code:
            keys += ["5"] * (figure % kinds + 1) + ["8"]
        keys += ["*"]
        for figure in code:
            keys += ["5"] * figure + ["8"]
        keys += ["*"]
        games.append((seed, keys, True))

        for seed, keys, wins in games:
            frames, statuses = play(harness, dump, seed, level, keys)
            if wins:
                if not statuses or statuses[-1][1:] != (0x5132, 0x13):
                    sys.exit(f"level {level} seed {seed:08x}: the secret did not win: {statuses}")
                # The port shows its YOU WON! page after the last check.
                keys, frames = keys[:-1], frames[:-1]
            prefix = f"seed{seed:08x}-menu-{menu_path(level)}"
            for n in list(range(0, len(keys), EVERY)) + [len(keys)]:
                name = prefix + "".join(PORT_KEY[k] for k in keys[:n])
                write_pgm(out / f"{name}.pgm", frames[n])
                written += 1
    print(f"wrote {written} frames to {out}")


if __name__ == "__main__":
    main()
