cmake_minimum_required(VERSION 3.20.0)

list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR}/../../..)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(agisostack_zephyr_minimal)

target_sources(app PRIVATE src/main.cpp)
target_link_libraries(app PRIVATE Isobus HardwareIntegration Utility)
