#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/utility/iop_file_interface.hpp"
#include "objectPoolObjects.h"

#ifdef WIN32
#include "isobus/hardware_integration/pcan_basic_windows_plugin.hpp"
static PCANBasicWindowsPlugin canDriver(PCAN_USBBUS1);
#else
#include "isobus/hardware_integration/socket_can_interface.hpp"
static SocketCANInterface canDriver("can0");
#endif

#include <csignal>
#include <iostream>
#include <memory>

static std::shared_ptr<isobus::InternalControlFunction> TestInternalECU = nullptr;
static std::shared_ptr<isobus::PartneredControlFunction> TestPartnerVT = nullptr;
static std::shared_ptr<isobus::VirtualTerminalClient> TestVirtualTerminalClient = nullptr;
std::vector<isobus::NAMEFilter> vtNameFilters;
const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
static std::vector<std::uint8_t> testPool;

using namespace std;

// A log sink for the CAN stack
class CustomLogger : public isobus::CANStackLogger
{
public:
	void sink_CAN_stack_log(CANStackLogger::LoggingLevel level, const std::string &text) override
	{
		switch (level)
		{
			case LoggingLevel::Debug:
			{
				std::cout << "["
				          << "\033[1;36m"
				          << "Debug"
				          << "\033[0m"
				          << "]";
			}
			break;

			case LoggingLevel::Info:
			{
				std::cout << "["
				          << "\033[1;32m"
				          << "Info"
				          << "\033[0m"
				          << "]";
			}
			break;

			case LoggingLevel::Warning:
			{
				std::cout << "["
				          << "\033[1;33m"
				          << "Warn"
				          << "\033[0m"
				          << "]";
			}
			break;

			case LoggingLevel::Error:
			{
				std::cout << "["
				          << "\033[1;31m"
				          << "Error"
				          << "\033[0m"
				          << "]";
			}
			break;

			case LoggingLevel::Critical:
			{
				std::cout << "["
				          << "\033[1;35m"
				          << "Debug"
				          << "\033[0m"
				          << "]";
			}
			break;
		}
		std::cout << text << std::endl; // Write the text to stdout
	}
};

static CustomLogger logger;

void signal_handler(int signum)
{
	CANHardwareInterface::stop();
	if (nullptr != TestVirtualTerminalClient)
	{
		TestVirtualTerminalClient->terminate();
	}
	exit(signum);
}

void update_CAN_network()
{
	isobus::CANNetworkManager::CANNetwork.update();
}

void raw_can_glue(isobus::HardwareInterfaceCANFrame &rawFrame, void *parentPointer)
{
	isobus::CANNetworkManager::CANNetwork.can_lib_process_rx_message(rawFrame, parentPointer);
}

// This callback will provide us with event driven notifications of button presses from the stack
void handleVTButton(isobus::VirtualTerminalClient::KeyActivationCode keyEvent, std::uint8_t, std::uint16_t objectID, std::uint16_t, isobus::VirtualTerminalClient *)
{
	static std::uint32_t exampleNumberOutput = 214748364; // In the object pool the output number has an offset of -214748364 so we use this to represent 0.

	switch (keyEvent)
	{
		case isobus::VirtualTerminalClient::KeyActivationCode::ButtonUnlatchedOrReleased:
		{
			switch (objectID)
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
			}
		}
		break;

		default:
		{
		}
		break;
	}
}

void setup()
{
	isobus::CANStackLogger::set_can_stack_logger_sink(&logger);
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, &canDriver);

	if ((!CANHardwareInterface::start()) || (!canDriver.get_is_valid()))
	{
		std::cout << "Failed to connect to the socket. The interface might be down." << std::endl;
	}

	CANHardwareInterface::add_can_lib_update_callback(update_CAN_network, nullptr);
	CANHardwareInterface::add_raw_can_message_rx_callback(raw_can_glue, nullptr);

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	isobus::NAME TestDeviceNAME(0);

	// Make sure you change these for your device!!!!
	// This is an example device that is using a manufacturer code that is currently unused at time of writing
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(1);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);
	vtNameFilters.push_back(testFilter);

	testPool = isobus::IOPFileInterface::read_iop_file("VT3TestPool.iop");

	if (0 != testPool.size())
	{
		std::cout << "Loaded object pool from VT3TestPool.iop" << std::endl;
	}
	else
	{
		std::cout << "Failed to load object pool from VT3TestPool.iop" << std::endl;
	}

	// Generate a unique version string for this object pool (this is optional, and is entirely application specific behavior)
	std::string objectPoolHash = isobus::IOPFileInterface::hash_object_pool_to_version(testPool);

	TestInternalECU = std::make_shared<isobus::InternalControlFunction>(TestDeviceNAME, 0x1C, 0);
	TestPartnerVT = std::make_shared<isobus ::PartneredControlFunction>(0, vtNameFilters);
	TestVirtualTerminalClient = std::make_shared<isobus::VirtualTerminalClient>(TestPartnerVT, TestInternalECU);
	TestVirtualTerminalClient->set_object_pool(0, isobus::VirtualTerminalClient::VTVersion::Version3, testPool.data(), testPool.size(), objectPoolHash);
	TestVirtualTerminalClient->register_vt_button_event_callback(handleVTButton);
	TestVirtualTerminalClient->register_vt_soft_key_event_callback(handleVTButton);
	TestVirtualTerminalClient->initialize(true);
	std::signal(SIGINT, signal_handler);
}

int main()
{
	setup();

	while (true)
	{
		// CAN stack runs in other threads. Do nothing forever.
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	CANHardwareInterface::stop();
	return 0;
}
