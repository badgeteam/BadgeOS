# BadgerOS

BadgerOS is the operating system currently in development for the upcoming MCH2025<sup>(1)</sup> badge.
The goal is the allow future badge users to get both the performance that native apps can afford as well as the portability made possible by this OS.

_(1) MCH2025 is a preliminary name, MCH2025 is an event that will be organised by IFCAT in 2025._



# Contributing

We are an open-source project, so we can always use more hands!
If you're looking for things to help with, look for the [issues with the PoC (Proof of Concept) milestone](https://github.com/badgeteam/BadgerOS/issues/33).
**We are currently using the ESP32-C6 for development.**

Make sure to check out the [style guide](docs/styleguide.md) so your code can fit right in.
If you're unsure where to put your code, look into the [project structure](docs/project-structure.md).

If you're not sure what to do but want to help, message us:
- [robotman2412](https://t.me/robotman2412) on telegram
- [Badge.team](https://t.me/+StQpEWyhnb96Y88p) telegram group



# Prerequisites

To build BadgerOS:

- `build-essential`
- `cmake`
- `riscv32-unknown-elf-gcc` ([RISC-V collab toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain))
- `python3`

To flash to an ESP:

- `esptool.py` from [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/#installation)

Optional recommends:

- `picocom`



# Build system

The build system is based on Makefiles and CMake.
The following commands can be used to perform relevant actions:

To build: `make build`

To remove build files: `make clean`

To flash to an ESP: `source <path to ESP-IDF>/export.sh` (once) `make flash` (every time you flash)

To open picocom: `make monitor`

To build, flash and open picocom: `make` or `make all`

To check cody style: `make clang-format-check` (code formatting) and `make clang-tidy-check` (programming guidelines)

Build artifacts will be put into the `firmware` folder once the project was successfully built.
