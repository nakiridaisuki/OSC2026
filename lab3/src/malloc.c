#include "malloc.h"
#include "printf.h"
#include "string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

PAGE PAGES[PAGE_N];
PAGE *FREE_LIST[MAX_ORDER];

uint8_t get_buddy_order(uint64_t size) {
    uint64_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    if (pages <= 1)
        return 0;
    return 64 - __builtin_clzll(pages - 1);
}

void init_palloc() {
    memset(PAGES, 0, sizeof(PAGES));
    for (size_t i = 0; i < MAX_ORDER; i++)
        FREE_LIST[i] = NULL;

    uint8_t max_order = get_buddy_order(MEM_SIZE);

    PAGES[0].order   = max_order;
    PAGES[0].is_free = true;

    FREE_LIST[max_order] = PAGES;
}

uint8_t *palloc(uint64_t bytes) {
    uint8_t order    = get_buddy_order(bytes);
    PAGE *avail_page = NULL;
    for (size_t i = order; i < MAX_ORDER; i++) {
        if (FREE_LIST[i] != NULL) {
            avail_page = FREE_LIST[i];

            // pop current available page from free list
            FREE_LIST[avail_page->order] = avail_page->prev;
            if (FREE_LIST[avail_page->order] != NULL)
                FREE_LIST[avail_page->order]->next = NULL;
            avail_page->next = avail_page->prev = NULL;

            break;
        }
    }
    if (avail_page == NULL)
        return NULL;

    while (avail_page->order > order) {
        avail_page->order -= 1;

        PAGE *right_page    = avail_page + (1 << avail_page->order);
        right_page->order   = avail_page->order;
        right_page->is_free = true;

        // append new available page into freelist
        right_page->prev = FREE_LIST[avail_page->order];
        if (FREE_LIST[avail_page->order] != NULL)
            FREE_LIST[avail_page->order]->next = right_page;
        FREE_LIST[avail_page->order] = right_page;

        printf(
            "Split %lu B memory at index %d\n",
            (1ULL << avail_page->order) * PAGE_SIZE,
            right_page - PAGES
        );
    }

    avail_page->is_free = false;
    size_t page_idx     = avail_page - PAGES;
    uint8_t *page_ptr   = (uint8_t *)(MEM_START + PAGE_SIZE * page_idx);
    return page_ptr;
}

void pfree(uint8_t *page) {
    size_t page_idx = ((uint64_t)page - MEM_START) / PAGE_SIZE;

    PAGES[page_idx].is_free = true;
    while (PAGES[page_idx].order < MAX_ORDER - 1) {
        size_t buddy_page_idx = page_idx ^ (1ULL << PAGES[page_idx].order);

        if (!PAGES[buddy_page_idx].is_free || PAGES[buddy_page_idx].order != PAGES[page_idx].order)
            break;

        PAGE *buddy_page_ptr = PAGES + buddy_page_idx;
        // delete buddy page from free list
        if (buddy_page_ptr->prev != NULL)
            buddy_page_ptr->prev->next = buddy_page_ptr->next;
        if (buddy_page_ptr->next != NULL)
            buddy_page_ptr->next->prev = buddy_page_ptr->prev;
        if (FREE_LIST[buddy_page_ptr->order] == buddy_page_ptr)
            FREE_LIST[buddy_page_ptr->order] = buddy_page_ptr->prev;
        buddy_page_ptr->next = buddy_page_ptr->prev = NULL;

        page_idx = page_idx < buddy_page_idx ? page_idx : buddy_page_idx;
        PAGES[page_idx].order++;
        PAGES[page_idx].is_free = true;

        printf(
            "Merge %lu B memory at index %d\n",
            (1ULL << PAGES[page_idx].order) * PAGE_SIZE,
            page_idx
        );
    }

    PAGE *page_ptr = PAGES + page_idx;
    // append new available page into freelist
    page_ptr->prev = FREE_LIST[page_ptr->order];
    if (FREE_LIST[page_ptr->order] != NULL)
        FREE_LIST[page_ptr->order]->next = page_ptr;
    FREE_LIST[page_ptr->order] = page_ptr;
}
