#include "isobus/isobus/can_stack_logger.hpp"

#include <iostream>

// A log sink for the CAN stack
class CustomLogger : public isobus::CANStackLogger
{
public:
	/// @brief Destructor for the custom logger.
	virtual ~CustomLogger() = default;

	void sink_CAN_stack_log(CANStackLogger::LoggingLevel level, const std::string &text) override
	{
		switch (level)
		{
			case LoggingLevel::Debug:
			{
				std::cout << "["
				          << "\033[1;36m"
				          << "Debug"
				          << "\033[0m"
				          << "]";
			}
			break;

			case LoggingLevel::Info:
			{
				std::cout << "["
				          << "\033[1;32m"
				          << "Info"
				          << "\033[0m"
				          << "]";
			}
			break;

			case LoggingLevel::Warning:
			{
				std::cout << "["
				          << "\033[1;33m"
				          << "Warn"
				          << "\033[0m"
				          << "]";
			}
			break;

			case LoggingLevel::Error:
			{
				std::cout << "["
				          << "\033[1;31m"
				          << "Error"
				          << "\033[0m"
				          << "]";
			}
			break;

			case LoggingLevel::Critical:
			{
				std::cout << "["
				          << "\033[1;35m"
				          << "Critical"
				          << "\033[0m"
				          << "]";
			}
			break;
		}
		std::cout << text << std::endl; // Write the text to stdout
	}
};

static CustomLogger logger;
