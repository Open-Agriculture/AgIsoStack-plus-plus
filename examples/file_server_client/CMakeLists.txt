cmake_minimum_required(VERSION 3.16)
project(file_server_client_example)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT BUILD_EXAMPLES)
  find_package(isobus REQUIRED)
endif()
find_package(Threads REQUIRED)

add_executable(FileServerClientExampleTarget main.cpp console_logger.cpp)
target_link_libraries(FileServerClientExampleTarget Isobus Threads::Threads isobus::Utility isobus::HardwareIntegration)
