cmake_minimum_required(VERSION 3.16)
project(nmea2000_generator_example)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(NMEA2KGeneratorTarget main.cpp)

target_link_libraries(
  NMEA2KGeneratorTarget PRIVATE isobus::Isobus isobus::HardwareIntegration
                                Threads::Threads isobus::Utility)
