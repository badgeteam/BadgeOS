
// SPDX-License-Identifier: MIT

#pragma once

#include "cpu/process/types.h"

// Memory management cache.
// The ESP32-C6 only has support for PMPs.
typedef proc_riscv_pmp_t proc_mpu_t;
