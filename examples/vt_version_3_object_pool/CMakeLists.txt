cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(VT3ExampleTarget main.cpp)
target_link_libraries(VT3ExampleTarget Isobus SocketCANInterface Threads::Threads SystemTiming)
