
# SPDX-License-Identifier: MIT

IDF_PATH ?= $(shell pwd)/../esp-idf
SHELL    := /usr/bin/env bash
PORT     ?= $(shell ls /dev/ttyUSB0 2>/dev/null || echo /dev/ttyACM0)
OUTPUT   ?= "$(shell pwd)/firmware"
BUILDDIR ?= "build"

.PHONY: all clean-tools clean build flash monitor test

all: build flash monitor

clean-tools:
	rm -rf tools
	@make -s -C elftool clean

build:
	@mkdir -p "$(BUILDDIR)"
	@cmake -B "$(BUILDDIR)"
	@cmake --build "$(BUILDDIR)"
	@cmake --install "$(BUILDDIR)" --prefix "$(OUTPUT)"

test:
	@mkdir -p "$(BUILDDIR)"
	@echo "Testing list.c…"
	@cc -g -I include -o "./$(BUILDDIR)/list-test" test/list.c src/list.c
	@./build/list-test

clean:
	rm -rf "$(BUILDDIR)"

flash: build
	esptool.py -b 921600 \
		write_flash --flash_mode dio --flash_freq 80m --flash_size 2MB \
		0x0 bin/bootloader.bin \
		0x10000 "$(OUTPUT)/badger-os.bin" \
		0x8000 bin/partition-table.bin

monitor:
	@echo -e "\033[1mType ^A^X to exit.\033[0m"
	@picocom -q -b 115200 $(PORT) | ./tools/address-filter.py "$(OUTPUT)/badger-os.elf"
