
// SPDX-License-Identifier: MIT

#include "port/port.h"

#include "port/pmu_init.h"


// Early hardware initialization.
void port_early_init() {
    // Initialise PMU.
    pmu_init();
}

// Full hardware initialization.
void port_init() {
}
