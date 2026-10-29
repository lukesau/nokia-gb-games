/* Munkiki's Castles: the picture, after the JAR's class c, which draws
   the world through the phone's M3D (a software OpenGL ES in fixed point)
   and lays Munkiki and the effects over it. The M3D rasterizer was not
   read: this is the port's own, following the JAR's geometry, camera and
   grays, so the picture is the same scene from the same place, not the
   same pixels. What is known of the phone's output (the dither patterns,
   the horizon's row, where the floor's edges fall) matches.

   Lengths: the 3D world's unit (a cell is 20) in sixteenths; eye space is
   right-handed with the camera looking down -z, as in GL. The screen is
   96 by 65: x = 48 + 48 x/d, y = 32.5 - 49 y/d, d the depth, from the
   JAR's frustum (3, 1000) over (-3, 3) by (-65/32, 65/32); the rows
   would be 48 y/d by that, but 49 is what puts every floor edge of the
   phone's frames on its row (its M3D rounds somewhere). Faces are
   sorted and drawn back to front (there is no room for a depth buffer on
   the Game Boy): every box is axis-aligned and boxes never overlap, so
   one is in front of another when the eye is on its side of a plane
   that separates them (munkiki_scene.c orders and draws the world
   through this file's meshes). */
#include "munkiki_int.h"

#ifdef __SDCC
#pragma opt_code_size /* the bank is full; the time goes in the helpers, not the straight code */
#endif

#include <string.h>



/* A quarter turn is 64 of these. */
#define ANGLE_TURN 256


/* sin of 0 to 90 degrees in 64 steps, 1.14. */
static const int16_t sine_table[65] = {
    0, 402, 804, 1205, 1606, 2006, 2404, 2801, 3196, 3590, 3981, 4370, 4756, 5139, 5520, 5897,
    6270, 6639, 7005, 7366, 7723, 8076, 8423, 8765, 9102, 9434, 9760, 10080, 10394, 10702, 11003, 11297,
    11585, 11866, 12140, 12406, 12665, 12916, 13160, 13395, 13623, 13842, 14053, 14256, 14449, 14635, 14811, 14978,
    15137, 15286, 15426, 15557, 15679, 15791, 15893, 15986, 16069, 16143, 16207, 16261, 16305, 16340, 16364, 16379,
    16384
};


static int16_t sine(uint8_t angle)
{
    angle &= ANGLE_TURN - 1;
    if (angle < 64)
        return sine_table[angle];
    if (angle < 128)
        return sine_table[128 - angle];
    if (angle < 192)
        return (int16_t)-sine_table[angle - 128];
    return (int16_t)-sine_table[256 - angle];
}


static int16_t cosine(uint8_t angle)
{
    return sine((uint8_t)(angle + 64));
}


/* The camera (munkiki_camera.c): eye = rot · (p - at) + move, p and the
   results in sixteenths of a unit. */
#include "munkiki_3d_int.h"
#define rot mk3d_rot
#define at_x mk3d_at_x
#define at_y mk3d_at_y
#define at_z mk3d_at_z
#define move_x mk3d_move_x
#define move_y mk3d_move_y
#define move_z mk3d_move_z


/* Products from eight-bit ones: SDCC's 32-bit multiply costs some ten
   times more on the Game Boy, its 32-bit division sixty times, so the
   rasterizer keeps to these and to 16-bit divisions, mostly by
   reciprocals. */
#if defined(__SDCC) && !defined(MK_NO_ASM)
/* On the Game Boy, in assembly through these, since SDCC's own 32-bit
   multiply costs some 2700 cycles and this some 1200: sixteen rounds of
   shift and add, the product gathering in de:hl as de shifts out. */
static uint16_t mul_x, mul_y;
static uint32_t mul_r;

static void mul_asm(void) __naked
{
    __asm
        ld a, (#_mul_x)
        ld e, a
        ld a, (#_mul_x + 1)
        ld d, a
        ld a, (#_mul_y)
        ld c, a
        ld a, (#_mul_y + 1)
        ld b, a
        ld hl, #0
        ld a, #16
    00001$:
        add hl, hl
        rl e
        rl d
        jr nc, 00002$
        add hl, bc
        jr nc, 00002$
        inc de
    00002$:
        dec a
        jr nz, 00001$
        ld a, l
        ld (#_mul_r), a
        ld a, h
        ld (#_mul_r + 1), a
        ld a, e
        ld (#_mul_r + 2), a
        ld a, d
        ld (#_mul_r + 3), a
        ret
    __endasm;
}

static uint32_t mulu16(uint16_t a, uint16_t b)
{
    mul_x = a;
    mul_y = b;
    mul_asm();
    return mul_r;
}
#else
static uint32_t mulu16(uint16_t a, uint16_t b)
{
    return (uint32_t)a * b;
}
#endif


static int32_t mul16(int16_t a, int16_t b)
{
    uint32_t r = mulu16((uint16_t)(a < 0 ? -a : a), (uint16_t)(b < 0 ? -b : b));

    return (a < 0) != (b < 0) ? -(int32_t)r : (int32_t)r;
}

/* num / den for den under 32768, with the remainder left in div_rem.
   SDCC's own 16-bit division costs some 6000 cycles on the Game Boy;
   this, sixteen rounds of shift and subtract, under a thousand. */
#ifdef __SDCC
static uint16_t div_num, div_den, div_quo, div_rem;
static uint8_t div_i;

static void div_asm(void) __naked
{
    __asm
        ld a, (#_div_den)
        ld e, a
        ld a, (#_div_den + 1)
        ld d, a
        ld a, (#_div_num)
        ld c, a
        ld a, (#_div_num + 1)
        ld b, a
        ld hl, #0
        ld a, #16
        ld (#_div_i), a
    00001$:
        sla c
        rl b
        ld a, l
        rla
        ld l, a
        ld a, h
        rla
        ld h, a
        ld a, l
        sub a, e
        ld a, h
        sbc a, d
        jr c, 00002$
        ld a, l
        sub a, e
        ld l, a
        ld a, h
        sbc a, d
        ld h, a
        inc c
    00002$:
        ld a, (#_div_i)
        dec a
        ld (#_div_i), a
        jr nz, 00001$
        ld a, c
        ld (#_div_quo), a
        ld a, b
        ld (#_div_quo + 1), a
        ld a, l
        ld (#_div_rem), a
        ld a, h
        ld (#_div_rem + 1), a
        ret
    __endasm;
}

static uint16_t udiv16(uint16_t num, uint16_t den)
{
    div_num = num;
    div_den = den;
    div_asm();
    return div_quo;
}
#else
static uint16_t div_rem;

static uint16_t udiv16(uint16_t num, uint16_t den)
{
    div_rem = (uint16_t)(num % den);
    return (uint16_t)(num / den);
}
#endif

/* 65536 / n, near enough, for n of 16 bits: for the slopes of edges,
   where a little error moves an edge the same for both faces on it. */
static uint16_t recip(uint16_t n)
{
    return n < 3 ? 32767 : udiv16(65535u, n); /* kept to 15 bits for mul16 */
}

/* 256 x / d exactly, for x <= d < 32768: eight steps of long division in
   sixteen bits. */
static uint16_t div256(uint16_t x, uint16_t d)
{
    uint8_t i, q = 0;

    if (x >= d)
        return 256;
    for (i = 8; i; i--) {
        x <<= 1;
        q <<= 1;
        if (x >= d) {
            x -= d;
            q |= 1;
        }
    }
    return q;
}

/* num / den exactly, den < 32768: long division with a sixteen-bit
   remainder, which is what the Game Boy can do quickly. */
static uint32_t udiv32(uint32_t num, uint16_t den)
{
    uint16_t rem = 0, hi = (uint16_t)(num >> 16), lo = (uint16_t)num, qhi = 0, qlo = 0;
    uint8_t i;

    if (hi) {
        for (i = 16; i; i--) {
            rem = (uint16_t)((rem << 1) | (hi >> 15));
            hi <<= 1;
            qhi <<= 1;
            if (rem >= den) {
                rem -= den;
                qhi |= 1;
            }
        }
    }
    for (i = 16; i; i--) {
        rem = (uint16_t)((rem << 1) | (lo >> 15));
        lo <<= 1;
        qlo <<= 1;
        if (rem >= den) {
            rem -= den;
            qlo |= 1;
        }
    }
    return ((uint32_t)qhi << 16) | qlo;
}


/* m is 1.14; x, y and z fit sixteen bits. */
static void matrix_apply(int32_t *out, const int16_t *m, int32_t x, int32_t y, int32_t z)
{
    int16_t x16 = (int16_t)x, y16 = (int16_t)y, z16 = (int16_t)z;

    out[0] = (mul16(m[0], x16) + mul16(m[1], y16) + mul16(m[2], z16)) >> FIX;
    out[1] = (mul16(m[3], x16) + mul16(m[4], y16) + mul16(m[5], z16)) >> FIX;
    out[2] = (mul16(m[6], x16) + mul16(m[7], y16) + mul16(m[8], z16)) >> FIX;
}


/* A vertex in eye space, sixteenths of a unit (which fit sixteen bits:
   nothing is further than a few hundred units), with its texture
   coordinates, 0 to 255 across the texture. */
struct vertex {
    int16_t x, y, z;
    int16_t u, v;
};


/* The dither: 4 by 4, indexed by the row counted from the screen's
   bottom and the column; a pixel is black where its value is under the
   shade's level. The patterns are bytes, two cells across. */
static const uint8_t bayer[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
static uint8_t pattern[4]; /* for the shade being drawn, by y & 3 */
static const uint8_t *texture; /* the texture being drawn, or null */
static uint8_t texture_size;   /* 32 or 16 */


static void shade_set(uint8_t gray)
{
    uint8_t level = (uint8_t)((255 - gray + 8) >> 4), y, x, p;

    for (y = 0; y < 4; y++) {
        p = 0;
        for (x = 0; x < 8; x++)
            if (bayer[(63 - y) & 3][x & 3] < level)
                p |= (uint8_t)(0x80 >> x);
        pattern[y] = p;
    }
}


static void shade_black(void)
{
    pattern[0] = pattern[1] = pattern[2] = pattern[3] = 0xff;
}


/* Fills pixels x0 to x1 - 1 of row y with the pattern. */
/* The row being filled: where it is in the frame and its dither byte,
   kept by the triangle filler. Statics throughout the filling: SDCC
   reaches one far more cheaply than a local or a field through a
   pointer, and these loops are where the Game Boy's time goes. */
static uint8_t *row_at;
static uint8_t row_pattern;

static void span(int16_t x0, int16_t x1)
{
    uint8_t *row = row_at;
    uint8_t p = row_pattern, first, last, m;

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


/* A textured span: u and v at x0 and x1 in 8.8 across the texture, which
   is black where its bit is set, else white. */
/* A textured span: u and v at x0 and x1 in 8.8 across the texture, which
   is black where its bit is set, else white. */
static void span_textured(int16_t x0, int16_t x1, int32_t u0, int32_t v0, int32_t u1, int32_t v1)
{
    uint8_t *row = row_at;
    int16_t n = (int16_t)(x1 - x0);
    uint16_t r, u, v, du, dv;
    uint8_t x, end, mask = (uint8_t)(texture_size - 1), wide = texture_size == 32, bit;

    if (n <= 0 || x0 >= MK_W || x1 <= 0)
        return;
    r = recip((uint16_t)n);
    /* u and v in 8.8, kept to sixteen bits: only (u >> 8) & mask is read,
       so their top bits never matter, and the steps may wrap. */
    du = (uint16_t)(mul16((int16_t)((u1 - u0) >> 4), (int16_t)r) >> 12);
    dv = (uint16_t)(mul16((int16_t)((v1 - v0) >> 4), (int16_t)r) >> 12);
    u = (uint16_t)u0;
    v = (uint16_t)v0;
    if (x0 < 0) {
        /* Skip to the first pixel on the screen. */
        u = (uint16_t)(u + mul16((int16_t)-x0, (int16_t)du));
        v = (uint16_t)(v + mul16((int16_t)-x0, (int16_t)dv));
        x0 = 0;
    }
    if (x1 > MK_W)
        x1 = MK_W;
    x = (uint8_t)x0;
    end = (uint8_t)x1;
    bit = (uint8_t)(0x80 >> (x & 7));
    row += x >> 3;
    for (; x < end; x++, u += du, v += dv) {
        uint8_t tx = (uint8_t)((u >> 8) & mask), ty = (uint8_t)((v >> 8) & mask);
        uint8_t at = (uint8_t)((wide ? ty << 2 : ty << 1) + (tx >> 3));

        if (texture[at] & (0x80 >> (tx & 7)))
            *row |= bit;
        else
            *row &= (uint8_t)~bit;
        bit >>= 1;
        if (!bit) {
            bit = 0x80;
            row++;
        }
    }
}


/* A point on the screen, 12.4 fixed, with texture coordinates 8.8. */
struct point {
    int16_t x, y;
    int32_t u, v;
};


/* An edge of a triangle being filled, walked a row at a time: x in 12.4
   with a Bresenham remainder, so that each row's x is exactly p.x + dx s
   / n as a division would give it; u and v in 8.8 with their steps per
   row. The divisions are 16-bit and once an edge: n, the edge's height,
   and dx are under 2048, and the first row's centre is under a row below
   p unless p is above the screen. The two edges are statics, L and R. */
struct edge {
    int16_t x, q, rem, err, n, dir;
    int32_t u, du, v, dv;
};
static struct edge L, R;

static void edge_set(struct edge *e, const struct point *p, const struct point *q, int32_t yc)
{
    int16_t n = (int16_t)(q->y - p->y), s = (int16_t)(yc - p->y), dx, acc;
    uint16_t adx, d;

    if (n <= 0) {
        e->x = q->x;
        e->q = e->rem = e->err = 0;
        e->n = 1;
        e->dir = 0;
        e->u = q->u;
        e->v = q->v;
        e->du = e->dv = 0;
        return;
    }
    dx = (int16_t)(q->x - p->x);
    adx = (uint16_t)(dx < 0 ? -dx : dx);
    e->n = n;
    d = (uint16_t)(adx << 4);
    e->q = (int16_t)udiv16(d, (uint16_t)n);
    e->rem = (int16_t)div_rem;
    if (s < 16) {
        d = (uint16_t)(adx * (uint16_t)s);
        acc = (int16_t)udiv16(d, (uint16_t)n);
        e->err = (int16_t)div_rem;
    } else {
        uint32_t big = mulu16(adx, (uint16_t)s);

        acc = (int16_t)udiv32(big, (uint16_t)n);
        e->err = (int16_t)(big - mulu16((uint16_t)acc, (uint16_t)n));
    }
    if (dx < 0) {
        e->x = (int16_t)(p->x - acc);
        e->q = (int16_t)-e->q;
        e->dir = -1;
    } else {
        e->x = (int16_t)(p->x + acc);
        e->dir = 1;
    }
    if (texture) {
        int16_t du = 0, dv = 0;
        uint16_t r = recip((uint16_t)n);

        if (n >= 2) {
            du = (int16_t)(mul16((int16_t)((q->u - p->u) >> 4), (int16_t)r) >> 12);
            dv = (int16_t)(mul16((int16_t)((q->v - p->v) >> 4), (int16_t)r) >> 12);
        }
        e->u = p->u + mul16(du, s);
        e->v = p->v + mul16(dv, s);
        e->du = (int32_t)du << 4;
        e->dv = (int32_t)dv << 4;
    }
}

#define EDGE_STEP(E) \
    do { \
        E.x += E.q; \
        E.err += E.rem; \
        if (E.err >= E.n) { \
            E.x += E.dir; \
            E.err -= E.n; \
        } \
    } while (0)

#ifdef __SDCC
/* The rows of an untextured triangle, in assembly: fill_count rows from
   row_at (row_y for the dither), between L and R stepped as EDGE_STEP
   does, R left of L when fill_neg. SDCC's own code for this took some
   eight thousand cycles a row. */
static uint8_t fill_count, fill_neg, row_y;
static uint8_t span_mf, span_ml;
static const uint8_t first_mask[8] = { 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01 };
static const uint8_t last_mask[8] = { 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };

static void fill_rows(void) __naked
{
    __asm
        ld a, (#_fill_count)
        or a, a
        ret z
    00001$:
        ;; the row's dither byte
        ld a, (#_row_y)
        and a, #3
        ld c, a
        ld b, #0
        ld hl, #_pattern
        add hl, bc
        ld a, (hl)
        ld (#_row_pattern), a
        ;; bc = the left edge's x, de = the right's
        ld a, (#_fill_neg)
        or a, a
        jr z, 00002$
        ld a, (#_R + 0)
        ld c, a
        ld a, (#_R + 1)
        ld b, a
        ld a, (#_L + 0)
        ld e, a
        ld a, (#_L + 1)
        ld d, a
        jr 00003$
    00002$:
        ld a, (#_L + 0)
        ld c, a
        ld a, (#_L + 1)
        ld b, a
        ld a, (#_R + 0)
        ld e, a
        ld a, (#_R + 1)
        ld d, a
    00003$:
        ;; pixels: (x + 7) >> 4, arithmetic
        ld hl, #7
        add hl, bc
        sra h
        rr l
        sra h
        rr l
        sra h
        rr l
        sra h
        rr l
        push hl
        ld hl, #7
        add hl, de
        sra h
        rr l
        sra h
        rr l
        sra h
        rr l
        sra h
        rr l
        ld e, l
        ld d, h
        pop bc
        call span_asm
        ;; the edges on
        ld a, (#_L + 2)
        ld e, a
        ld a, (#_L + 3)
        ld d, a
        ld a, (#_L + 0)
        add a, e
        ld (#_L + 0), a
        ld a, (#_L + 1)
        adc a, d
        ld (#_L + 1), a
        ld a, (#_L + 4)
        ld e, a
        ld a, (#_L + 5)
        ld d, a
        ld a, (#_L + 6)
        add a, e
        ld c, a
        ld a, (#_L + 7)
        adc a, d
        ld b, a
        ld a, (#_L + 8)
        ld e, a
        ld a, (#_L + 9)
        ld d, a
        ld a, c
        sub a, e
        ld l, a
        ld a, b
        sbc a, d
        jr c, 00029$
        ld (#_L + 7), a
        ld a, l
        ld (#_L + 6), a
        ld a, (#_L + 10)
        ld e, a
        ld a, (#_L + 11)
        ld d, a
        ld a, (#_L + 0)
        add a, e
        ld (#_L + 0), a
        ld a, (#_L + 1)
        adc a, d
        ld (#_L + 1), a
        jr 00028$
    00029$:
        ld a, c
        ld (#_L + 6), a
        ld a, b
        ld (#_L + 7), a
    00028$:
        ld a, (#_R + 2)
        ld e, a
        ld a, (#_R + 3)
        ld d, a
        ld a, (#_R + 0)
        add a, e
        ld (#_R + 0), a
        ld a, (#_R + 1)
        adc a, d
        ld (#_R + 1), a
        ld a, (#_R + 4)
        ld e, a
        ld a, (#_R + 5)
        ld d, a
        ld a, (#_R + 6)
        add a, e
        ld c, a
        ld a, (#_R + 7)
        adc a, d
        ld b, a
        ld a, (#_R + 8)
        ld e, a
        ld a, (#_R + 9)
        ld d, a
        ld a, c
        sub a, e
        ld l, a
        ld a, b
        sbc a, d
        jr c, 00039$
        ld (#_R + 7), a
        ld a, l
        ld (#_R + 6), a
        ld a, (#_R + 10)
        ld e, a
        ld a, (#_R + 11)
        ld d, a
        ld a, (#_R + 0)
        add a, e
        ld (#_R + 0), a
        ld a, (#_R + 1)
        adc a, d
        ld (#_R + 1), a
        jr 00038$
    00039$:
        ld a, c
        ld (#_R + 6), a
        ld a, b
        ld (#_R + 7), a
    00038$:
        ;; the next row
        ld a, (#_row_at)
        add a, #12
        ld (#_row_at), a
        ld a, (#_row_at + 1)
        adc a, #0
        ld (#_row_at + 1), a
        ld a, (#_row_y)
        inc a
        ld (#_row_y), a
        ld a, (#_fill_count)
        dec a
        ld (#_fill_count), a
        jp nz, 00001$
        ret

        ;; Pixels bc to de - 1 of the row at row_at, in row_pattern;
        ;; bc and de signed, clipped to the row.
    span_asm:
        bit 7, b
        jr z, 00011$
        ld bc, #0
    00011$:
        bit 7, d
        ret nz
        ld a, d
        or a, a
        jr nz, 00012$
        ld a, e
        cp a, #97
        jr c, 00013$
    00012$:
        ld e, #96
    00013$:
        ld a, b
        or a, a
        ret nz
        ld a, c
        cp a, e
        ret nc
        ;; c = x0, e = x1: the masks of the first and last bytes
        ld a, c
        and a, #7
        ld l, a
        ld h, #0
        ld de, #_first_mask
        add hl, de
        ld a, (hl)
        ld (#_span_mf), a
        ld a, e
        dec a
        ld e, a
        and a, #7
        ld l, a
        ld h, #0
        ld de, #_last_mask
        add hl, de
        ld a, (hl)
        ld (#_span_ml), a
        ;; b = first byte, d = last byte
        ld a, c
        srl a
        srl a
        srl a
        ld b, a
        ld a, e
        srl a
        srl a
        srl a
        ld d, a
        ;; hl = row_at + first
        ld a, (#_row_at)
        add a, b
        ld l, a
        ld a, (#_row_at + 1)
        adc a, #0
        ld h, a
        ld a, (#_row_pattern)
        ld c, a
        ld a, d
        cp a, b
        jr nz, 00015$
        ;; one byte: both masks
        ld a, (#_span_mf)
        ld e, a
        ld a, (#_span_ml)
        and a, e
        ld e, a
        cpl
        and a, (hl)
        ld d, a
        ld a, c
        and a, e
        or a, d
        ld (hl), a
        ret
    00015$:
        ;; the first byte
        ld a, (#_span_mf)
        ld e, a
        cpl
        and a, (hl)
        ld (hl), a
        ld a, c
        and a, e
        or a, (hl)
        ld (hl+), a
        ;; the bytes between: d - b - 1 of them
        ld a, d
        sub a, b
        dec a
        jr z, 00017$
        ld b, a
    00016$:
        ld a, c
        ld (hl+), a
        dec b
        jr nz, 00016$
    00017$:
        ;; the last byte
        ld a, (#_span_ml)
        ld e, a
        cpl
        and a, (hl)
        ld (hl), a
        ld a, c
        and a, e
        or a, (hl)
        ld (hl), a
        ret
    __endasm;
}
#endif

/* Rasterizes a triangle: a row is covered where its centre (y + 1/2)
   is inside, a pixel where its centre is. `neg` says whether the turn
   a, b, c is negative on the screen (whose y goes down), which the
   callers know already. */
static void triangle(const struct point *a, const struct point *b, const struct point *c, uint8_t neg)
{
    const struct point *t;
    int16_t y0, y1, y;
    uint8_t on_ab;

    if (a->y > b->y) {
        t = a;
        a = b;
        b = t;
        neg = !neg;
    }
    if (b->y > c->y) {
        t = b;
        b = c;
        c = t;
        neg = !neg;
    }
    if (a->y > b->y) {
        t = a;
        a = b;
        b = t;
        neg = !neg;
    }
    y0 = (int16_t)((a->y + 7) >> 4);
    y1 = (int16_t)(((c->y + 7) >> 4) - 1);
    if (y0 < 0)
        y0 = 0;
    if (y1 >= MK_H)
        y1 = MK_H - 1;
    if (y0 > y1 || c->y == a->y)
        return;
    /* The long edge, a to c, is L; the short one for the first row is R.
       The middle vertex is left of the long edge when the turn a, b, c
       is negative: R is then the left edge. */
    edge_set(&L, a, c, ((int32_t)y0 << 4) + 8);
    on_ab = (((int32_t)y0 << 4) + 8) < b->y;
    edge_set(&R, on_ab ? a : b, on_ab ? b : c, ((int32_t)y0 << 4) + 8);
    row_at = mk_frame + y0 * MK_STRIDE;
#ifdef __SDCC
    if (!texture) {
        /* The rows down to the middle vertex, then the rest. */
        int16_t ymid = on_ab ? (int16_t)((b->y + 7) >> 4) : y0;

        if (ymid > y1 + 1)
            ymid = (int16_t)(y1 + 1);
        row_y = (uint8_t)y0;
        fill_neg = neg;
        fill_count = (uint8_t)(ymid - y0);
        fill_rows();
        if (on_ab) {
            edge_set(&R, b, c, ((int32_t)ymid << 4) + 8);
            fill_count = (uint8_t)(y1 + 1 - ymid);
            fill_rows();
        }
        return;
    }
#endif
    for (y = y0; y <= y1; y++, row_at += MK_STRIDE) {
        int16_t px0, px1;

        if (on_ab && (((int32_t)y << 4) + 8) >= b->y) {
            edge_set(&R, b, c, ((int32_t)y << 4) + 8);
            on_ab = 0;
        }
        row_pattern = pattern[y & 3];
        if (neg) {
            px0 = (int16_t)((R.x + 7) >> 4);
            px1 = (int16_t)((L.x + 7) >> 4);
            if (texture)
                span_textured(px0, px1, R.u, R.v, L.u, L.v);
            else
                span(px0, px1);
        } else {
            px0 = (int16_t)((L.x + 7) >> 4);
            px1 = (int16_t)((R.x + 7) >> 4);
            if (texture)
                span_textured(px0, px1, L.u, L.v, R.u, R.v);
            else
                span(px0, px1);
        }
        EDGE_STEP(L);
        EDGE_STEP(R);
        if (texture) {
            L.u += L.du;
            L.v += L.dv;
            R.u += R.du;
            R.v += R.dv;
        }
    }
}


/* The turn of three points on the screen: negative for a front face. */
static int32_t turn(const struct point *p0, const struct point *p1, const struct point *p2)
{
    /* In whole pixels first, eight-bit products; the exact turn only
       when that is too close to call. */
    int8_t ax = (int8_t)((p1->x - p0->x) >> 4), ay = (int8_t)((p1->y - p0->y) >> 4);
    int8_t bx = (int8_t)((p2->x - p0->x) >> 4), by = (int8_t)((p2->y - p0->y) >> 4);
    int16_t t = (int16_t)(ax * by - bx * ay);
    /* The sixteenths left out can move it by under the sides' sum. */
    int16_t slack = (int16_t)((ax < 0 ? -ax : ax) + (ay < 0 ? -ay : ay) + (bx < 0 ? -bx : bx) + (by < 0 ? -by : by) + 1);

    if (t > slack || t < -slack)
        return t;
    return mul16((int16_t)(p1->x - p0->x), (int16_t)(p2->y - p0->y))
           - mul16((int16_t)(p2->x - p0->x), (int16_t)(p1->y - p0->y));
}


/* The frustum in eye space, d = -z: near d >= 48 (3 units), |x| <= d,
   |y| <= d (wider than the screen's 65 d / 96: the rows it adds are cut
   by the filler, and the planes then need no multiplying). A polygon is
   clipped against each plane in turn (Sutherland and Hodgman), and what
   is left is projected and drawn as a fan. */
#define NEAR 48
#define MAX_POLY 8 /* three vertices and one more a plane */


static int32_t plane_side(const struct vertex *v, uint8_t plane)
{
    switch (plane) {
    case 0:
        return -(int32_t)v->z - NEAR;
    case 1:
        return -(int32_t)v->z - v->x;
    case 2:
        return -(int32_t)v->z + v->x;
    case 3:
        return -(int32_t)v->z - v->y;
    default:
        return -(int32_t)v->z + v->y;
    }
}


/* a + (b - a) f, f in 0.15. */
static int16_t lerp1(int16_t a, int16_t b, uint16_t f)
{
    int32_t d = (int32_t)b - a;

    if (d < 0)
        return (int16_t)(a - (int32_t)(mulu16((uint16_t)-d, f) >> 15));
    return (int16_t)(a + (int32_t)(mulu16((uint16_t)d, f) >> 15));
}

static void lerp(struct vertex *out, const struct vertex *a, const struct vertex *b, int32_t sa, int32_t sb)
{
    /* The point where the side crosses zero: a + (b - a) sa / (sa - sb),
       the fraction found as 0.15 exactly. */
    int32_t n = sa - sb;
    uint8_t k = 0;
    uint16_t f;

    if (n < 0) {
        n = -n;
        sa = -sa;
    }
    while ((n >> k) > 32767)
        k++;
    f = (uint16_t)udiv32((uint32_t)(sa >> k) << 15, (uint16_t)(n >> k));
    out->x = lerp1(a->x, b->x, f);
    out->y = lerp1(a->y, b->y, f);
    out->z = lerp1(a->z, b->z, f);
    if (texture) {
        out->u = lerp1(a->u, b->u, f);
        out->v = lerp1(a->v, b->v, f);
    } else {
        out->u = out->v = 0;
    }
}


static uint8_t clip(struct vertex *poly, uint8_t count, uint8_t plane, struct vertex *out)
{
    static int32_t side[MAX_POLY];
    uint8_t i, n = 0;

    for (i = 0; i < count; i++)
        side[i] = plane_side(&poly[i], plane);
    for (i = 0; i < count; i++) {
        uint8_t j = i + 1 == count ? 0 : (uint8_t)(i + 1);
        int32_t sc = side[i], sn = side[j];

        if (sc >= 0)
            out[n++] = *poly;
        if ((sc >= 0) != (sn >= 0) && n < MAX_POLY)
            lerp(&out[n++], poly, &poly[j - i], sc, sn);
        poly++;
    }
    return n;
}


static void project(struct point *p, const struct vertex *v)
{
    int16_t d = (int16_t)-v->z, q;

    if (d < 1)
        d = 1;
    /* 768 x / d and 784 y / d, from 256 x / d; |x| and |y| are at most d
       after the clipping. */
    q = (int16_t)div256((uint16_t)(v->x < 0 ? -v->x : v->x), (uint16_t)d);
    if (v->x < 0)
        q = (int16_t)-q;
    p->x = (int16_t)(768 + q + (q << 1));
    q = (int16_t)div256((uint16_t)(v->y < 0 ? -v->y : v->y), (uint16_t)d);
    if (v->y < 0)
        q = (int16_t)-q;
    p->y = (int16_t)(520 - (((q << 5) + (q << 4) + q) >> 4));
    p->u = (int32_t)v->u << 8;
    p->v = (int32_t)v->v << 8;
}


/* Draws a triangle given in eye space, front faces only. */
#ifdef MK_STATE_AT
static __at(MK_SCRATCH_AT) struct vertex verts[20];
static __at(MK_SCRATCH_AT + 200) struct vertex poly_a[MAX_POLY];
static __at(MK_SCRATCH_AT + 280) struct vertex poly_b[MAX_POLY];
static __at(MK_SCRATCH_AT + 360) struct point pts[MAX_POLY];
static __at(MK_SCRATCH_AT + 456) uint8_t vmask[20];
static __at(MK_SCRATCH_AT + 476) int16_t vxy[20][2];
/* The box transform's doublings, in the clipping's room, which is idle
   while a mesh is placed. */
static __at(MK_SCRATCH_AT + 200) int16_t axis_pow[3][5][3];
#else
static struct vertex verts[20], poly_a[MAX_POLY], poly_b[MAX_POLY];
static struct point pts[MAX_POLY];
static uint8_t vmask[20];   /* the planes each vertex is outside, a bit each */
static int16_t vxy[20][2];  /* its projection, when inside them all */
static int16_t axis_pow[3][5][3]; /* an axis's rotated scale, doubled 0 to 4 times */
#endif


static void draw_triangle(const struct vertex *v0, const struct vertex *v1, const struct vertex *v2)
{
    struct vertex *a = poly_a, *b = poly_b;
    uint8_t n, i;
    int8_t ax, ay, az, bx, by, bz;
    int16_t nx, ny, nz;
    int32_t t;

    /* Culling, in eye space, in units of 128 so that the sides fit eight
       bits and their products sixteen: coarse, but a face this cannot
       tell is edge-on. */
    ax = (int8_t)((v1->x - v0->x) >> 7);
    ay = (int8_t)((v1->y - v0->y) >> 7);
    az = (int8_t)((v1->z - v0->z) >> 7);
    bx = (int8_t)((v2->x - v0->x) >> 7);
    by = (int8_t)((v2->y - v0->y) >> 7);
    bz = (int8_t)((v2->z - v0->z) >> 7);
    nx = (int16_t)(ay * bz - az * by);
    ny = (int16_t)(az * bx - ax * bz);
    nz = (int16_t)(ax * by - ay * bx);
    if (nx * (int16_t)(v0->x >> 7) + ny * (int16_t)(v0->y >> 7) + nz * (int16_t)(v0->z >> 7) >= 0)
        return;
    a[0] = *v0;
    a[1] = *v1;
    a[2] = *v2;
    n = 3;
    for (i = 0; i < 5; i++) {
        struct vertex *t;

        n = clip(a, n, i, b);
        if (n < 3)
            return;
        t = a;
        a = b;
        b = t;
    }
    for (i = 0; i < n; i++)
        project(&pts[i], &a[i]);
    /* The fan, each triangle's turn its own: three of the points can be
       in a line after the clipping. */
    for (i = 1; i + 1 < n; i++) {
        t = turn(&pts[0], &pts[i], &pts[i + 1]);
        if (t != 0)
            triangle(&pts[0], &pts[i], &pts[i + 1], t < 0);
    }
}


/* Which planes each of `count` placed vertices is outside, and the
   projection of those inside them all. */
/* Which planes each of `count` placed vertices is outside, and the
   projection of those inside them all. */
static void mesh_classify(uint8_t count)
{
    uint8_t i, m;

    for (i = 0; i < count; i++) {
        int16_t x = verts[i].x, y = verts[i].y, z = verts[i].z;

        m = 0;
        if (z > -NEAR)
            m |= 1;
        if (x > -z)
            m |= 2;
        if (x < z)
            m |= 4;
        if (y > -z)
            m |= 8;
        if (y < z)
            m |= 16;
        vmask[i] = m;
        if (m == 0) {
            struct point pt;

            project(&pt, &verts[i]);
            vxy[i][0] = pt.x;
            vxy[i][1] = pt.y;
        }
    }
}

/* A mesh: byte vertices, scaled by (sx, sy, sz) in 16.16 and moved to
   (ox, oy, oz) in sixteenths of a unit, both before the camera; the
   mesh's own rotation about y, if any, in binary angle `spin`; and the
   triangles. */
static void mesh_place(const int8_t *data, uint8_t count, int32_t sx, int32_t sy, int32_t sz, int32_t ox, int32_t oy,
                       int32_t oz, uint8_t spin, uint8_t spin_z, const int8_t *tex)
{
    uint8_t i;
    int32_t e[3];
    int16_t ss = sine(spin), sc = cosine(spin);
    int16_t zs = sine(spin_z), zc = cosine(spin_z);

    if (!spin && !spin_z) {
        /* A box: the camera's rotation of each axis, scaled, in
           sixteenths per unit of the data (kept from the last box when
           the camera and the scale are the same), and its doublings; then each vertex is
           the mesh's origin plus, for each axis, the doublings its
           datum's bits pick, added or taken away: no multiplying. */
        static int16_t ux_was, uy_was, uz_was;
        static uint8_t rot_was = 255; /* out of step with the serial at first */
        int32_t base[3];
        int16_t ux = (int16_t)(sx >> 12), uy = (int16_t)(sy >> 12), uz = (int16_t)(sz >> 12);
        uint8_t r, k;

        if (rot_was != mk3d_rot_serial || ux != ux_was || uy != uy_was || uz != uz_was) {
            for (r = 0; r < 3; r++) {
                axis_pow[0][0][r] = (int16_t)(mul16(rot[r * 3], ux) >> FIX);
                axis_pow[1][0][r] = (int16_t)(mul16(rot[r * 3 + 1], uy) >> FIX);
                axis_pow[2][0][r] = (int16_t)(mul16(rot[r * 3 + 2], uz) >> FIX);
            }
            for (k = 1; k < 5; k++)
                for (r = 0; r < 3; r++) {
                    axis_pow[0][k][r] = (int16_t)(axis_pow[0][k - 1][r] << 1);
                    axis_pow[1][k][r] = (int16_t)(axis_pow[1][k - 1][r] << 1);
                    axis_pow[2][k][r] = (int16_t)(axis_pow[2][k - 1][r] << 1);
                }
            ux_was = ux;
            uy_was = uy;
            uz_was = uz;
            rot_was = mk3d_rot_serial;
        }
        matrix_apply(base, rot, ox - at_x, oy - at_y, oz - at_z);
        base[0] += move_x;
        base[1] += move_y;
        base[2] += move_z;
        for (i = 0; i < count; i++) {
            int16_t vx = (int16_t)base[0], vy = (int16_t)base[1], vz = (int16_t)base[2];
            uint8_t axis;

            for (axis = 0; axis < 3; axis++) {
                int8_t d = data[i * 3 + axis];
                uint8_t m = (uint8_t)(d < 0 ? -d : d);
                const int16_t *pw = &axis_pow[axis][0][0];

                for (; m; m >>= 1, pw += 3) {
                    if (m & 1) {
                        if (d < 0) {
                            vx -= pw[0];
                            vy -= pw[1];
                            vz -= pw[2];
                        } else {
                            vx += pw[0];
                            vy += pw[1];
                            vz += pw[2];
                        }
                    }
                }
            }
            verts[i].x = vx;
            verts[i].y = vy;
            verts[i].z = vz;
            verts[i].u = verts[i].v = 0;
        }
        mesh_classify(count);
        return;
    }
    for (i = 0; i < count; i++) {
        int32_t x = ((int32_t)data[i * 3] * sx) >> 12;     /* sixteenths of a unit */
        int32_t y = ((int32_t)data[i * 3 + 1] * sy) >> 12;
        int32_t z = ((int32_t)data[i * 3 + 2] * sz) >> 12;

        if (spin_z) {
            int32_t rx = (mul16((int16_t)x, zc) - mul16((int16_t)y, zs)) >> FIX;
            int32_t ry = (mul16((int16_t)x, zs) + mul16((int16_t)y, zc)) >> FIX;

            x = rx;
            y = ry;
        }
        if (spin) {
            int32_t rx = (mul16((int16_t)x, sc) + mul16((int16_t)z, ss)) >> FIX;
            int32_t rz = (mul16((int16_t)z, sc) - mul16((int16_t)x, ss)) >> FIX;

            x = rx;
            z = rz;
        }
        matrix_apply(e, rot, x + ox - at_x, y + oy - at_y, z + oz - at_z);
        verts[i].x = (int16_t)(e[0] + move_x);
        verts[i].y = (int16_t)(e[1] + move_y);
        verts[i].z = (int16_t)(e[2] + move_z);
        if (tex) {
            verts[i].u = (int16_t)(tex[i * 2] * 255);
            verts[i].v = (int16_t)(tex[i * 2 + 1] * 255);
        } else {
            verts[i].u = verts[i].v = 0;
        }
    }
    mesh_classify(count);
}


static void mesh_draw(const int8_t *tris, uint8_t count)
{
    uint8_t i;

    for (i = 0; i < count; i += 3) {
        uint8_t i0 = (uint8_t)tris[i], i1 = (uint8_t)tris[i + 1], i2 = (uint8_t)tris[i + 2];

        if (vmask[i0] & vmask[i1] & vmask[i2])
            continue; /* all outside one plane */
        if ((vmask[i0] | vmask[i1] | vmask[i2]) == 0) {
            /* All inside: no clipping, the projections known. Front
               faces turn counter-clockwise on the screen, whose y goes
               down, so their turn comes out negative. */
            struct point p0, p1, p2;

            p0.x = vxy[i0][0];
            p0.y = vxy[i0][1];
            p1.x = vxy[i1][0];
            p1.y = vxy[i1][1];
            p2.x = vxy[i2][0];
            p2.y = vxy[i2][1];
            if (turn(&p0, &p1, &p2) >= 0)
                continue;
            if (texture) {
                p0.u = (int32_t)verts[i0].u << 8;
                p0.v = (int32_t)verts[i0].v << 8;
                p1.u = (int32_t)verts[i1].u << 8;
                p1.v = (int32_t)verts[i1].v << 8;
                p2.u = (int32_t)verts[i2].u << 8;
                p2.v = (int32_t)verts[i2].v << 8;
            }
            triangle(&p0, &p1, &p2, 1);
            continue;
        }
        draw_triangle(&verts[i0], &verts[i1], &verts[i2]);
    }
}


/* The backdrop (c.z): a triangle in eye space, the scene's (it holds
   the JAR's ground data), in the light gray. */
void mk3d_backdrop(const int16_t *v) MK_FAR
{
    uint8_t i;

    for (i = 0; i < 3; i++) {
        verts[i].x = v[i * 3];
        verts[i].y = v[i * 3 + 1];
        verts[i].z = v[i * 3 + 2];
        verts[i].u = verts[i].v = 0;
    }
    shade_set(0xe0);
    draw_triangle(&verts[0], &verts[1], &verts[2]);
}

void mk3d_shade(uint8_t gray) MK_FAR
{
    if (gray)
        shade_set(gray);
    else
        shade_black();
}

void mk3d_texture(uint8_t which) MK_FAR
{
    texture = which == MK_TEXTURE_NOTE ? munkiki_png_note : which == MK_TEXTURE_KEY ? munkiki_png_key
              : which == MK_TEXTURE_WARP ? munkiki_png_warp : which == MK_TEXTURE_LIFE ? munkiki_png_life : 0;
    texture_size = which == MK_TEXTURE_LIFE ? 16 : 32;
}

/* The meshes, by MK_MESH_ number. */
struct mesh_desc {
    const int8_t *verts;
    uint8_t count;
    const int8_t *tris;
    uint8_t tri_count;
    const int8_t *tex;
};
static int8_t warp_tex[8];
static const struct mesh_desc meshes[6] = {
    { munkiki_rim_verts, 6, munkiki_rim_tris, 12, 0 },
    { munkiki_top_verts, 4, munkiki_top_tris, 6, 0 },
    { munkiki_edge_verts, 20, munkiki_edge_tris, 72, 0 },
    { munkiki_cube_verts, 8, munkiki_cube_tris, 36, 0 },
    { munkiki_thing_verts, 4, munkiki_thing_tris, 12, munkiki_thing_tex },
    { munkiki_warp_verts, 4, munkiki_warp_tris, 12, warp_tex },
};

void mk3d_mesh(uint8_t which, int32_t sx, int32_t sy, int32_t sz, int32_t ox, int32_t oy, int32_t oz, uint8_t spin,
               uint8_t spin_z) MK_FAR
{
    const struct mesh_desc *m = &meshes[which];
    uint8_t i;

    if (which == MK_MESH_WARP) {
        /* The warp's texture coordinates are 0 to 4: one copy of it. */
        for (i = 0; i < 8; i++)
            warp_tex[i] = (int8_t)(munkiki_warp_tex[i] >> 2);
    } else {
        spin_z = 0;
        if (which != MK_MESH_THING)
            spin = 0;
    }
    mesh_place(m->verts, m->count, sx, sy, sz, ox, oy, oz, spin, spin_z, m->tex);
    mesh_draw(m->tris, m->tri_count);
}

uint8_t munkiki_shades_of(uint8_t kind) MK_FAR
{
    return munkiki_shades[kind - 1];
}
