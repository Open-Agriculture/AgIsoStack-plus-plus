//================================================================================================
/// @file system_timing.cpp
///
/// @brief Utility class for getting system time and handling u32 time rollover
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================

#include "isobus/utility/system_timing.hpp"

#include "isobus/utility/chrono_time_source.hpp"

namespace isobus
{
	TimeSource *SystemTiming::s_custom_timesource = nullptr;
	static ChronoTimeSource default_time_source;

	std::uint32_t SystemTiming::get_timestamp_ms()
	{
		return get_active_time_source()->get_current_time_ms();
	}

	std::uint64_t SystemTiming::get_timestamp_us()
	{
		return get_active_time_source()->get_current_time_us();
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

	void SystemTiming::override_time_source(TimeSource *source)
	{
		s_custom_timesource = source;
	}

	const TimeSource *SystemTiming::get_active_time_source()
	{
		if (nullptr != s_custom_timesource)
		{
			return s_custom_timesource;
		}
		return &default_time_source;
	}

}
