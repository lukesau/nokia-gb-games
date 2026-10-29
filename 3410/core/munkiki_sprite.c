/* Munkiki's picture (c.v), drawn from his frames, which the Game Boy
   keeps in a bank of their own with this. A frame code is an index in
   its low six bits, 0x40 for a half frame (20 wide, drawn twice, the
   second time mirrored, which makes him face the camera) and 0x80 for a
   full frame mirrored. The frames are in the Nokia API's vertical format:
   a byte per column per band of eight rows, the row's bit; the build
   keeps only the bands each frame's mask touches (munkiki_frame_table). */
#include "munkiki_int.h"

static void half(int16_t x, int16_t y, uint8_t width, uint8_t rows, const struct munkiki_frame *f, uint8_t flip)
{
    uint8_t col, row, band_end = (uint8_t)(f->band + f->bands);
    const uint8_t *bits = munkiki_sheet + f->at, *mask = munkiki_sheet_mask + f->at;

    for (row = (uint8_t)(f->band * 8); row < rows && (row >> 3) < band_end; row++) {
        int16_t py = (int16_t)(y + row);
        uint16_t base = (uint16_t)((row >> 3) - f->band) * width;
        uint8_t bit = (uint8_t)(1 << (row & 7));
        uint8_t *line, *p;

        if (py < 0 || py >= MK_H)
            continue;
        line = mk_frame + py * MK_STRIDE;
        for (col = 0; col < width; col++) {
            int16_t px = (int16_t)(x + (flip ? width - 1 - col : col));
            uint8_t screen_bit;

            if (!(mask[base + col] & bit) || px < 0 || px >= MK_W)
                continue;
            screen_bit = (uint8_t)(0x80 >> (px & 7));
            /* Without a branch on the bit: SDCC 4.6 drops the read of
               the frame's byte for `line[i] |= b` here. */
            p = line + (px >> 3);
            *p = (uint8_t)((*p & (uint8_t)~screen_bit) | (screen_bit & (uint8_t)-(uint8_t)((bits[base + col] & bit) != 0)));
        }
    }
}

void mk_frame_munkiki(int16_t x, int16_t y, uint8_t code, uint8_t rows) MK_SHEET_FAR
{
    if (code & 0x40) {
        const struct munkiki_frame *f = &munkiki_frame_table[MUNKIKI_HALF_FRAMES + (code & 0x3f)];

        half(x, y, 20, rows, f, 0);
        half((int16_t)(x + 20), y, 20, rows, f, 1);
    } else {
        half(x, y, 40, rows, &munkiki_frame_table[code & 0x3f], (uint8_t)(code & 0x80));
    }
}
