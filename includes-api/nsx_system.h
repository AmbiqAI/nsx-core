/**
 * @file nsx_system.h
 * @brief Modular system initialization for NSX applications.
 *
 * Provides composable, order-aware startup building blocks so that
 * applications can enable only the peripherals and subsystems they need.
 *
 * ## Boot Sequence Overview
 *
 * The ARM Cortex-M goes through several stages before reaching main().
 * Details vary by SoC (M4F on Apollo3/4, M55 on Apollo5/510):
 *
 * ```
 *  Reset_Handler (startup_gcc.c)
 *    ├─ VTOR, SP, FPU enable
 *    ├─ MSPLIM / PSPLIM (stack guard, M55 only)
 *    ├─ Copy .data → TCM                ← initialized globals
 *    ├─ Copy .shared → SHARED_SRAM      ← NSX_MEM_SRAM data (AP4+)
 *    ├─ Copy .itcm_text → ITCM          ← NSX_MEM_FAST_CODE (AP510)
 *    ├─ Zero .bss in TCM
 *    ├─ Zero .sram_bss in SHARED_SRAM   ← NSX_MEM_SRAM_BSS (AP4+)
 *    ├─ SystemInit()                     ← CMSIS, sets clocks
 *    ├─ __libc_init_array()              ← C++ static constructors
 *    └─ main()
 * ```
 *
 * ## Recommended Initialization Order in main()
 *
 * The `nsx_system_init()` convenience function performs all steps in the
 * correct order. For fine-grained control, call the individual helpers
 * in this order:
 *
 * ```c
 * // 1. Core runtime (required first — gates all other NSX calls)
 * ns_core_init(&core_cfg);
 *
 * // 2. Low-power hardware init (required for cache, clocks, SIMOBUCK)
 * //    NOTE: am_bsp_low_power_init() includes a 2-second delay.
 * //    Use nsx_minimal_hw_init() instead if you don't need full BSP setup.
 * nsx_hw_init();               // or nsx_minimal_hw_init()
 *
 * // 3. Cache (requires CPDLP from step 2 — will fail without it)
 * nsx_cache_enable();           // from nsx_mem.h
 *
 * // 4. Performance mode (HP clock)
 * nsx_set_perf_mode(NSX_PERF_HIGH);
 *
 * // 5. Debug output (order matters — see gotchas below)
 * nsx_debug_init(&debug_cfg);
 *
 * // 6. Application-specific peripherals
 * // ... your code ...
 * ```
 *
 * ## Common Gotchas
 *
 * ### I/D Cache silently not enabled (Apollo5 family)
 *   `am_hal_cachectrl_icache_enable()` checks `PWRMODCTL->CPDLPSTATE` and
 *   returns FAIL (silently!) if the cache power domain (RLP) is not active.
 *   This is configured by `am_hal_pwrctrl_low_power_init()` inside
 *   `am_bsp_low_power_init()`. Without it, inference runs 3–4x slower
 *   with no visible error.
 *   → Always call `nsx_hw_init()` or `nsx_minimal_hw_init()` before
 *     `nsx_cache_enable()`. Check the return value.
 *
 * ### SpotManager requires init before profile_set (Apollo5 family)
 *   `am_hal_spotmgr_init()` is called inside `am_hal_pwrctrl_low_power_init()`.
 *   Calling `am_hal_spotmgr_profile_set()` without it is undefined behavior.
 *   Apollo3 and Apollo4 do not have SpotManager — this is a no-op on those SoCs.
 *
 * ### SWO/ITM Printf Dependencies (all SoCs with DCU)
 *   Getting SWO printf output on Apollo4+ requires unlocking the DCU, which
 *   the Secure Bootloader (SBL) locks before transferring to user code.
 *   `nsx_debug_init()` handles this transparently on all SoC families.
 *
 *   The specific sequence is:
 *   1. **DCU unlock** — OTP + Crypto powered temporarily, `am_hal_dcu_update()`
 *      enables SWO/DWT/SWD/TRACE bits, then Crypto/OTP are powered down.
 *      This step is shared by all Apollo5 variants and is handled internally.
 *   2. **TPIU / ITM / SWO pin** — Configured by BSP helper on AP330P/510L/AP4/AP3,
 *      or manually on Apollo510/5B/5A (where `MCUCTRL_DBGCTRL_DBGTPIUCLKSEL_HFRC_96MHz`
 *      is available).
 *   3. **printf backend** — `am_util_stdio_printf_init()` with `am_hal_itm_print`.
 *
 * ### SWO Trace Clock and JLink Frequency
 *   The TPIU generates SWO output at: `baud = trace_clk / (ACPR + 1)`.
 *   JLink SWO viewer overrides ACPR based on what it believes is the trace
 *   clock (passed via `-cpufreq`). If the value is wrong, the baud rate
 *   mismatch causes silent data loss — SWO connects but shows no output.
 *
 *   | SoC         | Trace clock source | Freq     | JLink -cpufreq |
 *   |-------------|--------------------|----------|----------------|
 *   | Apollo510   | HFRC_96MHz         | 96 MHz   | 96000000       |
 *   | Apollo330P  | XTAL_HS            | 48 MHz   | 48000000       |
 *   | Apollo4P    | HFRC_96MHz         | 96 MHz   | 96000000       |
 *   | Apollo3P    | HFRC               | 48 MHz   | 48000000       |
 *
 *   The `nsx view` CLI command and the CMake `<app>_view` target handle
 *   this automatically via the per-SoC `NSX_SEGGER_CPUFREQ` setting in
 *   `cmake/segger/socs/<soc>.cmake`. When using JLink manually:
 *   ```
 *   JLinkSWOViewerCLExe -device <dev> -cpufreq <trace_freq> -swofreq 1000000 -itmport 0
 *   ```
 *
 * ### am_bsp_low_power_init() disables debug by default
 *   It calls `am_hal_debug_trace_disable()` and powers down the DEBUG
 *   peripheral for minimum power. If you need ITM/SWO, configure debug
 *   AFTER this call.
 *
 * ### C++ Static Constructors
 *   `__libc_init_array()` in the startup code runs C++ global constructors
 *   BEFORE main(). These run with hardware in reset-default state (no
 *   caches, no SIMOBUCK, no clock config). Keep global constructors
 *   lightweight — defer hardware-dependent init to main().
 *   The linker script MUST have `.preinit_array`, `.init_array`, and
 *   `.fini_array` sections or constructors silently won't run.
 *
 * ### Heap/Stack in TCM
 *   Both .stack and .heap are in MCU_TCM and are placed BEFORE .data
 *   and .bss. An oversized HEAP_SIZE (defined before including
 *   startup_gcc.c) consumes TCM budget and can push .data/.bss past
 *   the TCM boundary, causing a link-time overflow error. The heap is
 *   sized in uint32_t words, so HEAP_SIZE=1024 = 4KB.
 *   On Apollo3P, stack and heap SHARE a memory region — increasing
 *   HEAP_SIZE directly reduces stack space. On Apollo510+, they are
 *   separate sections in TCM.
 *
 * @copyright Copyright (c) 2024-2026, Ambiq Micro, Inc.
 */
#ifndef NSX_SYSTEM_H
#define NSX_SYSTEM_H

#include "ns_core.h"
#include "nsx_mem.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ===================================================================
 * Performance mode
 * =================================================================== */

/** @brief CPU performance / clock speed setting.
 *
 *  | SoC       | LOW         | MEDIUM      | HIGH        |
 *  |-----------|-------------|-------------|-------------|
 *  | Apollo3/P | 48 MHz      | Burst 96MHz | Burst 96MHz |
 *  | Apollo4/P | LP  96 MHz  | HP  192 MHz | HP  192 MHz |
 *  | Apollo510 | LP  96 MHz  | HP  192 MHz | HP  192 MHz |
 *  | AP330P    | LP  96 MHz  | HP1         | HP2         |
 */
typedef enum {
    NSX_PERF_LOW    = 0,   ///< Low-power mode
    NSX_PERF_MEDIUM = 1,   ///< Medium performance (HP1 on AP330P/510L)
    NSX_PERF_HIGH   = 2,   ///< High performance  (HP2 on AP330P/510L)
} nsx_perf_mode_e;

/* ===================================================================
 * Debug output configuration
 * =================================================================== */

/** @brief Debug output transport. */
typedef enum {
    NSX_DEBUG_NONE = 0,    ///< No debug output
    NSX_DEBUG_ITM  = 1,    ///< SWO/ITM (requires JLink + SWO pin)
    NSX_DEBUG_UART = 2,    ///< UART (requires BSP UART pins)
} nsx_debug_transport_e;

/**
 * @brief Debug output configuration.
 *
 * Controls ITM/SWO or UART printf output. Handles DCU enable,
 * TPIU/SWO pin config, and printf backend registration.
 */
typedef struct {
    nsx_debug_transport_e transport;  ///< Which output to enable
} nsx_debug_config_t;

/* ===================================================================
 * System configuration (one-shot convenience)
 * =================================================================== */

/**
 * @brief Complete system initialization configuration.
 *
 * Pass to nsx_system_init() for a single-call setup. Every field
 * has a safe default (zero-init = minimal, low-power, no debug).
 *
 * For fine-grained control, skip this and call individual helpers.
 */
typedef struct {
    nsx_perf_mode_e perf_mode;       ///< CPU performance mode
    bool            enable_cache;    ///< Enable I/D cache (unified on AP3/AP4, split on AP5)
    bool            enable_sram;     ///< Reserved — keep shared SRAM powered (not yet wired)

    nsx_debug_config_t debug;        ///< Debug output config

    bool            skip_bsp_init;   ///< Skip am_bsp_low_power_init() (use minimal HW init)
    bool            spot_mgr_profile;///< Enable SpotManager profile (Apollo5 family only)
} nsx_system_config_t;

/* ===================================================================
 * Preset configurations
 * =================================================================== */

/**
 * @brief Development preset: HP mode, caches, ITM, SRAM, SpotManager.
 *
 * Good for bring-up and debugging. Includes full BSP init.
 */
extern const nsx_system_config_t nsx_system_development;

/**
 * @brief Inference preset: HP mode, caches, no debug, SpotManager.
 *
 * Minimal footprint for production inference workloads.
 */
extern const nsx_system_config_t nsx_system_inference;

/**
 * @brief Minimal preset: zero-init safe defaults.
 *
 * Low-power mode, no caches, no debug. Use as a starting point
 * and enable only what you need.
 */
extern const nsx_system_config_t nsx_system_minimal;

/* ===================================================================
 * One-call system init
 * =================================================================== */

/**
 * @brief Initialize the system with a single call.
 *
 * Performs all steps in the correct order:
 *   1. ns_core_init()
 *   2. Hardware init (BSP or minimal)
 *   3. Cache enable (if requested)
 *   4. Performance mode
 *   5. SpotManager profile (if requested)
 *   6. Debug output (if requested)
 *
 * @param cfg  System configuration. Must not be NULL.
 * @return 0 on success, non-zero on failure.
 */
uint32_t nsx_system_init(const nsx_system_config_t *cfg);

/* ===================================================================
 * Individual startup building blocks
 *
 * Call these in order if you need fine-grained control.
 * See "Recommended Initialization Order" in the file header.
 * =================================================================== */

/**
 * @brief Full hardware init via BSP.
 *
 * Calls am_bsp_low_power_init() which, depending on SoC:
 *   - (AP510) Waits ~2 seconds, configures SIMOBUCK/CPDLP/clocks/SpotManager
 *   - (AP4)   Configures SIMOBUCK, clocks, peripherals, DAXI, cache
 *   - (AP3)   Configures cache, power, clocks
 *
 * On all SoCs this call disables debug/trace for minimum power.
 * Call nsx_debug_init() AFTER this if you need ITM/SWO.
 *
 * @return 0 on success.
 */
uint32_t nsx_hw_init(void);

/**
 * @brief Minimal hardware init (no BSP, no 2-second delay).
 *
 * Performs the minimum setup needed for basic operation:
 *   - Apollo5: CPDLP config (cache power domain), SpotManager init, FPU
 *   - Apollo4: Unified cache enable, FPU
 *   - Apollo3: Unified cache enable, FPU
 *
 * Does NOT configure SIMOBUCK, clock gates, or disable peripherals.
 * On Apollo5, caches are NOT enabled — call nsx_cache_enable() after.
 * On Apollo3/4, caches ARE enabled as part of minimal init.
 *
 * Use this when you want fast startup and don't need full low-power
 * optimization.
 *
 * @return 0 on success.
 */
uint32_t nsx_minimal_hw_init(void);

/**
 * @brief Set CPU performance mode.
 *
 * @param mode  Desired performance level.
 * @return 0 on success.
 */
uint32_t nsx_set_perf_mode(nsx_perf_mode_e mode);

/**
 * @brief Initialize debug output (ITM/SWO or UART).
 *
 * On all Apollo5 variants (AP510, AP330P, etc.) the Secure Bootloader locks
 * the DCU before transferring to user code.  This function unlocks SWO via
 * OTP + Crypto power-cycling and am_hal_dcu_update(), then configures TPIU
 * and ITM for printf output.
 *
 * Platform-specific behavior:
 *   - Apollo510/5B/5A: DCU unlock + manual TPIU (HFRC_96MHz, 96 MHz trace
 *     clock) + SWO pin (GPIO 28) + printf backend.
 *   - Apollo330P/510L: DCU unlock + am_bsp_itm_printf_enable() (XTAL_HS,
 *     48 MHz trace clock).
 *   - Apollo4P: BSP helper handles DCU + TPIU + SWO (GPIO 28) + printf.
 *   - Apollo3P: BSP helper handles TPIU + SWO (GPIO 41) + printf.
 *
 * @param cfg  Debug configuration. NULL disables debug output.
 * @return 0 on success.
 */
uint32_t nsx_debug_init(const nsx_debug_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* NSX_SYSTEM_H */
