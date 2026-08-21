#ifndef _MALLOC_H_
#define _MALLOC_H_

#include "dstruc.h"
#include "types.h"
#include <stdint.h>

typedef struct _page Page;
struct _page {
    int8_t order;
    uint8_t zone;
    uint8_t allocated;
    uint16_t slab_count;
    uint16_t slab_size;
    void *slab_head;
    LinkedListNode list;
};

typedef struct {
    phys_addr_t mem_start;
    phys_addr_t mem_size;
    Page *pages_arr;
    uint32_t arr_size;
} MemZone;

void init_malloc(const uint8_t *fdt_ptr);
uint8_t *malloc(uint64_t bytes);
void free(void *mem_ptr);

void init_palloc();
uint8_t *palloc(uint64_t bytes);
void pfree(Page *page);

void init_dalloc();
uint8_t *dalloc(uint32_t bytes);

#endif // !_MALLOC_H_
