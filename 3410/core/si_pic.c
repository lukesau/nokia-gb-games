#include "si_pic.h"

#include <string.h>

#include "lcd.h"

/* A platform short of work RAM may keep them elsewhere, as si.c its state,
   which comes after them. */
#ifdef SI_PIC_RENDER_ONLY
/* Only the drawing, built again where another game's pictures are. */
#elif defined SI_PICS_AT
__at(SI_PICS_AT) struct si_pic si_pics[SI_PIC_COUNT + 1];
typedef char si_pics_fit[SI_PICS_AT + sizeof si_pics <= SI_STATE_AT ? 1 : -1];
#else
struct si_pic si_pics[SI_PIC_COUNT + 1];
#endif

/* The list: si_pics[0] holds its first (next) and last (prev). */
#define FIRST si_pics[0].next
#define LAST si_pics[0].prev

#ifndef SI_PIC_RENDER_ONLY

void si_pic_reset(void) SI_FAR
{
    memset(si_pics, 0, sizeof si_pics);
}

static uint8_t alloc(uint8_t kind, uint8_t mode, int x, int y)
{
    uint8_t id;
    struct si_pic *p;

    for (id = 1; id <= SI_PIC_COUNT && si_pics[id].kind != SI_PIC_FREE; id++)
        ;
    if (id > SI_PIC_COUNT)
        return 0;
    p = &si_pics[id];
    memset(p, 0, sizeof *p);
    p->kind = kind;
    p->mode = mode;
    p->x = (int16_t)x;
    p->y = (int16_t)y;
    /* Appended. */
    p->prev = LAST;
    if (LAST)
        si_pics[LAST].next = id;
    else
        FIRST = id;
    LAST = id;
    return id;
}

uint8_t si_pic_create(uint8_t mode, int x, int y, const struct sprite_image *frames, uint8_t count) SI_FAR
{
    uint8_t id = alloc(SI_PIC_BITMAP, mode, x, y);

    if (id) {
        si_pics[id].frames = frames;
        si_pics[id].count = count;
    }
    return id;
}

uint8_t si_pic_create_fill(uint8_t mode, int x, int y, int w, int h) SI_FAR
{
    uint8_t id = alloc(SI_PIC_FILL, mode, x, y);

    if (id) {
        si_pics[id].x2 = (int16_t)w;
        si_pics[id].y2 = (int16_t)h;
    }
    return id;
}

uint8_t si_pic_create_line(uint8_t mode, int x, int y, int x2, int y2) SI_FAR
{
    uint8_t id = alloc(SI_PIC_LINE, mode, x, y);

    if (id) {
        si_pics[id].x2 = (int16_t)x2;
        si_pics[id].y2 = (int16_t)y2;
    }
    return id;
}

uint8_t si_pic_create_masked(uint8_t mode, int x, int y, const struct si_masked_image *image) SI_FAR
{
    uint8_t id = alloc(SI_PIC_MASKED, mode, x, y);

    if (id) {
        si_pics[id].frames = &image->image;
        si_pics[id].count = 1;
    }
    return id;
}

static void unlink_pic(uint8_t id)
{
    struct si_pic *p = &si_pics[id];

    if (p->prev)
        si_pics[p->prev].next = p->next;
    else
        FIRST = p->next;
    if (p->next)
        si_pics[p->next].prev = p->prev;
    else
        LAST = p->prev;
    p->next = p->prev = 0;
}

void si_pic_free(uint8_t id) SI_FAR
{
    if (!id)
        return;
    unlink_pic(id);
    memset(&si_pics[id], 0, sizeof si_pics[id]);
}

void si_pic_move_after(uint8_t id, uint8_t after) SI_FAR
{
    struct si_pic *p = &si_pics[id];

    if (!id || id == after)
        return;
    unlink_pic(id);
    if (!after) {
        p->next = FIRST;
        if (FIRST)
            si_pics[FIRST].prev = id;
        else
            LAST = id;
        FIRST = id;
        return;
    }
    p->prev = after;
    p->next = si_pics[after].next;
    if (p->next)
        si_pics[p->next].prev = id;
    else
        LAST = id;
    si_pics[after].next = id;
}

void si_pic_set_frames(uint8_t id, const struct sprite_image *frames, uint8_t count) SI_FAR
{
    si_pics[id].frames = frames;
    si_pics[id].count = count;
    si_pics[id].frame = 0;
}

void si_pic_next_frame(uint8_t id) SI_FAR
{
    struct si_pic *p = &si_pics[id];

    if (++p->frame >= p->count)
        p->frame = 0;
}

#endif

static void plot(int x, int y, uint8_t set, uint8_t op)
{
    uint8_t *d, bit;

    if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT)
        return;
    d = &sprite_screen[(y >> 3) * LCD_WIDTH + x];
    bit = (uint8_t)(1u << (y & 7));
    switch (op) {
    case SI_OP_COPY:
        *d = set ? (uint8_t)(*d | bit) : (uint8_t)(*d & ~bit);
        break;
    case SI_OP_OR:
        if (set)
            *d |= bit;
        break;
    case SI_OP_XOR:
        if (set)
            *d ^= bit;
        break;
    }
}

/* Combines `bits` into `count` bytes of the screen, `mask` the rows they
   cover: a column of eight rows at a time, as the screen is laid out. A
   platform with sprite.c's sprite_band (SPRITE_PLATFORM_BAND) does it
   with that. */
#ifdef SPRITE_PLATFORM_BAND
extern uint8_t *sprite_band_dst;
extern const uint8_t *sprite_band_src;
extern uint8_t sprite_band_n, sprite_band_valid, sprite_band_mode;
extern int8_t sprite_band_up;
void sprite_band(void);

/* A fill's bits, all set. */
static const uint8_t set_bits[LCD_WIDTH] = {
#define SET8 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
    SET8, SET8, SET8, SET8, SET8, SET8, SET8, SET8, SET8, SET8, SET8, SET8
#undef SET8
};
typedef char set_bits_fits[LCD_WIDTH == 96 ? 1 : -1];

static void put_column_bits(uint8_t *d, const uint8_t *src, uint8_t count, uint8_t shift, uint8_t left,
                            uint8_t mask, uint8_t op)
{
    sprite_band_dst = d;
    sprite_band_src = src ? src : set_bits;
    sprite_band_n = count;
    sprite_band_up = (int8_t)(left ? shift : -shift);
    sprite_band_valid = mask;
    sprite_band_mode = op == SI_OP_COPY ? SPRITE_MODE_OPAQUE : op == SI_OP_OR ? SPRITE_MODE_SET : SPRITE_MODE_XOR;
    sprite_band();
}
#else
static void put_column_bits(uint8_t *d, const uint8_t *src, uint8_t count, uint8_t shift, uint8_t left,
                            uint8_t mask, uint8_t op)
{
    uint8_t bits;

    /* The bits a source byte gives this band: shifted down into it from
       the band above (left) or up (not). */
    for (; count; count--, d++) {
        bits = src ? (uint8_t)(left ? *src++ << shift : *src++ >> shift) : 0xff;
        bits &= mask;
        switch (op) {
        case SI_OP_COPY:
            *d = (uint8_t)((*d & ~mask) | bits);
            break;
        case SI_OP_OR:
            *d |= bits;
            break;
        case SI_OP_XOR:
            *d ^= bits;
            break;
        }
    }
}
#endif

/* Where each band starts in sprite_screen: the Game Boy multiplies slowly. */
static const uint16_t band_start[SPRITE_SCREEN_BANDS] = {
    0, LCD_WIDTH, 2 * LCD_WIDTH, 3 * LCD_WIDTH, 4 * LCD_WIDTH, 5 * LCD_WIDTH, 6 * LCD_WIDTH, 7 * LCD_WIDTH,
    8 * LCD_WIDTH
};
typedef char band_start_fits[SPRITE_SCREEN_BANDS == 9 ? 1 : -1];

#define LAST_BAND (SPRITE_SCREEN_BANDS - 1)
#define LAST_BAND_ROWS ((LCD_HEIGHT & 7) ? (uint8_t)(0xff >> (8 - (LCD_HEIGHT & 7))) : 0xff)

/* One band of the bitmap's rows (`rows` of it set) into the screen's band
   `band`, moved down `shift` rows, the rows that spill over into the band
   after. */
static void bitmap_band(const uint8_t *src, uint8_t cols, uint8_t x, int8_t band, uint8_t shift, uint8_t rows,
                        uint8_t op)
{
    uint8_t mask;

    if (band >= 0 && band <= LAST_BAND) {
        mask = (uint8_t)(rows << shift);
        if (band == LAST_BAND)
            mask &= LAST_BAND_ROWS;
        if (mask)
            put_column_bits(&sprite_screen[band_start[band] + x], src, cols, shift, 1, mask, op);
    }
    band++;
    if (shift && band >= 0 && band <= LAST_BAND) {
        mask = (uint8_t)(rows >> (8 - shift));
        if (band == LAST_BAND)
            mask &= LAST_BAND_ROWS;
        if (mask)
            put_column_bits(&sprite_screen[band_start[band] + x], src, cols, (uint8_t)(8 - shift), 0, mask, op);
    }
}

static void draw_bitmap(const struct sprite_image *im, int16_t x, int16_t y, uint8_t op)
{
    uint8_t w = im->w, left = im->h, col = 0, end = w, shift;
    const uint8_t *src;
    int8_t band;

    /* One starting in the screen's last column or row is not drawn: seen
       in MAME, where a picture at x 94 shows its first two columns and one
       at 95 nothing, and a one-pixel star on the last row is not shown. */
    if (x >= LCD_WIDTH - 1 || y >= LCD_HEIGHT - 1)
        return;
    /* Wholly off the left or the top: nothing to draw. */
    if (x + w <= 0 || y + left <= 0)
        return;
    if (x < 0)
        col = (uint8_t)-x;
    if (x + w > LCD_WIDTH)
        end = (uint8_t)(LCD_WIDTH - x);
    if (col >= end)
        return;
    /* The band the top row is in, rounding down above the screen. */
    shift = (uint8_t)y & 7;
    band = (int8_t)((y - shift) >> 3);
    for (src = im->bitmap + col; left; src += w, band++) {
        uint8_t rows = left >= 8 ? 0xff : (uint8_t)(0xff >> (8 - left));

        if (band > LAST_BAND)
            break;
        bitmap_band(src, (uint8_t)(end - col), (uint8_t)(x + col), band, shift, rows, op);
        left = left >= 8 ? (uint8_t)(left - 8) : 0;
    }
}

static void draw_fill(const struct si_pic *p, uint8_t op)
{
    int16_t right = p->x + p->x2, under = p->y + p->y2;
    uint8_t x, end, top, bottom, band, first, last;

    if (right <= 0 || under <= 0 || p->x >= LCD_WIDTH || p->y >= LCD_HEIGHT)
        return;
    x = p->x < 0 ? 0 : (uint8_t)p->x;
    top = p->y < 0 ? 0 : (uint8_t)p->y;
    end = right > LCD_WIDTH ? LCD_WIDTH : (uint8_t)right;
    bottom = under > LCD_HEIGHT ? LCD_HEIGHT : (uint8_t)under;
    if (x >= end || top >= bottom)
        return;
    for (band = top >> 3; band <= (uint8_t)((bottom - 1) >> 3); band++) {
        first = band * 8 > top ? band * 8 : top;
        last = band * 8 + 8 < bottom ? band * 8 + 8 : bottom;
        /* A whole band set is a plain fill. */
        if (last - first == 8 && op != SI_OP_XOR)
            memset(&sprite_screen[band_start[band] + x], 0xff, (uint8_t)(end - x));
        else
            put_column_bits(&sprite_screen[band_start[band] + x], 0, (uint8_t)(end - x), 0, 1,
                            (uint8_t)((0xff >> (8 - (last - first))) << (first - band * 8)), op);
    }
}

static void draw_line(const struct si_pic *p, uint8_t op)
{
    int x = p->x, y = p->y, dx = p->x2 - x, dy = p->y2 - y, sx = dx < 0 ? -1 : 1, sy = dy < 0 ? -1 : 1, err;

    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    err = dx - dy;
    for (;;) {
        plot(x, y, 1, op);
        if (x == p->x2 && y == p->y2)
            return;
        if (2 * err > -dy) {
            err -= dy;
            x += sx;
        }
        if (2 * err < dx) {
            err += dx;
            y += sy;
        }
    }
}

void si_pic_render(void) SI_FAR
{
    uint8_t id;

    memset(sprite_screen, 0, sizeof sprite_screen);
    for (id = FIRST; id; id = si_pics[id].next) {
        const struct si_pic *p = &si_pics[id];
        uint8_t op = si_pic_op(p->mode);

        if (op == SI_OP_NONE)
            continue;
        switch (p->kind) {
        case SI_PIC_BITMAP:
            draw_bitmap(&p->frames[p->frame], p->x, p->y, op);
            break;
        case SI_PIC_MASKED: {
            /* The mask's bits cleared (set, then flipped), then the
               bitmap's set: its bits all lie within the mask's. */
            const struct si_masked_image *m = (const struct si_masked_image *)p->frames;
            struct sprite_image mask;

            mask.bitmap = m->mask;
            mask.w = m->image.w;
            mask.h = m->image.h;
            draw_bitmap(&mask, p->x, p->y, SI_OP_OR);
            draw_bitmap(&mask, p->x, p->y, SI_OP_XOR);
            draw_bitmap(&m->image, p->x, p->y, SI_OP_OR);
            break;
        }
        case SI_PIC_FILL:
            draw_fill(p, op);
            break;
        case SI_PIC_LINE:
            draw_line(p, op);
            break;
        }
    }
}
