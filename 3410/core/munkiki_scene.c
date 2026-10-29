/* Munkiki's Castles: the scene (the rest of the JAR's class c): the
   world's blocks and things in the order to draw them, back to front,
   through the rasterizer's meshes (munkiki_3d.c), then Munkiki and the
   effects over it: a block breaking, a fall into the water, and the
   curtains and notes of a castle taken. */
#include "munkiki_blit.h"

#ifndef MK_FRAME_SHARED
uint8_t mk_frame[MK_STRIDE * MK_H];
#endif

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


/* The drawables, in the order to draw them: a block, or a thing with
   MK_DRAW_THING, by index; its box is worked out when wanted, in
   sixteenths of a cell. */
#define MK_DRAW_THING 0x80
static uint8_t order[MK_MAX_BLOCKS + MK_MAX_THINGS];
static uint8_t order_count;
struct drawable {
    int16_t x0, x1, y0, y1, z0, z1;
};


static void box_of(uint8_t which, struct drawable *d)
{
    if (which & MK_DRAW_THING) {
        const struct mk_thing *t = &mk_things[which & 0x7f];

        d->x0 = (int16_t)(t->x - 6);
        d->x1 = (int16_t)(t->x + 6);
        d->y0 = (int16_t)(t->y + 2);
        d->y1 = (int16_t)(t->y + 14);
        d->z0 = (int16_t)(t->z - 6);
        d->z1 = (int16_t)(t->z + 6);
    } else {
        const struct mk_block *b = &mk_blocks[which];

        d->x0 = (int16_t)(b->x - 8 * b->sx);
        d->x1 = (int16_t)(b->x + 8 * b->sx);
        d->y0 = b->y;
        d->y1 = (int16_t)(b->y + 16 * b->sy);
        d->z0 = (int16_t)(b->z - 8 * b->sz);
        d->z1 = (int16_t)(b->z + 8 * b->sz);
    }
}


/* Whether a must be drawn before b: b is nearer the eye along every axis
   that separates them. */
static uint8_t before(const struct drawable *a, const struct drawable *b)
{
    uint8_t separated = 0;

    if (b->x1 <= a->x0) {
        separated = 1;
        if (mk3d_eye_x >= a->x0)
            return 0;
    } else if (b->x0 >= a->x1) {
        separated = 1;
        if (mk3d_eye_x <= a->x1)
            return 0;
    }
    if (b->y1 <= a->y0) {
        separated = 1;
        if (mk3d_eye_y >= a->y0)
            return 0;
    } else if (b->y0 >= a->y1) {
        separated = 1;
        if (mk3d_eye_y <= a->y1)
            return 0;
    }
    if (b->z1 <= a->z0) {
        separated = 1;
        if (mk3d_eye_z >= a->z0)
            return 0;
    } else if (b->z0 >= a->z1) {
        separated = 1;
        if (mk3d_eye_z <= a->z1)
            return 0;
    }
    return separated;
}


static void order_add(uint8_t which)
{
    struct drawable d, o;
    uint8_t i, place = 0;

    box_of(which, &d);
    /* After the last that must come before it. */
    for (i = 0; i < order_count; i++) {
        box_of(order[i], &o);
        if (before(&o, &d))
            place = (uint8_t)(i + 1);
    }
    memmove(&order[place + 1], &order[place], (size_t)(order_count - place));
    order[place] = which;
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
    mk3d_shade(0);
    mk3d_mesh(top ? MK_MESH_RIM : MK_MESH_EDGES, sx, sy, sz, px, py + oy, pz, 0, 0);
    mk3d_shade(munkiki_shades_of(kind));
    mk3d_mesh(top ? MK_MESH_TOP : MK_MESH_CUBE, sx, sy, sz, px, py + oy, pz, 0, 0);
}

static void thing_draw(const struct mk_thing *t)
{
    int32_t px = (int32_t)t->x * 20, py = (int32_t)t->y * 20, pz = (int32_t)t->z * 20;
    uint8_t spin = (uint8_t)((mkv.counter & 0x1f) * 8);

    mk3d_texture(t->kind == MK_THING_NOTE ? MK_TEXTURE_NOTE : t->kind == MK_THING_KEY ? MK_TEXTURE_KEY
                 : t->kind == MK_THING_WARP ? MK_TEXTURE_WARP : MK_TEXTURE_LIFE);
    if (t->kind == MK_THING_WARP)
        /* Up 10 units, turning about z a sixteenth of a turn a tick. */
        mk3d_mesh(MK_MESH_WARP, 65536L, 65536L, 65536L, px, py + 160, pz, spin, (uint8_t)((mkv.counter & 0xf) * 16));
    else
        mk3d_mesh(MK_MESH_THING, 65536L, 65536L, 65536L, px, py, pz, spin, 0);
    mk3d_texture(MK_TEXTURE_NONE);
}

/* The 3D part of c.j. */
static void world_draw(void)
{
    uint8_t i;
    int32_t wobble = jar_sine(((int32_t)(mkv.counter & 0xf) * 205887L) >> 3) >> 3;

    /* Standing on a bouncy block, the whole world bobs. */
    mk3d_camera(mkv.wobble ? (wobble * 20) >> 12 : 0);
    frame_clear();
    mk3d_backdrop();
    order_count = 0;
    for (i = 0; i < mk.block_count; i++)
        if (mk_blocks[i].kind)
            order_add(i);
    for (i = 0; i < mk.thing_count; i++)
        if (mk_things[i].kind)
            order_add((uint8_t)(i | MK_DRAW_THING));
    for (i = 0; i < order_count; i++) {
        if (order[i] & MK_DRAW_THING)
            thing_draw(&mk_things[order[i] & 0x7f]);
        else
            block_draw(&mk_blocks[order[i]], wobble);
    }
}

static void frame_pic(int16_t x, int16_t y, uint8_t which, uint8_t mask_which)
{
    const struct munkiki_pic *p = &munkiki_pic_table[which];

    frame_blit(x, y, p->w, p->h, munkiki_pics + p->at, munkiki_pics + munkiki_pic_table[mask_which].at, 0);
}

/* c.j's parts over the 3D picture (c.v is in munkiki_sprite.c). */
static void munkiki_draw_sprite(void)
{
    int8_t n = mkv.dy > 0 ? (int8_t)(2 + mkv.dy) : 0;

    if (40 - n < 0)
        return;
    mk_frame_munkiki((int16_t)(28 + mkv.dx), (int16_t)(22 + mkv.dy), mkv.sprite, (uint8_t)(40 - n));
}

void mk_scene_draw(void) MK_FAR
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
                        frame_pic(x, y, 5, 16);
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
                    frame_pic((int16_t)(26 + by2 * 5), (int16_t)(48 - by), small ? 1 : 2, small ? 12 : 13);
                if (by2 == 3) {
                    munkiki_draw_sprite();
                    drew_munkiki = 1;
                }
            }
        }
    } else {
        frame_clear();
    }
    if (mkv.curtain_kind) {
        uint8_t nn = mkv.curtain > 16 ? 16 : mkv.curtain;

        frame_fill(0, (int16_t)(65 - mkv.curtain), 96, (uint8_t)mkv.curtain, 0);
        for (by2 = 0; by2 < 3; by2++) {
            n3 = by2 == 2 ? (nn < 8 ? nn << 1 : 16) : by2 == 1 ? (nn < 12 ? nn * 8 / 3 : 32) : nn * 3;
            frame_blit((int16_t)(n3 - 16), by2 == 0 ? -1 : 0, 16, 53, munkiki_pics + munkiki_pic_table[0].at, 0, 0);
            frame_blit((int16_t)(96 - n3), by2 == 0 ? -1 : 0, 16, 53, munkiki_pics + munkiki_pic_table[0].at, 0, MK_FLIP);
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
                    frame_pic(n3, n2, (uint8_t)(5 + by), (uint8_t)(16 + by));
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
                    frame_pic(n3, (int16_t)(65 - n2), 3, 14);
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


