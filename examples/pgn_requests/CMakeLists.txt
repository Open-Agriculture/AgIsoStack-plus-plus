cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(PGNRequestExampleTarget main.cpp)
target_link_libraries(PGNRequestExampleTarget ${PROJECT_NAME}::Isobus ${PROJECT_NAME}::HardwareIntegration Threads::Threads ${PROJECT_NAME}::SystemTiming)
