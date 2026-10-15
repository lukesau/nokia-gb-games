/* 1-bit framebuffer and the drawing primitives the games use.

   The phone's LCD is 84x48. A platform may give the core a bigger
   framebuffer, the size of its own screen; the phone's LCD is then a window
   in the middle of it. Drawing happens in a view, which is either that
   window or the whole framebuffer, and is clipped to it. */
#ifndef CORE_LCD_H
#define CORE_LCD_H

#include <stdint.h>

#define LCD_WIDTH 84
#define LCD_HEIGHT 48

/* Framebuffer size; a platform overrides both on the compiler command line. */
#ifndef LCD_FB_WIDTH
#define LCD_FB_WIDTH LCD_WIDTH
#define LCD_FB_HEIGHT LCD_HEIGHT
#endif

#define LCD_STRIDE ((LCD_FB_WIDTH + 7) / 8) /* bytes per row */
#define LCD_CELLS_X LCD_STRIDE
#define LCD_CELLS_Y (LCD_FB_HEIGHT / 8)

/* Where the phone's LCD sits in the framebuffer. */
#define LCD_PHONE_X ((LCD_FB_WIDTH - LCD_WIDTH) / 2)
#define LCD_PHONE_Y ((LCD_FB_HEIGHT - LCD_HEIGHT) / 2)
/* A platform may show part of the framebuffer magnified by this much: the
   core names a rectangle, and the platform shows it LCD_ZOOM times bigger
   in the middle of the screen, over whatever lies under it. */
#ifndef LCD_ZOOM
#define LCD_ZOOM 1
#endif
/* A platform may magnify the games played on the phone's LCD in the
   full-screen variant by this much instead, when it is more than LCD_ZOOM,
   cutting off on the right what does not fit across its screen. */
#ifndef LCD_GAME_ZOOM
#define LCD_GAME_ZOOM LCD_ZOOM
#endif
/* The first screen row below the phone's LCD when it is the one magnified. */
#define LCD_BELOW_PHONE (LCD_FB_HEIGHT / 2 + LCD_HEIGHT * LCD_ZOOM / 2)
/* Whether there is room around the phone's LCD. */
#define LCD_HAS_SURROUND (LCD_FB_WIDTH > LCD_WIDTH)

/* One bit per pixel, row-major, the leftmost pixel in bit 7. A set bit is a
   dark pixel. */
extern uint8_t lcd_fb[LCD_STRIDE * LCD_FB_HEIGHT];

/* Bit of pixel x within its byte. */
extern const uint8_t lcd_bit[8];

/* Nonzero for each 8x8 cell of the framebuffer drawn to since the platform
   last cleared it: cell x / 8 + LCD_CELLS_X * (y / 8). Lets a platform
   update only what changed. */
extern uint8_t lcd_dirty[LCD_CELLS_X * LCD_CELLS_Y];

/* The view: its top-left corner in the framebuffer and its size. */
extern uint8_t lcd_view_x, lcd_view_y, lcd_view_w, lcd_view_h;

void lcd_view_phone(void);
void lcd_view_full(void);
void lcd_view_set(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/* The magnified rectangle, in framebuffer coordinates, and how many times
   bigger it is shown, LCD_ZOOM or LCD_GAME_ZOOM; none when lcd_zoom_w is
   0. */
extern uint8_t lcd_zoom_x, lcd_zoom_y, lcd_zoom_w, lcd_zoom_h, lcd_zoom_by;

void lcd_zoom_set(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t by);

/* The pixel shown at a position on the screen, magnification applied. */
uint8_t lcd_screen_pixel(uint8_t x, uint8_t y);

/* A pixel by framebuffer position, and by position in the view. */
#define lcd_fb_pixel(x, y) ((lcd_fb[(y) * LCD_STRIDE + ((x) >> 3)] & lcd_bit[(x) & 7]) != 0)
#define lcd_pixel(x, y) lcd_fb_pixel((x) + lcd_view_x, (y) + lcd_view_y)

/* Marks the cells touched by a rectangle given in framebuffer coordinates
   that lies inside the framebuffer. */
void lcd_mark_dirty(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/* Everything below takes coordinates in the view. */

void lcd_clear(void); /* clears the view */
/* Color 0 clears, 1 sets, LCD_INVERT flips every pixel of the rectangle. */
#define LCD_INVERT 2
void lcd_fill_rect(int x, int y, int w, int h, uint8_t color);

/* The innermost loops of the two routines above and below, which work down
   a column of framebuffer bytes one row apart, starting at p and touching
   the bits of `mask` in each. A platform whose compiler makes slow work of
   them may supply its own and define LCD_PLATFORM_COLUMNS. Their other
   arguments are in variables, to keep the calls cheap:
   lcd_column_fill sets, clears or inverts, by lcd_column_color, in
   lcd_column_rows bytes; lcd_column_blit sets or clears by the bits of
   lcd_column_bits from bit 0 up, in lcd_column_rows bytes, 1 to 8. */
extern uint8_t lcd_column_rows, lcd_column_color, lcd_column_bits;
void lcd_column_fill(uint8_t *p, uint8_t mask);
void lcd_column_blit(uint8_t *p, uint8_t mask);

/* Column-major bitmap as stored in the firmware: (h + 7) / 8 bytes per
   column, bit (y & 7) of each byte is row y. Draws both set and clear
   pixels; anything outside the view is clipped. */
void lcd_blit_bitmap(int x, int y, int w, int h, const uint8_t *data);

/* Bitmap stored as strips of 8 rows, w bytes per strip (the LCD
   controller's own layout): bit (y & 7) of byte x + w * (y / 8). */
void lcd_blit_strips(int x, int y, int w, int h, const uint8_t *data);

#endif
