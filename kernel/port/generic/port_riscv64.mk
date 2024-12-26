
OPENSBI        = $(shell pwd)/lib/opensbi/build/platform/generic/firmware/fw_dynamic.bin
QEMU          ?= qemu-system-riscv64

.PHONY: _cpu_on_config
_cpu_on_config:
	git submodule update --init lib/opensbi
	git submodule update --init lib/u-boot


$(OPENSBI):
	echo Build OpenSBI
	make -C lib/opensbi PLATFORM=generic CROSS_COMPILE=$(CROSS_COMPILE)

lib/u-boot/u-boot.itb:
	echo Configure U-boot
	make -C lib/u-boot sifive_unmatched_defconfig CROSS_COMPILE=$(CROSS_COMPILE)
	echo Build U-boot
	make -C lib/u-boot OPENSBI=$(OPENSBI) CROSS_COMPILE=$(CROSS_COMPILE)
	
$(OUTPUT)/image.hdd: $(BUILDDIR)/cache/OVMF.fd $(OUTPUT)/badger-os.elf port/generic/limine.cfg $(OPENSBI) lib/u-boot/u-boot.itb
	# Create boot filesystem
	echo Create EFI filesystem
	rm -rf $(BUILDDIR)/image.dir
	mkdir -p $(BUILDDIR)/image.dir/EFI/BOOT/
	mkdir -p $(BUILDDIR)/image.dir/boot/
	make -C lib/limine
	cp lib/limine/BOOTRISCV64.EFI $(BUILDDIR)/image.dir/EFI/BOOT/
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
	# 1M (SPL), 1007K (U-boot), 4M /boot, remainder /root
	echo pre sgdisk
	sgdisk -a 1 \
		--new=1:34:2081    --change-name=1:spl   --typecode=1:5B193300-FC78-40CD-8002-E86C45580B47 \
		--new=2:2082:4095  --change-name=2:uboot --typecode=2:2E54B353-1271-4842-806F-E436D6AF6985 \
		--new=3:4096:12287 --change-name=3:boot  --typecode=3:0x0700 \
		--new=4:12288:-0   --change-name=4:root  --typecode=4:0x8300 \
		$(OUTPUT)/image.hdd
	
	# Copy data onto partitions
	echo Copy data onto partitions
	dd if=lib/u-boot/spl/u-boot-spl.bin bs=512 seek=34   of=$(OUTPUT)/image.hdd conv=notrunc
	dd if=lib/u-boot/u-boot.itb         bs=512 seek=2082 of=$(OUTPUT)/image.hdd conv=notrunc
	dd if=$(BUILDDIR)/image_bootfs.bin  bs=512 seek=4096 of=$(OUTPUT)/image.hdd conv=notrunc

$(BUILDDIR)/cache/OVMF.fd:
	mkdir -p $(BUILDDIR)/cache
	cd $(BUILDDIR)/cache && curl -o OVMF.fd https://retrage.github.io/edk2-nightly/bin/RELEASERISCV64_VIRT_CODE.fd && dd if=/dev/zero of=OVMF.fd bs=1 count=0 seek=33554432

.PHONY: clean-image
clean-image:
	$(MAKE) -C lib/limine clean
	$(MAKE) -C lib/opensbi clean
	$(MAKE) -C lib/u-boot clean

.PHONY: qemu
qemu: $(BUILDDIR)/cache/OVMF.fd image
	$(QEMU) -s \
		-M virt,acpi=off -cpu rv64,sv48=false -smp 2 -m 4G \
		-device pcie-root-port,bus=pcie.0,id=pcisw0 \
		-device qemu-xhci,bus=pcisw0 -device usb-kbd \
		-drive if=pflash,unit=0,format=raw,file=$(BUILDDIR)/cache/OVMF.fd \
		-device virtio-scsi-pci,id=scsi \
		-drive id=hd0,format=raw,file=$(OUTPUT)/image.hdd \
		-serial mon:stdio -nographic \
	| ../tools/address-filter.py -L -A $(CROSS_COMPILE)addr2line $(OUTPUT)/badger-os.elf
