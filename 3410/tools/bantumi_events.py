#!/usr/bin/env python3
"""Turn a Bantumi autopilot log from MAME into the replay's events.

Usage: bantumi_events.py [--demos | --title] ERROR_LOG OUT_DIR

ERROR_LOG is the error.log of a run of the MAME fork's
mame_nokia_3410_bantumi_bot.lua. Writes OUT_DIR/events.txt, every call of
the game's handler from New game (0x0a) until the game closes back to its
menu at its end (Menu and Continue in between), one "EVENT A B" in hex per line, in the 3410's own numbering
(common/bantumi/bantumi.h); OUT_DIR/setup.txt, the level the autopilot set
and the beans a pit started with; and OUT_DIR/sounds.txt, for each call that asked for them, "N snd CODE",
the last sound asked for (0x3b2510) as a code in the phone's sound table, N
the call's line in events.txt.

With --demos the events are the Instructions' demos instead: from the
first demo (0x09) to the end of the log, and setup.txt says the level the
phone's record has (the demos never ask for it) and 4 beans. With --title
they are the title's, from 0x0e until it closes.
"""
import re
import sys
from pathlib import Path

# The 3410's sound ids, as its table at 0x4c3538 (0x3f7d0e) maps them onto
# the sound table, less 3.
SOUND_CODES = {0xFA0: 0x1F, 0xFA1: 0x20, 0xFA2: 0x21, 0xFA3: 0x22, 0xFA4: 0x23,
               0xFA5: 0x14, 0xFA6: 0x15, 0xFA7: 0x13, 0xFA8: 0x16, 0xFA9: 0x17}


def main():
    args = sys.argv[1:]
    demos = args[:1] == ["--demos"]
    title = args[:1] == ["--title"]
    if demos or title:
        args = args[1:]
    if len(args) != 2:
        sys.exit(__doc__)
    log = Path(args[0]).read_text(errors="replace").splitlines()
    out = Path(args[1])
    events, started, setup = [], False, "1 4" if demos or title else None
    for line in log:
        s = re.match(r"BSETUP (\d+) (\d+)", line)
        if s and started:
            # The first: Continue's New game logs it again.
            setup = setup or f"{s.group(1)} {s.group(2)}"
            continue
        # The game closes back to its menu at its end, and on Menu, after
        # which Continue goes on with it.
        if not demos and started and line.startswith("BCLOSE") and not events[-1][0].startswith("3 "):
            break
        s = re.match(r"BSND (\w+)", line)
        if s and started and events:
            events[-1][1] = f"snd {SOUND_CODES[int(s.group(1), 16)]:x}"
            continue
        m = re.match(r"BEV (\w+) (\w+) (\w+)", line)
        if not m:
            continue
        event, a, b = (int(v, 16) for v in m.group(1, 2, 3))
        if not started:
            if event != (0x09 if demos else 0x0E if title else 0x0A):
                continue
            started = True
        events.append([f"{event:x} {a:x} {b:x}", None])
    if not started or not setup:
        sys.exit(f"{args[0]}: no {'demo' if demos else 'title' if title else 'New game'} in the log")
    out.mkdir(parents=True, exist_ok=True)
    (out / "events.txt").write_text("".join(e[0] + "\n" for e in events))
    (out / "sounds.txt").write_text("".join(f"{n} {e[1]}\n" for n, e in enumerate(events) if e[1]))
    (out / "setup.txt").write_text(setup + "\n")
    print(f"{out}: {len(events)} events")


if __name__ == "__main__":
    main()
