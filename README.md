# nsx-core

`nsx-core` provides the common runtime initialization surface used by bare-metal NSX
applications.

## Contents

| Header | Purpose |
|--------|---------|
| `ns_core.h` | Core runtime init (`ns_core_init`) — must be called first |
| `nsx_system.h` | Modular system init: composable startup building blocks, presets, and documentation of boot sequence / gotchas |
| `nsx_mem.h` | Portable memory-placement macros (`NSX_MEM_SRAM`, `NSX_MEM_FAST_CODE`, etc.) and `nsx_cache_enable()` |

## nsx_system Quick Start

```c
#include "nsx_system.h"

int main(void) {
    // One-call init: HP mode, caches, ITM debug, SpotManager
    nsx_system_config_t cfg = nsx_system_development;
    cfg.skip_bsp_init = true;   // optional: skip 2s BSP delay
    nsx_system_init(&cfg);

    ns_printf("Hello from NSX\n");
}
```

Three built-in presets:

| Preset | Perf | Cache | Debug | SpotMgr | BSP init |
|--------|------|-------|-------|---------|----------|
| `nsx_system_development` | HIGH | yes | ITM | yes | yes |
| `nsx_system_inference` | HIGH | yes | none | yes | yes |
| `nsx_system_minimal` | LOW | no | none | no | skip |

Or call individual building blocks for fine-grained control — see the
header's "Recommended Initialization Order" section.

## Platform Backends

`nsx_system` is split into a platform-independent sequencing layer
(`src/nsx_system.c`) and per-SoC backends:

| Backend directory | SoCs covered |
|-------------------|--------------|
| `src/apollo3p/` | Apollo3, Apollo3P |
| `src/apollo4p/` | Apollo4P, Apollo4L |
| `src/apollo510/` | Apollo510, Apollo510B, Apollo510L, Apollo5A, Apollo5B, Apollo330P |

CMake selects the correct backend via `NSX_SOC_FAMILY`.

## nsx_mem Memory Placement

```c
NSX_MEM_SRAM_BSS alignas(16) uint8_t arena[65536];  // shared SRAM, zeroed
NSX_MEM_SRAM     const uint8_t weights[] = {...};    // shared SRAM, from NVM
NSX_MEM_FAST_CODE void hot_isr(void) { ... }         // ITCM (AP510) / TCM (AP3P)
```

Macros degrade gracefully on simpler SoCs (fall back to default sections).

This repo is CMake-first. Legacy Make integration files are intentionally omitted.
