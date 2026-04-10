/**
 * @file nsx_compiler.h
 * @brief Portable compiler-attribute macros for NSX modules.
 *
 * Abstracts GCC, Arm Compiler 6 (armclang), and IAR intrinsics behind
 * a single set of macros so NSX module code stays toolchain-agnostic.
 *
 * ## Supported Compilers
 *
 *  | Compiler     | Detection macro               | NSX family define        |
 *  |--------------|-------------------------------|--------------------------|
 *  | GCC          | `__GNUC__` w/o `__ARMCC_VERSION` | `NSX_COMPILER_GCC`    |
 *  | armclang v6  | `__ARMCC_VERSION >= 6000000`  | `NSX_COMPILER_ARMCLANG`  |
 *  | IAR          | `__IAR_SYSTEMS_ICC__`         | `NSX_COMPILER_IAR`       |
 *
 * ## Usage
 *
 * @code
 * #include "nsx_compiler.h"
 *
 * NSX_SECTION(".shared") NSX_USED uint8_t buf[4096];
 *
 * NSX_WEAK void my_handler(void) { }
 *
 * NSX_PACKED_BEGIN
 * typedef struct {
 *     uint8_t  tag;
 *     uint32_t value;
 * } NSX_PACKED_ATTR my_pkt_t;
 * NSX_PACKED_END
 * @endcode
 *
 * @copyright Copyright (c) 2024-2026, Ambiq Micro, Inc.
 */
#ifndef NSX_COMPILER_H
#define NSX_COMPILER_H

/* ===================================================================
 * Compiler family detection
 * =================================================================== */
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6000000)
  #define NSX_COMPILER_ARMCLANG  1
  #define NSX_COMPILER_GCC       0
  #define NSX_COMPILER_IAR       0
#elif defined(__GNUC__)
  #define NSX_COMPILER_GCC       1
  #define NSX_COMPILER_ARMCLANG  0
  #define NSX_COMPILER_IAR       0
#elif defined(__IAR_SYSTEMS_ICC__)
  #define NSX_COMPILER_IAR       1
  #define NSX_COMPILER_GCC       0
  #define NSX_COMPILER_ARMCLANG  0
#else
  #define NSX_COMPILER_GCC       0
  #define NSX_COMPILER_ARMCLANG  0
  #define NSX_COMPILER_IAR       0
#endif

/* ===================================================================
 * Attribute macros — portable across GCC, armclang, IAR
 * =================================================================== */

/**
 * @brief Place a variable or function in a named linker section.
 * @param s  Section name string, e.g. ".shared", ".itcm_text"
 */
#if NSX_COMPILER_GCC || NSX_COMPILER_ARMCLANG
  #define NSX_SECTION(s)    __attribute__((section(s)))
#elif NSX_COMPILER_IAR
  #define NSX_SECTION(s)    @ s
#else
  #define NSX_SECTION(s)
#endif

/** @brief Prevent the linker from discarding an otherwise-unreferenced symbol. */
#if NSX_COMPILER_GCC || NSX_COMPILER_ARMCLANG
  #define NSX_USED          __attribute__((used))
#elif NSX_COMPILER_IAR
  #define NSX_USED          __root
#else
  #define NSX_USED
#endif

/** @brief Declare a symbol as weak (overridable by a strong definition). */
#if NSX_COMPILER_GCC || NSX_COMPILER_ARMCLANG
  #define NSX_WEAK          __attribute__((weak))
#elif NSX_COMPILER_IAR
  #define NSX_WEAK          __weak
#else
  #define NSX_WEAK
#endif

/** @brief Align to *n* bytes. */
#if NSX_COMPILER_GCC || NSX_COMPILER_ARMCLANG
  #define NSX_ALIGNED(n)    __attribute__((aligned(n)))
#elif NSX_COMPILER_IAR
  #define NSX_ALIGNED(n)    _Pragma("data_alignment=" #n)
#else
  #define NSX_ALIGNED(n)
#endif

/** @brief Mark a function as never returning. */
#if NSX_COMPILER_GCC || NSX_COMPILER_ARMCLANG
  #define NSX_NORETURN      __attribute__((noreturn))
#elif NSX_COMPILER_IAR
  #define NSX_NORETURN      __noreturn
#else
  #define NSX_NORETURN
#endif

/** @brief Prevent inlining of a function. */
#if NSX_COMPILER_GCC || NSX_COMPILER_ARMCLANG
  #define NSX_NOINLINE      __attribute__((noinline))
#elif NSX_COMPILER_IAR
  #define NSX_NOINLINE      _Pragma("optimize=no_inline")
#else
  #define NSX_NOINLINE
#endif

/** @brief Force inlining of a function. */
#if NSX_COMPILER_GCC || NSX_COMPILER_ARMCLANG
  #define NSX_ALWAYS_INLINE __attribute__((always_inline)) inline
#elif NSX_COMPILER_IAR
  #define NSX_ALWAYS_INLINE _Pragma("inline=forced")
#else
  #define NSX_ALWAYS_INLINE inline
#endif

/* -------------------------------------------------------------------
 * Packed structures
 *
 * Usage:
 *   NSX_PACKED_BEGIN
 *   typedef struct {
 *       uint8_t  tag;
 *       uint32_t value;
 *   } NSX_PACKED_ATTR my_pkt_t;
 *   NSX_PACKED_END
 * ------------------------------------------------------------------- */
#if NSX_COMPILER_GCC || NSX_COMPILER_ARMCLANG
  #define NSX_PACKED_ATTR   __attribute__((packed))
  #define NSX_PACKED_BEGIN
  #define NSX_PACKED_END
#elif NSX_COMPILER_IAR
  #define NSX_PACKED_ATTR
  #define NSX_PACKED_BEGIN  _Pragma("pack(push, 1)")
  #define NSX_PACKED_END    _Pragma("pack(pop)")
#else
  #define NSX_PACKED_ATTR
  #define NSX_PACKED_BEGIN
  #define NSX_PACKED_END
#endif

/* ===================================================================
 * Newlib / retarget detection
 *
 * GCC bare-metal targets use newlib and need __wrap_* stubs.
 * armclang uses its own retarget mechanism.
 * =================================================================== */
#if NSX_COMPILER_GCC
  #define NSX_HAS_NEWLIB   1
#else
  #define NSX_HAS_NEWLIB   0
#endif

#endif /* NSX_COMPILER_H */
