#ifndef _MALLOC_H_
#define _MALLOC_H_

#include <stdint.h>

#define MEM_START 0x100000000
#define MEM_SIZE  0x100000000
#define PAGE_SIZE 4096

#define PAGE_N    MEM_SIZE / PAGE_SIZE
#define MAX_ORDER 64

typedef struct _page PAGE;
struct _page {
    uint8_t order;
    uint8_t is_free;
    PAGE *next;
    PAGE *prev;
};

void init_palloc();
uint8_t *palloc(uint64_t bytes);
void pfree(uint8_t *page);

#endif // !_MALLOC_H_
