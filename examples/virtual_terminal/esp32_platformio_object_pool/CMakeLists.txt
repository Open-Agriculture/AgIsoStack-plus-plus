cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(TestAgIsoStack)

target_add_binary_data(TestAgIsoStack.elf "src/object_pool/object_pool.iop"
                       BINARY)
