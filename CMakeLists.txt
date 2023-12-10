cmake_minimum_required(VERSION 3.16)

project(
  isobus
  VERSION 0.1
  LANGUAGES CXX
  DESCRIPTION
    "A control function focused implementation of the major ISOBUS and J1939 transport layers"
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

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
  add_subdirectory("examples/nmea2000/fast_packet_protocol")
  add_subdirectory("examples/nmea2000/nmea2000_generator")
  add_subdirectory("examples/nmea2000/nmea2000_parser")
  add_subdirectory("examples/virtual_terminal/version3_object_pool")
  add_subdirectory("examples/virtual_terminal/aux_functions")
  add_subdirectory("examples/virtual_terminal/aux_inputs")
  add_subdirectory("examples/task_controller_client")
  add_subdirectory("examples/guidance")
endif()

if(BUILD_TESTING)
  add_subdirectory("test")
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
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE
    "https://github.com/Open-Agriculture/AgIsoStack-plus-plus")
set(CPACK_PACKAGE_VENDOR "Open-Agriculture")
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
include(CPack)
