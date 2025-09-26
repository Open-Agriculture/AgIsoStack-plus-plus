#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client_update_helper.hpp"
#include "isobus/utility/iop_file_interface.hpp"

#include "console_logger.cpp"
#include "objectPoolObjects.h"

#include <atomic>
#include <csignal>
#include <iostream>

//! It is discouraged to use global variables, but it is done here for simplicity.
static std::shared_ptr<isobus::VirtualTerminalClient> virtualTerminalClient = nullptr;
static std::shared_ptr<isobus::VirtualTerminalClientUpdateHelper> virtualTerminalUpdateHelper = nullptr;
static std::atomic_bool running = { true };

void signal_handler(int)
{
	running = false;
}

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
#elif defined(ISOBUS_SYS_TEC_AVAILABLE)
	canDriver = std::make_shared<isobus::SysTecWindowsPlugin>();
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
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(1);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(1407);

	std::vector<std::uint8_t> version3pool = isobus::IOPFileInterface::read_iop_file("VT3TestPool.iop");
	std::vector<std::uint8_t> version4pool = isobus::IOPFileInterface::read_iop_file("window_masks.iop");

	if (version3pool.empty())
	{
		std::cout << "Failed to load object pool from VT3TestPool.iop" << std::endl;
		return -3;
	}
	std::cout << "Loaded object pool from VT3TestPool.iop" << std::endl;

	if (version4pool.empty())
	{
		std::cout << "Failed to load object pool from window_masks.iop" << std::endl;
		return -4;
	}
	std::cout << "Loaded object pool from window_masks.iop" << std::endl;

	// Generate a unique version string for this object pool (this is optional, and is entirely application specific behavior)
	std::string objectPoolHash = "";

	const isobus::NAMEFilter filterVirtualTerminal(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	const std::vector<isobus::NAMEFilter> vtNameFilters = { filterVirtualTerminal };
	auto TestInternalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);
	auto TestPartnerVT = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

	virtualTerminalClient = std::make_shared<isobus::VirtualTerminalClient>(TestPartnerVT, TestInternalECU);
	virtualTerminalClient->get_vt_soft_key_event_dispatcher().add_listener(handle_softkey_event);
	virtualTerminalClient->get_vt_button_event_dispatcher().add_listener(handle_button_event);
	virtualTerminalClient->set_on_ready_for_object_pool_callback([&version3pool, &version4pool, objectPoolHash](isobus::VirtualTerminalClient::VTVersion version) {
		// You can check the connected VT version if you need to know what features are available, and select which object pool(s) to use based on that.
		// This is optional though. If you want, you can just call set_object_pool() blindly exactly one time at any point if you want to try to use the same object pool for all VT versions.
		switch (virtualTerminalClient->get_connected_vt_version())
		{
			case isobus::VirtualTerminalClient::VTVersion::Version3:
			{
				// For version 3, we upload a base pool with only VT version 3 complaint objects
				virtualTerminalClient->set_object_pool(0, version3pool.data(), version3pool.size(), objectPoolHash);
			}
			break;

			case isobus::VirtualTerminalClient::VTVersion::Version4:
			case isobus::VirtualTerminalClient::VTVersion::Version5:
			case isobus::VirtualTerminalClient::VTVersion::Version6:
			{
				// For version 4, 5, and 6, we upload the same base pool as version 3, but also upload a second pool with version 4 objects
				virtualTerminalClient->set_object_pool(0, version3pool.data(), version3pool.size(), objectPoolHash);
				virtualTerminalClient->set_object_pool(1, version4pool.data(), version4pool.size(), objectPoolHash);
			}
			break;

			default:
			{
				// Either we're not ready yet, or we don't have an object pool for this version
			}
			break;
		}
	});
	virtualTerminalClient->initialize(true);

	virtualTerminalUpdateHelper = std::make_shared<isobus::VirtualTerminalClientUpdateHelper>(virtualTerminalClient);
	virtualTerminalUpdateHelper->add_tracked_numeric_value(ButtonExampleNumber_VarNum, 214748364); // In the object pool the output number has an offset of -214748364 so we use this to represent 0.
	virtualTerminalUpdateHelper->initialize();

	while (running)
	{
		// CAN stack runs in other threads. Do nothing forever.
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	virtualTerminalClient->terminate();
	isobus::CANHardwareInterface::stop();
	return 0;
}
