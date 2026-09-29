cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_INSTALL_INCLUDEDIR include)

function(prepend var prefix)
  set(listVar "")
  foreach(arg ${ARGN})
    list(APPEND listVar "${prefix}/${arg}")
  endforeach()
  set(${var} "${listVar}" PARENT_SCOPE)
endfunction()

if(NOT CONFIG_AGISOSTACK_CMSIS_RTOS2_THREADING)
  return()
endif()

# Zephyr provides its RTOS and compiler settings through this interface target.
set(CAN_DRIVER None)
set(CAN_STACK_DISABLE_THREADS OFF)
set(USE_CMSIS_RTOS2_THREADING ON)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../utility
                 ${CMAKE_CURRENT_BINARY_DIR}/agisostack_utility)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../isobus
                 ${CMAKE_CURRENT_BINARY_DIR}/agisostack_isobus)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../hardware_integration
                 ${CMAKE_CURRENT_BINARY_DIR}/agisostack_hardware_integration)

foreach(target Utility Isobus HardwareIntegration)
  target_link_libraries(${target} PUBLIC zephyr_interface)
endforeach()

# Applications can use the standard AgIsoStack++ component headers directly.
target_include_directories(Isobus INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
