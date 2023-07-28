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
#include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"

#include <algorithm>

namespace isobus
{
	InternalControlFunction::InternalControlFunction(NAME desiredName, std::uint8_t preferredAddress, std::uint8_t CANPort, CANLibBadge<InternalControlFunction>) :
	  ControlFunction(desiredName, NULL_CAN_ADDRESS, CANPort, Type::Internal),
	  stateMachine(preferredAddress, desiredName, CANPort)
	{
	}

	std::shared_ptr<InternalControlFunction> InternalControlFunction::create(NAME desiredName, std::uint8_t preferredAddress, std::uint8_t CANPort)
	{
		// Unfortunately, we can't use `std::make_shared` here because the constructor is private
		CANLibBadge<InternalControlFunction> badge; // This badge is used to allow creation of the PGN request protocol only from within this class
		auto controlFunction = std::shared_ptr<InternalControlFunction>(new InternalControlFunction(desiredName, preferredAddress, CANPort, badge));
		controlFunction->pgnRequestProtocol = std::make_unique<ParameterGroupNumberRequestProtocol>(controlFunction, badge);
		CANNetworkManager::CANNetwork.on_control_function_created(controlFunction, {});
		return controlFunction;
	}

	bool InternalControlFunction::destroy(std::uint32_t expectedRefCount)
	{
		// We need to destroy the PGN request protocol before we destroy the control function
		pgnRequestProtocol.reset();

		return ControlFunction::destroy(expectedRefCount);
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
