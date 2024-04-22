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
- `git`
- `build-essential`
- `cmake`
- `gcc-riscv64-linux-gnu`
- `python3`
- `picocom`



# Build system
The build system is based on Makefiles and CMake.
The following commands can be used to perform relevant actions:

To select target chip, choose one of:
- `export BADGEROS_PORT=esp32p4`
- `export BADGEROS_PORT=esp32c6` (default)

Before building, first run: `make prepare`

To build: `make build`

To remove build files: `make clean`

To flash to an ESP: `make flash`

To open picocom: `make monitor`

To build, flash and open picocom: `make` or `make all`

To check code style: `make clang-format-check` (code formatting) and `make clang-tidy-check` (programming guidelines)

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
