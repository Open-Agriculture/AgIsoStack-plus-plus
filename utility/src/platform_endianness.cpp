//================================================================================================
/// @file platform_endianness.cpp
///
/// @brief Provides a runtime way to determine platform endianness.
/// Useful when trying to convert bytes in memory (like float*) to a specific endianness.
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#include "isobus/utility/platform_endianness.hpp"

#include <cstdint>
#include <cstring>

namespace isobus
{
	bool is_little_endian()
	{
		std::int32_t number = 1;
		auto numPtr = reinterpret_cast<char *>(&number);
		return (numPtr[0] == 1);
	}

	bool is_big_endian()
	{
		return (false == is_little_endian());
	}

	std::uint32_t float_to_little_endian(float value)
	{
		static_assert(sizeof(float) == 4, "Float must be 4 bytes");
		std::uint32_t byteRepresentation;
		std::memcpy(&byteRepresentation, &value, sizeof(float));
		if (is_big_endian())
		{
			byteRepresentation =
			  ((byteRepresentation & 0x000000FF) << 24) |
			  ((byteRepresentation & 0x0000FF00) << 8) |
			  ((byteRepresentation & 0x00FF0000) >> 8) |
			  ((byteRepresentation & 0xFF000000) >> 24);
		}
		return byteRepresentation;
	}

	float little_endian_to_float(std::uint32_t byteRepresentation)
	{
		static_assert(sizeof(float) == 4, "Float must be 4 bytes");
		if (is_big_endian())
		{
			byteRepresentation =
			  ((byteRepresentation & 0x000000FF) << 24) |
			  ((byteRepresentation & 0x0000FF00) << 8) |
			  ((byteRepresentation & 0x00FF0000) >> 8) |
			  ((byteRepresentation & 0xFF000000) >> 24);
		}
		float value;
		std::memcpy(&value, &byteRepresentation, sizeof(float));
		return value;
	}
}
// namespace isobus
