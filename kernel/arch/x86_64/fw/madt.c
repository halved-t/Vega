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

#include <fw/madt.h>
#include <uacpi/tables.h>
#include <uacpi/acpi.h>
#include <uacpi/uacpi.h>
#include <mm/mm.h>
#include <debug/debug.h>
#define MAX_ISOS 256
#define MAX_CPUS 256

static struct acpi_madt *madt = NULL;
static uint64_t lapic_addr = 0;
static uint64_t ioapic_addr = 0;
static uint8_t  ioapic_id = 0;
static uint32_t ioapic_gsi_base = 0;
static struct acpi_madt_interrupt_source_override isos[MAX_ISOS];
static size_t iso_count = 0;
static uint8_t lapic_ids[MAX_CPUS];
static size_t cpu_count = 0;

void madt_init(void) {
    uacpi_table table;
    uacpi_status ret = uacpi_table_find_by_signature(ACPI_MADT_SIGNATURE, &table);
     
    if (uacpi_unlikely_error(ret)) {
        kprintf("[ACPI] [MADT]: Failed to find table. (%s)\n", uacpi_status_to_string(ret));
        for (;;) asm volatile("hlt");
    }

    madt = (struct acpi_madt *)table.virt_addr;
    lapic_addr = madt->local_interrupt_controller_address;
    
    uint8_t *entry = (uint8_t *)(madt + 1);
    uint8_t *end = (uint8_t *)madt + madt->hdr.length;

    while (entry < end) {
        uint8_t type = entry[0];
        uint8_t length = entry[1];

        switch (type) {
            case ACPI_MADT_ENTRY_TYPE_LAPIC:
                {
                    struct acpi_madt_lapic *lapic = (struct acpi_madt_lapic *)entry;

                    if (lapic->flags & 1) { // Processor enabled flag
                        if (cpu_count < MAX_CPUS) {
                            lapic_ids[cpu_count++] = lapic->id;
                        }
                    }

                    break;
                }

            case ACPI_MADT_ENTRY_TYPE_IOAPIC:
                {
                    struct acpi_madt_ioapic *ioapic = (struct acpi_madt_ioapic *)entry;

                    if (ioapic_addr == 0) { // Just take the first one.
                        ioapic_addr = ioapic->address;
                        ioapic_id = ioapic->id;
                        ioapic_gsi_base = ioapic->gsi_base;
                    }

                    break;
                }

            case ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE:
                {
                    struct acpi_madt_interrupt_source_override *iso = (struct acpi_madt_interrupt_source_override *)entry;

                    if (iso_count < MAX_ISOS) {
                        isos[iso_count++] = *iso;
                    }

                    break;
                }

            case ACPI_MADT_ENTRY_TYPE_LAPIC_ADDRESS_OVERRIDE:
                {
                    struct acpi_madt_lapic_address_override *lao = (struct acpi_madt_lapic_address_override *)entry;

                    lapic_addr = lao->address;

                    break;
                }
        }

        entry += length;
    }
}

struct acpi_madt_interrupt_source_override *madt_get_isos(size_t *count) {
    *count = iso_count;
    return isos;
}

uint64_t madt_get_lapic_addr(void) { return lapic_addr; }
uint8_t *madt_get_lapic_ids(void) { return lapic_ids; }
uint64_t madt_get_ioapic_addr(void) { return ioapic_addr; }
uint8_t madt_get_ioapic_id(void) { return ioapic_id; }
uint32_t madt_get_ioapic_gsi(void) { return ioapic_gsi_base; }
size_t madt_get_cpu_count(void) { return cpu_count; }