/* Munkiki's Castles: drawing into its frame (munkiki_int.h), row-major
   pictures in the JAR's format 1. Included by munkiki_3d.c and
   munkiki_game.c as static functions, a copy in each: on the Game Boy
   they are in different banks, and each reads pictures from its own
   bank, which code in another bank could not. Define MK_BLIT_PAGES for
   the pages' lines, outlines and arcs. */
#ifndef CORE_MUNKIKI_BLIT_H
#define CORE_MUNKIKI_BLIT_H

#include <string.h>

#include "munkiki_int.h"

#ifdef __SDCC
#define MK_BLIT static
#else
#define MK_BLIT static inline
#endif

MK_BLIT void frame_clear(void)
{
    memset(mk_frame, 0, MK_STRIDE * MK_H);
}

/* Pixels x to x + w - 1 of row y, black or white. */
MK_BLIT void frame_row(uint8_t y, int16_t x0, int16_t x1, uint8_t p)
{
    uint8_t *row = mk_frame + y * MK_STRIDE;
    uint8_t first, last, m;

    if (x0 < 0)
        x0 = 0;
    if (x1 > MK_W)
        x1 = MK_W;
    if (x0 >= x1)
        return;
    first = (uint8_t)(x0 >> 3);
    last = (uint8_t)((x1 - 1) >> 3);
    if (first == last) {
        m = (uint8_t)((0xff >> (x0 & 7)) & (0xff << (7 - ((x1 - 1) & 7))));
        row[first] = (uint8_t)((row[first] & ~m) | (p & m));
        return;
    }
    m = (uint8_t)(0xff >> (x0 & 7));
    row[first] = (uint8_t)((row[first] & ~m) | (p & m));
    for (first++; first < last; first++)
        row[first] = p;
    m = (uint8_t)(0xff << (7 - ((x1 - 1) & 7)));
    row[last] = (uint8_t)((row[last] & ~m) | (p & m));
}

MK_BLIT void frame_fill(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t black)
{
    int16_t y1 = (int16_t)(y + h), yy;

    for (yy = y < 0 ? 0 : y; yy < y1 && yy < MK_H; yy++)
        frame_row((uint8_t)yy, x, (int16_t)(x + w), black ? 0xff : 0);
}

MK_BLIT uint8_t pixel_of(const uint8_t *bits, uint8_t stride, uint8_t x, uint8_t y)
{
    return (uint8_t)((bits[y * stride + (x >> 3)] >> (7 - (x & 7))) & 1);
}

MK_BLIT void plot(int16_t x, int16_t y, uint8_t black)
{
    uint8_t bit;

    if (x < 0 || x >= MK_W || y < 0 || y >= MK_H)
        return;
    bit = (uint8_t)(0x80 >> (x & 7));
    if (black)
        mk_frame[y * MK_STRIDE + (x >> 3)] |= bit;
    else
        mk_frame[y * MK_STRIDE + (x >> 3)] &= (uint8_t)~bit;
}

/* Eight pixels of a row of a picture from column `col`, which may be up
   to seven before its first; columns past its last byte are white. */
MK_BLIT uint8_t fetch8(const uint8_t *row, uint8_t stride, int16_t col)
{
    uint8_t i, sh;
    uint16_t v;

    if (col < 0)
        return (uint8_t)(row[0] >> (uint8_t)-col);
    i = (uint8_t)(col >> 3);
    sh = (uint8_t)(col & 7);
    if (i >= stride)
        return 0;
    if (sh == 0)
        return row[i];
    v = (uint16_t)(row[i] << 8);
    if ((uint8_t)(i + 1) < stride)
        v |= row[i + 1];
    return (uint8_t)(v >> (8 - sh));
}

MK_BLIT uint8_t reverse8(uint8_t v)
{
    static const uint8_t nibble[16] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15};

    return (uint8_t)((nibble[v & 15] << 4) | nibble[v >> 4]);
}

/* Rows sy on of a picture of `stride` bytes a row, its columns sx to
   sx + w - 1, at (x, y) in the frame, h rows; with a mask of the same
   layout or none, and mirrored left to right with MK_FLIP. Clipped to the
   frame. A byte at a time, in 8-bit steps for SDCC's sake: each eight
   source columns, read at the source's bit offset, are shifted to the
   frame's by a carry into the next byte. */
#define MK_FLIP 1
MK_BLIT void blit_rows(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t *bits, const uint8_t *mask,
                       uint8_t stride, uint8_t sx, uint8_t sy, uint8_t flags)
{
    uint8_t n = (uint8_t)((w + 7) >> 3), s = (uint8_t)(x & 7), t = (uint8_t)(sx & 7), sb = (uint8_t)(sx >> 3);
    uint8_t last_m = (uint8_t)(0xff << ((8 - (w & 7)) & 7)), avail = (uint8_t)(stride - sb), row = 0, rows;
    int8_t d0 = (int8_t)(x >> 3);
    const uint8_t *src, *msk = 0;
    uint8_t *dst;

    if (x >= MK_W || x + w <= 0 || y >= MK_H || y + h <= 0)
        return;
    if (y < 0)
        row = (uint8_t)-y;
    rows = (uint8_t)(y + h > MK_H ? MK_H - y : h);
    src = bits + (uint16_t)(sy + row) * stride + sb;
    if (mask)
        msk = mask + (uint16_t)(sy + row) * stride + sb;
    dst = mk_frame + (y + row) * MK_STRIDE;
    if (!(flags & MK_FLIP) && !mask && s == 0 && t == 0) {
        /* Byte-aligned and unmasked (the full-width pictures): copied. */
        uint8_t skip = (uint8_t)(d0 < 0 ? -d0 : 0), whole = (uint8_t)(n - 1), take;

        if (whole > (uint8_t)(MK_STRIDE - d0))
            whole = (uint8_t)(MK_STRIDE - d0);
        take = (uint8_t)(whole - skip);
        for (; row < rows; row++, src += stride, dst += MK_STRIDE) {
            memcpy(dst + d0 + skip, src + skip, take);
            if (d0 + whole < MK_STRIDE)
                dst[d0 + whole] = (uint8_t)((dst[d0 + whole] & ~last_m) | (src[whole] & last_m));
        }
        return;
    }
    for (; row < rows; row++, src += stride, dst += MK_STRIDE) {
        uint8_t cv = 0, cm = 0, k;
        int8_t d = d0;

        for (k = 0; k < n; k++, d++) {
            uint8_t v, m, ov, om;

            if (flags & MK_FLIP) {
                /* The eight columns before w - 8k, reversed; the first
                   of them is at least -7 before the picture's. */
                int16_t c = (int16_t)(sx + w - 8 - (k << 3));

                v = reverse8(fetch8(src - sb, stride, c));
                m = msk ? reverse8(fetch8(msk - sb, stride, c)) : 0xff;
            } else if (t == 0) {
                v = src[k];
                m = msk ? msk[k] : 0xff;
            } else {
                v = (uint8_t)(src[k] << t);
                m = msk ? (uint8_t)(msk[k] << t) : 0xff;
                if ((uint8_t)(k + 1) < avail) {
                    v |= (uint8_t)(src[k + 1] >> (8 - t));
                    if (msk)
                        m |= (uint8_t)(msk[k + 1] >> (8 - t));
                }
            }
            if (k == n - 1)
                m &= last_m;
            if (s == 0) {
                ov = v;
                om = m;
            } else {
                ov = (uint8_t)(cv | (v >> s));
                om = (uint8_t)(cm | (m >> s));
                cv = (uint8_t)(v << (8 - s));
                cm = (uint8_t)(m << (8 - s));
            }
            if (d >= 0 && d < MK_STRIDE)
                dst[d] = (uint8_t)((dst[d] & ~om) | (ov & om));
        }
        if (s != 0 && d >= 0 && d < MK_STRIDE)
            dst[d] = (uint8_t)((dst[d] & ~cm) | (cv & cm));
        if (msk)
            msk += stride;
    }
}

/* A picture, with a mask of the same layout or none, mirrored left to
   right with MK_FLIP; clipped to the frame. */
MK_BLIT void frame_blit(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t *bits, const uint8_t *mask,
                        uint8_t flags)
{
    blit_rows(x, y, w, h, bits, mask, (uint8_t)((w + 7) >> 3), 0, 0, flags);
}

/* Part of one: `w` by `h` of it from (sx, sy), at (x, y). */
MK_BLIT void frame_blit_part(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t *bits, uint8_t stride,
                             uint8_t sx, uint8_t sy)
{
    blit_rows(x, y, w, h, bits, 0, stride, sx, sy, 0);
}

#ifdef MK_BLIT_PAGES
MK_BLIT void frame_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    int16_t dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int16_t sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = (int16_t)(dx - dy);

    for (;;) {
        int16_t e2;

        plot(x0, y0, 1);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = (int16_t)(err * 2);
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* MIDP's drawRect: an outline w + 1 by h + 1. */
MK_BLIT void frame_rect(int16_t x, int16_t y, uint8_t w, uint8_t h)
{
    frame_line(x, y, (int16_t)(x + w), y);
    frame_line(x, (int16_t)(y + h), (int16_t)(x + w), (int16_t)(y + h));
    frame_line(x, y, x, (int16_t)(y + h));
    frame_line((int16_t)(x + w), y, (int16_t)(x + w), (int16_t)(y + h));
}

/* MIDP's drawArc of half an ellipse in the box w + 1 by h + 1 at (x, y):
   the upper half, or the lower; column by column, joined into a line.
   Each column's height r is the integer root of s = (4a^2 - dx^2) b^2 /
   a^2, found from the column before's (a circle's needs no division). */
MK_BLIT void frame_arc(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t lower)
{
    uint16_t a2 = (uint16_t)w * w, b2 = (uint16_t)h * h, r = 0;
    int16_t cx2 = (int16_t)(2 * x + w), cy2 = (int16_t)(2 * y + h), px, last_py = -1;

    for (px = x; px <= x + w; px++) {
        int16_t dx2 = (int16_t)(2 * px - cx2);
        uint16_t t = (uint16_t)(4 * a2 - (uint16_t)(dx2 * dx2)), s;
        int16_t py16;

        s = w == h ? t : (uint16_t)(((uint32_t)t * b2) / a2);
        while (r * r > s)
            r--;
        while ((uint16_t)((r + 1) * (r + 1)) <= s)
            r++;
        py16 = (int16_t)((lower ? cy2 + (int16_t)r + 1 : cy2 - (int16_t)r) / 2);
        if (last_py != -1)
            frame_line((int16_t)(px - 1), last_py, px, py16);
        else
            plot(px, py16, 1);
        last_py = py16;
    }
}
#endif

#endif
