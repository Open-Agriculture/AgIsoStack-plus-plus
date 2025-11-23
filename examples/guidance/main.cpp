#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_guidance_interface.hpp"

#include "../common/console_logger.cpp"

#include <atomic>
#include <csignal>
#include <functional>
#include <iostream>
#include <memory>

//! It is discouraged to use global variables, but it is done here for simplicity.
static std::atomic_bool running = { true };
static bool is_first_machine_info_message = true;
static bool is_first_system_command_message = true;

void signal_handler(int)
{
	running = false;
}

void on_guidance_machine_info_message(const std::shared_ptr<isobus::AgriculturalGuidanceInterface::GuidanceMachineInfo> info, bool changed)
{
	//! @note changed is true when the info has changed since the last time,
	//!       this means that your initial message callback might not be flagged as changed.
	if (changed || is_first_machine_info_message)
	{
		is_first_machine_info_message = false;
		std::cout << "Agriculture Guidance Machine Info: " << std::endl;
		std::cout << "  Estimated curvature: " << info->get_estimated_curvature() << std::endl;
		std::cout << "  Limit status: " << static_cast<int>(info->get_guidance_limit_status()) << std::endl;
		std::cout << "  Steering-input position status: " << static_cast<int>(info->get_guidance_steering_input_position_status()) << std::endl;
		std::cout << "  Steering-system readiness state: " << static_cast<int>(info->get_guidance_steering_system_readiness_state()) << std::endl;
		std::cout << "  Steering-system command exit reason code: " << static_cast<int>(info->get_guidance_system_command_exit_reason_code()) << std::endl;
		std::cout << "  Steering-system remote engage switch status: " << static_cast<int>(info->get_guidance_system_remote_engage_switch_status()) << std::endl;
		std::cout << "  Mechanical system lockout: " << static_cast<int>(info->get_mechanical_system_lockout()) << std::endl;
		std::cout << "  Request reset command status: " << static_cast<int>(info->get_request_reset_command_status()) << std::endl;
	}
}

void on_guidance_system_command_message(const std::shared_ptr<isobus::AgriculturalGuidanceInterface::GuidanceSystemCommand> status, bool changed)
{
	//! @note changed is true when the info has changed since the last time,
	//!       this means that your initial message callback might not be flagged as changed.
	if (changed || is_first_system_command_message)
	{
		is_first_system_command_message = false;
		std::cout << "Agriculture Guidance System Command: " << std::endl;
		std::cout << "  Curvature: " << status->get_curvature() << std::endl;
		std::cout << "  Status: " << static_cast<int>(status->get_status()) << std::endl;
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
	TestDeviceNAME.set_identity_number(3);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(1407);

	isobus::AgriculturalGuidanceInterface TestGuidanceInterface(nullptr, nullptr);

	// Register listeners for the (guidance) events we want to receive
	TestGuidanceInterface.get_guidance_machine_info_event_publisher().add_listener(on_guidance_machine_info_message);
	TestGuidanceInterface.get_guidance_system_command_event_publisher().add_listener(on_guidance_system_command_message);

	// Finally we can initialize the guidance interface to start sending and receiving messages
	TestGuidanceInterface.initialize();

	while (running)
	{
		TestGuidanceInterface.update();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	isobus::CANHardwareInterface::stop();
	return 0;
}
