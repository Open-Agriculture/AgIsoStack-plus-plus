# This file was automatically generated for projects without default
# 'CMakeLists.txt' file.

file(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources} REQUIRES driver)

# Embeds the object pool into the firmware image so it can be referenced via the
# _binary_BasePool_iop_start / _end symbols (see src/vt_application.cpp). There
# is no filesystem on the ESP32 to read BasePool.iop from at runtime.
target_add_binary_data(${COMPONENT_TARGET} "object_pool/BasePool.iop" BINARY)

# AgIsoStack++ requires C++17.
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
