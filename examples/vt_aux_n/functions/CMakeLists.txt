cmake_minimum_required(VERSION 3.16)
project(vt_aux_functions_example)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(VTAuxFunctionsExample main.cpp console_logger.cpp
                                     object_pool_ids.h)
target_link_libraries(
  VTAuxFunctionsExample PRIVATE isobus::Isobus isobus::HardwareIntegration
                                Threads::Threads isobus::Utility)

add_custom_command(
  TARGET VTAuxFunctionsExample
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/aux_functions_pooldata.iop
    $<TARGET_FILE_DIR:VTAuxFunctionsExample>/aux_functions_pooldata.iop)
