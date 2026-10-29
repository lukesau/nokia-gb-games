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
    uint8_t col, band, band_end = (uint8_t)(f->band + f->bands);
    const uint8_t *bits = munkiki_sheet + f->at, *mask = munkiki_sheet_mask + f->at;

    /* A column of a band at a time: its eight rows are one byte of the
       frame's bits and one of its mask, and sit in one bit of eight
       bytes of the picture, a row apart. */
    for (band = f->band; band < band_end && (uint8_t)(band << 3) < rows; band++, bits += width, mask += width) {
        uint8_t row0 = (uint8_t)(band << 3), last = (uint8_t)(rows - row0 > 8 ? 8 : rows - row0);
        int16_t py0 = (int16_t)(y + row0);

        for (col = 0; col < width; col++) {
            int16_t px = (int16_t)(x + (flip ? width - 1 - col : col));
            uint8_t m = mask[col], b = bits[col], screen_bit, r;
            int16_t py = py0;
            uint8_t *p;

            if (!m || px < 0 || px >= MK_W)
                continue;
            screen_bit = (uint8_t)(0x80 >> (px & 7));
            p = mk_frame + (px >> 3);
            for (r = 0; r < last; r++, py++, m >>= 1, b >>= 1) {
                if ((m & 1) && py >= 0 && py < MK_H) {
                    uint8_t *q = p + py * MK_STRIDE;

                    *q = (uint8_t)((*q & (uint8_t)~screen_bit) | (screen_bit & (uint8_t)-(uint8_t)(b & 1)));
                }
            }
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
