# Project structure
<!-- SPDX-License-Identifier: MIT -->
## Index
- [How the build system works](#how-the-build-system-works)
- [Code style](#code-style)
- [General folder structure](#general-folder-structure)
- [Kernel folder structure](#project-structure)
- [Custom tools and scripts](#custom-tools-and-scripts)


# How the build system works
## Prerequisites
You can get a copy of the build tools **[from our buildroot fork](https://github.com/badgeteam/mch2025-badgeros-buildroot/releases/tag/badgeros-sdk-release-1.1)**.

If you prefer to install manually, get:
- CMake
- GNU make
- One of: `riscv32-unknown-linux-gnu-gcc`, `riscv32-linux-gnu-gcc`, `riscv64-unknown-linux-gnu-gcc` or `riscv64-linux-gnu-gcc`
- Picocom
- `esptool.py` (from ESP-IDF)
- OpenOCD (for debugging)

## Recommended software
Not required for build, but might be useful:
- clang-format
- clang-tidy

## Build process
BadgerOS uses a combination of CMake (for C source code), Makefiles (for issuing build commands) and Python scripts (to generate source code or files). When you go to the root of the project and type `make build`, the following happens:
1. The [system applications](../files/init/) are built
2. The non-application files [are generated](../files/Makefile)
3. The [filesystem image](../kernel/Makefile#L18) is generated from these
4. The [kernel](../kernel/) is built using [CMake](../kernel/CMakeLists.txt)
5. The output files for the kernel are copied to kernel/firmware.

## Convenience functions
For your convenience, there are a couple other functions than just building the project, including:
- `make flash` (requires esptool.py from [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/#installation)), for flashing the kernel to your hardware
- `make monitor` (requires picocom), for opening the serial monitor with an address to line filter
- `make clang-tidy-check` (requires clang-tidy), for running the clang-tidy pull request check locally
- `make clang-format-check` (requires clang-format), for running the clang-format pull request check locally


# Code style
BagderOS uses clang-format 18.0 or later to keep code style consistent.
When you create a pull request, a code formatting check will run.
If it fails, please make sure to run clang-format yourself, format on save in your IDE of choice is recommended.


# General folder structure
The BadgerOS repository contains source code for the BadgerOS kernel, all official preinstalled applications (except sponsored ones) and the other files required to create a BadgerOS installation.
It can be broadly divided into four categories:
- [Kernel source code](#kernel-source-code)
- [Application source code](#application-source-code)
- [Shared source code](#shared-source-code)
- [Installation file structure](#installation-file-structure)

## Kernel source code
All source code unique to the BadgerOS kernel lives in the `kernel` folder. This [is further divided](#kernel-folder-structure) among [CPU-specific](../kernel/cpu), [platform-specific](../kernel/port/) and [generic kernel code](../kernel/).
The `kernel` folder also contains the scripts used to build just the kernel, like the [`kernel/Makefile`](../kernel/Makefile) and [`kernel/CMakeLists.txt`](../kernel/CMakeLists.txt).

## Application source code
Applications that are part of the operating system are also in this repository.
This includes [`/sbin/init`](../files/init/src/) (the [init program](https://en.wikipedia.org/wiki/Init)), other non-kernel system programs, settings, the application store / installer / hatchery, name tag applications and other utilities.

The application source code lives in [`files`](../files), then with a folder per system application.

## Shared source code
A small number of files contains information that is shared between both the kernel and the applications. These files, named "shared source" or "common source", live in the [`common`](../common) directory. Most of this pertains to the interface between the kernel and applications: system calls and APIs.

## Installation file structure
One of the final products is an image that contains all the files required for a normal BadgerOS installation. These files are generated, currently by [a Makefile](../files/Makefile), and are output into the `files/root` folder. These can, along with the kernel, be used to create an image to be installed onto hardware.


# Kernel folder structure
There are three levels of abstraction: CPU, platform and kernel.
The CPU level contains divers for CPU-specific logic like interrupt handling,
the platform level contains drivers for hardware inside the CPU and
the kernel level contains the business logic that allows applications to run.

The folder structure reflects this:
| path           | purpose
| :------------- | :------
| src            | Kernel-implemented source
| include        | Kernel-declared includes
| cpu/*/src      | CPU-implemented source
| cpu/*/include  | CPU-declared include
| port/*/src     | Platform-implemented source
| port/*/include | Platform-declared include

In the future, there will be more details on the structure these folders will have internally but for now it's flat.

## The source folders
This applies to `src`, `cpu/*/src` and `port/*/src`.

The source folders contain exclusively C files and files to embed into the kernel verbatim.
If you're not sure where to put a new function, consider:
- Does the function depend on the target platform?
    - If so, it should be in `port/*/src`
- Does the function depend on the CPU, but not the target platform?
    - If so, it should be in `cpu/*/src`
- Can the function be described in a platform- and CPU-independent way?
    - If so, it should be in `src`

## The include folders
This applies to `include`, `cpu/*/include` and `port/*/include`.

The include folders contain exclusively C header files.
If you're not sure where to put a new definition, consider:
- Does the value of a it or signature of a function depend on the target platform?
    - If so, it should be in `port/*/include/*`
- Does the value of a it or signature of a function depend on the CPU, but not the target platform?
    - If so, it should be in `cpu/*/include/*`
- Can the it be described in a platform- and CPU-independent way?
    - If so, it should be in `include`

_Note: The value of `*` may change, but is always the same in a path: `port/abc/include/abc` is valid, but `port/one/include/another` is not._


# Custom tools and scripts
BadgerOS uses a couple custom tools during the build process for converting data. Some of these tools can also be used from the command line.

## address-filter.py
`address-filter.py` is used to look for addresses in the serial monitor and, if found, prints the linenumber associated with the address.

## bin2c.py
`bin2c.py` is a simple tool used to convert the contents of a file to a C file containing a `uint8_t` array with the same value. It can be used to load constants from a non-code file without having to store it in the BagderOS installation's filesystem.

## pack-image.py
`pack-image.py` is a required step of making binaries that can be loaded by Espressif's bootloader. It reads the image header, calculates and appends a checksum, which is checked by the bootloader before it hands over control to BadgerOS. This tool is used in the build process to create the final executable that can be flashed onto an ESP32.

## ramfs-gen.py
`ramfs-gen.py` generates a C file that represents a folder structure. It is intended to generate the contents of a RAM filesystem, though it can also be used for any other type of media. This tool is used before building the kernel, so a temporary filesystem can be stored in RAM before drivers for nonvolatile media exist.
