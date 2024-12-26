
QEMU_CPU_OPTS  =
QEMU          ?= qemu-system-x86_64

.PHONY: _cpu_on_config
_cpu_on_config:


$(OUTPUT)/image.hdd: $(OUTPUT)/badger-os.elf
	# Create boot filesystem
	echo Create EFI filesystem
	rm -rf $(BUILDDIR)/image.dir
	mkdir -p $(BUILDDIR)/image.dir/EFI/BOOT/
	mkdir -p $(BUILDDIR)/image.dir/boot/
	make -C lib/limine
	cp lib/limine/BOOTX64.EFI $(BUILDDIR)/image.dir/EFI/BOOT/
	cp lib/limine/limine-bios.sys $(BUILDDIR)/image.dir/boot/
	cp port/generic/limine.cfg $(BUILDDIR)/image.dir/boot/
	cp $(OUTPUT)/badger-os.elf $(BUILDDIR)/image.dir/boot/
	
	# Format FAT filesystem
	echo Create FAT filesystem blob
	rm -f $(BUILDDIR)/image_bootfs.bin
	dd if=/dev/zero bs=1M count=4  of=$(BUILDDIR)/image_bootfs.bin
	mformat -i $(BUILDDIR)/image_bootfs.bin
	mcopy -s -i $(BUILDDIR)/image_bootfs.bin $(BUILDDIR)/image.dir/* ::/
	
	# Create image
	echo Create image
	rm -f $(OUTPUT)/image.hdd
	dd if=/dev/zero bs=1M count=64 of=$(OUTPUT)/image.hdd
	# 4M /boot, remainder /root
	echo pre sgdisk
	sgdisk -a 1 \
		--new=3:34:8225 --change-name=3:boot --typecode=3:0x0700 \
		--new=4:8226:-0 --change-name=4:root --typecode=4:0x8300 \
		$(OUTPUT)/image.hdd
	
	# Install Limine BIOS bootloader
	./lib/limine/limine bios-install $(OUTPUT)/image.hdd
	
	# Copy data onto partitions
	echo Copy data onto partitions
	dd if=$(BUILDDIR)/image_bootfs.bin bs=512 seek=34 of=$(OUTPUT)/image.hdd conv=notrunc

.PHONY: clean-image
clean-image:

.PHONY: qemu
qemu: image
	$(QEMU) -s \
		-smp 2 -m 4G \
		-device pcie-root-port,bus=pci.0,id=pcisw0 \
		-device qemu-xhci,bus=pcisw0 -device usb-kbd \
		-device virtio-scsi-pci,id=scsi \
		-drive id=hd0,format=raw,file=$(OUTPUT)/image.hdd \
		-debugcon stdio -display none \
	| ../tools/address-filter.py -L -A $(CROSS_COMPILE)addr2line $(OUTPUT)/badger-os.elf
