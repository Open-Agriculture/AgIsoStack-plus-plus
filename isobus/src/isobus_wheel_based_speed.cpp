//================================================================================================
/// @file isobus_wheel_based_speed.cpp
///
/// @brief Implements an interface for sending and receiving ISOBUS wheel-based speed messages.
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
#include "isobus/isobus/isobus_wheel_based_speed.hpp"

namespace isobus
{
	WheelBasedMachineSpeedData::WheelBasedMachineSpeedData(std::shared_ptr<ControlFunction> sender) :
	  controlFunction(sender)
	{
	}

	std::uint32_t WheelBasedMachineSpeedData::get_machine_distance() const
	{
		std::uint32_t retVal = wheelBasedMachineDistance_mm;

		// Values above the max are sort of implicitly defined and should be ignored.
		if (wheelBasedMachineDistance_mm > SAEds05_MAX_VALUE)
		{
			retVal = 0;
		}
		return retVal;
	}

	bool WheelBasedMachineSpeedData::set_machine_distance(std::uint32_t distance)
	{
		bool retVal = (distance != wheelBasedMachineDistance_mm);
		wheelBasedMachineDistance_mm = distance;
		return retVal;
	}

	std::uint16_t WheelBasedMachineSpeedData::get_machine_speed() const
	{
		std::uint16_t retVal = wheelBasedMachineSpeed_mm_per_sec;

		if (wheelBasedMachineSpeed_mm_per_sec > SAEvl01_MAX_VALUE)
		{
			retVal = 0;
		}
		return retVal;
	}

	bool WheelBasedMachineSpeedData::set_machine_speed(std::uint16_t speed)
	{
		bool retVal = (speed != wheelBasedMachineSpeed_mm_per_sec);
		wheelBasedMachineSpeed_mm_per_sec = speed;
		return retVal;
	}

	std::uint8_t WheelBasedMachineSpeedData::get_maximum_time_of_tractor_power() const
	{
		return maximumTimeOfTractorPower_min;
	}

	bool WheelBasedMachineSpeedData::set_maximum_time_of_tractor_power(std::uint8_t maxTime)
	{
		bool retVal = (maximumTimeOfTractorPower_min != maxTime);
		maximumTimeOfTractorPower_min = maxTime;
		return retVal;
	}

	MachineDirection WheelBasedMachineSpeedData::get_machine_direction_of_travel() const
	{
		return machineDirectionState;
	}

	bool WheelBasedMachineSpeedData::set_machine_direction_of_travel(MachineDirection direction)
	{
		bool retVal = (machineDirectionState != direction);
		machineDirectionState = direction;
		return retVal;
	}

	WheelBasedMachineSpeedData::KeySwitchState WheelBasedMachineSpeedData::get_key_switch_state() const
	{
		return keySwitchState;
	}

	bool WheelBasedMachineSpeedData::set_key_switch_state(KeySwitchState state)
	{
		bool retVal = (keySwitchState != state);
		keySwitchState = state;
		return retVal;
	}

	WheelBasedMachineSpeedData::ImplementStartStopOperations WheelBasedMachineSpeedData::get_implement_start_stop_operations_state() const
	{
		return implementStartStopOperationsState;
	}

	bool WheelBasedMachineSpeedData::set_implement_start_stop_operations_state(ImplementStartStopOperations state)
	{
		bool retVal = (implementStartStopOperationsState != state);
		implementStartStopOperationsState = state;
		return retVal;
	}

	WheelBasedMachineSpeedData::OperatorDirectionReversed WheelBasedMachineSpeedData::get_operator_direction_reversed_state() const
	{
		return operatorDirectionReversedState;
	}

	bool WheelBasedMachineSpeedData::set_operator_direction_reversed_state(OperatorDirectionReversed reverseState)
	{
		bool retVal = (operatorDirectionReversedState != reverseState);
		operatorDirectionReversedState = reverseState;
		return retVal;
	}

	std::shared_ptr<ControlFunction> WheelBasedMachineSpeedData::get_sender_control_function() const
	{
		return controlFunction;
	}

	void WheelBasedMachineSpeedData::set_timestamp_ms(std::uint32_t timestamp)
	{
		timestamp_ms = timestamp;
	}

	std::uint32_t WheelBasedMachineSpeedData::get_timestamp_ms() const
	{
		return timestamp_ms;
	}

}
