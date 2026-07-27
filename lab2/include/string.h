#ifndef _STRING_H_
#define _STRING_H_

#include <stddef.h>

void *memset(void *dst, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
void strcpy(char *dst, const char *src);
char *strchr(const char *str, int ch);

#endif // _STRING_H_
