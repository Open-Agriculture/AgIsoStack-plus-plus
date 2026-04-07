//================================================================================================
/// @file system_timing.hpp
///
/// @brief Utility class for getting system time and handling u32 and u64 time rollover
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#ifndef SYSTEM_TIMING_HPP
#define SYSTEM_TIMING_HPP

#include "isobus/utility/time_source.hpp"

namespace isobus
{
	/// @brief Utility class that provides a consistent way to get a timestamp, check for
	/// timeouts, and perform basic time comparisons. By default, the system time comes
	/// from std::chrono.
	class SystemTiming
	{
	public:
		/// @brief Returns a monotonic millisecond timestamp
		/// starting at 0 and incrementing until the application exits.
		static std::uint32_t get_timestamp_ms();

		/// @brief Returns a monotonic microsecond timestamp
		/// starting at 0 and incrementing until the application exits.
		static std::uint64_t get_timestamp_us();

		/// @brief Returns the amount of time (in milliseconds) that has elapsed since
		/// the provided millisecond timestamp.
		/// @param[in] timestamp_ms The millisecond timestamp to compare against
		static std::uint32_t get_time_elapsed_ms(std::uint32_t timestamp_ms);

		/// @brief Returns the amount of time (in microseconds) that has elapsed since
		/// the provided microsecond timestamp.
		/// @param[in] timestamp_us The microsecond timestamp to compare against
		static std::uint64_t get_time_elapsed_us(std::uint64_t timestamp_us);

		/// @brief Returns true if the amount of time elapsed since timestamp_ms is at least
		// as long as timeout_ms. Basically an alternate way to check if get_time_elapsed_ms >= timeout_ms
		/// @param[in] timestamp_ms The timestamp to check against (in milliseconds)
		/// @param[in] timeout_ms The timeout in milliseconds to check against
		/// @returns true if at least timeout_ms has passed since timestamp_ms was taken, otherwise false
		static bool time_expired_ms(std::uint32_t timestamp_ms, std::uint32_t timeout_ms);

		/// @brief Returns true if the amount of time elapsed since timestamp_us is at least
		// as long as timeout_us. Basically an alternate way to check if get_time_elapsed_us >= timeout_us
		/// @param[in] timestamp_us The timestamp to check against (in microseconds)
		/// @param[in] timeout_us The timeout in microseconds to check against
		/// @returns true if at least timeout_us has passed since timestamp_us was taken, otherwise false
		static bool time_expired_us(std::uint64_t timestamp_us, std::uint64_t timeout_us);

		/// @brief Allows changing the source of the current time from std::chrono to a custom implementation.
		/// @param[in] source The time source for getting the current time. You can pass in nullptr to return to the
		/// default std::chrono time source.
		/// @attention The provided time source must not be deleted while this class is using it! Additionally, this
		/// interface is not thread safe, so you should only set it while the stack is in a stopped condition.
		static void override_time_source(TimeSource *source);

	private:
		static std::uint32_t incrementing_difference(std::uint32_t currentValue, std::uint32_t previousValue);
		static std::uint64_t incrementing_difference(std::uint64_t currentValue, std::uint64_t previousValue);
		static TimeSource const *get_active_time_source();
		static TimeSource *s_custom_timesource;
	};

} // namespace isobus

#endif // SYSTEM_TIMING_HPP
