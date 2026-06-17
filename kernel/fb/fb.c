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

#include <fb/fb.h>
#include "Flanterm/src/flanterm.h"
#include "Flanterm/src/flanterm_backends/fb.h"
#include <limine.h>
#include <klibc/string.h>
#include <mm/slab.h>
#define FLANTERM_FB_DISABLE_BUMP_ALLOC

struct flanterm_context *ft_ctx;
uint8_t initialised = 0;

static void ft_free(void *addr, size_t sz) {
    (void)sz;
    kfree(addr);
}

void framebuffer_init(struct framebuffer *fb) {
    ft_ctx = flanterm_fb_init(
        kmalloc,
        ft_free,
        fb->address, fb->width, fb->height, fb->pitch,
        fb->red_mask_size, fb->red_mask_shift,
        fb->green_mask_size, fb->green_mask_shift,
        fb->blue_mask_size, fb->blue_mask_shift,
        NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, 0, 0, 1,
        0, 0,
        0,
        0
    );
    
    initialised = 1;
}

void framebuffer_putchar(char chr) {
    if (initialised == 0) return;

    if (chr == '\n') {
        flanterm_write(ft_ctx, "\r\n", 2);
        return;
    }

    flanterm_write(ft_ctx, &chr, 1);
}

void framebuffer_puts(char* str) {
    if (initialised == 0) return;

    for (size_t i = 0; str[i]; i++) {
        framebuffer_putchar(str[i]);
    }
}

void framebuffer_clear(void) {
    if (initialised == 0) return;

    flanterm_write(ft_ctx, "\033[2J\033[H", 7);
}