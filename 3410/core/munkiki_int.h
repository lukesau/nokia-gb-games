/* Munkiki's Castles' insides, shared by munkiki.c (the world and
   Munkiki's moves, the JAR's class d), munkiki_3d.c (the picture, class
   c) and munkiki_game.c (the screens around the game, class KKM and the
   canvas b). The JAR's names are obfuscated; each thing here says which
   of its fields it is, so that the Java can be read beside it.

   Lengths are the JAR's: a cell is 20 units of the 3D world; positions
   of blocks are kept in sixteenths of a cell (the Java's 16.16 fixed
   point shifted down twelve, which loses nothing: every move is a whole
   number of sixteenths), Munkiki's own in the Java's 16.16, since his
   moves divide a step by the length of an animation. */
#ifndef CORE_MUNKIKI_INT_H
#define CORE_MUNKIKI_INT_H

#include <stdint.h>

#include "munkiki.h"
#include "munkiki_data.h"

#define MK_W 96
#define MK_H 65
#define MK_STRIDE 12

/* The world: 16 across (x), 8 up (y), 16 deep (z); a cell's index is
   x + (z << 4) + (y << 8). */
#define MK_CELLS 2048
#define MK_CELL(x, y, z) ((uint16_t)((x) + ((z) << 4) + ((y) << 8)))

/* A cell's byte (cl): the block's kind in bits 0 to 2, the thing on it in
   bits 3 to 5, and the two top bits mark a travelling block (both) or its
   path (the top alone). */
#define MK_KIND(c) ((c) & 7)
#define MK_THING(c) ((c) & 0x38)
#define MK_TRAVEL 0xc0
#define MK_PATH 0x80
enum {
    MK_BLOCK_PLAIN = 1,
    MK_BLOCK_BOUNCY,
    MK_BLOCK_BREAKABLE,
    MK_BLOCK_SLIDING,
    MK_BLOCK_WALL,    /* joined with its neighbours into one box */
    MK_BLOCK_FIXED    /* neither pushed nor fallen */
};
enum {
    MK_THING_NOTE = 8,
    MK_THING_KEY = 16,
    MK_THING_WARP = 24,
    MK_THING_LIFE = 32,
    MK_THING_MUNKIKI = 40 /* where he stands */
};

/* A cell's reference (ck): the index plus one of its block, or of its
   thing with MK_REF_THING, and MK_REF_MOVING while a mover has it. */
#define MK_REF_INDEX(r) ((r) & 0x7ff)
#define MK_REF_MOVING 0x800
#define MK_REF_THING 0x1000

#define MK_MAX_BLOCKS 72
#define MK_MAX_THINGS 16

/* A block as the picture keeps it (c.bd): its position in sixteenths of
   a cell, its kind (0 once gone) and its size in cells, more than one
   each way only for a wall. */
struct mk_block {
    int16_t x, y, z;
    uint8_t kind, sx, sy, sz;
};

/* A thing to collect (c.ba): where it is in sixteenths of a cell (it can
   fall) and what it is, 0 once taken. */
struct mk_thing {
    int16_t x, y, z;
    uint8_t kind;
};

/* A mover (d.bz): what moves (a cell's reference), how far each tick in
   sixteenths of a cell, the ticks left, and the cell it is in. */
struct mk_mover {
    uint16_t ref;
    int8_t vx, vy, vz;
    uint8_t ticks;
    uint8_t x, y, z;
};

#define MK_MAX_MOVERS 32
/* The Game Boy keeps the world in its cartridge RAM, from MK_STATE_AT:
   the cells, their references, the blocks, the things, the movers, and
   then the picture's working room (munkiki_3d.c). */
#define MK_BLOCKS_AT (MK_STATE_AT + 3 * MK_CELLS)
#define MK_THINGS_AT (MK_BLOCKS_AT + MK_MAX_BLOCKS * 10)
#define MK_MOVERS_AT (MK_THINGS_AT + MK_MAX_THINGS * 7)
#define MK_SCRATCH_AT (MK_MOVERS_AT + MK_MAX_MOVERS * 9)
#ifdef MK_STATE_AT
extern __at(MK_STATE_AT) uint8_t mk_cell[MK_CELLS];
extern __at(MK_STATE_AT + MK_CELLS) uint16_t mk_ref[MK_CELLS];
extern __at(MK_BLOCKS_AT) struct mk_block mk_blocks[MK_MAX_BLOCKS];
extern __at(MK_THINGS_AT) struct mk_thing mk_things[MK_MAX_THINGS];
extern __at(MK_MOVERS_AT) struct mk_mover mk_movers[MK_MAX_MOVERS];
#else
extern uint8_t mk_cell[MK_CELLS];
extern uint16_t mk_ref[MK_CELLS];
extern struct mk_block mk_blocks[MK_MAX_BLOCKS];
extern struct mk_thing mk_things[MK_MAX_THINGS];
extern struct mk_mover mk_movers[MK_MAX_MOVERS];
#endif

/* The world's state (class d). */
struct mk_world {
    uint8_t block_count;  /* be and ca */
    uint8_t thing_count;  /* bc and b9 */
    uint8_t mover_count;  /* by */
    uint8_t note_count;   /* b8: the notes the level starts with */
    uint16_t time;        /* b7: the level's par, in moves */
    uint8_t start_x, start_y, start_z, start_facing; /* cj, ci, ch, cf */
    uint8_t mark_x, mark_y, mark_z; /* ce, cd, cc: the cell marked as his */
    int32_t px, py, pz;   /* c_, cz, cy: Munkiki, 16.16 cells */
    int32_t facing;       /* cx: 16.16 quarter turns, 0 to 4 */
    int32_t vx, vy, vz;   /* ct, cv, cr: a tick's move, 16.16 */
    int32_t vyaw;         /* cw */
    int32_t speed;        /* cu */
    int32_t look_pitch, look_yaw; /* c0, c1: 16.16 degrees */
    uint8_t left;         /* cq: ticks the move has to go */
    int8_t anim;          /* cn: which of the animation tables */
    uint8_t frame;        /* bh: the frame of it next */
    int16_t action;       /* cp */
    uint8_t idle;         /* co: ticks standing still */
    int8_t phase;         /* aj: -1 sliding in, 0 playing, 1 looking around */
    uint8_t ended;        /* c2: 0, 1 died, else the thing that ended the level */
    uint8_t keys;         /* c3 */
    uint16_t score;       /* c4 */
    int8_t lives;         /* c5 */
    uint8_t level;        /* c6 */
    uint8_t notes_left;   /* c7 */
    uint8_t deaths;       /* c8: on this level */
    uint16_t moves;       /* c9 */
    int16_t land;         /* b4: a cell found for a move, -1 none */
    uint16_t ride;        /* b5: the block he stands on, plus one */
    uint8_t riding;       /* b6: and whether it carries him */
    uint8_t life_taken;   /* bx: this level's life is gone */
    uint8_t demo;         /* db: the Instructions' demo, with its keypad */
    int8_t demo_key;      /* da: the key it shows pressed, -1 none */
};
extern struct mk_world mk;

/* The picture's state (class c). */
struct mk_view {
    uint8_t counter;     /* bt: ticks, for the things' turning and the bouncy blocks */
    uint8_t wobble;      /* bs: Munkiki stands on a bouncy block */
    uint8_t show;        /* br: draw Munkiki */
    int32_t tilt;        /* bq: looking up or down, 16.16 degrees */
    uint8_t sprite;      /* bh: Munkiki's frame code */
    int8_t dx, dy;       /* bg, bf: where his picture is moved to */
    int8_t sparkle;      /* a7: a block breaking, -1 none */
    int8_t splash;       /* a6: a fall into the water, -1 none */
    uint8_t curtain;     /* a5: the level's end, counting up */
    uint8_t curtain_kind; /* a4: 1 a castle taken, 2 the last */
};
extern struct mk_view mkv;

/* Keys pressed since the last tick (KKM.an) and released since they were
   pressed (b.d), bits 0 to 11: the digits, then # and *. */
extern uint16_t mk_pressed, mk_released;
#define MK_BIT_HASH 0x400
#define MK_BIT_STAR 0x800

/* The sounds the game plays, by index in the JAR. */
enum {
    MK_SOUND_NOTE,
    MK_SOUND_KEY,
    MK_SOUND_WARP,
    MK_SOUND_BREAK,
    MK_SOUND_PUSH,
    MK_SOUND_JUMP,
    MK_SOUND_GAME_OVER,
    MK_SOUND_FALL,
    MK_SOUND_LEVEL_DONE,
    MK_SOUND_LIFE,
    MK_SOUND_TITLE,
    MK_SOUND_DROWN
};
void mk_play(uint8_t sound) MK_FAR; /* munkiki_game.c */

/* The bank the Game Boy keeps the sounds in, for sound_play_notes; 0
   elsewhere. */
#ifndef MUNKIKI_SOUND_BANK
#define MUNKIKI_SOUND_BANK 0
#endif

/* The phone's fonts, a glyph at a time: its columns (bit y the row), and
   its width, which is returned alone when `columns` is null. The fonts
   are 0 the small plain, 1 the medium bold, 2 the large bold, 3 the tiny
   and 4 the small bold. On the Game Boy they are in other banks, and
   far.c supplies this. */
uint8_t mk_glyph(uint8_t font, char c, uint16_t *columns);
/* On the Game Boy, the two that are: in the menus' bank for the fonts 0,
   1, 2 and 4 (menu.c) and in the full-screen menus' for the tiny one
   (menu_header.c). */
uint8_t mk_glyph_menu(uint8_t font, char c, uint16_t *columns) MK_FAR;
uint8_t mk_glyph_tiny(char c, uint16_t *columns) MK_FAR;

/* munkiki.c */
void mk_world_load(uint8_t level) MK_FAR;  /* aw: a level afresh */
void mk_world_resume(void) MK_FAR;         /* ay: the level as it was, Munkiki at its start */
void mk_world_tick(void) MK_FAR;           /* d.m */
uint8_t mk_any_pressed(void) MK_FAR;       /* KKM.o: and forgets them */
uint8_t mk_level_exists(uint8_t level) MK_FAR;
void mk_lives_reset(void) MK_FAR;          /* a new game: no level's life taken yet */
void mk_life_take(void) MK_FAR;            /* d.au: this level's is */

/* The picture: a row of MK_STRIDE bytes per row, the leftmost pixel in
   the top bit, 1 black. The Game Boy draws it in the other games' picture
   buffer, which is idle meanwhile (MK_FRAME_SHARED); munkiki_blit.h has
   the drawing into it. */
#ifdef MK_FRAME_SHARED
#include "sprite.h"
#define mk_frame sprite_screen
#else
extern uint8_t mk_frame[MK_STRIDE * MK_H];
#endif
/* munkiki_scene.c: c.j, the world, then Munkiki and the effects. */
void mk_scene_draw(void) MK_FAR;

/* munkiki_3d.c: the rasterizer, which the scene draws through. The
   camera for the frame, moved down by `bob` sixteenths of a unit; the
   backdrop; the shade of what follows (a gray, or 0 for black) and its
   texture; and a mesh, scaled by (sx, sy, sz) in 16.16, at (ox, oy, oz)
   in sixteenths of a unit, turned about y by `spin` and about z by
   `spin_z` (binary angles, a turn 256). The eye's place in the world, in
   sixteenths of a cell, for the order. */
void mk3d_camera(int32_t bob) MK_FAR;
void mk3d_backdrop(void) MK_FAR;
void mk3d_shade(uint8_t gray) MK_FAR;
enum {
    MK_TEXTURE_NONE,
    MK_TEXTURE_NOTE,
    MK_TEXTURE_KEY,
    MK_TEXTURE_WARP,
    MK_TEXTURE_LIFE
};
void mk3d_texture(uint8_t which) MK_FAR;
enum {
    MK_MESH_RIM,   /* a floor block's black rim */
    MK_MESH_TOP,   /* and its top */
    MK_MESH_EDGES, /* a block's black edges */
    MK_MESH_CUBE,  /* and its faces */
    MK_MESH_THING, /* the diamond a note, key or life is on */
    MK_MESH_WARP   /* the warp's square */
};
void mk3d_mesh(uint8_t which, int32_t sx, int32_t sy, int32_t sz, int32_t ox, int32_t oy, int32_t oz, uint8_t spin,
               uint8_t spin_z) MK_FAR;
extern int16_t mk3d_eye_x, mk3d_eye_y, mk3d_eye_z;
/* The gray of a block kind (munkiki_shades, kept with the rasterizer). */
uint8_t munkiki_shades_of(uint8_t kind) MK_FAR;
/* munkiki_sprite.c: Munkiki's picture, frame code `code`, at (x, y), only
   `rows` rows of it. */
void mk_frame_munkiki(int16_t x, int16_t y, uint8_t code, uint8_t rows) MK_SHEET_FAR;

#endif
