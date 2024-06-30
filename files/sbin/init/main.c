
// SPDX-License-Identifier: MIT

#include "hal/i2c.h"
#include "syscall.h"

#define SDA_PIN   6
#define SCL_PIN   7
#define CH32_ADDR 0x42

#define I2C_REG_FW_VERSION_0 0 // LSB
#define I2C_REG_FW_VERSION_1 1 // MSB
#define I2C_REG_RESERVED_0   2
#define I2C_REG_RESERVED_1   3
#define I2C_REG_LED_0        4 // LSB
#define I2C_REG_LED_1        5
#define I2C_REG_LED_2        6
#define I2C_REG_LED_3        7  // MSB
#define I2C_REG_BTN_0        8  // Button 0
#define I2C_REG_BTN_1        9  // Button 1
#define I2C_REG_BTN_2        10 // Button 2
#define I2C_REG_BTN_3        11 // Button 3
#define I2C_REG_BTN_4        12 // Button 4

uint8_t read_reg(uint8_t regno) {
    badge_err_t ec = {0};
    i2c_master_write_to(&ec, 0, CH32_ADDR, &regno, 1);
    uint8_t ret = 0;
    i2c_master_read_from(&ec, 0, CH32_ADDR, &ret, 1);
    return ret;
}

size_t strlen(char const *cstr) {
    char const *pre = cstr;
    while (*cstr) cstr++;
    return cstr - pre;
}

void print(char const *cstr) {
    syscall_temp_write(cstr, strlen(cstr));
}

char const hextab[] = "0123456789ABCDEF";

int main() {
    badge_err_t ec = {0};
    print("Hi, Ther.");
    // syscall_sys_shutdown(false);
    // return 420;

    // Set up I2C to the CH32V003.
    i2c_master_init(&ec, 0, SDA_PIN, SCL_PIN, 100000);
    i2c_master_write_to(&ec, 0, CH32_ADDR, "\x00", 1);
    i2c_master_write_to(&ec, 0, CH32_ADDR, "\x00", 1);
    uint8_t tmp;
    // i2c_master_read_from(&ec, 0, CH32_ADDR, &tmp, 1);
    // i2c_master_read_from(&ec, 0, CH32_ADDR, &tmp, 1);
    // i2c_master_read_from(&ec, 0, CH32_ADDR, &tmp, 1);
    // i2c_master_write_to(&ec, 0, CH32_ADDR, "\x04\xff\xff", 3);
    // syscall_sys_shutdown(false);
    // return 69;

    // Read coproc FW ver.
    uint16_t fwv = read_reg(I2C_REG_FW_VERSION_0) | (read_reg(I2C_REG_FW_VERSION_1) << 8);
    char     buf[4];
    buf[0] = hextab[(fwv >> 12) & 15];
    buf[1] = hextab[(fwv >> 8) & 15];
    buf[2] = hextab[(fwv >> 4) & 15];
    buf[3] = hextab[(fwv >> 0) & 15];
    print("CH32 version: 0x");
    syscall_temp_write(buf, 4);
    print("\n");

    // Poll inputs until one of the buttons is pressed, then exit.
    // while (true) {
    if (read_reg(I2C_REG_BTN_0)) {
        print("BTN0 triggered\n");
    }
    if (read_reg(I2C_REG_BTN_1)) {
        print("BTN1 triggered\n");
    }
    if (read_reg(I2C_REG_BTN_2)) {
        print("BTN2 triggered\n");
    }
    if (read_reg(I2C_REG_BTN_3)) {
        print("BTN3 triggered\n");
    }
    if (read_reg(I2C_REG_BTN_4)) {
        print("BTN4 triggered\n");
    }
    // }

    syscall_sys_shutdown(false);
    return 0;
}
