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

#include <string.h>



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


/* The camera: eye = rot · (p - at) + move, p and the results in
   sixteenths of a unit. */
static int16_t rot[9];
static int32_t at_x, at_y, at_z;
static int32_t move_x, move_y, move_z;
/* Where the eye is in the world, in sixteenths of a cell, for the order. */
static int16_t eye_x, eye_y, eye_z;
int16_t mk3d_eye_x, mk3d_eye_y, mk3d_eye_z;


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
    int32_t xl, xr, dl, ul, vl, ur, vr, dul, dvl;
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
        return -(int32_t)v->z - NEAR;
    case 1:
        return -(int32_t)v->z - v->x;
    case 2:
        return -(int32_t)v->z + v->x;
    case 3:
        return -(int32_t)v->z * 65 - (int32_t)v->y * 96;
    default:
        return -(int32_t)v->z * 65 + (int32_t)v->y * 96;
    }
}


static void lerp(struct vertex *out, const struct vertex *a, const struct vertex *b, int32_t sa, int32_t sb)
{
    /* The point where the side crosses zero: a + (b - a) sa / (sa - sb). */
    int32_t n = sa - sb;

    out->x = (int16_t)(a->x + ((int32_t)(b->x - a->x) * sa) / n);
    out->y = (int16_t)(a->y + ((int32_t)(b->y - a->y) * sa) / n);
    out->z = (int16_t)(a->z + ((int32_t)(b->z - a->z) * sa) / n);
    out->u = (int16_t)(a->u + ((int32_t)(b->u - a->u) * sa) / n);
    out->v = (int16_t)(a->v + ((int32_t)(b->v - a->v) * sa) / n);
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
    int32_t d = -(int32_t)v->z;

    if (d < 1)
        d = 1;
    p->x = (int16_t)(768 + (768L * v->x) / d);
    p->y = (int16_t)(520 - (784L * v->y) / d);
    p->u = (int32_t)v->u << 8;
    p->v = (int32_t)v->v << 8;
}


/* Draws a triangle given in eye space, front faces only. */
#ifdef MK_STATE_AT
static __at(MK_SCRATCH_AT) struct vertex verts[20];
static __at(MK_SCRATCH_AT + 20 * 10) struct vertex poly_a[MAX_POLY];
static __at(MK_SCRATCH_AT + 32 * 10) struct vertex poly_b[MAX_POLY];
static __at(MK_SCRATCH_AT + 44 * 10) struct point pts[MAX_POLY];
#else
static struct vertex verts[20], poly_a[MAX_POLY], poly_b[MAX_POLY];
static struct point pts[MAX_POLY];
#endif


static void draw_triangle(const struct vertex *v0, const struct vertex *v1, const struct vertex *v2)
{
    struct vertex *a = poly_a, *b = poly_b;
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
    for (i = 1; i + 1 < n; i++)
        triangle(&pts[0], &pts[i], &pts[i + 1]);
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
}


static void mesh_draw(const int8_t *tris, uint8_t count)
{
    uint8_t i;

    for (i = 0; i < count; i += 3)
        draw_triangle(&verts[tris[i]], &verts[tris[i + 1]], &verts[tris[i + 2]]);
}


/* The backdrop (c.z): a triangle fixed in eye space, scaled by ten, in
   the light gray; looking up stretches it, looking down lowers it. */
static void backdrop_draw(void)
{
    uint8_t i;
    int32_t sy = 655360L + (mkv.tilt > 0 ? mkv.tilt : 0);
    int32_t oy = mkv.tilt > 0 ? 0 : (mkv.tilt << 1);

    for (i = 0; i < 3; i++) {
        verts[i].x = (int16_t)(((int32_t)munkiki_ground[i * 3] * 655360L) >> 12);
        verts[i].y = (int16_t)((((int32_t)munkiki_ground[i * 3 + 1] * sy) >> 12) + (oy >> 12));
        verts[i].z = (int16_t)(((int32_t)munkiki_ground[i * 3 + 2] * 655360L) >> 12);
        verts[i].u = verts[i].v = 0;
    }
    shade_set(0xe0);
    draw_triangle(&verts[0], &verts[1], &verts[2]);
}


/* What the scene draws with (munkiki_int.h). */
void mk3d_camera(int32_t bob) MK_FAR
{
    camera();
    at_y += bob;
    mk3d_eye_x = eye_x;
    mk3d_eye_y = eye_y;
    mk3d_eye_z = eye_z;
}

void mk3d_backdrop(void) MK_FAR
{
    backdrop_draw();
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

void mk3d_mesh(uint8_t which, int32_t sx, int32_t sy, int32_t sz, int32_t ox, int32_t oy, int32_t oz, uint8_t spin,
               uint8_t spin_z) MK_FAR
{
    static int8_t warp_tex[8];
    uint8_t i;

    switch (which) {
    case MK_MESH_RIM:
        mesh_place(munkiki_rim_verts, 6, sx, sy, sz, ox, oy, oz, 0, 0, 0);
        mesh_draw(munkiki_rim_tris, 12);
        break;
    case MK_MESH_TOP:
        mesh_place(munkiki_top_verts, 4, sx, sy, sz, ox, oy, oz, 0, 0, 0);
        mesh_draw(munkiki_top_tris, 6);
        break;
    case MK_MESH_EDGES:
        mesh_place(munkiki_edge_verts, 20, sx, sy, sz, ox, oy, oz, 0, 0, 0);
        mesh_draw(munkiki_edge_tris, 72);
        break;
    case MK_MESH_CUBE:
        mesh_place(munkiki_cube_verts, 8, sx, sy, sz, ox, oy, oz, 0, 0, 0);
        mesh_draw(munkiki_cube_tris, 36);
        break;
    case MK_MESH_THING:
        mesh_place(munkiki_thing_verts, 4, sx, sy, sz, ox, oy, oz, spin, 0, munkiki_thing_tex);
        mesh_draw(munkiki_thing_tris, 12);
        break;
    default:
        /* The warp's texture coordinates are 0 to 4: one copy of it. */
        for (i = 0; i < 8; i++)
            warp_tex[i] = (int8_t)(munkiki_warp_tex[i] >> 2);
        mesh_place(munkiki_warp_verts, 4, sx, sy, sz, ox, oy, oz, spin, spin_z, warp_tex);
        mesh_draw(munkiki_warp_tris, 12);
        break;
    }
}

uint8_t munkiki_shades_of(uint8_t kind) MK_FAR
{
    return munkiki_shades[kind - 1];
}
