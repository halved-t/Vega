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
        case UACPI_LOG_INFO: pfx = "[uACPI] [INFO] "; break;
        case UACPI_LOG_WARN: pfx = "[uACPI] [WARN] "; break;
        case UACPI_LOG_ERROR: pfx = "[uACPI] [ERROR]"; break;
        default: pfx = "[uACPI] [?] "; break;
    }

    kprintf("%s: %s", pfx, fmt);
}

/*

uacpi_status uacpi_kernel_pci_device_open(
    uacpi_pci_address address, uacpi_handle *out_handle
) {
}
void uacpi_kernel_pci_device_close(uacpi_handle) {
}

uacpi_status uacpi_kernel_pci_read8(
    uacpi_handle device, uacpi_size offset, uacpi_u8 *value
) {
}
uacpi_status uacpi_kernel_pci_read16(
    uacpi_handle device, uacpi_size offset, uacpi_u16 *value
) {
}
uacpi_status uacpi_kernel_pci_read32(
    uacpi_handle device, uacpi_size offset, uacpi_u32 *value
) {
}

uacpi_status uacpi_kernel_pci_write8(
    uacpi_handle device, uacpi_size offset, uacpi_u8 value
) {
}
uacpi_status uacpi_kernel_pci_write16(
    uacpi_handle device, uacpi_size offset, uacpi_u16 value
) {
}
uacpi_status uacpi_kernel_pci_write32(
    uacpi_handle device, uacpi_size offset, uacpi_u32 value
) {
}

uacpi_status uacpi_kernel_io_map(
    uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle
) {
}

void uacpi_kernel_io_unmap(uacpi_handle handle) {
}

uacpi_status uacpi_kernel_io_read8(
    uacpi_handle, uacpi_size offset, uacpi_u8 *out_value
) {
}

uacpi_status uacpi_kernel_io_read16(
    uacpi_handle, uacpi_size offset, uacpi_u16 *out_value
) {
}

uacpi_status uacpi_kernel_io_read32(
    uacpi_handle, uacpi_size offset, uacpi_u32 *out_value
) {
}

uacpi_status uacpi_kernel_io_write8(
    uacpi_handle, uacpi_size offset, uacpi_u8 in_value
) {
}

uacpi_status uacpi_kernel_io_write16(
    uacpi_handle, uacpi_size offset, uacpi_u16 in_value
) {
}

uacpi_status uacpi_kernel_io_write32(
    uacpi_handle, uacpi_size offset, uacpi_u32 in_value
) {
}

void *uacpi_kernel_alloc(uacpi_size size) {
    return kmalloc(size);
}

void *uacpi_kernel_alloc_zeroed(uacpi_size size) {
    return kcalloc(size);
}

void uacpi_kernel_free(void *mem) {
    kfree(mem);
}

uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
    return 0; // FIXME: stub
}

void uacpi_kernel_stall(uacpi_u8 usec) {

}

void uacpi_kernel_sleep(uacpi_u64 msec) {

}

uacpi_handle uacpi_kernel_create_mutex(void) {

}

void uacpi_kernel_free_mutex(uacpi_handle) {

}

uacpi_handle uacpi_kernel_create_event(void) {

}

void uacpi_kernel_free_event(uacpi_handle) {

}

uacpi_thread_id uacpi_kernel_get_thread_id(void) {
    return (uacpi_thread_id)1; // FIXME: stub
}

uacpi_interrupt_state uacpi_kernel_disable_interrupts(void) {
/**
 * Restore the state of the interrupt flags to the kernel-defined value provided
 * in 'state'.
 *
}

void uacpi_kernel_restore_interrupts(uacpi_interrupt_state state) {
  /*  sable interrupts and return a kernel-defined value representing the
 * "before" state. This value is used in the subsequent call to restore the
 * prior state.
 *
 * Note that this is talking about ALL interrupts on the current CPU, not just
 * those installed by uACPI. This is typically achieved by executing the 'cli'
 * instruction on x86, 'msr daifset, #3' on aarch64 etc.*
}

uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle, uacpi_u16) {
}

void uacpi_kernel_release_mutex(uacpi_handle) {

}

/**
 * Try to wait for an event (counter > 0) with a millisecond timeout.
 * A timeout value of 0xFFFF implies infinite wait.
 *
 * The internal counter is decremented by 1 if wait was successful.
 *
 * A successful wait is indicated by returning UACPI_TRUE.
 *
uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle, uacpi_u16) {
    
}

void uacpi_kernel_signal_event(uacpi_handle) {

}

void uacpi_kernel_reset_event(uacpi_handle) {

}

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request*) {

}

uacpi_status uacpi_kernel_install_interrupt_handler(
    uacpi_u32 irq, uacpi_interrupt_handler, uacpi_handle ctx,
    uacpi_handle *out_irq_handle
) {

}

uacpi_status uacpi_kernel_uninstall_interrupt_handler(
    uacpi_interrupt_handler, uacpi_handle irq_handle
) {

}

uacpi_handle uacpi_kernel_create_spinlock(void) {

}

void uacpi_kernel_free_spinlock(uacpi_handle) {

}

uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle) {

}

void uacpi_kernel_unlock_spinlock(uacpi_handle, uacpi_cpu_flags) {

}

uacpi_status uacpi_kernel_schedule_work(
    uacpi_work_type, uacpi_work_handler, uacpi_handle ctx
) {

}

/**
 * Waits for two types of work to finish:
 * 1. All in-flight interrupts installed via uacpi_kernel_install_interrupt_handler
 * 2. All work scheduled via uacpi_kernel_schedule_work
 *
 * Note that the waits must be done in this order specifically.
 *
uacpi_status uacpi_kernel_wait_for_work_completion(void) {

}
*/