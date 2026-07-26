#ifndef _H_UTILS_
#define _H_UTILS_

#include <stdint.h>

uint32_t LE_uint32(const uint8_t *ptr);
uint32_t BE_uint32(const uint8_t *ptr);
uint64_t LE_uint64(const uint8_t *ptr);
uint64_t BE_uint64(const uint8_t *ptr);

#endif // !_H_UTILS_
