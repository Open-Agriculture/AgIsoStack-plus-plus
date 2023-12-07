cmake_minimum_required(VERSION 3.16)
project(nmea2000_parser_example)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(NMEA2KParserTarget main.cpp)

target_compile_features(NMEA2KParserTarget PUBLIC cxx_std_11)
set_target_properties(NMEA2KParserTarget PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
  NMEA2KParserTarget PRIVATE isobus::Isobus isobus::HardwareIntegration
                             Threads::Threads isobus::Utility)
