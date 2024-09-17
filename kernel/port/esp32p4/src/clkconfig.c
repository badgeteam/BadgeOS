
// SPDX-License-Identifier: MIT

#include "port/clkconfig.h"

#include "attributes.h"
#include "log.h"
#include "soc/spi_reg.h"
#include "soc/spi_struct.h"
#include "soc/hp_sys_clkrst_struct.h"
#include "soc/clk_tree_defs.h"


// Nominal frequency of XTAL_CLK.
#define FREQ_XTAL_CLK      40000000

// Compute frequency dividers for a certain target frequency and source
// frequency.
static spi_clock_reg_t spi_clk_compute_div(uint32_t source_hz, uint32_t target_hz, uint8_t duty) PURE;

static spi_clock_reg_t spi_clk_compute_div(uint32_t source_hz, uint32_t target_hz, uint8_t duty) {
    uint32_t const pre_max = SPI_CLKDIV_PRE_V + 1;
    uint32_t const n_max   = SPI_CLKCNT_N_V + 1;

    spi_clock_reg_t spi_clock_reg = {.val = 0};

    uint32_t best_n = 1, best_pre = 1;

    if ((target_hz < source_hz)) {
        if ((source_hz / (pre_max * n_max)) > target_hz) {
            logkf(LOG_DEBUG, "Maximum SPI clock pre-scaling");
            best_pre = pre_max;
            best_n   = n_max;
        } else if (target_hz < source_hz) {
            logkf(LOG_DEBUG, "Finding optimal SPI clock pre-scaling");
            int32_t best_err = (int32_t)target_hz; // assume worst possible

            // Try to get the most counter resolution and lowest pre-divider
            for (uint32_t n = n_max; n >= 2; n--) {
                uint32_t pre = (source_hz / n) / target_hz;

                if (pre < 1)
                    pre = 1;
                // Prefer to err on the side of too low clock frequency to not damage peripherals
                if ((source_hz / n / pre) > target_hz) {
                    pre += 1;
                }
                if (pre > pre_max)
                    continue;

                int32_t err = (int32_t)target_hz - (int32_t)(source_hz / n / pre);
                if (err < best_err) {
                    best_n   = n;
                    best_pre = pre;
                    best_err = err;
                }
            }
        }

        // Map duty from [0..255] to  [1..n-1]
        uint32_t h = 1 + ((duty * ((best_n - 1) - 1)) / 255);

        spi_clock_reg.clkdiv_pre = best_pre - 1;
        spi_clock_reg.clkcnt_n   = best_n - 1;
        spi_clock_reg.clkcnt_l   = best_n - 1;
        spi_clock_reg.clkcnt_h   = h - 1;
    } else {
        logkf(LOG_DEBUG, "No SPI clock pre-scaling");
        spi_clock_reg.clk_equ_sysclk = 1;
    }

    return spi_clock_reg;
}

// Configure SPI2 clock.
void clkconfig_spi2(uint32_t freq_hz, bool enable, bool reset) {
    // SPI2 is configured on FREQ_XTAL_CLK.
    HP_SYS_CLKRST.hp_rst_en2.reg_rst_en_spi2 = reset;
    HP_SYS_CLKRST.hp_rst_en2.reg_rst_en_spi2 = 0;

    HP_SYS_CLKRST.soc_clk_ctrl1.reg_gpspi2_sys_clk_en = enable;
    HP_SYS_CLKRST.soc_clk_ctrl2.reg_gpspi2_apb_clk_en = enable;
    HP_SYS_CLKRST.peri_clk_ctrl116.reg_gpspi2_hs_clk_en = enable;
    HP_SYS_CLKRST.peri_clk_ctrl116.reg_gpspi2_mst_clk_en = enable;
    // HP_SYS_CLKRST.peri_clk_ctrl116.reg_gpspi2_clk_src_sel = SPI_CLK_SRC_DEFAULT;
    HP_SYS_CLKRST.peri_clk_ctrl116.reg_gpspi2_hs_clk_div_num = 0;
    HP_SYS_CLKRST.peri_clk_ctrl116.reg_gpspi2_mst_clk_div_num = 0;
    GPSPI2.clock = spi_clk_compute_div(FREQ_XTAL_CLK, freq_hz, 256 / 2);
    // GPSPI2.clk_gate.mst_clk_active = true;
    // GPSPI2.clk_gate.mst_clk_sel = 1;
    logkf(LOG_DEBUG, "soc_clk_ctrl1    :      %{u32;x}", HP_SYS_CLKRST.soc_clk_ctrl1.val);
    logkf(LOG_DEBUG, "peri_clk_ctrl116:       %{u32;x}", HP_SYS_CLKRST.peri_clk_ctrl116.val);
    logkf(
        LOG_DEBUG,
        "CLKDIV_PRE %{u32;d}, CLKCNT_N: %{u32;d}, CLKCNT_L: %{u32;d}, CLKCNT_H: %{u32;d}",
        GPSPI2.clock.clkdiv_pre,
        GPSPI2.clock.clkcnt_n,
        GPSPI2.clock.clkcnt_l,
        GPSPI2.clock.clkcnt_h
    );
}
