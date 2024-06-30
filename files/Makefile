
# SPDX-License-Identifier: MIT

MAKEFLAGS += --silent
SHELL    := /usr/bin/env bash
BUILDDIR ?= $(shell pwd)/build
OUTPUT   ?= $(shell pwd)/root

.PHONY: all clean build

all: build

build:
	mkdir -p '$(BUILDDIR)'
	mkdir -p '$(OUTPUT)'
	cmake -B '$(BUILDDIR)'
	cmake --build '$(BUILDDIR)'
	cmake --install '$(BUILDDIR)' --prefix '$(OUTPUT)'

clean:
	rm -rf '$(BUILDDIR)' '$(OUTPUT)'
