if(NOT DEFINED NSX_BOARD_FLAGS_TARGET)
    message(FATAL_ERROR "Board must define NSX_BOARD_FLAGS_TARGET before ns-core is added.")
endif()

add_library(nsx_core STATIC
    ${CMAKE_CURRENT_LIST_DIR}/src/ns_core.c
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
