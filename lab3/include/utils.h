#ifndef _UTILS_H_
#define _UTILS_H_

#include <stddef.h>
#include <stdint.h>

#define ALIGN_UP_4(addr)      ((__typeof__(addr))(((uintptr_t)(addr) + 3) & ~3))
#define ALIGN_UP_8(addr)      ((__typeof__(addr))(((uintptr_t)(addr) + 7) & ~7))
#define ALIGN_UP(addr, len)   ((__typeof__(addr))(((uintptr_t)(addr) + len - 1) & ~(len - 1)))
#define ALIGN_DOWN(addr, len) ((__typeof__(addr))(((uintptr_t)(addr)) & ~(len - 1)))

#define LOWBIT(x)  (x & -x)
#define SWAP(a, b) a ^= b ^= a ^= b

#ifdef DEBUG
#define DBG_PRINTF(fmt, ...) printf("[DEBUG %s] " fmt, __func__, ##__VA_ARGS__)
#else
#define DBG_PRINTF(...)          \
    do {                         \
        if (0) {                 \
            printf(__VA_ARGS__); \
        }                        \
    } while (0)
#endif // DEBUG

uint32_t LE_uint32(const uint8_t *ptr);
uint32_t BE_uint32(const uint8_t *ptr);
uint64_t LE_uint64(const uint8_t *ptr);
uint64_t BE_uint64(const uint8_t *ptr);

static inline void write_BE32(uint8_t *dest, uint32_t val) {
    dest[0] = (val >> 24) & 0xff;
    dest[1] = (val >> 16) & 0xff;
    dest[2] = (val >> 8) & 0xff;
    dest[3] = val & 0xff;
}
static inline void write_BE64(uint8_t *dest, uint64_t val) {
    dest[0] = (val >> 56) & 0xff;
    dest[1] = (val >> 48) & 0xff;
    dest[2] = (val >> 40) & 0xff;
    dest[3] = (val >> 32) & 0xff;
    dest[4] = (val >> 24) & 0xff;
    dest[5] = (val >> 16) & 0xff;
    dest[6] = (val >> 8) & 0xff;
    dest[7] = val & 0xff;
}

static inline uint8_t __ffs64(uint64_t word) { return __builtin_ctzll(word); }
static inline uint8_t __fls64(uint64_t word) { return 63 - __builtin_clzll(word); }

#endif // !_UTILS_H_
