cmake_minimum_required(VERSION 3.16)
project(guidance_example)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(GuidanceExampleTarget main.cpp console_logger.cpp)

set_target_properties(
  GuidanceExampleTarget
  PROPERTIES CXX_STANDARD 20
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON)

target_link_libraries(
  GuidanceExampleTarget PRIVATE isobus::Isobus isobus::HardwareIntegration
                                isobus::Utility Threads::Threads)
