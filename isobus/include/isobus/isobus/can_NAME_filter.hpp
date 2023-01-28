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

#include "isobus/isobus/can_NAME.hpp"

namespace isobus
{
	//================================================================================================
	/// @class NAMEFilter
	///
	/// @brief A class that associates a NAME parameter with a value of that parameter.
	/// @details This class is used to match a partner control function with specific criteria that
	/// defines it. Use these to define what device you want to talk to.
	//================================================================================================
	class NAMEFilter
	{
	public:
		/// @brief Constructor for the NAMEFilter
		/// @param[in] nameParameter The component of the NAME to filter on
		/// @param[in] parameterMatchValue The value to match with the nameParameter
		NAMEFilter(NAME::NAMEParameters nameParameter, std::uint32_t parameterMatchValue);

		/// @brief Returns the parameter data associated with this filter
		/// @returns The parameter/NAME component associated with this filter
		NAME::NAMEParameters get_parameter() const;

		/// @brief Returns the value associated with this filter
		/// @returns The data associated with this filter component
		std::uint32_t get_value() const;

		/// @brief Returns true if a NAME matches this filter class's components
		/// @returns true if a NAME matches this filter class's components
		bool check_name_matches_filter(const NAME &nameToCompare) const;

	private:
		NAME::NAMEParameters parameter; ///< The NAME component to filter against
		std::uint32_t value; ///< The value of the data associated with the filter component
	};

} // namespace isobus

#endif // CAN_NAME_FILTER_HPP
