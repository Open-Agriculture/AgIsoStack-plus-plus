cmake_minimum_required(VERSION 3.16)

# Set library public name
set(ISOBUS_PUBLIC_NAME "ISOBUS")

# Set source and include directories
set(ISOBUS_SRC_DIR "src")
set(ISOBUS_INCLUDE_DIR "include/isobus/isobus")

# Set source files
set(ISOBUS_SRC
    "can_NAME.cpp"
    "can_identifier.cpp"
    "can_control_function.cpp"
    "can_message.cpp"
    "can_network_manager.cpp"
    "can_internal_control_function.cpp"
    "can_partnered_control_function.cpp"
    "can_NAME_filter.cpp"
    "can_transport_protocol.cpp"
    "can_transport_protocol_base.cpp"
    "can_stack_logger.cpp"
    "can_network_configuration.cpp"
    "can_callbacks.cpp"
    "can_message_frame.cpp"
    "isobus_virtual_terminal_client.cpp"
    "can_extended_transport_protocol.cpp"
    "isobus_diagnostic_protocol.cpp"
    "can_parameter_group_number_request_protocol.cpp"
    "nmea2000_fast_packet_protocol.cpp"
    "isobus_data_dictionary.cpp"
    "isobus_language_command_interface.cpp"
    "isobus_time_date_interface.cpp"
    "isobus_task_controller_client_objects.cpp"
    "isobus_task_controller_client.cpp"
    "isobus_device_descriptor_object_pool.cpp"
    "isobus_shortcut_button_interface.cpp"
    "isobus_functionalities.cpp"
    "isobus_guidance_interface.cpp"
    "isobus_speed_distance_messages.cpp"
    "isobus_maintain_power_interface.cpp"
    "isobus_virtual_terminal_objects.cpp"
    "isobus_virtual_terminal_client_state_tracker.cpp"
    "isobus_virtual_terminal_client_update_helper.cpp"
    "isobus_heartbeat.cpp"
    "isobus_task_controller_server.cpp"
    "isobus_task_controller_server_options.cpp"
    "nmea2000_message_definitions.cpp"
    "nmea2000_message_interface.cpp"
    "isobus_device_descriptor_object_pool_helpers.cpp"
    "can_message_data.cpp"
    "isobus_virtual_terminal_server.cpp"
    "isobus_virtual_terminal_working_set_base.cpp"
    "isobus_virtual_terminal_server_managed_working_set.cpp")

# Prepend the source directory path to all the source files
prepend(ISOBUS_SRC ${ISOBUS_SRC_DIR} ${ISOBUS_SRC})

# Set the include files
set(ISOBUS_INCLUDE
    "can_NAME.hpp"
    "can_badge.hpp"
    "can_identifier.hpp"
    "can_control_function.hpp"
    "can_message.hpp"
    "can_general_parameter_group_numbers.hpp"
    "can_network_manager.hpp"
    "can_NAME_filter.hpp"
    "can_transport_protocol.hpp"
    "can_transport_protocol_base.hpp"
    "can_stack_logger.hpp"
    "can_network_configuration.hpp"
    "can_callbacks.hpp"
    "can_message_frame.hpp"
    "can_hardware_abstraction.hpp"
    "can_internal_control_function.hpp"
    "can_partnered_control_function.hpp"
    "isobus_virtual_terminal_client.hpp"
    "can_extended_transport_protocol.hpp"
    "isobus_diagnostic_protocol.hpp"
    "can_parameter_group_number_request_protocol.hpp"
    "nmea2000_fast_packet_protocol.hpp"
    "isobus_data_dictionary.hpp"
    "isobus_virtual_terminal_objects.hpp"
    "isobus_language_command_interface.hpp"
    "isobus_time_date_interface.hpp"
    "isobus_standard_data_description_indices.hpp"
    "isobus_task_controller_client_objects.hpp"
    "isobus_task_controller_client.hpp"
    "isobus_device_descriptor_object_pool.hpp"
    "isobus_shortcut_button_interface.hpp"
    "isobus_functionalities.hpp"
    "isobus_speed_distance_messages.hpp"
    "isobus_maintain_power_interface.hpp"
    "isobus_virtual_terminal_client_state_tracker.hpp"
    "isobus_virtual_terminal_client_update_helper.hpp"
    "isobus_heartbeat.hpp"
    "isobus_task_controller_server.hpp"
    "isobus_task_controller_server_options.hpp"
    "nmea2000_message_definitions.hpp"
    "nmea2000_message_interface.hpp"
    "isobus_preferred_addresses.hpp"
    "isobus_device_descriptor_object_pool_helpers.hpp"
    "can_message_data.hpp"
    "isobus_virtual_terminal_base.hpp"
    "isobus_virtual_terminal_server.hpp"
    "isobus_virtual_terminal_working_set_base.hpp"
    "isobus_virtual_terminal_server_managed_working_set.hpp")

# Prepend the include directory path to all the include files
prepend(ISOBUS_INCLUDE ${ISOBUS_INCLUDE_DIR} ${ISOBUS_INCLUDE})

option(DISABLE_CAN_STACK_LOGGER
       "Compiles out all logging to minimize binary size" OFF)

option(DISABLE_ISOBUS_DATA_DICTIONARY
       "Disables the ISOBUS data dictionary to minimize binary size" OFF)

set(CAN_PORT_MAXIMUM_VALUE
    "4"
    CACHE STRING "Set the maximum number of CAN ports (1-16, default: 4)")
if(NOT CAN_PORT_MAXIMUM_VALUE MATCHES "^[0-9]+$")
  message(
    FATAL_ERROR
      "CAN_PORT_MAXIMUM_VALUE must be a positive integer, got: ${CAN_PORT_MAXIMUM_VALUE}"
  )
endif()
if(CAN_PORT_MAXIMUM_VALUE LESS 1 OR CAN_PORT_MAXIMUM_VALUE GREATER 16)
  message(
    FATAL_ERROR
      "CAN_PORT_MAXIMUM_VALUE must be between 1 and 16, got: ${CAN_PORT_MAXIMUM_VALUE}"
  )
endif()

# Create the library from the source and include files
add_library(Isobus ${ISOBUS_SRC} ${ISOBUS_INCLUDE})
add_library(${PROJECT_NAME}::Isobus ALIAS Isobus)

if(NOT CAN_PORT_MAXIMUM_VALUE STREQUAL "4")
  message(STATUS "CAN_PORT_MAXIMUM_VALUE set to: ${CAN_PORT_MAXIMUM_VALUE}")
endif()
target_compile_definitions(
  Isobus PUBLIC CAN_PORT_MAXIMUM_VALUE=${CAN_PORT_MAXIMUM_VALUE})

target_compile_features(Isobus PUBLIC cxx_std_11)
set_target_properties(Isobus PROPERTIES CXX_EXTENSIONS OFF)

if(DISABLE_CAN_STACK_LOGGER)
  message(STATUS "CAN Stack logger is disabled.")
  target_compile_definitions(Isobus PUBLIC DISABLE_CAN_STACK_LOGGER)
endif()

if(CAN_STACK_DISABLE_THREADS OR ARDUINO)
  message(STATUS "Disabled built-in multi-threading for CAN stack.")
  target_compile_definitions(Isobus PUBLIC CAN_STACK_DISABLE_THREADS)
endif()
if(DISABLE_ISOBUS_DATA_DICTIONARY)
  message(STATUS "ISOBUS data dictionary is disabled.")
  target_compile_definitions(Isobus PUBLIC DISABLE_ISOBUS_DATA_DICTIONARY)
endif()

# Specify the include directory to be exported for other moduels to use. The
# PUBLIC keyword here allows other libraries or exectuables to link to this
# library and use its functionality.
target_include_directories(
  Isobus PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(Isobus PRIVATE ${PROJECT_NAME}::Utility)

install(
  TARGETS Isobus
  EXPORT isobusTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin)

install(
  DIRECTORY include/
  DESTINATION include
  FILES_MATCHING
  PATTERN "*.hpp")
