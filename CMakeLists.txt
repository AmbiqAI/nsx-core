if(NOT DEFINED NSX_BOARD_FLAGS_TARGET)
    message(FATAL_ERROR "Board must define NSX_BOARD_FLAGS_TARGET before ns-core is added.")
endif()

# Map NSX_SOC_FAMILY to architecture subdirectory
set(NSX_CORE_ARCH_DIR "")
if(NSX_SOC_FAMILY STREQUAL "apollo5b" OR NSX_SOC_FAMILY STREQUAL "apollo510"
   OR NSX_SOC_FAMILY STREQUAL "apollo510b" OR NSX_SOC_FAMILY STREQUAL "apollo5a"
   OR NSX_SOC_FAMILY STREQUAL "apollo330P" OR NSX_SOC_FAMILY STREQUAL "apollo510L")
    set(NSX_CORE_ARCH_DIR "apollo510")
elseif(NSX_SOC_FAMILY STREQUAL "apollo4p" OR NSX_SOC_FAMILY STREQUAL "apollo4l")
    set(NSX_CORE_ARCH_DIR "apollo4p")
elseif(NSX_SOC_FAMILY STREQUAL "apollo3p" OR NSX_SOC_FAMILY STREQUAL "apollo3")
    set(NSX_CORE_ARCH_DIR "apollo3p")
endif()

set(NSX_CORE_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/ns_core.c
    ${CMAKE_CURRENT_LIST_DIR}/src/nsx_system.c
)
if(NSX_CORE_ARCH_DIR)
    list(APPEND NSX_CORE_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/src/${NSX_CORE_ARCH_DIR}/nsx_system_platform.c
    )
endif()

add_library(nsx_core STATIC
    ${NSX_CORE_SOURCES}
)
set_target_properties(nsx_core PROPERTIES EXPORT_NAME core)
add_library(nsx::core ALIAS nsx_core)

target_link_libraries(nsx_core PUBLIC ${NSX_BOARD_FLAGS_TARGET} nsx_soc_hal)

target_include_directories(nsx_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/includes-api>
    $<BUILD_INTERFACE:${NSX_ROOT}/modules/nsx-harness/includes-api>
    $<BUILD_INTERFACE:${NSX_ROOT}/modules/nsx-utils/includes-api>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/nsx/modules/nsx-core/includes-api>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/nsx/modules/nsx-harness/includes-api>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/nsx/modules/nsx-utils/includes-api>
)

add_library(nsx_runtime_core INTERFACE)
set_target_properties(nsx_runtime_core PROPERTIES EXPORT_NAME runtime_core)
add_library(nsx::runtime_core ALIAS nsx_runtime_core)
add_library(nsx::neuralspot_core_min ALIAS nsx_runtime_core)
target_link_libraries(nsx_runtime_core INTERFACE nsx_core)

install(DIRECTORY
    ${CMAKE_CURRENT_LIST_DIR}/includes-api
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nsx/modules/nsx-core
)

install(TARGETS
    nsx_core
    nsx_runtime_core
    EXPORT nsxTargets
)
