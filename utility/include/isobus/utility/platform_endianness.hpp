//================================================================================================
/// @file platform_endianness.hpp
///
/// @brief Provides a runtime way to determine platform endianness.
/// Useful when trying to convert bytes in memory (like float*) to a specific endianness.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#ifndef PLATFORM_ENDIANNESS_HPP
#define PLATFORM_ENDIANNESS_HPP

namespace isobus
{
	/// @brief Returns if the platform is little endian
	/// @returns `true` if the platform is little endian, otherwise false
	bool is_little_endian();

	/// @brief Returns if the platform is big endian
	/// @returns `true` if the platform is big endian, otherwise false
	bool is_big_endian();

} // namespace isobus

#endif //PLATFORM_ENDIANNESS_HPP
