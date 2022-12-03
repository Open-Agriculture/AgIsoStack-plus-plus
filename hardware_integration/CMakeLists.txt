cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set source and include directories
set(HARDWARE_INTEGRATION_SRC_DIR "src")
set(HARDWARE_INTEGRATION_INCLUDE_DIR "include/isobus/hardware_integration")

if(NOT CAN_DRIVER)
  message(AUTHOR_WARNING "No CAN driver specified, choosing Linux socket CAN by default. Use -DCAN_DRIVER=socketCAN to avoid this, or choose another driver and set it explicitly.")
  set(CAN_DRIVER "socketCAN")
endif()

if(CAN_DRIVER AND CAN_DRIVER MATCHES "socketCAN") 
	set(HARDWARE_INTEGRATION_SRC 
	  "can_hardware_interface.cpp"
	  "socket_can_interface.cpp"
	)

	# Set the include files
	set(HARDWARE_INTEGRATION_INCLUDE
	  "can_hardware_interface.hpp"
	  "can_hardware_plugin.hpp"
	  "socket_can_interface.hpp"
	)
endif()

# Prepend the source directory path to all the source files
PREPEND(HARDWARE_INTEGRATION_SRC ${HARDWARE_INTEGRATION_SRC_DIR} ${HARDWARE_INTEGRATION_SRC})

# Prepend the include directory path to all the include files
PREPEND(HARDWARE_INTEGRATION_INCLUDE ${HARDWARE_INTEGRATION_INCLUDE_DIR} ${HARDWARE_INTEGRATION_INCLUDE})

# Create the library from the source and include files
add_library(HardwareIntegration ${HARDWARE_INTEGRATION_SRC} ${HARDWARE_INTEGRATION_INCLUDE})
add_library(${PROJECT_NAME}::HardwareIntegration ALIAS HardwareIntegration)
target_link_libraries(HardwareIntegration PRIVATE ${PROJECT_NAME}::SystemTiming ${PROJECT_NAME}::Isobus)

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
