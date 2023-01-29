cmake_minimum_required(VERSION 3.16)
project(vt_aux_n_example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(VTAuxNExample main.cpp console_logger.cpp object_pool_ids.h)
target_link_libraries(
  VTAuxNExample PRIVATE isobus::Isobus isobus::HardwareIntegration
                        Threads::Threads isobus::Utility)

add_custom_command(
  TARGET VTAuxNExample
  POST_BUILD
  COMMENT "Copying vtpooldata.iop to build directory"
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/vtpooldata.iop
          $<TARGET_FILE_DIR:VTAuxNExample>/vtpooldata.iop)
