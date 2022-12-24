//================================================================================================
/// @file twai_plugin.cpp
///
/// @brief A driver for Two-Wire Automotive Interface (TWAI).
/// @author Daan Steenbergen
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================
#include <isobus/hardware_integration/twai_plugin.hpp>
#include <isobus/isobus/can_warning_logger.hpp>
#include "isobus/utility/system_timing.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <limits>

TWAIPlugin::TWAIPlugin(const twai_general_config_t generalConfig, const twai_timing_config_t timingConfig, const twai_filter_config_t filterConfig) :
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
	if (twai_get_status_info(&status) == ESP_OK)
	{
		return status.state == TWAI_STATE_RUNNING;
	}
	return false;
}

void TWAIPlugin::close()
{
	twai_stop();
	twai_driver_uninstall();
}

void TWAIPlugin::open()
{
	twai_driver_install(&generalConfig, &timingConfig, &filterConfig);
	twai_start();
}

bool TWAIPlugin::read_frame(isobus::HardwareInterfaceCANFrame &canFrame)
{
	bool retVal = false;

	//Wait for message to be received
	twai_message_t message;
	if (twai_receive(&message, portMAX_DELAY) == ESP_OK)
	{
		// Process received message
		if (!(message.rtr))
		{
			canFrame.identifier = message.identifier;
			canFrame.isExtendedFrame = message.extd;
			canFrame.dataLength = message.data_length_code;
			memset(canFrame.data, 0, sizeof(canFrame.data));
			memcpy(canFrame.data, message.data, canFrame.dataLength);
			retVal = true;
		}
	}

	return retVal;
}

bool TWAIPlugin::write_frame(const isobus::HardwareInterfaceCANFrame &canFrame)
{
	bool retVal = false;
	twai_message_t message;

	message.identifier = canFrame.identifier;
	message.extd = canFrame.isExtendedFrame;
	message.data_length_code = canFrame.dataLength;
	memcpy(message.data, canFrame.data, canFrame.dataLength);

	if (twai_transmit(&message, portMAX_DELAY) == ESP_OK)
	{
		retVal = true;
	}
	return retVal;
}