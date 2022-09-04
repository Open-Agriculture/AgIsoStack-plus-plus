//================================================================================================
/// @file can_partnered_control_function.cpp
///
/// @brief A class that describes a control function on the bus that the stack should communicate
/// with. Use these to describe ECUs you want to send messages to.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "can_partnered_control_function.hpp"

#include "can_constants.hpp"

#include <algorithm>

namespace isobus
{
	std::vector<PartneredControlFunction*> PartneredControlFunction::partneredControlFunctionList;

	PartneredControlFunction::PartneredControlFunction(std::uint8_t CANPort, const std::vector<NAMEFilter> NAMEFilters) :
	  ControlFunction(NAME(0), NULL_CAN_ADDRESS, CANPort),
	  NAMEFilterList(NAMEFilters)
	{
		partneredControlFunctionList.push_back(this);
	}

	PartneredControlFunction::~PartneredControlFunction()
	{
		auto thisObject = std::find(partneredControlFunctionList.begin(), partneredControlFunctionList.end(), this);
		partneredControlFunctionList.erase(thisObject);
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

	std::uint32_t PartneredControlFunction::get_number_name_filters() const
	{
		return NAMEFilterList.size();
	}

	bool PartneredControlFunction::get_name_filter_parameter(std::uint32_t index, NAME::NAMEParameters &parameter, std::uint32_t &filterValue) const
	{
		bool retVal = false;

		if (index < get_number_name_filters())
		{
			parameter = NAMEFilterList[index].get_parameter();
			filterValue = NAMEFilterList[index].get_value();
			retVal = true;
		}
		return retVal;
	}

	std::uint32_t PartneredControlFunction::get_number_name_filters_with_parameter_type(NAME::NAMEParameters parameter)
	{
		std::uint32_t retVal = 0;

		for (uint32_t i = 0; i < NAMEFilterList.size(); i++)
		{
			if (parameter == NAMEFilterList[i].get_parameter())
			{
				retVal++;
			}
		}
		return retVal;
	}

	bool PartneredControlFunction::check_matches_name(NAME NAMEToCheck) const
	{
		std::uint32_t currentFilterValue;
		NAME::NAMEParameters currentFilterParameter;
		bool retVal = true;

		if (0 == get_number_name_filters())
		{
			retVal = false;
		}

		for (std::uint32_t i = 0; i < get_number_name_filters(); i++)
		{
			get_name_filter_parameter(i, currentFilterParameter, currentFilterValue);

			switch (currentFilterParameter)
			{
				case NAME::NAMEParameters::IdentityNumber:
				{
					retVal = (currentFilterValue == NAMEToCheck.get_identity_number());
				}
				break;

				case NAME::NAMEParameters::ManufacturerCode:
				{
					retVal = (currentFilterValue == NAMEToCheck.get_manufacturer_code());
				}
				break;

				case NAME::NAMEParameters::EcuInstance:
				{
					retVal = (currentFilterValue == NAMEToCheck.get_ecu_instance());
				}
				break;

				case NAME::NAMEParameters::FunctionInstance:
				{
					retVal = (currentFilterValue == NAMEToCheck.get_function_instance());
				}
				break;

				case NAME::NAMEParameters::FunctionCode:
				{
					retVal = (currentFilterValue == NAMEToCheck.get_function_code());
				}
				break;

				case NAME::NAMEParameters::DeviceClass:
				{
					retVal = (currentFilterValue == NAMEToCheck.get_device_class());
				}
				break;

				case NAME::NAMEParameters::DeviceClassInstance:
				{
					retVal = (currentFilterValue == NAMEToCheck.get_device_class_instance());
				}
				break;

				case NAME::NAMEParameters::IndustryGroup:
				{
					retVal = (currentFilterValue == NAMEToCheck.get_industry_group());
				}
				break;

				case NAME::NAMEParameters::ArbitraryAddressCapable:
				{
					retVal = (currentFilterValue == NAMEToCheck.get_arbitrary_address_capable());
				}
				break;

				default:
				{
					retVal = false;
				}
				break;
			}

			if (false == retVal)
			{
				break;
			}
		}
		return retVal;
	}

	PartneredControlFunction *PartneredControlFunction::get_partnered_control_function(std::uint32_t index)
	{
		PartneredControlFunction *retVal = nullptr;

		if (index < get_number_partnered_control_functions())
		{
			auto listPosition = partneredControlFunctionList.begin();

			std::advance(listPosition, index);
			retVal = *listPosition;
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
