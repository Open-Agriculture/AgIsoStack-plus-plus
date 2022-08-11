//================================================================================================
/// @file can_managed_message.hpp
///
/// @brief A CAN message that allows setter access to private  data, to be used by the library
/// itself internally under some circumstances.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_MANAGED_MESSAGE_HPP
#define CAN_MANAGED_MESSAGE_HPP

#include "can_message.hpp"

namespace isobus
{
    
class CANLibManagedMessage : public CANMessage
{
public:
    CANLibManagedMessage(std::uint8_t CANPort);

    void set_data(std::uint8_t *dataBuffer, std::uint32_t length);
    void set_data(std::uint8_t dataByte, const std::uint32_t insertPosition);

    void set_data_size(std::uint32_t length);

    void set_source_control_function(ControlFunction *value);

    void set_destination_control_function(ControlFunction *value);

    void set_identifier(CANIdentifier value);
};

} // namespace isobus

#endif // CAN_MANAGED_MESSAGE_HPP
