
// SPDX-License-Identifier: MIT

#include "port/pmu_init.h"

#include "soc/pmu_struct.h"
#include "soc/soc_types.h"



// Initialise the power management unit.
void pmu_init() {
    /* ==== HP system default ==== */
    PMU.hp_sys[PMU_HP_MODE_ACTIVE] = (pmu_hp_hw_regmap_t){
        .dig_power =
            {
                .vdd_spi_pd_en = 0,
                .mem_dslp      = 0,
                .mem_pd_en     = 0,
                .wifi_pd_en    = 0,
                .cpu_pd_en     = 0,
                .aon_pd_en     = 0,
                .top_pd_en     = 0,
            },
        .clk_power =
            {
                .i2c_iso_en    = 0,
                .i2c_retention = 0,
                .xpd_bb_i2c    = 1,
                .xpd_bbpll_i2c = 1,
                .xpd_bbpll     = 1,
            },
        .xtal =
            {
                .xpd_xtal = 1,
            },
        .icg_func = 0xffffffff,
        .icg_apb  = 0xffffffff,
        .icg_modem =
            {
                .code = PMU_HP_ICG_MODEM_CODE_ACTIVE,
            },
        .sysclk =
            {
                .dig_sysclk_nodiv = 0,
                .icg_sysclk_en    = 1,
                .sysclk_slp_sel   = 0,
                .icg_slp_sel      = 0,
                .dig_sysclk_sel   = PMU_HP_SYSCLK_XTAL,
            },
        .syscntl =
            {
                .uart_wakeup_en  = 0,
                .lp_pad_hold_all = 0,
                .hp_pad_hold_all = 0,
                .dig_pad_slp_sel = 0,
                .dig_pause_wdt   = 0,
                .dig_cpu_stall   = 0,
            },
        .bias =
            {
                .xpd_bias   = 1,
                .dbg_atten  = 0x0,
                .pd_cur     = 0,
                .bias_sleep = 0,
            },
        .regulator0 =
            {
                .lp_dbias_vol    = 0xd,
                .hp_dbias_vol    = 0x1c,
                .dbias_sel       = 1,
                .dbias_init      = 1,
                .slp_mem_xpd     = 0,
                .slp_logic_xpd   = 0,
                .xpd             = 1,
                .slp_mem_dbias   = 0,
                .slp_logic_dbias = 0,
                .dbias           = HP_CALI_DBIAS,
            },
        .regulator1 =
            {
                .drv_b = 0x0,
            },
        .backup =
            {
                .hp_sleep2active_backup_modem_clk_code = 2,
                .hp_modem2active_backup_modem_clk_code = 2,
                .hp_active_retention_mode              = 0,
                .hp_sleep2active_retention_en          = 0,
                .hp_modem2active_retention_en          = 0,
                .hp_sleep2active_backup_clk_sel        = 0,
                .hp_modem2active_backup_clk_sel        = 1,
                .hp_sleep2active_backup_mode           = PMU_HP_RETENTION_REGDMA_CONFIG(0, 0),
                .hp_modem2active_backup_mode           = PMU_HP_RETENTION_REGDMA_CONFIG(0, 2),
                .hp_sleep2active_backup_en             = 0,
                .hp_modem2active_backup_en             = 0,
            },
        .backup_clk = 0xffffffff,
    };

    PMU.hp_sys[PMU_HP_MODE_MODEM] = (pmu_hp_hw_regmap_t){
        .dig_power =
            {
                .vdd_spi_pd_en = 0,
                .mem_dslp      = 0,
                .mem_pd_en     = 0,
                .wifi_pd_en    = 0,
                .cpu_pd_en     = 1,
                .aon_pd_en     = 0,
                .top_pd_en     = 0,
            },
        .clk_power =
            {
                .i2c_iso_en    = 0,
                .i2c_retention = 0,
                .xpd_bb_i2c    = 1,
                .xpd_bbpll_i2c = 1,
                .xpd_bbpll     = 1,
            },
        .xtal =
            {
                .xpd_xtal = 1,
            },
        .icg_func = 0,
        .icg_apb  = 0,
        .icg_modem =
            {
                .code = PMU_HP_ICG_MODEM_CODE_MODEM,
            },
        .sysclk =
            {
                .dig_sysclk_nodiv = 0,
                .icg_sysclk_en    = 1,
                .sysclk_slp_sel   = 1,
                .icg_slp_sel      = 1,
                .dig_sysclk_sel   = PMU_HP_SYSCLK_PLL,
            },
        .syscntl =
            {
                .uart_wakeup_en  = 1,
                .lp_pad_hold_all = 0,
                .hp_pad_hold_all = 0,
                .dig_pad_slp_sel = 0,
                .dig_pause_wdt   = 1,
                .dig_cpu_stall   = 1,
            },
        .bias =
            {
                .xpd_bias   = 0,
                .dbg_atten  = 0x0,
                .pd_cur     = 0,
                .bias_sleep = 0,
            },
        .regulator0 =
            {
                .slp_mem_xpd     = 0,
                .slp_logic_xpd   = 0,
                .xpd             = 1,
                .slp_mem_dbias   = 0,
                .slp_logic_dbias = 0,
                .dbias           = HP_CALI_DBIAS,
            },
        .regulator1 =
            {
                .drv_b = 0x0,
            },
        .backup =
            {
                .hp_sleep2modem_backup_modem_clk_code = 1,
                .hp_modem_retention_mode              = 0,
                .hp_sleep2modem_retention_en          = 0,
                .hp_sleep2modem_backup_clk_sel        = 0,
                .hp_sleep2modem_backup_mode           = PMU_HP_RETENTION_REGDMA_CONFIG(0, 1),
                .hp_sleep2modem_backup_en             = 0,
            },
        .backup_clk = 0xffffffff,
    };

    PMU.hp_sys[PMU_HP_MODE_SLEEP] = (pmu_hp_hw_regmap_t){
        .dig_power =
            {
                .vdd_spi_pd_en = 1,
                .mem_dslp      = 0,
                .mem_pd_en     = 0,
                .wifi_pd_en    = 1,
                .cpu_pd_en     = 0,
                .aon_pd_en     = 0,
                .top_pd_en     = 0,
            },
        .clk_power =
            {
                .i2c_iso_en    = 1,
                .i2c_retention = 1,
                .xpd_bb_i2c    = 1,
                .xpd_bbpll_i2c = 0,
                .xpd_bbpll     = 0,
            },
        .xtal =
            {
                .xpd_xtal = 0,
            },
        .icg_func = 0,
        .icg_apb  = 0,
        .icg_modem =
            {
                .code = PMU_HP_ICG_MODEM_CODE_SLEEP,
            },
        .sysclk =
            {
                .dig_sysclk_nodiv = 0,
                .icg_sysclk_en    = 0,
                .sysclk_slp_sel   = 1,
                .icg_slp_sel      = 1,
                .dig_sysclk_sel   = PMU_HP_SYSCLK_XTAL,
            },
        .syscntl =
            {
                .uart_wakeup_en  = 1,
                .lp_pad_hold_all = 0,
                .hp_pad_hold_all = 0,
                .dig_pad_slp_sel = 1,
                .dig_pause_wdt   = 1,
                .dig_cpu_stall   = 1,
            },
        .bias =
            {
                .xpd_bias   = 0,
                .dbg_atten  = 0x0,
                .pd_cur     = 0,
                .bias_sleep = 0,
            },
        .regulator0 =
            {
                .slp_mem_xpd     = 0,
                .slp_logic_xpd   = 0,
                .xpd             = 1,
                .slp_mem_dbias   = 0,
                .slp_logic_dbias = 0,
                .dbias           = 1,
            },
        .regulator1 =
            {
                .drv_b = 0x0,
            },
        .backup =
            {
                .hp_modem2sleep_backup_modem_clk_code  = 0,
                .hp_active2sleep_backup_modem_clk_code = 2,
                .hp_sleep_retention_mode               = 0,
                .hp_modem2sleep_retention_en           = 0,
                .hp_active2sleep_retention_en          = 0,
                .hp_modem2sleep_backup_clk_sel         = 0,
                .hp_active2sleep_backup_clk_sel        = 0,
                .hp_modem2sleep_backup_mode            = PMU_HP_RETENTION_REGDMA_CONFIG(1, 1),
                .hp_active2sleep_backup_mode           = PMU_HP_RETENTION_REGDMA_CONFIG(1, 0),
                .hp_modem2sleep_backup_en              = 0,
                .hp_active2sleep_backup_en             = 0,
            },
        .backup_clk = 0xffffffff,
    };

    /* ==== LP system default ==== */
    // TODO.

    /* ==== Set power state ==== */
    pmu_hp_power_domain_t const domains[] = {
        PMU_HP_PD_TOP,
        PMU_HP_PD_HP_AON,
        PMU_HP_PD_CPU,
        PMU_HP_PD_WIFI,
    };
    for (int i = 0; i < 4; i++) {
        pmu_hp_power_domain_t domain           = domains[i];
        PMU.power.hp_pd[domain].force_reset    = false;
        PMU.power.hp_pd[domain].force_iso      = false;
        PMU.power.hp_pd[domain].force_pu       = false;
        PMU.power.hp_pd[domain].force_no_reset = false;
        PMU.power.hp_pd[domain].force_no_iso   = false;
        PMU.power.hp_pd[domain].force_pd       = false;
    }
    PMU.power.mem_cntl.force_hp_mem_no_iso = false;
}
