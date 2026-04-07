/**
 * @file nsx_system_platform.c
 * @brief Apollo3/3P platform backend for nsx_system.h.
 *
 * Covers: Apollo3, Apollo3P.
 * Key differences from Apollo5 family:
 *   - Unified cache (am_hal_cachectrl_config/enable instead of split I/D)
 *   - No CPDLP, no SpotManager, no DCU
 *   - Burst mode API instead of am_hal_pwrctrl_mcu_mode_select()
 *   - Simple ITM/SWO via BSP helper (no DCU gate, GPIO 41)
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
 * nsx_platform_minimal_hw_init — lightweight init for Apollo3
 *
 * Apollo3 doesn't need CPDLP or SpotManager.  Just enable caching
 * and FPU.
 * =================================================================== */

uint32_t nsx_platform_minimal_hw_init(void) {
    /* Enable cache with default config (unified on Apollo3) */
    am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
    am_hal_cachectrl_enable();

    /* FPU */
    am_hal_sysctrl_fpu_enable();
    am_hal_sysctrl_fpu_stacking_enable(true);

    return 0;
}

/* ===================================================================
 * nsx_platform_set_perf_mode — burst mode on Apollo3
 *
 * Apollo3 has 48 MHz normal / 96 MHz burst.
 * =================================================================== */

uint32_t nsx_platform_set_perf_mode(nsx_perf_mode_e mode) {
    if (mode == NSX_PERF_HIGH || mode == NSX_PERF_MEDIUM) {
        am_hal_burst_avail_e avail;
        uint32_t st = am_hal_burst_mode_initialize(&avail);
        if (st != 0) return st;
        if (avail != AM_HAL_BURST_AVAIL) return 1;

        am_hal_burst_mode_e status;
        return am_hal_burst_mode_enable(&status);
    } else {
        am_hal_burst_mode_e status;
        return am_hal_burst_mode_disable(&status);
    }
}

/* ===================================================================
 * nsx_platform_spot_mgr_profile — no-op on Apollo3
 * =================================================================== */

uint32_t nsx_platform_spot_mgr_profile(void) {
    return 0;
}

/* ===================================================================
 * nsx_platform_debug_init — ITM/SWO setup for Apollo3
 *
 * Apollo3 is straightforward:
 *   - No DCU gate required
 *   - BSP helper handles TPIU, ITM, SWO pin (GPIO 41), printf init
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
