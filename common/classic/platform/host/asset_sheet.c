/* Draws the extracted assets through the core's blitter and writes them as
   PGM frames, to check the extraction and the bitmap format by eye. */
#include <stdio.h>

#include "game_assets.h"
#include "lcd.h"
#include "pgm.h"
#include "phone.h"

static int write_frame(const char *dir, const char *name)
{
    char path[1024];

    snprintf(path, sizeof path, "%s/%s.pgm", dir, name);
    if (pgm_write_lcd(path) != 0) {
        perror(path);
        return 1;
    }
    printf("wrote %s\n", path);
    return 0;
}

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    int tiles = (int)sizeof game_tile_bitmaps / 7;
    int failed = 0;
    int i;

    /* Tiles, ten per row on an 8 px pitch; 60 fit on one screen. */
    for (i = 0; i < tiles; i++) {
        if (i % 60 == 0)
            lcd_clear();
        lcd_blit_bitmap(i % 10 * 8, i % 60 / 10 * 8, 7, 7, game_tile_bitmaps + i * 7);
        if (i % 60 == 59 || i == tiles - 1)
            failed |= write_frame(dir, i < 60 ? "sheet_tiles_0" : "sheet_tiles_1");
    }

    lcd_clear();
#if PHONE_ROTATION
    for (i = 0; i < 10; i++)
        lcd_blit_bitmap(i * 4, 0, 3, 5, games_digit_glyphs + i * 3);
#endif
    lcd_blit_bitmap(0, 8, 7, 7, game_tile_back);
    lcd_blit_bitmap(10, 8, 7, 7, game_tile_cursor);
    lcd_blit_bitmap(20, 8, 4, 4, snake_food_bitmap);
#if PHONE_ROTATION
    for (i = 0; i < 8; i++)
        lcd_blit_bitmap(i % 4 * 12, 20 + i / 4 * 12, 10, 10, game3_sprites + i * 20);
#endif
    failed |= write_frame(dir, "sheet_small");

#if PHONE_ROTATION
    lcd_clear();
    lcd_blit_bitmap(0, 0, LCD_WIDTH, LCD_HEIGHT, game3_background);
    failed |= write_frame(dir, "sheet_background");
#endif

    return failed;
}
