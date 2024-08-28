
// SPDX-License-Identifier: MIT

#pragma once

// Requests the scheduler to prepare a switch from inside an interrupt routine.
void sched_request_switch_from_isr();
