/* The firmware's rand/srand (the ANSI C example generator). */
#ifndef CORE_RAND_H
#define CORE_RAND_H

#include <stdint.h>

void game_srand(uint32_t seed);
int game_rand(void);

#endif
