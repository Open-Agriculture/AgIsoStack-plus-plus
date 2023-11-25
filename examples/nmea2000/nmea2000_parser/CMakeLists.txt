cmake_minimum_required(VERSION 3.16)
project(nmea2000_parser_example)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(NMEA2KParserTarget main.cpp)

set_target_properties(
  NMEA2KParserTarget
  PROPERTIES CXX_STANDARD 20
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON)

target_link_libraries(
  NMEA2KParserTarget PRIVATE isobus::Isobus isobus::HardwareIntegration
                             Threads::Threads isobus::Utility)
