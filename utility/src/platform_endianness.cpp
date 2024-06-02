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
} // namespace isobus
