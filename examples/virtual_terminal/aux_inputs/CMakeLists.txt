cmake_minimum_required(VERSION 3.16)
project(vt_aux_inputs_example)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(VTAuxInputsExample main.cpp ../../common/console_logger.cpp
                                  object_pool_ids.h)

target_compile_features(VTAuxInputsExample PUBLIC cxx_std_11)
set_target_properties(VTAuxInputsExample PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
  VTAuxInputsExample PRIVATE isobus::Isobus isobus::HardwareIntegration
                             Threads::Threads isobus::Utility)

add_custom_command(
  TARGET VTAuxInputsExample
  POST_BUILD
  COMMENT "Copying aux_inputs_pooldata.iop to build directory"
  COMMAND
    ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/aux_inputs_pooldata.iop
    $<TARGET_FILE_DIR:VTAuxInputsExample>/aux_inputs_pooldata.iop)
