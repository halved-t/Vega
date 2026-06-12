.PHONY: all kernel run clean

CORES ?= 1
DISK  := disk.hdd

all: $(DISK)

kernel/Vega.elf:
	$(MAKE) -C kernel

kernel: kernel/Vega.elf
.PHONY: kernel

Limine/GNUMakefile:
	cd Limine && ./bootstrap && ./configure --enable-uefi-x86-64

Limine/bin/BOOTX64.EFI: Limine/GNUMakefile
	$(MAKE) -C Limine

$(DISK): kernel/Vega.elf Limine/bin/BOOTX64.EFI limine.conf
	dd if=/dev/zero bs=1M count=0 seek=64 of=$(DISK)
	parted -s $(DISK) mklabel gpt
	parted -s $(DISK) mkpart ESP fat32 2048s 100%
	parted -s $(DISK) set 1 esp on
	mformat -i $(DISK)@@1048576 -F ::
	mmd -i $(DISK)@@1048576 ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	mcopy -i $(DISK)@@1048576 kernel/Vega.elf          ::/boot/
	mcopy -i $(DISK)@@1048576 limine.conf               ::/boot/limine/
	mcopy -i $(DISK)@@1048576 Limine/bin/BOOTX64.EFI   ::/EFI/BOOT/

hdd: $(DISK)
.PHONY: hdd

run: $(DISK)
	qemu-system-x86_64 -M q35 -smp $(CORES) -m 1G \
	    -bios /usr/share/ovmf/x64/OVMF.4m.fd \
	    -drive file=$(DISK),format=raw \
	    -serial stdio

clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C Limine clean
	rm -f $(DISK)