#include "can_lib_managed_message.hpp"

namespace isobus
{
	CANLibManagedMessage::CANLibManagedMessage(std::uint8_t CANPort) :
	  CANMessage(CANPort)
	{
	}

	void CANLibManagedMessage::set_data(std::uint8_t *dataBuffer)
	{
		if (nullptr != dataBuffer)
		{
			data.resize(8);
			data[0] = dataBuffer[0];
			data[1] = dataBuffer[1];
			data[2] = dataBuffer[2];
			data[3] = dataBuffer[3];
			data[4] = dataBuffer[4];
			data[5] = dataBuffer[5];
			data[6] = dataBuffer[6];
			data[7] = dataBuffer[7];
		}
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
