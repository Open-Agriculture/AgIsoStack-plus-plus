cmake_minimum_required(VERSION 3.16)
project(can_hardware_bridge)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(CANHardwareBridge main.cpp)

target_compile_features(CANHardwareBridge PUBLIC cxx_std_11)
set_target_properties(CANHardwareBridge PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
  CANHardwareBridge PRIVATE isobus::Isobus isobus::HardwareIntegration
                            isobus::Utility Threads::Threads)
