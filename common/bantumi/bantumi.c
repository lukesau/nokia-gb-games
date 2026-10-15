#include "bantumi.h"

#include <stddef.h>
#include <string.h>

#include "game_assets.h"
#include "sprite.h"

/* The pictures, by the firmware address of their 12-byte descriptors
   (bitmap address, four unused bytes, width, height), and the hands'
   bitmaps, which the game copies to RAM and draws with its own sizes. */
#define BOARDS 0x31d0c0ul        /* 84x48: the open board, two opening frames, the closed box */
#define THINKING 0x31d22cul      /* 16x16, eight pictures */
#define OPEN_BOTTOM 0x31d0f0ul   /* 12x16 */
#define OPEN_TOP 0x31d108ul
#define OPEN_BOTTOM_MASK 0x31d120ul
#define OPEN_TOP_MASK 0x31d138ul
#define CLOSED_BOTTOM 0x31d150ul /* 11x13 */
#define CLOSED_TOP 0x31d168ul
#define CLOSED_TOP_MASK 0x31d180ul
#define CLOSED_BOTTOM_MASK 0x31d198ul
#define PIT_XY 0x31d1ecul        /* 14 x, then 14 y: each pit's box */

#define OPEN_W 12
#define OPEN_H 16
#define CLOSED_W 11
#define CLOSED_H 13

#define SPRITES 45
#define THINK_X 34
#define THINK_Y 16

/* The pits: 0..5 the player's row left to right, 6 the player's store,
   7..12 the phone's row right to left, 13 its store. */
#define STORE 6
#define PHONE_STORE 13
#define PITS 14

/* Whose turn it is and what is under way. */
enum {
    TURN_INTRO,
    TURN_PLAYER_HAND, /* the player's hand going to a pit: to pick it up, or a capture */
    TURN_PHONE_HAND,
    TURN_PLAYER,      /* the player's turn, and the player's sowing */
    TURN_PHONE,       /* the phone thinking, and its sowing */
    TURN_OVER_PAUSE,
    TURN_HINT,
    TURN_OVER
};

/* The hand's animation. */
enum {
    HAND_IDLE,
    HAND_WAIT,
    HAND_MOVE,
    HAND_ARRIVE,
    HAND_ACT
};

/* Sowing. */
enum {
    SOW_IDLE,
    SOW_BEAN,        /* one bean into the next pit */
    SOW_CAPTURE,     /* the last bean to the store, then the opposite pit's */
    SOW_CAPTURE_LAST /* the beans in hand to the store */
};

#define SIDE_PLAYER 3
#define SIDE_PHONE 4
#define NO_MOVE 0x0e
#define NO_PIT 0xff
/* The search goes at most 8 plies deep: the root and 8 more. */
#define NODES 10

struct node {
    uint8_t side;
    int16_t alpha, beta, best;
    uint8_t pits[PITS];
    uint8_t first;   /* the move tried first, less 7 for the phone; NO_MOVE before it is chosen */
    uint8_t counter; /* the next move in pit order */
    uint8_t empty;   /* a row is empty: the game would be over */
};

struct bantumi {
    int8_t pits[PITS];
    uint8_t cursor;
    uint8_t chosen;    /* the phone's pit */
    uint8_t hand_pit;  /* where the hand is, which decides its row; NO_PIT while the phone thinks */
    uint8_t turn;
    uint8_t countdown; /* ticks of the game-over pause */
    uint8_t level;
    uint8_t flag, flag_ticks;
    uint8_t sow_pit;
    uint8_t in_hand;
    uint8_t think_frame, think_ticks;
    uint8_t sow;
    uint8_t capture_store;

    uint8_t won;
    uint8_t sound;
    uint8_t intro_step, intro_toggle, intro_frame;
    uint8_t mask_id, image_id, board_id, think_id;
    uint8_t closed;
    uint8_t anim, counter;
    int8_t x, y, tx, ty;
    uint8_t units[PITS], tens[PITS]; /* the digits' sprites */
    /* The hands' bitmaps in RAM, open and closed, each with a height of
       its own: every sprite made from one shows what it holds now. */
    uint8_t open_mask[OPEN_W * 2], open_image[OPEN_W * 2];
    uint8_t closed_mask[CLOSED_W * 2], closed_image[CLOSED_W * 2];

    /* The search: the nodes from the root down to the one being worked on. */
    struct node nodes[NODES];
};

/* A platform short of work RAM keeps the state elsewhere: the Game Boy
   in cartridge RAM, with SDCC's __at (see core/pairs2.c). */
#ifdef BANTUMI_STATE_AT
static __at(BANTUMI_STATE_AT) struct bantumi b;
#ifdef BANTUMI_STATE_END
typedef char bantumi_state_fits[BANTUMI_STATE_AT + sizeof(struct bantumi) <= BANTUMI_STATE_END ? 1 : -1];
#endif
#else
static struct bantumi b;
#endif

/* The search's own state, outside the game's for the Game Boy's version
   of it in assembly (platform/gb/search.s), which this layout is for: the
   node being worked on, the root, the live board the phone's capture is
   tested on, the plies left, the best move at the root, and the steps a
   call takes and whether the search ended in them. */
struct node *bantumi_cur, *bantumi_root;
const int8_t *bantumi_board;
uint8_t bantumi_depth, bantumi_pick, bantumi_search_steps, bantumi_search_done;
#define cur bantumi_cur

#ifdef BANTUMI_PLATFORM_SEARCH
void bantumi_search_gb(void);
typedef char node_layout[sizeof(struct node) == 24 && offsetof(struct node, alpha) == 1 && offsetof(struct node, beta) == 3
                         && offsetof(struct node, best) == 5 && offsetof(struct node, pits) == 7
                         && offsetof(struct node, first) == 21 && offsetof(struct node, counter) == 22
                         && offsetof(struct node, empty) == 23 ? 1 : -1];
#endif

/* How the winner's store digits are drawn, flipped on every tick of the
   game-over pause. The game never sets it at the start, so it goes on
   from where the last game left it. */
static uint8_t winner_mode;

/* The data, by firmware address. */

static const uint8_t *data(uint32_t address)
{
    return bantumi_data + (uint16_t)(address - BANTUMI_DATA_BASE);
}

static uint8_t pit_x(uint8_t pit)
{
    return data(PIT_XY)[pit];
}

static uint8_t pit_y(uint8_t pit)
{
    return data(PIT_XY)[PITS + pit];
}

static void image(struct sprite_image *out, uint32_t descriptor)
{
    const uint8_t *d = data(descriptor);

    out->bitmap = data((uint32_t)d[1] << 16 | (uint16_t)d[2] << 8 | d[3]);
    out->w = d[8];
    out->h = d[9];
}

static uint8_t create(uint32_t descriptor, uint8_t mode, uint8_t layer, int x, int y)
{
    struct sprite_image im;

    image(&im, descriptor);
    return (uint8_t)sprite_create(&im, mode, layer, x, y);
}

static void free_id(uint8_t id)
{
    if (id)
        sprite_free(id);
}

static void digit(struct sprite_image *out, uint8_t n)
{
    out->bitmap = si_digit_glyphs + 4 * n;
    out->w = 4;
    out->h = 5;
}

/* Sets a pit's count and its digits: one digit in the middle of the box,
   or two, the tens made and freed as the count crosses ten. */
static void set_pit(uint8_t pit, int8_t n)
{
    uint8_t store = pit == STORE || pit == PHONE_STORE;
    uint8_t dx = store ? 2 : 1, dy = store ? 5 : 2;
    struct sprite_image im;

    if (n < 10 && b.pits[pit] >= 10) {
        free_id(b.tens[pit]);
        sprite_move(b.units[pit], pit_x(pit) + dx + 3, pit_y(pit) + dy);
    } else if (n >= 10 && b.pits[pit] >= 10) {
        digit(&im, (uint8_t)(n / 10));
        sprite_set_image(b.tens[pit], &im);
    } else if (n >= 10) {
        digit(&im, (uint8_t)(n / 10));
        b.tens[pit] = (uint8_t)sprite_create(&im, SPRITE_MODE_XOR, 2, pit_x(pit) + dx, pit_y(pit) + dy);
        sprite_move(b.units[pit], pit_x(pit) + dx + 4, pit_y(pit) + dy);
    }
    digit(&im, (uint8_t)(n % 10));
    sprite_set_image(b.units[pit], &im);
    b.pits[pit] = n;
}

/* The hand: a mask that clears the board under it and the picture over
   that, each copied to RAM and shifted up when the hand is above the
   top of the screen, where the phone cuts it off. The phone's shifts
   lose everything past 8 rows up. */
static void copy_hand(uint8_t *out, const uint8_t *in, uint8_t w, int8_t y)
{
    uint8_t i, shift = (uint8_t)-y;

    for (i = 0; i < w; i++) {
        if (y >= 0) {
            out[i] = in[i];
            out[w + i] = in[w + i];
        } else {
            out[i] = (uint8_t)((shift < 8 ? in[i] >> shift : 0) | (shift <= 8 ? in[w + i] << (8 - shift) : 0));
            out[w + i] = (uint8_t)(shift < 8 ? in[w + i] >> shift : 0);
        }
    }
}

/* A bitmap's new height, for every sprite made from it. */
static void set_height(const uint8_t *bitmap, uint8_t h)
{
    uint8_t i;

    for (i = 1; i <= SPRITE_COUNT; i++)
        if (sprites[i].image.bitmap == bitmap)
            sprites[i].image.h = h;
    sprite_changed = 1;
}

static void draw_hand(uint8_t keep)
{
    uint8_t bottom = b.hand_pit <= 6, w, h;
    const uint8_t *mask, *picture;
    uint8_t *ram_mask, *ram_image;
    struct sprite_image im;

    if (!keep) {
        free_id(b.mask_id);
        free_id(b.image_id);
    }
    if (!b.closed) {
        mask = data(bottom ? OPEN_BOTTOM_MASK : OPEN_TOP_MASK);
        picture = data(bottom ? OPEN_BOTTOM : OPEN_TOP);
        w = OPEN_W;
        h = OPEN_H;
        ram_mask = b.open_mask;
        ram_image = b.open_image;
    } else {
        mask = data(bottom ? CLOSED_BOTTOM_MASK : CLOSED_TOP_MASK);
        picture = data(bottom ? CLOSED_BOTTOM : CLOSED_TOP);
        w = CLOSED_W;
        h = CLOSED_H;
        ram_mask = b.closed_mask;
        ram_image = b.closed_image;
    }
    if (b.y < 0)
        h = (uint8_t)(h + b.y);
    copy_hand(ram_mask, mask, w, b.y);
    copy_hand(ram_image, picture, w, b.y);
    set_height(ram_mask, h);
    set_height(ram_image, h);
    im.w = w;
    im.h = h;
    im.bitmap = ram_mask;
    b.mask_id = (uint8_t)sprite_create(&im, SPRITE_MODE_CLEAR_BACKGROUND, 3, b.x, b.y < 0 ? 0 : b.y);
    im.bitmap = ram_image;
    b.image_id = (uint8_t)sprite_create(&im, SPRITE_MODE_SET, 3, b.x, b.y < 0 ? 0 : b.y);
}

/* The open hand over a pit of the player's row. */
static void hand_at(uint8_t pit)
{
    b.x = (int8_t)(pit_x(pit) - 1);
    b.y = (int8_t)(pit_y(pit) - 14);
}

/* Sends the hand from one pit to another: where it starts and stops
   depend on its row, on whether it is picking up, and on the corners. */
static void hand_path(uint8_t from, uint8_t to)
{
    uint8_t picking = b.turn == TURN_PLAYER_HAND || b.turn == TURN_PHONE_HAND;

    b.anim = HAND_WAIT;
    b.counter = 2;
    b.x = (int8_t)(pit_x(from) - 1);
    b.tx = (int8_t)(pit_x(to) - 1);
    if (b.hand_pit <= 6) {
        b.y = (int8_t)(pit_y(from) - 14);
        b.ty = (int8_t)(pit_y(to) - (picking ? 4 : 11));
    } else {
        b.y = (int8_t)(pit_y(from) + 10);
        b.ty = (int8_t)(pit_y(to) + (picking ? 0 : b.hand_pit == PHONE_STORE ? 10 : 7));
    }
    if (from == PHONE_STORE || (from == 12 && b.turn == TURN_PLAYER)) {
        b.x = (int8_t)pit_x(PHONE_STORE);
        b.y = (int8_t)pit_y(PHONE_STORE);
    }
    if (from == STORE || (from == 5 && b.turn == TURN_PHONE)) {
        b.x = (int8_t)pit_x(STORE);
        b.y = (int8_t)pit_y(STORE);
    }
}

/* The search. Its loops are written for the Game Boy, where a level 5
   search takes most of the time there is: counts as bytes, the node
   being worked on by pointer. */

static uint8_t row_empty(const uint8_t *p)
{
    return !(p[0] | p[1] | p[2] | p[3] | p[4] | p[5]) || !(p[7] | p[8] | p[9] | p[10] | p[11] | p[12]);
}

static void ai_start(void)
{
    struct node *root = &b.nodes[0];

    memset(root, 0, sizeof *root);
    root->alpha = -32000;
    root->beta = 32000;
    root->best = -32000;
    if (b.turn == TURN_HINT) {
        root->side = SIDE_PLAYER;
        bantumi_depth = 5;
    } else {
        root->side = SIDE_PHONE;
        bantumi_depth = (uint8_t)(b.level * 2 - 1);
        if (b.level == 5)
            bantumi_depth = 8;
    }
    root->first = NO_MOVE;
    memcpy(root->pits, b.pits, PITS);
    root->empty = row_empty(root->pits);
    cur = bantumi_root = root;
    bantumi_board = b.pits;
}

#ifndef BANTUMI_PLATFORM_SEARCH
static int16_t evaluate(const struct node *n, uint8_t terminal)
{
    const uint8_t *p = n->pits;
    int16_t v = (int16_t)p[PHONE_STORE] - p[STORE];

    if (terminal) {
        v += (int16_t)(p[7] + p[8] + p[9] + p[10] + p[11] + p[12]);
        v -= (int16_t)(p[0] + p[1] + p[2] + p[3] + p[4] + p[5]);
        if (v > 0)
            v += 50;
        else if (v < 0)
            v -= 50;
    }
    return n->side == SIDE_PLAYER ? (int16_t)-v : v;
}

/* The move tried first: one that ends in the store (an empty store counts
   as one, which is then skipped as empty), else the last of the greatest
   captures as the firmware reckons them, one pit off, else the fullest
   pit, the first of equals. */
static uint8_t first_move(const struct node *n)
{
    const uint8_t *p = n->pits;
    uint8_t base = n->side == SIDE_PLAYER ? 0 : 7, store = (uint8_t)(base + 6), i, j, best = 0, found = 0xff;

    for (i = 6; ; i--) {
        if (p[base + i] == (uint8_t)(6 - i))
            return (uint8_t)(base + i);
        if (!i)
            break;
    }
    for (i = base; i < store; i++) {
        uint8_t c = p[i];

        if (!c)
            continue;
        j = (uint8_t)(i + c);
        if (j < store && p[j] == 0 && (found == 0xff || p[13 - j] >= best)) {
            best = p[13 - j];
            found = i;
        }
    }
    if (found != 0xff)
        return found;
    found = base;
    best = p[base];
    for (i = (uint8_t)(base + 1); i < store; i++)
        if (p[i] > best) {
            best = p[i];
            found = i;
        }
    return found;
}

static uint8_t next_move(struct node *n)
{
    uint8_t i;

    if (n->first == NO_MOVE) {
        i = first_move(n);
        n->first = n->side != SIDE_PHONE ? i : (uint8_t)(i - 7);
        return i;
    }
    i = n->counter++;
    if (i == n->first) {
        i++;
        n->counter++;
    }
    return n->side == SIDE_PHONE ? (uint8_t)(i + 7) : i;
}

/* A child of the node being worked on, after the move from `pit`. The
   phone's capture is tested on the live board, not the child's, and goes
   to the player's store: the firmware's mistakes, kept. */
/* Kept static: SDCC reaches them on the Game Boy much faster than locals
   on the stack. */
static uint8_t *sow_q;
static uint8_t sow_p, sow_n;

static void child(uint8_t pit)
{
    struct node *parent = cur, *c = cur + 1;

    sow_q = c->pits;
    sow_p = pit;
    bantumi_depth--;
    memcpy(sow_q, parent->pits, PITS);
    c->first = NO_MOVE;
    c->counter = 0;
    c->best = -32000;
    sow_n = sow_q[pit];
    sow_q[pit] = 0;
    if (parent->side != SIDE_PLAYER) {
        while (sow_n--) {
            if (++sow_p == PITS)
                sow_p = 0;
            else if (sow_p == STORE)
                sow_p++;
            sow_q[sow_p]++;
        }
        if (sow_p == PHONE_STORE) {
            c->alpha = parent->alpha;
            c->beta = parent->beta;
            c->side = parent->side;
        } else {
            c->alpha = (int16_t)-parent->beta;
            c->beta = (int16_t)-parent->alpha;
            c->side = SIDE_PLAYER;
            if (b.pits[sow_p] == 1 && sow_p > 6) {
                sow_q[STORE] = (uint8_t)(sow_q[STORE] + sow_q[12 - sow_p] + 1);
                sow_q[12 - sow_p] = 0;
                sow_q[sow_p] = 0;
            }
        }
    } else {
        while (sow_n--) {
            if (++sow_p == PHONE_STORE)
                sow_p = 0;
            sow_q[sow_p]++;
        }
        if (sow_p == STORE) {
            c->alpha = parent->alpha;
            c->beta = parent->beta;
            c->side = parent->side;
        } else {
            c->alpha = (int16_t)-parent->beta;
            c->beta = (int16_t)-parent->alpha;
            c->side = SIDE_PHONE;
            if (sow_q[sow_p] == 1 && sow_p < 6) {
                sow_q[STORE] = (uint8_t)(sow_q[STORE] + sow_q[12 - sow_p] + 1);
                sow_q[12 - sow_p] = 0;
                sow_q[sow_p] = 0;
            }
        }
    }
    c->empty = row_empty(sow_q);
    cur = c;
}

static void close_node(uint8_t terminal)
{
    struct node *n = cur, *parent = cur - 1;
    int16_t v;

    if (terminal || bantumi_depth == 0)
        n->best = evaluate(n, terminal);
    v = n->best;
    if (n->side != parent->side)
        v = (int16_t)-v;
    if (v > parent->best) {
        parent->best = v;
        if (parent == bantumi_root) {
            uint8_t m = parent->counter == 0 ? parent->first : (uint8_t)(parent->counter - 1);

            bantumi_pick = parent->side == SIDE_PHONE ? (uint8_t)(m + 7) : m;
        }
    }
    if (parent->best > parent->alpha)
        parent->alpha = parent->best;
    bantumi_depth++;
    cur = parent;
}

/* Steps of the search, bantumi_search_steps of them or until it ends,
   which it says in bantumi_search_done. */
static void search(void)
{
    bantumi_search_done = 0;
    for (; bantumi_search_steps; bantumi_search_steps--) {
        struct node *n = cur;

        if (bantumi_depth != 0 && n->counter < 6 && n->best < n->beta) {
            uint8_t m;

            if (n->empty) {
                close_node(1);
                continue;
            }
            m = next_move(n);
            if (n->counter > 6 || n->pits[m] == 0)
                continue;
            child(m);
        } else if (n != bantumi_root) {
            close_node(0);
        } else {
            bantumi_search_done = 1;
            return;
        }
    }
}
#endif

/* A hundred steps of the search; at its end the phone's hand sets out for
   its pit, or the hint moves the cursor. */
static void ai_step(void)
{
    bantumi_search_steps = 100;
#ifdef BANTUMI_PLATFORM_SEARCH
    bantumi_search_gb();
#else
    search();
#endif
    if (!bantumi_search_done)
        return;
    if (b.turn == TURN_HINT) {
        b.cursor = bantumi_pick;
        b.turn = TURN_PLAYER;
    } else {
        b.turn = TURN_PHONE_HAND;
        b.hand_pit = bantumi_pick;
        b.chosen = bantumi_pick;
        b.closed = 0;
        hand_path(bantumi_pick, bantumi_pick);
    }
}

/* The board, and on the open board the hand (or the phone thinking) and
   the digits. All sprites are made afresh. */
static void think_create(void)
{
    b.think_ticks = 2;
    b.think_frame = 0;
    b.think_id = create(THINKING, SPRITE_MODE_OPAQUE, 2, THINK_X, THINK_Y);
}

static void draw_board(uint8_t frame)
{
    uint8_t i;

    sprite_reset(SPRITES);
    b.board_id = create(BOARDS + 12ul * frame, SPRITE_MODE_OPAQUE, 0, 0, 0);
    if (frame != 0)
        return;
    if (b.hand_pit == NO_PIT) {
        think_create();
    } else {
        b.closed = 0;
        hand_at(b.cursor);
        draw_hand(1);
    }
    for (i = 0; i < PITS; i++) {
        uint8_t store = i == STORE || i == PHONE_STORE;
        int8_t n = b.pits[i];
        struct sprite_image im;

        b.pits[i] = 0;
        digit(&im, 0);
        b.units[i] = (uint8_t)sprite_create(&im, SPRITE_MODE_XOR, 2, pit_x(i) + (store ? 5 : 4),
                                            pit_y(i) + (store ? 5 : 2));
        set_pit(i, n);
    }
}

/* The hand takes a pit's beans and sets out for the next pit. */
static void pick(uint8_t pit)
{
    b.sow_pit = (uint8_t)(pit + 1);
    hand_path(pit, (uint8_t)(pit + 1));
    if (b.turn == TURN_PHONE) {
        free_id(b.think_id);
    } else if (b.turn == TURN_PLAYER) {
        free_id(b.mask_id);
        free_id(b.image_id);
    }
    b.closed = 1;
    draw_hand(1);
    b.in_hand = (uint8_t)b.pits[pit];
    set_pit(pit, 0);
    b.sow = SOW_BEAN;
}

/* The end of a move; `pass` gives the turn to the other side. With a row
   empty, each side's beans go to its store and the game is over. */
static void turn_end(uint8_t pass)
{
    uint8_t i;

    b.anim = HAND_IDLE;
    free_id(b.mask_id);
    free_id(b.image_id);
    if (pass)
        b.turn = b.turn == TURN_PHONE ? TURN_PLAYER : TURN_PHONE;
    if (!row_empty((const uint8_t *)b.pits)) {
        if (b.turn == TURN_PHONE) {
            b.hand_pit = 7;
            ai_start();
            think_create();
        } else if (b.turn == TURN_PLAYER) {
            b.hand_pit = 1;
            b.closed = 0;
            hand_at(b.cursor);
            draw_hand(1);
        }
        return;
    }
    for (i = 0; i < 6; i++) {
        set_pit(PHONE_STORE, (int8_t)(b.pits[PHONE_STORE] + b.pits[7 + i]));
        set_pit((uint8_t)(7 + i), 0);
        set_pit(STORE, (int8_t)(b.pits[STORE] + b.pits[i]));
        set_pit(i, 0);
    }
    if (b.pits[STORE] > b.pits[PHONE_STORE]) {
        b.won = 1;
    } else if (b.pits[STORE] < b.pits[PHONE_STORE]) {
        sprite_set_mode(b.units[PHONE_STORE], SPRITE_MODE_OPAQUE_BLINK);
        if (b.pits[PHONE_STORE] >= 10)
            sprite_set_mode(b.tens[PHONE_STORE], SPRITE_MODE_OPAQUE_BLINK);
    }
    b.turn = TURN_OVER_PAUSE;
}

/* The thinking picture's next frame every other tick. */
static int think_tick(void)
{
    struct sprite_image im;

    if (--b.think_ticks)
        return GAME_RESULT_NONE;
    b.think_ticks = 2;
    b.think_frame = (uint8_t)((b.think_frame + 1) & 7);
    image(&im, THINKING + 12ul * b.think_frame);
    sprite_set_image(b.think_id, &im);
    return GAME_RESULT_REDRAW;
}

/* One bean from the hand, or the end of a capture. */
static void act(void)
{
    if (b.sow == SOW_CAPTURE_LAST) {
        set_pit(b.capture_store, (int8_t)(b.pits[b.capture_store] + b.in_hand));
        turn_end(1);
        b.sow = SOW_IDLE;
        return;
    }
    if (b.sow == SOW_CAPTURE) {
        b.sow = SOW_CAPTURE_LAST;
        b.hand_pit = (uint8_t)(12 - b.sow_pit);
        b.turn = b.turn == TURN_PLAYER ? TURN_PLAYER_HAND : TURN_PHONE_HAND;
        hand_path(b.hand_pit, b.hand_pit);
        set_pit(b.capture_store, (int8_t)(b.pits[b.capture_store] + b.in_hand));
        b.in_hand = (uint8_t)b.pits[b.hand_pit];
        b.closed = 0;
        draw_hand(0);
        return;
    }
    if (b.sow != SOW_BEAN)
        return;
    set_pit(b.sow_pit, (int8_t)(b.pits[b.sow_pit] + 1));
    b.in_hand--;
    b.sound = 1;
    if (b.in_hand == 0) {
        uint8_t p = b.sow_pit;

        if (p == STORE || p == PHONE_STORE) {
            turn_end(0);
        } else if (b.pits[p] == 1 && ((b.turn == TURN_PLAYER && p < 6) || (b.turn == TURN_PHONE && p > 6))) {
            b.sow = b.pits[12 - p] != 0 ? SOW_CAPTURE : SOW_CAPTURE_LAST;
            b.capture_store = b.turn == TURN_PLAYER ? STORE : PHONE_STORE;
            set_pit(p, 1);
            b.turn = b.turn == TURN_PLAYER ? TURN_PLAYER_HAND : TURN_PHONE_HAND;
            b.hand_pit = p;
            b.closed = 0;
            hand_path(p, p);
            b.in_hand = 1;
        } else {
            turn_end(1);
        }
        return;
    }
    {
        uint8_t from = b.sow_pit;

        b.sow_pit++;
        if ((b.sow_pit == PHONE_STORE && b.turn == TURN_PLAYER) || b.sow_pit == PITS)
            b.sow_pit = 0;
        else if (b.sow_pit == STORE && b.turn == TURN_PHONE)
            b.sow_pit++;
        b.hand_pit = b.sow_pit;
        hand_path(from, b.sow_pit);
    }
}

/* Moves a coordinate at most 6 pixels toward its target. */
static int8_t toward(int8_t at, int8_t to)
{
    if (at > to + 6)
        return (int8_t)(at - 6);
    if (at < to - 6)
        return (int8_t)(at + 6);
    return to;
}

static int tick(struct game_context *ctx)
{
    int result = GAME_RESULT_NONE;

    switch (b.anim) {
    case HAND_MOVE:
        b.x = toward(b.x, b.tx);
        b.y = toward(b.y, b.ty);
        if (b.x == b.tx && b.y == b.ty)
            b.anim = HAND_ARRIVE;
        b.counter = 2;
        draw_hand(0);
        result = GAME_RESULT_REDRAW;
        break;
    case HAND_WAIT:
        if (--b.counter == 0) {
            draw_hand(0);
            b.anim = HAND_MOVE;
            result = GAME_RESULT_REDRAW;
        }
        break;
    case HAND_ARRIVE:
        if (b.counter == 2) {
            b.closed = b.turn == TURN_PLAYER_HAND || b.turn == TURN_PHONE_HAND;
            draw_hand(0);
            result = GAME_RESULT_REDRAW;
        }
        if (--b.counter != 0)
            break;
        if (b.turn != TURN_PLAYER_HAND && b.turn != TURN_PHONE_HAND) {
            b.closed = 1;
            draw_hand(0);
            b.anim = HAND_ACT;
            result = GAME_RESULT_REDRAW;
        } else if (b.sow == SOW_CAPTURE_LAST || b.sow == SOW_CAPTURE) {
            hand_path(b.hand_pit, b.capture_store);
            set_pit(b.hand_pit, 0);
            b.turn = b.turn == TURN_PLAYER_HAND ? TURN_PLAYER : TURN_PHONE;
        } else {
            free_id(b.mask_id);
            free_id(b.image_id);
            if (b.turn == TURN_PLAYER_HAND) {
                b.turn = TURN_PLAYER;
                b.hand_pit = b.cursor;
            } else {
                b.turn = TURN_PHONE;
                b.hand_pit = b.chosen;
            }
            pick(b.hand_pit);
        }
        break;
    case HAND_ACT:
        act();
        result = GAME_RESULT_REDRAW;
        break;
    default:
        switch (b.turn) {
        case TURN_PHONE:
            b.hand_pit = NO_PIT;
            result = think_tick();
            ai_step();
            if (b.sow == SOW_BEAN)
                result = GAME_RESULT_REDRAW;
            break;
        case TURN_HINT:
            result = think_tick();
            ai_step();
            if (b.turn == TURN_PLAYER) {
                free_id(b.think_id);
                b.closed = 0;
                hand_at(b.cursor);
                b.hand_pit = b.cursor;
                draw_hand(1);
                result = GAME_RESULT_REDRAW;
            }
            break;
        case TURN_OVER_PAUSE:
            if (--b.countdown == 0) {
                b.turn = TURN_OVER;
                ctx->period = 120;
                result = GAME_RESULT_REDRAW;
            } else if (b.won) {
                winner_mode = winner_mode == SPRITE_MODE_XOR ? SPRITE_MODE_HIDDEN : SPRITE_MODE_XOR;
                sprite_set_mode(b.units[STORE], winner_mode);
                if (b.pits[STORE] >= 10)
                    sprite_set_mode(b.tens[STORE], winner_mode);
                result = GAME_RESULT_REDRAW;
            }
            break;
        case TURN_INTRO:
            if (b.intro_step < 5) {
                sprite_move(b.board_id, 40 - 8 * b.intro_step, 0);
                b.intro_step++;
                result = GAME_RESULT_REDRAW;
            } else if (b.intro_frame == 0) {
                ctx->period = 0;
                b.turn = TURN_PLAYER;
                result = GAME_RESULT_REDRAW;
            } else {
                if (b.intro_toggle) {
                    b.intro_frame--;
                    draw_board(b.intro_frame);
                    result = GAME_RESULT_REDRAW;
                }
                b.intro_toggle = !b.intro_toggle;
            }
            break;
        case TURN_OVER:
            ctx->period = 0;
            ctx->score = (uint32_t)(int32_t)(b.pits[STORE] - b.pits[PHONE_STORE]);
            result = GAME_RESULT_END;
            break;
        default:
            break;
        }
        break;
    }
    /* A flag the game flips every fifth tick of the player's turn: nothing
       reads it, but it asks for a redraw. */
    if (b.anim == HAND_IDLE && b.turn == TURN_PLAYER && --b.flag_ticks == 0) {
        b.flag_ticks = 5;
        b.flag = !b.flag;
        result = GAME_RESULT_REDRAW;
    }
    if (b.sound) {
        b.sound = 0;
        ctx->sound = BANTUMI_SOUND_BEAN;
        result = GAME_RESULT_SOUND;
    }
    return result;
}

static int new_game(struct game_context *ctx)
{
    uint8_t i;

    sprite_reset(SPRITES);
    memset(&b, 0, sizeof b);
    b.countdown = 40;
    b.level = ctx->level;
    b.flag_ticks = 5;
    for (i = 0; i < PITS; i++)
        b.pits[i] = (int8_t)(i == STORE || i == PHONE_STORE ? 0 : 4);
    b.intro_frame = 3;
    draw_board(3);
    sprite_move(b.board_id, 40, 0);
    ctx->period = 120;
    return GAME_RESULT_RESTART_TICK;
}

/* Back from the pause menu: the phone makes its sprites afresh. */
static int resume(struct game_context *ctx)
{
    b.mask_id = b.image_id = 0;
    draw_board(0);
    ctx->one_shot = 0;
    if (b.turn != TURN_OVER_PAUSE)
        ctx->period = 120;
    return GAME_RESULT_NONE;
}

/* The cursor one pit along, on the player's turn with the hand still. */
static int cursor_move(int8_t by)
{
    if (b.turn != TURN_PLAYER || b.anim != HAND_IDLE)
        return GAME_RESULT_NONE;
    b.cursor = (uint8_t)(b.cursor + by);
    hand_at(b.cursor);
    sprite_move(b.mask_id, b.x, b.y);
    sprite_move(b.image_id, b.x, b.y);
    return GAME_RESULT_REDRAW;
}

int bantumi_handler(int event, struct game_context *ctx)
{
    switch (event) {
    case GAME_EVENT_START:
        ctx->period = 120;
        return new_game(ctx);
    case GAME_EVENT_TICK:
        return tick(ctx);
    case GAME_KEY_SCROLL_DOWN:
    case GAME_KEY_4:
        return b.cursor != 0 ? cursor_move(-1) : GAME_RESULT_NONE;
    case GAME_KEY_SCROLL_UP:
    case GAME_KEY_6:
        return b.cursor < 5 ? cursor_move(1) : GAME_RESULT_NONE;
    case 0x08:
    case GAME_KEY_0:
    case GAME_KEY_5:
        if (b.pits[b.cursor] <= 0 || b.turn != TURN_PLAYER || b.anim != HAND_IDLE)
            return GAME_RESULT_NONE;
        b.turn = TURN_PLAYER_HAND;
        ctx->period = 120;
        b.hand_pit = b.cursor;
        hand_path(b.cursor, b.cursor);
        return GAME_RESULT_RESTART_TICK;
    case GAME_KEY_STAR:
        if (b.turn != TURN_PLAYER || b.anim != HAND_IDLE || b.level != 1)
            return GAME_RESULT_NONE;
        b.turn = TURN_HINT;
        think_create();
        ai_start();
        free_id(b.mask_id);
        free_id(b.image_id);
        return GAME_RESULT_REDRAW;
    case 0x13:
    case 0x15:
    case 0x17:
    case 0x18:
    case 0x1a:
    case 0x1c:
    case 0x1d:
        /* The search starts over. */
        if (b.turn == TURN_PHONE || b.turn == TURN_HINT)
            ai_start();
        return GAME_RESULT_UNUSED;
    case GAME_EVENT_RESUME:
        return resume(ctx);
    default:
        return GAME_RESULT_NONE;
    }
}

/* The phone asks for a redraw on every tick it thinks, when as a rule
   nothing has moved: the picture is drawn again only when a sprite has
   changed. */
void bantumi_render(void)
{
    if (!sprite_changed)
        return;
    sprite_changed = 0;
    sprite_render();
}
