//================================================================================================
/// @file can_managed_message.cpp
///
/// @brief A CAN message that allows setter access to private  data, to be used by the library
/// itself internally under some circumstances.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#include "can_managed_message.hpp"

namespace isobus
{
	CANLibManagedMessage::CANLibManagedMessage(std::uint8_t CANPort) :
	  CANMessage(CANPort)
	{
	}

	void CANLibManagedMessage::set_data(const std::uint8_t *dataBuffer, std::uint32_t length)
	{
		if (nullptr != dataBuffer)
		{
			data.insert(data.end(), dataBuffer, dataBuffer + length);
		}
	}

	void CANLibManagedMessage::set_data(std::uint8_t dataByte, const std::uint32_t insertPosition)
	{
		if (insertPosition < data.size())
		{
			data[insertPosition] = dataByte;
		}
	}

	void CANLibManagedMessage::set_data_size(std::uint32_t length)
	{
		data.resize(length);
	}

	void CANLibManagedMessage::set_source_control_function(ControlFunction *value)
	{
		source = value;
	}

	void CANLibManagedMessage::set_destination_control_function(ControlFunction *value)
	{
		destination = value;
	}

	void CANLibManagedMessage::set_identifier(CANIdentifier value)
	{
		identifier = value;
	}

} // namespace isobus
