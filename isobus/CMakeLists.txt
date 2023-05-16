cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set library public name
set(ISOBUS_PUBLIC_NAME "ISOBUS")

# Set source and include directories
set(ISOBUS_SRC_DIR "src")
set(ISOBUS_INCLUDE_DIR "include/isobus/isobus")

# Set source files
set(ISOBUS_SRC
    "can_NAME.cpp"
    "can_protocol.cpp"
    "can_identifier.cpp"
    "can_control_function.cpp"
    "can_message.cpp"
    "can_network_manager.cpp"
    "can_address_claim_state_machine.cpp"
    "can_internal_control_function.cpp"
    "can_partnered_control_function.cpp"
    "can_NAME_filter.cpp"
    "can_transport_protocol.cpp"
    "can_stack_logger.cpp"
    "can_network_configuration.cpp"
    "can_callbacks.cpp"
    "can_message_frame.cpp"
    "isobus_virtual_terminal_client.cpp"
    "can_extended_transport_protocol.cpp"
    "isobus_diagnostic_protocol.cpp"
    "can_parameter_group_number_request_protocol.cpp"
    "nmea2000_fast_packet_protocol.cpp"
    "isobus_language_command_interface.cpp"
    "isobus_task_controller_client_objects.cpp"
    "isobus_task_controller_client.cpp"
    "isobus_device_descriptor_object_pool.cpp"
    "isobus_shortcut_button_interface.cpp"
    "isobus_functionalities.cpp"
    "isobus_guidance_interface.cpp")

# Prepend the source directory path to all the source files
prepend(ISOBUS_SRC ${ISOBUS_SRC_DIR} ${ISOBUS_SRC})

# Set the include files
set(ISOBUS_INCLUDE
    "can_NAME.hpp"
    "can_protocol.hpp"
    "can_badge.hpp"
    "can_identifier.hpp"
    "can_control_function.hpp"
    "can_message.hpp"
    "can_general_parameter_group_numbers.hpp"
    "can_network_manager.hpp"
    "can_address_claim_state_machine.hpp"
    "can_NAME_filter.hpp"
    "can_transport_protocol.hpp"
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
    "isobus_virtual_terminal_objects.hpp"
    "isobus_language_command_interface.hpp"
    "isobus_standard_data_description_indices.hpp"
    "isobus_task_controller_client_objects.hpp"
    "isobus_task_controller_client.hpp"
    "isobus_device_descriptor_object_pool.hpp"
    "isobus_shortcut_button_interface.hpp"
    "isobus_functionalities.hpp")
# Prepend the include directory path to all the include files
prepend(ISOBUS_INCLUDE ${ISOBUS_INCLUDE_DIR} ${ISOBUS_INCLUDE})

# Create the library from the source and include files
add_library(Isobus ${ISOBUS_SRC} ${ISOBUS_INCLUDE})
add_library(${PROJECT_NAME}::Isobus ALIAS Isobus)

# Specify the include directory to be exported for other moduels to use. The
# PUBLIC keyword here allows other libraries or exectuables to link to this
# library and use its functionality.
target_include_directories(
  Isobus PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(Isobus PRIVATE ${PROJECT_NAME}::Utility)

install(
  TARGETS Isobus
  EXPORT IsobusTargets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin)
