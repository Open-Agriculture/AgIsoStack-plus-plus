cmake_minimum_required(VERSION 3.16)

project(
  isobus
  VERSION 0.1
  LANGUAGES CXX
  DESCRIPTION "A control function focused implementation of the major ISOBUS and J1939 transport layers"
)

set(CMAKE_BUILD_TYPE Debug)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Default to C99
if(NOT CMAKE_C_STANDARD)
set(CMAKE_C_STANDARD 99)
endif()

# Default to C++11
if(NOT CMAKE_CXX_STANDARD)
set(CMAKE_CXX_STANDARD 11)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find packages required for Socket CAN
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# A handy function to PREPEND text to all elements in a list (useful for subdirectories)
FUNCTION(PREPEND var prefix)
   SET(listVar "")
   FOREACH(f ${ARGN})
      LIST(APPEND listVar "${prefix}/${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

# Add subdirectories
add_subdirectory("utility")
add_subdirectory("isobus")
add_subdirectory("hardware_integration")

if(NOT NO_EXAMPLES)
    add_subdirectory("examples/vt_version_3_object_pool")
    add_subdirectory("examples/transport_layer")
    add_subdirectory("examples/diagnostic_protocol")
    add_subdirectory("examples/pgn_requests")
endif()

# Make CTest available which adds the option BUILD_TESTING
include(CTest)
if(BUILD_TESTING)
find_package(GTest QUIET)
if(NOT GTest_FOUND)
  # Find GoogleTest
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
  )
  # For Windows: Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  # For before CMAKE 3.20, GTest::gtest_main didn't exist, so add it in
  # All linking to gtest should be done using the new GTest::gtest_main library for forward compatability
  if(NOT TARGET GTest::gtest_main)
    add_library(GTest::gtest_main ALIAS GTest::Main)
  endif()
endif()

# For cmake<3.20 and older GTest
if(NOT TARGET GTest::gtest_main)
  set_target_properties(GTest::Main PROPERTIES IMPORTED_GLOBAL TRUE)
  add_library(GTest::gtest_main ALIAS GTest::Main)
endif()

add_executable(unit_tests test/address_claim_test.cpp test/test_CAN_glue.cpp test/identifier_tests.cpp)
target_link_libraries(unit_tests GTest::gtest_main ${PROJECT_NAME}::Isobus ${PROJECT_NAME}::HardwareIntegration)

include(GoogleTest)
gtest_discover_tests(unit_tests name_tests identifier_tests)
endif()
