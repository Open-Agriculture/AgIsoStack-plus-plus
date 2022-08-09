//================================================================================================
/// @file can_NAME_filter.hpp
///
/// @brief Defines a filter value for an ISONAME component. Used to tell the stack what kind of
/// ECU you want to talk to when creating a partnered control function.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_NAME_FILTER_HPP
#define CAN_NAME_FILTER_HPP

#include "can_NAME.hpp"

namespace isobus
{

	class NAMEFilter
	{
	public:
		NAMEFilter(NAME::NAMEParameters nameParameter, std::uint32_t parameterMatchValue);

		NAME::NAMEParameters get_parameter() const;

		std::uint32_t get_value() const;

		bool check_name_matches_filter(const NAME &nameToCompare);

	private:
		NAME::NAMEParameters parameter;
		std::uint32_t value;
	};

} // namespace isobus

#endif // CAN_NAME_FILTER_HPP
