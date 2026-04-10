/**
 * @file nsx_mem.h
 * @brief Portable memory placement macros for NSX targets.
 *
 * Provides a uniform API for placing variables and functions in specific
 * physical memory regions across all supported Ambiq SoC families.
 *
 * ## Logical Memory Roles
 *
 *  | Macro              | Purpose                            | Init         |
 *  |--------------------|------------------------------------|--------------|
 *  | NSX_MEM_NVM        | Non-volatile (flash/MRAM) const    | In-place     |
 *  | NSX_MEM_FAST       | Fast data (TCM/tightly-coupled)    | Copy or zero |
 *  | NSX_MEM_FAST_BSS   | Fast data, zero-initialized        | Zeroed       |
 *  | NSX_MEM_SRAM       | Large SRAM, initialized from NVM   | Copy from NVM|
 *  | NSX_MEM_SRAM_BSS   | Large SRAM, zero-initialized       | Zeroed      |
 *  | NSX_MEM_FAST_CODE  | Code in tightly-coupled memory     | Copy from NVM|
 *
 * ## Graceful Degradation
 *
 * On SoCs without shared SRAM (e.g. Apollo3), NSX_MEM_SRAM and
 * NSX_MEM_SRAM_BSS fall back to the default data region. Code using
 * these macros is portable — it just won't get the large-SRAM benefit
 * on simpler parts.
 *
 * ## Usage Examples
 *
 * @code
 * // 64KB tensor arena — uninit, in SRAM, no boot copy cost
 * NSX_MEM_SRAM_BSS alignas(16) uint8_t g_arena[65536];
 *
 * // Model weights — init'd from NVM into SRAM for fast reads
 * NSX_MEM_SRAM alignas(16) uint8_t model[] = { 0x1c, ... };
 *
 * // Large const LUT — keep in NVM, don't waste RAM
 * NSX_MEM_NVM const int16_t big_lut[8192] = { ... };
 *
 * // Hot inner loop — run from tightly-coupled code memory
 * NSX_MEM_FAST_CODE void fast_isr(void) { ... }
 * @endcode
 *
 * @note  Place NSX_MEM_* before the type, after any storage class:
 *        `static NSX_MEM_SRAM_BSS uint8_t buf[1024];`
 *
 * @note  For SRAM placements to work at runtime, shared SRAM must be
 *        powered on. If using ns_power_config(), ensure need_ssram
 *        is true.
 *
 * @copyright Copyright (c) 2024-2026, Ambiq Micro, Inc.
 */
#ifndef NSX_MEM_H
#define NSX_MEM_H

#include "nsx_compiler.h"

/* ===================================================================
 * Compiler-portable helpers (delegated to nsx_compiler.h)
 * =================================================================== */
#define NSX_MEM__SEC(s)   NSX_SECTION(s)
#define NSX_MEM__USED     NSX_USED

/* ===================================================================
 * Per-SoC memory section mapping
 *
 * Each SoC defines which linker sections correspond to each logical
 * memory role. Add new SoC families by adding an #elif block.
 *
 * Section names must match the linker script output sections:
 *   .shared     → SHARED_SRAM, initialized from NVM  (AT>MCU_MRAM)
 *   .sram_bss   → SHARED_SRAM, NOLOAD
 *   .itcm_text  → MCU_ITCM, initialized from NVM     (AT>MCU_MRAM)
 *   .dtcm_text  → MCU_TCM, initialized from NVM      (AT>MCU_MRAM)
 *   .tcm        → TCM, initialized from NVM           (AT>ROMEM)
 *   .rodata     → NVM in-place
 * =================================================================== */

/* ------ Apollo510 / Apollo510B (Cortex-M55, ITCM + TCM + 3 MB SRAM) ------ */
#if defined(AM_PART_APOLLO510) || defined(AM_PART_APOLLO510B)
  #define NSX_MEM__HAS_SRAM        1
  #define NSX_MEM__HAS_SRAM_BSS    1
  #define NSX_MEM__HAS_FAST_CODE   1
  #define NSX_MEM__SEC_SRAM        ".shared"
  #define NSX_MEM__SEC_SRAM_BSS    ".sram_bss"
  #define NSX_MEM__SEC_FAST_CODE   ".itcm_text"

/* ------ Apollo5A / Apollo5B (Cortex-M55, ITCM + TCM + 3 MB SRAM) ------ */
#elif defined(AM_PART_APOLLO5A) || defined(AM_PART_APOLLO5B)
  #define NSX_MEM__HAS_SRAM        1
  #define NSX_MEM__HAS_SRAM_BSS    0  /* no .sram_bss in AP5A/5B linker */
  #define NSX_MEM__HAS_FAST_CODE   1
  #define NSX_MEM__SEC_SRAM        ".shared"
  /* NSX_MEM__SEC_SRAM_BSS not defined — falls back */
  #define NSX_MEM__SEC_FAST_CODE   ".itcm_text"

/* ------ Apollo510L / Apollo330P (no ITCM, uses DTCM overlay) ------ */
#elif defined(AM_PART_APOLLO510L) || defined(AM_PART_APOLLO330P)
  #define NSX_MEM__HAS_SRAM        1
  #define NSX_MEM__HAS_SRAM_BSS    1
  #define NSX_MEM__HAS_FAST_CODE   1
  #define NSX_MEM__SEC_SRAM        ".shared"
  #define NSX_MEM__SEC_SRAM_BSS    ".sram_bss"
  #define NSX_MEM__SEC_FAST_CODE   ".dtcm_text"

/* ------ Apollo4P / Apollo4L (Cortex-M4F, TCM + 1 MB SRAM) ------ */
#elif defined(AM_PART_APOLLO4P) || defined(AM_PART_APOLLO4L) || defined(AM_PART_APOLLO4)
  #define NSX_MEM__HAS_SRAM        1
  #define NSX_MEM__HAS_SRAM_BSS    1
  #define NSX_MEM__HAS_FAST_CODE   0
  #define NSX_MEM__SEC_SRAM        ".shared"
  #define NSX_MEM__SEC_SRAM_BSS    ".sram_bss"
  /* NSX_MEM__SEC_FAST_CODE not defined — falls back to NVM */

/* ------ Apollo3P (Cortex-M4F, 64 KB TCM + 768 KB SRAM) ------ */
#elif defined(AM_PART_APOLLO3P)
  #define NSX_MEM__HAS_SRAM        0  /* no shared SRAM concept */
  #define NSX_MEM__HAS_SRAM_BSS    0
  #define NSX_MEM__HAS_FAST_CODE   1
  #define NSX_MEM__SEC_FAST_CODE   ".tcm"

/* ------ Apollo3 (Cortex-M4F, single SRAM) ------ */
#elif defined(AM_PART_APOLLO3)
  #define NSX_MEM__HAS_SRAM        0
  #define NSX_MEM__HAS_SRAM_BSS    0
  #define NSX_MEM__HAS_FAST_CODE   0

/* ------ Unknown SoC — safe defaults (everything goes to compiler default) */
#else
  #define NSX_MEM__HAS_SRAM        0
  #define NSX_MEM__HAS_SRAM_BSS    0
  #define NSX_MEM__HAS_FAST_CODE   0
#endif

/* ===================================================================
 * Public API macros
 * =================================================================== */

/**
 * @brief Place data in non-volatile memory (MRAM/flash), no RAM copy.
 *
 * Use for large const data you want to keep out of RAM.
 * The variable should also be declared `const`.
 *
 * On GCC this targets .rodata which naturally lives in NVM.
 * The explicit section name documents intent and prevents the linker
 * from accidentally pulling it into a RAM-copied section.
 */
#define NSX_MEM_NVM  /* const data defaults to .rodata → NVM; no attribute needed */

/**
 * @brief Place initialized data in the fastest data memory (TCM).
 *
 * On all current SoCs, .data already targets TCM — this is a no-op
 * that documents intent. Initialized from NVM at boot.
 */
#define NSX_MEM_FAST  /* .data default is TCM on all current SoCs */

/**
 * @brief Place zero-initialized data in the fastest data memory (TCM).
 *
 * On all current SoCs, .bss already targets TCM — this is a no-op
 * that documents intent. Zeroed at boot.
 */
#define NSX_MEM_FAST_BSS  /* .bss default is TCM on all current SoCs */

/**
 * @brief Place initialized data in shared SRAM (copied from NVM at boot).
 *
 * Use for large buffers that need fast access but would overflow TCM.
 * Falls back to default (TCM) on SoCs without shared SRAM.
 *
 * @warning Requires shared SRAM to be powered on at runtime.
 */
#if NSX_MEM__HAS_SRAM
  #define NSX_MEM_SRAM  NSX_MEM__SEC(NSX_MEM__SEC_SRAM)
#else
  #define NSX_MEM_SRAM  /* fallback: default data region */
#endif

/**
 * @brief Place uninitialized data in shared SRAM (NOLOAD — not zeroed).
 *
 * Use for tensor arenas, DMA buffers, scratch space.
 * No boot-time copy cost. Falls back to default (TCM .bss) on SoCs
 * that lack a .sram_bss section.
 *
 * @note  Zeroed at boot by the startup code (same as .bss).
 * @warning Requires shared SRAM to be powered on at runtime.
 */
#if NSX_MEM__HAS_SRAM_BSS
  #define NSX_MEM_SRAM_BSS  NSX_MEM__SEC(NSX_MEM__SEC_SRAM_BSS)
#else
  #define NSX_MEM_SRAM_BSS  /* fallback: default .bss region */
#endif

/**
 * @brief Place code (or small data) in tightly-coupled code memory.
 *
 * Maps to ITCM on AP510/AP5x, DTCM on AP510L/AP330P, TCM on AP3P.
 * Falls back to NVM (normal .text) on SoCs without code TCM.
 * Copied from NVM at boot by the startup code.
 */
#if NSX_MEM__HAS_FAST_CODE
  #define NSX_MEM_FAST_CODE  NSX_MEM__SEC(NSX_MEM__SEC_FAST_CODE)
#else
  #define NSX_MEM_FAST_CODE  /* fallback: stays in NVM .text */
#endif

/* ===================================================================
 * Backward compatibility aliases
 *
 * These map legacy macros to the new NSX_MEM_* system.
 * Prefer NSX_MEM_* in new code.
 * =================================================================== */
#ifndef AM_SHARED_RW  /* don't override if SDK already defined it */
  #define AM_SHARED_RW   NSX_MEM_SRAM
#endif

/* Fix: NS_SRAM_BSS was broken on Apollo510/510B (empty expansion).
 * This provides a working definition for all SoCs. */
#undef NS_SRAM_BSS
#define NS_SRAM_BSS      NSX_MEM_SRAM_BSS

#undef NS_PUT_IN_TCM
#define NS_PUT_IN_TCM    NSX_MEM_FAST

/* ===================================================================
 * Cache helpers
 *
 * Lightweight cache control without the full ns_power_config() teardown.
 * Only available on SoCs with hardware I/D cache (AP4+).
 * =================================================================== */
#if defined(AM_PART_APOLLO510) || defined(AM_PART_APOLLO510B) || \
    defined(AM_PART_APOLLO5A) || defined(AM_PART_APOLLO5B)   || \
    defined(AM_PART_APOLLO510L) || defined(AM_PART_APOLLO330P)

  #include "am_mcu_apollo.h"

  /**
   * @brief Enable I-cache and D-cache.
   * Call after ns_core_init() for best inference / DSP performance.
   * @return 0 on success, non-zero if cache power domain not active
   *         (call am_hal_pwrctrl_low_power_init or configure CPDLP first).
   */
  static inline uint32_t nsx_cache_enable(void) {
      uint32_t st = am_hal_cachectrl_icache_enable();
      if (st != 0) return st;
      return am_hal_cachectrl_dcache_enable(true);
  }

  /** @brief Disable I-cache and D-cache. */
  static inline void nsx_cache_disable(void) {
      am_hal_cachectrl_dcache_disable();
      am_hal_cachectrl_icache_disable();
  }

  #define NSX_HAS_CACHE 1

#elif defined(AM_PART_APOLLO4P) || defined(AM_PART_APOLLO4L) || defined(AM_PART_APOLLO4)

  #include "am_mcu_apollo.h"

  /**
   * @brief Enable unified cache (Apollo4 family).
   * @return 0 on success.
   */
  static inline uint32_t nsx_cache_enable(void) {
      am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
      am_hal_cachectrl_enable();
      return 0;
  }

  static inline void nsx_cache_disable(void) {
      am_hal_cachectrl_disable();
  }

  #define NSX_HAS_CACHE 1

#else
  /* Apollo3 family — no separate cache controller (uses CACHECTRL differently) */
  /** @brief No-op cache enable for SoCs without a cache controller. */
  static inline uint32_t nsx_cache_enable(void) { return 0; }
  static inline void nsx_cache_disable(void) { /* no-op */ }

  #define NSX_HAS_CACHE 0

#endif

#endif /* NSX_MEM_H */
