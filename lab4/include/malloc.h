#ifndef _MALLOC_H_
#define _MALLOC_H_

#include <stdint.h>

void init_malloc(const uint8_t *fdt_ptr);
void *malloc(uint64_t bytes);
void free(void *mem_ptr);

#endif // !_MALLOC_H_
