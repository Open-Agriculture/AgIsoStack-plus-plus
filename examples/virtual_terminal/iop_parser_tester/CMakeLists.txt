cmake_minimum_required(VERSION 3.16)
project(iop_parser)

include(GNUInstallDirs)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(iop_parser main.cpp ../../common/console_logger.cpp)

install(TARGETS iop_parser RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

target_compile_features(iop_parser PUBLIC cxx_std_11)
set_target_properties(iop_parser PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
  iop_parser PRIVATE isobus::Isobus Threads::Threads
                     isobus::HardwareIntegration isobus::Utility)
