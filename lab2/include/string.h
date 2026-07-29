#ifndef _STRING_H_
#define _STRING_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static inline char tolower(char c) {
    if ('A' <= c && c <= 'Z')
        return c - 'A' + 'a';
    return c;
}
static inline bool isdigit(char c) { return '0' <= c && c <= '9'; }
static inline bool isxdigit(char c) {
    c = tolower(c);
    return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f');
}
static inline uint8_t hextou8(char c) {
    c = tolower(c);
    if ('0' <= c && c <= '9')
        return c - '0';
    if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    return 0;
}

void *memset(void *dst, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
char *strchr(const char *str, int ch);
char *strtok_r(char *str, const char *delim, char **save_ptr);
char *strtok(char *str, const char *delim);
uint32_t strtou32(const char *str, const char **endptr, int base);
uint32_t strntou32(const char *str, const char **endptr, int n, int base);

#endif // _STRING_H_
