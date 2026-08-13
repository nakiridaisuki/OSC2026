#include "malloc.h"
#include "cpio.h"
#include "dstruc.h"
#include "dtb.h"
#include "printf.h"
#include "string.h"
#include "types.h"
#include "utils.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

uint8_t TOTAL_ZONES;
MemZone zones[64];
LINKED_LIST_NODE free_pages[MAX_ORDER];
LINKED_LIST_NODE free_slabs[SLAB_COUNT];

extern uint8_t _stack_top[];
extern uint8_t _start[];
phys_addr_t early_mem_ptr = (phys_addr_t)_stack_top;

#define Node2Page(nodeptr) container_of(nodeptr, PAGE, list)

#define PageStart(page_ptr) (zones[page_ptr->zone].pages_arr)
#define PageSize(page_ptr)  (zones[page_ptr->zone].arr_size)
#define PageIdx(page_ptr)   (page_ptr - PageStart(page_ptr))
#define MemStart(page_ptr)  (zones[page_ptr->zone].mem_start)
#define MemSize(page_ptr)   (zones[page_ptr->zone].mem_size)

void *_early_alloc(uint32_t size) {
    void *mem_ptr = (void *)early_mem_ptr;
    early_mem_ptr += size;
    return mem_ptr;
}

uint8_t get_buddy_order(uint64_t size) {
    uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    if (pages <= 1)
        return 0;
    return 64 - __builtin_clzll(pages - 1);
}

uint8_t get_slab_order(uint32_t size) {
    uint32_t slabs = (size + MIN_SLAB - 1) / MIN_SLAB;
    if (slabs <= 1)
        return 0;
    return 32 - __builtin_clz(slabs - 1);
}

PAGE *addr2page(uint8_t *addr) {
    size_t zid = 0;
    while ((phys_addr_t)addr > zones[zid].mem_start + zones[zid].mem_size)
        zid++;
    uint64_t offset = (phys_addr_t)addr - zones[zid].mem_start;
    return &zones[zid].pages_arr[offset / PAGE_SIZE];
}

void lock_mem_region(phys_addr_t start_addr, phys_addr_t size) {
    phys_addr_t end_addr = start_addr + size;
    start_addr           = start_addr & ~(PAGE_SIZE - 1);
    while (start_addr < end_addr) {
        PAGE *page      = addr2page((uint8_t *)(start_addr));
        page->allocated = true;
        start_addr += PAGE_SIZE;
    }
}

void fdt_rsvmem_cb(const uint8_t *node_ptr, const char *node_name, void *dt_string_ptr) {
    FDTProp mem_reg = fdt_find_prop(node_ptr, dt_string_ptr, "reg");
    if (mem_reg.val_ptr == NULL)
        return;
    phys_addr_t start_addr = BE_uint64(mem_reg.val_ptr);
    phys_addr_t size       = BE_uint64(mem_reg.val_ptr + 8);
    lock_mem_region(start_addr, size);
}

void fdt_mem_cb(const uint8_t *node_ptr, const char *node_name, void *dt_string_ptr) {
    if (fdt_node_name_eq(node_name, "memory", 6)) {
        FDTProp mem_reg = fdt_find_prop(node_ptr, dt_string_ptr, "reg");
        if (mem_reg.val_ptr == NULL)
            return;
        zones[TOTAL_ZONES].mem_start = BE_uint64(mem_reg.val_ptr);
        zones[TOTAL_ZONES].mem_size  = BE_uint64(mem_reg.val_ptr + 8);
        zones[TOTAL_ZONES].arr_size  = zones[TOTAL_ZONES].mem_size / PAGE_SIZE;
        zones[TOTAL_ZONES].pages_arr = _early_alloc(zones[TOTAL_ZONES].arr_size * sizeof(PAGE));
        TOTAL_ZONES++;
    }
}

void init_malloc(uint8_t *fdt_ptr) {
    FDTHeader fdt_header          = get_fdt_header(fdt_ptr);
    const uint8_t *dt_struct_ptr  = fdt_ptr + fdt_header.off_dt_struct;
    const uint8_t *dt_strings_ptr = fdt_ptr + fdt_header.off_dt_strings;

    TOTAL_ZONES = 0;
    fdt_foreach_subnode(dt_struct_ptr, fdt_mem_cb, (void *)dt_strings_ptr);

    init_palloc();
    init_dalloc();

    const uint8_t *reserved_mem = fdt_find_node_by_path(dt_struct_ptr, "/reserved-memory");
    fdt_foreach_subnode(reserved_mem, fdt_rsvmem_cb, (void *)dt_strings_ptr);

    // Align early memory allocater ptr to 4kB
    early_mem_ptr = (early_mem_ptr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    lock_mem_region((phys_addr_t)_start, early_mem_ptr - (phys_addr_t)_start);
    lock_mem_region((phys_addr_t)fdt_ptr, fdt_header.totalsize);
    lock_mem_region(CPIO_START_ADDR, CPIO_END_ADDR - CPIO_START_ADDR);

    for (size_t i = 0; i < TOTAL_ZONES; i++) {
        for (size_t j = 0; j < zones[i].arr_size; j++) {
            PAGE *page = &zones[i].pages_arr[j];
            if (!page->allocated && page->order >= 0) {
                pfree(page);
            }
        }
    }
}

uint8_t *malloc(uint64_t bytes) {
    if (bytes <= PAGE_SIZE / 2)
        return dalloc(bytes);
    return palloc(bytes);
}

void free(uint8_t *ptr) {
    uint64_t mem_ptr = (uint64_t)ptr & ~(PAGE_SIZE - 1);
    PAGE *page       = addr2page((uint8_t *)mem_ptr);
    if (page->slab_size != 0) {

        if (page->slab_head == NULL) {
            uint8_t order = get_slab_order(page->slab_size);
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
        DBG_PRINTF("[CF] Free 0x%p at order %d, page %d.\n", ptr, page->order, PageIdx(page));
    } else
        pfree(page);
}

void init_palloc() {
    for (size_t i = 0; i < MAX_ORDER; i++)
        lln_init(&free_pages[i]);

    for (size_t zid = 0; zid < TOTAL_ZONES; zid++) {
        PAGE *pages      = zones[zid].pages_arr;
        uint32_t arrsize = zones[zid].arr_size;
        memset(pages, 0, sizeof(PAGE) * arrsize);
        for (size_t j = 0; j < arrsize; j++) {
            lln_init(&pages[j].list);
            pages[j].order     = 0;
            pages[j].zone      = zid;
            pages[j].allocated = false;
        }
    }
}

uint8_t *palloc(uint64_t bytes) {
    uint8_t order    = get_buddy_order(bytes);
    PAGE *avail_page = NULL;
    for (size_t i = order; i < MAX_ORDER; i++) {
        if (free_pages[i].next != &free_pages[i]) {
            avail_page = Node2Page(free_pages[i].next);
            lln_remove(free_pages[i].next);

            DBG_PRINTF(
                "[-] Remove page %d from order %d. Range of pages: [%d, %d]\n",
                avail_page - PageStart(avail_page),
                avail_page->order,
                avail_page - PageStart(avail_page),
                avail_page - PageStart(avail_page) + (1 << avail_page->order)
            );
            break;
        }
    }
    if (avail_page == NULL)
        return NULL;

    while (avail_page->order > order) {
        avail_page->order -= 1;

        PAGE *right_page      = avail_page + (1ULL << avail_page->order);
        right_page->order     = avail_page->order;
        right_page->allocated = false;

        lln_add(&free_pages[avail_page->order], &right_page->list);

        DBG_PRINTF(
            "[+] Add page %d to order %d. Range of pages: [%d, %d]\n",
            right_page - PageStart(right_page),
            right_page->order,
            right_page - PageStart(right_page),
            right_page - PageStart(right_page) + (1 << right_page->order)
        );
    }

    avail_page->allocated = true;
    size_t page_idx       = PageIdx(avail_page);
    uint8_t *mem_ptr      = (uint8_t *)(MemStart(avail_page) + PAGE_SIZE * page_idx);

    DBG_PRINTF(
        "[PA] Allocate 0x%lx at order %d, page %d for require size %lu. Next page at order %d in "
        "freelist: 0x%lx\n",
        mem_ptr,
        avail_page->order,
        page_idx,
        bytes,
        avail_page->order,
        free_pages[avail_page->order].next == &free_pages[avail_page->order]
            ? NULL
            : (uint8_t *)(MemStart(avail_page) +
                          PAGE_SIZE * (Node2Page(free_pages[avail_page->order].next) -
                                       PageStart(avail_page)))
    );
    return mem_ptr;
}

void pfree(PAGE *page) {
    if (page->order < 0)
        return;

    page->allocated = false;
    while (page->order < MAX_ORDER - 1) {
        size_t page_idx       = PageIdx(page);
        size_t buddy_page_idx = page_idx ^ (1ULL << page->order);
        if (buddy_page_idx >= PageSize(page))
            break;

        PAGE *buddy_page = &PageStart(page)[buddy_page_idx];
        if (buddy_page->allocated || buddy_page->order != page->order)
            break;
        lln_remove(&buddy_page->list);

        DBG_PRINTF(
            "[PM] Merge pages %d and %d at order %d.\n", page_idx, buddy_page_idx, buddy_page->order
        );

        if (page_idx > buddy_page_idx) {
            PAGE *tmp  = page;
            page       = buddy_page;
            buddy_page = tmp;
        }
        buddy_page->order = -1;
        page->order++;
        page->allocated = false;
    }
    lln_add(&free_pages[page->order], &page->list);
    DBG_PRINTF("[PF] Free 0x%p at order %d, page %d.\n", page, page->order, PageIdx(page));
}

void init_dalloc() {
    for (size_t i = 0; i < MAX_ORDER; i++)
        lln_init(&free_slabs[i]);
}

uint8_t *dalloc(uint32_t bytes) {
    uint8_t order = get_slab_order(bytes);

    // Allocate a new page for dynamic allocater
    if (free_slabs[order].next == &free_slabs[order]) {
        uint8_t *new_mem_ptr = palloc(PAGE_SIZE);
        if (new_mem_ptr == NULL)
            return NULL;

        PAGE *page = addr2page(new_mem_ptr);

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
    PAGE *avail_page = Node2Page(free_slabs[order].next);

    uint8_t *slab_ptr     = avail_page->slab_head;
    avail_page->slab_head = *(void **)slab_ptr;
    avail_page->slab_count++;

    if (avail_page->slab_head == NULL)
        lln_remove(&avail_page->list);

    DBG_PRINTF(
        "[CA] Allocate 0x%lx at order %d, page %d for require size %lu.\n",
        slab_ptr,
        avail_page->order,
        avail_page - PageStart(avail_page),
        bytes
    );

    return slab_ptr;
}
