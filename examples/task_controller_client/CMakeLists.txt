cmake_minimum_required(VERSION 3.16)
project(task_controller_client_example)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(
  TaskControllerClientExample
  main.cpp console_logger.cpp section_control_implement_sim.cpp
  section_control_implement_sim.hpp)

set_target_properties(
  TaskControllerClientExample
  PROPERTIES CXX_STANDARD 14
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON)

target_link_libraries(
  TaskControllerClientExample
  PRIVATE isobus::Isobus isobus::HardwareIntegration Threads::Threads
          isobus::Utility)
