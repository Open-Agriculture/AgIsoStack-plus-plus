cmake_minimum_required(VERSION 3.16)
project(fast_packet_protocol_example)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(NMEA2KFastPacketTarget main.cpp)

target_compile_features(NMEA2KFastPacketTarget PUBLIC cxx_std_11)
set_target_properties(NMEA2KFastPacketTarget PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
  NMEA2KFastPacketTarget PRIVATE isobus::Isobus isobus::HardwareIntegration
                                 Threads::Threads isobus::Utility)
