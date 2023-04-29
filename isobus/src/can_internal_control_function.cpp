//================================================================================================
/// @file can_internal_control_function.cpp
///
/// @brief A representation of an ISOBUS ECU that we can send from. Use this class
/// when defining your own control functions that will claim an address within your program.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/can_internal_control_function.hpp"

#include "isobus/isobus/can_constants.hpp"

#include <algorithm>

namespace isobus
{
	std::vector<InternalControlFunction *> InternalControlFunction::internalControlFunctionList;
	bool InternalControlFunction::anyChangedAddress = false;

	InternalControlFunction::InternalControlFunction(NAME desiredName, std::uint8_t preferredAddress, std::uint8_t CANPort) :
	  ControlFunction(desiredName, NULL_CAN_ADDRESS, CANPort),
	  stateMachine(preferredAddress, desiredName, CANPort)
	{
		const std::lock_guard<std::mutex> lock(ControlFunction::controlFunctionProcessingMutex);
		controlFunctionType = Type::Internal;

		auto location = std::find(internalControlFunctionList.begin(), internalControlFunctionList.end(), nullptr);
		if (internalControlFunctionList.end() != location)
		{
			*location = this; // Use the available space in the list
		}
		else
		{
			internalControlFunctionList.push_back(this); // Allocate space in the list for this ICF
		}
	}

	InternalControlFunction::~InternalControlFunction()
	{
		const std::lock_guard<std::mutex> lock(ControlFunction::controlFunctionProcessingMutex);
		if (!internalControlFunctionList.empty())
		{
			auto thisObject = std::find(internalControlFunctionList.begin(), internalControlFunctionList.end(), this);

			if (internalControlFunctionList.end() != thisObject)
			{
				*thisObject = nullptr; // Don't erase, just null it out. Erase could cause a double free.
			}
		}
	}

	InternalControlFunction *InternalControlFunction::get_internal_control_function(std::uint32_t index)
	{
		InternalControlFunction *retVal = nullptr;

		if (index < get_number_internal_control_functions())
		{
			auto listPosition = internalControlFunctionList.begin();

			std::advance(listPosition, index);
			retVal = *listPosition;
		}
		return retVal;
	}

	std::uint32_t InternalControlFunction::get_number_internal_control_functions()
	{
		return internalControlFunctionList.size();
	}

	bool InternalControlFunction::get_any_internal_control_function_changed_address(CANLibBadge<CANNetworkManager>)
	{
		return anyChangedAddress;
	}

	bool InternalControlFunction::get_changed_address_since_last_update(CANLibBadge<CANNetworkManager>) const
	{
		return objectChangedAddressSinceLastUpdate;
	}

	void InternalControlFunction::process_commanded_address(std::uint8_t commandedAddress, CANLibBadge<CANNetworkManager>)
	{
		stateMachine.process_commanded_address(commandedAddress);
	}

	void InternalControlFunction::update_address_claiming(CANLibBadge<CANNetworkManager>)
	{
		anyChangedAddress = false;

		for (auto currentControlFunction : internalControlFunctionList)
		{
			if (nullptr != currentControlFunction)
			{
				currentControlFunction->update();
			}
		}
	}

	void InternalControlFunction::update()
	{
		std::uint8_t previousAddress = address;
		objectChangedAddressSinceLastUpdate = false;
		stateMachine.update();
		address = stateMachine.get_claimed_address();

		if (previousAddress != address)
		{
			anyChangedAddress = true;
			objectChangedAddressSinceLastUpdate = true;
		}
	}

} // namespace isobus
