ARCH := x86_64

C_ARCH_FILES := $(wildcard arch/$(ARCH)/*/*.c) $(wildcard arch/$(ARCH)/fw/uACPI/source/*/*.c)
C_FILES      := $(wildcard */*.c) fb/Flanterm/src/flanterm.c fb/Flanterm/src/flanterm_backends/fb.c
ASM_FILES    := $(wildcard arch/$(ARCH)/*/*.asm)


OBJ         := $(C_ARCH_FILES:.c=.o) $(C_FILES:.c=.o) $(ASM_FILES:.asm=.o)
HEADER_DEPS := $(C_ARCH_FILES:.c=.d) $(C_FILES:.c=.d)

GIT_HASH := $(shell [ -n "$$(git status --porcelain 2>/dev/null)" ] && echo "dev" || git rev-parse --short=8 HEAD 2>/dev/null || echo "unknown")

ifeq ($(ARCH),x86_64)
    CC     := clang
    LD     := ld.lld
    AS     := nasm
    TARGET := x86_64-elf

    CFLAGS := \
        -Wall -Wextra -O2 -pipe \
        -I include/ \
        -I arch/$(ARCH)/include/ \
        -I ../Limine/limine-protocol/include/ \
        -I arch/$(ARCH)/fw/uACPI/include/

	CFLAGS += -DGIT_VERSION=\"$(GIT_HASH)\"

    CFLAGS += -DUACPI_BAREBONES_MODE -DUACPI_NATIVE_ALLOC_ZEROED # TODO: Implement the things that this requires.

    INTERNAL_CFLAGS := \
        -target $(TARGET) \
        -std=gnu11 \
        -ffreestanding \
        -fno-stack-protector \
        -fno-pie \
        -masm=intel \
        -mcmodel=kernel \
        -mno-80387 \
        -mno-mmx \
        -mno-3dnow \
        -mno-sse \
        -mno-sse2 \
        -mno-red-zone \
        -MMD

    INTERNAL_LDFLAGS := \
        -T arch/$(ARCH)/linker.ld \
        -nostdlib \
        -static \
        -z max-page-size=0x1000 \
        -z noexecstack

    AS_FLAGS := -g
endif

.PHONY: all clean

all: Vega.elf

Vega.elf: $(OBJ)
	$(LD) $(INTERNAL_LDFLAGS) $^ -o $@

-include $(HEADER_DEPS)

%.o: %.asm
	$(AS) $(AS_FLAGS) -f elf64 $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INTERNAL_CFLAGS) -c $< -o $@

clean:
	rm -f Vega.elf $(OBJ) $(HEADER_DEPS)