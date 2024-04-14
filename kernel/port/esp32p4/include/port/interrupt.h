
// SPDX-License-Identifier: MIT

#pragma once

#include "cpu/interrupt.h"
#include "soc/interrupts.h"

#define EXT_IRQ_COUNT ETS_MAX_INTR_SOURCE



// Disable interrupts by mask.
void esprv_int_intc_enable(uint32_t enable_mask);

// Enable interrupts by mask.
void esprv_int_intc_disable(uint32_t disable_mask);

// Select edge-triggered or level-triggered interrutps.
void esprv_int_intc_set_type(int intr_num, bool edge_trigger);

// Returns whether an interrupt is currently edge-triggered.
bool esprv_int_intc_get_type(int intr_num);

// Set interrupt priority (1-7).
void esprv_int_intc_set_priority(int rv_int_num, int priority);

// Get interrupt priority (1-7).
int esprv_int_intc_get_priority(int rv_int_num);

// Set interrupt priority threshold (0-7).
// Interrupts with lower levels are masked.
void esprv_int_intc_set_threshold(int priority_threshold);

// TODO: Determine what Espressif means by this:
/**
 * @brief Get interrupt unmask
 * @param none
 * @return uint32_t interrupt unmask
 */
uint32_t esprv_get_interrupt_unmask(void);

// TODO: Determine what it means for an individual interrupt to be vectored:
/**
 * @brief Check if the given interrupt is hardware vectored
 *
 * @param rv_int_num Interrupt number
 *
 * @return true if the interrupt is vectored, false if it is not.
 */
bool esprv_int_intc_is_vectored(int rv_int_num);

// TODO: Determine what it means for an individual interrupt to be vectored:
/**
 * @brief Set interrupt vectored
 *
 * Configure the given interrupt number to hardware vectored or non-vectored.
 *
 * @param rv_int_num Interrupt number
 * @param vectored True to set it to vectored, false to set it to non-vectored
 */
void esprv_int_intc_set_vectored(int rv_int_num, bool vectored);
