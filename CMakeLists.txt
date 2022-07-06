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

# Link GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

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
add_subdirectory("socket_can")

add_executable(address_claim_tests test/address_claim_test.cpp test/test_CAN_glue.cpp)
target_link_libraries(address_claim_tests gtest_main Isobus SocketCANInterface)

add_executable(integration_test_app test/main.cpp test/test_CAN_glue.cpp)
target_link_libraries(integration_test_app Isobus SocketCANInterface Threads::Threads)

include(GoogleTest)
gtest_discover_tests(address_claim_tests name_tests)



