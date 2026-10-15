/* Host checks for the core services. Needs no firmware. */
#include <stdio.h>

#include "lcd.h"
#include "rand.h"

static int failures;

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failures++; \
        } \
    } while (0)

static void test_rand(void)
{
    /* seed 1: 1103527590, 2524885223, 662824084 -> bits 30..16 */
    game_srand(1);
    CHECK(game_rand() == 16838);
    CHECK(game_rand() == 5758);
    CHECK(game_rand() == 10113);
}

static void test_blit(void)
{
    /* 3 columns, 10 rows: two bytes per column, bit (y & 7) is row y. */
    static const uint8_t bitmap[] = { 0x01, 0x00, 0x80, 0x01, 0x00, 0x02 };
    int x, y, set = 0;

    lcd_fill_rect(0, 0, LCD_WIDTH, LCD_HEIGHT, 1);
    lcd_blit_bitmap(2, 3, 3, 10, bitmap);
    CHECK(lcd_pixel(2, 3) == 1);
    CHECK(lcd_pixel(3, 10) == 1);
    CHECK(lcd_pixel(3, 11) == 1);
    CHECK(lcd_pixel(4, 12) == 1);
    for (y = 3; y < 13; y++)
        for (x = 2; x < 5; x++)
            set += lcd_pixel(x, y);
    CHECK(set == 4);

    /* Clipped at every edge without writing outside the buffer. */
    lcd_clear();
    lcd_blit_bitmap(-1, -1, 3, 10, bitmap);
    lcd_blit_bitmap(LCD_WIDTH - 1, LCD_HEIGHT - 1, 3, 10, bitmap);
    lcd_fill_rect(-5, -5, 6, 6, 1);
    CHECK(lcd_pixel(0, 0) == 1);
    CHECK(lcd_pixel(LCD_WIDTH - 1, LCD_HEIGHT - 1) == 1);
}

int main(void)
{
    test_rand();
    test_blit();
    if (failures)
        return 1;
    printf("ok\n");
    return 0;
}
