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
   that separates them. */
#include "munkiki_int.h"

#include <string.h>

#include "lcd.h"

uint8_t mk_frame[MK_STRIDE * MK_H];

/* Fixed point: 1.14 for the rotation, so that a product with a position
   in sixteenths stays in 32 bits. */
#define FIX 14
#define FIX_ONE (1L << FIX)
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

/* sin and cos of the look-around angles, -45 to 45 degrees by 5, 1.14. */
static const int16_t sine5[10] = { 0, 1428, 2845, 4240, 5604, 6924, 8192, 9397, 10531, 11585 };
static const int16_t cosine5[10] = { 16384, 16322, 16135, 15826, 15396, 14849, 14189, 13420, 12551, 11585 };

/* The pitch of 20 degrees. */
#define PITCH_SIN 5603
#define PITCH_COS 15396

/* c.t: the JAR's sine, of an angle in 16.16 radians, in 16.16. */
static int32_t jar_sine(int32_t n)
{
    int32_t n2 = 1, n3;

    if (n > 308830L) {
        n = 411774L - n;
        n2 = -1;
    } else if (n > 205887L) {
        n -= 205887L;
        n2 = -1;
    } else if (n > 102943L) {
        n = 205887L - n;
    }
    n3 = (n >> 8) * (n >> 8);
    return n2 * (((((n3 >> 8) - 10882L) >> 8) * (n3 >> 8) + 65536L) >> 8) * (n >> 8);
}

/* The camera: eye = rot · (p - at) + move, p and the results in
   sixteenths of a unit. */
static int16_t rot[9];
static int32_t at_x, at_y, at_z;
static int32_t move_x, move_y, move_z;
/* Where the eye is in the world, in sixteenths of a cell, for the order. */
static int16_t eye_x, eye_y, eye_z;

static void matrix_mul(int16_t *out, const int16_t *a, const int16_t *b)
{
    uint8_t i, j, k;

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            int32_t s = 0;

            for (k = 0; k < 3; k++)
                s += (int32_t)a[i * 3 + k] * b[k * 3 + j];
            out[i * 3 + j] = (int16_t)(s >> FIX);
        }
    }
}

static void matrix_apply(int32_t *out, const int16_t *m, int32_t x, int32_t y, int32_t z)
{
    out[0] = (m[0] * x + m[1] * y + m[2] * z) >> FIX;
    out[1] = (m[3] * x + m[4] * y + m[5] * z) >> FIX;
    out[2] = (m[6] * x + m[7] * y + m[8] * z) >> FIX;
}

/* glRotate about y by `angle` (binary), and about x by (s, c). */
static void rotation_y(int16_t *m, uint8_t angle)
{
    int16_t s = sine(angle), c = cosine(angle);

    m[0] = c;
    m[1] = 0;
    m[2] = s;
    m[3] = 0;
    m[4] = FIX_ONE;
    m[5] = 0;
    m[6] = (int16_t)-s;
    m[7] = 0;
    m[8] = c;
}

static void rotation_x(int16_t *m, int16_t s, int16_t c)
{
    m[0] = FIX_ONE;
    m[1] = 0;
    m[2] = 0;
    m[3] = 0;
    m[4] = c;
    m[5] = (int16_t)-s;
    m[6] = 0;
    m[7] = s;
    m[8] = c;
}

/* The tail of d.m: the camera for this frame. */
static void camera(void)
{
    int16_t ry[9], rx[9], tmp[9];
    int32_t v[3];
    int8_t deg;
    uint8_t yaw = (uint8_t)((mk.facing * 64 + 32768L) >> 16);
    int32_t py = mk.py < 65536L ? 65536L : mk.py;

    /* Munkiki's centre, (x, y + 0.5, z) cells, in sixteenths of a unit. */
    at_x = (mk.px * 20) >> 12;
    at_y = ((py + 32768L) * 20) >> 12;
    at_z = (mk.pz * 20) >> 12;
    rotation_y(ry, yaw);
    if (mk.phase != 1) {
        rotation_x(rx, PITCH_SIN, PITCH_COS);
        matrix_mul(rot, rx, ry);
        move_x = 0;
        move_y = -8;
        move_z = -320;
    } else {
        /* Looking around: T(0, -0.5, -2) Rx(c0) Ry(c1) T(0, 0, c0 / 8) Ry(yaw). */
        int16_t s, c;

        deg = (int8_t)(mk.look_yaw >> 16);
        s = deg < 0 ? (int16_t)-sine5[-deg / 5] : sine5[deg / 5];
        c = cosine5[(deg < 0 ? -deg : deg) / 5];
        tmp[0] = c;
        tmp[1] = 0;
        tmp[2] = s;
        tmp[3] = 0;
        tmp[4] = FIX_ONE;
        tmp[5] = 0;
        tmp[6] = (int16_t)-s;
        tmp[7] = 0;
        tmp[8] = c;
        deg = (int8_t)(mk.look_pitch >> 16);
        s = deg < 0 ? (int16_t)-sine5[-deg / 5] : sine5[deg / 5];
        c = cosine5[(deg < 0 ? -deg : deg) / 5];
        rotation_x(rx, s, c);
        matrix_mul(rot, rx, tmp);   /* Rx(c0) Ry(c1) */
        matrix_apply(v, rot, 0, 0, ((int32_t)deg * 16) / 8);
        move_x = v[0];
        move_y = v[1] - 8;
        move_z = v[2] - 32;
        memcpy(tmp, rot, sizeof tmp);
        matrix_mul(rot, tmp, ry);
    }
    /* The eye is where eye = 0: at - rot^T move, in sixteenths of a cell. */
    {
        int32_t ex = -((int32_t)rot[0] * move_x + rot[3] * move_y + rot[6] * move_z) >> FIX;
        int32_t ey = -((int32_t)rot[1] * move_x + rot[4] * move_y + rot[7] * move_z) >> FIX;
        int32_t ez = -((int32_t)rot[2] * move_x + rot[5] * move_y + rot[8] * move_z) >> FIX;

        eye_x = (int16_t)(((at_x + ex) * 4) / 5);
        eye_y = (int16_t)(((at_y + ey) * 4) / 5);
        eye_z = (int16_t)(((at_z + ez) * 4) / 5);
    }
}

/* A vertex in eye space, sixteenths of a unit, with its texture
   coordinates, 0 to 255 across the texture. */
struct vertex {
    int32_t x, y, z;
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
static void span(uint8_t y, int16_t x0, int16_t x1)
{
    uint8_t *row = mk_frame + y * MK_STRIDE;
    uint8_t p = pattern[y & 3], first, last, m;

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
static void span_textured(uint8_t y, int16_t x0, int16_t x1, int32_t u0, int32_t v0, int32_t u1, int32_t v1)
{
    uint8_t *row = mk_frame + y * MK_STRIDE;
    int32_t du, dv, u, v;
    int16_t x, n = (int16_t)(x1 - x0);
    uint8_t mask = (uint8_t)(texture_size - 1), stride = (uint8_t)(texture_size >> 3);

    if (n <= 0)
        return;
    du = (u1 - u0) / n;
    dv = (v1 - v0) / n;
    u = u0;
    v = v0;
    for (x = x0; x < x1; x++, u += du, v += dv) {
        if (x >= 0 && x < MK_W) {
            uint8_t tx = (uint8_t)((u >> 8) & mask), ty = (uint8_t)((v >> 8) & mask);
            uint8_t bit = (uint8_t)(0x80 >> (x & 7));

            if (texture[ty * stride + (tx >> 3)] & (0x80 >> (tx & 7)))
                row[x >> 3] |= bit;
            else
                row[x >> 3] &= (uint8_t)~bit;
        }
    }
}

/* A point on the screen, 12.4 fixed, with texture coordinates 8.8. */
struct point {
    int16_t x, y;
    int32_t u, v;
};

/* Rasterizes a triangle: a row is covered where its centre (y + 1/2)
   is inside, a pixel where its centre is. */
static void triangle(const struct point *a, const struct point *b, const struct point *c)
{
    const struct point *t;
    int16_t y0, y1, y;
    int32_t xl, xr, dl, dr, ul, vl, ur, vr, dul, dvl, dur, dvr;
    uint8_t mid_left;

    if (a->y > b->y) {
        t = a;
        a = b;
        b = t;
    }
    if (b->y > c->y) {
        t = b;
        b = c;
        c = t;
    }
    if (a->y > b->y) {
        t = a;
        a = b;
        b = t;
    }
    y0 = (int16_t)((a->y + 7) >> 4);
    y1 = (int16_t)(((c->y + 7) >> 4) - 1);
    if (y0 < 0)
        y0 = 0;
    if (y1 >= MK_H)
        y1 = MK_H - 1;
    if (y0 > y1 || c->y == a->y)
        return;
    /* Whether the middle vertex is left of the long edge. */
    mid_left = ((int32_t)(b->x - a->x) * (c->y - a->y) - (int32_t)(c->x - a->x) * (b->y - a->y)) < 0;
    for (y = y0; y <= y1; y++) {
        int32_t yc = ((int32_t)y << 4) + 8, s, n;
        const struct point *p, *q;

        /* The long edge, a to c. */
        n = c->y - a->y;
        s = yc - a->y;
        xl = a->x + ((int32_t)(c->x - a->x) * s) / n;
        ul = a->u + ((c->u - a->u) / n) * s;
        vl = a->v + ((c->v - a->v) / n) * s;
        /* The short edge for this row. */
        if (yc < b->y) {
            p = a;
            q = b;
        } else {
            p = b;
            q = c;
        }
        n = q->y - p->y;
        s = yc - p->y;
        if (n <= 0) {
            xr = q->x;
            ur = q->u;
            vr = q->v;
        } else {
            xr = p->x + ((int32_t)(q->x - p->x) * s) / n;
            ur = p->u + ((q->u - p->u) / n) * s;
            vr = p->v + ((q->v - p->v) / n) * s;
        }
        if (mid_left) {
            dl = xl;
            xl = xr;
            xr = dl;
            dul = ul;
            ul = ur;
            ur = dul;
            dvl = vl;
            vl = vr;
            vr = dvl;
        }
        {
            int16_t px0 = (int16_t)((xl + 7) >> 4), px1 = (int16_t)((xr + 7) >> 4);

            if (texture)
                span_textured((uint8_t)y, px0, px1, ul, vl, ur, vr);
            else
                span((uint8_t)y, px0, px1);
        }
    }
    (void)dr;
    (void)dur;
    (void)dvr;
}

/* The frustum in eye space, d = -z: near d >= 48 (3 units), |x| <= d,
   |y| <= 65 d / 96. A polygon is clipped against each plane in turn
   (Sutherland and Hodgman), and what is left is projected and drawn as a
   fan. */
#define NEAR 48
#define MAX_POLY 12

static int32_t plane_side(const struct vertex *v, uint8_t plane)
{
    switch (plane) {
    case 0:
        return -v->z - NEAR;
    case 1:
        return -v->z - v->x;
    case 2:
        return -v->z + v->x;
    case 3:
        return -v->z * 65 - v->y * 96;
    default:
        return -v->z * 65 + v->y * 96;
    }
}

static void lerp(struct vertex *out, const struct vertex *a, const struct vertex *b, int32_t sa, int32_t sb)
{
    /* The point where the side crosses zero: a + (b - a) sa / (sa - sb). */
    int32_t n = sa - sb;

    out->x = a->x + ((b->x - a->x) * sa) / n;
    out->y = a->y + ((b->y - a->y) * sa) / n;
    out->z = a->z + ((b->z - a->z) * sa) / n;
    out->u = (int16_t)(a->u + ((b->u - a->u) * sa) / n);
    out->v = (int16_t)(a->v + ((b->v - a->v) * sa) / n);
}

static uint8_t clip(struct vertex *poly, uint8_t count, uint8_t plane, struct vertex *out)
{
    uint8_t i, n = 0;

    for (i = 0; i < count; i++) {
        const struct vertex *cur = &poly[i], *next = &poly[(i + 1) % count];
        int32_t sc = plane_side(cur, plane), sn = plane_side(next, plane);

        if (sc >= 0)
            out[n++] = *cur;
        if ((sc >= 0) != (sn >= 0) && n < MAX_POLY)
            lerp(&out[n++], cur, next, sc, sn);
    }
    return n;
}

static void project(struct point *p, const struct vertex *v)
{
    int32_t d = -v->z;

    if (d < 1)
        d = 1;
    p->x = (int16_t)(768 + (768 * v->x) / d);
    p->y = (int16_t)(520 - (784 * v->y) / d);
    p->u = (int32_t)v->u << 8;
    p->v = (int32_t)v->v << 8;
}

/* Draws a triangle given in eye space, front faces only. */
static void draw_triangle(const struct vertex *v0, const struct vertex *v1, const struct vertex *v2)
{
    struct vertex a[MAX_POLY], b[MAX_POLY];
    struct point pts[MAX_POLY];
    uint8_t n, i;
    int32_t ax, ay, az, bx, by, bz, nx, ny, nz;

    /* Culling, in eye space, in whole units so that it fits. */
    ax = (v1->x - v0->x) >> 4;
    ay = (v1->y - v0->y) >> 4;
    az = (v1->z - v0->z) >> 4;
    bx = (v2->x - v0->x) >> 4;
    by = (v2->y - v0->y) >> 4;
    bz = (v2->z - v0->z) >> 4;
    nx = ay * bz - az * by;
    ny = az * bx - ax * bz;
    nz = ax * by - ay * bx;
    if (nx * (v0->x >> 4) + ny * (v0->y >> 4) + nz * (v0->z >> 4) >= 0)
        return;
    a[0] = *v0;
    a[1] = *v1;
    a[2] = *v2;
    n = 3;
    for (i = 0; i < 5; i++) {
        n = clip(a, n, i, b);
        if (n < 3)
            return;
        memcpy(a, b, n * sizeof *a);
    }
    for (i = 0; i < n; i++)
        project(&pts[i], &a[i]);
    for (i = 1; i + 1 < n; i++)
        triangle(&pts[0], &pts[i], &pts[i + 1]);
}

/* A mesh: byte vertices, scaled by (sx, sy, sz) in 16.16 and moved to
   (ox, oy, oz) in sixteenths of a unit, both before the camera; the
   mesh's own rotation about y, if any, in binary angle `spin`; and the
   triangles. */
static struct vertex verts[20];

static void mesh_place(const int8_t *data, uint8_t count, int32_t sx, int32_t sy, int32_t sz, int32_t ox, int32_t oy,
                       int32_t oz, uint8_t spin, uint8_t spin_z, const int8_t *tex)
{
    uint8_t i;
    int32_t e[3];
    int16_t ss = sine(spin), sc = cosine(spin);
    int16_t zs = sine(spin_z), zc = cosine(spin_z);

    for (i = 0; i < count; i++) {
        int32_t x = ((int32_t)data[i * 3] * sx) >> 12;     /* sixteenths of a unit */
        int32_t y = ((int32_t)data[i * 3 + 1] * sy) >> 12;
        int32_t z = ((int32_t)data[i * 3 + 2] * sz) >> 12;

        if (spin_z) {
            int32_t rx = (x * zc - y * zs) >> FIX, ry = (x * zs + y * zc) >> FIX;

            x = rx;
            y = ry;
        }
        if (spin) {
            int32_t rx = (x * sc + z * ss) >> FIX, rz = (-x * ss + z * sc) >> FIX;

            x = rx;
            z = rz;
        }
        matrix_apply(e, rot, x + ox - at_x, y + oy - at_y, z + oz - at_z);
        verts[i].x = e[0] + move_x;
        verts[i].y = e[1] + move_y;
        verts[i].z = e[2] + move_z;
        if (tex) {
            verts[i].u = (int16_t)(tex[i * 2] * 255);
            verts[i].v = (int16_t)(tex[i * 2 + 1] * 255);
        } else {
            verts[i].u = verts[i].v = 0;
        }
    }
}

static void mesh_draw(const int8_t *tris, uint8_t count)
{
    uint8_t i;

    for (i = 0; i < count; i += 3)
        draw_triangle(&verts[tris[i]], &verts[tris[i + 1]], &verts[tris[i + 2]]);
}

/* The drawables, in the order to draw them: a block or a thing, with its
   box in sixteenths of a cell. */
struct drawable {
    uint8_t index, thing;
    int16_t x0, x1, y0, y1, z0, z1;
};
static struct drawable order[MK_MAX_BLOCKS + MK_MAX_THINGS];
static uint8_t order_count;

/* Whether a must be drawn before b: b is nearer the eye along every axis
   that separates them. */
static uint8_t before(const struct drawable *a, const struct drawable *b)
{
    uint8_t separated = 0;

    if (b->x1 <= a->x0) {
        separated = 1;
        if (eye_x >= a->x0)
            return 0;
    } else if (b->x0 >= a->x1) {
        separated = 1;
        if (eye_x <= a->x1)
            return 0;
    }
    if (b->y1 <= a->y0) {
        separated = 1;
        if (eye_y >= a->y0)
            return 0;
    } else if (b->y0 >= a->y1) {
        separated = 1;
        if (eye_y <= a->y1)
            return 0;
    }
    if (b->z1 <= a->z0) {
        separated = 1;
        if (eye_z >= a->z0)
            return 0;
    } else if (b->z0 >= a->z1) {
        separated = 1;
        if (eye_z <= a->z1)
            return 0;
    }
    return separated;
}

static void order_add(uint8_t index, uint8_t thing, int16_t x0, int16_t x1, int16_t y0, int16_t y1, int16_t z0,
                      int16_t z1)
{
    struct drawable d;
    uint8_t i, place = 0;

    d.index = index;
    d.thing = thing;
    d.x0 = x0;
    d.x1 = x1;
    d.y0 = y0;
    d.y1 = y1;
    d.z0 = z0;
    d.z1 = z1;
    /* After the last that must come before it. */
    for (i = 0; i < order_count; i++)
        if (before(&order[i], &d))
            place = (uint8_t)(i + 1);
    memmove(&order[place + 1], &order[place], (order_count - place) * sizeof d);
    order[place] = d;
    order_count++;
}

static void block_draw(const struct mk_block *b, int32_t wobble)
{
    uint8_t kind = b->kind, floor = b->y == 0, top = floor;
    int32_t sx = 65536L, sy = 65536L, sz = 65536L, oy = 0;
    int32_t px = (int32_t)b->x * 20, py = (int32_t)b->y * 20, pz = (int32_t)b->z * 20;

    if (top) {
        if (kind == MK_BLOCK_BOUNCY && wobble > 0)
            top = 0;
        else if (kind == MK_BLOCK_WALL && b->sy > 1)
            top = 0;
        if (!top)
            oy = 320;
    }
    if (kind == MK_BLOCK_BOUNCY && !top) {
        sy = (floor ? 0 : 65536L) + wobble;
    } else if (kind == MK_BLOCK_WALL) {
        sx = (int32_t)b->sx << 16;
        sz = (int32_t)b->sz << 16;
        sy = top ? 65536L : ((int32_t)b->sy << 16) - (floor ? 65536L : 0);
    } else if (b->y < 16 && !top) {
        oy = 320 - py;
        sy = (int32_t)b->y << 12;
    }
    if (top) {
        shade_black();
        mesh_place(munkiki_rim_verts, 6, sx, sy, sz, px, py + oy, pz, 0, 0, 0);
        mesh_draw(munkiki_rim_tris, 12);
        shade_set(munkiki_shades[kind - 1]);
        mesh_place(munkiki_top_verts, 4, sx, sy, sz, px, py + oy, pz, 0, 0, 0);
        mesh_draw(munkiki_top_tris, 6);
    } else {
        shade_black();
        mesh_place(munkiki_edge_verts, 20, sx, sy, sz, px, py + oy, pz, 0, 0, 0);
        mesh_draw(munkiki_edge_tris, 72);
        shade_set(munkiki_shades[kind - 1]);
        mesh_place(munkiki_cube_verts, 8, sx, sy, sz, px, py + oy, pz, 0, 0, 0);
        mesh_draw(munkiki_cube_tris, 36);
    }
}

static void thing_draw(const struct mk_thing *t)
{
    int32_t px = (int32_t)t->x * 20, py = (int32_t)t->y * 20, pz = (int32_t)t->z * 20;
    uint8_t spin = (uint8_t)((mkv.counter & 0x1f) * 8);

    switch (t->kind) {
    case MK_THING_NOTE:
        texture = munkiki_png_note;
        texture_size = 32;
        break;
    case MK_THING_KEY:
        texture = munkiki_png_key;
        texture_size = 32;
        break;
    case MK_THING_WARP:
        texture = munkiki_png_warp;
        texture_size = 32;
        break;
    default:
        texture = munkiki_png_life;
        texture_size = 16;
        break;
    }
    if (t->kind == MK_THING_WARP) {
        /* Up 10 units, turning about z a sixteenth of a turn a tick. */
        static int8_t tex[8];
        uint8_t i;

        for (i = 0; i < 8; i++)
            tex[i] = (int8_t)(munkiki_warp_tex[i] >> 2);
        mesh_place(munkiki_warp_verts, 4, 65536L, 65536L, 65536L, px, py + 160, pz, spin,
                   (uint8_t)((mkv.counter & 0xf) * 16), tex);
        mesh_draw(munkiki_warp_tris, 12);
    } else {
        mesh_place(munkiki_thing_verts, 4, 65536L, 65536L, 65536L, px, py, pz, spin, 0, munkiki_thing_tex);
        mesh_draw(munkiki_thing_tris, 12);
    }
    texture = 0;
}

/* The backdrop (c.z): a triangle fixed in eye space, scaled by ten, in
   the light gray; looking up stretches it, looking down lowers it. */
static void backdrop_draw(void)
{
    uint8_t i;
    int32_t sy = 655360L + (mkv.tilt > 0 ? mkv.tilt : 0);
    int32_t oy = mkv.tilt > 0 ? 0 : (mkv.tilt << 1);

    for (i = 0; i < 3; i++) {
        verts[i].x = ((int32_t)munkiki_ground[i * 3] * 655360L) >> 12;
        verts[i].y = (((int32_t)munkiki_ground[i * 3 + 1] * sy) >> 12) + (oy >> 12);
        verts[i].z = ((int32_t)munkiki_ground[i * 3 + 2] * 655360L) >> 12;
        verts[i].u = verts[i].v = 0;
    }
    shade_set(0xe0);
    draw_triangle(&verts[0], &verts[1], &verts[2]);
}

void mk_frame_clear(void)
{
    memset(mk_frame, 0, sizeof mk_frame);
}

/* The 3D part of c.j, then Munkiki and the effects over it. */
static void world_draw(void)
{
    uint8_t i;
    int32_t wobble = jar_sine(((int32_t)(mkv.counter & 0xf) * 205887L) >> 3) >> 3;

    camera();
    /* Standing on a bouncy block, the whole world bobs. */
    if (mkv.wobble)
        at_y += (wobble * 20) >> 12;
    mk_frame_clear();
    backdrop_draw();
    order_count = 0;
    for (i = 0; i < mk.block_count; i++) {
        const struct mk_block *b = &mk_blocks[i];

        if (b->kind)
            order_add(i, 0, (int16_t)(b->x - 8 * b->sx), (int16_t)(b->x + 8 * b->sx), b->y,
                      (int16_t)(b->y + 16 * b->sy), (int16_t)(b->z - 8 * b->sz), (int16_t)(b->z + 8 * b->sz));
    }
    for (i = 0; i < mk.thing_count; i++) {
        const struct mk_thing *t = &mk_things[i];

        if (t->kind)
            order_add(i, 1, (int16_t)(t->x - 6), (int16_t)(t->x + 6), (int16_t)(t->y + 2), (int16_t)(t->y + 14),
                      (int16_t)(t->z - 6), (int16_t)(t->z + 6));
    }
    for (i = 0; i < order_count; i++) {
        if (order[i].thing)
            thing_draw(&mk_things[order[i].index]);
        else
            block_draw(&mk_blocks[order[i].index], wobble);
    }
}

/* Row-major pictures. */
void mk_frame_fill(int8_t x, int8_t y, uint8_t w, uint8_t h, uint8_t black)
{
    uint8_t p = black ? 0xff : 0;
    int16_t y1 = y + h, yy;

    if (black)
        shade_black();
    else
        pattern[0] = pattern[1] = pattern[2] = pattern[3] = 0;
    for (yy = y < 0 ? 0 : y; yy < y1 && yy < MK_H; yy++)
        span((uint8_t)yy, x, (int16_t)(x + w));
    (void)p;
}

static uint8_t pixel_of(const uint8_t *bits, uint8_t stride, uint8_t x, uint8_t y)
{
    return (uint8_t)((bits[y * stride + (x >> 3)] >> (7 - (x & 7))) & 1);
}

static void plot(int16_t x, int16_t y, uint8_t black)
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

void mk_frame_blit(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t *bits, const uint8_t *mask, uint8_t flags)
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

void mk_frame_blit_part(int16_t x, int16_t y, uint8_t w, uint8_t h, const uint8_t *bits, uint8_t stride, uint8_t sx,
                        uint8_t sy)
{
    uint8_t i, j;

    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            plot((int16_t)(x + i), (int16_t)(y + j), pixel_of(bits, stride, (uint8_t)(sx + i), (uint8_t)(sy + j)));
}

void mk_frame_pic(int16_t x, int16_t y, uint8_t which, uint8_t mask_which)
{
    const struct munkiki_pic *p = &munkiki_pic_table[which];

    mk_frame_blit(x, y, p->w, p->h, munkiki_pics + p->at,
                  mask_which == 0xff ? 0 : munkiki_pics + munkiki_pic_table[mask_which].at, 0);
}

/* d.av: a count in the HUD's small digits, 5 wide and 4 apart in the
   digits picture from column 24, the last ending at x. */
void mk_frame_digits(int16_t x, int16_t y, uint16_t value)
{
    uint8_t n = 0;

    do {
        int16_t at = (int16_t)(x - n++ * 5);

        mk_frame_blit_part(at, y, 5, 7, munkiki_digits, 8, (uint8_t)(24 + value % 10 * 4), 0);
    } while ((value /= 10) > 0);
}

void mk_frame_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    int16_t dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int16_t sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx - dy;

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

void mk_frame_rect(int16_t x, int16_t y, uint8_t w, uint8_t h)
{
    mk_frame_line(x, y, x + w, y);
    mk_frame_line(x, y + h, x + w, y + h);
    mk_frame_line(x, y, x, y + h);
    mk_frame_line(x + w, y, x + w, y + h);
}

/* MIDP's drawArc of half an ellipse in the box w + 1 by h + 1 at (x, y):
   the upper half, or the lower. */
void mk_frame_arc(int16_t x, int16_t y, uint8_t w, uint8_t h, uint8_t lower)
{
    int32_t a = w, b = h, cx2 = 2 * x + w, cy2 = 2 * y + h; /* twice the centre */
    int16_t px, last_py = -1;

    /* Column by column, the ellipse's y at each x, joined into a line. */
    for (px = x; px <= x + w; px++) {
        int32_t dx2 = 2 * px - cx2; /* twice the offset */
        int32_t t = a * a * 4 - dx2 * dx2 * 1;
        int32_t dy2, py;
        int16_t py16;

        if (t < 0)
            t = 0;
        /* dy2 = b sqrt(t) / a, in halves. */
        {
            int32_t r = 0, s = t * b * b / (a * a);

            while ((r + 1) * (r + 1) <= s)
                r++;
            dy2 = r;
        }
        py = lower ? (cy2 + dy2) : (cy2 - dy2);
        py16 = (int16_t)((py + (lower ? 1 : 0)) / 2);
        if (last_py != -1)
            mk_frame_line((int16_t)(px - 1), last_py, px, py16);
        else
            plot(px, py16, 1);
        last_py = py16;
    }
}

/* c.j's parts over the 3D picture (c.v is in munkiki_sprite.c). */
static void munkiki_draw_sprite(void)
{
    int8_t n = mkv.dy > 0 ? (int8_t)(2 + mkv.dy) : 0;

    if (40 - n < 0)
        return;
    mk_frame_munkiki((int16_t)(28 + mkv.dx), (int16_t)(22 + mkv.dy), mkv.sprite, (uint8_t)(40 - n));
}

void mk_scene_draw(void)
{
    uint8_t drew_munkiki = 0, by2;
    int16_t n, n2, n3;

    mkv.counter++;
    if (mkv.curtain < 16) {
        world_draw();
        if (mkv.sparkle != -1) {
            for (by2 = 0; by2 < 9; by2++) {
                n2 = by2 % 3 - 1;
                n = by2 / 3 - 1;
                {
                    int16_t x = (int16_t)(40 + (n2 << 1) * (6 + (mkv.sparkle * (n == 0 ? 3 : 2) >> 1)));
                    int16_t y = (int16_t)(20 + n * (12 + (mkv.sparkle * (n2 == 0 ? 15 : 10) >> 3)) + (mkv.sparkle * mkv.sparkle >> 2));

                    if (y < 40)
                        mk_frame_pic(x, y, 5, 16);
                }
            }
            if (++mkv.sparkle == 20)
                mkv.sparkle = -1;
        }
        if (mkv.splash != -1) {
            for (by2 = 0; by2 < 8; by2++) {
                uint8_t small = by2 % 3 != 0;
                int16_t by = (int16_t)((((mkv.splash + by2) % 5) << 1) + (small ? 8 - (by2 * 5 & 7) : 0));

                if (by < (mkv.splash << 1) && by > (mkv.splash << 2) - 28)
                    mk_frame_pic((int16_t)(26 + by2 * 5), (int16_t)(48 - by), small ? 1 : 2, small ? 12 : 13);
                if (by2 == 3) {
                    munkiki_draw_sprite();
                    drew_munkiki = 1;
                }
            }
        }
    } else {
        mk_frame_clear();
    }
    if (mkv.curtain_kind) {
        uint8_t nn = mkv.curtain > 16 ? 16 : mkv.curtain;

        mk_frame_fill(0, (int8_t)(65 - mkv.curtain), 96, (uint8_t)mkv.curtain, 0);
        for (by2 = 0; by2 < 3; by2++) {
            n3 = by2 == 2 ? (nn < 8 ? nn << 1 : 16) : by2 == 1 ? (nn < 12 ? nn * 8 / 3 : 32) : nn * 3;
            mk_frame_blit((int16_t)(n3 - 16), by2 == 0 ? -1 : 0, 16, 53, munkiki_pics + munkiki_pic_table[0].at, 0, 0);
            mk_frame_blit((int16_t)(96 - n3), by2 == 0 ? -1 : 0, 16, 53, munkiki_pics + munkiki_pic_table[0].at, 0, MK_FLIP);
        }
        if (mkv.curtain_kind == 1) {
            for (by2 = 0; by2 < 8; by2++) {
                uint8_t by;

                n3 = by2 * 17 % 80 + 4;
                n2 = (by2 * 29 + mkv.curtain * 3) % (76 + (by2 << 1)) - 20;
                by = (uint8_t)((mkv.curtain + by2) % 12);
                if (by > 6)
                    by = (uint8_t)(12 - by);
                if (n2 < 45 && n2 < mkv.curtain * 3)
                    mk_frame_pic(n3, n2, (uint8_t)(5 + by), (uint8_t)(16 + by));
            }
        } else {
            for (by2 = 0; by2 < 8; by2++) {
                uint8_t by = by2;

                if (by == 2 || by == 4)
                    by = (uint8_t)(6 - by);
                n3 = by * 17 % 80 + 4;
                n2 = (by * 29 + mkv.curtain * 2) % (76 + (by << 1)) - 20;
                if (by2 < 3) {
                    if (mkv.curtain > 20 && n2 < mkv.curtain * 2 - 40)
                        n2 = (int16_t)((int32_t)(mkv.curtain + by) * (mkv.curtain + by) * (mkv.curtain + by) % 3 + 63);
                    mk_frame_pic(n3, (int16_t)(65 - n2), 3, 14);
                } else if (n2 < 53 && n2 < mkv.curtain * 3) {
                    n3 = n3 < 38 ? ((n3 - 4) >> 1) + 4 : 84 - ((84 - n3) >> 1);
                    /* Clipped to 25 by 15 at (n3, n2): two rows of the picture, 14 apart. */
                    {
                        const struct munkiki_pic *p = &munkiki_pic_table[4];
                        int16_t yy;
                        uint8_t ry;

                        for (ry = 0; ry < 2; ry++) {
                            int16_t top = (int16_t)((n2 / 14 + ry) * 14);

                            for (yy = top; yy < top + 14; yy++) {
                                uint8_t i;

                                if (yy < n2 || yy >= n2 + 15)
                                    continue;
                                for (i = 0; i < 24 && i < 25; i++) {
                                    if (!pixel_of(munkiki_pics + munkiki_pic_table[15].at, 3, i, (uint8_t)(yy - top)))
                                        continue;
                                    plot((int16_t)(n3 + i), yy, pixel_of(munkiki_pics + p->at, 3, i, (uint8_t)(yy - top)));
                                }
                            }
                        }
                    }
                }
                if (by2 == 2) {
                    munkiki_draw_sprite();
                    drew_munkiki = 1;
                }
            }
        }
    }
    if (mkv.show && !drew_munkiki)
        munkiki_draw_sprite();
}

/* The frame onto the LCD view, marking what changed. */
void mk_frame_present(void)
{
    uint8_t y, x;
    uint8_t *dst;
    const uint8_t *src = mk_frame;

    for (y = 0; y < MK_H; y++) {
        dst = lcd_fb + (lcd_view_y + y) * LCD_STRIDE + (lcd_view_x >> 3);
        for (x = 0; x < MK_STRIDE; x++) {
            if (dst[x] != src[x]) {
                dst[x] = src[x];
                lcd_mark_dirty((uint8_t)(lcd_view_x + x * 8), (uint8_t)(lcd_view_y + y), 8, 1);
            }
        }
        src += MK_STRIDE;
    }
}
