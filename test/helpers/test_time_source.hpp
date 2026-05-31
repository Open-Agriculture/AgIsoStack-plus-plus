//================================================================================================
/// @file test_time_source.cpp
///
/// @brief Provides fake system time information for unit testing
/// @author Adrian Del Grosso
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef TEST_TIME_SOURCE_HPP
#define TEST_TIME_SOURCE_HPP

#include "isobus/utility/time_source.hpp"

namespace test_helpers
{
	/// @brief Allows full control over SystemTiming.
	/// Useful for unit testing.
	class TestTimeSource : public isobus::TimeSource
	{
	public:
		std::uint32_t get_current_time_ms() const override;
		std::uint64_t get_current_time_us() const override;

		/// @brief Set the current time to a specific timestamp
		/// @param[in] time_ms The time to set in milliseconds
		void set_time_ms(std::uint32_t time_ms);

		/// @brief Set the current time to a specific timestamp
		/// @param[in] time_us The time to set in microseconds
		void set_time_us(std::uint64_t time_us);

		/// @brief Advances time by the amount specified
		/// @param[in] delay_time_ms The amount of time in milliseconds to advance the current time
		void simulate_delay_ms(std::uint32_t delay_time_ms);

		/// @brief Advances time by the amount specified
		/// @param[in] delay_time_us The amount of time in microseconds to advance the current time
		void simulate_delay_us(std::uint64_t delay_time_us);

		/// Calls the CANHardwareInterface update function while performing a simulated delay.
		/// @param[in] time_ms The time to advance the current time by, in milliseconds
		void update_for_ms(std::uint32_t time_ms);

	private:
		std::uint64_t s_timestamp_us = 0; /// The current simulated time, stored in microseconds
	};
} // namespace test_helpers

#endif // TEST_TIME_SOURCE_HPP
