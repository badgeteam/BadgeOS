# Project requirements
## Philosophy
BadgerOS aims to be the new stable target for Badge.Team badges and their applications.
To be a stable target means things like generic interfaces and stable APIs.
It should also have a nice, responsive user interface, which helps with appealing to the users.

This original philosophy created a set of requirements:
- The application's environment should be as standard as possible
    - Applications have access to the C standard library
    - Applications running on the same CPU architecture on different badges shouldn't need to be recompiled.
    - Applications should have traditional memory protection
- The kernel should not be architecture-specific
    - CPU architectures need to be abstracted
    - SoCs architectures need to be abstracted
- The user interface should be intuitive

This document expands on all of these requirements as a part of forming a project plan for BadgerOS.


## Index
- [Kernel requirements](#kernel-requirements)
    - [CPU low-level](#cpu-low-level)
    - [Low-level drivers](#low-level-drivers)
    - [High-level drivers](#high-level-drivers)
    - [Kernel infrastructure](#kernel-infrastructure)
    - [Application binary interface](#application-binary-interface)
- [System application requirements](#system-application-requirements)
    - [Application list](#application-list)
    - [Settings](#settings)
    - [App store](#app-store)
    - [Name tag](#name-tag)
    - [Text editor](#text-editor)
    - [File browser](#file-browser)
    - [Simple application IDE](#simple-application-ide)
    - [Event schedule / calendar](#event-schedule--calendar)
    - [Image viewer](#image-viewer)



# Kernel requirements
These requirements are for the kernel and the services it provides.

## CPU low-level
The following CPU-related features must be supported:
- ☐ Support for dynamic interrupt allocation
- ☐ Support for 1-8 CPUs
- ☐ Support for heterogeneous CPU systems


## Low-level drivers
The ESP32-P4 has GPIOs, which also support a subset of the communications interfaces.
The GPIO driver must support the following:
- ☐ Digital I/O
    - ☑ Direction control
    - ☑ Pull resistor control
    - ☐ Open drain support
- ☐ PWM driver
    - ☐ Configurable frequency
The GPIO driver should support the following:
- ☐ ADC driver
- ☐ DAC driver

The ESP32-P4 has many hardware communications interfaces.
The following interfaces must be supported:
- ☐ I²C master drivers
    - ☐ Configurable speed
    - ☐ Configurable pins
    - ☐ Interrupt capable
    - ☐ Low-power I²C
    - ☐ High-power I²C
    - ☐ I³C
- ☐ SPI master drivers
    - ☐ Configurable speed
    - ☐ Configurable pins
    - ☐ Interrupt capable
    - ☐ DMA capable
- ☐ UART drivers
    - ☐ Configurable speed
    - ☐ Configurable pins
    - ☐ Interrupt capable
    - ☐ DMA capable
- ☐ I²S drivers
    - ☐ Configurable speed
    - ☐ Configurable pins
    - ☐ Configurable sample type
    - ☐ Interrupt capable
    - ☐ DMA capable
- ☐ SDIO drivers
    - ☐ Configurable speed
    - ☐ Configurable pins
    - ☐ Interrupt capable
    - ☐ DMA capable
- ☐ MIPI DSI drivers
<!-- ☐ TODO: Ask Renze about hardware-imposed MIPI DSI driver requirements -->
The following interfaces should be supported:
- ☐ USB host drivers
    - ☐ Keyboard
    - ☐ Mouse
    - ☐ Mass storage device
- ☐ Ethernet drivers

The ESP32-P4 also has some image processing hardware.
The following image processing should be supported:
- ☐ Pixel Processing Accelerator (rotate / scale unit)
- ☐ JPEG encoder
- ☐ JPEG decoder


## High-level drivers
There must be high-level drivers for:
- ☐ I²C CH32V003 comms
    - ☐ Keyboard
    - ☐ I/O expander
- ☐ SDIO ESP32-C6 comms
    - ☐ Networking
    - ☐ I/O expander


## Kernel infrastructure
TODO: Infrastructure that the kernel itself depends on.

## Application binary interface
The ABI must support:
- ☑ File I/O
- ☐ Multithreading and thread management
- ☐ Process spawning and management
- ☐ Badge-style interface
    - ☐ Keyboard input
    - ☐ Gamepad input
    - ☐ One or more displays
- ☐ Networking
The ABI should support:
- ☐ Rotate / scale unit, if present
- ☐ Hardware JPEG codec, if present



# System application requirements
These requirements are for the preinstalled applications integral to the operating system, like settings.

## Application list
The following applications must exist:
- ☐ Settings
- ☐ App store (name TBD)
- ☐ Name tag
- ☐ Text editor
- ☐ File browser

The following applications should exist
- ☐ Simple application IDE
- ☐ Event schedule / calendar
- ☐ Image viewer

## Settings
The settings must include:
- ☐ Name tag adjustment
- ☐ Multiple WiFi network management
- ☐ System updates
The settings should include:
- ☐ OTA server address
- ☐ App store server address

## App store

## Name tag
The name tag should support:
- ☐ Images
- ☐ Font selection
- ☐ Color selection
- ☐ Border selection

## Text editor

## File browser

## Simple application IDE

## Event schedule / calendar

## Image viewer
The image viewer, if present, should support:
- ☐ JPEG images
- ☐ PNG images
