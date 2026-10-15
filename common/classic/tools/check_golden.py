#!/usr/bin/env python3
"""Compare host frames with frames captured from the original firmware.

Usage: check_golden.py FRAME_TOOL GOLDEN_DIR

GOLDEN_DIR holds PGM frames from MAME named after the host frame they must
equal, for example menu-sd.pgm. They are derived from the firmware, so the
directory is ignored and the check is skipped when it is missing or empty.
"""
import subprocess
import sys
import tempfile
from pathlib import Path

LCD_PIXELS = 84 * 48


def pixels(path):
    return Path(path).read_bytes()[-LCD_PIXELS:]


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    tool, golden = sys.argv[1], sorted(Path(sys.argv[2]).glob("*.pgm"))
    if not golden:
        print(f"no golden frames in {sys.argv[2]}; skipped")
        return
    failed = 0
    with tempfile.TemporaryDirectory() as tmp:
        for frame in golden:
            out = Path(tmp) / frame.name
            subprocess.run([tool, frame.stem, str(out)], check=True, stdout=subprocess.DEVNULL)
            ours, theirs = pixels(out), pixels(frame)
            wrong = sum(a != b for a, b in zip(ours, theirs))
            if wrong:
                failed += 1
                print(f"FAIL {frame.stem}: {wrong} pixels differ")
                for y in range(48):
                    row = "".join(
                        (" " if ours[i] else "#") if ours[i] == theirs[i] else ("+" if theirs[i] else "-")
                        for i in range(y * 84, y * 84 + 84)
                    )
                    print("  " + row)
            else:
                print(f"ok   {frame.stem}")
    if failed:
        sys.exit(f"{failed} of {len(golden)} frames differ ('-' missing from ours, '+' extra in ours)")


if __name__ == "__main__":
    main()
