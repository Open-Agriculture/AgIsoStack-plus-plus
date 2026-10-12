//================================================================================================
/// @file test_time_source.cpp
///
/// @brief Provides fake system time for unit test purposes.
/// @author Adrian Del Grosso
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================

#include "test_time_source.hpp"

#include "isobus/hardware_integration/can_hardware_interface.hpp"

namespace test_helpers
{
	std::uint32_t TestTimeSource::get_current_time_ms() const
	{
		return s_timestamp_us / 1000U;
	}

	std::uint64_t TestTimeSource::get_current_time_us() const
	{
		return s_timestamp_us;
	}

	void TestTimeSource::set_time_ms(std::uint32_t time_ms)
	{
		s_timestamp_us = time_ms * 1000U;
	}

	void TestTimeSource::set_time_us(std::uint64_t time_us)
	{
		s_timestamp_us = time_us;
	}

	void TestTimeSource::simulate_delay_ms(std::uint32_t delay_time_ms)
	{
		s_timestamp_us += (delay_time_ms * 1000U);
	}

	void TestTimeSource::simulate_delay_us(std::uint64_t delay_time_us)
	{
		s_timestamp_us += delay_time_us;
	}

	void TestTimeSource::update_for_ms(std::uint32_t time_ms)
	{
		for (std::uint32_t i = 0; i < time_ms; i++)
		{
			isobus::CANHardwareInterface::update();
			simulate_delay_ms(1);
		}
	}
} // namespce test_helpers
