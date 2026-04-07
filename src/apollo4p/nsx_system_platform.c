/**
 * @file nsx_system_platform.c
 * @brief Apollo4-family platform backend for nsx_system.h.
 *
 * Covers: Apollo4P, Apollo4L (Apollo4B shares the same HAL).
 * Key differences from Apollo5 family:
 *   - Unified cache (am_hal_cachectrl_config/enable)
 *   - No CPDLP, no SpotManager
 *   - Has DCU (am_hal_dcu_update) — needed for ITM/SWO
 *   - am_hal_pwrctrl_mcu_mode_select() with 2 levels (LP/HP)
 *   - BSP helper am_bsp_itm_printf_enable() handles DCU + ITM
 */

#include "nsx_system.h"
#include "am_bsp.h"
#include "am_mcu_apollo.h"
#include "am_util.h"

/* ===================================================================
 * nsx_platform_hw_init — full BSP low-power init
 * =================================================================== */

uint32_t nsx_platform_hw_init(void) {
    am_bsp_low_power_init();
    return 0;
}

/* ===================================================================
 * nsx_platform_minimal_hw_init — lightweight init for Apollo4
 *
 * Apollo4 doesn't need CPDLP or SpotManager.  Enable caching
 * and FPU.
 * =================================================================== */

uint32_t nsx_platform_minimal_hw_init(void) {
    /* Enable cache with default config (unified on Apollo4) */
    am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
    am_hal_cachectrl_enable();

    /* FPU */
    am_hal_sysctrl_fpu_enable();
    am_hal_sysctrl_fpu_stacking_enable(true);

    return 0;
}

/* ===================================================================
 * nsx_platform_set_perf_mode — LP / HP on Apollo4
 * =================================================================== */

uint32_t nsx_platform_set_perf_mode(nsx_perf_mode_e mode) {
    if (mode == NSX_PERF_HIGH || mode == NSX_PERF_MEDIUM) {
        return am_hal_pwrctrl_mcu_mode_select(AM_HAL_PWRCTRL_MCU_MODE_HIGH_PERFORMANCE);
    } else {
        return am_hal_pwrctrl_mcu_mode_select(AM_HAL_PWRCTRL_MCU_MODE_LOW_POWER);
    }
}

/* ===================================================================
 * nsx_platform_spot_mgr_profile — no-op on Apollo4
 * =================================================================== */

uint32_t nsx_platform_spot_mgr_profile(void) {
    return 0;
}

/* ===================================================================
 * nsx_platform_debug_init — ITM/SWO setup for Apollo4
 *
 * Apollo4 has a DCU gate but the BSP helper handles it:
 *   am_bsp_itm_printf_enable() internally does:
 *     - Power OTP + Crypto
 *     - am_hal_dcu_update() for debug bits
 *     - TPIU + ITM enable
 *     - SWO pin config
 *     - printf backend
 * =================================================================== */

uint32_t nsx_platform_debug_init(const nsx_debug_config_t *cfg) {
    if (cfg == NULL) return 0;

    if (cfg->transport == NSX_DEBUG_ITM) {
        am_bsp_itm_printf_enable();
    } else if (cfg->transport == NSX_DEBUG_UART) {
        am_bsp_uart_printf_enable();
    }

    return 0;
}
