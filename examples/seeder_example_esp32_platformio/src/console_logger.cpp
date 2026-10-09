#include "esp_log.h"
#include "isobus/isobus/can_stack_logger.hpp"

#include <sstream>

// A log sink for the CAN stack
class CustomLogger : public isobus::CANStackLogger
{
public:
	void sink_CAN_stack_log(CANStackLogger::LoggingLevel level, const std::string &text) override
	{
		static const char *TAG = "AgIsoStack";
		std::ostringstream oss;
		switch (level)
		{
			case LoggingLevel::Debug:
			{
				oss << "["
				    << "\033[1;36m"
				    << "Debug"
				    << "\033[0m"
				    << "]";
				oss << text;
				ESP_LOGD(TAG, "%s", oss.str().c_str());
			}
			break;

			case LoggingLevel::Info:
			{
				oss << "["
				    << "\033[1;32m"
				    << "Info"
				    << "\033[0m"
				    << "]";
				oss << text;
				ESP_LOGI(TAG, "%s", oss.str().c_str());
			}
			break;

			case LoggingLevel::Warning:
			{
				oss << "["
				    << "\033[1;33m"
				    << "Warn"
				    << "\033[0m"
				    << "]";
				oss << text;
				ESP_LOGW(TAG, "%s", oss.str().c_str());
			}
			break;

			case LoggingLevel::Error:
			{
				oss << "["
				    << "\033[1;31m"
				    << "Error"
				    << "\033[0m"
				    << "]";
				oss << text;
				ESP_LOGE(TAG, "%s", oss.str().c_str());
			}
			break;

			case LoggingLevel::Critical:
			{
				oss << "["
				    << "\033[1;35m"
				    << "Critical"
				    << "\033[0m"
				    << "]";
				oss << text;
				ESP_LOGE(TAG, "%s", oss.str().c_str());
			}
			break;
		}
	}
};

static CustomLogger logger;
