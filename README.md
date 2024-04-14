# BadgerOS

BadgerOS is the operating system currently in development for the upcoming MCH2025<sup>(1)</sup> badge.
The goal is the allow future badge users to get both the performance that native apps can afford as well as the portability made possible by this OS.

_(1) MCH2025 is a preliminary name, MCH2025 is an event that will be organised by IFCAT in 2025._

## Index
- [Contributing](#contributing)
- [Prerequisites](#prerequisites)
- [Build system](#build-system)
- [Credits](#credits)



# [Documentation](./docs/README.md)



# Contributing
We are an open-source project, so we can always use more hands!
If you're new to this project and want to help, message us:
- [RobotMan2412](https://t.me/robotman2412) on telegram
- [Badge.team](https://t.me/+StQpEWyhnb96Y88p) telegram group

After that, see [Project structure](./docs/project_structure.md) for reference about how this project works.



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

Build artifacts will be put into the `kernel/firmware` folder once the project was successfully built.



# Credits
Contributors
| Nickname       | Name                  | Components
| :------------- | :-------------------- | :---------
| RobotMan2412   | Julian Scheffers      | CPU low-level, peripheral low-level
| TMM2K          | Hein-Pieter van Braam | Memory management
| Quantumcatgirl | Joyce Ng Rui Lin      | Filesystems

Ex-contributors
| Nickname     | Name                  | Components
| :----------- | :-------------------- | :---------
| Ronaksm      | Ronak S. Manani       | SPI research
| ikskuh       | Felix queißner        | Continuous integration, temporary scheduler
