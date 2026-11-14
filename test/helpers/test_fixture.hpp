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
		isobus::SystemTiming::override_time_source(nullptr);
		isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Info);
		isobus::CANStackLogger::set_can_stack_logger_sink(nullptr);
	}

	test_helpers::TestTimeSource time_source;
	isobus::CANStackLogger discardingLogger;
};

#endif // TEST_TIME_SOURCE_HPP
