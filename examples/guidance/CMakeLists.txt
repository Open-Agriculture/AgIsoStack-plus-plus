cmake_minimum_required(VERSION 3.16)
project(guidance_example)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(GuidanceExampleTarget main.cpp console_logger.cpp)
target_link_libraries(
  GuidanceExampleTarget PRIVATE isobus::Isobus isobus::HardwareIntegration
                                isobus::Utility Threads::Threads)
