//================================================================================================
/// @file twai_plugin.cpp
///
/// @brief A driver for Two-Wire Automotive Interface (TWAI).
/// @author Daan Steenbergen
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================
#ifdef ESP_PLATFORM
#include "isobus/hardware_integration/twai_plugin.hpp"
#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <limits>

namespace isobus
{
	TWAIPlugin::TWAIPlugin(const twai_general_config_t *generalConfig, const twai_timing_config_t *timingConfig, const twai_filter_config_t *filterConfig) :
	  generalConfig(generalConfig),
	  timingConfig(timingConfig),
	  filterConfig(filterConfig)
	{
	}

	TWAIPlugin::~TWAIPlugin()
	{
		close();
	}

	bool TWAIPlugin::get_is_valid() const
	{
		twai_status_info_t status;
		esp_err_t error = twai_get_status_info(&status);
		if (ESP_OK == error)
		{
			return status.state == TWAI_STATE_RUNNING;
		}
		else
		{
			LOG_ERROR("[TWAI] Error getting status: " + isobus::to_string(esp_err_to_name(error)));
		}
		return false;
	}

	void TWAIPlugin::close()
	{
		esp_err_t error = twai_stop();
		if (ESP_OK != error)
		{
			LOG_ERROR("[TWAI] Error stopping driver: " + isobus::to_string(esp_err_to_name(error)));
		}
		error = twai_driver_uninstall();
		if (ESP_OK != error)
		{
			LOG_ERROR("[TWAI] Error uninstalling driver: " + isobus::to_string(esp_err_to_name(error)));
		}
	}

	void TWAIPlugin::open()
	{
		esp_err_t error = twai_driver_install(generalConfig, timingConfig, filterConfig);
		if (ESP_OK != error)
		{
			LOG_CRITICAL("[TWAI] Error installing driver: " + isobus::to_string(esp_err_to_name(error)));
		}
		error = twai_start();
		if (ESP_OK != error)
		{
			LOG_CRITICAL("[TWAI] Error starting driver: " + isobus::to_string(esp_err_to_name(error)));
		}
	}

	bool TWAIPlugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		bool retVal = false;

		//Wait for message to be received
		twai_message_t message = {};
		esp_err_t error = twai_receive(&message, pdMS_TO_TICKS(100));
		if (ESP_OK == error)
		{
			// Process received message
			if (!(message.rtr))
			{
				canFrame.identifier = message.identifier;
				canFrame.isExtendedFrame = message.extd;
				canFrame.dataLength = message.data_length_code;
				if (isobus::CAN_DATA_LENGTH >= canFrame.dataLength)
				{
					memset(canFrame.data, 0, sizeof(canFrame.data));
					memcpy(canFrame.data, message.data, canFrame.dataLength);
					retVal = true;
				}
			}
		}
		else if (ESP_ERR_TIMEOUT != error)
		{
			LOG_ERROR("[TWAI] Error receiving message: " + isobus::to_string(esp_err_to_name(error)));
		}

		return retVal;
	}

	bool TWAIPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
	{
		bool retVal = false;
		twai_message_t message = {};

		message.identifier = canFrame.identifier;
		message.extd = canFrame.isExtendedFrame;
		message.data_length_code = canFrame.dataLength;
		memcpy(message.data, canFrame.data, canFrame.dataLength);

		esp_err_t error = twai_transmit(&message, pdMS_TO_TICKS(100));
		if (ESP_OK == error)
		{
			retVal = true;
		}
		else
		{
			LOG_ERROR("[TWAI] Error sending message: " + isobus::to_string(esp_err_to_name(error)));
		}
		return retVal;
	}
}
#endif // ESP_PLATFORM
