#include "rand.h"

static uint32_t seed = 1;

void game_srand(uint32_t s)
{
    seed = s;
}

int game_rand(void)
{
    seed = seed * 0x41c64e6du + 0x3039u;
    return (int)((seed & 0x7fffffffu) >> 16);
}
