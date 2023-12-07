cmake_minimum_required(VERSION 3.16)
project(transport_layer_example)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(TransportLayerExampleTarget main.cpp)

target_compile_features(TransportLayerExampleTarget PUBLIC cxx_std_11)
set_target_properties(TransportLayerExampleTarget PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
  TransportLayerExampleTarget
  PRIVATE isobus::Isobus isobus::HardwareIntegration Threads::Threads
          isobus::Utility)
