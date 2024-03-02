# Memory management
All the memory available to the system is handled centrally by the memory manager. The memory manager is responsible for accounting the available memory and distributing said memory between processes, kernel-side caches, kernel stacks and kernel data structures.
Memory management also includes the configuration of virtual memory or memory protection, whichever is applicable.

Currently responsible: Hein-Pieter van Braam (HP).

## Index
- [Scope](#scope)
- [Dependents](#dependents)
- [Dependencies](#dependencies)
- [Data types](#data-types)
- [API](#api)


# Scope
Memory management is in charge of allocating and protecting all the memory available to the system. Various systems in the kernel and applications can request memory for various purposes. The memory allocator decides how much memory at what addresses to assign to applications or the kernel. This information is then relayed to memory protection for the application or kernel respectively.


# Dependents
## [Process management](./process.md)
The process management component relays requests for memory from the processes to the memory management component.


# Dependencies
The memory allocator does not depend on other components.


# Data types
TODO.


# API
TODO.
