
// SPDX-License-Identifier: MIT

#include <i2c.h>
#include <gpio.h>
#include <rawgpio.h>
#include <hardware.h>
#include <clkconfig.h>

// Signal number for I²C SDA.
#define I2C_0_SDA_SIGNAL 46
// Signal number for I²C SCL.
#define I2C_0_SCL_SIGNAL 45


// Configures the low level width of the SCL Clock (Access: R/W)
#define I2C_SCL_LOW_PERIOD_REG			(I2C_BASE + 0x0000)
// Configures the hold time after a negative SCL edge (Access: R/W)
#define I2C_SDA_HOLD_REG				(I2C_BASE + 0x0030)
// Configures the sample time after a positive SCL edge (Access: R/W)
#define I2C_SDA_SAMPLE_REG				(I2C_BASE + 0x0034)
// Configures the high level width of SCL (Access: R/W)
#define I2C_SCL_HIGH_PERIOD_REG			(I2C_BASE + 0x0038)
// Configures the delay between the SDA and SCL negative edge for a start condition (Access: R/W)
#define I2C_SCL_START_HOLD_REG			(I2C_BASE + 0x0040)
// Configures the delay between the positive edge of SCL and the negative edge of SDA (Access: R/W)
#define I2C_SCL_RSTART_SETUP_REG		(I2C_BASE + 0x0044)
// Configures the delay after the SCL clock edge for a stop condition (Access: R/W)
#define I2C_SCL_STOP_HOLD_REG			(I2C_BASE + 0x0048)
// Configures the delay between the SDA and SCL rising edge for a stop condition (Access: R/W)
#define I2C_SCL_STOP_SETUP_REG			(I2C_BASE + 0x004C)
// SCL status time out register (Access: R/W)
#define I2C_SCL_ST_TIME_OUT_REG			(I2C_BASE + 0x0078)
// SCL main status time out register (Access: R/W)
#define I2C_SCL_MAIN_ST_TIME_OUT_REG	(I2C_BASE + 0x007C)
// Transmission setting (Access: varies)
#define I2C_CTR_REG						(I2C_BASE + 0x0004)
// Setting time out control for receiving data (Access: R/W)
#define I2C_TO_REG						(I2C_BASE + 0x000C)
// Local slave address setting (Access: R/W)
#define I2C_SLAVE_ADDR_REG				(I2C_BASE + 0x0010)
// FIFO configuration register (Access: R/W)
#define I2C_FIFO_CONF_REG				(I2C_BASE + 0x0018)
// SCL and SDA filter configuration register (Access: R/W)
#define I2C_FILTER_CFG_REG				(I2C_BASE + 0x0050)
// Power configuration register (Access: varies)
#define I2C_SCL_SP_CONF_REG				(I2C_BASE + 0x0080)
// Set SCL stretch of I2C slave (Access: varies)
#define I2C_SCL_STRETCH_CONF_REG		(I2C_BASE + 0x0084)
// Describe I2C work status (Access: RO)
#define I2C_SR_REG						(I2C_BASE + 0x0008)
// FIFO status register (Access: RO)
#define I2C_FIFO_ST_REG					(I2C_BASE + 0x0014)
// Rx FIFO read data (Access: HRO)
#define I2C_DATA_REG					(I2C_BASE + 0x001C)
// Raw interrupt status (Access: R/SS/WTC)
#define I2C_INT_RAW_REG					(I2C_BASE + 0x0020)
// Interrupt clear bits (Access: WT)
#define I2C_INT_CLR_REG					(I2C_BASE + 0x0024)
// Interrupt enable bits (Access: R/W)
#define I2C_INT_ENA_REG					(I2C_BASE + 0x0028)
// Status of captured I2C communication events (Access: RO)
#define I2C_INT_STATUS_REG				(I2C_BASE + 0x002C)
// I2C command register 0 (Access: varies)
#define I2C_COMD0_REG					(I2C_BASE + 0x0058)
// I2C command register 1 (Access: varies)
#define I2C_COMD1_REG					(I2C_BASE + 0x005C)
// I2C command register 2 (Access: varies)
#define I2C_COMD2_REG					(I2C_BASE + 0x0060)
// I2C command register 3 (Access: varies)
#define I2C_COMD3_REG					(I2C_BASE + 0x0064)
// I2C command register 4 (Access: varies)
#define I2C_COMD4_REG					(I2C_BASE + 0x0068)
// I2C command register 5 (Access: varies)
#define I2C_COMD5_REG					(I2C_BASE + 0x006C)
// I2C command register 6 (Access: varies)
#define I2C_COMD6_REG					(I2C_BASE + 0x0070)
// I2C command register 7 (Access: varies)
#define I2C_COMD7_REG					(I2C_BASE + 0x0074)
// Version register (Access: R/W)
#define I2C_DATE_REG					(I2C_BASE + 0x00F8)
// I2C TXFIFO base address register (Access: HRO)
#define I2C_TXFIFO_START_ADDR_REG		(I2C_BASE + 0x0100)
// I2C RXFIFO base address register (Access: HRO)
#define I2C_RXFIFO_START_ADDR_REG		(I2C_BASE + 0x0180)


// Copy of I²C peripheral configuration.
typedef struct {
	int sda_pin;
	int scl_pin;
	int32_t bitrate;
	bool is_master;
	bool enabled;
} i2c_sw_config_t;

static i2c_sw_config_t i2c_config[1];


// Initialises I²C peripheral i2c_num in slave mode with SDA pin `sda_pin`, SCL pin `scl_pin` and clock speed/bitrate bitrate.
// When initialised as an I²C master, the modes of the SDA and SCL pins are changed automatically.
// This function may be used again to change the settings on an initialised I²C peripheral in master mode.
void i2c_master_init(badge_err_t *ec, int i2c_num, int sda_pin, int scl_pin, int32_t bitrate) {
	badge_err_t ec_dummy;
	if (!ec) ec = &ec_dummy;
	
	// Check pin bounds.
	if (sda_pin < 0 || scl_pin > io_count() || scl_pin < 0 || scl_pin > io_count()) {
		ec->cause = ECAUSE_RANGE;
		ec->location = ELOC_GPIO;
		return;
	}
	
	// Check I²C peripheral bounds.
	if (i2c_num != 0) {
		ec->cause = ECAUSE_RANGE;
		ec->location = ELOC_I2C;
		return;
	}
	
	// Assert GPIOs are not in use.
	if (io_is_peripheral(NULL, sda_pin) || io_is_peripheral(NULL, scl_pin)) {
		ec->cause = ECAUSE_INUSE;
		ec->location = ELOC_GPIO;
		return;
	}
	
	// Assert I²C is not in slave mode.
	if (i2c_config[i2c_num].enabled && !i2c_config[i2c_num].is_master) {
		ec->cause = ECAUSE_INUSE;
		ec->location = ELOC_I2C;
		return;
	}
	
	// Set I²C peripheral clocks, enable and reset I2C0.
	// According to TRM, clock should be 20x bitrate.
	clkconfig_i2c0(bitrate * 20, 1, 1);
	
	// Configure I²C timing parameters.
	WRITE_REG(I2C_SCL_LOW_PERIOD_REG,		10);
	WRITE_REG(I2C_SDA_HOLD_REG,				3);
	WRITE_REG(I2C_SDA_SAMPLE_REG,			5);
	WRITE_REG(I2C_SCL_HIGH_PERIOD_REG,		10);
	WRITE_REG(I2C_SCL_STOP_HOLD_REG,		20);
	WRITE_REG(I2C_SCL_STOP_SETUP_REG,		40);
	WRITE_REG(I2C_SCL_ST_TIME_OUT_REG,		22);
	WRITE_REG(I2C_SCL_MAIN_ST_TIME_OUT_REG,	22);
	
	WRITE_REG(I2C_TO_REG,					(1<<5) | 9);
	
	// Configure I²C peripheral.
	
	// Perform final config and synchronise.
	
	// Route GPIO pins to I²C.
	rawgpio_route_input (ec, sda_pin, I2C_0_SDA_SIGNAL);
	if (ec->cause) return;
	rawgpio_route_output(ec, sda_pin, I2C_0_SDA_SIGNAL);
	if (ec->cause) return;
	rawgpio_route_input (ec, scl_pin, I2C_0_SCL_SIGNAL);
	if (ec->cause) return;
	rawgpio_route_output(ec, scl_pin, I2C_0_SCL_SIGNAL);
	if (ec->cause) return;
}

// De-initialises I²C peripheral i2c_num in master mode.
void i2c_master_deinit(badge_err_t *ec, int i2c_num) {
	
}

// Reads len bytes into buffer buf from I²C slave with ID slave_id.
// This function blocks until the entire transaction is completed and returns the number of acknowledged read bytes.
size_t i2c_master_read_from(badge_err_t *ec, int i2c_num, int slave_id, uint8_t *buf, size_t len) {
	return 0;
}

// Writes len bytes from buffer buf to I²C slave with ID slave_id.
// This function blocks until the entire transaction is completed and returns the number of acknowledged written bytes.
size_t i2c_master_write_to(badge_err_t *ec, int i2c_num, int slave_id, uint8_t *buf, size_t len) {
	return 0;
}
