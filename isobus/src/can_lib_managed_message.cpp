#include "can_lib_managed_message.hpp"

namespace isobus
{
	CANLibManagedMessage::CANLibManagedMessage(std::uint8_t CANPort) :
	  CANMessage(CANPort)
	{
	}

	void CANLibManagedMessage::set_data(std::vector<std::uint8_t> &dataBuffer)
	{
		data = dataBuffer;
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
