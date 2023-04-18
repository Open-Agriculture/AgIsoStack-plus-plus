cmake_minimum_required(VERSION 3.16)

project(
  isobus
  VERSION 0.1
  LANGUAGES CXX
  DESCRIPTION
    "A control function focused implementation of the major ISOBUS and J1939 transport layers"
)

set(CMAKE_BUILD_TYPE Debug)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

# Make CTest available which adds the option BUILD_TESTING
include(CTest)
if(BUILD_TESTING
   AND NOT MSVC
   AND NOT APPLE)
  # Set --coverage flag for gcovr (SonarCloud)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find packages required for Socket CAN
set(THREADS_PREFER_PTHREAD_FLAG ON)
# ESP-IDF doesn't implement find_package(Threads) correctly (dec 2022)
if(NOT ESP_PLATFORM)
  find_package(Threads REQUIRED)
endif()

# A handy function to prepend text to all elements in a list (useful for
# subdirectories)
function(prepend var prefix)
  set(listVar "")
  foreach(arg ${ARGN})
    list(APPEND listVar "${prefix}/${arg}")
  endforeach(arg)
  set(${var}
      "${listVar}"
      PARENT_SCOPE)
endfunction(prepend)

# Add subdirectories
add_subdirectory("utility")
add_subdirectory("isobus")
add_subdirectory("hardware_integration")

option(BUILD_EXAMPLES "Set to ON to enable building of examples from top level"
       OFF)
if(BUILD_EXAMPLES)
  add_subdirectory("examples/transport_layer")
  add_subdirectory("examples/diagnostic_protocol")
  add_subdirectory("examples/pgn_requests")
  add_subdirectory("examples/nmea2000")
  add_subdirectory("examples/virtual_terminal/version3_object_pool")
  add_subdirectory("examples/virtual_terminal/aux_functions")
  add_subdirectory("examples/virtual_terminal/aux_inputs")
  add_subdirectory("examples/task_controller_client")
endif()

if(BUILD_TESTING)
  find_package(GTest QUIET)
  if(NOT GTest_FOUND)
    # Find GoogleTest
    include(FetchContent)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG release-1.12.1)

    # For Windows: Prevent overriding the parent project's compiler/linker
    # settings
    set(gtest_force_shared_crt
        ON
        CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # For before CMAKE 3.20, GTest::gtest_main didn't exist, so add it in All
    # linking to gtest should be done using the new GTest::gtest_main library
    # for forward compatability
    if(NOT TARGET GTest::gtest_main)
      add_library(GTest::gtest_main ALIAS GTest::Main)
    endif()
  endif()

  # For cmake<3.20 and older GTest
  if(NOT TARGET GTest::gtest_main)
    set_target_properties(GTest::Main PROPERTIES IMPORTED_GLOBAL TRUE)
    add_library(GTest::gtest_main ALIAS GTest::Main)
  endif()

  # Set test source files
  set(TEST_SRC
      test/identifier_tests.cpp
      test/diagnostic_protocol_tests.cpp
      test/core_network_management_tests.cpp
      test/virtual_can_plugin_tests.cpp
      test/address_claim_tests.cpp
      test/can_name_tests.cpp
      test/hardware_interface_tests.cpp
      test/vt_client_tests.cpp
      test/language_command_interface_tests.cpp
      test/tc_client_tests.cpp
      test/ddop_tests.cpp
      test/event_dispatcher_tests.cpp
      test/isb_tests.cpp)

  add_executable(unit_tests ${TEST_SRC})
  target_link_libraries(
    unit_tests
    PRIVATE GTest::gtest_main ${PROJECT_NAME}::Isobus
            ${PROJECT_NAME}::HardwareIntegration ${PROJECT_NAME}::Utility)

  include(GoogleTest)
  gtest_discover_tests(unit_tests name_tests identifier_tests)
endif()

install(
  TARGETS Isobus Utility HardwareIntegration
  EXPORT isobusTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin)

# Install all the header files for all the libraries
install(
  DIRECTORY isobus/include/
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.hpp")
install(
  DIRECTORY hardware_integration/include/
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.hpp")
install(
  DIRECTORY utility/include/
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.hpp")

install(
  EXPORT isobusTargets
  FILE isobusTargets.cmake
  NAMESPACE isobus::
  DESTINATION lib/cmake/isobus)
configure_file(cmake/isobusConfig.cmake.in isobusConfig.cmake @ONLY)

write_basic_package_version_file(isobusConfigVersion.cmake
                                 COMPATIBILITY SameMajorVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/isobusConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/isobusConfigVersion.cmake"
        DESTINATION lib/cmake/isobus)

set(CPACK_PACKAGE_CONTACT "delgrossoengineering@protonmail.com")
set(CPACK_DEBIAN_FILENAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/ad3154/Isobus-plus-plus")
set(CPACK_PACKAGE_VENDOR "https://github.com/ad3154/Isobus-plus-plus")
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
include(CPack)
