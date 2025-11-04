#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_standard_data_description_indices.hpp"
#include "isobus/isobus/isobus_task_controller_client.hpp"

#include "../common/console_logger.cpp"
#include "section_control_implement_sim.hpp"

#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>

//! It is discouraged to use global variables, but it is done here for simplicity.
static std::shared_ptr<isobus::TaskControllerClient> TestTCClient = nullptr;
static std::atomic_bool running = { true };

using namespace std;

void signal_handler(int)
{
	running = false;
}

int main()
{
	std::signal(SIGINT, signal_handler);

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
	isobus::CANStackLogger::set_log_level(isobus::CANStackLogger::LoggingLevel::Debug); // Change this to Debug to see more information
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
	TestDeviceNAME.set_industry_group(2);
	TestDeviceNAME.set_device_class(6);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::RateControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(1407);

	const isobus::NAMEFilter filterTaskController(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::TaskController));
	const isobus::NAMEFilter filterTaskControllerInstance(isobus::NAME::NAMEParameters::FunctionInstance, 0);
	const isobus::NAMEFilter filterTaskControllerIndustryGroup(isobus::NAME::NAMEParameters::IndustryGroup, static_cast<std::uint8_t>(isobus::NAME::IndustryGroup::AgriculturalAndForestryEquipment));
	const isobus::NAMEFilter filterTaskControllerDeviceClass(isobus::NAME::NAMEParameters::DeviceClass, static_cast<std::uint8_t>(isobus::NAME::DeviceClass::NonSpecific));
	const std::vector<isobus::NAMEFilter> tcNameFilters = { filterTaskController,
		                                                      filterTaskControllerInstance,
		                                                      filterTaskControllerIndustryGroup,
		                                                      filterTaskControllerDeviceClass };
	auto TestInternalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);
	auto TestPartnerTC = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, tcNameFilters);

	TestTCClient = std::make_shared<isobus::TaskControllerClient>(TestPartnerTC, TestInternalECU, nullptr);

	// Set up some TC specific variables
	auto myDDOP = std::make_shared<isobus::DeviceDescriptorObjectPool>();
	bool tcClientStarted = false;

	constexpr std::uint16_t NUMBER_OF_SECTIONS_TO_CREATE = 16;
	SectionControlImplementSimulator rateController;
	rateController.set_number_of_sections(NUMBER_OF_SECTIONS_TO_CREATE);

	while (running)
	{
		if (!tcClientStarted)
		{
			if (rateController.create_ddop(myDDOP, TestInternalECU->get_NAME()))
			{
				TestTCClient->configure(myDDOP, 1, NUMBER_OF_SECTIONS_TO_CREATE, 1, true, false, true, false, true);
				TestTCClient->add_request_value_callback(SectionControlImplementSimulator::request_value_command_callback, &rateController);
				TestTCClient->add_value_command_callback(SectionControlImplementSimulator::value_command_callback, &rateController);
				TestTCClient->initialize(true);
				tcClientStarted = true;
			}
			else
			{
				std::cout << "Failed to create DDOP" << std::endl;
				break;
			}
		}

		// The CAN stack runs in other threads. Not much to do here.
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	TestTCClient->terminate();
	isobus::CANHardwareInterface::stop();
	return (!tcClientStarted);
}
