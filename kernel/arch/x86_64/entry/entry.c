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

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>

#include <fb/fb.h>
#include <debug/debug.h>
#include <sys/gdt.h>
#include <sys/idt.h>

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(6);


__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] =LIMINE_REQUESTS_END_MARKER;


static void hcf(void) {
    for(;;) {
        asm("hlt");
    }
}

void arch_entry(void) {
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) hcf();
    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1) hcf();

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];
    struct framebuffer framebuf = {
        .address = framebuffer->address,
        .width = framebuffer->width,
        .height = framebuffer->height,
        .pitch = framebuffer->pitch,
        .bpp = framebuffer->bpp,
        .memory_model = framebuffer->memory_model,
        .red_mask_size = framebuffer->red_mask_size,
        .red_mask_shift = framebuffer->red_mask_shift,
        .green_mask_size = framebuffer->green_mask_size,
        .green_mask_shift = framebuffer->green_mask_shift,
        .blue_mask_size = framebuffer->blue_mask_size,
        .blue_mask_shift = framebuffer->blue_mask_shift
    };

    serial_init();
    serial_puts("\n\n\n"); // Stupid hack to get out of the EDK2 messages' way.

    gdt_init();
    idt_init();

    framebuffer_init(&framebuf);
    framebuffer_clear();

    kprintf("Hello, Vega build %s!\n", GIT_VERSION);
    kprintf("\nKane Parsons is officially the youngest director ever to have a #1 film at the domestic box office in its opening weekend.\n\n%s", "He is only 20 years old.");

    hcf();
}