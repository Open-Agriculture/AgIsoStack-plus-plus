cmake_minimum_required(VERSION 3.16)
project(seeder_example)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(
  SeederExample
  main.cpp
  ../common/console_logger.cpp
  vt_application.cpp
  vt_application.hpp
  seeder.cpp
  seeder.hpp
  object_pool.hpp
  section_control_implement_sim.hpp
  section_control_implement_sim.cpp)
target_link_libraries(
  SeederExample PRIVATE isobus::Isobus isobus::HardwareIntegration
                        Threads::Threads isobus::Utility)

add_custom_command(
  TARGET SeederExample
  POST_BUILD
  COMMENT "Copying BasePool.iop to build directory"
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/BasePool.iop
          $<TARGET_FILE_DIR:SeederExample>/BasePool.iop)
