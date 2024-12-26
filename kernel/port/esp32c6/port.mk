
include port/esp_common/port.mk

PORT              ?= /dev/ttyACM0

.PHONY: openocd
openocd:
	openocd -c 'set ESP_RTOS "none"' -f board/esp32c6-builtin.cfg

.PHONY: flash
flash: build
	../.venv/bin/python -m esptool -b 921600 --port "$(PORT)" \
		write_flash --flash_mode dio --flash_freq 80m --flash_size 2MB \
		0x0 port/esp32c6/bootloader.bin \
		0x10000 "$(OUTPUT)/badger-os.bin" \
		0x8000 port/esp32c6/partition-table.bin

.PHONY: monitor
monitor:
	echo -e "\033[1mType ^A^X to exit.\033[0m"
	picocom -q -b 115200 '$(PORT)' \
	| ../tools/address-filter.py -A $(CROSS_COMPILE)addr2line '$(OUTPUT)/badger-os.elf'; echo -e '\033[0m'
