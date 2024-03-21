
// SPDX-License-Identifier: MIT

#pragma once

#include "badge_err.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Returns the amount of SPI peripherals present.
// Cannot produce an error.
#define spi_count() (1)

void spi_master_init(badge_err_t *ec, int spi_num, int sclk_pin, int mosi_pin, int miso_pin, int ss_pin);
void spi_write_buffer(badge_err_t *ec, uint8_t *data, int len);
