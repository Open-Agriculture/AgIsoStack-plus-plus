//================================================================================================
/// @file isobus_machine_selected_speed_command.cpp
///
/// @brief Implements an interface for sending and receiving ISOBUS machine-selected speed command messages.
/// These messages are used to receive or transmit machine odometry commands.
/// Generally these are meant to be used in conjunction with the SpeedMessagesInterface class.
///
/// @author Adrian Del Grosso
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/isobus_machine_selected_speed_command.hpp"

namespace isobus
{
	MachineSelectedSpeedCommandData::MachineSelectedSpeedCommandData(std::shared_ptr<ControlFunction> sender) :
	  controlFunction(sender)
	{
	}

	std::uint16_t MachineSelectedSpeedCommandData::get_machine_speed_setpoint_command() const
	{
		std::uint16_t retVal = speedCommandedSetpoint;

		// Values over the max are implicitly defined, best to treat as zeros.
		if (speedCommandedSetpoint > SAEvl01_MAX_VALUE)
		{
			retVal = 0;
		}
		return retVal;
	}

	bool MachineSelectedSpeedCommandData::set_machine_speed_setpoint_command(std::uint16_t speed)
	{
		bool retVal = (speed != speedCommandedSetpoint);
		speedCommandedSetpoint = speed;
		return retVal;
	}

	std::uint16_t MachineSelectedSpeedCommandData::get_machine_selected_speed_setpoint_limit() const
	{
		std::uint16_t retVal = speedSetpointLimit;

		// Values over the max are implicitly defined, best to treat as zeros.
		if (speedSetpointLimit > SAEvl01_MAX_VALUE)
		{
			retVal = 0;
		}
		return retVal;
	}

	bool MachineSelectedSpeedCommandData::set_machine_selected_speed_setpoint_limit(std::uint16_t speedLimit)
	{
		bool retVal = (speedSetpointLimit != speedLimit);
		speedSetpointLimit = speedLimit;
		return retVal;
	}

	MachineDirection MachineSelectedSpeedCommandData::get_machine_direction_command() const
	{
		return machineDirectionCommand;
	}

	bool MachineSelectedSpeedCommandData::set_machine_direction_of_travel(MachineDirection commandedDirection)
	{
		bool retVal = (commandedDirection != machineDirectionCommand);
		machineDirectionCommand = commandedDirection;
		return retVal;
	}

	std::shared_ptr<ControlFunction> MachineSelectedSpeedCommandData::get_sender_control_function() const
	{
		return controlFunction;
	}

	void MachineSelectedSpeedCommandData::set_timestamp_ms(std::uint32_t timestamp)
	{
		timestamp_ms = timestamp;
	}

	std::uint32_t MachineSelectedSpeedCommandData::get_timestamp_ms() const
	{
		return timestamp_ms;
	}
}
