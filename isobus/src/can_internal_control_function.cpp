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

#include <algorithm>

namespace isobus
{
	std::vector<std::shared_ptr<InternalControlFunction>> InternalControlFunction::internalControlFunctionList;
	bool InternalControlFunction::anyChangedAddress = false;

	InternalControlFunction::InternalControlFunction(NAME desiredName, std::uint8_t preferredAddress, std::uint8_t CANPort) :
	  ControlFunction(desiredName, NULL_CAN_ADDRESS, CANPort, Type::Internal),
	  stateMachine(preferredAddress, desiredName, CANPort)
	{
	}

	std::shared_ptr<InternalControlFunction> InternalControlFunction::create(NAME desiredName, std::uint8_t preferredAddress, std::uint8_t CANPort)
	{
		// Unfortunately, we can't use `std::make_shared` here because the constructor is private
		auto createdControlFunction = std::shared_ptr<InternalControlFunction>(new InternalControlFunction(desiredName, preferredAddress, CANPort));
		internalControlFunctionList.push_back(createdControlFunction);
		return createdControlFunction;
	}

	bool InternalControlFunction::destroy(std::uint32_t expectedRefCount)
	{
		std::unique_lock<std::mutex> lock(controlFunctionProcessingMutex);
		internalControlFunctionList.erase(std::find(internalControlFunctionList.begin(), internalControlFunctionList.end(), shared_from_this()));
		lock.unlock();

		return ControlFunction::destroy(expectedRefCount);
	}

	std::shared_ptr<InternalControlFunction> InternalControlFunction::get_internal_control_function(std::uint32_t index)
	{
		std::shared_ptr<InternalControlFunction> retVal = nullptr;

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
