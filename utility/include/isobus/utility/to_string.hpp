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
} // namespace isobus_utils

#endif // TO_STRING_HPP
