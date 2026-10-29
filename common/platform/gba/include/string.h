/* The build is freestanding; common/platform/gba/libc.c holds these. */
#ifndef GBA_STRING_H
#define GBA_STRING_H

#include <stddef.h>

void *memset(void *dst, int value, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
int memcmp(const void *a, const void *b, size_t n);
size_t strlen(const char *s);
char *strcpy(char *dst, const char *src);

#endif
