#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/twai_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/utility/iop_file_interface.hpp"

#include "console_logger.cpp"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "objectPoolObjects.h"

#include <functional>
#include <iostream>
#include <memory>

//! It is discouraged to use global variables, but it is done here for simplicity.
static std::shared_ptr<isobus::VirtualTerminalClient> TestVirtualTerminalClient = nullptr;

// This callback will provide us with event driven notifications of button presses from the stack
void handleVTKeyEvents(const isobus::VirtualTerminalClient::VTKeyEvent &event)
{
	static std::uint32_t exampleNumberOutput = 214748364; // In the object pool the output number has an offset of -214748364 so we use this to represent 0.

	switch (event.keyEvent)
	{
		case isobus::VirtualTerminalClient::KeyActivationCode::ButtonUnlatchedOrReleased:
		{
			switch (event.objectID)
			{
				case Plus_Button:
				{
					TestVirtualTerminalClient->send_change_numeric_value(ButtonExampleNumber_VarNum, ++exampleNumberOutput);
				}
				break;

				case Minus_Button:
				{
					TestVirtualTerminalClient->send_change_numeric_value(ButtonExampleNumber_VarNum, --exampleNumberOutput);
				}
				break;

				case alarm_SoftKey:
				{
					TestVirtualTerminalClient->send_change_active_mask(example_WorkingSet, example_AlarmMask);
				}
				break;

				case acknowledgeAlarm_SoftKey:
				{
					TestVirtualTerminalClient->send_change_active_mask(example_WorkingSet, mainRunscreen_DataMask);
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

extern "C" const uint8_t object_pool_start[] asm("_binary_object_pool_iop_start");
extern "C" const uint8_t object_pool_end[] asm("_binary_object_pool_iop_end");

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
	isobus::CANHardwareInterface::set_periodic_update_interval(1);

	if (!isobus::CANHardwareInterface::start() || !canDriver->get_is_valid())
	{
		ESP_LOGE("AgIsoStack", "Failed to start hardware interface, the CAN driver might be invalid");
	}

	isobus::NAME TestDeviceNAME(0);

	//! Make sure you change these for your device!!!!
	//! This is an example device that is using a manufacturer code that is currently unused at time of writing
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(1);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);

	const std::uint8_t *testPool = object_pool_start;

	const isobus::NAMEFilter filterVirtualTerminal(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	const std::vector<isobus::NAMEFilter> vtNameFilters = { filterVirtualTerminal };
	auto TestInternalECU = isobus::InternalControlFunction::create(TestDeviceNAME, 0x1C, 0);
	auto TestPartnerVT = isobus::PartneredControlFunction::create(0, vtNameFilters);

	TestVirtualTerminalClient = std::make_shared<isobus::VirtualTerminalClient>(TestPartnerVT, TestInternalECU);
	TestVirtualTerminalClient->set_object_pool(0, isobus::VirtualTerminalClient::VTVersion::Version3, testPool, (object_pool_end - object_pool_start) - 1, "ais1");
	auto softKeyListener = TestVirtualTerminalClient->add_vt_soft_key_event_listener(handleVTKeyEvents);
	auto buttonListener = TestVirtualTerminalClient->add_vt_button_event_listener(handleVTKeyEvents);
	TestVirtualTerminalClient->initialize(true);

	while (true)
	{
		// CAN stack runs in other threads. Do nothing forever.
		vTaskDelay(10);
	}

	TestVirtualTerminalClient->terminate();
	isobus::CANHardwareInterface::stop();
}
