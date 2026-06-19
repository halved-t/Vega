#ifndef MADT_H
#define MADT_H
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
#include <stdint.h>
#include <uacpi/acpi.h>

void madt_init(void);

uint64_t madt_get_lapic_addr(void);
uint8_t *madt_get_lapic_ids(void);
uint64_t madt_get_ioapic_addr(void);
uint8_t madt_get_ioapic_id(void);
uint32_t madt_get_ioapic_gsi(void);
size_t madt_get_cpu_count(void);
struct acpi_madt_interrupt_source_override *madt_get_isos(size_t *count);

#endif