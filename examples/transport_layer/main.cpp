#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/socket_can_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_configuration.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_transport_protocol.hpp"

#include <csignal>
#include <iostream>
#include <iterator>
#include <memory>

static constexpr std::uint16_t MAX_TP_SIZE_BYTES = 1785;
static constexpr std::uint32_t ETP_TEST_SIZE = 2048;

using namespace std;

void signal_handler(int)
{
	CANHardwareInterface::stop();
	_exit(EXIT_FAILURE);
}

void update_CAN_network()
{
	isobus::CANNetworkManager::CANNetwork.update();
}

void raw_can_glue(isobus::HardwareInterfaceCANFrame &rawFrame, void *parentPointer)
{
	isobus::CANNetworkManager::CANNetwork.can_lib_process_rx_message(rawFrame, parentPointer);
}

int main()
{
	std::signal(SIGINT, signal_handler);

	std::shared_ptr<CANHardwarePlugin> canDriver = nullptr;
#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
	canDriver = std::make_shared<SocketCANInterface>("can0");
#elif defined(ISOBUS_WINDOWSPCANBASIC_AVAILABLE)
	canDriver = std::make_shared<PCANBasicWindowsPlugin>(PCAN_USBBUS1);
#elif defined(ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE)
	canDriver = std::make_shared<InnoMakerUSB2CANWindowsPlugin>(0); // CAN0
#endif
	if (nullptr == canDriver)
	{
		std::cout << "Unable to find a CAN driver. Please make sure you have one of the above drivers installed with the library." << std::endl;
		std::cout << "If you want to use a different driver, please add it to the list above." << std::endl;
		return -1;
	}

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

	if ((!CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
	{
		std::cout << "Failed to start hardware interface. The CAN driver might be invalid." << std::endl;
		return -2;
	}

	CANHardwareInterface::add_can_lib_update_callback(update_CAN_network, nullptr);
	CANHardwareInterface::add_raw_can_message_rx_callback(raw_can_glue, nullptr);

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

	const isobus::NAMEFilter filterVirtualTerminal(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));

	isobus::InternalControlFunction TestInternalECU(TestDeviceNAME, 0x1C, 0);
	isobus::PartneredControlFunction TestPartner(0, { filterVirtualTerminal });

	// Wait to make sure address claiming is done. The time is arbitrary.
	//! @todo Check this instead of asuming it is done
	std::this_thread::sleep_for(std::chrono::milliseconds(1250));

	// Set up some test CAN messages
	std::uint8_t TPTestBuffer[MAX_TP_SIZE_BYTES];
	std::uint8_t ETPTestBuffer[ETP_TEST_SIZE];

	for (uint16_t i = 0; i < MAX_TP_SIZE_BYTES; i++)
	{
		TPTestBuffer[i] = (i % 0xFF); // Fill buffer with junk data
	}
	for (uint32_t i = 0; i < ETP_TEST_SIZE; i++)
	{
		ETPTestBuffer[i] = (i % 0xFF); // Fill buffer with junk data
	}

	// Send a classic CAN message to a specific destination(8 bytes or less)
	if (isobus::CANNetworkManager::CANNetwork.send_can_message(0xEF00, ETPTestBuffer, isobus::CAN_DATA_LENGTH, &TestInternalECU, &TestPartner))
	{
		cout << "Sent a normal CAN Message with length 8" << endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(4)); // Arbitrary
	}

	// Send a classic CAN message to global (0xFF) (8 bytes or less)
	if (isobus::CANNetworkManager::CANNetwork.send_can_message(0xEF00, ETPTestBuffer, isobus::CAN_DATA_LENGTH, &TestInternalECU))
	{
		cout << "Sent a broadcast CAN Message with length 8" << endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(4)); // Arbitrary
	}

	// CM Tx Example
	// This loop sends all possible TP CM message sizes.
	// This will take a long time
	for (std::uint32_t i = 9; i <= MAX_TP_SIZE_BYTES; i++)
	{
		// Send message
		if (isobus::CANNetworkManager::CANNetwork.send_can_message(0xEF00, TPTestBuffer, i, &TestInternalECU, &TestPartner))
		{
			cout << "Started TP CM Session with length " << i << endl;
		}
		else
		{
			cout << "Failed starting TP CM Session with length " << i << endl;
		}
		// Wait for this session to complete before starting the next
		// This sleep value is arbitrary
		std::this_thread::sleep_for(std::chrono::milliseconds(i * 2));
	}

	// BAM Tx Exmaple
	// This loop sends all possible BAM message sizes
	// This will take a very long time
	for (std::uint32_t i = 9; i <= MAX_TP_SIZE_BYTES; i++)
	{
		// Send message
		if (isobus::CANNetworkManager::CANNetwork.send_can_message(0xEF00, TPTestBuffer, i, &TestInternalECU))
		{
			cout << "Started BAM Session with length " << i << endl;
		}
		else
		{
			cout << "Failed starting BAM Session with length " << i << endl;
		}
		// Wait for this session to complete before starting the next, or it will fail as only 1 BAM session is possible at a time
		std::this_thread::sleep_for(std::chrono::milliseconds(2 * (isobus::CANNetworkConfiguration::get_minimum_time_between_transport_protocol_bam_frames() * ((i + 1) / 7))));
	}

	// ETP Example
	// Send one ETP message
	if (isobus::CANNetworkManager::CANNetwork.send_can_message(0xEF00, ETPTestBuffer, ETP_TEST_SIZE, &TestInternalECU, &TestPartner))
	{
		cout << "Started ETP Session with length " << ETP_TEST_SIZE << endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	}

	CANHardwareInterface::stop();

	return 0;
}
