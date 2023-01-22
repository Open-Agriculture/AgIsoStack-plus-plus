cmake_minimum_required(VERSION 3.16)
project(transport_layer_example)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(isobus CONFIG) # Normally, you set REQUIRED, however if building
                            # from top level, it already exists
find_package(Threads REQUIRED)

add_executable(TransportLayerExampleTarget main.cpp)
target_link_libraries(
  TransportLayerExampleTarget
  PRIVATE isobus::Isobus isobus::HardwareIntegration Threads::Threads
          isobus::Utility)
