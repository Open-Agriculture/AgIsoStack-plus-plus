#pragma once

#include "can_message.hpp"

namespace isobus
{
    
class CANLibManagedMessage : public CANMessage
{
public:
    CANLibManagedMessage(std::uint8_t CANPort);

    void set_data(std::vector<std::uint8_t> &dataBuffer);

    void set_source_control_function(ControlFunction *value);

    void set_destination_control_function(ControlFunction *value);

    void set_identifier(CANIdentifier value);
};

} // namespace isobus
