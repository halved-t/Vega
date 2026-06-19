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

#include <io/pci.h>
#include <io/mmio.h>
#include <io/ports.h>
#include <mm/vmm.h>
#include <mm/mm.h>
#include <debug/debug.h>
#include <uacpi/tables.h>
#include <uacpi/acpi.h>
#include <uacpi/uacpi.h>

#define LEGACY_PCI_ADDR 0xCF8
#define LEGACY_PCI_DATA 0xCFC
#define MAX_MCFG_ALLOCATIONS 16
static struct acpi_mcfg *mcfg = NULL;
static size_t mcfg_alloc_count = 0;
static struct acpi_mcfg_allocation mcfg_allocs[MAX_MCFG_ALLOCATIONS];

static uint32_t (*IREAD)(uint16_t, uint8_t, uint8_t, uint8_t, uint16_t, uint8_t);
static void (*IWRITE)(uint16_t, uint8_t, uint8_t, uint8_t, uint16_t, uint32_t, uint8_t);

static uint32_t pci_make_addr(uint16_t bus, uint8_t device, uint32_t function, uint16_t offset) {
    return (1u << 31) | ((uint32_t)bus << 16) | ((uint32_t)device << 11) | ((uint32_t)function << 8) | (offset & 0xFC);
}

static uint64_t mcfg_make_addr(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset) {
    for (size_t i = 0; i < mcfg_alloc_count; i++) {
        struct acpi_mcfg_allocation *alloc = &mcfg_allocs[i];

        if (alloc->segment == segment && bus >= alloc->start_bus && bus <= alloc->end_bus) {
            uint64_t phys_offset = ((uint64_t)(bus - alloc->start_bus) << 20) | ((uint64_t)device << 15) | ((uint64_t)function << 12) | offset;

            return alloc->address + phys_offset + MEM_OFFSET;
        }
    }

    return 0;
}

static uint32_t legacy_read(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint8_t size) {
    (void)segment;

    outd(LEGACY_PCI_ADDR, pci_make_addr(bus, device, function, offset));

    switch (size) {
        case 1:
            return inb(LEGACY_PCI_DATA + (offset & 3));
        case 2:
            return inw(LEGACY_PCI_DATA + (offset & 2));
        case 4:
            return ind(LEGACY_PCI_DATA);
        default:
            return 0;
    };
}

static void legacy_write(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint32_t value, uint8_t size) {
    (void)segment;

    outd(LEGACY_PCI_ADDR, pci_make_addr(bus, device, function, offset));

    switch (size) {
        case 1:
            outb(LEGACY_PCI_DATA + (offset & 3), value);
            break;
        case 2:
            outw(LEGACY_PCI_DATA + (offset & 2), value);
            break;
        case 4:
            outd(LEGACY_PCI_DATA, value);
            break;
        default:
            break;
    }
}

static uint32_t mcfg_read(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint8_t size) {
    uint64_t addr = mcfg_make_addr(segment, bus, device, function, offset);
    if (addr == 0) goto fail;

    switch (size) {
        case 1:
            return inmmb((void*)addr);
        case 2:
            return inmmw((void*)addr);
        case 4:
            return inmmd((void*)addr);
        default:
            goto fail;
    };

    fail:
        return 0;
}

static void mcfg_write(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint32_t value, uint8_t size) {
    uint64_t addr = mcfg_make_addr(segment, bus, device, function, offset);
    if (addr == 0) return;

    switch (size) {
        case 1:
            outmmb((void*)addr, value);
            break;
        case 2:
            outmmw((void*)addr, value);
            break;
        case 4:
            outmmd((void*)addr, value);
            break;
        default:
            break;
    };
}

void pci_init(void) {
    uacpi_table table;
    uacpi_status ret = uacpi_table_find_by_signature(ACPI_MCFG_SIGNATURE, &table);

    if (uacpi_unlikely_error(ret)) {
        kprintf("[PCI]: Falling back to legacy.\n");
        IREAD = legacy_read;
        IWRITE = legacy_write;
        return;
    }

    mcfg = (struct acpi_mcfg *)table.ptr;

    uint8_t *entry = (uint8_t *)(mcfg + 1);
    uint8_t *end = (uint8_t *)mcfg + mcfg->hdr.length;  

    while (entry < end) {
        struct acpi_mcfg_allocation *alloc = (struct acpi_mcfg_allocation *)entry;

        if (mcfg_alloc_count < MAX_MCFG_ALLOCATIONS) {
            mcfg_allocs[mcfg_alloc_count++] = *alloc;
        }

        entry += sizeof(struct acpi_mcfg_allocation);
    }

    kprintf("[PCI]: Using MCFG.\n");
    IREAD = mcfg_read;
    IWRITE = mcfg_write;

    uacpi_table_unref(&table);
}

uint8_t pci_read8(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset) {
    return (uint8_t)IREAD(segment, bus, device, function, offset, 1);
}

uint16_t pci_read16(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset) {
    return (uint16_t)IREAD(segment, bus, device, function, offset, 2);
}

uint32_t pci_read32(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset) {
    return IREAD(segment, bus, device, function, offset, 4);
}

void pci_write8(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint8_t value) {
    IWRITE(segment, bus, device, function, offset, value, 1);
}
void pci_write16(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint16_t value) {
    IWRITE(segment, bus, device, function, offset, value, 2);
}
void pci_write32(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint32_t value) {
    IWRITE(segment, bus, device, function, offset, value, 4);
}