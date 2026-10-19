//================================================================================================
/// @file isobus_machine_selected_speed.cpp
///
/// @brief Implements an interface for sending and receiving ISOBUS machine-selected speed messages.
/// These messages are used to receive or transmit data about how fast the machine is going.
/// Generally these are meant to be used in conjunction with the SpeedMessagesInterface class.
///
/// @note Generally this is the "best" speed to use, as the TECU chooses its favorite speed
/// and reports it in this message. If this is not present, you can fall back to wheel-based or
/// ground-based speed messages for example, but chose with care as those speeds may be
/// less accurate or not filtered as well.
///
/// @author Adrian Del Grosso
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/isobus_machine_selected_speed.hpp"

namespace isobus
{
	MachineSelectedSpeedData::MachineSelectedSpeedData(std::shared_ptr<ControlFunction> sender) :
	  controlFunction(sender)
	{
	}

	std::uint32_t MachineSelectedSpeedData::get_machine_distance() const
	{
		std::uint32_t retVal = machineSelectedSpeedDistance_mm;

		// Values above the max are sort of implicitly defined and should be ignored.
		if (machineSelectedSpeedDistance_mm > SAEds05_MAX_VALUE)
		{
			retVal = 0;
		}
		return retVal;
	}

	bool MachineSelectedSpeedData::set_machine_distance(std::uint32_t distance)
	{
		bool retVal = (machineSelectedSpeedDistance_mm != distance);
		machineSelectedSpeedDistance_mm = distance;
		return retVal;
	}

	std::uint16_t MachineSelectedSpeedData::get_machine_speed() const
	{
		std::uint16_t retVal = machineSelectedSpeed_mm_per_sec;

		if (machineSelectedSpeed_mm_per_sec > SAEvl01_MAX_VALUE)
		{
			retVal = 0;
		}
		return retVal;
	}

	bool MachineSelectedSpeedData::set_machine_speed(std::uint16_t speed)
	{
		bool retVal = (speed != machineSelectedSpeed_mm_per_sec);
		machineSelectedSpeed_mm_per_sec = speed;
		return retVal;
	}

	std::uint8_t MachineSelectedSpeedData::get_exit_reason_code() const
	{
		return exitReasonCode;
	}

	bool MachineSelectedSpeedData::set_exit_reason_code(std::uint8_t exitCode)
	{
		bool retVal = (exitCode != exitReasonCode);
		exitReasonCode = exitCode;
		return retVal;
	}

	MachineSelectedSpeedData::SpeedSource MachineSelectedSpeedData::get_speed_source() const
	{
		return source;
	}

	bool MachineSelectedSpeedData::set_speed_source(SpeedSource selectedSource)
	{
		bool retVal = (source != selectedSource);
		source = selectedSource;
		return retVal;
	}

	MachineSelectedSpeedData::LimitStatus MachineSelectedSpeedData::get_limit_status() const
	{
		return limitStatus;
	}

	bool MachineSelectedSpeedData::set_limit_status(LimitStatus statusToSet)
	{
		bool retVal = (limitStatus != statusToSet);
		limitStatus = statusToSet;
		return retVal;
	}

	MachineDirection MachineSelectedSpeedData::get_machine_direction_of_travel() const
	{
		return machineDirectionState;
	}

	bool MachineSelectedSpeedData::set_machine_direction_of_travel(MachineDirection directionOfTravel)
	{
		bool retVal = (directionOfTravel != machineDirectionState);
		machineDirectionState = directionOfTravel;
		return retVal;
	}

	std::shared_ptr<ControlFunction> MachineSelectedSpeedData::get_sender_control_function() const
	{
		return controlFunction;
	}

	void MachineSelectedSpeedData::set_timestamp_ms(std::uint32_t timestamp)
	{
		timestamp_ms = timestamp;
	}

	std::uint32_t MachineSelectedSpeedData::get_timestamp_ms() const
	{
		return timestamp_ms;
	}
}
