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

#include <sys/gdt.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct tss_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed));

struct gdt {
    struct gdt_entry null;
    struct gdt_entry kernel_code;
    struct gdt_entry kernel_data;
    struct gdt_entry user_code;
    struct gdt_entry user_data;
    struct tss_entry tss;
} __attribute__((packed));

struct gdtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct gdt gdt = {0};
struct gdtr gdtr = {0};

extern void gdt_reload(void);
extern void tss_reload(void);

static struct gdt_entry make_entry(uint8_t access, uint8_t flags) {
    return (struct gdt_entry) {
        .limit_low = 0xFFFF,
        .base_low = 0,
        .base_mid = 0,
        .access = access,
        .granularity = (flags << 4) | 0xF,
        .base_high = 0
    };
}

void gdt_init(void) {
    gdt.null = (struct gdt_entry){0};
    gdt.kernel_code = make_entry(0x9A, 0xA);
    gdt.kernel_data = make_entry(0x92, 0xC);
    gdt.user_code = make_entry(0xFA, 0xA);
    gdt.user_data = make_entry(0xF2, 0xC);
    gdt.tss = (struct tss_entry){0};

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uint64_t)&gdt;

    gdt_reload();
}

void gdt_load_tss(struct tss *tss) {
    uint64_t addr = (uint64_t)tss;
    gdt.tss.limit_low  = sizeof(struct tss) - 1;
    gdt.tss.base_low   = (uint16_t)addr;
    gdt.tss.base_mid   = (uint8_t)(addr >> 16);
    gdt.tss.access     = 0x89;
    gdt.tss.granularity = 0;
    gdt.tss.base_high  = (uint8_t)(addr >> 24);
    gdt.tss.base_upper = (uint32_t)(addr >> 32);
    tss_reload();
}