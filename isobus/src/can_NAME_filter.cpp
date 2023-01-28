//================================================================================================
/// @file can_NAME_filter.cpp
///
/// @brief Defines a filter value for an ISONAME component. Used to tell the stack what kind of
/// ECU you want to talk to when creating a partnered control function.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/can_NAME_filter.hpp"

namespace isobus
{
	NAMEFilter::NAMEFilter(NAME::NAMEParameters nameParameter, std::uint32_t parameterMatchValue) :
	  parameter(nameParameter),
	  value(parameterMatchValue)
	{
	}

	NAME::NAMEParameters NAMEFilter::get_parameter() const
	{
		return parameter;
	}

	std::uint32_t NAMEFilter::get_value() const
	{
		return value;
	}

	bool NAMEFilter::check_name_matches_filter(const NAME &nameToCompare) const
	{
		bool retVal = false;

		switch (parameter)
		{
			case NAME::NAMEParameters::IdentityNumber:
			{
				retVal = (nameToCompare.get_identity_number() == value);
			}
			break;

			case NAME::NAMEParameters::ManufacturerCode:
			{
				retVal = (nameToCompare.get_manufacturer_code() == value);
			}
			break;

			case NAME::NAMEParameters::EcuInstance:
			{
				retVal = (nameToCompare.get_ecu_instance() == value);
			}
			break;

			case NAME::NAMEParameters::FunctionInstance:
			{
				retVal = (nameToCompare.get_function_instance() == value);
			}
			break;

			case NAME::NAMEParameters::FunctionCode:
			{
				retVal = (nameToCompare.get_function_code() == value);
			}
			break;

			case NAME::NAMEParameters::DeviceClass:
			{
				retVal = (nameToCompare.get_device_class() == value);
			}
			break;

			case NAME::NAMEParameters::DeviceClassInstance:
			{
				retVal = (nameToCompare.get_device_class_instance() == value);
			}
			break;

			case NAME::NAMEParameters::IndustryGroup:
			{
				retVal = (nameToCompare.get_industry_group() == value);
			}
			break;

			case NAME::NAMEParameters::ArbitraryAddressCapable:
			{
				retVal = (nameToCompare.get_arbitrary_address_capable() == (0 != value));
			}
			break;

			default:
			{
			}
			break;
		}
		return retVal;
	}

} // namespace isobus