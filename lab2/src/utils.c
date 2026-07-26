#include "utils.h"
#include <stddef.h>
#include <stdint.h>

uint32_t LE_uint32(const uint8_t *ptr) {
    uint32_t result = 0;
    for (int i = 0; i < 4; i++) {
        result |= ((uint32_t)ptr[i] & 0xff) << (8 * i);
    }
    return result;
}

uint32_t BE_uint32(const uint8_t *ptr) {
    uint32_t result = 0;
    for (int i = 0; i < 4; i++) {
        result <<= 8;
        result |= ((uint32_t)ptr[i] & 0xff);
    }
    return result;
}

uint64_t LE_uint64(const uint8_t *ptr) {
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result |= ((uint64_t)ptr[i] & 0xff) << (8 * i);
    }
    return result;
}

uint64_t BE_uint64(const uint8_t *ptr) {
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result <<= 8;
        result |= ((uint64_t)ptr[i] & 0xff);
    }
    return result;
}
