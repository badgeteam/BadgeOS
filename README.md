# BadgerOS

BadgerOS is the operating system currently in development for the upcoming [WHY2025](https://why2025.org/) badge.
The goal is the allow future badge users to get both the performance that native apps can afford as well as the portability made possible by this OS.

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
All of:
- `git`
- `build-essential`
- `cmake`
- A RISC-V toolchain, one of:
    - [BadgerOS buildroot](https://github.com/badgeteam/mch2025-badgeros-buildroot), preferably riscv64
    - `gcc-riscv64-linux-gnu` (ubuntu) / `riscv64-gnu-toolchain-glibc-bin` (arch)
- `python3`
- `picocom`
## For RISC-V PC port
If you don't know what this is, you don't need this. All of:
- `mtools`
- `swig`
- `gptfdisk`



# Build system
The build system is based on Makefiles and CMake.
The following commands can be used to perform relevant actions:

To select target platform, choose one of:
- `make config` (manual configuration)
- `make hh24_defconfig` (HackerHotel 2024 badge)
- `make why2025_defconfig` (WHY2025 badge)
- `make unmatched_defconfig` (RISC-V PC port)

To build: `make build`

To remove build files: `make clean`

To flash to an ESP: `make flash`

To open picocom: `make monitor`

To build, flash and open picocom: `make` or `make all`

To check code style: `make clang-format-check` (code formatting) and `make clang-tidy-check` (programming guidelines)

Build artifacts will be put into the `kernel/firmware` folder once the project was successfully built.
