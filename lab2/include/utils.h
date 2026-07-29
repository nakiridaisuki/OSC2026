#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdint.h>

#define ALIGN_4(addr) ((__typeof__(addr))(((uintptr_t)(addr) + 3) & ~3))

uint32_t LE_uint32(const uint8_t *ptr);
uint32_t BE_uint32(const uint8_t *ptr);
uint64_t LE_uint64(const uint8_t *ptr);
uint64_t BE_uint64(const uint8_t *ptr);

#endif // !_UTILS_H_
