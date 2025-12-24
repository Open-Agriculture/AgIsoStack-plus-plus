#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/twai_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client_update_helper.hpp"
#include "isobus/utility/iop_file_interface.hpp"

#include "console_logger.cpp"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "objectPoolObjects.h"

#include <functional>
#include <iostream>
#include <memory>

//! It is discouraged to use global variables, but it is done here for simplicity.
static std::shared_ptr<isobus::VirtualTerminalClient> virtualTerminalClient = nullptr;
static std::shared_ptr<isobus::VirtualTerminalClientUpdateHelper> virtualTerminalUpdateHelper = nullptr;

// This callback will provide us with event driven notifications of softkey presses from the stack
void handle_softkey_event(const isobus::VirtualTerminalClient::VTKeyEvent &event)
{
	if (event.keyNumber == 0)
	{
		// We have the alarm ACK code, so if we have an active alarm, acknowledge it by going back to the main runscreen
		virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(example_WorkingSet, mainRunscreen_DataMask);
	}

	switch (event.keyEvent)
	{
		case isobus::VirtualTerminalClient::KeyActivationCode::ButtonUnlatchedOrReleased:
		{
			switch (event.objectID)
			{
				case alarm_SoftKey:
				{
					virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(example_WorkingSet, example_AlarmMask);
				}
				break;

				case acknowledgeAlarm_SoftKey:
				{
					virtualTerminalUpdateHelper->set_active_data_or_alarm_mask(example_WorkingSet, mainRunscreen_DataMask);
				}
				break;

				default:
					break;
			}
		}
		break;

		default:
			break;
	}
}

// This callback will provide us with event driven notifications of button presses from the stack
void handle_button_event(const isobus::VirtualTerminalClient::VTKeyEvent &event)
{
	switch (event.keyEvent)
	{
		case isobus::VirtualTerminalClient::KeyActivationCode::ButtonUnlatchedOrReleased:
		case isobus::VirtualTerminalClient::KeyActivationCode::ButtonStillHeld:
		{
			switch (event.objectID)
			{
				case Plus_Button:
				{
					virtualTerminalUpdateHelper->increase_numeric_value(ButtonExampleNumber_VarNum);
				}
				break;

				case Minus_Button:
				{
					virtualTerminalUpdateHelper->decrease_numeric_value(ButtonExampleNumber_VarNum);
				}
				break;

				default:
					break;
			}
		}
		break;

		default:
			break;
	}
}

extern "C" const std::uint8_t object_pool_start[] asm("_binary_object_pool_iop_start");
extern "C" const std::uint8_t object_pool_end[] asm("_binary_object_pool_iop_end");

extern "C" void app_main()
{
	// Automatically load the desired CAN driver based on the available drivers
	twai_general_config_t twaiConfig = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, TWAI_MODE_NORMAL);
	twai_timing_config_t twaiTiming = TWAI_TIMING_CONFIG_250KBITS();
	twai_filter_config_t twaiFilter = TWAI_FILTER_CONFIG_ACCEPT_ALL();
	std::shared_ptr<isobus::CANHardwarePlugin> canDriver = std::make_shared<isobus::TWAIPlugin>(&twaiConfig, &twaiTiming, &twaiFilter);

	isobus::CANStackLogger::set_can_stack_logger_sink(&logger);
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Info); // Change this to Debug to see more information
	isobus::CANHardwareInterface::set_number_of_can_channels(1);
	isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);
	// isobus::CANHardwareInterface::set_periodic_update_interval(10); // 10ms update period matches the default FreeRTOS tick rate of 100Hz

	if (!isobus::CANHardwareInterface::start() || !canDriver->get_is_valid())
	{
		ESP_LOGE("AgIsoStack", "Failed to start hardware interface, the CAN driver might be invalid");
	}

	isobus::NAME TestDeviceNAME(0);

	//! Make sure you change these for your device!!!!
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(1);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(1407);

	const std::uint8_t *testPool = object_pool_start;

	const isobus::NAMEFilter filterVirtualTerminal(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	const std::vector<isobus::NAMEFilter> vtNameFilters = { filterVirtualTerminal };
	auto TestInternalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);
	auto TestPartnerVT = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

	virtualTerminalClient = std::make_shared<isobus::VirtualTerminalClient>(TestPartnerVT, TestInternalECU);
	virtualTerminalClient->set_object_pool(0, testPool, (object_pool_end - object_pool_start), "ais1");
	virtualTerminalClient->get_vt_soft_key_event_dispatcher().add_listener(handle_softkey_event);
	virtualTerminalClient->get_vt_button_event_dispatcher().add_listener(handle_button_event);
	virtualTerminalClient->initialize(true);

	virtualTerminalUpdateHelper = std::make_shared<isobus::VirtualTerminalClientUpdateHelper>(virtualTerminalClient);
	virtualTerminalUpdateHelper->add_tracked_numeric_value(ButtonExampleNumber_VarNum, 214748364); // In the object pool the output number has an offset of -214748364 so we use this to represent 0.
	virtualTerminalUpdateHelper->initialize();

	while (true)
	{
		// CAN stack runs in other threads. Do nothing forever.
		vTaskDelay(10);
	}

	virtualTerminalClient->terminate();
	isobus::CANHardwareInterface::stop();
}
