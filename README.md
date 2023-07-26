# BadgerOS

BadgerOS is the operating system currently in development for the upcoming MCH2025<sup>(1)</sup> badge.
The goal is the allow future badge users to get both the performance that native apps can afford as well as the portability made possible by this OS.

_(1) MCH2025 is a preliminary name, MCH2025 is an event that will be organised by IFCAT in 2025._

# Contributing

Make sure to check out the [style guide](docs/styleguide.md) so your code can fit right in.
If you're unsure where to put your code, look into the [project structure](docs/project-structure.md).

# Documents

[OS specification doc](https://docs.google.com/document/d/1qgpeLhjLZvecd4yATPb-9B1IthdMw18idM6reSSEXS8/edit?usp=sharing)

[OS notes doc](https://docs.google.com/document/d/1y2fYwdAGRNWYJmHFczQzzb913dGTHXzgqrCjLKwj9k8/edit?usp=sharing)

# Prerequisites

To build BadgerOS:

- `build-essential`
- `cmake`
- `riscv32-unknown-elf-gcc` ([RISC-V collab toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain))
- `python3`

To flash to an ESP:

- ESP-IDF ([Espressif toolchain](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/#installation))

Optional recommends:

- `picocom`

# Build system

The build system is based on Makefiles and CMake.
The following commands can be used to perform relevant actions:

To build: `make build`

To remove build files: `make clean`

To flash to an ESP: `export IDF_PATH=<path to ESP-IDF>` (once) `make flash` (every time you flash)

To open picocom: `make monitor`

To build, flash and open picocom: `make` or `make all`

Build artifacts will be put into the `firmware` folder once the project was successfully built.
