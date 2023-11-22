//================================================================================================
/// @file can_control_function_reference.cpp
///
/// @brief Holds a weak reference to a control function, but in a way that it is more robust.
/// @author Daan Steenbergen
///
/// @copyright 2023 Open-Agriculture Contributors
//================================================================================================

#include "isobus/isobus/can_control_function_reference.hpp"
#include "isobus/isobus/can_constants.hpp"

namespace isobus
{
	const ControlFunctionReference ControlFunctionReference::ANY_CONTROL_FUNCTION = ControlFunctionReference(static_cast<std::shared_ptr<ControlFunction>>(nullptr));

	ControlFunctionReference::ControlFunctionReference(std::shared_ptr<ControlFunction> controlFunction) :
	  std::weak_ptr<ControlFunction>(controlFunction),
	  is_global(controlFunction == nullptr)
	{
	}

	ControlFunctionReference::ControlFunctionReference(std::shared_ptr<PartneredControlFunction> controlFunction) :
	  ControlFunctionReference(std::static_pointer_cast<ControlFunction>(controlFunction))
	{
	}

	bool ControlFunctionReference::is_stale() const
	{
		return !is_global && std::weak_ptr<ControlFunction>::expired();
	}

	bool ControlFunctionReference::has_valid_address() const
	{
		if (is_global)
		{
			return true;
		}
		else
		{
			auto controlFunction = std::weak_ptr<ControlFunction>::lock();
			return controlFunction != nullptr && controlFunction->get_address_valid();
		}
	}

	bool ControlFunctionReference::get_address(std::uint8_t &address) const
	{
		if (is_global)
		{
			address = BROADCAST_CAN_ADDRESS;
			return true;
		}
		else if (auto controlFunction = std::weak_ptr<ControlFunction>::lock())
		{
			if (controlFunction->get_address_valid())
			{
				address = controlFunction->get_address();
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	bool ControlFunctionReference::is_broadcast() const
	{
		return is_global;
	}

	const PartneredControlFunctionReference PartneredControlFunctionReference::ANY_CONTROL_FUNCTION = PartneredControlFunctionReference(nullptr);

	PartneredControlFunctionReference::PartneredControlFunctionReference(std::shared_ptr<PartneredControlFunction> controlFunction) :
	  std::weak_ptr<PartneredControlFunction>(controlFunction),
	  ControlFunctionReference(controlFunction)
	{
	}
}
