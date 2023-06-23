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
#include "isobus/isobus/can_network_manager.hpp"

#include <algorithm>

namespace isobus
{
	InternalControlFunction::InternalControlFunction(NAME desiredName, std::uint8_t preferredAddress, std::uint8_t CANPort) :
	  ControlFunction(desiredName, NULL_CAN_ADDRESS, CANPort, Type::Internal),
	  stateMachine(preferredAddress, desiredName, CANPort)
	{
	}

	std::shared_ptr<InternalControlFunction> InternalControlFunction::create(NAME desiredName, std::uint8_t preferredAddress, std::uint8_t CANPort)
	{
		// Unfortunately, we can't use `std::make_shared` here because the constructor is private
		auto controlFunction = std::shared_ptr<InternalControlFunction>(new InternalControlFunction(desiredName, preferredAddress, CANPort));
		CANNetworkManager::CANNetwork.on_control_function_created(controlFunction, {});
		return controlFunction;
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

} // namespace isobus
