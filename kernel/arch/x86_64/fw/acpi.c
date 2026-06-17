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

#include <fw/acpi.h>
#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>
#include <uacpi/tables.h>
#include <uacpi/status.h>
#include <uacpi/event.h>
#include <debug/debug.h>

void acpi_init(void) {
    /*uacpi_status ret = uacpi_initialize(0);
    if (uacpi_unlikely_error(ret))
        for (;;) {
            kprintf("[uACPI] [ERROR OH FUCK]: %s", uacpi_status_to_string(ret));
            asm("hlt");
        }
    */
}