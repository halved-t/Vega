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

#include <uacpi/uacpi.h>
#include <uacpi/kernel_api.h>
#include <mm/vmm.h>
#include <mm/pmm.h>
#include <mm/slab.h>
#include <mm/mm.h>
#include <klibc/misc.h>
#include <debug/debug.h>
#include <sync/spinlock.h>
#include <io/pci.h>
#include <io/mmio.h>
#include <io/ports.h>
#include <stdint.h>
#include <stddef.h>
#include <limine.h>

__attribute__((used, section(".limine_requests")))
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST_ID,
    .revision = 0
};

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
    if (rsdp_request.response == NULL) return UACPI_STATUS_NOT_FOUND;

    // Limine returns virtual, uACPI wants physical.
    *out_rsdp_address = (uacpi_phys_addr)((uint64_t)rsdp_request.response->address - MEM_OFFSET);
    return UACPI_STATUS_OK;
}

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
    (void)len;
    return (void *)(addr + MEM_OFFSET);
}

void uacpi_kernel_unmap(void *addr, uacpi_size len) {
    (void)addr;
    (void)len;
}

void uacpi_kernel_log(uacpi_log_level lvl, const uacpi_char* fmt) {
    const char *pfx;

    switch (lvl) {
        case UACPI_LOG_DEBUG: pfx = "[uACPI] [DEBUG]"; break;
        case UACPI_LOG_TRACE: pfx = "[uACPI] [TRACE]"; break;
        case UACPI_LOG_INFO: pfx = "[uACPI] [INFO]"; break;
        case UACPI_LOG_WARN: pfx = "[uACPI] [WARN]"; break;
        case UACPI_LOG_ERROR: pfx = "[uACPI] [ERROR]"; break;
        default: pfx = "[uACPI] [?] "; break;
    }

    kprintf("%s: %s", pfx, fmt);
}

uacpi_status uacpi_kernel_pci_device_open(
    uacpi_pci_address address, uacpi_handle *out_handle
) {
    uacpi_pci_address *addr;
    addr = uacpi_kernel_alloc(sizeof(*addr));
    if (addr == NULL) return UACPI_STATUS_OUT_OF_MEMORY;

    *addr = address;
    *out_handle = addr;

    return UACPI_STATUS_OK;
}
void uacpi_kernel_pci_device_close(uacpi_handle handle) {
    uacpi_kernel_free(handle);
}

uacpi_status uacpi_kernel_pci_read8(
    uacpi_handle device, uacpi_size offset, uacpi_u8 *value
) {
    uacpi_pci_address *addr = device;
    *value = pci_read8(addr->segment, addr->bus, addr->device, addr->function, offset);

    return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_read16(
    uacpi_handle device, uacpi_size offset, uacpi_u16 *value
) {
    uacpi_pci_address *addr = device;
    *value = pci_read16(addr->segment, addr->bus, addr->device, addr->function, offset);

    return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_read32(
    uacpi_handle device, uacpi_size offset, uacpi_u32 *value
) {
    uacpi_pci_address *addr = device;
    *value = pci_read32(addr->segment, addr->bus, addr->device, addr->function, offset);

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_write8(
    uacpi_handle device, uacpi_size offset, uacpi_u8 value
) {
    uacpi_pci_address *addr = device;
    pci_write8(addr->segment, addr->bus, addr->device, addr->function, offset, value);

    return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_write16(
    uacpi_handle device, uacpi_size offset, uacpi_u16 value
) {
    uacpi_pci_address *addr = device;
    pci_write16(addr->segment, addr->bus, addr->device, addr->function, offset, value);

    return UACPI_STATUS_OK;
}
uacpi_status uacpi_kernel_pci_write32(
    uacpi_handle device, uacpi_size offset, uacpi_u32 value
) {
    uacpi_pci_address *addr = device;
    pci_write32(addr->segment, addr->bus, addr->device, addr->function, offset, value);

    return UACPI_STATUS_OK;
}

struct uacpi_io_region {
    uacpi_io_addr base;
    uacpi_size len;
};

uacpi_status uacpi_kernel_io_map(
    uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle
) {
    struct uacpi_io_region *r = uacpi_kernel_alloc(sizeof(*r));
    if (r == NULL) return UACPI_STATUS_OUT_OF_MEMORY;

    r->base = base;
    r->len = len;
    *out_handle = r;
    return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle handle) {
    uacpi_kernel_free(handle);
}

uacpi_status uacpi_kernel_io_read8(
    uacpi_handle handle, uacpi_size offset, uacpi_u8 *out_value
) {
    struct uacpi_io_region *r = (struct uacpi_io_region *)handle;
    if (offset >= r->len) return UACPI_STATUS_INVALID_ARGUMENT;

    *out_value = inb(r->base + offset);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read16(
    uacpi_handle handle, uacpi_size offset, uacpi_u16 *out_value
) {
    struct uacpi_io_region *r = (struct uacpi_io_region *)handle;
    if (offset + 1 >= r->len) return UACPI_STATUS_INVALID_ARGUMENT;

    *out_value = inw(r->base + offset);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read32(
    uacpi_handle handle, uacpi_size offset, uacpi_u32 *out_value
) {
    struct uacpi_io_region *r = (struct uacpi_io_region *)handle;
    if (offset + 3 >= r->len) return UACPI_STATUS_INVALID_ARGUMENT;

    *out_value = ind(r->base + offset);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write8(
    uacpi_handle handle, uacpi_size offset, uacpi_u8 in_value
) {
    struct uacpi_io_region *r = (struct uacpi_io_region *)handle;
    if (offset >= r->len) return UACPI_STATUS_INVALID_ARGUMENT;

    outb(r->base + offset, in_value);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write16(
    uacpi_handle handle, uacpi_size offset, uacpi_u16 in_value
) {
    struct uacpi_io_region *r = (struct uacpi_io_region *)handle;
    if (offset + 1>= r->len) return UACPI_STATUS_INVALID_ARGUMENT;

    outw(r->base + offset, in_value);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write32(
    uacpi_handle handle, uacpi_size offset, uacpi_u32 in_value
) {
    struct uacpi_io_region *r = (struct uacpi_io_region *)handle;
    if (offset + 3 >= r->len) return UACPI_STATUS_INVALID_ARGUMENT;

    outd(r->base + offset, in_value);
    return UACPI_STATUS_OK;
}

void *uacpi_kernel_alloc(uacpi_size size) {
    return kmalloc(size);
}


void uacpi_kernel_free(void *mem) {
    kfree(mem);
}

uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
    return 0; // FIXME: stub
}

void uacpi_kernel_stall(uacpi_u8 usec) {
    // FIXME: stub
    for (volatile uacpi_u64 i = 0; i < (usec * 100); i++) asm volatile("pause");
}

void uacpi_kernel_sleep(uacpi_u64 msec) {
    // FIXME: stub
    for (uacpi_u64 i = 0; i < msec; i++) {
        uacpi_kernel_stall(200); // shittiest workaround EVER.
        uacpi_kernel_stall(200);
        uacpi_kernel_stall(200);
        uacpi_kernel_stall(200);
        uacpi_kernel_stall(200);
    }
}

uacpi_handle uacpi_kernel_create_mutex(void) {
    // FIXME: stub
    return (uacpi_handle)1;
}

void uacpi_kernel_free_mutex(uacpi_handle handle) {
    // FIXME: stub
    (void)handle;
}

uacpi_handle uacpi_kernel_create_event(void) {
    // FIXME: stub
    return (uacpi_handle)1;
}

void uacpi_kernel_free_event(uacpi_handle handle) {
    // FIXME: stub
    (void)handle;
}

uacpi_thread_id uacpi_kernel_get_thread_id(void) {
    return (uacpi_thread_id)1; // FIXME: stub
}

uacpi_interrupt_state uacpi_kernel_disable_interrupts(void) {
    uacpi_interrupt_state state;

    asm volatile("pushfq;" "pop %0;" "cli" : "=r"(state) :: "memory");

    return state;
}

void uacpi_kernel_restore_interrupts(uacpi_interrupt_state state) {
    uacpi_interrupt_state current;

    asm volatile("pushfq;" "pop %0" : "=r"(current) :: "memory");

    if (state & (1 << 9)) {
        asm volatile("sti");
    } else {
        asm volatile("cli");
    }
}

uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle handle, uacpi_u16 timeout) {
    // FIXME: stub
    (void)handle;
    (void)timeout;
    return UACPI_STATUS_UNIMPLEMENTED;
}

void uacpi_kernel_release_mutex(uacpi_handle handle) {
    // FIXME: stub
    (void)handle;
}


uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle handle, uacpi_u16 timeout) {
    // FIXME: stub
    (void)handle;
    (void)timeout;

    return UACPI_FALSE;
}

void uacpi_kernel_signal_event(uacpi_handle handle) {
    (void)handle; // FIXME: stub
}

void uacpi_kernel_reset_event(uacpi_handle handle) {
    (void)handle; // FIXME: stub
}

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request* request) {
    switch (request->type) {
        case UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT:
            asm volatile("int $3");
            return UACPI_STATUS_OK;

        case UACPI_FIRMWARE_REQUEST_TYPE_FATAL:
            kprintf("[ACPI] [FATAL]: type=%u, code=%u, arg=%llu\n", request->fatal.type, request->fatal.code, request->fatal.arg);
            // TODO: Replace with panic later
            for (;;) asm volatile ("hlt");
        
        default:
            return UACPI_STATUS_UNIMPLEMENTED;
    };
}

uacpi_status uacpi_kernel_install_interrupt_handler(
    uacpi_u32 irq, uacpi_interrupt_handler handler, uacpi_handle ctx,
    uacpi_handle *out_irq_handle
) {
    // FIXME: Stub
    (void)irq;
    (void)handler;
    (void)ctx;
    (void)out_irq_handle;
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(
    uacpi_interrupt_handler handler, uacpi_handle irq_handle
) {
    // FIXME: stub
    (void)handler;
    (void)irq_handle;
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_handle uacpi_kernel_create_spinlock(void) {
    spinlock_t *lock = uacpi_kernel_alloc(sizeof(*lock));
    if (lock == NULL) return NULL;

    lock->locked = false;
    return lock;
}

void uacpi_kernel_free_spinlock(uacpi_handle handle) {
    uacpi_kernel_free(handle);
}

uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle handle) {
    uacpi_cpu_flags flags;

    asm volatile("pushfq;" "pop %0;" "cli" : "=r"(flags) :: "memory");

    spinlock_acquire(handle);
    return flags;
}

void uacpi_kernel_unlock_spinlock(uacpi_handle handle, uacpi_cpu_flags flags) {
    spinlock_release(handle);

    if (flags & (1UL << 9)) {
        asm volatile("sti");
    }
}

uacpi_status uacpi_kernel_schedule_work(
    uacpi_work_type type, uacpi_work_handler handler, uacpi_handle ctx
) {
    // FIXME: stub
    (void)type;
    (void)handler;
    (void)ctx;
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_wait_for_work_completion(void) {
    // FIXME: Stub
    return UACPI_STATUS_UNIMPLEMENTED;
}
