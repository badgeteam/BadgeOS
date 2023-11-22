
// SPDX-License-Identifier: MIT

#include "time.h"

#include "cpu/isr.h"
#include "log.h"
#include "port/hardware_allocation.h"
#include "port/interrupt.h"
#include "port/intmtx.h"
#include "scheduler/isr.h"
#include "scheduler/scheduler.h"

#define LP_WDT_CONFIG0_REG       (LP_WDT_BASE + 0x0000)
#define LP_WDT_CONFIG1_REG       (LP_WDT_BASE + 0x0004)
#define LP_WDT_CONFIG2_REG       (LP_WDT_BASE + 0x0008)
#define LP_WDT_CONFIG3_REG       (LP_WDT_BASE + 0x000c)
#define LP_WDT_CONFIG4_REG       (LP_WDT_BASE + 0x0010)
#define LP_WDT_FEED_REG          (LP_WDT_BASE + 0x0014)
#define LP_WDT_WRPROTECT_REG     (LP_WDT_BASE + 0x0018)
#define LP_WDT_SWD_CONFIG_REG    (LP_WDT_BASE + 0x001c)
#define LP_WDT_SWD_WRPROTECT_REG (LP_WDT_BASE + 0x0020)
#define LP_WDT_INT_RAW_REG       (LP_WDT_BASE + 0x0024)
#define LP_WDT_IN_ST_REG         (LP_WDT_BASE + 0x0028)
#define LP_WDT_INT_ENA_REG       (LP_WDT_BASE + 0x002c)
#define LP_WDT_INT_CLR_REG       (LP_WDT_BASE + 0x0030)
#define LP_WDT_DATE_REG          (LP_WDT_BASE + 0x03fc)


#define LD_WDT_WRPROTECT_MAGIC 0x50D83AA1

// Timer 0 configuration register (Access: varies)
#define T0CONFIG_REG       0x0000
// Timer 0 current value, low 32 bits (Access: RO)
#define T0LO_REG           0x0004
// Timer 0 current value, high 22 bits (Access: RO)
#define T0HI_REG           0x0008
// Write to copy current timer value to T0LO_REG or T0HI_REG (Access: R/W/SC)
#define T0UPDATE_REG       0x000C
// Timer 0 alarm value, low 32 bits (Access: R/W)
#define T0ALARMLO_REG      0x0010
// Timer 0 alarm value, high bits (Access: R/W)
#define T0ALARMHI_REG      0x0014
// Timer 0 reload value, low 32 bits (Access: R/W)
#define T0LOADLO_REG       0x0018
// Timer 0 reload value, high 22 bits (Access: R/W)
#define T0LOADHI_REG       0x001C
// Write to reload timer from T0LOADLO_REG or T0LOADHI_REG (Access: WT)
#define T0LOAD_REG         0x0020
// Watchdog timer configuration register (Access: varies)
#define WDTCONFIG0_REG     0x0048
// Watchdog timer prescaler register (Access: varies)
#define WDTCONFIG1_REG     0x004C
// Watchdog timer stage 0 timeout value (Access: R/W)
#define WDTCONFIG2_REG     0x0050
// Watchdog timer stage 1 timeout value (Access: R/W)
#define WDTCONFIG3_REG     0x0054
// Watchdog timer stage 2 timeout value (Access: R/W)
#define WDTCONFIG4_REG     0x0058
// Watchdog timer stage 3 timeout value (Access: R/W)
#define WDTCONFIG5_REG     0x005C
// Write to feed the watchdog timer (Access: WT)
#define WDTFEED_REG        0x0060
// Watchdog write protect register (Access: R/W)
#define WDTWPROTECT_REG    0x0064
// RTC frequency calculation configuration register 0 (Access: varies)
#define RTCCALICFG_REG     0x0068
// RTC frequency calculation configuration register 1 (Access: RO)
#define RTCCALICFG1_REG    0x006C
// RTC frequency calculation configuration register 2 (Access: varies)
#define RTCCALICFG2_REG    0x0080
// Interrupt enable bits (Access: R/W)
#define INT_ENA_TIMERS_REG 0x0070
// Raw interrupt status (Access: R/SS/WTC)
#define INT_RAW_TIMERS_REG 0x0074
// Masked interrupt status (Access: RO)
#define INT_ST_TIMERS_REG  0x0078
// Interrupt clear bits (Access: WT)
#define INT_CLR_TIMERS_REG 0x007C
// Timer version control register (Access: R/W)
#define NTIMERS_DATE_REG   0x00F8
// Timer group clock gate register (Access: R/W)
#define REGCLK_REG         0x00FC

// Timer group timer enable.
#define TIMG_TCONFIG_EN_BIT         0x80000000
// Timer group timer count up.
#define TIMG_TCONFIG_INCREASE_BIT   0x40000000
// Timer group timer autoreload.
#define TIMG_TCONFIG_AUTORELOAD_BIT 0x20000000
// Timer group timer divider.
#define TIMG_TCONFIG_DIVIDER_MASK   0x1fffe000
// Timer group timer divider (bit position).
#define TIMG_TCONFIG_DIVIDER_POS    13
// Timer group timer divider counter reset.
#define TIMG_TCONFIG_DIVCNT_RST_BIT 0x00001000
// Timer group timer alarm enable.
#define TIMG_TCONFIG_ALARM_EN_BIT   0x00000400

// Timer group timer interrupt clear bit.
#define TIMG_T0_INT_CLR_BIT 1
// Timer group watchdog interrupt clear bit.
#define TIMG_T0_WDT_CLR_BIT 2

// Timer group timer interrupt state bit.
#define TIMG_T0_INT_ST_BIT 1
// Timer group watchdog interrupt state bit.
#define TIMG_T0_WDT_ST_BIT 2

// Timer group timer interrupt enable bit.
#define TIMG_T0_INT_EN_BIT 1
// Timer group watchdog interrupt enable bit.
#define TIMG_T0_WDT_EN_BIT 2



// Get timer group base address based on timer index.
static inline size_t timg_base(int timerno) {
    return timerno ? TIMG1_BASE : TIMG0_BASE;
}

// Get timer clock source frequency based on timer index.
#define timer_clk_freq(x) (40000000)



// Initialise timer and watchdog subsystem.
void time_init() {
    // Disable LP WDT.
    WRITE_REG(LP_WDT_WRPROTECT_REG, LD_WDT_WRPROTECT_MAGIC);
    WRITE_REG(LP_WDT_CONFIG0_REG, 0);

    // Set TIMG0 T0 frequency to 1MHz.
    timer_stop(0);
    timer_set_freq(0, 1000000);
    timer_value_set(0, 0);

    // Set TIMG1 T0 frequency to 1MHz.
    timer_stop(1);
    timer_set_freq(1, 1000000);
    timer_value_set(1, 0);

    // Start systick timer.
    timer_start(TIMER_SYSTICK_NUM);
}

// Get current time in microseconds.
int64_t time_us() {
    return timer_value_get(TIMER_SYSTICK_NUM);
}

void time_set_next_task_switch(timestamp_us_t timestamp) {
    timer_alarm_config(TIMER_SYSTICK_NUM, timestamp, false);
}

// Set the counting frequency of a hardware timer.
void timer_set_freq(int timerno, int32_t frequency) {
    size_t addr    = timg_base(timerno) + T0CONFIG_REG;
    int    divider = timer_clk_freq(timerno) / frequency;
    if (divider < 2 || divider > 65535) {
        logk(LOG_ERROR, "Unachievable timer frequency requested");
        divider = 2;
    }
    WRITE_REG(addr, (READ_REG(addr) & 0xe0001fff) | (divider << 13));
}

// Configure timer interrupt settings.
void timer_int_config(int timerno, bool enable, int channel) {
    // Disable interrupts before changing interrupt settings.
    bool mie = interrupt_disable();

    size_t base = timg_base(timerno);
    if (enable) {
        // Route interrupt.
        if (timerno) {
            intmtx_route(INTMTX_CORE0_TG1_T0_INTR_MAP_REG, channel);
        } else {
            intmtx_route(INTMTX_CORE0_TG0_T0_INTR_MAP_REG, channel);
        }

        // Enable timer interrupt output.
        WRITE_REG(base + T0CONFIG_REG, READ_REG(base + T0CONFIG_REG) | TIMG_TCONFIG_ALARM_EN_BIT);
        WRITE_REG(base + INT_ENA_TIMERS_REG, READ_REG(base + INT_ENA_TIMERS_REG) | TIMG_T0_INT_EN_BIT);
    } else {
        // Disable timer interrupt output.
        WRITE_REG(base + T0CONFIG_REG, READ_REG(base + T0CONFIG_REG) & ~TIMG_TCONFIG_ALARM_EN_BIT);
        WRITE_REG(base + INT_ENA_TIMERS_REG, READ_REG(base + INT_ENA_TIMERS_REG) & ~TIMG_T0_INT_EN_BIT);
    }

    // Re-enable interrupts.
    asm volatile("fence");
    if (mie) {
        interrupt_enable();
    }
}

// Configure timer alarm.
void timer_alarm_config(int timerno, int64_t threshold, bool reset_on_alarm) {
    size_t base = timg_base(timerno);
    if (reset_on_alarm) {
        WRITE_REG(base + T0CONFIG_REG, READ_REG(base + T0CONFIG_REG) | TIMG_TCONFIG_AUTORELOAD_BIT);
    } else {
        WRITE_REG(base + T0CONFIG_REG, READ_REG(base + T0CONFIG_REG) & ~TIMG_TCONFIG_AUTORELOAD_BIT);
    }
    if (threshold < 1 || threshold >= (1ll << 56)) {
        logk(LOG_ERROR, "Unachievable timer alarm value requested");
        threshold = (1ll << 56) - 1;
    }
    WRITE_REG(base + T0ALARMHI_REG, -1);
    WRITE_REG(base + T0ALARMLO_REG, threshold);
    WRITE_REG(base + T0ALARMHI_REG, threshold >> 32);
}

// Get the current value of timer.
int64_t timer_value_get(int timerno) {
    size_t   base = timg_base(timerno);
    uint32_t lo   = READ_REG(base + T0LO_REG);
    int      div  = 32;
    WRITE_REG(base + T0UPDATE_REG, -1);
    while (READ_REG(base + T0LO_REG) == lo && --div)
        ;
    return READ_REG(base + T0LO_REG) | ((uint64_t)READ_REG(base + T0HI_REG) << 32LLU);
}

// Set the current value of timer.
void timer_value_set(int timerno, int64_t value) {
    size_t base = timg_base(timerno);
    WRITE_REG(base + T0LOADLO_REG, value);
    WRITE_REG(base + T0LOADLO_REG, value >> 32);
    WRITE_REG(base + T0LOAD_REG, -1);
}

// Enable the timer counting.
void timer_start(int timerno) {
    size_t addr = timg_base(timerno) + T0CONFIG_REG;
    WRITE_REG(addr, READ_REG(addr) | 0xc0000000);
}

// Disable the timer counting.
void timer_stop(int timerno) {
    size_t addr = timg_base(timerno) + T0CONFIG_REG;
    WRITE_REG(addr, READ_REG(addr) & ~0x80000000);
}



// Callback to the timer driver for when a timer alarm fires.
void timer_isr_timer_alarm() {
    if (READ_REG(timg_base(TIMER_PREEMPT_NUM) + INT_ST_TIMERS_REG) & TIMG_T0_INT_ST_BIT) {
        // Timer used for preempting had an interrupt, perform task switch.
        sched_request_switch_from_isr();
    }

    // Check TIMG0 T0 interrupt.
    if (READ_REG(TIMG0_BASE + INT_ST_TIMERS_REG) & TIMG_T0_INT_ST_BIT) {
        // Acknowledge timer interrupt.
        WRITE_REG(TIMG0_BASE + INT_CLR_TIMERS_REG, TIMG_T0_INT_CLR_BIT);
        WRITE_REG(TIMG0_BASE + INT_CLR_TIMERS_REG, 0);
    }

    // Check TIMG1 T0 interrupt.
    if (READ_REG(TIMG1_BASE + INT_ST_TIMERS_REG) & TIMG_T0_INT_ST_BIT) {
        // Acknowledge timer interrupt.
        WRITE_REG(TIMG1_BASE + INT_CLR_TIMERS_REG, TIMG_T0_INT_CLR_BIT);
        WRITE_REG(TIMG1_BASE + INT_CLR_TIMERS_REG, 0);
    }
}

// Callback to the timer driver for when a watchdog alarm fires.
void timer_isr_watchdog_alarm() {
    logk(LOG_DEBUG, "Watchdog alarm ISR");
}
