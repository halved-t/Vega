OUTPUT_FORMAT(elf64-x86-64)
ENTRY(arch_entry)

PHDRS
{
    limine_requests PT_LOAD;
    text            PT_LOAD;
    rodata          PT_LOAD;
    data            PT_LOAD;
}

SECTIONS
{
    . = 0xffffffff80000000;

    limine_requests_start_addr = .;
    .limine_requests : ALIGN(8) {
        KEEP(*(.limine_requests_start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests_end))
    } :limine_requests
    limine_requests_end_addr = .;

    . = ALIGN(0x1000);

    text_start_addr = .;
    .text : {
        *(.text .text.*)
    } :text
    text_end_addr = .;

    . = ALIGN(0x1000);

    rodata_start_addr = .;
    .rodata : {
        *(.rodata .rodata.*)
    } :rodata
    rodata_end_addr = .;

    . = ALIGN(0x1000);

    data_start_addr = .;
    .data : {
        *(.data .data.*)
    } :data
    .bss : ALIGN(16) {
        *(.bss .bss.*)
        *(COMMON)
    } :data
    data_end_addr = .;

    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
    }
}