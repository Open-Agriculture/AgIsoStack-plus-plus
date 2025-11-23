cmake_minimum_required(VERSION 3.16)

find_package(GTest QUIET)
if(NOT GTest_FOUND)
  # Find GoogleTest
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0)

  # For Windows: Prevent overriding the parent project's compiler/linker
  # settings
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  # For before CMAKE 3.20, GTest::gtest_main didn't exist, so add it in All
  # linking to gtest should be done using the new GTest::gtest_main library for
  # forward compatability
  if(NOT TARGET GTest::gtest_main)
    add_library(GTest::gtest_main ALIAS GTest::Main)
  endif()
endif()

# For cmake<3.20 and older GTest
if(NOT TARGET GTest::gtest_main)
  set_target_properties(GTest::Main PROPERTIES IMPORTED_GLOBAL TRUE)
  add_library(GTest::gtest_main ALIAS GTest::Main)
endif()

# Set test include files
set(TEST_INCLUDE helpers/control_function_helpers.hpp
                 helpers/messaging_helpers.hpp)

# Set test source files
set(TEST_SRC
    core_network_management_tests.cpp
    identifier_tests.cpp
    transport_protocol_tests.cpp
    diagnostic_protocol_tests.cpp
    virtual_can_plugin_tests.cpp
    address_claim_tests.cpp
    can_name_tests.cpp
    hardware_interface_tests.cpp
    vt_client_tests.cpp
    language_command_interface_tests.cpp
    tc_client_tests.cpp
    ddop_tests.cpp
    event_dispatcher_tests.cpp
    isb_tests.cpp
    cf_functionalities_tests.cpp
    time_date_tests.cpp
    guidance_tests.cpp
    speed_distance_message_tests.cpp
    maintain_power_tests.cpp
    vt_object_tests.cpp
    nmea2000_message_tests.cpp
    isobus_data_dictionary_tests.cpp
    can_message_tests.cpp
    heartbeat_tests.cpp
    tc_server_tests.cpp
    helpers/control_function_helpers.cpp
    helpers/messaging_helpers.cpp)

add_executable(unit_tests ${TEST_SRC} ${TEST_INCLUDE})
set_target_properties(
  unit_tests
  PROPERTIES CXX_STANDARD 11
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON)
target_link_libraries(
  unit_tests
  PRIVATE GTest::gtest_main ${PROJECT_NAME}::Isobus
          ${PROJECT_NAME}::HardwareIntegration ${PROJECT_NAME}::Utility)

include(GoogleTest)
gtest_discover_tests(unit_tests name_tests identifier_tests)
