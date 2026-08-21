#include "malloc.h"
#include "cpio.h"
#include "dstruc.h"
#include "dtb.h"
#include "string.h"
#include "types.h"
#include "utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE  4096
#define MIN_SLAB   16
#define MAX_ORDER  32
#define SLAB_COUNT 8 // 16, 32, 64, 128, 256, 512, 1024, 2048

extern uint8_t _stack_top[];
extern uint8_t _start[];
phys_addr_t early_mem_ptr = (phys_addr_t)_stack_top;

bool __malloc_init_done;
#define ALLOC_LOG(...)               \
    do {                             \
        if (__malloc_init_done) {    \
            DBG_PRINTF(__VA_ARGS__); \
        }                            \
    } while (0)

uint8_t total_zones;
MemZone zones[8];
LinkedListNode free_pages[MAX_ORDER];
LinkedListNode free_slabs[SLAB_COUNT];

static inline Page *_node2page(LinkedListNode *node_ptr) {
    return container_of(node_ptr, Page, list);
}
static Page *_mem2page(uint8_t *mem_ptr) {
    phys_addr_t addr = (phys_addr_t)mem_ptr;
    for (size_t zid = 0; zid < total_zones; zid++) {
        if (zones[zid].mem_start <= addr && addr < zones[zid].mem_start + zones[zid].mem_size) {
            uint64_t offset = (phys_addr_t)mem_ptr - zones[zid].mem_start;
            return &zones[zid].pages_arr[offset / PAGE_SIZE];
        }
    }
    return NULL;
}
static inline Page *_pagearr_start(Page *page_ptr) { return (zones[page_ptr->zone].pages_arr); }
static inline uint32_t _pagearr_size(Page *page_ptr) { return (zones[page_ptr->zone].arr_size); }
static inline size_t _pageidx(Page *page_ptr) { return (page_ptr - _pagearr_start(page_ptr)); }
static inline phys_addr_t _mem_start(Page *page_ptr) { return (zones[page_ptr->zone].mem_start); }

static void *_early_alloc(uint32_t size) {
    early_mem_ptr = ALIGN_UP_8(early_mem_ptr);
    void *mem_ptr = (void *)early_mem_ptr;
    early_mem_ptr += size;
    return mem_ptr;
}

static uint8_t _buddy_order(uint64_t size) {
    uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    if (pages <= 1)
        return 0;
    return 64 - __builtin_clzll(pages - 1);
}

static uint8_t _slab_order(uint32_t size) {
    uint32_t slabs = (size + MIN_SLAB - 1) / MIN_SLAB;
    if (slabs <= 1)
        return 0;
    return 32 - __builtin_clz(slabs - 1);
}

static void _lock_mem(phys_addr_t start_addr, phys_addr_t size) {
    phys_addr_t end_addr = start_addr + size;
    start_addr           = start_addr & ~(PAGE_SIZE - 1);
    DBG_PRINTF("[R] Reserve address [0x%lx, 0x%lx).\n", start_addr, end_addr);
    while (start_addr < end_addr) {
        Page *page = _mem2page((uint8_t *)(start_addr));
        start_addr += PAGE_SIZE;
        if (page == NULL)
            continue;
        page->allocated = true;
    }
}

static void _cb_rsvmem(const uint8_t *node_ptr, const char *node_name, void *dt_string_ptr) {
    FDTProp mem_reg = fdt_find_prop(node_ptr, dt_string_ptr, "reg");
    if (mem_reg.val_ptr == NULL)
        return;
    phys_addr_t start_addr = BE_uint64(mem_reg.val_ptr);
    phys_addr_t size       = BE_uint64(mem_reg.val_ptr + 8);
    _lock_mem(start_addr, size);
}

static void _cb_mem(const uint8_t *node_ptr, const char *node_name, void *dt_string_ptr) {
    if (fdt_node_name_eq(node_name, "memory", 6)) {
        FDTProp mem_reg = fdt_find_prop(node_ptr, dt_string_ptr, "reg");
        if (mem_reg.val_ptr == NULL)
            return;

        phys_addr_t raw_start = BE_uint64(mem_reg.val_ptr);
        phys_addr_t raw_size  = BE_uint64(mem_reg.val_ptr + 8);

        phys_addr_t aligned_start = ALIGN_UP(raw_start, PAGE_SIZE);
        phys_addr_t aligned_end   = ALIGN_DOWN(raw_start + raw_size, PAGE_SIZE);

        if (aligned_start >= aligned_end)
            return;

        zones[total_zones].mem_start = aligned_start;
        zones[total_zones].mem_size  = aligned_end - aligned_start;
        zones[total_zones].arr_size  = zones[total_zones].mem_size / PAGE_SIZE;
        zones[total_zones].pages_arr = _early_alloc(zones[total_zones].arr_size * sizeof(Page));
        total_zones++;
    }
}

void init_malloc(const uint8_t *fdt_ptr) {
    __malloc_init_done = false;

    FDTHeader fdt_header          = get_fdt_header(fdt_ptr);
    const uint8_t *dt_struct_ptr  = fdt_ptr + fdt_header.off_dt_struct;
    const uint8_t *dt_strings_ptr = fdt_ptr + fdt_header.off_dt_strings;

    total_zones = 0;
    fdt_foreach_subnode(dt_struct_ptr, _cb_mem, (void *)dt_strings_ptr);

    init_palloc();
    init_dalloc();

    const uint8_t *reserved_mem = fdt_find_node_by_path(dt_struct_ptr, "/reserved-memory");
    fdt_foreach_subnode(reserved_mem, _cb_rsvmem, (void *)dt_strings_ptr);

    early_mem_ptr = ALIGN_UP(early_mem_ptr, PAGE_SIZE);
    _lock_mem((phys_addr_t)_start, early_mem_ptr - (phys_addr_t)_start);
    _lock_mem((phys_addr_t)fdt_ptr, fdt_header.totalsize);
    _lock_mem(CPIO_START_ADDR, CPIO_END_ADDR - CPIO_START_ADDR);

    for (size_t i = 0; i < total_zones; i++) {
        for (size_t j = 0; j < zones[i].arr_size; j++) {
            Page *page = &zones[i].pages_arr[j];
            if (!page->allocated && page->order >= 0) {
                pfree(page);
            }
        }
    }
    __malloc_init_done = true;
}

uint8_t *malloc(uint64_t bytes) {
    if (bytes <= PAGE_SIZE / 2)
        return dalloc(bytes);
    return palloc(bytes);
}

void free(void *ptr) {
    phys_addr_t mem_ptr = ALIGN_DOWN((phys_addr_t)ptr, PAGE_SIZE);
    Page *page          = _mem2page((uint8_t *)mem_ptr);
    if (page == NULL)
        return;
    if (page->slab_size != 0) {
        ALLOC_LOG("[CF] Free 0x%lx at order %d, page %d.\n", ptr, page->order, _pageidx(page));
        if (page->slab_head == NULL) {
            uint8_t order = _slab_order(page->slab_size);
            lln_add(&free_slabs[order], &page->list);
        }

        *(void **)ptr   = page->slab_head;
        page->slab_head = ptr;

        page->slab_count--;
        if (page->slab_count == 0) {
            page->slab_size = 0;
            lln_remove(&page->list);
            pfree(page);
        }
    } else
        pfree(page);
}

void init_palloc() {
    for (size_t i = 0; i < MAX_ORDER; i++)
        lln_init(&free_pages[i]);

    for (size_t zid = 0; zid < total_zones; zid++) {
        Page *pages      = zones[zid].pages_arr;
        uint32_t arrsize = zones[zid].arr_size;
        memset(pages, 0, sizeof(Page) * arrsize);
        for (size_t j = 0; j < arrsize; j++) {
            lln_init(&pages[j].list);
            pages[j].order     = 0;
            pages[j].zone      = zid;
            pages[j].allocated = false;
        }
    }
}

uint8_t *palloc(uint64_t bytes) {
    uint8_t order    = _buddy_order(bytes);
    Page *avail_page = NULL;
    for (size_t i = order; i < MAX_ORDER; i++) {
        if (free_pages[i].next != &free_pages[i]) {
            avail_page = _node2page(free_pages[i].next);
            lln_remove(free_pages[i].next);

            ALLOC_LOG(
                "[-] Remove page %d from order %d. Range of pages: [%d, %d)\n",
                avail_page - _pagearr_start(avail_page),
                avail_page->order,
                avail_page - _pagearr_start(avail_page),
                avail_page - _pagearr_start(avail_page) + (1 << avail_page->order)
            );
            break;
        }
    }
    if (avail_page == NULL)
        return NULL;

    while (avail_page->order > order) {
        avail_page->order -= 1;

        Page *right_page      = avail_page + (1ULL << avail_page->order);
        right_page->order     = avail_page->order;
        right_page->allocated = false;

        lln_add(&free_pages[avail_page->order], &right_page->list);

        ALLOC_LOG(
            "[+] Add page %d to order %d. Range of pages: [%d, %d)\n",
            right_page - _pagearr_start(right_page),
            right_page->order,
            right_page - _pagearr_start(right_page),
            right_page - _pagearr_start(right_page) + (1 << right_page->order)
        );
    }

    avail_page->allocated = true;
    size_t page_idx       = _pageidx(avail_page);
    uint8_t *mem_ptr      = (uint8_t *)(_mem_start(avail_page) + PAGE_SIZE * page_idx);

    ALLOC_LOG(
        "[PA] Allocate 0x%lx at order %d, page %d for require size %lu. Next page at order %d in "
        "freelist: 0x%lx\n",
        mem_ptr,
        avail_page->order,
        page_idx,
        bytes,
        avail_page->order,
        free_pages[avail_page->order].next == &free_pages[avail_page->order]
            ? NULL
            : (uint8_t *)(_mem_start(avail_page) +
                          PAGE_SIZE * (_node2page(free_pages[avail_page->order].next) -
                                       _pagearr_start(avail_page)))
    );
    return mem_ptr;
}

void pfree(Page *page) {
    if (page->order < 0)
        return;

    page->allocated = false;
    while (page->order < MAX_ORDER - 1) {
        size_t page_idx = _pageidx(page);

        phys_addr_t page_phys_addr = _mem_start(page) + (page_idx * PAGE_SIZE);
        size_t pfn                 = page_phys_addr / PAGE_SIZE;
        size_t buddy_pfn           = pfn ^ (1ULL << page->order);
        size_t start_pfn           = _mem_start(page) / PAGE_SIZE;
        size_t buddy_page_idx      = buddy_pfn - start_pfn;

        if (buddy_page_idx >= _pagearr_size(page))
            break;

        Page *buddy_page = &_pagearr_start(page)[buddy_page_idx];
        if (buddy_page->allocated || buddy_page->order != page->order)
            break;
        lln_remove(&buddy_page->list);

        ALLOC_LOG(
            "[PM] Merge pages %d and %d at order %d.\n", page_idx, buddy_page_idx, buddy_page->order
        );

        if (page_idx > buddy_page_idx) {
            Page *tmp  = page;
            page       = buddy_page;
            buddy_page = tmp;
        }
        buddy_page->order = -1;
        page->order++;
        page->allocated = false;
    }
    lln_add(&free_pages[page->order], &page->list);
    ALLOC_LOG("[PF] Free 0x%lx at order %d, page %d.\n", page, page->order, _pageidx(page));
}

void init_dalloc() {
    for (size_t i = 0; i < SLAB_COUNT; i++)
        lln_init(&free_slabs[i]);
}

uint8_t *dalloc(uint32_t bytes) {
    uint8_t order = _slab_order(bytes);

    // Allocate a new page for dynamic allocater
    if (free_slabs[order].next == &free_slabs[order]) {
        uint8_t *new_mem_ptr = palloc(PAGE_SIZE);
        if (new_mem_ptr == NULL)
            return NULL;

        Page *page = _mem2page(new_mem_ptr);

        page->slab_count = 0;
        page->slab_size  = (MIN_SLAB << order);
        page->slab_head  = new_mem_ptr;

        // Init block list
        uint32_t total_blocks = PAGE_SIZE / page->slab_size;
        for (int i = 0; i < total_blocks - 1; i++) {
            void **curr_block = (void **)(new_mem_ptr + i * page->slab_size);
            void *next_block  = (new_mem_ptr + (i + 1) * page->slab_size);
            *curr_block       = next_block;
        }
        void **last_block = (void **)(new_mem_ptr + (total_blocks - 1) * page->slab_size);
        *last_block       = NULL;

        lln_add(&free_slabs[order], &page->list);
    }
    Page *avail_page = _node2page(free_slabs[order].next);

    uint8_t *slab_ptr     = avail_page->slab_head;
    avail_page->slab_head = *(void **)slab_ptr;
    avail_page->slab_count++;

    if (avail_page->slab_head == NULL)
        lln_remove(&avail_page->list);

    ALLOC_LOG(
        "[CA] Allocate 0x%lx at order %d, page %d for require size %lu.\n",
        slab_ptr,
        avail_page->order,
        avail_page - _pagearr_start(avail_page),
        bytes
    );

    return slab_ptr;
}
