#!/usr/bin/env python3
"""Extract Munkiki's Castles' data from its JAR into C arrays.

Usage: extract_munkiki.py JAR OUT_DIR

JAR is the MIDlet the 3410's PMM holds (munkikis_castles.jar, 40888
bytes, see README.md). Writes OUT_DIR/munkiki_data.h and six sources,
split as the Game Boy keeps them in banks beside the code that reads
them: munkiki_data.c (the levels and the animation tables, with the
world), munkiki_sounds.c (with the game's flow), munkiki_text.c (the
text and the pages' pictures, with the pages), munkiki_gfx.c (the 3D
geometry and the textures, with the rasterizer), munkiki_pics.c (the
effects' pictures, with the scene) and munkiki_sheet.c (Munkiki's
frames). The output is derived from the JAR and must stay in an ignored
directory.

What is where in the JAR, as the game's own code reads it (the classes are
obfuscated; the names below are the decompiled ones):
- /d: twelve ringing tones (u16 length, Smart Messaging bytes), then at
  554 the 3D geometry (c.ab), the score digits (64x13) and 23 pictures
  (bi), then at 1945 the 17 animation tables (d.ab), then three pictures:
  the level map (64x64, 2116), the game-over picture (96x54, 2632) and
  the title (96x65, 3284). A picture is u16 width, u16 height, then rows
  of width / 8 bytes, the leftmost pixel in the top bit, 1 black.
- /imageV3: Munkiki's frames (c.v): 8200 bytes of pixels then 8200 of
  mask, packed as the Nokia API's TYPE_BYTE_1_GRAY_VERTICAL: a byte per
  column per band of eight rows, bit y & 7 the row. 27 frames of 40x40,
  then 28 half frames of 20x40, which are shown mirrored for the other
  half.
- /MBF: the levels (d.ab, d.an): 13 bytes of block kinds, a count, that
  many level sizes, then the levels, each its number and a bit stream.
- /note.png, /key.png, /warp.png, /life.png: the textures of the things
  to collect; /kb.png and /ky.png the keypad the Instructions show.
- /en: the 32 English texts, UTF-16, each ended by a zero.
"""
import struct
import sys
import zipfile
import zlib
from pathlib import Path

GFX_AT = 554
ANIMS_AT = 1945
MAP_AT = 2116
OVER_AT = 2632
TITLE_AT = 3284
SOUND_COUNT = 12
PIC_COUNT = 23
SHEET_FULL, SHEET_HALF = 27, 28
TEXT_COUNT = 32

# The 3D geometry (c.ab): name and size in order, None for what the game
# reads and never uses.
GEOMETRY = [
    ("shades", 6),        # a3: the gray of each block kind, 1 to 6
    ("cube_verts", 24),   # a2: the eight corners of a block's faces
    ("cube_tris", 36),    # a1: twelve triangles over them
    ("edge_verts", 60),   # a0: twenty corners of the black edges
    ("edge_tris", 72),    # a_: twenty-four triangles over them
    ("ground", 9),        # ap: the backdrop triangle
    (None, 12), (None, 8),
    ("warp_verts", 12),   # az: the warp's square
    ("warp_tex", 8),      # ay
    ("warp_tris", 12),    # ax
    (None, 18), (None, 12), (None, 12), (None, 24),
    ("top_verts", 12),    # aw: a floor block's top face
    ("top_tris", 6),      # av
    ("rim_verts", 18),    # au: its black rim
    ("rim_tris", 12),     # at
]
# Those in the class's own static initializer.
GEOMETRY_STATIC = [
    ("thing_verts", [0, 18, 0, 8, 10, 0, 0, 2, 0, -8, 10, 0]),  # as: the diamond a note, key or life is on
    ("thing_tex", [0, 0, 1, 0, 1, 1, 0, 1]),                      # ar
    ("thing_tris", [0, 1, 2, 2, 3, 0, 0, 2, 1, 2, 0, 3]),         # aq
]

# The Smart Messaging ringing tone's tempos, by the five-bit value.
TEMPOS = [25, 28, 31, 35, 40, 45, 50, 56, 63, 70, 80, 90, 100, 112, 125, 140, 160, 180, 200, 225, 250, 285, 320,
          355, 400, 450, 500, 565, 635, 715, 800, 900]
DEFAULT_TEMPO = 63
# The phone's timer unit, in microseconds (core/games.h).
UNIT_US = 7781
SOUND_REST, SOUND_END = 0xFE, 0xFF  # core/sound.h
SOUND_LOW = 0x40  # plus semitones above 110 Hz, two octaves under 440 (core/sound.h)


class Bits:
    def __init__(self, data):
        self.data = data
        self.at = 0

    def left(self):
        return len(self.data) * 8 - self.at

    def get(self, n):
        v = 0
        for _ in range(n):
            v = v << 1 | self.data[self.at >> 3] >> (7 - (self.at & 7)) & 1
            self.at += 1
        return v


def decode_tone(data, which):
    """A ringing tone as the port's notes: pairs of note and length in timer
    units, SOUND_END last. The notes are semitones above 440 Hz, or
    SOUND_LOW plus those above 110 Hz for the ones under 440. Style 0 (natural) sounds a note for
    seven eighths of its time."""
    b = Bits(data)
    count = b.get(8)
    commands = [b.get(8) for _ in range(count)]
    if commands != [0x4A, 0x3A]:
        sys.exit(f"sound {which}: not a ringing tone ({commands})")
    b.get(3)  # the song type: 2, a temporary tone
    for _ in range(b.get(4)):  # the title
        b.get(8)
    b.get(8)  # patterns in the song
    b.get(3)  # the pattern header
    b.get(2)  # its id
    b.get(4)  # how often it repeats: never here
    b.get(8)  # what the header counts is not the instructions; the stream ends on its own
    b.get(7)  # seven bits more, meaning unknown, before the first instruction
    tempo, scale, style = DEFAULT_TEMPO, 1, 0
    notes = []
    while b.left() >= 5:
        kind = b.get(3)
        if kind == 0:
            if b.left() > 16 or b.get(b.left()):
                sys.exit(f"sound {which}: a second pattern")
            break
        if kind == 1:
            value, duration, specifier = b.get(4), b.get(3), b.get(2)
            # A whole note is four beats.
            us = 240000000 // tempo >> duration
            if specifier == 1:
                us = us * 3 // 2
            elif specifier == 2:
                us = us * 7 // 4
            elif specifier == 3:
                us = us * 2 // 3
            units = max(1, (us + UNIT_US // 2) // UNIT_US)
            if value == 0:
                notes.append((SOUND_REST, units))
                continue
            semitone = 12 * (scale - 1) + value - 10
            if semitone < -24 or semitone > 47:
                sys.exit(f"sound {which}: note {semitone} is out of the buzzer's range")
            note = semitone if semitone >= 0 else SOUND_LOW + semitone + 24
            if style == 0:
                on = max(1, units * 7 // 8)
                notes.append((note, on))
                if units - on:
                    notes.append((SOUND_REST, units - on))
            elif style == 2:
                on = max(1, units // 2)
                notes.append((note, on))
                notes.append((SOUND_REST, units - on))
            else:
                notes.append((note, units))
        elif kind == 2:
            scale = b.get(2)
        elif kind == 3:
            style = b.get(2)
        elif kind == 4:
            tempo = TEMPOS[b.get(5)]
        elif kind == 5:
            b.get(4)  # the volume
        else:
            sys.exit(f"sound {which}: instruction {kind}")
    out = bytearray()
    for note, units in notes:
        while units > 255:
            out += bytes((note, 255))
            units -= 255
        out += bytes((note, units))
    return bytes(out + bytes((SOUND_END,)))


def png_rows(data):
    """The rows of a 1-bit palette PNG as bytes, 1 black (the palette's
    index 0 is black in the JAR's), and its size."""
    at, width, height, idat = 8, 0, 0, b""
    while at < len(data):
        length, kind = struct.unpack_from(">I4s", data, at)
        chunk = data[at + 8:at + 8 + length]
        if kind == b"IHDR":
            width, height, depth, color = struct.unpack_from(">IIBB", chunk)
            if depth != 1 or color != 3:
                sys.exit("a PNG that is not 1-bit palette")
        elif kind == b"PLTE":
            if chunk[:3] != b"\0\0\0":
                sys.exit("a PNG whose first color is not black")
        elif kind == b"IDAT":
            idat += chunk
        at += 12 + length
    raw = zlib.decompress(idat)
    stride = (width + 7) // 8
    rows, prev, at = [], bytes(stride), 0
    for _ in range(height):
        kind, line = raw[at], bytearray(raw[at + 1:at + 1 + stride])
        at += 1 + stride
        for i in range(stride):
            a = line[i - 1] if i else 0
            up = prev[i]
            c = prev[i - 1] if i else 0
            if kind == 1:
                line[i] = (line[i] + a) & 255
            elif kind == 2:
                line[i] = (line[i] + up) & 255
            elif kind == 3:
                line[i] = (line[i] + (a + up) // 2) & 255
            elif kind == 4:
                p = a + up - c
                pa, pb, pc = abs(p - a), abs(p - up), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else up if pb <= pc else c)) & 255
        # Index 0 is black: a set bit is white, so flip; the padding bits
        # past the width are cleared.
        line = bytes(~v & 255 for v in line)
        if width & 7:
            line = line[:-1] + bytes((line[-1] & (0xFF << (8 - width % 8)) & 255,))
        rows.append(line)
        prev = line
    return b"".join(rows), width, height


def picture(data, at):
    w, h = struct.unpack_from("<HH", data, at)
    return data[at + 4:at + 4 + (w // 8) * h], w, h


def c_bytes(name, data, kind="uint8_t"):
    lines = [f"const {kind} {name}[{len(data)}] = {{"]
    for i in range(0, len(data), 16):
        lines.append("    " + " ".join(f"{b}," for b in data[i:i + 16]))
    lines.append("};")
    return "\n".join(lines)


def c_signed(name, data):
    return c_bytes(name, [b - 256 if b > 127 else b for b in data], "int8_t")


def c_string(text):
    out = '"'
    for ch in text:
        if ch == '"' or ch == "\\":
            out += "\\" + ch
        elif 0x20 <= ord(ch) <= 0x7E:
            out += ch
        else:
            sys.exit(f"a text with a character outside ASCII: {text!r}")
    return out + '"'


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    jar = zipfile.ZipFile(sys.argv[1])
    out = Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)
    d = jar.read("d")
    header = ["/* Generated by tools/extract_munkiki.py from the game's JAR. Do not commit. */",
              "#ifndef MUNKIKI_DATA_H", "#define MUNKIKI_DATA_H", "", "#include <stdint.h>", ""]
    source = ["/* Generated by tools/extract_munkiki.py from the game's JAR. Do not commit. */",
              '#include "munkiki_data.h"', ""]
    gfx = list(source)
    sheet = list(source)
    sounds_out = list(source)
    text_out = list(source)
    pics_out = list(source)

    # The sounds.
    at, sounds, places = 0, bytearray(), []
    for i in range(SOUND_COUNT):
        n = struct.unpack_from("<H", d, at)[0]
        places.append(len(sounds))
        sounds += decode_tone(d[at + 2:at + 2 + n], i)
        at += 2 + n
    if at != GFX_AT:
        sys.exit(f"the sounds end at {at}, not {GFX_AT}")
    header.append(f"#define MUNKIKI_SOUND_COUNT {SOUND_COUNT}")
    header.append(f"extern const uint8_t munkiki_sounds[{len(sounds)}];")
    header.append(f"extern const uint16_t munkiki_sound_at[{SOUND_COUNT}];")
    sounds_out.append(c_bytes("munkiki_sounds", sounds))
    sounds_out.append(c_bytes("munkiki_sound_at", places, "uint16_t"))

    # The geometry.
    for name, size in GEOMETRY:
        if name:
            header.append(f"extern const {'uint8_t' if name == 'shades' else 'int8_t'} munkiki_{name}[{size}];")
            (pics_out if name == "ground" else gfx).append(
                (c_bytes if name == "shades" else c_signed)(f"munkiki_{name}", d[at:at + size]))
        at += size
    for name, values in GEOMETRY_STATIC:
        header.append(f"extern const int8_t munkiki_{name}[{len(values)}];")
        gfx.append(c_bytes(f"munkiki_{name}", values, "int8_t"))

    # The digits and the 23 pictures.
    digits, w, h = picture(d, at)
    if (w, h) != (64, 13):
        sys.exit("the digits are not 64x13")
    at += 4 + len(digits)
    header.append(f"extern const uint8_t munkiki_digits[{len(digits)}];")
    text_out.append(c_bytes("munkiki_digits", digits))
    pics, table = bytearray(), []
    for i in range(PIC_COUNT):
        pic, w, h = picture(d, at)
        at += 4 + len(pic)
        table.append((len(pics), w, h))
        pics += pic
    if at != ANIMS_AT:
        sys.exit(f"the pictures end at {at}, not {ANIMS_AT}")
    header.append(f"#define MUNKIKI_PIC_COUNT {PIC_COUNT}")
    header.append("struct munkiki_pic { uint16_t at; uint8_t w, h; };")
    header.append(f"extern const struct munkiki_pic munkiki_pic_table[{PIC_COUNT}];")
    header.append(f"extern const uint8_t munkiki_pics[{len(pics)}];")
    pics_out.append(f"const struct munkiki_pic munkiki_pic_table[{PIC_COUNT}] = {{\n"
                    + "".join(f"    {{ {a}, {w}, {h} }},\n" for a, w, h in table) + "};")
    pics_out.append(c_bytes("munkiki_pics", pics))

    # The animations.
    count = d[at]
    at += 1
    anims, frames = [], bytearray()
    for i in range(count):
        n = d[at]
        anims.append((len(frames), n))
        frames += d[at + 1:at + 1 + n]
        at += 1 + n
    if at != MAP_AT:
        sys.exit(f"the animations end at {at}, not {MAP_AT}")
    header.append(f"#define MUNKIKI_ANIM_COUNT {count}")
    header.append("struct munkiki_anim { uint8_t at, count; };")
    header.append(f"extern const struct munkiki_anim munkiki_anim_table[{count}];")
    header.append(f"extern const uint8_t munkiki_anim_frames[{len(frames)}];")
    source.append(f"const struct munkiki_anim munkiki_anim_table[{count}] = {{\n"
                  + "".join(f"    {{ {a}, {n} }},\n" for a, n in anims) + "};")
    source.append(c_bytes("munkiki_anim_frames", frames))

    # The three big pictures.
    for name, where, size in (("map", MAP_AT, (64, 64)), ("over", OVER_AT, (96, 54)), ("title", TITLE_AT, (96, 65))):
        pic, w, h = picture(d, where)
        if (w, h) != size:
            sys.exit(f"the {name} picture is {w}x{h}")
        header.append(f"extern const uint8_t munkiki_{name}[{len(pic)}];")
        text_out.append(c_bytes(f"munkiki_{name}", pic))

    # The textures and the keypad.
    for name in ("note", "key", "warp", "life", "kb", "ky"):
        rows, w, h = png_rows(jar.read(name + ".png"))
        header.append(f"extern const uint8_t munkiki_png_{name}[{len(rows)}]; /* {w}x{h} */")
        (text_out if name in ("kb", "ky") else gfx).append(c_bytes(f"munkiki_png_{name}", rows))

    # Munkiki's frames, each trimmed to the bands of eight rows its mask
    # touches: {first band, bands} and where its bytes start.
    image = jar.read("imageV3")
    pixels, masks = image[:8200], image[8200:16400]
    frame_table, pix_out, mask_out = [], bytearray(), bytearray()
    for i in range(SHEET_FULL + SHEET_HALF):
        if i < SHEET_FULL:
            offset, width = i * 200, 40
        else:
            offset, width = 5400 + (i - SHEET_FULL) * 100, 20
        bands = [b for b in range(5) if any(masks[offset + b * width:offset + (b + 1) * width])]
        first, last = (bands[0], bands[-1]) if bands else (0, -1)
        frame_table.append((len(pix_out), first, last + 1 - first))
        pix_out += pixels[offset + first * width:offset + (last + 1) * width]
        mask_out += masks[offset + first * width:offset + (last + 1) * width]
    header.append(f"#define MUNKIKI_FRAME_COUNT {SHEET_FULL + SHEET_HALF}")
    header.append(f"#define MUNKIKI_HALF_FRAMES {SHEET_FULL}")
    header.append("struct munkiki_frame { uint16_t at; uint8_t band, bands; };")
    header.append(f"extern const struct munkiki_frame munkiki_frame_table[{len(frame_table)}];")
    header.append(f"extern const uint8_t munkiki_sheet[{len(pix_out)}];")
    header.append(f"extern const uint8_t munkiki_sheet_mask[{len(mask_out)}];")
    sheet.append(f"const struct munkiki_frame munkiki_frame_table[{len(frame_table)}] = {{\n"
                 + "".join(f"    {{ {a}, {b}, {n} }},\n" for a, b, n in frame_table) + "};")
    sheet.append(c_bytes("munkiki_sheet", pix_out))
    sheet.append(c_bytes("munkiki_sheet_mask", mask_out))

    # The levels.
    mbf = jar.read("MBF")
    kinds = mbf[:13]
    count = mbf[13]
    sizes = mbf[14:14 + count]
    levels = mbf[14 + count:]
    if sum(sizes) != len(levels):
        sys.exit("the levels' sizes do not add up")
    ids, starts, at = [], [], 0
    for size in sizes:
        ids.append(levels[at])
        starts.append(at + 1)
        at += size
    header.append("extern const uint8_t munkiki_level_kinds[13];")
    header.append(f"#define MUNKIKI_LEVEL_COUNT {count}")
    header.append(f"extern const uint8_t munkiki_level_ids[{count}];")
    header.append(f"extern const uint16_t munkiki_level_at[{count}];")
    header.append(f"extern const uint8_t munkiki_levels[{len(levels)}];")
    source.append(c_bytes("munkiki_level_kinds", kinds))
    source.append(c_bytes("munkiki_level_ids", ids))
    source.append(c_bytes("munkiki_level_at", starts, "uint16_t"))
    source.append(c_bytes("munkiki_levels", levels))

    # The English text.
    en = jar.read("en")
    texts, at = [], 0
    for _ in range(TEXT_COUNT):
        text = ""
        while True:
            ch = en[at] << 8 | en[at + 1]
            at += 2
            if not ch:
                break
            text += chr(ch)
        texts.append(text)
    header.append(f"#define MUNKIKI_TEXT_COUNT {TEXT_COUNT}")
    header.append(f"extern const char *const munkiki_texts[{TEXT_COUNT}];")
    header.append(f"#define MUNKIKI_TEXT_MAX {max(len(t) for t in texts) + 1}")
    text_out.append(f"const char *const munkiki_texts[{TEXT_COUNT}] = {{\n"
                    + "".join(f"    {c_string(t)},\n" for t in texts) + "};")

    header += ["", "#endif", ""]
    (out / "munkiki_data.h").write_text("\n".join(header))
    (out / "munkiki_data.c").write_text("\n\n".join(source) + "\n")
    (out / "munkiki_sounds.c").write_text("\n\n".join(sounds_out) + "\n")
    (out / "munkiki_text.c").write_text("\n\n".join(text_out) + "\n")
    (out / "munkiki_gfx.c").write_text("\n\n".join(gfx) + "\n")
    (out / "munkiki_pics.c").write_text("\n\n".join(pics_out) + "\n")
    (out / "munkiki_sheet.c").write_text("\n\n".join(sheet) + "\n")
    print(f"wrote {out}/munkiki_data.c ({len(pix_out) * 2} bytes of frames in munkiki_sheet.c)")


if __name__ == "__main__":
    main()
