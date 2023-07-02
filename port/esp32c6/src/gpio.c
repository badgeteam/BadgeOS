
// SPDX-License-Identifier: MIT

#include "gpio.h"
#include "port/hardware.h"

// Function index for GPIO mode.
#define ESP32C6_IOMUX_FUNC_GPIO 1


// GPIO output register (Access: R/W/SC/WTC)
#define GPIO_OUT_REG					(GPIOMTX_BASE + 0x0004)
// GPIO output set register (Access: WT)
#define GPIO_OUT_W1TS_REG				(GPIOMTX_BASE + 0x0008)
// GPIO output clear register (Access: WT)
#define GPIO_OUT_W1TC_REG				(GPIOMTX_BASE + 0x000C)
// GPIO output enable register (Access: R/W/WTC)
#define GPIO_ENABLE_REG					(GPIOMTX_BASE + 0x0020)
// GPIO output enable set register (Access: WT)
#define GPIO_ENABLE_W1TS_REG			(GPIOMTX_BASE + 0x0024)
// GPIO output enable clear register (Access: WT)
#define GPIO_ENABLE_W1TC_REG			(GPIOMTX_BASE + 0x0028)
// Strapping pin register (Access: RO)
#define GPIO_STRAP_REG					(GPIOMTX_BASE + 0x0038)
// GPIO input register (Access: RO)
#define GPIO_IN_REG						(GPIOMTX_BASE + 0x003C)
// GPIO interrupt status register (Access: R/W/WTC)
#define GPIO_STATUS_REG					(GPIOMTX_BASE + 0x0044)
// GPIO interrupt status set register (Access: WT)
#define GPIO_STATUS_W1TS_REG			(GPIOMTX_BASE + 0x0048)
// GPIO interrupt status clear register (Access: WT)
#define GPIO_STATUS_W1TC_REG			(GPIOMTX_BASE + 0x004C)
// GPIO CPU interrupt status register (Access: RO)
#define GPIO_PCPU_INT_REG				(GPIOMTX_BASE + 0x005C)
// GPIO CPU non-maskable interrupt status register (Access: RO)
#define GPIO_PCPU_NMI_INT_REG			(GPIOMTX_BASE + 0x0060)
// GPIO interrupt source register (Access: RO)
#define GPIO_STATUS_NEXT_REG			(GPIOMTX_BASE + 0x014C)
// GPIO 0-30 configuration register (Access: R/W)
#define GPIO_PIN_N_REG(N)				(GPIOMTX_BASE + 0x0074 + 4 * (N))
// Configuration register for input signal 0-127 (Access: R/W)
#define GPIO_FUNC_N_IN_SEL_CFG_REG(N)	(GPIOMTX_BASE + 0x0154 + 4 * (N))
// Configuration register for GPIO 0-30 output (Access: varies)
#define GPIO_FUNC_N_OUT_SEL_CFG_REG(N)	(GPIOMTX_BASE + 0x0554 + 4 * (N))
// GPIO version register (Access: R/W)
#define GPIO_DATE_REG					(GPIOMTX_BASE + 0x06FC)
// GPIO clock gate register (Access: R/W)
#define GPIO_CLOCK_GATE_REG				(GPIOMTX_BASE + 0x062C)


// Clock output configuration register (Access: R/W)
#define IO_MUX_PIN_CTRL_REG				(IOMUX_BASE + 0x0000)
// IO MUX configuration register for GPIO 0-30 (Access: R/W)
#define IO_MUX_GPIO_N_REG(N)			(IOMUX_BASE + 0x0004 + 4 * (N))
// Version control register (Access: R/W)
#define IO_MUX_DATE_REG					(IOMUX_BASE + 0x00FC)

// Enable the output of GPIO N in sleep mode.
#define IO_MUX_GPIO_N_MCU_OE_BIT		0x0001
// Configures whether or not to enter sleep mode for GPIO N.
#define IO_MUX_GPIO_N_SLP_SEL_BIT		0x0002
// Configure whether or not to enable pull-down resistor of GPIO N in sleep mode.
#define IO_MUX_GPIO_N_MCU_WPD_BIT		0x0004
// Configures whether or not to enable pull-up resistor of GPIO N during sleep mode.
#define IO_MUX_GPIO_N_MCU_WPU_BIT		0x0008
// Configures whether or not to enable the input of GPIO N during sleep mode.
#define IO_MUX_GPIO_N_MCU_IE_BIT		0x0010
// Configures the drive strength of GPIO N during sleep mode.
#define IO_MUX_GPIO_N_MCU_DRV_MASK		0x0060
// Configures the drive strength of GPIO N during sleep mode (bit position).
#define IO_MUX_GPIO_N_MCU_DRV_POS		5
// Configures whether or not to enable pull-down resistor of GPIO N.
#define IO_MUX_GPIO_N_FUN_WPD_BIT		0x0080
// Configures whether or not enable pull-up resistor of GPIO N.
#define IO_MUX_GPIO_N_FUN_WPU_BIT		0x0100
// Configures whether or not to enable input of GPIO N.
#define IO_MUX_GPIO_N_FUN_IE_BIT		0x0200
// Configures the drive strength of GPIO N.
#define IO_MUX_GPIO_N_FUN_DRV_MASK		0x0c00
// Configures the drive strength of GPIO N (bit position).
#define IO_MUX_GPIO_N_FUN_DRV_POS		10
// Configures to select IO MUX function for this signal.
#define IO_MUX_GPIO_N_MCU_SEL_MASK		0x7000
// Configures to select IO MUX function for this signal (bit position).
#define IO_MUX_GPIO_N_MCU_SEL_POS		12
// Configures whether or not to enable filter for pin input signals.
#define IO_MUX_GPIO_N_FILTER_EN_BIT		0x8000


// Bitmask of GPIO pins used for peripherals.
// The owning peripheral must be deinitialised before the pins can be used for another purpose.
static uint32_t gpio_is_peripheral = 0;
// Bitmask of GPIO pins used for regular outputs.
// Mutually exclusive with `gpio_is_peripheral` bits.
static uint32_t gpio_is_output     = 0;


void io_mode(badge_err_t *err, int pin, io_mode_t mode) {
	// Check pin bounds.
	if (pin < 0 || pin > io_count()) {
		if (err) err->cause = ECAUSE_RANGE;
		if (err) err->location = ELOC_GPIO;
		return;
	}
	
	// Check pin capabilities.
	if (mode != IO_MODE_INPUT && mode != IO_MODE_OUTPUT) {
		if (err) err->cause = ECAUSE_UNSUPPORTED;
		if (err) err->location = ELOC_GPIO;
		return;
	}
	
	// Assert pin is configured as a GPIO.
	if ((gpio_is_peripheral >> pin) & 1) {
		if (err) err->cause = ECAUSE_INUSE;
		if (err) err->location = ELOC_GPIO;
		return;
	}
	
	// Configure IOMUX to GPIO.
	uint32_t mux = READ_REG(IO_MUX_GPIO_N_REG(pin));
	mux &= ~IO_MUX_GPIO_N_MCU_SEL_MASK;
	mux |= ESP32C6_IOMUX_FUNC_GPIO << IO_MUX_GPIO_N_MCU_SEL_POS;
	WRITE_REG(IO_MUX_GPIO_N_REG(pin), mux);
	
	// Configure FUNC_OUT_SEL to GPIO_OUT_REG.
	WRITE_REG(GPIO_FUNC_N_OUT_SEL_CFG_REG(pin), 128);
	
	// Configure output enable bit.
	uint32_t out = READ_REG(GPIO_ENABLE_REG);
	if (mode == IO_MODE_INPUT) {
		out &= ~(1 << pin);
		gpio_is_output &= ~(1 << pin);
	} else {
		out |= 1 << pin;
		gpio_is_output |= 1 << pin;
	}
	WRITE_REG(GPIO_ENABLE_REG, out);
	
	// Clear error status.
	if (err) err->cause = 0;
}

void io_pull(badge_err_t *err, int pin, io_pull_t pull) {
	// Check pin bounds.
	if (pin < 0 || pin > io_count()) {
		if (err) err->cause = ECAUSE_RANGE;
		if (err) err->location = ELOC_GPIO;
		return;
	}
	
	// Configure pull bits.
	uint32_t mux = READ_REG(IO_MUX_GPIO_N_REG(pin));
	if (pull == IO_PULL_UP) {
		mux |=  IO_MUX_GPIO_N_FUN_WPU_BIT;
		mux &= ~IO_MUX_GPIO_N_FUN_WPD_BIT;
	} else if (pull == IO_PULL_DOWN) {
		mux |=  IO_MUX_GPIO_N_FUN_WPD_BIT;
		mux &= ~IO_MUX_GPIO_N_FUN_WPU_BIT;
	} else if (pull == IO_PULL_NONE) {
		mux &= ~IO_MUX_GPIO_N_FUN_WPU_BIT;
		mux &= ~IO_MUX_GPIO_N_FUN_WPD_BIT;
	} else {
		if (err) err->cause = ECAUSE_PARAM;
		if (err) err->location = ELOC_GPIO;
		return;
	}
	
	// Clear error status.
	if (err) err->cause = 0;
}

void io_write(badge_err_t *err, int pin, bool level) {
	// Check pin bounds.
	if (pin < 0 || pin > io_count()) {
		if (err) err->cause = ECAUSE_RANGE;
		if (err) err->location = ELOC_GPIO;
		return;
	}
	
	// Assert pin is an output.
	if (!((gpio_is_output >> pin) & 1)) {
		if (err) err->cause = ECAUSE_NOTCONFIG;
		if (err) err->location = ELOC_GPIO;
		return;
	}
	
	// Write to the GPIO output register.
	if (level) WRITE_REG(GPIO_OUT_W1TS_REG, 1 << pin);
	else WRITE_REG(GPIO_OUT_W1TC_REG, 1 << pin);
	
	// Clear error status.
	if (err) err->cause = 0;
}

bool io_read(badge_err_t *err, int pin) {
	// Check pin bounds.
	if (pin < 0 || pin > io_count()) {
		if (err) err->cause = ECAUSE_RANGE;
		if (err) err->location = ELOC_GPIO;
		return 0;
	}
	
	// Clear error status.
	if (err) err->cause = 0;
	
	// Read GPIO input register.
	return (READ_REG(GPIO_IN_REG) >> pin) & 1;
}

// Determine whether GPIO `pin` is claimed by a peripheral.
// Returns false on error.
bool io_is_peripheral(badge_err_t *err, int pin) {
	// Check pin bounds.
	if (pin < 0 || pin > io_count()) {
		if (err) err->cause = ECAUSE_RANGE;
		if (err) err->location = ELOC_GPIO;
		return 0;
	}
	
	// Clear error status.
	if (err) err->cause = 0;
	
	return (gpio_is_peripheral >> pin) & 1;
}



// Enable or disable "claimed by peripheral" mode for GPIO `pin`.
void rawgpio_set_peripheral(badge_err_t *err, int pin, bool claim_as_peripheral) {
	// Check pin bounds.
	if (pin < 0 || pin > io_count()) {
		if (err) err->cause = ECAUSE_RANGE;
		if (err) err->location = ELOC_GPIO;
		return;
	}
	
	// Set claimed flag.
	if (claim_as_peripheral) {
		gpio_is_peripheral |= 1 << pin;
		gpio_is_output     &= ~(1 << pin);
	} else {
		gpio_is_peripheral &= ~(1 << pin);
	}
	
	// Clear error status.
	if (err) err->cause = 0;
}

// Route GPIO `pin` to input signal `signo`.
void rawgpio_route_input(badge_err_t *err, int pin, int signo) {
	// Check bounds.
	if (pin < 0 || pin > io_count() || signo < 0 || signo > 127) {
		if (err) err->cause = ECAUSE_RANGE;
		if (err) err->location = ELOC_GPIO;
		return;
	}
	
	// Assert that pin is claimed for peripheral use.
	if (!((gpio_is_peripheral >> pin) & 1)) {
		if (err) err->cause = ECAUSE_NOTCONFIG;
		if (err) err->location = ELOC_GPIO;
		return;
	}
	
	// Configure FUNC_OUT_SEL.
	WRITE_REG(GPIO_FUNC_N_OUT_SEL_CFG_REG(pin), signo);
	
	// Clear error status.
	if (err) err->cause = 0;
}

// Route GPIO `pin` to output signal `signo`.
void rawgpio_route_output(badge_err_t *err, int pin, int signo) {
	// Check bounds.
	if (pin < 0 || pin > io_count() || signo < 0 || signo > 127) {
		if (err) err->cause = ECAUSE_RANGE;
		if (err) err->location = ELOC_GPIO;
		return;
	}
	
	// Assert that pin is claimed for peripheral use.
	if (!((gpio_is_peripheral >> pin) & 1)) {
		if (err) err->cause = ECAUSE_NOTCONFIG;
		if (err) err->location = ELOC_GPIO;
		return;
	}
	
	// Configure FUNC_IN_SEL.
	WRITE_REG(GPIO_FUNC_N_IN_SEL_CFG_REG(signo), pin);
	
	// Clear error status.
	if (err) err->cause = 0;
}

// Disable the signal routing and turn GPIO `pin` into a regular GPIO.
void rawgpio_unroute(badge_err_t *err, int pin) {
	// Check pin bounds.
	if (pin < 0 || pin > io_count()) {
		if (err) err->cause = ECAUSE_RANGE;
		if (err) err->location = ELOC_GPIO;
		return;
	}
	
	// Assert that pin is claimed for peripheral use.
	if (!((gpio_is_peripheral >> pin) & 1)) {
		if (err) err->cause = ECAUSE_NOTCONFIG;
		if (err) err->location = ELOC_GPIO;
		return;
	}
	
	// Configure FUNC_OUT_SEL.
	WRITE_REG(GPIO_FUNC_N_OUT_SEL_CFG_REG(pin), 128);
	
	// Search FUNC_IN_SEL registers.
	for (int i = 0; i < 128; i++) {
		int fpin = READ_REG(GPIO_FUNC_N_IN_SEL_CFG_REG(i)) & 31;
		if (fpin == pin) {
			WRITE_REG(GPIO_FUNC_N_IN_SEL_CFG_REG(i), 0x3C);
		}
	}
	
	// Clear error status.
	if (err) err->cause = 0;
}
