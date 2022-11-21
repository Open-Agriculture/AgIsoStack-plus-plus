//================================================================================================
/// @file can_message.cpp
///
/// @brief An abstraction  of a CAN message, could be > 8 data bytes.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/can_message.hpp"

namespace isobus
{
	std::uint32_t CANMessage::lastGeneratedUniqueID = 0;

	CANMessage::CANMessage(std::uint8_t CANPort) :
	  source(nullptr),
	  destination(nullptr),
	  identifier(0),
	  messageType(Type::Receive),
	  messageUniqueID(lastGeneratedUniqueID++),
	  CANPortIndex(CANPort)
	{
	}

	CANMessage::Type CANMessage::get_type() const
	{
		return messageType;
	}

	std::vector<std::uint8_t> &CANMessage::get_data()
	{
		return data;
	}

	std::uint32_t CANMessage::get_data_length() const
	{
		return data.size();
	}

	ControlFunction *CANMessage::get_source_control_function() const
	{
		return source;
	}

	ControlFunction *CANMessage::get_destination_control_function() const
	{
		return destination;
	}

	CANIdentifier CANMessage::get_identifier() const
	{
		return identifier;
	}

	std::uint32_t CANMessage::get_message_unique_id() const
	{
		return messageUniqueID;
	}

	std::uint8_t CANMessage::get_can_port_index() const
	{
		return CANPortIndex;
	}

} // namespace isobus
