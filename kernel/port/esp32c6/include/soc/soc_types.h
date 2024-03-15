
// SPDX-License-Identifier: MIT

#pragma once

typedef enum {
    PMU_HP_MODE_ACTIVE,
    PMU_HP_MODE_MODEM,
    PMU_HP_MODE_SLEEP,
} pmu_hp_mode_t;

typedef enum {
    PMU_HP_SYSCLK_XTAL,
    PMU_HP_SYSCLK_PLL,
    PMU_HP_SYSCLK_FOSC,
} pmu_hp_sysclk_src_t;

typedef enum {
    PMU_HP_ICG_MODEM_CODE_SLEEP,
    PMU_HP_ICG_MODEM_CODE_MODEM,
    PMU_HP_ICG_MODEM_CODE_ACTIVE,
} pmu_hp_icg_modem_mode_t;

typedef enum {
    PMU_HP_PD_TOP,
    PMU_HP_PD_HP_AON,
    PMU_HP_PD_CPU,
    PMU_HP_PD_RESERVED,
    PMU_HP_PD_WIFI,
} pmu_hp_power_domain_t;

#define HP_CALI_DBIAS 25

#define PMU_HP_RETENTION_REGDMA_CONFIG(dir, entry) ((((dir) << 2) | (entry & 3)) & 7)
