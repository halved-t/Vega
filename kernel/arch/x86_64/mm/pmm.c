/*
 * Copyright 2026 Halved
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <mm/mm.h>
#include <mm/pmm.h>
#include <klibc/string.h>
#include <stdint.h>
#include <stdbool.h>
#include <limine.h>
#include <sync/spinlock.h>

static uint8_t *bitmap;
static size_t total_pages;
static size_t free_pages;
static size_t bitmap_size;
static spinlock_t pmm_lock = SPINLOCK_INIT;

static void bitmap_set(size_t page) {
    bitmap[page / 8] |= (1 << (page % 8));
}

static void bitmap_clear(size_t page) {
    bitmap[page / 8] &= ~(1 << (page % 8));
}

static bool bitmap_get(size_t page) {
    return bitmap[page / 8] & (1 << (page % 8));
}

void pmm_init(struct limine_memmap_entry **entries, uint64_t entry_count) {
    uint64_t highest = 0;
    for (uint64_t i = 0; i < entry_count; i++) {
        if (entries[i]->type == LIMINE_MEMMAP_USABLE) {
            uint64_t top = entries[i]->base + entries[i]->length;
            if (top > highest) highest = top;
        }
    }

    total_pages = highest / PAGE_SIZE;
    bitmap_size = (total_pages + 7) / 8;

    for (uint64_t i = 0; i < entry_count; i++) {
        if (entries[i]->type == LIMINE_MEMMAP_USABLE && entries[i]->length >= bitmap_size) {
            bitmap = (uint8_t *)(entries[i]->base + MEM_OFFSET);
            break;
        } 
    }

    memset(bitmap, 0xFF, bitmap_size);

    for (uint64_t i = 0; i < entry_count; i++) {
        if (entries[i]->type == LIMINE_MEMMAP_USABLE) {
            uint64_t base = entries[i]->base;
            uint64_t pages = entries[i]->length / PAGE_SIZE;

            for (uint64_t j = 0; j < pages; j++) {
                bitmap_clear((base / PAGE_SIZE) + j);
                free_pages++;
            }
        }
    }

    size_t bitmap_pages = (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint64_t bitmap_page = ((uint64_t)bitmap - MEM_OFFSET) / PAGE_SIZE;
    for(size_t i = 0; i < bitmap_pages; i++) {
        bitmap_set(bitmap_page + i);
        free_pages--;
    }
}

static size_t last_index = 0;

static void *pmm_ialloc(size_t start_idx, size_t end_idx, size_t pages) {
    size_t consecutive = 0;
    size_t start = 0;

    for (size_t i = start_idx; i < end_idx; i++) {
        if (!bitmap_get(i)) {
            if (consecutive == 0) start = i;
            consecutive++;
            if (consecutive == pages) {
                for (size_t j = start; j < start + pages; j++) {
                    bitmap_set(j);
                }
                free_pages -= pages;
                last_index = start + pages;
                return (void *)(start * PAGE_SIZE);
            }
        } else {
            consecutive = 0;
        }
    }

    return NULL;
}

void *pmm_alloc(size_t pages) {
    spinlock_acquire(&pmm_lock);
    void *result = pmm_ialloc(last_index, total_pages, pages);
    if (!result) result = pmm_ialloc(0, last_index, pages);
    spinlock_release(&pmm_lock);
    return result;
}

void *pmm_allocz(size_t pages) {
	void *alloc = pmm_alloc(pages);

	if (alloc != NULL)
		memset((void *)((uint64_t)alloc + MEM_OFFSET), 0, pages * PAGE_SIZE);

	return alloc;
}

void pmm_free(void *addr, size_t pages) {
    spinlock_acquire(&pmm_lock);
    size_t start = (uint64_t)addr / PAGE_SIZE;
    for (size_t i = start; i < start + pages; i++) {
        bitmap_clear(i);
    }
    free_pages += pages;
    spinlock_release(&pmm_lock);
}

size_t pmm_free_pages(void) {
    return free_pages;
}

size_t pmm_total_pages(void) {
    return total_pages;
}