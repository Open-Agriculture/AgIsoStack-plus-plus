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
  main.cpp console_logger.cpp vt_application.cpp vt_application.hpp seeder.cpp seeder.hpp)
target_link_libraries(
  SeederExample
  PRIVATE isobus::Isobus isobus::HardwareIntegration Threads::Threads
          isobus::Utility)
