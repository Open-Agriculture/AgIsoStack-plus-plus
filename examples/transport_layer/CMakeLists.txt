cmake_minimum_required(VERSION 3.16)
project(transport_layer_example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(isobus CONFIG)
find_package(Threads REQUIRED)

add_executable(TransportLayerExampleTarget main.cpp)
target_link_libraries(TransportLayerExampleTarget isobus::Isobus isobus::HardwareIntegration Threads::Threads isobus::SystemTiming)
