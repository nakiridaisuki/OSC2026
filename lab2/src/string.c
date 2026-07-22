#include "string.h"
#include <stddef.h>

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

void *memset(void *dst, int c, size_t n) {
    char *cdst = (char *)dst;
    for (size_t i = 0; i < n; i++) {
        cdst[i] = c;
    }
    return dst;
}
void *memcpy(void *dst, const void *src, size_t n) {
    char *d = (char *)dst;
    char *s = (char *)src;

    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

size_t strlen(const char *s) {
    int len = 0;
    while (*s++)
        len++;
    return len;
}
