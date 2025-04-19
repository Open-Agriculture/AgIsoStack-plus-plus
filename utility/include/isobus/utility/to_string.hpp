//================================================================================================
/// @file to_string.hpp
///
/// @brief A compatibility template to replace `std::to_string`
/// @details Some compilers don't support `std::to_string` so this file is meant to abstract it
/// away with a workaround if it's not supported. This solution was inspired by Catch2's
/// implementation.
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#ifndef TO_STRING_HPP
#define TO_STRING_HPP

#include <sstream>
#include <string>
#include "../../isobus/include/isobus/isobus/can_constants.hpp"

namespace isobus
{
	template<typename T>
	/// @brief A replacement for std::to_string
	/// @tparam T The data type
	/// @param t The thing to convert to string
	/// @returns the string form of `t`
	std::string to_string(T const &t)
	{
		std::ostringstream oss;
		oss << t;
		return oss.str();
	}

	template<typename T>
	/// @brief A specialized replacement for std::to_string
	/// @tparam object ID
	/// @returns in the case if the object_id is 65535 (NULL object ID) returns "NULL" otherwise it returns the number as string
	std::string object_id_to_string(T const &object_id)
	{
		if (isobus::NULL_OBJECT_ID == object_id)
		{
			return "NULL";
		}
		std::ostringstream oss;
		oss << object_id;
		return oss.str();
	}
} // namespace isobus_utils

#endif // TO_STRING_HPP
