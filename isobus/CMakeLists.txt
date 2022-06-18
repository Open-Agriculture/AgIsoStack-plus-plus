cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set library public name
set(ISOBUS_PUBLIC_NAME "ISOBUS")

# Set source and include directories
set(ISOBUS_SRC_DIR "src")
set(ISOBUS_INCLUDE_DIR "include")

# Set source files 
set(ISOBUS_SRC 
  "can_NAME.cpp"
  "can_lib_protocol.cpp"
  "can_identifier.cpp"
  "can_control_function.cpp"
  "can_message.cpp"
  "can_network_manager.cpp"
  "can_address_claim_state_machine.cpp"
  "can_internal_control_function.cpp"
  "can_lib_managed_message.cpp"
  "can_partnered_control_function.cpp"
  "can_NAME_filter.cpp"
  "can_transport_protocol.cpp"
  "can_lib_warning_logger.cpp"
  "can_lib_configuration.cpp"
)

# Prepend the source directory path to all the source files
PREPEND(ISOBUS_SRC ${ISOBUS_SRC_DIR} ${ISOBUS_SRC})

# Set the include files
set(ISOBUS_INCLUDE 
  "can_NAME.hpp"
  "can_lib_protocol.hpp"
  "can_lib_badge.hpp"
  "can_identifier.hpp"
  "can_control_function.hpp"
  "can_message.hpp"
  "can_lib_parameter_group_numbers.hpp"
  "can_network_manager.hpp"
  "can_address_claim_state_machine.hpp"
  "can_lib_managed_message.hpp"
  "can_NAME_filter.hpp"
  "can_transport_protocol.hpp"
  "can_lib_warning_logger.hpp"
  "can_lib_configuration.hpp"
  "can_lib_callback.hpp"
)

# Prepend the include directory path to all the include files
PREPEND(ISOBUS_INCLUDE ${ISOBUS_INCLUDE_DIR} ${ISOBUS_INCLUDE})

# Create the library from the source and include files
add_library(Isobus SHARED ${ISOBUS_SRC} ${ISOBUS_INCLUDE})

# Specify the include directory to be exported for other moduels to use. The 
# PUBLIC keyword here allows other libraries or exectuables to link to this
# library and use its functionality.
target_include_directories(Isobus PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${ISOBUS_PUBLIC_NAME}/include>
 ) 
 
target_link_libraries(Isobus PRIVATE SystemTiming)

# Add install instructions
set_target_properties(Isobus PROPERTIES PUBLIC_HEADER "${ISOBUS_INCLUDE}")

install(TARGETS Isobus EXPORT OdinTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_PREFIX}/${ISOBUS_PUBLIC_NAME}/${ISOBUS_INCLUDE_DIR}
)
