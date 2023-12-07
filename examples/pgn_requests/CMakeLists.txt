cmake_minimum_required(VERSION 3.16)
project(pgn_requests_example)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(PGNRequestExampleTarget main.cpp)

target_compile_features(PGNRequestExampleTarget PUBLIC cxx_std_11)
set_target_properties(PGNRequestExampleTarget PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
  PGNRequestExampleTarget PRIVATE isobus::Isobus isobus::HardwareIntegration
                                  Threads::Threads isobus::Utility)
