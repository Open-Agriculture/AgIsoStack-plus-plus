//================================================================================================
/// @file chrono_time_source.hpp
///
/// @brief An interface used by SystemTiming to get the time from std::chrono
/// @author Adrian Del Grosso
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef CHRONO_TIME_SOURCE_HPP
#define CHRONO_TIME_SOURCE_HPP

#include "isobus/utility/time_source.hpp"

namespace isobus
{
	/// @brief An interface used by SystemTiming to get the time from std::chrono
	class ChronoTimeSource : public TimeSource
	{
	public:
		std::uint32_t get_current_time_ms() const override;
		std::uint64_t get_current_time_us() const override;

	private:
		static std::uint64_t s_timestamp_ms;
		static std::uint64_t s_timestamp_us;
	};
} // namespace isobus

#endif // CHRONO_TIME_SOURCE_HPP
