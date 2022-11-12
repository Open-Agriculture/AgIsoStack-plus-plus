cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(VT3ExampleTarget main.cpp)
target_link_libraries(VT3ExampleTarget Isobus HardwareIntegration Threads::Threads SystemTiming)

add_custom_command(
        TARGET VT3ExampleTarget POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/VT3TestPool.iop
                ${CMAKE_CURRENT_BINARY_DIR}/VT3TestPool.iop)
