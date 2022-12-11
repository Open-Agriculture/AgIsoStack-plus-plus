#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/socket_can_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/nmea2000_fast_packet_protocol.hpp"

#include <csignal>
#include <iostream>
#include <iterator>
#include <memory>

static std::shared_ptr<isobus::InternalControlFunction> TestInternalECU = nullptr;
static SocketCANInterface canDriver("vcan0");

using namespace std;

void signal_handler(int signum)
{
	CANHardwareInterface::stop();
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

void setup()
{
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
	TestDeviceNAME.set_industry_group(0);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);

	TestInternalECU = std::make_shared<isobus::InternalControlFunction>(TestDeviceNAME, 0x1C, 0);

	std::this_thread::sleep_for(std::chrono::milliseconds(250)); // Wait to make sure our address was claimed

	std::signal(SIGINT, signal_handler);
}

int main()
{
	constexpr std::uint8_t TEST_MESSAGE_LENGTH = 100;
	std::uint8_t testMessageData[TEST_MESSAGE_LENGTH];

	// Initialize some test data
	for (std::uint8_t i = 0; i < TEST_MESSAGE_LENGTH; i++)
	{
		testMessageData[i] = i;
	}

	setup();

	while (true)
	{
		// Send a fast packet message
		isobus::FastPacketProtocol::Protocol.send_multipacket_message(0x1F001, testMessageData, TEST_MESSAGE_LENGTH, TestInternalECU.get(), nullptr);

		// Sleep for a while
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	}

	CANHardwareInterface::stop();
	return 0;
}