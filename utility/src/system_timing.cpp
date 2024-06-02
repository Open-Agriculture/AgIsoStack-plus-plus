//================================================================================================
/// @file system_timing.cpp
///
/// @brief Utility class for getting system time and handling u32 time rollover
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================

#include "isobus/utility/system_timing.hpp"

#include <chrono>
#include <limits>

namespace isobus
{
	std::uint64_t SystemTiming::s_timestamp_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
	std::uint64_t SystemTiming::s_timestamp_us = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());

	std::uint32_t SystemTiming::get_timestamp_ms()
	{
		return incrementing_difference(static_cast<std::uint32_t>(static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) & std::numeric_limits<std::uint32_t>::max()), static_cast<std::uint32_t>(s_timestamp_ms));
	}

	std::uint64_t SystemTiming::get_timestamp_us()
	{
		return incrementing_difference(static_cast<std::uint64_t>(static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()) & std::numeric_limits<std::uint64_t>::max()), s_timestamp_us);
	}

	std::uint32_t SystemTiming::get_time_elapsed_ms(std::uint32_t timestamp_ms)
	{
		return (get_timestamp_ms() - timestamp_ms);
	}

	std::uint64_t SystemTiming::get_time_elapsed_us(std::uint64_t timestamp_us)
	{
		return (get_timestamp_us() - timestamp_us);
	}

	bool SystemTiming::time_expired_ms(std::uint32_t timestamp_ms, std::uint32_t timeout_ms)
	{
		return (get_time_elapsed_ms(timestamp_ms) >= timeout_ms);
	}

	bool SystemTiming::time_expired_us(std::uint64_t timestamp_us, std::uint64_t timeout_us)
	{
		return (get_time_elapsed_us(timestamp_us) >= timeout_us);
	}

	std::uint32_t SystemTiming::incrementing_difference(std::uint32_t currentValue, std::uint32_t previousValue)
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

	std::uint64_t SystemTiming::incrementing_difference(std::uint64_t currentValue, std::uint64_t previousValue)
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

}
