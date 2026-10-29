#!/usr/bin/env python3
"""Write the phone's font metrics for the Munkiki's Castles harness.

Usage: munkiki_fontmetrics.py ASSETS_DIR OUT

Reads the fonts tools/extract_assets.py wrote into ASSETS_DIR
(game_assets.c) and writes OUT: a line per MIDP font size, "size height
w32 ... w126", the widths of the characters 32 to 126 in the font the
phone's KVM uses for it (SMALL the small plain, MEDIUM the medium bold,
LARGE the large bold) and the height the KVM reports for it. Derived
from the firmware, so OUT must stay in an ignored directory.
"""
import re
import sys
from pathlib import Path

FONTS = [(8, "font_small_plain", 11), (0, "font_medium_bold", 13), (16, "font_large_bold", 15)]


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    source = (Path(sys.argv[1]) / "game_assets.c").read_text()
    out = []
    for size, name, height in FONTS:
        m = re.search(r"const uint16_t %s_offsets\[\d*\] = \{([^}]*)\}" % name, source)
        offsets = [int(x, 0) for x in m.group(1).replace("\n", " ").split(",") if x.strip()]
        widths = [offsets[i + 1] - offsets[i] for i in range(95)]
        out.append(f"{size} {height} " + " ".join(map(str, widths)))
    Path(sys.argv[2]).write_text("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
