cmake_minimum_required(VERSION 3.16)
project(guidance_example)

include(GNUInstallDirs)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(GuidanceExampleTarget main.cpp ../common/console_logger.cpp
                                     ../common/create_can_driver.cpp)

install(TARGETS GuidanceExampleTarget
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")

target_compile_features(GuidanceExampleTarget PUBLIC cxx_std_11)
set_target_properties(GuidanceExampleTarget PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
  GuidanceExampleTarget PRIVATE isobus::Isobus isobus::HardwareIntegration
                                isobus::Utility Threads::Threads)
