
// SPDX-License-Identifier: MIT
// Port of esp_assert.h

#pragma once

#include "assertions.h"

#define assert assert_dev_drop

#define ESP_STATIC_ASSERT(...) _Static_assert(__VA_ARGS__)
