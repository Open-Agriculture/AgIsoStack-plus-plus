cmake_minimum_required(VERSION 3.16)
project(vt_client_multiple_object_pools_example)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(VTClientMultiplePoolsExampleTarget main.cpp console_logger.cpp
                                                  objectPoolObjects.h)

target_compile_features(VTClientMultiplePoolsExampleTarget PUBLIC cxx_std_11)
set_target_properties(VTClientMultiplePoolsExampleTarget
                      PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
  VTClientMultiplePoolsExampleTarget
  PRIVATE isobus::Isobus isobus::HardwareIntegration Threads::Threads
          isobus::Utility)

add_custom_command(
  TARGET VTClientMultiplePoolsExampleTarget
  POST_BUILD
  COMMENT "Copying object pools to build directory"
  COMMAND
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/../version3_object_pool/VT3TestPool.iop
    $<TARGET_FILE_DIR:VTClientMultiplePoolsExampleTarget>/VT3TestPool.iop
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/window_masks.iop
          $<TARGET_FILE_DIR:VTClientMultiplePoolsExampleTarget>/window_masks.iop
)
