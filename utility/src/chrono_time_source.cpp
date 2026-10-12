//================================================================================================
/// @file chrono_time_source.cpp
///
/// @brief A time source for SystemTiming that uses std::chrono
/// @author Adrian Del Grosso
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include "isobus/utility/chrono_time_source.hpp"

#include <chrono>
#include <limits>

namespace isobus
{

	std::uint64_t ChronoTimeSource::s_timestamp_ms = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
	std::uint64_t ChronoTimeSource::s_timestamp_us = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());

	static inline std::uint32_t incrementing_difference(std::uint32_t currentValue, std::uint32_t previousValue)
	{
		std::uint32_t retVal;

		if (currentValue >= previousValue)
		{
			retVal = currentValue - previousValue;
		}
		else
		{
			retVal = (std::numeric_limits<std::uint32_t>::max() - previousValue) + currentValue + 1;
		}
		return retVal;
	}

	static inline std::uint64_t incrementing_difference(std::uint64_t currentValue, std::uint64_t previousValue)
	{
		std::uint64_t retVal;

		if (currentValue >= previousValue)
		{
			retVal = currentValue - previousValue;
		}
		else
		{
			retVal = (std::numeric_limits<std::uint64_t>::max() - previousValue) + currentValue + 1;
		}
		return retVal;
	}

	std::uint32_t ChronoTimeSource::get_current_time_ms() const
	{
		return incrementing_difference(static_cast<std::uint32_t>(static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) & std::numeric_limits<std::uint32_t>::max()), static_cast<std::uint32_t>(s_timestamp_ms));
	}

	std::uint64_t ChronoTimeSource::get_current_time_us() const
	{
		return incrementing_difference(static_cast<std::uint64_t>(static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) & std::numeric_limits<std::uint64_t>::max()), s_timestamp_us);
	}

} // namespace isobus
