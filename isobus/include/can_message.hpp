#pragma once

#include "can_identifier.hpp"
#include "can_control_function.hpp"

#include <vector>

namespace isobus
{
    
class CANMessage
{
public:

    enum class Type
    {
        Transmit,
        Receive,
        Internal
    };

    CANMessage(std::uint8_t CANPort);

    Type get_type() const;

    std::vector<std::uint8_t> &get_data();
    
    std::uint32_t get_data_length() const;

    ControlFunction *get_source_control_function() const;

    ControlFunction *get_destination_control_function() const;

    CANIdentifier get_identifier() const;

    std::uint32_t get_message_unique_id() const;

    std::uint8_t get_can_port_index() const;

    // ISO11783-3:
    // The maximum number of packets that can be sent in a single connection
    // with extended transport protocol is restricted by the extended data packet offset (3 bytes).
    // This yields a maximum message size of (2^24-1 packets) x (7 bytes/packet) = 117440505 bytes
    static const std::uint32_t ABSOLUTE_MAX_MESSAGE_LENGTH = 117440505;

protected:
    std::vector<std::uint8_t> data;
    ControlFunction *source;
    ControlFunction *destination;
    CANIdentifier identifier;
    Type messageType;
    const std::uint32_t messageUniqueID;
    const std::uint8_t CANPortIndex;
private:
    static std::uint32_t lastGeneratedUniqueID;
};

} // namespace isobus
