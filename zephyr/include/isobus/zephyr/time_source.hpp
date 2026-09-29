//================================================================================================
/// @file time_source.hpp
/// @brief Zephyr uptime backed time source for AgIsoStack++.
//================================================================================================
#ifndef ISOBUS_ZEPHYR_TIME_SOURCE_HPP
#define ISOBUS_ZEPHYR_TIME_SOURCE_HPP

#include "isobus/utility/time_source.hpp"

#include <zephyr/kernel.h>

#include <cstdint>

namespace isobus
{
	class ZephyrTimeSource : public TimeSource
	{
	public:
		ZephyrTimeSource() :
		  startTicks(k_uptime_ticks())
		{
		}

		std::uint32_t get_current_time_ms() const override
		{
			return static_cast<std::uint32_t>(k_ticks_to_ms_floor64(elapsed_ticks()));
		}

		std::uint64_t get_current_time_us() const override
		{
			return k_ticks_to_us_floor64(elapsed_ticks());
		}

	private:
		std::uint64_t elapsed_ticks() const
		{
			return static_cast<std::uint64_t>(k_uptime_ticks() - startTicks);
		}

		const std::int64_t startTicks;
	};
} // namespace isobus

#endif // ISOBUS_ZEPHYR_TIME_SOURCE_HPP
