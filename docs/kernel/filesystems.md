# Filesystems
The filesystem component is reponsible for managing all the mounted filesystems. It abstracts the filesystem details away, leaving only a generic interface that applies to all types of filesystem.

Currently responsible: Joyce (Quantumcatgirl).

## Index
- [Scope](#scope)
- [Dependents](#dependents)
- [Dependencies](#dependencies)
- [Data types](#data-types)
- [API](#api)


# Scope
BadgerOS will initially support both a RAM filesystem, also called RAMFS, and a FAT filesystem. It supports FAT before EXT2/3/4 as a compromise to allow mounting the filesystem from Windows or MacOS without the need for additional software. In the future, EXT2/3/4 *may* be supported, but there are higher priorities than that.

Most filesystems will be stored on nonvolatile physical media. This physical media is abstracted from the filesystem implementation by the block device API. The block device API is similar to the HAL in purpose, but not dependant on the target platform.

## API scope
- API support for UNIX permissions
- API support for reading and writing
- API support for sybolic links (TODO)
- API support for hard links (TODO)

## FAT filesystem scope
- Support for FAT12, FAT16 and FAT32 (TODO)
- Support for [long filename](https://en.wikipedia.org/wiki/Long_filename) entries (TODO)
- Support for reading and writing (TODO)
- Support for formatting (TODO)

## RAM filesystem scope
- Support for UNIX permissions
- Support for reading and writing
- Support for device special files (TODO)
- Support for symbolic links (TODO)
- Support for hard links

## Block device scope
- Support for reading, writing and erasing blocks
- Support for block size and total size detection


# Dependents
## [Process management](./process.md)
The process management component uses filesystems to load executable files to start programs.


# Dependencies
## [Hardware abstraction layer](./hal.md)
The block device part of the filesystems component uses the HAL to abstract the implementation details of communication with the storage media.


# Data types
TODO.


# API
Because of the multiple abstractions present in the filesystems, the API is split into 5 parts:
- High-level filesystem API: `fs_*`
- Filesystem abstraction layer: `vfs_*`
- Filesystem implementations: `vfs_fat_*`, `vfs_ramfs_*`, etc.
- Block device abstraction layer: `blkdev_*`
- Block device implementations: `blkdev_ram_*`, etc.

## High-level filesystem API
The high-level filesystem API translates traditional file operations into calls to the filesystem abstraction layer that more directly represent operations on the filesystem.

The high-level filesystem API also converts multiple file handles per logical file into one single handle so individual filesystem implementations only need to manage one handle per file / directory.

## Filesystem abstraction layer
The filesystem abstraction layer defines which functions each filesystem needs to implement and forwards every function call directly to the implementation.

The filesystem abstraction layer and the filesystem implementation only manage one handle per file, so they only need to synchronize accesses between different files not symbolic or hard linked.

## Block device abstraction layer
The block device abstraction layer implements caches and some common utilities for accessing block devices. It calls the block device implementations for read, write and erase commands, which handle the uncached accesses.

The block device API does synchronize multiple accesses to the same partition, which is the taks of the filesystem implementation.
