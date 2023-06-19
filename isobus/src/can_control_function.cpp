//================================================================================================
/// @file can_control_function.cpp
///
/// @brief Defines a base class to represent a generic ISOBUS control function.
/// @author Adrian Del Grosso
/// @author Daan Steenbergen
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#include "isobus/isobus/can_control_function.hpp"

#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_network_manager.hpp"

namespace isobus
{
	std::mutex ControlFunction::controlFunctionProcessingMutex;

	isobus::ControlFunction::ControlFunction(NAME NAMEValue, std::uint8_t addressValue, std::uint8_t CANPort, Type type) :
	  controlFunctionType(type),
	  controlFunctionNAME(NAMEValue),
	  address(addressValue),
	  canPortIndex(CANPort)
	{
	}

	std::shared_ptr<ControlFunction> ControlFunction::create(NAME NAMEValue, std::uint8_t addressValue, std::uint8_t CANPort)
	{
		// Unfortunately, we can't use `std::make_shared` here because the constructor is private
		return std::shared_ptr<ControlFunction>(new ControlFunction(NAMEValue, addressValue, CANPort));
	}

	bool ControlFunction::destroy(std::uint32_t expectedRefCount)
	{
		std::lock_guard<std::mutex> lock(controlFunctionProcessingMutex);

		CANNetworkManager::CANNetwork.on_control_function_destroyed(shared_from_this(), {});

		return shared_from_this().use_count() == expectedRefCount + 1;
	}

	std::uint8_t ControlFunction::get_address() const
	{
		return address;
	}

	bool ControlFunction::get_address_valid() const
	{
		return ((BROADCAST_CAN_ADDRESS != address) && (NULL_CAN_ADDRESS != address));
	}

	std::uint8_t ControlFunction::get_can_port() const
	{
		return canPortIndex;
	}

	NAME ControlFunction::get_NAME() const
	{
		return controlFunctionNAME;
	}

	ControlFunction::Type ControlFunction::get_type() const
	{
		return controlFunctionType;
	}

	std::string ControlFunction::get_type_string() const
	{
		switch (controlFunctionType)
		{
			case Type::Internal:
				return "Internal";
			case Type::External:
				return "External";
			case Type::Partnered:
				return "Partnered";
			default:
				return "Unknown";
		}
	}

} // namespace isobus
