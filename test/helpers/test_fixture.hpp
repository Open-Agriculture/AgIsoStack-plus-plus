//================================================================================================
/// @file test_fixture.hpp
///
/// @brief Provides common setup code for unit tests
/// @author Adrian Del Grosso
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#ifndef TEST_FIXTURE_HPP
#define TEST_FIXTURE_HPP

#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include "test_time_source.hpp"

//namespace test_helpers

class AgIsoStackTestFixture : public ::testing::Test
{
public:
	void SetUp() override
	{
		// Code here runs BEFORE every TEST_F(MyTestFixture, ...)
		isobus::SystemTiming::override_time_source(&time_source);
		// LOG_* skips its arguments when nothing would be logged, so log everything to keep them evaluated in tests
		isobus::CANStackLogger::set_can_stack_logger_sink(&discardingLogger);
		isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Debug);
	}

	void TearDown() override
	{
		// A test that stops early on a failed ASSERT leaves the interface running, and the next test could not set it up
		isobus::CANHardwareInterface::stop();

		// Deactivate the internal and partnered control functions, so that a later test in the same process can claim their addresses.
		// A partner that still has a valid address is replaced by an external control function, which keeps its address claimed.
		for (const auto &partner : isobus::CANNetworkManager::CANNetwork.get_partnered_control_functions())
		{
			isobus::CANNetworkManager::CANNetwork.deactivate_control_function(partner);
		}
		for (const auto &internal : isobus::CANNetworkManager::CANNetwork.get_internal_control_functions())
		{
			isobus::CANNetworkManager::CANNetwork.deactivate_control_function(internal);
		}

		isobus::SystemTiming::override_time_source(nullptr);
		isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Info);
		isobus::CANStackLogger::set_can_stack_logger_sink(nullptr);
	}

	test_helpers::TestTimeSource time_source;
	isobus::CANStackLogger discardingLogger;
};

#endif // TEST_TIME_SOURCE_HPP
