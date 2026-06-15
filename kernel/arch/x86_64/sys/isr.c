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
#include <sys/isr.h>
#include <debug/debug.h>

static isr_handler_t handlers[256] = {0};

static const char *except_names[] = {
    "Division By Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating Point",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating Point",
    "Virtualization",
    "Control Protection",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor Injection",
    "VMM Communication",
    "Security Exception",
    "Reserved"
};

void isr_handler(cpu_registers_t *regs) {
    if (regs->isrNum < 256 && handlers[regs->isrNum]) {
        handlers[regs->isrNum](regs);
        return;
    }

    if (regs->isrNum < 32) {
        kprintf("\n!!! EXCEPTION: %s (vector=%llu, err=%llu)\n",
            except_names[regs->isrNum],
            regs->isrNum,
            regs->errCode);
        kprintf("RIP=%016llx RSP=%016llx RFLAGS=%016llx\n",
            regs->rip, regs->rsp, regs->rflags);
        kprintf("RAX=%016llx RBX=%016llx RCX=%016llx RDX=%016llx\n",
            regs->rax, regs->rbx, regs->rcx, regs->rdx);
        for (;;) asm("hlt");
    }
}

void isr_register(uint8_t vector, isr_handler_t handler) {
    handlers[vector] = handler;
}

extern void *isr_stub_table[256];

void isr_init(void) {
    for (int i = 0; i < 256; i++) {
        idt_set_entry(i, isr_stub_table[i], 0, 0x8E);
    }

    idt_set_entry(8, isr_stub_table[8], 1, 0x8E);
}