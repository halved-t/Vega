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

#define LEGACY_PCI_ADDR 0xCF8
#define LEGACY_PCI_DATA 0xCFC

static uint32_t (*IREAD)(uint16_t, uint8_t, uint8_t, uint8_t, uint16_t, uint8_t);
static void (*IWRITE)(uint16_t, uint8_t, uint8_t, uint8_t, uint16_t, uint32_t, uint8_t);

static uint32_t pci_make_addr(uint16_t bus, uint8_t device, uint32_t function, uint16_t offset) {
    return (1u << 31) | ((uint32_t)bus << 16) | ((uint32_t)device << 11) | ((uint32_t)function << 8) | (offset & 0xFC);
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

// Defaults in case someone somehow uses pci_read/write before pci_init
IREAD = legacy_read;
IWRITE = legacy_write;

void pci_init(void) {
    // We don't have MCFG read/write so it always defaults to legacy.
    IREAD = legacy_read;
    IWRITE = legacy_write;
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