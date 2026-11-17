//================================================================================================
/// @file isobus_ground_based_speed.cpp
///
/// @brief Implements an interface for sending and receiving ISOBUS ground-based speed messages.
/// These messages are used to receive or transmit data about how fast the machine is going.
/// Generally these are meant to be used in conjunction with the SpeedMessagesInterface class.
///
/// @note Generally you will want to use the machine selected speed rather than the other
/// speeds, as the TECU chooses its favorite speed and reports it in that message.
///
/// @author Adrian Del Grosso
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/isobus_ground_based_speed.hpp"

namespace isobus
{
	GroundBasedSpeedData::GroundBasedSpeedData(std::shared_ptr<ControlFunction> sender) :
	  controlFunction(sender)
	{
	}

	std::uint32_t GroundBasedSpeedData::get_machine_distance() const
	{
		std::uint32_t retVal = groundBasedMachineDistance_mm;

		// Values above the max are sort of implicitly defined and should be ignored.
		if (groundBasedMachineDistance_mm > SAEds05_MAX_VALUE)
		{
			retVal = 0;
		}
		return retVal;
	}

	bool GroundBasedSpeedData::set_machine_distance(std::uint32_t distance)
	{
		bool retVal = (distance != groundBasedMachineDistance_mm);
		groundBasedMachineDistance_mm = distance;
		return retVal;
	}

	std::uint16_t GroundBasedSpeedData::get_machine_speed() const
	{
		std::uint16_t retVal = groundBasedMachineSpeed_mm_per_sec;

		if (groundBasedMachineSpeed_mm_per_sec > SAEvl01_MAX_VALUE)
		{
			retVal = 0;
		}
		return retVal;
	}

	bool GroundBasedSpeedData::set_machine_speed(std::uint16_t speed)
	{
		bool retVal = (speed != groundBasedMachineSpeed_mm_per_sec);
		groundBasedMachineSpeed_mm_per_sec = speed;
		return retVal;
	}

	MachineDirection GroundBasedSpeedData::get_machine_direction_of_travel() const
	{
		return machineDirectionState;
	}

	bool GroundBasedSpeedData::set_machine_direction_of_travel(MachineDirection directionOfTravel)
	{
		bool retVal = (directionOfTravel != machineDirectionState);
		machineDirectionState = directionOfTravel;
		return retVal;
	}

	std::shared_ptr<ControlFunction> GroundBasedSpeedData::get_sender_control_function() const
	{
		return controlFunction;
	}

	void GroundBasedSpeedData::set_timestamp_ms(std::uint32_t timestamp)
	{
		timestamp_ms = timestamp;
	}

	std::uint32_t GroundBasedSpeedData::get_timestamp_ms() const
	{
		return timestamp_ms;
	}
}
