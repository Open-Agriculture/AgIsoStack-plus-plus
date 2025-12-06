#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_preferred_addresses.hpp"
#include "isobus/isobus/isobus_task_controller_server.hpp"

#include "../common/console_logger.cpp"

#include <atomic>
#include <csignal>

//! It is discouraged to use global variables, but it is done here for simplicity.
static std::atomic_bool running = { true };

using namespace std;

void signal_handler(int)
{
	running = false;
}

// Define a very basic TC server.
// You can use this as a starting point for your own TC server!
// You'll have to implement the functions here to make it "do" something.
class MyTCServer : public isobus::TaskControllerServer
{
public:
	MyTCServer(std::shared_ptr<isobus::InternalControlFunction> internalControlFunction,
	           std::uint8_t numberBoomsSupported,
	           std::uint8_t numberSectionsSupported,
	           std::uint8_t numberChannelsSupportedForPositionBasedControl,
	           const isobus::TaskControllerOptions &options) :
	  TaskControllerServer(internalControlFunction,
	                       numberBoomsSupported,
	                       numberSectionsSupported,
	                       numberChannelsSupportedForPositionBasedControl,
	                       options)
	{
	}

	bool activate_object_pool(std::shared_ptr<isobus::ControlFunction>, ObjectPoolActivationError &, ObjectPoolErrorCodes &, std::uint16_t &, std::uint16_t &) override
	{
		return true;
	}

	bool change_designator(std::shared_ptr<isobus::ControlFunction>, std::uint16_t, const std::vector<std::uint8_t> &) override
	{
		return true;
	}

	bool deactivate_object_pool(std::shared_ptr<isobus::ControlFunction>) override
	{
		return true;
	}

	bool delete_device_descriptor_object_pool(std::shared_ptr<isobus::ControlFunction>, ObjectPoolDeletionErrors &) override
	{
		return true;
	}

	bool get_is_stored_device_descriptor_object_pool_by_structure_label(std::shared_ptr<isobus::ControlFunction>, const std::vector<std::uint8_t> &, const std::vector<std::uint8_t> &) override
	{
		return false;
	}

	bool get_is_stored_device_descriptor_object_pool_by_localization_label(std::shared_ptr<isobus::ControlFunction>, const std::array<std::uint8_t, 7> &) override
	{
		return false;
	}

	bool get_is_enough_memory_available(std::uint32_t) override
	{
		return true;
	}

	void identify_task_controller(std::uint8_t) override
	{
		// When this is called, the TC is supposed to display its TC number for 3 seconds if possible (which is passed into this function).
		// Your TC's number is your function code + 1, in the range of 1-32.
	}

	void on_client_timeout(std::shared_ptr<isobus::ControlFunction>) override
	{
		// You can use this function to handle when a client times out (6 Seconds)
	}

	void on_process_data_acknowledge(std::shared_ptr<isobus::ControlFunction>, std::uint16_t, std::uint16_t, std::uint8_t, ProcessDataCommands) override
	{
		// This callback lets you know when a client sends a process data acknowledge (PDACK) message to you
	}

	bool on_value_command(std::shared_ptr<isobus::ControlFunction>, std::uint16_t, std::uint16_t, std::int32_t, std::uint8_t &) override
	{
		return true;
	}

	bool store_device_descriptor_object_pool(std::shared_ptr<isobus::ControlFunction>, const std::vector<std::uint8_t> &, bool) override
	{
		return true;
	}
};

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
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::TaskController));
	TestDeviceNAME.set_identity_number(20);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0); // TC #1. If you want to change the TC number, change this.
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(1407);

	auto TestInternalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0, isobus::preferred_addresses::IndustryGroup2::TaskController_MappingComputer);
	MyTCServer server(TestInternalECU,
	                  4, // Booms
	                  255, // Sections
	                  16, // Channels
	                  isobus::TaskControllerOptions()
	                    .with_documentation()
	                    .with_implement_section_control()
	                    .with_tc_geo_with_position_based_control());
	auto &languageInterface = server.get_language_command_interface();
	languageInterface.set_language_code("en"); // This is the default, but you can change it if you want
	languageInterface.set_country_code("US"); // This is the default, but you can change it if you want
	server.initialize();

	while (running)
	{
		server.update();

		// Update again in a little bit
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	server.terminate();
	isobus::CANHardwareInterface::stop();
	return 0;
}
