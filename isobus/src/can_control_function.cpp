#include "can_control_function.hpp"

#include "can_types.hpp"

namespace isobus
{
	ControlFunction::ControlFunction(NAME NAMEValue, std::uint8_t addressValue, std::uint8_t CANPort) :
	  controlFunctionNAME(NAMEValue),
	  controlFunctionType(Type::External),
	  address(addressValue),
	  canPortIndex(CANPort)

	{
	}

	ControlFunction::~ControlFunction()
	{
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

} // namespace isobus
