#include "string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}
int strncmp(const char *s1, const char *s2, size_t n) {
    for (int i = 0; i < n; i++) {
        if (s1[i] != s2[i])
            return (unsigned char)s1[i] - (unsigned char)s2[i];
    }
    return 0;
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
    const char *p = s;
    while (*p)
        p++;
    return p - s;
}

char *strcpy(char *dst, const char *src) {
    char *saved_dst = dst;
    while ((*dst++ = *src++) != '\0')
        ;
    return saved_dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    while (i < n && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    while (i < n)
        dst[i++] = 0;
    return dst;
}

char *strchr(const char *str, int ch) {
    while (*str) {
        if (*str == ch)
            return (char *)str;
        str++;
    }
    if (ch == 0)
        return (char *)str;
    return NULL;
}

char *strtok_r(char *str, const char *delim, char **save_ptr) {
    if (str == NULL) {
        str = *save_ptr;
    }
    if (str == NULL)
        return NULL;

    while (*str != '\0') {
        size_t i = 0;
        while (delim[i] && *str != delim[i])
            i++;
        if (delim[i] == '\0')
            break;
        str++;
    }

    if (*str == '\0') {
        *save_ptr = NULL;
        return NULL;
    }

    char *token = str;
    while (*str != '\0') {
        size_t i = 0;
        while (delim[i] && *str != delim[i])
            i++;
        if (delim[i] != '\0') {
            *str      = '\0';
            *save_ptr = str + 1;
            return token;
        }
        str++;
    }
    *save_ptr = str;
    return token;
}

char *strtok(char *str, const char *delim) {
    static char *olds;
    return strtok_r(str, delim, &olds);
}

uint32_t strtou32(const char *str, const char **endptr, int base) {
    uint32_t result = 0;

    bool (*avail_digit)(char c);
    if (base == 10)
        avail_digit = isdigit;
    else if (base == 16)
        avail_digit = isxdigit;
    else
        return 0;

    while (avail_digit(*str)) {
        result = result * base + hextou8(*str);
        str++;
    }
    if (endptr != NULL)
        *endptr = str;
    return result;
}
uint32_t strntou32(const char *str, const char **endptr, int n, int base) {
    uint32_t result = 0;

    bool (*avail_digit)(char c);
    if (base == 10)
        avail_digit = isdigit;
    else if (base == 16)
        avail_digit = isxdigit;
    else
        return 0;

    while (n-- && avail_digit(*str)) {
        result = result * base + hextou8(*str);
        str++;
    }
    if (endptr != NULL)
        *endptr = str;
    return result;
}
