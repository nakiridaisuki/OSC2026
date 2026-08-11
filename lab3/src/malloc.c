#include "malloc.h"
#include "dstruc.h"
#include "printf.h"
#include "string.h"
#include "utils.h"
#include <iso646.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

PAGE pages_arr[PAGE_N];
LINKED_LIST_NODE free_pages[MAX_ORDER];
LINKED_LIST_NODE free_slabs[SLAB_COUNT];

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

void init_palloc() {
    memset(pages_arr, 0, sizeof(pages_arr));
    memset(free_pages, 0, sizeof(free_pages));
    for (size_t i = 0; i < PAGE_N; i++)
        lln_init(&pages_arr[i].list);
    for (size_t i = 0; i < MAX_ORDER; i++)
        lln_init(&free_pages[i]);

    uint8_t max_order = get_buddy_order(MEM_SIZE);

    pages_arr[0].order     = max_order;
    pages_arr[0].allocated = false;

    lln_add(&free_pages[max_order], &pages_arr[0].list);
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
                avail_page - pages_arr,
                avail_page->order,
                avail_page - pages_arr,
                avail_page - pages_arr + (1 << avail_page->order)
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
            right_page - pages_arr,
            right_page->order,
            right_page - pages_arr,
            right_page - pages_arr + (1 << right_page->order)
        );
    }

    avail_page->allocated = true;
    size_t page_idx       = avail_page - pages_arr;
    uint8_t *page_ptr     = (uint8_t *)(MEM_START + PAGE_SIZE * page_idx);

    DBG_PRINTF(
        "[PA] Allocate 0x%lx at order %d, page %d for require size %lu. Next page at order %d in "
        "freelist: 0x%lx\n",
        page_ptr,
        avail_page->order,
        page_idx,
        bytes,
        avail_page->order,
        free_pages[avail_page->order].next == &free_pages[avail_page->order]
            ? NULL
            : (uint8_t *)(MEM_START +
                          PAGE_SIZE * (Node2Page(free_pages[avail_page->order].next) - pages_arr))
    );
    return page_ptr;
}

void pfree(uint8_t *page) {
    size_t page_idx = ((uint64_t)page - MEM_START) / PAGE_SIZE;

    pages_arr[page_idx].allocated = false;
    while (pages_arr[page_idx].order < MAX_ORDER - 1) {
        size_t buddy_page_idx = page_idx ^ (1ULL << pages_arr[page_idx].order);
        if (buddy_page_idx >= PAGE_N)
            break;

        if (pages_arr[buddy_page_idx].allocated ||
            pages_arr[buddy_page_idx].order != pages_arr[page_idx].order)
            break;

        PAGE *buddy_page_ptr = pages_arr + buddy_page_idx;
        lln_remove(&buddy_page_ptr->list);

        DBG_PRINTF(
            "[PM] Merge pages %d and %d at order %d.\n",
            page_idx,
            buddy_page_idx,
            buddy_page_ptr->order
        );

        page_idx = page_idx < buddy_page_idx ? page_idx : buddy_page_idx;
        pages_arr[page_idx].order++;
        pages_arr[page_idx].allocated = false;
    }

    PAGE *page_ptr = pages_arr + page_idx;
    lln_add(&free_pages[page_ptr->order], &page_ptr->list);

    DBG_PRINTF("[PF] Free 0x%p at order %d, page %d.\n", page, page_ptr->order, page_idx);
}

void init_dalloc() {
    memset(free_slabs, 0, sizeof(free_slabs));
    for (size_t i = 0; i < MAX_ORDER; i++)
        lln_init(&free_slabs[i]);
}

uint8_t *dalloc(uint32_t bytes) {
    uint8_t order = get_slab_order(bytes);

    // Allocate a new page for dynamic allocater
    if (free_slabs[order].next == &free_slabs[order]) {
        uint8_t *new_page = palloc(PAGE_SIZE);
        if (new_page == NULL)
            return NULL;

        size_t page_idx = ((uint64_t)new_page - MEM_START) / PAGE_SIZE;
        PAGE *page      = &pages_arr[page_idx];

        page->slab_count = 0;
        page->slab_size  = (MIN_SLAB << order);
        page->slab_head  = new_page;

        // Init block list
        uint32_t total_blocks = PAGE_SIZE / page->slab_size;
        for (int i = 0; i < total_blocks - 1; i++) {
            void **curr_block = (void **)(new_page + i * page->slab_size);
            void *next_block  = (new_page + (i + 1) * page->slab_size);
            *curr_block       = next_block;
        }
        void **last_block = (void **)(new_page + (total_blocks - 1) * page->slab_size);
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
        avail_page - pages_arr,
        bytes
    );

    return slab_ptr;
}

void dfree(uint8_t *slab) {
    uint64_t page_ptr = (uint64_t)slab & ~(PAGE_SIZE - 1);
    size_t page_idx   = (page_ptr - MEM_START) / PAGE_SIZE;
    PAGE *page        = &pages_arr[page_idx];

    if (page->slab_head == NULL) {
        uint8_t order = get_slab_order(page->slab_size);
        lln_add(&free_slabs[order], &page->list);
    }

    *(void **)slab  = page->slab_head;
    page->slab_head = slab;

    page->slab_count--;
    if (pages_arr[page_idx].slab_count == 0) {
        pages_arr[page_idx].slab_size = 0;
        lln_remove(&pages_arr[page_idx].list);
        pfree((uint8_t *)page_ptr);
    }
    DBG_PRINTF("[CF] Free 0x%p at order %d, page %d.\n", slab, pages_arr[page_idx].order, page_idx);
}

uint8_t *malloc(uint64_t bytes) {
    if (bytes <= PAGE_SIZE / 2)
        return dalloc(bytes);
    return palloc(bytes);
}

void free(uint8_t *ptr) {
    uint64_t page_ptr = (uint64_t)ptr & ~(PAGE_SIZE - 1);
    size_t page_idx   = (page_ptr - MEM_START) / PAGE_SIZE;
    if (pages_arr[page_idx].slab_size != 0)
        dfree(ptr);
    else
        pfree(ptr);
}
