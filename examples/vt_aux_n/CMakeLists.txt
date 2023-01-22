cmake_minimum_required(VERSION 3.16)
project(vt_aux_n_example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(isobus CONFIG) # Normally, you set REQUIRED, however if building
                            # from top level, it already exists
find_package(Threads REQUIRED)

add_executable(VTAuxNExample main.cpp object_pool_ids.h)
target_link_libraries(
  VTAuxNExample PRIVATE isobus::Isobus isobus::HardwareIntegration
                           Threads::Threads isobus::Utility)

add_custom_command(
  TARGET VTAuxNExample
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/vtpooldata.iop
          $<TARGET_FILE_DIR:VTAuxNExample>/vtpooldata.iop)
