
# SPDX-License-Identifier: MIT

MAKEFLAGS         += --silent
IDF_PATH          ?= $(shell pwd)/../esp-idf
SHELL             := /usr/bin/env bash
PORT              ?= $(shell find /dev/ -name ttyUSB* -or -name ttyACM* | head -1)
OUTPUT            ?= "$(shell pwd)/firmware"
BUILDDIR          ?= "build"
BADGER_RAMFS_ROOT ?= ../files/root

.PHONY: all clean-tools clean configure build flash monitor test clang-format-check clang-tidy-check openocd gdb

all: build flash monitor

configure:
	mkdir -p "$(BUILDDIR)"
	../tools/ramfs-gen.py '$(BADGER_RAMFS_ROOT)' build/fs_root.c init_ramfs
	cmake -B "$(BUILDDIR)" "-DCMAKE_BUILD_TYPE=$${CMAKE_BUILD_TYPE:-Debug}"

build: configure
	cmake --build "$(BUILDDIR)"
	cmake --install "$(BUILDDIR)" --prefix "$(OUTPUT)"

clang-format-check: configure
	echo "clang-format check the following files:"
	jq -r '.[].file' build/compile_commands.json | grep '\.[ch]$$'
	echo "analysis results:"
	clang-format --dry-run $(shell jq -r '.[].file' build/compile_commands.json | grep '\.[ch]$$')

clang-tidy-check: configure
	echo "clang-tidy check the following files:"
	jq -r '.[].file' build/compile_commands.json | grep '\.[ch]$$'
	echo "analysis results:"
	clang-tidy -p build $(shell jq -r '.[].file' build/compile_commands.json | grep '\.[ch]$$') --warnings-as-errors="*"

openocd:
	openocd -c 'set ESP_RTOS "none"' -f board/esp32c6-builtin.cfg

gdb:
	riscv32-unknown-elf-gdb -x port/esp32c6/gdbinit build/badger-os.elf

clean-all: clean
clean:
	rm -rf "$(BUILDDIR)"

flash: build
	esptool.py -b 921600 --port "$(PORT)" \
		write_flash --flash_mode dio --flash_freq 80m --flash_size 2MB \
		0x0 port/esp32c6/bootloader.bin \
		0x10000 "$(OUTPUT)/badger-os.bin" \
		0x8000 port/esp32c6/partition-table.bin

monitor:
	echo -e "\033[1mType ^A^X to exit.\033[0m"
	picocom -q -b 115200 $(PORT) | ../tools/address-filter.py "$(OUTPUT)/badger-os.elf"; echo -e '\033[0m'
