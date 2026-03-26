#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"
#include "isobus/utility/iop_file_interface.hpp"

#include "../../common/console_logger.cpp"

#include <atomic>
#include <csignal>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

//! It is discouraged to use global variables, but it is done here for simplicity.
static std::shared_ptr<isobus::VirtualTerminalClient> virtualTerminalClient = nullptr;
static std::atomic_bool running = { true };
static std::uint16_t wsID = isobus::NULL_OBJECT_ID;
static std::uint32_t pagingIndex = 0;

struct MaskInfo
{
	std::uint16_t id;
	bool isAlarm;
};

std::vector<MaskInfo> availableMasks;

void signal_handler(int)
{
	running = false;
}

bool collect_masks(std::uint8_t *pool, std::uint32_t size)
{
	isobus::VirtualTerminalServerManagedWorkingSet vt;
	bool result = vt.parse_iop_into_objects(pool, size);

	if (!result)
		return false;

	for (const auto &obj : vt.get_object_tree())
	{
		using ObjType = isobus::VirtualTerminalObjectType;

		if (obj.second->get_object_type() == ObjType::DataMask || obj.second->get_object_type() == ObjType::AlarmMask)
		{
			availableMasks.push_back({ obj.first, obj.second->get_object_type() == ObjType::AlarmMask });
		}
		else if (obj.second->get_object_type() == ObjType::WorkingSet)
		{
			wsID = obj.first;
		}
	}
	return true;
}

void console_command_loop()
{
	std::cout << "Enter number to activate mask, use n or p to move to the next or prev mask, use 'q' to quit" << std::endl;
	while (running)
	{
		std::string input;
		std::getline(std::cin, input);

		if (input.empty())
			continue;

		std::uint16_t maskID = isobus::NULL_OBJECT_ID;
		if (input == "n")
		{
			if (pagingIndex < (availableMasks.size() - 1))
			{
				pagingIndex++;
			}
			else
			{
				pagingIndex = 0;
			}
			maskID = availableMasks.at(pagingIndex).id;
		}
		else if (input == "p")
		{
			if (pagingIndex == 0)
			{
				pagingIndex = availableMasks.size() - 1;
			}
			else
			{
				pagingIndex--;
			}
			maskID = availableMasks.at(pagingIndex).id;
		}
		else if (input == "q")
		{
			running = false;
			break;
		}
		else
		{
			try
			{
				maskID = static_cast<std::uint16_t>(std::stoi(input));
			}
			catch (const std::exception &)
			{
				std::cout << "Invalid input: " << input << std::endl;
				continue;
			}
			if (!std::any_of(
			      availableMasks.begin(),
			      availableMasks.end(),
			      [maskID](const MaskInfo &m) {
				      return m.id == maskID;
			      }))
			{
				std::cout << "The " << maskID << " is not an alarm or data mask ID" << std::endl;
				continue;
			}
		}

		if (virtualTerminalClient)
		{
			virtualTerminalClient->send_change_active_mask(wsID, maskID);
			std::cout << "Mask " << maskID << " selected" << std::endl;
		}
	}
}

int main(int argc, char **argv)
{
	std::signal(SIGINT, signal_handler);
	if (argc < 2)
	{
		std::cout << "Least one argument needs to be passed!" << std::endl;
		std::cout << "Usage: iop_loader <iop file name> [CAN interface name]" << std::endl;
		return -1;
	}

	std::string interfaceName = argc <= 2 ? "vcan0" : argv[2];
	std::shared_ptr<isobus::CANHardwarePlugin> canDriver = nullptr;
#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
	canDriver = std::make_shared<isobus::SocketCANInterface>(interfaceName);
#elif defined(ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE)
	int channel = interfaceName.empty() ? 0 : std::stoi(interfaceName);
	canDriver = std::make_shared<isobus::InnoMakerUSB2CANWindowsPlugin>(channel);
#elif (defined(ISOBUS_MACCANPCAN_AVAILABLE) || defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE))
	int channel = interfaceName.empty() ? PCAN_USBBUS1 : (std::stoi(interfaceName) - 1 + PCAN_USBBUS1);
#if defined(ISOBUS_MACCANPCAN_AVAILABLE)
	canDriver = std::make_shared<isobus::MacCANPCANPlugin>(channel);
#elif defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
	canDriver = std::make_shared<isobus::PCANBasicWindowsPlugin>(channel);
#endif
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

	std::vector<std::uint8_t> poolData = isobus::IOPFileInterface::read_iop_file(argv[1]);
	if (poolData.empty())
	{
		std::cout << "Failed to load object pool from " << argv[1] << std::endl;
		return -3;
	}
	std::cout << "Loaded object pool from " << argv[1] << std::endl;

	collect_masks(poolData.data(), poolData.size());

	std::cout << "Working set ID: " << wsID << std::endl;
	std::cout << "Available masks: " << std::endl;

	for (const auto &item : availableMasks)
	{
		std::cout << " - " << item.id << "(" << (item.isAlarm ? "alarm" : "data") << ")" << std::endl;
	}

	// Generate a unique version string for this object pool (this is optional, and is entirely application specific behavior)
	std::string objectPoolHash = isobus::IOPFileInterface::hash_object_pool_to_version(poolData);

	const isobus::NAMEFilter filterVirtualTerminal(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	const std::vector<isobus::NAMEFilter> vtNameFilters = { filterVirtualTerminal };
	auto TestInternalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);
	auto TestPartnerVT = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

	virtualTerminalClient = std::make_shared<isobus::VirtualTerminalClient>(TestPartnerVT, TestInternalECU);
	virtualTerminalClient->set_object_pool(0, poolData.data(), poolData.size(), objectPoolHash);
	virtualTerminalClient->initialize(true);
	std::thread consoleThread(console_command_loop);
	if (consoleThread.joinable())
		consoleThread.join();

	virtualTerminalClient->terminate();
	isobus::CANHardwareInterface::stop();
	return 0;
}
