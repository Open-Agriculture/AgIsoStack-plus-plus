#include "can_partnered_control_function.hpp"

#include "can_types.hpp"

#include <algorithm>

namespace isobus
{
	std::list<PartneredControlFunction> PartneredControlFunction::partneredControlFunctionList;

	PartneredControlFunction::PartneredControlFunction(std::uint8_t CANPort, const std::vector<NAMEFilter> NAMEFilters) :
	  ControlFunction(NAME(DEFAULT_NAME), NULL_CAN_ADDRESS, CANPort),
	  NAMEFilterList(NAMEFilters)
	{
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

} // namespace isobus
