cmake_minimum_required(VERSION 3.16)
project(diagnostic_protocol_example)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(DiagnosticProtocolExample main.cpp)

target_compile_features(DiagnosticProtocolExample PUBLIC cxx_std_11)
set_target_properties(DiagnosticProtocolExample PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
  DiagnosticProtocolExample PRIVATE isobus::Isobus isobus::HardwareIntegration
                                    Threads::Threads isobus::Utility)
