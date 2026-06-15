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

#include <sys/idt.h>
#include <sys/gdt.h>

struct idt_entry {
    uint16_t isr_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  flags;
    uint16_t isr_mid;
    uint32_t isr_high;
    uint32_t reserved;
} __attribute__((packed));

struct idtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idtr idtr;

void idt_set_entry(uint8_t vector, void *isr, uint8_t ist, uint8_t flags) {
    uint64_t addr = (uint64_t)isr;
    idt[vector] = (struct idt_entry){
        .isr_low  = addr & 0xFFFF,
        .selector = GDT_KERNEL_CODE,
        .ist      = ist,
        .flags    = flags,
        .isr_mid  = (addr >> 16) & 0xFFFF,
        .isr_high = (addr >> 32) & 0xFFFFFFFF,
        .reserved = 0
    };
}

void idt_init(void) {
    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint64_t)&idt;

    asm volatile("lidt %0" :: "m"(idtr) : "memory");
}