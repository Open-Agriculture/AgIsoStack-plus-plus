//================================================================================================
/// @file time_source.hpp
///
/// @brief Abstraction for getting the current time.
/// @author Adrian Del Grosso
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef TIME_SOURCE_HPP
#define TIME_SOURCE_HPP

#include <cstdint>

namespace isobus
{
	/// @brief Abstraction for getting the current time. Allows for overriding the time source.
	class TimeSource
	{
	public:
		/// @brief Returns the current time as a millisecond timestamp.
		/// Should return 0 at the instant the application is started and increment
		/// monotonically until the application exits.
		virtual std::uint32_t get_current_time_ms() const = 0;

		/// @brief Returns the current time as a microsecond timestamp.
		/// Should return 0 at the instant the application is started and increment
		/// monotonically until the application exits.
		virtual std::uint64_t get_current_time_us() const = 0;
	};

} // namespace isobus

#endif // TIME_SOURCE_HPP
