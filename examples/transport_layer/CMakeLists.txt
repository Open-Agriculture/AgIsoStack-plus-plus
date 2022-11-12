cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(TransportLayerExampleTarget main.cpp)
target_link_libraries(TransportLayerExampleTarget Isobus HardwareIntegration Threads::Threads SystemTiming)
