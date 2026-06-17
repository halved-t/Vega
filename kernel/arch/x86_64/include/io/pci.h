#ifndef PCI_H
#define PCI_H
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

void pci_init(void);

uint8_t pci_read8(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset);
uint16_t pci_read16(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset);
uint32_t pci_read32(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset);

void pci_write8(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint8_t value);
void pci_write16(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint16_t value);
void pci_write32(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function, uint16_t offset, uint32_t value);

#endif