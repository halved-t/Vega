#ifndef MMIO_H
#define MMIO_H
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

static inline void outmmb(void *addr, uint8_t val) { asm volatile("mov byte ptr [%0], %1" :: "r"(addr), "r"(val) : "memory"); }
static inline void outmmw(void *addr, uint16_t val) { asm volatile("mov word ptr [%0], %1" :: "r"(addr), "r"(val) : "memory"); }
static inline void outmmd(void *addr, uint32_t val) { asm volatile("mov dword ptr [%0], %1" :: "r"(addr), "r"(val) : "memory"); }
static inline void outmmq(void *addr, uint64_t val) { asm volatile("mov qword ptr [%0], %1" :: "r"(addr), "r"(val) : "memory"); }

static inline uint8_t inmmb(void *addr) { uint8_t val; asm volatile("mov %0, byte ptr [%1]" : "=r"(val) : "r"(addr) : "memory"); return val; }
static inline uint16_t inmmw(void *addr) { uint16_t val; asm volatile("mov %0, word ptr [%1]" : "=r"(val) : "r"(addr) : "memory"); return val; }
static inline uint32_t inmmd(void *addr) { uint32_t val; asm volatile("mov %0, dword ptr [%1]" : "=r"(val) : "r"(addr) : "memory"); return val; }
static inline uint64_t inmmq(void *addr) { uint64_t val; asm volatile("mov %0, qword ptr [%1]" : "=r"(val) : "r"(addr) : "memory"); return val; }

#endif