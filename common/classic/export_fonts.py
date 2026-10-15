#!/usr/bin/env python3
"""Write the phone's fonts as ASCII-art glyph sheets, for use in other projects.

Usage: export_fonts.py PHONE DUMP OUT_DIR

Writes one sheet per font (large bold, small plain, small bold, tiny plain)
in the layout of the EZ Flash Jr project's font12.txt: a `glyph 0xNN 'c'`
header, then one line per pixel row, `#` ink and `.` paper. Glyphs are
left-aligned in a cell as wide as the font's widest glyph; a header's
`width` is the glyph's own advance, which includes the gap after it. The
sheets are derived from the firmware and must stay in an ignored directory.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from extract_assets import FIRST_CHAR, FONT_NAMES, PHONES, extract_fonts, load_image  # noqa: E402


def main():
    if len(sys.argv) != 4 or sys.argv[1] not in PHONES:
        sys.exit(__doc__)
    phone = sys.argv[1]
    image = load_image(Path(sys.argv[2]), phone)
    out_dir = Path(sys.argv[3])
    out_dir.mkdir(parents=True, exist_ok=True)
    for name, (height, baseline, glyphs) in zip(FONT_NAMES, extract_fonts(image, phone)):
        cell = max(len(g) for g in glyphs)
        lines = [
            f"# Nokia {phone} ({PHONES[phone]['name']}) {name.replace('font_', '').replace('_', ' ')} font.",
            f"# '.' = paper, '#' = ink. {height} rows per glyph, baseline after row {baseline},",
            f"# cells {cell} columns wide, glyphs left-aligned. Extracted from a firmware dump.",
            "",
        ]
        for i, columns in enumerate(glyphs):
            code = FIRST_CHAR + i
            if not columns:
                continue
            lines.append(f"glyph 0x{code:02X} '{chr(code)}' width {len(columns)}")
            for y in range(height):
                lines.append("".join("#" if x < len(columns) and columns[x] >> y & 1 else "." for x in range(cell)))
            lines.append("")
        path = out_dir / f"nokia{phone}_{name.replace('font_', '')}.txt"
        path.write_text("\n".join(lines))
        widths = sorted({len(g) for g in glyphs if g})
        print(f"wrote {path}: {sum(1 for g in glyphs if g)} glyphs, {height} rows, advances {widths[0]}..{widths[-1]}")


if __name__ == "__main__":
    main()
