#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/utility/iop_file_interface.hpp"

#include "console_logger.cpp"
#include "objectPoolObjects.h"

#include <atomic>
#include <csignal>
#include <functional>
#include <iostream>
#include <memory>

//! It is discouraged to use global variables, but it is done here for simplicity.
static std::shared_ptr<isobus::VirtualTerminalClient> TestVirtualTerminalClient = nullptr;
static std::atomic_bool running = { true };

void signal_handler(int)
{
	running = false;
}

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

int main()
{
	std::signal(SIGINT, signal_handler);

	// Automatically load the desired CAN driver based on the available drivers
	std::shared_ptr<isobus::CANHardwarePlugin> canDriver = nullptr;
#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
	canDriver = std::make_shared<isobus::SocketCANInterface>("can0");
#elif defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
	canDriver = std::make_shared<isobus::PCANBasicWindowsPlugin>(PCAN_USBBUS1);
#elif defined(ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE)
	canDriver = std::make_shared<isobus::InnoMakerUSB2CANWindowsPlugin>(0); // CAN0
#elif defined(ISOBUS_MACCANPCAN_AVAILABLE)
	canDriver = std::make_shared<isobus::MacCANPCANPlugin>(PCAN_USBBUS1);
#endif
	if (nullptr == canDriver)
	{
		std::cout << "Unable to find a CAN driver. Please make sure you have one of the above drivers installed with the library." << std::endl;
		std::cout << "If you want to use a different driver, please add it to the list above." << std::endl;
		return -1;
	}

	isobus::CANStackLogger::set_can_stack_logger_sink(&logger);
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Info); // Change this to Debug to see more information
	isobus::CANHardwareInterface::set_number_of_can_channels(1);
	isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

	if ((!isobus::CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
	{
		std::cout << "Failed to start hardware interface. The CAN driver might be invalid." << std::endl;
		return -2;
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

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

	std::vector<std::uint8_t> testPool = isobus::IOPFileInterface::read_iop_file("VT3TestPool.iop");

	if (testPool.empty())
	{
		std::cout << "Failed to load object pool from VT3TestPool.iop" << std::endl;
		return -3;
	}
	std::cout << "Loaded object pool from VT3TestPool.iop" << std::endl;

	// Generate a unique version string for this object pool (this is optional, and is entirely application specific behavior)
	std::string objectPoolHash = isobus::IOPFileInterface::hash_object_pool_to_version(testPool);

	const isobus::NAMEFilter filterVirtualTerminal(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	const std::vector<isobus::NAMEFilter> vtNameFilters = { filterVirtualTerminal };
	auto TestInternalECU = isobus::InternalControlFunction::create(TestDeviceNAME, 0x1C, 0);
	auto TestPartnerVT = isobus::PartneredControlFunction::create(0, vtNameFilters);

	TestVirtualTerminalClient = std::make_shared<isobus::VirtualTerminalClient>(TestPartnerVT, TestInternalECU);
	TestVirtualTerminalClient->set_object_pool(0, isobus::VirtualTerminalClient::VTVersion::Version3, testPool.data(), testPool.size(), objectPoolHash);
	auto softKeyListener = TestVirtualTerminalClient->add_vt_soft_key_event_listener(handleVTKeyEvents);
	auto buttonListener = TestVirtualTerminalClient->add_vt_button_event_listener(handleVTKeyEvents);
	TestVirtualTerminalClient->initialize(true);

	while (running)
	{
		// CAN stack runs in other threads. Do nothing forever.
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	TestVirtualTerminalClient->terminate();
	isobus::CANHardwareInterface::stop();
	return 0;
}
