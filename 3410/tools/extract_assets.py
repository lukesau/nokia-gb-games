#!/usr/bin/env python3
"""Extract the 3410 v5.46 Snake II data, fonts and text from your own dump into C arrays.

Usage: extract_assets.py DUMP OUT_DIR

DUMP is the 0x370000-byte flash image the MAME fork's `make normalize-3410`
writes (3410f546e.fls), or the 16-bit byte-swapped form the static tools
use; it is identified by hash. Writes OUT_DIR/game_assets.h, si_data.h and si_data.c (Space Impact's, see
extract_si.py), bantumi_data.h and bantumi_data.c (Bantumi's, see
extract_bantumi.py), snake2_data.c
(Snake II's pictures, mazes and speeds, its title pictures and score box,
and the score's digits),
game_tables.c (the sounds), game_assets.c (the menus' fonts and text) and
game_apart.c (the Instructions' texts and the page headers' font),
apart so that a platform can place them apart. The output is derived from the firmware and must stay in an ignored
directory.

Snake II's data is in three places of the image. Its pictures are bitmaps
followed by 24-byte descriptors that point at them by absolute address, and
its maze records point at their walls the same way; each part is copied
whole and the core reads it by firmware address. See core/snake2_data.h and
the map in the MAME fork's docs/games_applications_3410.md.
"""
import hashlib
import struct
import sys
from pathlib import Path

from extract_bantumi import extract_bantumi
from extract_si import extract_si

FLASH_BASE = 0x200000
SHA256_RAW = "4b0e815a07dc18b3b5f1cff1134de55aa514eaa7ce5a0eecc44a270867562b04"

# Snake II: the bitmaps and their descriptors; the maze walls and the six
# 12-byte maze records; nine bytes of speeds.
SNAKE2_PICTURES = (0x4B2E50, 0x4B323C)
SNAKE2_MAZES = (0x4973B8, 0x4974A0)
SNAKE2_SPEEDS = (0x4BEF64, 9)
# The title: a 96x65 picture and five more the animation lays over it, 0x300
# bytes apart, so that the last row of each is the first of the next (and
# of the last, the start of their descriptors), as the phone shows them.
SNAKE2_TITLE = (0x4974A0, 0x498700)
# The score box the game-over picture and the High scores page show: its
# two 6x12 ends, then ten 6x8 digits; and further on, the 11x11 medal the
# High scores page puts beside its box.
SNAKE2_BOX = (0x4B47D4, 0x4B4AC6)
DIGIT_GLYPHS = (0x49017C, 40)  # ten 4x5 digits, 4 bytes each

# The phone's sounds, by id: 8-byte records that start with the address of
# a tone script. The games' are a zero byte, the command 9, pairs of note
# and length, and the command 11; a note of 0x40 is a rest, and the
# commands 5 n ... 6 play what they enclose n times.
SOUND_TABLE = 0x4A9078
SOUND_IDS = [0x13, 0x14, 0x15, 0x16, 0x17, 0x1F, 0x20, 0x21, 0x23]  # the games' SOUND_ codes (si.h, snake2.h, bantumi.h)
NOTE_FIRST, NOTE_LAST = 0x7C, 0xAB  # 440 Hz and 47 semitones above it
NOTE_REST = 0x40
SCRIPT_END, SCRIPT_REPEAT, SCRIPT_AGAIN = 0x0B, 0x05, 0x06
SOUND_REST = 0xFE  # core/sound.h

# The language pack ("PPM") holds the fonts and the text. Each chunk is
# {u32 checksum, u32 length, 4-byte name, ...}.
FONT_CHUNK = 0x4D0414
TEXT_CHUNK = 0x4D8740
ENGLISH_BLOCK = 0x1BC  # offset of the ENGL block inside the TEXT chunk

# The first six of the pack's nine fonts; the last three have nothing in
# the range used here.
FONT_NAMES = ["font_large_bold", "font_small_plain", "font_medium_bold", "font_small_bold",
              "font_tiny_plain", "font_tiny_bold"]
# Those the port draws with: the rest are left out, to save the room.
FONTS_USED = FONT_NAMES[:5]
# The one only the menu pages' headers draw with, in game_apart.c.
FONTS_APART = ["font_tiny_plain"]
FIRST_CHAR, LAST_CHAR = 0x20, 0x7E

# English text by index in the language pack's string table.
STRINGS = [
    ("text_select", 565),
    ("text_back", 1316),
    ("text_exit", 244),
    ("text_menu", 858),
    ("text_ok", 1474),
    ("text_more", 1469),
    ("text_games", 199),
    ("text_select_game", 122),
    ("text_settings", 762),
    ("text_snake", 769),
    ("text_space_impact", 770),
    ("text_bumper", 103),
    ("text_bantumi", 753),
    ("text_link5", 751),
    ("text_continue", 755),
    ("text_new_game", 761),
    ("text_high_scores", 136),
    ("text_options", 121),
    ("text_instructions", 756),
    ("text_game_options", 107),
    ("text_mazes", 745),
    ("text_level", 750),
    ("text_no_maze", 752),
    ("text_maze_box", 124),
    ("text_maze_tunnel", 126),
    ("text_maze_spiral", 125),
    ("text_maze_blockade", 123),
    ("text_maze_twisted", 127),
    ("text_game_sounds", 763),
    ("text_game_lights", 758),
    ("text_shakes", 749),
    ("text_off", 764),
    ("text_on", 765),
    ("text_club_nokia_id", 768),
    ("text_no_id", 767),
    ("text_help_snake_1", 1958),
    ("text_help_snake_2", 1959),
    ("text_help_snake_3", 1960),
    ("text_help_snake_4", 1961),
    ("text_chapters", 128),
    ("text_genevas_world", 134),
    ("text_done", 707),
    ("text_help_si_1", 1962),
    ("text_help_si_2", 1963),
    ("text_help_si_3", 1964),
    ("text_help_si_4", 1965),
    ("text_no_of_beans", 130),
    ("text_beans_3", 131),
    ("text_beans_4", 132),
    ("text_beans_5", 133),
    ("text_help_bantumi_1", 1943),
    ("text_help_bantumi_2", 1944),
    ("text_help_bantumi_3", 1945),
    ("text_help_bantumi_4", 1946),
    ("text_help_bantumi_5", 1947),
]


def swap16(data):
    out = bytearray(data)
    out[0::2], out[1::2] = data[1::2], data[0::2]
    return bytes(out)


def load_image(path):
    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest() == SHA256_RAW:
        return data
    if hashlib.sha256(swap16(data)).hexdigest() == SHA256_RAW:
        return swap16(data)
    sys.exit(f"{path} is not the NHM-2 v5.46 PPM E image (SHA-256 {SHA256_RAW})")


def chunk(image, addr, name):
    start = addr - FLASH_BASE
    length, tag = struct.unpack_from(">I4s", image, start + 4)
    if tag != name:
        sys.exit(f"no {name.decode()} chunk at 0x{addr:06x}")
    return image[start:start + 8 + length]


def extract_fonts(image):
    """Returns [(height, baseline, [columns per character])] for the fonts.

    A font record is 44 bytes: offsets (relative to the record) of the glyph
    pool table and of the character range table, and the range count. A range
    entry is {u16 first, u16 last, u32 packed}: row offset << 14, pool << 10,
    height << 5, baseline. A pool is one tall bitmap of a fixed width, stored
    as strips of 8 rows with one byte per column; a glyph is `height` rows of
    it starting at the row offset.
    """
    data = chunk(image, FONT_CHUNK, b"FONT")
    count, = struct.unpack_from(">I", data, 0x24)
    fonts = []
    for index in range(min(count, len(FONT_NAMES))):
        record = 0x28 + index * 44
        pools_at, _, ranges_at, ranges = struct.unpack_from(">IIII", data, record)
        glyphs, height, baseline = [], 0, 0
        for char in range(FIRST_CHAR, LAST_CHAR + 1):
            columns = []
            for r in range(ranges):
                first, last, packed = struct.unpack_from(">HHI", data, record + ranges_at + r * 8)
                if first <= char <= last:
                    height, baseline = packed >> 5 & 31, packed & 31
                    pool = record + pools_at + (packed >> 10 & 15) * 12
                    pool_data, _, width = struct.unpack_from(">IHH", data, pool)
                    row = (packed >> 14) + (char - first) * height
                    for x in range(width):
                        bits = 0
                        for y in range(height):
                            byte = data[pool + pool_data + ((row + y) >> 3) * width + x]
                            bits |= (byte >> ((row + y) & 7) & 1) << y
                        columns.append(bits)
                    break
            glyphs.append(columns)
        fonts.append((height, baseline, glyphs))
    return fonts


def extract_strings(image):
    data = chunk(image, TEXT_CHUNK, b"TEXT")
    size, tag = struct.unpack_from(">I4s", data, ENGLISH_BLOCK + 4)
    if tag != b"ENGL":
        sys.exit("no ENGL block in the TEXT chunk")
    lengths, end = ENGLISH_BLOCK + 16, ENGLISH_BLOCK + size
    count = total = 0
    while lengths + count + total < end:
        total += data[lengths + count]
        count += 1
    strings, at = [], lengths + count
    for i in range(count):
        strings.append(data[at:at + data[lengths + i]])
        at += data[lengths + i]
    if strings[0] != b"English\0":
        sys.exit("unexpected ENGL string table layout")
    return strings


def c_string(text):
    out = ""
    for byte in text.rstrip(b"\0"):
        char = chr(byte)
        if char == "\n":
            out += "\\n"
        elif char in '"\\':
            out += "\\" + char
        elif 0x20 <= byte < 0x7F:
            out += char
        else:
            out += f'\\x{byte:02x}""'
    return f'"{out}"'


def c_bytes(name, data):
    lines = [f"const uint8_t {name}[{len(data)}] = {{"]
    for i in range(0, len(data), 16):
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in data[i:i + 16]) + ",")
    lines.append("};")
    return "\n".join(lines)


def extract_sounds(image):
    """Returns the games' sounds as one run of bytes and, by id less the
    first id, each sound's place in it (0xff for an id that is not the
    games'). A sound is pairs of note, in semitones above 440 Hz or
    SOUND_REST, and length in timer units, ended by 0xff. Repeats are
    written out, and notes of one pitch in a row are made one, as the
    phone's buzzer sounds them."""
    scripts, places = bytearray(), [0xFF] * (SOUND_IDS[-1] - SOUND_IDS[0] + 1)
    for sound in SOUND_IDS:
        record = SOUND_TABLE + sound * 8 - FLASH_BASE
        at = int.from_bytes(image[record:record + 4], "big") - FLASH_BASE
        if image[at:at + 2] != b"\x00\x09":
            sys.exit(f"sound {sound:#x} is not a plain run of notes")
        at += 2
        steps, repeat = [], None
        while image[at] != SCRIPT_END:
            if image[at] == SCRIPT_REPEAT:
                repeat = (len(steps), image[at + 1])
                at += 2
                continue
            if image[at] == SCRIPT_AGAIN and repeat:
                first, times = repeat
                steps += steps[first:] * (times - 1)
                repeat = None
                at += 1
                continue
            note, length = image[at], image[at + 1]
            if note == NOTE_REST and length:
                steps.append((SOUND_REST, length))
            elif NOTE_FIRST <= note <= NOTE_LAST and length:
                steps.append((note - NOTE_FIRST, length))
            else:
                sys.exit(f"sound {sound:#x} has something other than a note at 0x{at + FLASH_BASE:06x}")
            at += 2
        notes = []
        for note, length in steps:
            if notes and notes[-1][0] == note and notes[-1][1] + length < 256:
                notes[-1][1] += length
            else:
                notes.append([note, length])
        places[sound - SOUND_IDS[0]] = len(scripts)
        scripts += bytes(b for note in notes for b in note) + b"\xff"
    return bytes(scripts), bytes(places)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    image = load_image(Path(sys.argv[1]))
    out = Path(sys.argv[2])
    out.mkdir(parents=True, exist_ok=True)

    def at(addr, size):
        return image[addr - FLASH_BASE:addr - FLASH_BASE + size]

    pictures = at(SNAKE2_PICTURES[0], SNAKE2_PICTURES[1] - SNAKE2_PICTURES[0])
    mazes = at(SNAKE2_MAZES[0], SNAKE2_MAZES[1] - SNAKE2_MAZES[0])
    sounds, sound_places = extract_sounds(image)

    header = [f"""/* Generated by tools/extract_assets.py from the firmware dump. Do not commit. */
#ifndef GAME_ASSETS_H
#define GAME_ASSETS_H

#include <stdint.h>

#define SNAKE2_PICTURES_BASE 0x{SNAKE2_PICTURES[0]:06x}ul
#define SNAKE2_PICTURES_SIZE {len(pictures)}
extern const uint8_t snake2_pictures[SNAKE2_PICTURES_SIZE];
#define SNAKE2_MAZES_BASE 0x{SNAKE2_MAZES[0]:06x}ul
#define SNAKE2_MAZES_SIZE {len(mazes)}
extern const uint8_t snake2_mazes[SNAKE2_MAZES_SIZE];
extern const uint8_t snake2_speeds[{SNAKE2_SPEEDS[1]}];
#define SNAKE2_TITLE_BASE 0x{SNAKE2_TITLE[0]:06x}ul
extern const uint8_t snake2_title[{SNAKE2_TITLE[1] - SNAKE2_TITLE[0]}];
#define SNAKE2_BOX_BASE 0x{SNAKE2_BOX[0]:06x}ul
extern const uint8_t snake2_box[{SNAKE2_BOX[1] - SNAKE2_BOX[0]}];

extern const uint8_t game_digit_glyphs[{DIGIT_GLYPHS[1]}];
/* The games' sounds: pairs of note, in semitones above 440 Hz, and length
   in the phone's timer units, each sound ended by 0xff; and by sound code
   less GAME_SOUND_FIRST, where each starts. */
#define GAME_SOUND_FIRST 0x{SOUND_IDS[0]:02x}
extern const uint8_t game_sounds[{len(sounds)}];
extern const uint8_t game_sound_places[{len(sound_places)}];
"""]
    banner = '/* Generated by tools/extract_assets.py from the firmware dump. Do not commit. */\n#include "game_assets.h"'
    (out / "snake2_data.c").write_text("\n\n".join([
        banner,
        c_bytes("snake2_pictures", pictures),
        c_bytes("snake2_mazes", mazes),
        c_bytes("snake2_speeds", at(*SNAKE2_SPEEDS)),
        c_bytes("game_digit_glyphs", at(*DIGIT_GLYPHS)),
        c_bytes("snake2_title", at(SNAKE2_TITLE[0], SNAKE2_TITLE[1] - SNAKE2_TITLE[0])),
        c_bytes("snake2_box", at(SNAKE2_BOX[0], SNAKE2_BOX[1] - SNAKE2_BOX[0])),
    ]) + "\n")
    (out / "game_tables.c").write_text("\n\n".join([
        banner,
        c_bytes("game_sounds", sounds),
        c_bytes("game_sound_places", sound_places),
    ]) + "\n")
    si_header, si_source, (rows_header, rows_sources) = extract_si(at, c_bytes)
    (out / "si_rows_data.h").write_text(rows_header)
    for name, text in rows_sources.items():
        (out / name).write_text('/* Generated by tools/extract_assets.py from the firmware dump. Do not commit. */\n'
                                '#include "si_rows_data.h"\n\n' + text + "\n")
    (out / "si_data.h").write_text("\n".join([
        "/* Generated by tools/extract_assets.py from the firmware dump. Do not commit. */",
        "#ifndef SI_DATA_H", "#define SI_DATA_H", "", "#include <stdint.h>", "", '#include "sprite.h"', "",
        *si_header, "", "#endif", ""]))
    (out / "si_data.c").write_text("\n\n".join([
        '/* Generated by tools/extract_assets.py from the firmware dump. Do not commit. */\n#include "si_data.h"',
        si_source]) + "\n")
    bantumi_header, bantumi_source = extract_bantumi(at, c_bytes)
    (out / "bantumi_data.h").write_text("\n".join([
        "/* Generated by tools/extract_assets.py from the firmware dump. Do not commit. */",
        "#ifndef BANTUMI_DATA_H", "#define BANTUMI_DATA_H", "", "#include <stdint.h>", "", '#include "si_pic.h"', "",
        *bantumi_header, "", "#endif", ""]))
    (out / "bantumi_data.c").write_text("\n\n".join([
        '/* Generated by tools/extract_assets.py from the firmware dump. Do not commit. */\n#include "bantumi_data.h"',
        bantumi_source]) + "\n")
    source = [banner, ""]

    header += ["", '#include "font.h"', ""]
    apart = [banner, ""]
    for name, (height, baseline, glyphs) in zip(FONT_NAMES, extract_fonts(image)):
        if name not in FONTS_USED:
            continue
        # The page headers' font apart, with the Instructions' texts.
        out_list = apart if name in FONTS_APART else source
        columns = [c for glyph in glyphs for c in glyph]
        offsets, place = [], 0
        for glyph in glyphs:
            offsets.append(place)
            place += len(glyph)
        offsets.append(place)
        out_list.append(f"static const uint16_t {name}_columns[{len(columns)}] = {{")
        for i in range(0, len(columns), 10):
            out_list.append("    " + " ".join(f"0x{c:04x}," for c in columns[i:i + 10]))
        out_list.append("};")
        out_list.append(f"static const uint16_t {name}_offsets[{len(offsets)}] = {{")
        for i in range(0, len(offsets), 12):
            out_list.append("    " + " ".join(f"{o}," for o in offsets[i:i + 12]))
        out_list.append("};")
        out_list.append(f"const struct font {name} = {{ {height}, {baseline}, {name}_offsets, {name}_columns }};")
        out_list.append("")
        header.append(f"extern const struct font {name};")

    header.append("")
    strings = extract_strings(image)
    for name, index in STRINGS:
        # The Instructions' texts apart, for a platform to place apart.
        (apart if name.startswith("text_help_") else source).append(
            f"const char {name}[] = {c_string(strings[index])};")
        header.append(f"extern const char {name}[];")
    header.append(f"#define TEXT_HELP_MAX {max(len(strings[i]) for n, i in STRINGS if n.startswith('text_help_')) + 1}")
    source.append("")
    (out / "game_apart.c").write_text("\n".join(apart) + "\n")
    header += ["", "#endif", ""]

    (out / "game_assets.h").write_text("\n".join(header))
    (out / "game_assets.c").write_text("\n".join(source))
    print(f"wrote {out}/game_assets.c and {out}/snake2_data.c")


if __name__ == "__main__":
    main()
