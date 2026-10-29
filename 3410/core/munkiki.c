/* Munkiki's Castles: the world and Munkiki's moves, after the JAR's class
   d, method for method (the Java's names are in the comments), with the
   adding of blocks and things that the JAR's class c does (c.x, c.w) since
   they are the world's tables. See core/munkiki_int.h for the state.

   Positions: Munkiki's are the Java's 16.16 fixed point; blocks', things'
   and the movers' steps are in sixteenths of a cell (the Java's shifted
   down twelve, which is exact). A cell's index is x + (z << 4) + (y << 8),
   and the Java looks past the ends of the array without a care; here a
   cell outside it is empty. */
#include "munkiki_int.h"

#include <string.h>

#ifdef MK_STATE_AT
__at(MK_STATE_AT) uint8_t mk_cell[MK_CELLS];
__at(MK_STATE_AT + MK_CELLS) uint16_t mk_ref[MK_CELLS];
__at(MK_STATE_AT + 3 * MK_CELLS) struct mk_block mk_blocks[MK_MAX_BLOCKS];
#else
uint8_t mk_cell[MK_CELLS];
uint16_t mk_ref[MK_CELLS];
struct mk_block mk_blocks[MK_MAX_BLOCKS];
#endif
struct mk_thing mk_things[MK_MAX_THINGS];
struct mk_mover mk_movers[MK_MAX_MOVERS];
struct mk_world mk;
struct mk_view mkv;
uint16_t mk_pressed, mk_released;

/* The levels whose life has been taken this game (the JAR's KLP record
   store), a bit per level. */
static uint8_t lives_taken[(MUNKIKI_LEVEL_COUNT + 7) / 8];

#define ONE 65536L
#define ONE16 16 /* a cell in sixteenths */

#define in_world(i) ((i) >= 0 && (i) < MK_CELLS)

static uint8_t cell_at(int16_t i)
{
    return in_world(i) ? mk_cell[i] : 0;
}

/* d.ak: whether a cell holds a block. */
static uint8_t solid(int16_t i)
{
    return MK_KIND(cell_at(i)) != 0;
}

static void cell_or(int16_t i, uint8_t bits)
{
    if (in_world(i))
        mk_cell[i] |= bits;
}

static void cell_and(int16_t i, uint8_t bits)
{
    if (in_world(i))
        mk_cell[i] &= bits;
}

static uint16_t ref_at(int16_t i)
{
    return in_world(i) ? mk_ref[i] : 0;
}

static void ref_set(int16_t i, uint16_t value)
{
    if (in_world(i))
        mk_ref[i] = value;
}

static uint8_t pressed(uint8_t key)
{
    return (uint8_t)((mk_pressed >> key) & 1);
}

static void unpress(uint8_t key)
{
    mk_pressed &= (uint16_t)~(1u << key);
}

void mk_lives_reset(void)
{
    memset(lives_taken, 0, sizeof lives_taken);
}

/* d.at: whether this level's life is gone already. */
static uint8_t life_taken(uint8_t level)
{
    uint8_t i;

    for (i = 0; i < MUNKIKI_LEVEL_COUNT; i++)
        if (munkiki_level_ids[i] == level)
            return (uint8_t)((lives_taken[i >> 3] >> (i & 7)) & 1);
    return 0;
}

/* d.au: it is now. */
void mk_life_take(void)
{
    uint8_t i;

    for (i = 0; i < MUNKIKI_LEVEL_COUNT; i++)
        if (munkiki_level_ids[i] == mk.level)
            lives_taken[i >> 3] |= (uint8_t)(1 << (i & 7));
}

/* c.w: a thing on a cell. */
static uint8_t add_thing(uint8_t x, uint8_t y, uint8_t z, uint8_t kind)
{
    struct mk_thing *t = &mk_things[mk.thing_count];

    t->x = (int16_t)(x * ONE16);
    t->y = (int16_t)(y * ONE16);
    t->z = (int16_t)(z * ONE16);
    t->kind = kind;
    return mk.thing_count++;
}

/* c.x: a block, and after the last of a level, the walls joined into
   boxes: each wall block grows along x, then y, then z while every cell
   the box would take is a wall block later in the table; those are then
   taken out, and the box is moved to sit on its middle. */
static uint8_t add_block(uint8_t x, uint8_t y, uint8_t z, uint8_t kind)
{
    struct mk_block *b = &mk_blocks[mk.block_count];

    b->x = (int16_t)(x * ONE16);
    b->y = (int16_t)(y * ONE16);
    b->z = (int16_t)(z * ONE16);
    b->kind = kind;
    b->sx = b->sy = b->sz = 1;
    return mk.block_count++;
}

static void join_walls(uint8_t count)
{
    uint8_t n7, n9, axis, sx, sy, sz, inside;
    struct mk_block *b, *o;

    for (n7 = 0; n7 < count; n7++) {
        b = &mk_blocks[n7];
        if (b->kind != MK_BLOCK_WALL)
            continue;
        sx = b->sx;
        sy = b->sy;
        sz = b->sz;
        axis = 0;
        while (axis != 3) {
            if (axis == 0)
                sx++;
            else if (axis == 1)
                sy++;
            else
                sz++;
            inside = 0;
            for (n9 = (uint8_t)(n7 + 1); n9 < count; n9++) {
                o = &mk_blocks[n9];
                if (o->kind == MK_BLOCK_WALL && o->x < b->x + sx * ONE16 && o->x >= b->x
                    && o->y < b->y + sy * ONE16 && o->y >= b->y && o->z < b->z + sz * ONE16 && o->z >= b->z)
                    inside++;
            }
            if (inside == sx * sy * sz - 1)
                continue;
            if (axis == 0)
                sx--;
            else if (axis == 1)
                sy--;
            else
                sz--;
            axis++;
        }
        if (sx != 1 || sy != 1 || sz != 1) {
            for (n9 = (uint8_t)(n7 + 1); n9 < count; n9++) {
                o = &mk_blocks[n9];
                if (o->kind == MK_BLOCK_WALL && o->x < b->x + sx * ONE16 && o->x >= b->x
                    && o->y < b->y + sy * ONE16 && o->y >= b->y && o->z < b->z + sz * ONE16 && o->z >= b->z)
                    o->kind = 0;
            }
            b->sx = sx;
            b->sy = sy;
            b->sz = sz;
        }
        b->x = (int16_t)(b->x + (sx - 1) * (ONE16 / 2));
        b->z = (int16_t)(b->z + (sz - 1) * (ONE16 / 2));
    }
}

/* d.as: the level stream's bit reader, least significant bit first. */
static const uint8_t *bits_at;
static uint8_t bits_byte, bits_left;

static uint8_t bits(uint8_t n)
{
    uint8_t out = 0, i;

    for (i = 0; i < n; i++) {
        if (bits_left == 8) {
            bits_byte = *bits_at++;
            bits_left = 0;
        }
        if (bits_byte & (1 << bits_left))
            out |= (uint8_t)(1 << i);
        bits_left++;
    }
    return out;
}

/* c.y: the picture's state for a level. */
static void view_reset(void)
{
    mkv.wobble = 0;
    mkv.tilt = 0;
    mkv.dx = mkv.dy = 0;
    mkv.curtain_kind = 0;
    mkv.curtain = 0;
    mkv.sparkle = -1;
    mkv.splash = -1;
    mkv.show = 1;
}

/* d.an: the level from its stream: the facing, the par, then a record
   per kind of thing: a block kind's cells by layer as a box with a bit
   per cell (none when the box is a cell or two), a thing's as one cell. */
static void level_decode(const uint8_t *stream)
{
    uint8_t n6, kind, x, y, z, x1, z1, full;
    uint16_t i;

    memset(mk_cell, 0, sizeof mk_cell);
    memset(mk_ref, 0, sizeof mk_ref);
    mk.block_count = mk.thing_count = mk.mover_count = 0;
    mk.note_count = 0;
    bits_at = stream;
    bits_left = 8;
    mk.start_facing = bits(2);
    mk.time = (uint16_t)(bits(8) | (uint16_t)bits(8) << 8);
    for (n6 = 0; n6 < 13; n6++) {
        if (!bits(1))
            continue;
        kind = munkiki_level_kinds[n6];
        if (n6 >= 5 && n6 <= 8) {
            x = bits(4);
            y = bits(3);
            z = bits(4);
            if (mk.life_taken && kind == MK_THING_LIFE)
                continue;
            mk_cell[MK_CELL(x, y, z)] |= kind;
            if (kind == MK_THING_MUNKIKI) {
                mk.start_x = mk.mark_x = x;
                mk.start_y = mk.mark_y = y;
                mk.start_z = mk.mark_z = z;
            }
            continue;
        }
        for (y = 0; y < 8; y++) {
            if (!bits(1))
                continue;
            x = bits(4);
            x1 = (uint8_t)(x + bits(4));
            z = bits(4);
            z1 = (uint8_t)(z + bits(4));
            full = (uint8_t)(x1 - x + (z1 - z) < 2);
            for (; z <= z1; z++) {
                uint8_t xx;

                for (xx = x; xx <= x1; xx++) {
                    if (full || bits(1) == 1) {
                        mk_cell[MK_CELL(xx, y, z)] |= kind;
                        if (kind == MK_THING_NOTE)
                            mk.note_count++;
                    }
                }
            }
        }
    }
    /* The tables, in cell order; travelling blocks start as movers at
       rest, which pick their way on the first tick. */
    view_reset();
    i = 0;
    for (y = 0; y < 8; y++) {
        for (z = 0; z < 16; z++) {
            for (x = 0; x < 16; x++, i++) {
                uint8_t c = mk_cell[i];

                if (MK_THING(c) == MK_THING_MUNKIKI)
                    continue;
                if (MK_KIND(c)) {
                    mk_ref[i] = (uint16_t)(1 + add_block(x, y, z, MK_KIND(c)));
                } else if (MK_THING(c)) {
                    mk_ref[i] = (uint16_t)((1 + add_thing(x, y, z, MK_THING(c))) | MK_REF_THING);
                }
                if ((c & MK_TRAVEL) == MK_TRAVEL && mk.mover_count < MK_MAX_MOVERS) {
                    struct mk_mover *m = &mk_movers[mk.mover_count++];

                    m->ref = mk_ref[i];
                    m->vx = m->vy = m->vz = 0;
                    m->ticks = 1;
                    m->x = x;
                    m->y = y;
                    m->z = z;
                }
            }
        }
    }
    join_walls(mk.block_count);
}

uint8_t mk_level_exists(uint8_t level)
{
    uint8_t i;

    for (i = 0; i < MUNKIKI_LEVEL_COUNT; i++)
        if (munkiki_level_ids[i] == level)
            return 1;
    return 0;
}

/* d.aq. */
static void level_find(uint8_t level)
{
    uint8_t i;

    for (i = 0; i < MUNKIKI_LEVEL_COUNT; i++) {
        if (munkiki_level_ids[i] == level) {
            level_decode(munkiki_levels + munkiki_level_at[i]);
            return;
        }
    }
}

/* d.aw: the level afresh, with Munkiki sliding in. */
void mk_world_load(uint8_t level)
{
    mk.level = level;
    mk.life_taken = life_taken(level);
    level_find(level);
    mk.vz = mk.vx = mk.speed = mk.vy = mk.vyaw = 0;
    mk.left = 0;
    mk.px = (int32_t)mk.start_x * ONE;
    mk.py = (int32_t)mk.start_y * ONE;
    mk.pz = (int32_t)mk.start_z * ONE;
    mk.facing = (int32_t)mk.start_facing * ONE;
    mk.notes_left = mk.note_count;
    mk.moves = 0;
    mk.ended = 0;
    mk.phase = -1;
    mkv.dy = -64;
    mk.frame = 0;
    mk.action = 17;
    mk.anim = 15;
    mk.left = munkiki_anim_table[15].count;
}

/* d.ah: a block or thing (`flag` MK_REF_THING) at cell `n` starts to
   fall, a cell at a time, landing in the cell (x, y, z) the caller names. */
static void start_fall(int16_t n, uint8_t x, uint8_t y, uint8_t z, uint16_t flag)
{
    struct mk_mover *m;
    uint16_t r = ref_at(n);

    if (mk.mover_count >= MK_MAX_MOVERS)
        return;
    m = &mk_movers[mk.mover_count++];

    m->ref = (uint16_t)(MK_REF_INDEX(r) | flag);
    m->vx = 0;
    m->vy = -4;
    m->vz = 0;
    m->ticks = 4;
    m->x = x;
    m->y = y;
    m->z = z;
    cell_and(n - 256, MK_PATH);
    cell_or(n - 256, cell_at(n) & 0x7f);
    ref_set(n - 256, (uint16_t)(r | MK_REF_MOVING));
    cell_and(n, MK_PATH);
    ref_set(n, 0);
}

/* d.ai: a block falls unless it is a wall, fixed or travelling. */
static void block_fall(int16_t n, uint8_t x, uint8_t y, uint8_t z)
{
    uint8_t c = cell_at(n);

    if (MK_KIND(c) == MK_BLOCK_WALL || MK_KIND(c) == MK_BLOCK_FIXED || (c & MK_TRAVEL) == MK_TRAVEL)
        return;
    start_fall(n, x, y, z, 0);
}

/* d.ac: every mover moves, and one whose move is done is seen to: a
   travelling block picks its next cell along its path, a block with
   nothing under it falls, a sliding block goes on, and the rest stop. */
static void movers_tick(void)
{
    uint8_t n;

    for (n = 0; n < mk.mover_count; n++) {
        struct mk_mover *m = &mk_movers[n];
        int8_t n3 = m->vx, n4 = m->vy, n5 = m->vz;
        uint16_t index = (uint16_t)(MK_REF_INDEX(m->ref) - 1);
        int16_t n23, n28;
        uint8_t keep = 1;

        if (!(m->ref & MK_REF_THING)) {
            struct mk_block *b = &mk_blocks[index];

            b->x = (int16_t)(b->x + n3);
            b->y = (int16_t)(b->y + n4);
            b->z = (int16_t)(b->z + n5);
        } else {
            struct mk_thing *t = &mk_things[index];

            t->x = (int16_t)(t->x + n3);
            t->y = (int16_t)(t->y + n4);
            t->z = (int16_t)(t->z + n5);
        }
        if (--m->ticks)
            continue;
        n23 = (int16_t)MK_CELL(m->x, m->y, m->z);
        ref_set(n23, (uint16_t)(ref_at(n23) & ~MK_REF_MOVING));
        if (n3)
            n3 = n3 > 0 ? 1 : -1;
        if (n4) {
            if (n4 > 0) {
                n4 = 1;
            } else {
                if (n4 == -4)
                    keep = 0;
                n4 = -1;
            }
        }
        if (n5)
            n5 = n5 > 0 ? 1 : -1;
        n28 = (int16_t)(n3 + n5 * 16 + n4 * 256);
        if (keep && n28) {
            ref_set(n23 - n28, 0);
            cell_and(n23 - n28, MK_PATH);
        }
        if ((cell_at(n23) & MK_TRAVEL) == MK_TRAVEL) {
            uint8_t ways = 0, from = 0, axis, go;

            if (cell_at(n23 + 1) == MK_PATH)
                ways |= 1;
            if (cell_at(n23 - 1) == MK_PATH)
                ways |= 2;
            if (cell_at(n23 + 16) == MK_PATH)
                ways |= 0x10;
            if (cell_at(n23 - 16) == MK_PATH)
                ways |= 0x20;
            if (cell_at(n23 + 256) == MK_PATH || cell_at(n23 + 256) == (MK_PATH | MK_THING_MUNKIKI))
                ways |= 4;
            if (m->y > 0 && cell_at(n23 - 256) == MK_PATH)
                ways |= 8;
            if (!ways) {
                m->vx = m->vy = m->vz = 0;
                m->ticks = 1;
                continue;
            }
            if (n3 == -1)
                from = 1;
            if (n3 == 1)
                from = 2;
            if (n4 == -1)
                from = 4;
            if (n4 == 1)
                from = 8;
            if (n5 == -1)
                from = 16;
            if (n5 == 1)
                from = 32;
            axis = 3;
            if (n4)
                axis = 12;
            if (n5)
                axis = 48;
            n3 = n4 = n5 = 0;
            ways &= (uint8_t)~from;
            if (!ways) {
                go = from;
            } else {
                go = (uint8_t)(ways & axis);
                if (!go)
                    go = (uint8_t)(ways & ~axis);
            }
            if (go & 1)
                n3 = 1;
            else if (go & 2)
                n3 = -1;
            else if (go & 0x10)
                n5 = 1;
            else if (go & 0x20)
                n5 = -1;
            else if (go & 4)
                n4 = 1;
            else if (go & 8)
                n4 = -1;
            n28 = (int16_t)(n3 + n5 * 16 + n4 * 256);
            if (n28) {
                cell_or(n23 + n28, cell_at(n23) & 0x7f);
                ref_set(n23 + n28, (uint16_t)(ref_at(n23) | MK_REF_MOVING));
                ref_set(n23, (uint16_t)(ref_at(n23) | MK_REF_MOVING));
            }
            m->vx = (int8_t)(n3 * 2);
            m->vy = (int8_t)(n4 * 2);
            m->vz = (int8_t)(n5 * 2);
            m->ticks = 8;
            m->x = (uint8_t)(m->x + n3);
            m->y = (uint8_t)(m->y + n4);
            m->z = (uint8_t)(m->z + n5);
        } else if (m->y > 0 && (cell_at((int16_t)MK_CELL(m->x, m->y - 1, m->z)) & 0x3f) == 0) {
            cell_or(n23 - 256, cell_at(n23) & 0x7f);
            cell_and(n23, MK_PATH);
            ref_set(n23 - 256, (uint16_t)(m->ref | MK_REF_MOVING));
            ref_set(n23, 0);
            m->vx = 0;
            m->vy = -4;
            m->vz = 0;
            m->ticks = 4;
            m->y--;
        } else if (MK_KIND(cell_at(n23)) == MK_BLOCK_SLIDING && (n5 || n3) && !solid(n23 + n28)) {
            cell_or(n23 + n28, cell_at(n23) & 0x7f);
            ref_set(n23 + n28, (uint16_t)(m->ref | MK_REF_MOVING));
            m->ticks = 8;
            m->x = (uint8_t)(m->x + n3);
            m->z = (uint8_t)(m->z + n5);
        } else {
            mk.mover_count--;
            memmove(m, m + 1, (size_t)(mk.mover_count - n) * sizeof *m);
            n--;
        }
    }
}

/* d.ae: what a step forward from (x, y, z) in the direction (dx, dz) is,
   and in mk.land the cell it leads to. */
static int16_t step_kind(uint8_t x, uint8_t y, uint8_t z, int8_t dx, int8_t dz)
{
    int16_t n6 = (int16_t)MK_CELL(x, y, z), n7 = (int16_t)(dx + dz * 16);
    int16_t n8 = (int16_t)(n6 + n7), n9 = (int16_t)(n8 + n7);
    int8_t y8, y9;

    if (y > 0 && MK_KIND(cell_at(n6 - 256)) == MK_BLOCK_BOUNCY) {
        if (solid(n8) && solid(n8 + 256) && !solid(n8 + 512)) {
            mk.land = (int16_t)(n8 + 256);
            return 4;
        }
        if (solid(n9) && !solid(n8 - 256) && !solid(n9 + 256)) {
            mk.land = n9;
            return 5;
        }
    }
    if (!solid(n8)) {
        if (y <= 0 || solid(n9)) {
            mk.land = (int16_t)(n8 - 256);
            return 1;
        }
        while (n9 > 0 && !solid(n9))
            n9 -= 256;
        while (n8 > 0 && !solid(n8))
            n8 -= 256;
        if (n9 < 0 && n8 < 0) {
            mk.land = -1;
            return 1;
        }
        y9 = (int8_t)(n9 >> 8);
        y8 = (int8_t)(n8 >> 8);
        if (y8 >= y9) {
            if (mk.riding && MK_REF_INDEX(ref_at(n8)) == MK_REF_INDEX(ref_at(n6 - 256))) {
                mk.land = n9;
                return 3;
            }
            mk.land = n8;
            return 1;
        }
        if (y > 1 && y9 == y - 1 && y8 == y - 2) {
            mk.land = n8;
            return 1;
        }
        mk.land = n9;
        return 3;
    }
    if (!solid(n8 + 256) && !solid(n6 + 256)) {
        mk.land = n8;
        return 2;
    }
    mk.land = (int16_t)(n6 - 256);
    return 0;
}

/* d.ad: the step, and whether it lands on or leaves a travelling block. */
static int16_t step(uint8_t x, uint8_t y, uint8_t z, int8_t dx, int8_t dz)
{
    int16_t n6 = step_kind(x, y, z, dx, dz);
    uint8_t was = mk.riding;

    mk.riding = 0;
    if (mk.land > 0 && (cell_at(mk.land) & MK_TRAVEL) == MK_TRAVEL) {
        mk.riding = 1;
        mk.ride = MK_REF_INDEX(ref_at(mk.land));
        n6 = 14;
    }
    if (was && n6 != 14) {
        if (mk.land > 0)
            mk.land = (int16_t)MK_REF_INDEX(ref_at(mk.land));
        n6 = 15;
    }
    return n6;
}

/* d.af: what is under Munkiki: 0 a block (noting a travelling one), else
   how far he falls. */
static int16_t footing(uint8_t x, uint8_t y, uint8_t z)
{
    int16_t n4;

    mk.riding = 0;
    if (y <= 0)
        return -111;
    n4 = (int16_t)MK_CELL(x, y - 1, z);
    if (!solid(n4)) {
        if (y <= 1)
            return -111;
        if (y == 2 && !solid(n4 - 256))
            return -117;
        if (y > 2 && !solid(n4 - 256))
            return -115;
        return -116;
    }
    if ((cell_at(n4) & MK_TRAVEL) == MK_TRAVEL) {
        mk.ride = MK_REF_INDEX(ref_at(n4));
        mk.riding = 1;
    }
    return 0;
}

/* d.ag: the stick hits the cell: a breakable block that is not moving
   breaks, and what was above it comes down. */
static uint8_t hit(uint8_t x, uint8_t y, uint8_t z)
{
    int16_t n4 = (int16_t)MK_CELL(x, y, z);
    uint16_t n5;
    uint8_t n7, n8;

    if (MK_KIND(cell_at(n4)) != MK_BLOCK_BREAKABLE)
        return 0;
    if ((ref_at(n4) & 0xf800) == MK_REF_MOVING)
        return 0;
    n5 = (uint16_t)(MK_REF_INDEX(ref_at(n4)) - 1);
    ref_set(n4, 0);
    cell_and(n4, (uint8_t)~7);
    n7 = MK_THING(cell_at(n4));
    if (n7) {
        ref_set(n4, (uint16_t)((1 + add_thing(x, y, z, n7)) | MK_REF_THING));
    } else {
        for (n8 = (uint8_t)(y + 1); n8 < 8; n8++) {
            n4 += 256;
            if (solid(n4))
                block_fall(n4, x, (uint8_t)(n8 - 1), z);
            else if (MK_THING(cell_at(n4)))
                start_fall(n4, x, (uint8_t)(n8 - 1), z, MK_REF_THING);
        }
    }
    mk_blocks[n5].kind = 0;
    mkv.sparkle = 0;
    return 1;
}

/* d.aj: a push on the cell ahead, (x, y, z), in the direction (dx, dz):
   2 for a sliding block, 1 for one that goes a cell, 0 for none. */
static uint8_t push(uint8_t x, uint8_t y, uint8_t z, int8_t dx, int8_t dz)
{
    int16_t n6 = (int16_t)MK_CELL(x, y, z), n7 = (int16_t)(dx + 16 * dz);
    uint8_t c = cell_at(n6), above = cell_at(n6 + 256), out;
    struct mk_mover *m;

    if (!MK_KIND(c) || MK_KIND(c) == MK_BLOCK_FIXED || MK_KIND(c) == MK_BLOCK_WALL)
        return 0;
    if (ref_at(n6) & MK_REF_MOVING)
        return 0;
    if (solid(n6 + n7) || MK_THING(cell_at(n6 + n7)) || MK_THING(above))
        return 0;
    if (MK_KIND(above) && MK_KIND(above) != MK_BLOCK_WALL && (above & MK_TRAVEL) != MK_TRAVEL)
        return 0;
    if (mk.mover_count >= MK_MAX_MOVERS)
        return 0;
    cell_or(n6 + n7, c & 0x7f);
    ref_set(n6 + n7, (uint16_t)(ref_at(n6) | MK_REF_MOVING));
    m = &mk_movers[mk.mover_count++];
    m->ref = MK_REF_INDEX(ref_at(n6 + n7));
    if (MK_KIND(c) == MK_BLOCK_SLIDING) {
        m->vx = (int8_t)(dx * 2);
        m->vz = (int8_t)(dz * 2);
        m->ticks = 8;
        out = 2;
    } else {
        m->vx = dx;
        m->vz = dz;
        m->ticks = 16;
        out = 1;
    }
    m->vy = 0;
    m->x = (uint8_t)(x + dx);
    m->y = y;
    m->z = (uint8_t)(z + dz);
    return out;
}

/* d.al: a bouncy block at the cell. */
static uint8_t bouncy_at(int8_t x, int8_t y, int8_t z)
{
    if (x >= 0 && x < 16 && y >= 0 && y < 8 && z >= 0 && z < 16)
        return MK_KIND(mk_cell[MK_CELL(x, y, z)]) == MK_BLOCK_BOUNCY;
    return 0;
}

/* d.am: takes the thing on Munkiki's cell, if any, and says what it was. */
static uint8_t take(uint8_t x, uint8_t y, uint8_t z)
{
    int16_t n4 = (int16_t)MK_CELL(x, y, z);
    uint8_t n5 = MK_THING(cell_at(n4));

    if (n5 == MK_THING_MUNKIKI)
        n5 = 0;
    if (n5) {
        if (ref_at(n4) & MK_REF_THING)
            mk_things[MK_REF_INDEX(ref_at(n4)) - 1].kind = 0;
        cell_and(n4, (uint8_t)~0x38);
        ref_set(n4, 0);
    }
    return n5;
}

/* d.ao: marks the cell as Munkiki's; a block above the one he left falls
   (into the cell the JAR names, his new one, as it does). */
static void mark(uint8_t x, uint8_t y, uint8_t z)
{
    int16_t n4;

    if (mk.mark_x == x && mk.mark_z == z && mk.mark_y == y)
        return;
    n4 = (int16_t)MK_CELL(mk.mark_x, mk.mark_y, mk.mark_z);
    cell_and(n4, (uint8_t)~0x38);
    if (solid(n4 + 256))
        block_fall(n4 + 256, x, y, z);
    mk.mark_x = x;
    mk.mark_y = y;
    mk.mark_z = z;
    cell_or((int16_t)MK_CELL(x, y, z), MK_THING_MUNKIKI);
}

uint8_t mk_any_pressed(void)
{
    uint8_t any = mk_pressed != 0;

    mk_pressed = 0;
    return any;
}

static int32_t idiv(int32_t a, uint8_t b)
{
    /* Java's division truncates towards zero, as C99's. */
    return a / b;
}

/* d.m: a tick of the world. */
void mk_world_tick(void)
{
    int8_t n2, n3;
    uint8_t n4, n5, n6;
    int16_t was;

    if (mk.phase == -1) {
        mkv.dy = (int8_t)(mkv.dy + 8);
        if (mkv.dy >= 0) {
            mkv.dy = 0;
            mk.phase = 0;
        }
    }
    movers_tick();
    if (mk.phase == 1) {
        if (pressed(MK_KEY_HASH)) {
            mk.phase = 0;
            mkv.show = 1;
            mkv.tilt = 0;
            unpress(MK_KEY_HASH);
        } else {
            if (pressed(6) && mk.look_yaw < 0x2d0000L)
                mk.look_yaw += 327680L;
            else if (pressed(4) && mk.look_yaw > -2949120L)
                mk.look_yaw -= 327680L;
            if (pressed(8) && mk.look_pitch < 0x2d0000L)
                mk.look_pitch += 327680L;
            else if (pressed(2) && mk.look_pitch > -2949120L)
                mk.look_pitch -= 327680L;
            mkv.tilt = mk.look_pitch;
        }
        unpress(0);
        unpress(5);
    } else {
        mkv.tilt = 0;
    }
    if (mk.demo && mk.demo_key == -1) {
        for (n4 = 0; n4 < 12; n4++)
            if (pressed(n4))
                mk.demo_key = (int8_t)n4;
    }
    n4 = (uint8_t)(mk.facing >> 16);
    n2 = (int8_t)((n4 & 1) ? 2 - n4 : 0);
    n3 = (int8_t)((n4 & 1) ? 0 : n4 - 1);
    n4 = (uint8_t)((mk.px + 32768L) >> 16);
    n5 = (uint8_t)((mk.py + 32768L) >> 16);
    n6 = (uint8_t)((mk.pz + 32768L) >> 16);
    if (mk_pressed && (mk.action & 0x40))
        mk.left = 0;
    if (mk.left == 0 && mk.phase == 0 && mk.ended == 0) {
        was = mk.action;
        mk.action = 0;
        mk.vz = mk.vx = mk.speed = mk.vy = mk.vyaw = 0;
        switch (take(n4, n5, n6)) {
        case MK_THING_NOTE:
            if (--mk.notes_left == 0) {
                mk_play(MK_SOUND_LEVEL_DONE);
                mk.ended = 2;
            } else {
                mk_play(MK_SOUND_NOTE);
            }
            break;
        case MK_THING_LIFE:
            mk.life_taken = 1;
            mk_play(MK_SOUND_LIFE);
            mk.lives++;
            break;
        case MK_THING_KEY:
            mk.keys++;
            mk_play(MK_SOUND_KEY);
            mk.ended = MK_THING_KEY;
            break;
        case MK_THING_WARP:
            mk_play(MK_SOUND_WARP);
            mk.ended = MK_THING_WARP;
            break;
        default:
            break;
        }
        mark(n4, n5, n6);
        mk.action = footing(n4, n5, n6);
        if (mk.action >= 0 && mk.ended == 0) {
            if (pressed(MK_KEY_HASH)) {
                mk.phase = 1;
                mkv.show = 0;
                mk.look_pitch = mk.look_yaw = 0;
                unpress(MK_KEY_HASH);
            } else if (pressed(2)) {
                mk.action = step(n4, n5, n6, n2, n3);
            } else if (pressed(6)) {
                mk.action = 6;
            } else if (pressed(4)) {
                mk.action = 7;
            } else if (pressed(5)) {
                uint8_t by = push((uint8_t)(n4 + n2), n5, (uint8_t)(n6 + n3), n2, n3);

                if (by == 1)
                    mk.action = 8;
                if (by == 2)
                    mk.action = 16;
            } else if (pressed(0)) {
                mk.action = 9;
            } else if (pressed(8) && !solid((int16_t)MK_CELL(n4 - n2, n5, n6 - n3))) {
                mk.action = 10;
            }
        }
        switch (mk.action) {
        case 0:
            mk.anim = 0;
            mk.facing = (mk.facing + 32768L) & 0x30000L;
            if (!mk.riding) {
                mk.px = (mk.px + 32768L) & ~0xffffL;
                mk.py = (mk.py + 32768L) & ~0xffffL;
                mk.pz = (mk.pz + 32768L) & ~0xffffL;
            }
            break;
        case 1:
            mk.moves++;
            mk.anim = 1;
            mk.speed = ONE;
            break;
        case 2:
            mk_play(MK_SOUND_JUMP);
            mk.moves++;
            mk.anim = 4;
            mk.speed = ONE;
            mk.vy = ONE;
            break;
        case 3:
            mk_play(MK_SOUND_JUMP);
            mk.speed = 2 * ONE;
            mk.moves++;
            mk.anim = 4;
            break;
        case 14:
            mk.moves++;
            mk.anim = 4;
            break;
        case 15:
            mk_play(MK_SOUND_JUMP);
            if (mk.land > 0) {
                const struct mk_block *b = &mk_blocks[mk.land - 1];

                mk.vx = (int32_t)b->x * 4096L - mk.px;
                mk.vz = (int32_t)b->z * 4096L - mk.pz;
                mk.vy = ONE + (int32_t)b->y * 4096L - mk.py;
            } else {
                mk.speed = 2 * ONE;
                mk.vy = -1;
            }
            if (mk.vy < 0) {
                const struct mk_block *b = &mk_blocks[mk.ride - 1];

                mk.vy = ONE + (int32_t)b->y * 4096L - mk.py >= 0 ? ONE : 0;
            }
            mk.moves++;
            mk.anim = 4;
            break;
        case 4:
            mk_play(MK_SOUND_JUMP);
            mk.moves++;
            mk.anim = 4;
            mk.speed = ONE;
            mk.vy = 2 * ONE;
            break;
        case 5:
            mk_play(MK_SOUND_JUMP);
            mk.moves++;
            mk.anim = 4;
            mk.speed = 2 * ONE;
            mk.vy = ONE;
            break;
        case 6:
            mk.anim = 3;
            mk.vyaw = ONE;
            break;
        case 7:
            mk.anim = 2;
            mk.vyaw = -ONE;
            break;
        case 8:
            mk.moves++;
            mk.anim = 5;
            mk.speed = ONE;
            mk_play(MK_SOUND_PUSH);
            break;
        case 16:
            mk.moves++;
            mk.anim = 14;
            mk.speed = ONE;
            mk_play(MK_SOUND_PUSH);
            break;
        case 9:
            mk.moves++;
            mk.anim = 6;
            break;
        case 10:
            mk.moves++;
            mk.anim = 13;
            mk.speed = -ONE;
            break;
        case -117:
            mk.anim = 12;
            mk.vy = -98304L;
            break;
        case -111:
            if (was == -117)
                mk.vy = -ONE;
            else
                mk.vy = -98304L;
            mk.anim = 16;
            mk_play(MK_SOUND_DROWN);
            break;
        case -115:
            if (was != -115)
                mk_play(MK_SOUND_FALL);
            mk.anim = 8;
            mk.vy = -ONE;
            break;
        case -116:
            mk.anim = was == -115 ? 9 : 7;
            mk.vy = -ONE;
            break;
        default:
            break;
        }
        if (mk.action != 0) {
            mk.idle = 0;
            mk.left = munkiki_anim_table[mk.anim].count;
            if (mk.speed) {
                mk.vx = n2 * mk.speed;
                mk.vz = n3 * mk.speed;
            }
            mk.vy = idiv(mk.vy, mk.left);
            mk.vyaw = idiv(mk.vyaw, mk.left);
            mk.vx = idiv(mk.vx, mk.left);
            mk.vz = idiv(mk.vz, mk.left);
        } else if (mk.idle++ >= 40) {
            mk.idle = 0;
            mk.anim = 10;
            mk.left = munkiki_anim_table[mk.anim].count;
            mk.action = 82;
        }
        if (was != mk.action)
            mk.frame = 0;
    }
    if (mk.riding && (mk.action == 6 || mk.action == 7 || mk.action == 0 || mk.action == 82 || mk.action == 9)) {
        const struct mk_block *b = &mk_blocks[mk.ride - 1];

        mk.px = (int32_t)b->x * 4096L;
        mk.py = ONE + (int32_t)b->y * 4096L;
        mk.pz = (int32_t)b->z * 4096L;
    }
    if (mk.left > 0) {
        if (mk.action == 14) {
            const struct mk_block *b = &mk_blocks[mk.ride - 1];

            mk.vx = idiv((int32_t)b->x * 4096L - mk.px, mk.left);
            mk.vy = idiv(ONE + (int32_t)b->y * 4096L - mk.py, mk.left);
            mk.vz = idiv((int32_t)b->z * 4096L - mk.pz, mk.left);
        }
        if (mk.action == 9 && mk.left == 2 && hit((uint8_t)(n4 + n2), n5, (uint8_t)(n6 + n3)))
            mk_play(MK_SOUND_BREAK);
        if (mk.action == -111)
            mkv.splash++;
        mk.px += mk.vx;
        mk.py += mk.vy;
        mk.pz += mk.vz;
        if (mk.py < ONE)
            mkv.dy = (int8_t)(mkv.dy + (int8_t)(-mk.vy >> 11));
        mk.facing = (mk.facing + mk.vyaw + 262144L) & 0x3ffffL;
        if (--mk.left == 0) {
            mk.facing = (mk.facing + 32768L) & 0x30000L;
            if (mk.action == -111) {
                mk.ended = 1;
                mk.anim = 0;
            }
        }
    }
    if (mk.frame >= munkiki_anim_table[mk.anim].count)
        mk.frame = 0;
    mkv.sprite = munkiki_anim_frames[munkiki_anim_table[mk.anim].at + mk.frame++];
    mkv.wobble = bouncy_at((int8_t)n4, (int8_t)(n5 - 1), (int8_t)n6);
    if (mk.py < ONE)
        mk.py = ONE;
}
