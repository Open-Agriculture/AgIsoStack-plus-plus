//================================================================================================
/// @file platform_endianness.hpp
///
/// @brief Provides a runtime way to determine platform endianness.
/// Useful when trying to convert bytes in memory (like float*) to a specific endianness.
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#ifndef PLATFORM_ENDIANNESS_HPP
#define PLATFORM_ENDIANNESS_HPP

#include <cstdint>

namespace isobus
{
	/// @brief Returns if the platform is little endian
	/// @returns `true` if the platform is little endian, otherwise false
	bool is_little_endian();

	/// @brief Returns if the platform is big endian
	/// @returns `true` if the platform is big endian, otherwise false
	bool is_big_endian();

	/// @brief Convert a float attribute to a little endian byte representation
	/// @param[in] value The float value to convert
	/// @returns The float value as a little endian byte representation
	std::uint32_t float_to_little_endian(float value);

	/// @brief Convert a little endian byte representation to a float attribute
	/// @param[in] byteRepresentation The little endian byte representation to convert
	/// @returns The float value
	float little_endian_to_float(std::uint32_t byteRepresentation);

} // namespace isobus

#endif //PLATFORM_ENDIANNESS_HPP
