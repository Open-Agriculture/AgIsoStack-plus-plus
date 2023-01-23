//================================================================================================
/// @file can_stack_logger.cpp
///
/// @brief A class that acts as a logging sink. The intent is that someone could make their own
/// derived class of logger and inject it into the CAN stack to get helpful debug logging.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/can_stack_logger.hpp"

#include <iostream>

namespace isobus
{
	CANStackLogger *CANStackLogger::logger = nullptr;
	CANStackLogger::LoggingLevel CANStackLogger::currentLogLevel = LoggingLevel::Info;
	std::mutex CANStackLogger::loggerMutex;

	CANStackLogger::CANStackLogger()
	{
	}

	CANStackLogger::~CANStackLogger()
	{
	}

	void CANStackLogger::CAN_stack_log(LoggingLevel level, const std::string &logText)
	{
		const std::lock_guard<std::mutex> lock(loggerMutex);
		CANStackLogger *canStackLogger = nullptr;

		if ((get_can_stack_logger(canStackLogger)) &&
		    (level >= get_log_level()))
		{
			canStackLogger->sink_CAN_stack_log(level, logText);
		}
	}

	void CANStackLogger::debug(const std::string &logText)
	{
		CAN_stack_log(LoggingLevel::Debug, logText);
	}

	void CANStackLogger::info(const std::string &logText)
	{
		CAN_stack_log(LoggingLevel::Info, logText);
	}

	void CANStackLogger::warn(const std::string &logText)
	{
		CAN_stack_log(LoggingLevel::Warning, logText);
	}

	void CANStackLogger::error(const std::string &logText)
	{
		CAN_stack_log(LoggingLevel::Error, logText);
	}

	void CANStackLogger::critical(const std::string &logText)
	{
		CAN_stack_log(LoggingLevel::Critical, logText);
	}

	void CANStackLogger::set_can_stack_logger_sink(CANStackLogger *logSink)
	{
		logger = logSink;
	}

	CANStackLogger::LoggingLevel CANStackLogger::get_log_level()
	{
		return currentLogLevel;
	}

	void CANStackLogger::set_log_level(LoggingLevel newLogLevel)
	{
		currentLogLevel = newLogLevel;
	}

	void CANStackLogger::sink_CAN_stack_log(LoggingLevel, const std::string &)
	{
		// Override this function to use the log sink
	}

	bool CANStackLogger::get_can_stack_logger(CANStackLogger *&canStackLogger)
	{
		canStackLogger = logger;
		return (nullptr != canStackLogger);
	}
} // namespace isobus
