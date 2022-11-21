cmake_minimum_required(VERSION 3.16)
project(pgn_requests_example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(isobus CONFIG)
find_package(Threads REQUIRED)

add_executable(PGNRequestExampleTarget main.cpp)
target_link_libraries(PGNRequestExampleTarget isobus::Isobus isobus::HardwareIntegration Threads::Threads isobus::SystemTiming)
