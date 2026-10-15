"""Bantumi's data on the 3410 v5.46, for extract_assets.py.

The game draws through the 3410's graphics library (core/si_pic.h): each
picture is a list of 24-byte descriptors {u16 w, u16 h, u16 0, u16 1, u32
bitmap, u32 mask, ...}, the bitmaps in the LCD's band layout. The bitmaps
are copied whole, a run of the image at a time, and the descriptors are
written out as C tables pointing into them. See common/bantumi/bantumi.c and
the MAME fork's docs/games_bantumi_3410.md.
"""

# The runs of the image the bitmaps are in.
RUNS = [
    ("bantumi_bits", 0x4A8334, 0x4A8ADC),        # the thinking picture, the closing board, the hands
    ("bantumi_board_bits", 0x4B4454, 0x4B4754),  # the open board, 96x60
    ("bantumi_digit_bits", 0x4B47B4, 0x4B47D2),  # ten 3x5 digits
    ("bantumi_demo_bits", 0x4B4A38, 0x4B4A7D),   # the Instructions' keypad and arrow
    ("bantumi_title_bits", 0x4B6120, 0x4B6384),  # the title's background and its six pictures
]
# Picture tables: name, the first descriptor, how many.
TABLES = [
    ("bantumi_boards", 0x4B4754, 4),  # the open board, then the closing frames
    ("bantumi_digits", 0x4B4828, 10),
    ("bantumi_think", 0x4BECF8, 8),
    ("bantumi_keypad", 0x4B4A80, 1),
    ("bantumi_arrow", 0x4B4A98, 1),
    ("bantumi_title_background", 0x4B6384, 1),
    ("bantumi_title_pictures", 0x4B639C, 6),
]
# The hands, which the game builds in RAM (0x127190..0x1271ef): w, h,
# bitmap, mask. Open below the pits and above, closed below and above.
HANDS = [
    (12, 16, 0x4A8A1C, 0x4A8A4C),
    (12, 16, 0x4A8A34, 0x4A8A64),
    (11, 13, 0x4A8A7C, 0x4A8AAC),
    (11, 13, 0x4A8A94, 0x4A8AC4),
]
# The pits: 14 x, then 14 y, each the top left of its box.
PIT_XY = (0x4BECCC, 28)
# Where the title puts its small pictures, a step at a time (RAM the phone
# fills at boot, 0x12e1dc; read in MAME): the first is off the screen.
TITLE_AT = [(-10, -10), (23, 18), (34, 23), (37, 33), (45, 19), (53, 15), (62, 27)]


def extract_bantumi(at, c_bytes):
    """Returns the lines of bantumi_data.h and the text of bantumi_data.c."""
    def u16(addr):
        return int.from_bytes(at(addr, 2), "big")

    def u32(addr):
        return int.from_bytes(at(addr, 4), "big")

    def ref(addr, size):
        for name, start, end in RUNS:
            if start <= addr and addr + size <= end:
                return f"{name} + {addr - start}"
        raise SystemExit(f"Bantumi: a bitmap at 0x{addr:06x} is outside the runs copied")

    def image(w, h, bitmap):
        return f"{{ {ref(bitmap, w * ((h + 7) // 8))}, {w}, {h} }}"

    header = [f"extern const uint8_t {name}[{end - start}];" for name, start, end in RUNS]
    source = [c_bytes(name, at(start, end - start)) for name, start, end in RUNS]
    for name, first, count in TABLES:
        items = []
        for n in range(count):
            d = first + 24 * n
            w, h, zero, one, bitmap = u16(d), u16(d + 2), u16(d + 4), u16(d + 6), u32(d + 8)
            if zero != 0 or one != 1 or u32(d + 12):
                raise SystemExit(f"Bantumi: 0x{d:06x} is not a plain picture's descriptor")
            items.append("    " + image(w, h, bitmap) + ",")
        header.append(f"extern const struct sprite_image {name}[{count}];")
        source.append("\n".join([f"const struct sprite_image {name}[{count}] = {{", *items, "};"]))
    header.append(f"extern const struct si_masked_image bantumi_hands[{len(HANDS)}];")
    source.append("\n".join([f"const struct si_masked_image bantumi_hands[{len(HANDS)}] = {{",
                             *(f"    {{ {image(w, h, bitmap)}, {ref(mask, w * ((h + 7) // 8))} }},"
                               for w, h, bitmap, mask in HANDS), "};"]))
    header.append(f"extern const uint8_t bantumi_pit_xy[{PIT_XY[1]}];")
    source.append(c_bytes("bantumi_pit_xy", at(*PIT_XY)))
    header.append(f"extern const int8_t bantumi_title_at[{len(TITLE_AT)}][2];")
    source.append("\n".join([f"const int8_t bantumi_title_at[{len(TITLE_AT)}][2] = {{",
                             "    " + ", ".join(f"{{ {x}, {y} }}" for x, y in TITLE_AT) + ",", "};"]))
    return header, "\n\n".join(source)
