cmake_minimum_required(VERSION 3.16)
project(task_controller_server_example)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(TaskControllerServerExample main.cpp
                                           ../common/console_logger.cpp)

target_compile_features(TaskControllerServerExample PUBLIC cxx_std_11)
set_target_properties(TaskControllerServerExample PROPERTIES CXX_EXTENSIONS OFF)

target_link_libraries(
  TaskControllerServerExample
  PRIVATE isobus::Isobus isobus::HardwareIntegration Threads::Threads
          isobus::Utility)
