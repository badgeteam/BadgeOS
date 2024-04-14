# Hardware abstraction layer
The hardware abstraction layer, or HAL for short, provides a generic interface to various hardware functions, including GPIO, I²C, SPI, FLASH, etc.
Other components can then use the HAL to perform the same operations on different hardware without the need for porting them.
Take filesystems for example, where one type of filesystem can be stored on almost any kind of media without the need for explicit support from the filesystem driver.

Currently responsible: Julian (RobotMan2412).

## Index
- [Scope](#scope)
- [Dependents](#dependents)
- [Dependencies](#dependencies)
- [Data types](#data-types)
- [API](#api)


# Scope
The HAL needs to provide an abstract interface that can perform all actions that BagderOS needs it to. There will abstractions for:
- GPIO
    - Digital I/O
    - PWM outputs
    - ADC
    - DAC
- I²C
- UART
- SPI
- I²S
- FLASH

## GPIO scope
The GPIOs need to be able to function as digital I/Os and all alternate functions provided by the hardware for which there is a driver (I²C, UART, SPI, etc.). The GPIO API also needs to be able to tell whether a pin is currently in use by one of such alternate functions. At least one GPIO input pin needs to support interrupts.

## I²C scope
The I²C HAL needs to support master, slave, 10-bit addressing and interrupts.

## UART scope
The UART HAL needs to support baudrate configuration and interrupts.

## SPI scope
The SPI HAL needs to support DMA, one-wire SPI, two-wire SPI and interrupts.

## I²S scope
TODO: Depends on sound architecture

## FLASH scope
The FLASH HAL needs to support FLASH MMUs (if present), reading, erasing and writing.


# Dependents
## [Filesystems](./filesystems.md)
The block device drivers from the filesystems component depend on the HAL to abstract communications with physical storage media.


# Dependencies
The HAL does not depend on other components.


# Data types
TODO.


# API
TODO.
