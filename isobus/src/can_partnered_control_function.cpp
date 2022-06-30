#include "can_partnered_control_function.hpp"

#include "can_types.hpp"

#include <algorithm>

namespace isobus
{
	std::vector<PartneredControlFunction> PartneredControlFunction::partneredControlFunctionList;

	PartneredControlFunction::PartneredControlFunction(std::uint8_t CANPort, const std::vector<NAMEFilter> NAMEFilters) :
	  ControlFunction(NAME(0), NULL_CAN_ADDRESS, CANPort),
	  NAMEFilterList(NAMEFilters)
	{
	}

	void PartneredControlFunction::add_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback)
	{
		parameterGroupNumberCallbacks.push_back(ParameterGroupNumberCallbackData(parameterGroupNumber, callback));
	}

	void PartneredControlFunction::remove_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback)
	{
		ParameterGroupNumberCallbackData tempObject(parameterGroupNumber, callback);
		auto callbackLocation = std::find(parameterGroupNumberCallbacks.begin(), parameterGroupNumberCallbacks.end(), tempObject);
		if (parameterGroupNumberCallbacks.end() != callbackLocation)
		{
			parameterGroupNumberCallbacks.erase(callbackLocation);
		}
	}

    std::uint32_t PartneredControlFunction::get_number_parameter_group_number_callbacks() const
	{
		return parameterGroupNumberCallbacks.size();
	}

	PartneredControlFunction *PartneredControlFunction::get_partnered_control_function(std::uint32_t index)
	{
		PartneredControlFunction *retVal = nullptr;

		if (index < get_number_partnered_control_functions())
		{
			auto listPosition = partneredControlFunctionList.begin();

			std::advance(listPosition, index);
			retVal = &*listPosition;
		}
		return retVal;
	}

	std::uint32_t PartneredControlFunction::get_number_partnered_control_functions()
	{
		return partneredControlFunctionList.size();
	}

	ParameterGroupNumberCallbackData PartneredControlFunction::get_parameter_group_number_callback(std::uint32_t index) const
	{
		ParameterGroupNumberCallbackData retVal(0, nullptr);

		if (index < get_number_parameter_group_number_callbacks())
		{
			retVal = parameterGroupNumberCallbacks[index];
		}
		return retVal;
	}

} // namespace isobus
