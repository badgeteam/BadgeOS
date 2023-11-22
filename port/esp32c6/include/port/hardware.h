
// SPDX-License-Identifier: MIT

#pragma once

#include <stddef.h>
#include <stdint.h>



/* ==== CPU INFO ==== */

// Number of PMP regions supported by the CPU.
#define RISCV_PMP_REGION_COUNT 16


/* ==== PERIPHERAL MAP ==== */

// Base address of peripherals.
#define PERI_BASE   0x60000000
// Base address of CPU sub-system.
#define CPUSYS_BASE 0x20000000

// Base address of UART0 peripheral.
#define UART0_BASE        (PERI_BASE + 0x00000)
// Base address of UART1 peripheral.
#define UART1_BASE        (PERI_BASE + 0x01000)
// Base address of SPI0 peripheral.
#define SPI0_BASE         (PERI_BASE + 0x02000)
// Base address of SPI1 peripheral.
#define SPI1_BASE         (PERI_BASE + 0x03000)
// Base address of I2C peripheral.
#define I2C_BASE          (PERI_BASE + 0x04000)
// Base address of UHCI peripheral.
#define UHCI_BASE         (PERI_BASE + 0x05000)
// Base address of RMT peripheral.
#define RMT_BASE          (PERI_BASE + 0x06000)
// Base address of LEDC peripheral.
#define LEDC_BASE         (PERI_BASE + 0x07000)
// Base address of TIMG0 peripheral.
#define TIMG0_BASE        (PERI_BASE + 0x08000)
// Base address of TIMG1 peripheral.
#define TIMG1_BASE        (PERI_BASE + 0x09000)
// Base address of SYSTIMER peripheral.
#define SYSTIMER_BASE     (PERI_BASE + 0x0A000)
// Base address of TWAI0 peripheral.
#define TWAI0_BASE        (PERI_BASE + 0x0B000)
// Base address of I2S peripheral.
#define I2S_BASE          (PERI_BASE + 0x0C000)
// Base address of TWAI1 peripheral.
#define TWAI1_BASE        (PERI_BASE + 0x0D000)
// Base address of SAR ADC peripheral.
#define SAR_ADC_BASE      (PERI_BASE + 0x0E000)
// Base address of USB serial / JTAG controller.
#define USB_JTAG_BASE     (PERI_BASE + 0x0F000)
// Base address of interrupt matrix controller.
#define INTMTX_BASE       (PERI_BASE + 0x10000)
// Base address of Pulse Count Controller.
#define PCNT_BASE         (PERI_BASE + 0x12000)
// Base address of Event Task Matrix.
#define SOC_ETM_BASE      (PERI_BASE + 0x13000)
// Base address of Motor Control PWM.
#define MCPWM_BASE        (PERI_BASE + 0x14000)
// Base address of Parallel IO Controller.
#define PARL_IO_BASE      (PERI_BASE + 0x15000)
// Base address of SDIO HINF.
#define SDIO_HINF_BASE    (PERI_BASE + 0x16000)
// Base address of SDIO SLC.
#define SDIO_SLC_BASE     (PERI_BASE + 0x17000)
// Base address of SDIO SLCHOST.
#define SDIO_SLCHOST_BASE (PERI_BASE + 0x18000)
// Base address of GDMA Controller.
#define GDMA_BASE         (PERI_BASE + 0x80000)
// Base address of General Purpose SPI2.
#define SPI2_BASE         (PERI_BASE + 0x81000)
// Base address of AES Accelerator.
#define AES_BASE          (PERI_BASE + 0x88000)
// Base address of SHA Accelerator.
#define SHA_BASE          (PERI_BASE + 0x89000)
// Base address of RSA Accelerator.
#define RSA_BASE          (PERI_BASE + 0x8A000)
// Base address of ECC Accelerator.
#define ECC_BASE          (PERI_BASE + 0x8B000)
// Base address of Digital Signature.
#define DS_BASE           (PERI_BASE + 0x8C000)
// Base address of HMAC Accelerator.
#define HMAC_BASE         (PERI_BASE + 0x8D000)
// Base address of IO MUX.
#define IOMUX_BASE        (PERI_BASE + 0x90000)
// Base address of GPIO Matrix.
#define GPIOMTX_BASE      (PERI_BASE + 0x91000)
// Base address of Memory Assess Monitor (MEM_MONITOR).
#define MEMMON_BASE       (PERI_BASE + 0x92000)
// Base address of HP System Register.
#define HP_SYSREG_BASE    (PERI_BASE + 0x95000)
// Base address of Power/Clock/Reset (PCR) Register.
#define PCR_BASE          (PERI_BASE + 0x96000)
// Base address of Trusted Execution Environment (TEE) Register.
#define HP_TEE_BASE       (PERI_BASE + 0x98000)
// Base address of Access Permission Management Controller (HP_APM).
#define HP_APM_BASE       (PERI_BASE + 0x99000)
// Base address of Power Management Unit.
#define PMU_BASE          (PERI_BASE + 0xB0000)
// Base address of Low-power Clock/Reset Register.
#define LP_CLKRST_BASE    (PERI_BASE + 0xB0400)
// Base address of eFuse Controller.
#define EFUSE_BASE        (PERI_BASE + 0xB0800)
// Base address of Low-power Timer.
#define LP_TIMER_BASE     (PERI_BASE + 0xB0C00)
// Base address of Low-power Always-on Register.
#define LP_AON_BASE       (PERI_BASE + 0xB1000)
// Base address of Low-power UART.
#define LP_UART_BASE      (PERI_BASE + 0xB1400)
// Base address of Low-power I2C.
#define LP_I2C_BASE       (PERI_BASE + 0xB1800)
// Base address of Low-power Watch Dog Timer.
#define LP_WDT_BASE       (PERI_BASE + 0xB1C00)
// Base address of Low-power IO MUX (LP IO MUX).
#define LPIOMUX_BASE      (PERI_BASE + 0xB2000)
// Base address of I2C Analog Master.
#define I2C_ANA_MST_BASE  (PERI_BASE + 0xB2400)
// Base address of Low-power Peripheral.
#define LPPERI_BASE       (PERI_BASE + 0xB2800)
// Base address of Low-power Analog Peripheral.
#define LP_ANA_PERI_BASE  (PERI_BASE + 0xB2C00)
// Base address of Low-power Trusted Execution Environment (LP_TEE).
#define LP_TEE_BASE       (PERI_BASE + 0xB3400)
// Base address of Low-power Access Permission Management (LP_APM).
#define LP_APM_BASE       (PERI_BASE + 0xB3800)
// Base address of RISC-V Trace Encoder.
#define TRACE_BASE        (PERI_BASE + 0xC0000)
// Base address of DEBUG ASSIST (ASSIST_DEBUG).
#define DEBUG_BASE        (PERI_BASE + 0xC2000)
// Base address of Interrupt Priority Register.
#define INTPRI_BASE       (PERI_BASE + 0xC5000)

// Base address of PLIC.
#define PLIC_MX_BASE (CPUSYS_BASE + 0x1000)



#define WRITE_REG(addr, val)                                                                                           \
    do {                                                                                                               \
        *(volatile uint32_t *)(addr) = (val);                                                                          \
    } while (0)
#define READ_REG(addr) (*(volatile uint32_t *)(addr))
