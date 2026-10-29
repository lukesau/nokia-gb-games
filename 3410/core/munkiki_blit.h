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

/* A picture, with a mask of the same layout or none, mirrored left to
   right with MK_FLIP; clipped to the frame. */
#define MK_FLIP 1
MK_BLIT void frame_blit(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t *bits, const uint8_t *mask,
                        uint8_t flags)
{
    uint8_t stride = (uint8_t)((w + 7) >> 3), sx, sy;

    for (sy = 0; sy < h; sy++) {
        if (y + sy < 0 || y + sy >= MK_H)
            continue;
        for (sx = 0; sx < w; sx++) {
            uint8_t col = flags & MK_FLIP ? (uint8_t)(w - 1 - sx) : sx;

            if (mask && !pixel_of(mask, stride, col, sy))
                continue;
            plot((int16_t)(x + sx), (int16_t)(y + sy), pixel_of(bits, stride, col, sy));
        }
    }
}

/* Part of one: `w` by `h` of it from (sx, sy), at (x, y). */
MK_BLIT void frame_blit_part(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t *bits, uint8_t stride,
                             uint8_t sx, uint8_t sy)
{
    uint8_t i, j;

    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            plot((int16_t)(x + i), (int16_t)(y + j), pixel_of(bits, stride, (uint8_t)(sx + i), (uint8_t)(sy + j)));
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
   the upper half, or the lower; column by column, joined into a line. */
MK_BLIT void frame_arc(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t lower)
{
    int32_t a = w, b = h, cx2 = 2 * x + w, cy2 = 2 * y + h;
    int16_t px, last_py = -1;

    for (px = x; px <= x + w; px++) {
        int32_t dx2 = 2 * px - cx2, t = a * a * 4 - dx2 * dx2, r = 0, s, py;
        int16_t py16;

        if (t < 0)
            t = 0;
        s = t * b * b / (a * a);
        while ((r + 1) * (r + 1) <= s)
            r++;
        py = lower ? cy2 + r : cy2 - r;
        py16 = (int16_t)((py + (lower ? 1 : 0)) / 2);
        if (last_py != -1)
            frame_line((int16_t)(px - 1), last_py, px, py16);
        else
            plot(px, py16, 1);
        last_py = py16;
    }
}
#endif

#endif
