cmake_minimum_required(VERSION 3.16)
project(nmea2000_generator_example)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(NMEA2KGeneratorTarget main.cpp)

target_compile_features(NMEA2KGeneratorTarget PUBLIC cxx_std_11)
set_target_properties(NMEA2KGeneratorTarget PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
  NMEA2KGeneratorTarget PRIVATE isobus::Isobus isobus::HardwareIntegration
                                Threads::Threads isobus::Utility)
