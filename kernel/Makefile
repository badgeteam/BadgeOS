
# SPDX-License-Identifier: MIT

CONFIG_PATH       ?= ../.config/config.mk
include $(CONFIG_PATH)

MAKEFLAGS         += --silent
SHELL             := /usr/bin/env bash
OUTPUT            ?= $(shell pwd)/firmware
BUILDDIR          ?= build
BADGER_RAMFS_ROOT ?= ../files/root
CROSS_COMPILE      = $(CONFIG_TC_PREFIX)
GDB               ?= $(CROSS_COMPILE)gdb
CLANG_FORMAT      ?= clang-format-18
CLANG_TIDY        ?= clang-tidy-18

include port/$(CONFIG_TARGET)/port.mk

.PHONY: cmake-configure
cmake-configure:  $(BUILDDIR)/fs_root.c
	mkdir -p '$(BUILDDIR)'
	cmake -B '$(BUILDDIR)' "-DCMAKE_BUILD_TYPE=$${CMAKE_BUILD_TYPE:-Debug}"

$(BUILDDIR)/fs_root.c: $(shell find '$(BADGER_RAMFS_ROOT)' 2>/dev/null)
	mkdir -p '$(BUILDDIR)'
	../tools/ramfs-gen.py '$(BADGER_RAMFS_ROOT)' '$(BUILDDIR)/fs_root.c' init_ramfs

.PHONY: _on_config
_on_config: _port_on_config
	git submodule update --init lib/kbelf

.PHONY: build
build: cmake-configure
	cmake --build '$(BUILDDIR)'
	cmake --install '$(BUILDDIR)' --prefix '$(OUTPUT)'

.PHONY: clean
clean:
	rm -rf "$(BUILDDIR)"

.PHONY: clang-format-check
clang-format-check: cmake-configure
	echo "clang-format check the following files:"
	jq -r '.[].file' build/compile_commands.json | grep '\.[ch]$$'
	echo "analysis results:"
	$(CLANG_FORMAT) --dry-run $(shell jq -r '.[].file' build/compile_commands.json | grep '\.[ch]$$')

.PHONY: clang-tidy-check
clang-tidy-check: cmake-configure
	echo "clang-tidy check the following files:"
	jq -r '.[].file' build/compile_commands.json | grep '\.[ch]$$'
	echo "analysis results:"
	$(CLANG_TIDY) -p build $(shell jq -r '.[].file' build/compile_commands.json | grep '\.[ch]$$') \
		--checks=-*widening*multiplication*,-*insecureAPI* \
		--warnings-as-errors="*"

.PHONY: gdb
gdb:
	$(GDB) -x port/$(CONFIG_TARGET)/gdbinit build/badger-os.elf
