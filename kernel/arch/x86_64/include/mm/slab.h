#ifndef SLAB_H
#define SLAB_H
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
#include <klibc/string.h>

void slab_init(void);
void *slab_alloc(size_t size);
void *slab_realloc(void *ptr, size_t size);
void slab_free(void *ptr);

#define kmalloc slab_alloc
#define krealloc slab_realloc
#define kfree slab_free
static inline void *kcalloc(size_t count, size_t size) {
    void *ret = kmalloc(count * size);
    if (ret == NULL) return NULL;

    memzero(ret, count * size);
    return ret;
}

#endif