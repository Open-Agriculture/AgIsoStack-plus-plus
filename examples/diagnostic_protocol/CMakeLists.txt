cmake_minimum_required(VERSION 3.16)
project(diagnostic_protocol_example)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(DiagnosticProtocolExampleTarget main.cpp)

set_target_properties(
  DiagnosticProtocolExampleTarget
  PROPERTIES CXX_STANDARD 20
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON)

target_link_libraries(
  DiagnosticProtocolExampleTarget
  PRIVATE isobus::Isobus isobus::HardwareIntegration Threads::Threads
          isobus::Utility)
