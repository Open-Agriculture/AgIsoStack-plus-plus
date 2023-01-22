cmake_minimum_required(VERSION 3.16)
project(vt3_version_3_object_pool_example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(isobus CONFIG) # Normally, you set REQUIRED, however if building
                            # from top level, it already exists
find_package(Threads REQUIRED)

add_executable(VT3ExampleTarget main.cpp objectPoolObjects.h)
target_link_libraries(
  VT3ExampleTarget PRIVATE isobus::Isobus isobus::HardwareIntegration
                           Threads::Threads isobus::Utility)

add_custom_command(
  TARGET VT3ExampleTarget
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/VT3TestPool.iop
          ${CMAKE_CURRENT_BINARY_DIR}/VT3TestPool.iop)
