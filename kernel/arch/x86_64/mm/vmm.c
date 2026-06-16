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
#include <mm/vmm.h>
#include <mm/slab.h>
#include <klibc/string.h>
#include <klibc/misc.h>
#include <stdbool.h>
#include <limine.h>

__attribute__((used, section(".limine_requests")))
static volatile struct limine_paging_mode_request paging_mode_request = {
    .id = LIMINE_PAGING_MODE_REQUEST_ID,
    .revision = 0,
    .mode = LIMINE_PAGING_MODE_X86_64_4LVL
};

struct pagemap *kernel_pagemap = NULL;

#define PML4_IDX(v) (((v) >> 39) & 0x1FF)
#define PDPT_IDX(v) (((v) >> 30) & 0x1FF)
#define PD_IDX(v) (((v) >> 21) & 0x1FF)
#define PT_IDX(v) (((v) >> 12) & 0x1FF)
#define PAGE_ADDR_MASK 0x000FFFFFFFFFF000ull

extern char limine_requests_start_addr[], limine_requests_end_addr[];
extern char text_start_addr[], text_end_addr[];
extern char rodata_start_addr[], rodata_end_addr[];
extern char data_start_addr[], data_end_addr[];

static uint64_t *get_next_level(uint64_t *table, size_t idx, bool alloc) {
    if (table[idx] & VMM_PRESENT) {
        return (uint64_t *)((table[idx] & PAGE_ADDR_MASK) + MEM_OFFSET);
    }

    if (!alloc) return NULL;

    void *next = pmm_allocz(1);
    if (next == NULL) return NULL;

    table[idx] = (uint64_t)next | VMM_USER_RW;

    return (uint64_t *)((uintptr_t)next + MEM_OFFSET);
}

uint64_t *vmm_virt_to_pte(struct pagemap *pagemap, uint64_t virt, bool alloc) {
    size_t pml4_idx = (virt >> 39) & 0x1FF;
    size_t pdpt_idx = (virt >> 30) & 0x1FF;
    size_t pd_idx   = (virt >> 21) & 0x1FF;
    size_t pt_idx   = (virt >> 12) & 0x1FF;

    uint64_t *pml4 = (uint64_t *)((uint64_t)pagemap->pml4 + MEM_OFFSET);

    uint64_t *pdpt = get_next_level(pml4, pml4_idx, alloc);
    if (!pdpt) return NULL;
    uint64_t *pd = get_next_level(pdpt, pdpt_idx, alloc);
    if (!pd) return NULL;
    uint64_t *pt = get_next_level(pd, pd_idx, alloc);
    if (!pt) return NULL;

    return &pt[pt_idx];
}

uint64_t vmm_virt_to_phys(struct pagemap *pagemap, uint64_t virt) {
    uint64_t *pte = vmm_virt_to_pte(pagemap, virt, false);
    if (!pte || !(*pte & VMM_PRESENT)) return INVALID_PHYS;
    return *pte & PAGE_ADDR_MASK;
}

void vmm_map(struct pagemap *pagemap, uint64_t virt, uint64_t phys, uint64_t flags) {
    spinlock_acquire(&pagemap->lock);
    uint64_t *pte = vmm_virt_to_pte(pagemap, virt, true);
    if (pte) *pte = phys | flags;
    spinlock_release(&pagemap->lock);
}

void vmm_unmap(struct pagemap *pagemap, uint64_t virt) {
    spinlock_acquire(&pagemap->lock);
    uint64_t *pte = vmm_virt_to_pte(pagemap, virt, false);
    if (pte) {
        *pte = 0;
        asm volatile("invlpg [%0]" :: "r"(virt) : "memory");
    }
    spinlock_release(&pagemap->lock);
}

bool vmm_remap(struct pagemap *pagemap, uint64_t virt, uint64_t flags) {
    spinlock_acquire(&pagemap->lock);
    uint64_t *pte = vmm_virt_to_pte(pagemap, virt, false);
    if (!pte || !(*pte & VMM_PRESENT)) {
        spinlock_release(&pagemap->lock);
        return false;
    }
    *pte = (*pte & PAGE_ADDR_MASK) + flags;
    asm volatile("invlpg [%0]" :: "r"(virt) : "memory");
    spinlock_release(&pagemap->lock);
    return true;
}

void vmm_switch(struct pagemap *pagemap) {
    asm volatile("mov cr3, %0" :: "r"((uint64_t)pagemap->pml4) : "memory");
}

struct pagemap *vmm_new_pagemap(void) {
    struct pagemap *pagemap = kmalloc(sizeof(struct pagemap));
    if (!pagemap) return NULL;
    pagemap->lock = (spinlock_t)SPINLOCK_INIT;
    pagemap->pml4 = pmm_allocz(1);
    if (!pagemap->pml4) {
        kfree(pagemap);
        return NULL;
    }

    uint64_t *new_pml4  = (uint64_t *)((uint64_t)pagemap->pml4  + MEM_OFFSET);
    uint64_t *kern_pml4 = (uint64_t *)((uint64_t)kernel_pagemap->pml4 + MEM_OFFSET);
    for (int i = 256; i < 512; i++) {
        new_pml4[i] = kern_pml4[i];
    }

    return pagemap;
}

static void destroy_level(uint64_t *pml, size_t start, size_t end, int level) {
    if (level == 0) return;

    for (size_t i = start; i < end; i++) {
        uint64_t *next = get_next_level(pml, i, false);
        if (!next) continue;
        destroy_level(next, 0, 512, level - 1);
    }

    pmm_free((void *)((uint64_t)pml - MEM_OFFSET), 1);
}

void vmm_destroy_pagemap(struct pagemap *pagemap) {
    destroy_level(
        (uint64_t *)((uint64_t)pagemap->pml4 + MEM_OFFSET),
        0, 256, 4);
    kfree(pagemap);
}

void vmm_init(struct limine_memmap_entry **entries, uint64_t entry_count) {
    if (!paging_mode_request.response) for (;;) asm("hlt");

    kernel_pagemap = kmalloc(sizeof(struct pagemap));
    kernel_pagemap->pml4 = pmm_allocz(1);
    kernel_pagemap->lock = (spinlock_t)SPINLOCK_INIT;

    uint64_t *pml4 = (uint64_t *)((uint64_t)kernel_pagemap->pml4 + MEM_OFFSET);
    for (int i = 256; i < 512; i++) {
        get_next_level(pml4, i, true);
    }

    for (uint64_t phys = 0; phys < 0x100000000; phys += PAGE_SIZE) {
        vmm_map(kernel_pagemap, phys + MEM_OFFSET, phys, VMM_KERNEL_RW);
    }

    for (uint64_t i = 0; i < entry_count; i++) {
        uint64_t base = entries[i]->base;
        uint64_t top  = base + entries[i]->length;

        if (base < 0x100000000) base = 0x100000000;
        if (base >= top) continue;

        uint64_t aligned_base = ALIGN_DOWN(base, PAGE_SIZE);
        uint64_t aligned_top  = ALIGN_UP(top, PAGE_SIZE);

        for (uint64_t phys = aligned_base; phys < aligned_top; phys += PAGE_SIZE) {
            vmm_map(kernel_pagemap, phys + MEM_OFFSET, phys, VMM_KERNEL_RW);
        }
    }

    uintptr_t req_start = ALIGN_DOWN((uintptr_t)limine_requests_start_addr, PAGE_SIZE);
    uintptr_t req_end   = ALIGN_UP((uintptr_t)limine_requests_end_addr, PAGE_SIZE);
    uintptr_t text_start = ALIGN_DOWN((uintptr_t)text_start_addr, PAGE_SIZE);
    uintptr_t text_end = ALIGN_UP((uintptr_t)text_end_addr, PAGE_SIZE);
    uintptr_t rodata_start = ALIGN_DOWN((uintptr_t)rodata_start_addr, PAGE_SIZE);
    uintptr_t rodata_end = ALIGN_UP((uintptr_t)rodata_end_addr, PAGE_SIZE);
    uintptr_t data_start = ALIGN_DOWN((uintptr_t)data_start_addr, PAGE_SIZE);
    uintptr_t data_end = ALIGN_UP((uintptr_t)data_end_addr, PAGE_SIZE);

    for (uintptr_t v = req_start; v < req_end; v += PAGE_SIZE) {
        uintptr_t phys = v - KERNEL_V_ADDR + KERNEL_P_ADDR;
        vmm_map(kernel_pagemap, v, phys, VMM_PRESENT | VMM_NO_EXECUTE);
    }

    for (uintptr_t v = text_start; v < text_end; v += PAGE_SIZE) {
        uintptr_t phys = v - KERNEL_V_ADDR + KERNEL_P_ADDR;
        vmm_map(kernel_pagemap, v, phys, VMM_PRESENT);
    }

    for (uintptr_t v = rodata_start; v < rodata_end; v += PAGE_SIZE) {
        uintptr_t phys = v - KERNEL_V_ADDR + KERNEL_P_ADDR;
        vmm_map(kernel_pagemap, v, phys, VMM_PRESENT | VMM_NO_EXECUTE);
    }

    for (uintptr_t v = data_start; v < data_end; v += PAGE_SIZE) {
        uintptr_t phys = v - KERNEL_V_ADDR + KERNEL_P_ADDR;
        vmm_map(kernel_pagemap, v, phys, VMM_KERNEL_RW | VMM_NO_EXECUTE);
    }

    vmm_switch(kernel_pagemap);
}