//================================================================================================
/// @file isobus_standard_data_description_indices.cpp
///
/// @brief Defines some standard DDIs. This list does not include proprietary DDIs.
/// @note The full list of standardized DDIs can be found at "isobus.net"
/// @author Daan Steenbergen
///
/// @copyright 2025 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/isobus_standard_data_description_indices.hpp"

#include <cmath>

#include "isobus/isobus/isobus_data_dictionary.hpp"

namespace isobus
{
	std::string DataDescriptionIndexHelper::to_string(const DataDescriptionIndex ddi)
	{
		auto index = static_cast<std::uint16_t>(ddi);
#ifdef DISABLE_ISOBUS_DATA_DICTIONARY
		return std::to_string(index) + " (Data Dictionary Disabled)";
#else
		return DataDictionary::get_entry(index).name;
#endif
	}

	std::string DataDescriptionIndexHelper::format_value(const DataDescriptionIndex ddi, const std::int32_t value)
	{
		auto index = static_cast<std::uint16_t>(ddi);
#ifdef DISABLE_ISOBUS_DATA_DICTIONARY
		return std::to_string(value) + " (Data Dictionary Disabled)";
#else
		const auto &entry = DataDictionary::get_entry(index);

		std::string suffix;
		if (entry.units.compare("None") != 0)
		{
			suffix = " " + entry.units;
		}
		switch (ddi)
		{
			case DataDescriptionIndex::ActualWorkState:
			case DataDescriptionIndex::SetpointWorkState:
			{
				if (value == 0)
				{
					return "Off";
				}
				else if (value == 1)
				{
					return "On";
				}
				else if (value == 2)
				{
					return "Error";
				}
				else if (value == 3)
				{
					return "Not installed";
				}
				else
				{
					return "Unknown";
				}
			}
		}

		constexpr float epsilon = 1e-6f;
		if (std::fabs(entry.resolution - 1.0f) > epsilon)
		{
			// If the resolution is not 1.0, scale the value
			double valuedouble = static_cast<double>(value) * entry.resolution;
			return std::to_string(valuedouble) + " " + suffix;
		}
		else
		{
			return std::to_string(value) + " " + suffix;
		}
#endif
	}
} // namespace isobus