#ifndef VMM_H
#define VMM_H
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
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define VMM_PRESENT (1ull << 0)
#define VMM_WRITABLE (1ull << 1)
#define VMM_USER (1ull << 2)
#define VMM_WRITE_THROUGH (1ull << 3)
#define VMM_NO_CACHE (1ull << 4)
#define VMM_ACCESSED (1ull << 5)
#define VMM_DIRTY (1ull << 6)
#define VMM_HUGE (1ull << 7)
#define VMM_GLOBAL (1ull << 8)
#define VMM_NO_EXECUTE (1ull << 63)
#define VMM_KERNEL_RW (VMM_PRESENT | VMM_WRITABLE)
#define VMM_KERNEL_RO (VMM_PRESENT | VMM_NO_EXECUTE)
#define VMM_USER_RW (VMM_PRESENT | VMM_WRITABLE | VMM_USER)
#define VMM_USER_RO (VMM_PRESENT | VMM_USER | VMM_NO_EXECUTE)
#define INVALID_PHYS (~0ull)

struct pagemap {
    uint64_t *pml4;
};

extern struct pagemap *kernel_pagemap;

void vmm_init(struct limine_memmap_entry **entries, uint64_t entry_count);
struct pagemap *vmm_new_pagemap(void);
void vmm_destroy_pagemap(struct pagemap *pagemap);
void vmm_map(struct pagemap *pagemap, uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_unmap(struct pagemap *pagemap, uint64_t virt);
void vmm_switch(struct pagemap *pagemap);
uint64_t *vmm_virt_to_pte(struct pagemap *pagemap, uint64_t virt, bool alloc);
uint64_t vmm_virt_to_phys(struct pagemap *pagemap, uint64_t virt);
bool vmm_remap(struct pagemap *pagemap, uint64_t virt, uint64_t flags);

#endif