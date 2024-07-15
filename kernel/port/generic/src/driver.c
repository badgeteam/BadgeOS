
// SPDX-License-Identifier: MIT

#include "driver.h"

extern driver_t const pcie_driver;
extern driver_t const pcie_fu740_driver;



driver_t const *const drivers[] = {
    &pcie_driver,
    &pcie_fu740_driver,
};
size_t const drivers_len = sizeof(drivers) / sizeof(driver_t const *);
