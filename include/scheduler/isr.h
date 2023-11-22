
// SPDX-License-Identifier: MIT

#pragma once

#include "attributes.h"
#include "badge_err.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



// Requests the scheduler to prepare a switch from inside an interrupt routine.
void sched_request_switch_from_isr();
