#ifndef SPINLOCK_H
#define SPINLOCK_H
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

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool locked;
} spinlock_t;


#define SPINLOCK_INIT {0}
void spinlock_acquire(spinlock_t *lock);
bool spinlock_try(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);

#endif