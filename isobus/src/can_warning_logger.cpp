//================================================================================================
/// @file can_warning_logger.cpp
///
/// @brief A class that acts as a logging sink. The intent is that someone could make their own
/// derived class of logger and inject it into the CAN stack to get helpful debug logging.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/can_warning_logger.hpp"

#include <iostream>

namespace isobus
{
	CANStackLogger *CANStackLogger::logger = nullptr;

	CANStackLogger::CANStackLogger()
	{
	}

	CANStackLogger::~CANStackLogger()
	{
	}

	void CANStackLogger::CAN_stack_log(const std::string &warningText)
	{
		CANStackLogger *canStackLogger = nullptr;

		if (get_can_stack_logger(canStackLogger))
		{
			canStackLogger->LogCANLibWarning(warningText);
		}
	}

	void CANStackLogger::set_can_stack_logger_sink(CANStackLogger *logSink)
	{
		logger = logSink;
	}

	void CANStackLogger::LogCANLibWarning(const std::string &)
	{
		// Override this function to use the log sink
	}

	bool CANStackLogger::get_can_stack_logger(CANStackLogger *canStackLogger)
	{
		canStackLogger = logger;
		return (nullptr != canStackLogger);
	}
} // namespace isobus
