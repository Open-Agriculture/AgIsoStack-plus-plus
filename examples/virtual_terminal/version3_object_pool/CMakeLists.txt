cmake_minimum_required(VERSION 3.16)
project(vt3_version_3_object_pool_example)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(VT3ExampleTarget main.cpp ../../common/console_logger.cpp
                                objectPoolObjects.h)

target_compile_features(VT3ExampleTarget PUBLIC cxx_std_11)
set_target_properties(VT3ExampleTarget PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
  VT3ExampleTarget PRIVATE isobus::Isobus isobus::HardwareIntegration
                           Threads::Threads isobus::Utility)

add_custom_command(
  TARGET VT3ExampleTarget
  POST_BUILD
  COMMENT "Copying VT3TestPool.iop to build directory"
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/VT3TestPool.iop
          $<TARGET_FILE_DIR:VT3ExampleTarget>/VT3TestPool.iop)
