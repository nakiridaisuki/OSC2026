#ifndef _MALLOC_H_
#define _MALLOC_H_

#include "dstruc.h"
#include <stdint.h>

#define MEM_START 0x100000000
#define MEM_SIZE  0x100000000
#define PAGE_SIZE 4096
#define MIN_SLAB  16

#define PAGE_N     MEM_SIZE / PAGE_SIZE
#define MAX_ORDER  64
#define SLAB_COUNT 8 // 16, 32, 64, 128, 256, 512, 1024, 2048

#define Node2Page(nodeptr) container_of(nodeptr, PAGE, list)

typedef struct _page PAGE;
struct _page {
    uint8_t order;
    union {
        uint8_t allocated;
        uint8_t slab_count;
    };
    uint16_t slab_size;
    void *slab_head;
    LINKED_LIST_NODE list;
};

void init_palloc();
void init_dalloc();
uint8_t *palloc(uint64_t bytes);
void pfree(uint8_t *page);
uint8_t *dalloc(uint32_t bytes);
void dfree(uint8_t *page);
uint8_t *malloc(uint64_t bytes);
void free(uint8_t *page);

#endif // !_MALLOC_H_
