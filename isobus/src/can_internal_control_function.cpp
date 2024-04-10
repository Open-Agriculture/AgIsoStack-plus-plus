//================================================================================================
/// @file can_internal_control_function.cpp
///
/// @brief A representation of an ISOBUS ECU that we can send from. Use this class
/// when defining your own control functions that will claim an address within your program.
/// @author Adrian Del Grosso
/// @author Daan Steenbergen
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/can_internal_control_function.hpp"

#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"

#include <algorithm>

namespace isobus
{
	InternalControlFunction::InternalControlFunction(NAME desiredName, std::uint8_t preferredAddress, std::uint8_t CANPort) :
	  ControlFunction(desiredName, NULL_CAN_ADDRESS, CANPort, Type::Internal),
	  stateMachine(preferredAddress, desiredName, CANPort)
	{
	}

	void InternalControlFunction::on_address_violation(CANLibBadge<CANNetworkManager>)
	{
		stateMachine.on_address_violation();
	}

	void InternalControlFunction::process_commanded_address(std::uint8_t commandedAddress, CANLibBadge<CANNetworkManager>)
	{
		stateMachine.process_commanded_address(commandedAddress);
	}

	bool InternalControlFunction::update_address_claiming(CANLibBadge<CANNetworkManager>)
	{
		std::uint8_t previousAddress = address;
		stateMachine.update();
		address = stateMachine.get_claimed_address();

		return previousAddress != address;
	}

	std::weak_ptr<ParameterGroupNumberRequestProtocol> InternalControlFunction::get_pgn_request_protocol() const
	{
		return pgnRequestProtocol;
	}

} // namespace isobus
