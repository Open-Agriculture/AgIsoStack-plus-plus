cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set source and include directories
set(HARDWARE_INTEGRATION_SRC_DIR "src")
set(HARDWARE_INTEGRATION_INCLUDE_DIR "include/isobus/hardware_integration")

if(NOT CAN_DRIVER)
    message(AUTHOR_WARNING "No CAN driver specified, choosing Linux socket CAN by default. Use -DCAN_DRIVER=\"SocketCAN\" to specify another driver. or -DCAN_DRIVER=\"SocketCAN;WindowsPCANBasic\" to specify multiple drivers.")
	set(CAN_DRIVER "SocketCAN")
endif()

# Set the source files
set(HARDWARE_INTEGRATION_SRC 
  "can_hardware_interface.cpp"
)

# Set the include files
set(HARDWARE_INTEGRATION_INCLUDE
  "can_hardware_interface.hpp"
  "can_hardware_plugin.hpp"
)

# Add the source/include files based on the CAN driver chosen
if ("SocketCAN" IN_LIST CAN_DRIVER)
	list(APPEND HARDWARE_INTEGRATION_SRC "socket_can_interface.cpp")
	list(APPEND HARDWARE_INTEGRATION_INCLUDE "socket_can_interface.hpp")
endif()
if ("WindowsPCANBasic" IN_LIST CAN_DRIVER)
	list(APPEND HARDWARE_INTEGRATION_SRC "pcan_basic_windows_plugin.cpp")
	list(APPEND HARDWARE_INTEGRATION_INCLUDE "pcan_basic_windows_plugin.hpp")
endif()

# Prepend the source directory path to all the source files
PREPEND(HARDWARE_INTEGRATION_SRC ${HARDWARE_INTEGRATION_SRC_DIR} ${HARDWARE_INTEGRATION_SRC})

# Prepend the include directory path to all the include files
PREPEND(HARDWARE_INTEGRATION_INCLUDE ${HARDWARE_INTEGRATION_INCLUDE_DIR} ${HARDWARE_INTEGRATION_INCLUDE})

# Create the library from the source and include files
add_library(HardwareIntegration ${HARDWARE_INTEGRATION_SRC} ${HARDWARE_INTEGRATION_INCLUDE})
add_library(${PROJECT_NAME}::HardwareIntegration ALIAS HardwareIntegration)
target_link_libraries(HardwareIntegration PRIVATE ${PROJECT_NAME}::SystemTiming ${PROJECT_NAME}::Isobus)

if ("WindowsPCANBasic" IN_LIST CAN_DRIVER)
	if(MSVC)
		# See https://gitlab.kitware.com/cmake/cmake/-/issues/15170
		set(CMAKE_SYSTEM_PROCESSOR ${MSVC_CXX_ARCHITECTURE_ID})
	endif()

	message(STATUS "Target Arch: ${CMAKE_SYSTEM_PROCESSOR}")
	if(CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x64")
		message(STATUS "Detected AMD64, linking to PCAN x64 Library")
			target_link_libraries(HardwareIntegration PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lib/Windows/PCANBasic_x64.lib)
			add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lib/Windows/PCANBasic_x64.lib
			COMMAND ${CMAKE_COMMAND} -E copy
					${CMAKE_CURRENT_LIST_DIR}/lib/Windows/PCANBasic_x64.lib
					${CMAKE_CURRENT_BINARY_DIR}/PCANBasic_x64.lib)
	elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
		message(STATUS "Detected ARM64, linking to PCAN ARM64 Library")
		target_link_libraries(HardwareIntegration PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lib/Windows/PCANBasic_ARM64.lib)
		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lib/Windows/PCANBasic_ARM64.lib
			COMMAND ${CMAKE_COMMAND} -E copy
					${CMAKE_CURRENT_LIST_DIR}/lib/Windows/PCANBasic_ARM64.lib
					${CMAKE_CURRENT_BINARY_DIR}/PCANBasic_ARM64.lib)
	elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "X86")
		message(STATUS "Detected x86, linking to PCAN x86 Library")
		target_link_libraries(HardwareIntegration PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lib/Windows/PCANBasic_x86.lib)
		add_custom_command(
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/lib/Windows/PCANBasic_x86.lib
			COMMAND ${CMAKE_COMMAND} -E copy
					${CMAKE_CURRENT_LIST_DIR}/lib/Windows/PCANBasic_x86.lib
					${CMAKE_CURRENT_BINARY_DIR}/PCANBasic_x86.lib)
	else()
		message(FATAL_ERROR "Windows PCAN Selected but no supported processor arch was detected. Only x64, x86, and ARM64 are supported by PEAK's drivers.")
	endif()
endif()

# Specify the include directory to be exported for other moduels to use. The 
# PUBLIC keyword here allows other libraries or exectuables to link to this
# library and use its functionality.
target_include_directories(HardwareIntegration PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

 install(TARGETS HardwareIntegration EXPORT IsobusTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
)
