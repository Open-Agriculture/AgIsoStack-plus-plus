//================================================================================================
/// @file logger_tests.cpp
///
/// @brief Checks that a log statement below the log level, or with no sink set, is dropped
/// before its arguments are evaluated or formatted.
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include <gtest/gtest.h>

#include "isobus/isobus/can_stack_logger.hpp"

#include <string>
#include <vector>

using namespace isobus;

#ifndef DISABLE_CAN_STACK_LOGGER

class RecordingLogger : public CANStackLogger
{
public:
	void sink_CAN_stack_log(LoggingLevel, const std::string &text) override
	{
		logged.push_back(text);
	}

	std::vector<std::string> logged;
};

class LoggerTest : public ::testing::Test
{
protected:
	void TearDown() override
	{
		// The sink and level are static, so later tests in the same process would inherit them
		CANStackLogger::set_log_level(CANStackLogger::LoggingLevel::Info);
		CANStackLogger::set_can_stack_logger_sink(nullptr);
	}

	int count_evaluation()
	{
		return ++argumentEvaluations;
	}

	RecordingLogger logger;
	int argumentEvaluations = 0;
};

TEST_F(LoggerTest, DroppedLogDoesNotEvaluateArguments)
{
	CANStackLogger::set_can_stack_logger_sink(nullptr);
	LOG_DEBUG("[Test]: %d", count_evaluation());
	EXPECT_EQ(0, argumentEvaluations);

	CANStackLogger::set_can_stack_logger_sink(&logger);
	CANStackLogger::set_log_level(CANStackLogger::LoggingLevel::Info);
	LOG_DEBUG("[Test]: %d", count_evaluation());
	EXPECT_EQ(0, argumentEvaluations);
	EXPECT_TRUE(logger.logged.empty());

	LOG_INFO("[Test]: %d", count_evaluation());
	EXPECT_EQ(1, argumentEvaluations);
	EXPECT_EQ(std::vector<std::string>{ "[Test]: 1" }, logger.logged);

	CANStackLogger::set_log_level(CANStackLogger::LoggingLevel::Debug);
	LOG_DEBUG("[Test]: %d", count_evaluation());
	EXPECT_EQ((std::vector<std::string>{ "[Test]: 1", "[Test]: 2" }), logger.logged);
}
#endif
