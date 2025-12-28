cmake_minimum_required(VERSION 3.16)
project(iop_loader)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(iop_loader main.cpp ../../common/console_logger.cpp)

target_compile_features(iop_loader PUBLIC cxx_std_11)
set_target_properties(iop_loader PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
  iop_loader PRIVATE isobus::Isobus Threads::Threads
                     isobus::HardwareIntegration isobus::Utility)
