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

#include <stddef.h>
#include <sync/spinlock.h>
#include <klibc/string.h>

bool spinlock_try(spinlock_t *lock) {
    if (!lock) return false;
    return __sync_bool_compare_and_swap(&lock->locked, 0, 1);
}

void spinlock_acquire(spinlock_t *lock) {
    if (!lock) return;
    for (;;) {
        if (spinlock_try(lock)) break;
        asm volatile("pause");
    }
}

void spinlock_release(spinlock_t *lock) {
    if (!lock) return;
    __atomic_store_n(&lock->locked, 0, __ATOMIC_SEQ_CST);
}