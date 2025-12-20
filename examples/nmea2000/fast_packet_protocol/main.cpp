#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/socket_can_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/nmea2000_fast_packet_protocol.hpp"

#include <atomic>
#include <csignal>
#include <iostream>
#include <iterator>
#include <memory>
#include <thread>

static std::atomic_bool running = { true };

void nmea2k_callback(const isobus::CANMessage &message, void *)
{
	std::cout << "Received a NMEA2K fast packet PGN " << message.get_identifier().get_parameter_group_number() << " message with length " << message.get_data_length() << std::endl;
}

void nmea2k_transmit_complete_callback(std::uint32_t parameterGroupNumber,
                                       std::uint32_t dataLength,
                                       std::shared_ptr<isobus::InternalControlFunction>,
                                       std::shared_ptr<isobus::ControlFunction>,
                                       bool successful,
                                       void *)
{
	if (successful)
	{
		std::cout << "Successfully sent a NMEA2K Fast Packet PGN " << parameterGroupNumber << " message with length " << dataLength << std::endl;
	}
	else
	{
		std::cout << "Failed sending a NMEA2K Fast Packet PGN " << parameterGroupNumber << " message with length " << dataLength << std::endl;
	}
}

void signal_handler(int)
{
	running = false;
}

int main()
{
	std::signal(SIGINT, signal_handler);

	std::shared_ptr<isobus::CANHardwarePlugin> canDriver = nullptr;
#if defined(ISOBUS_SOCKETCAN_AVAILABLE)
	canDriver = std::make_shared<isobus::SocketCANInterface>("vcan0");
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

	isobus::CANHardwareInterface::set_number_of_can_channels(1);
	isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canDriver);

	if ((!isobus::CANHardwareInterface::start()) || (!canDriver->get_is_valid()))
	{
		std::cout << "Failed to start hardware interface. A CAN driver might be invalid." << std::endl;
		return -2;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	isobus::NAME TestDeviceNAME(0);

	//! Make sure you change these for your device!!!!
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(0);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(1407);

	auto TestInternalECU = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);

	isobus::CANNetworkManager::CANNetwork.get_fast_packet_protocol(0)->register_multipacket_message_callback(0x1F001, nmea2k_callback, nullptr);

	// Wait to make sure address claiming is done. The time is arbitrary.
	//! @todo Check this instead of assuming it is done
	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	constexpr std::uint8_t TEST_MESSAGE_LENGTH = 100;
	std::uint8_t testMessageData[TEST_MESSAGE_LENGTH];

	// Initialize some test data
	for (std::uint8_t i = 0; i < TEST_MESSAGE_LENGTH; i++)
	{
		testMessageData[i] = i;
	}

	while (running)
	{
		// Send a fast packet message
		isobus::CANNetworkManager::CANNetwork.get_fast_packet_protocol(0)->send_multipacket_message(0x1F001, testMessageData, TEST_MESSAGE_LENGTH, TestInternalECU, nullptr, isobus::CANIdentifier::CANPriority::PriorityLowest7, nmea2k_transmit_complete_callback);

		// Sleep for a while
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	}

	isobus::CANHardwareInterface::stop();
	isobus::CANNetworkManager::CANNetwork.get_fast_packet_protocol(0)->remove_multipacket_message_callback(0x1F001, nmea2k_callback, nullptr);
	return 0;
}
