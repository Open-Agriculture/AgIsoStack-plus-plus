#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"

#include <atomic>
#include <csignal>
#include <future>
#include <iostream>
#include <memory>
#include <thread>

static constexpr std::uint32_t PARAMETER_GROUP_NUMBER = 0xEF00; ///< The parameter group number we will use for testing
static constexpr std::uint16_t MAX_TP_MESSAGE_SIZE_BYTES = 1785; ///< The max number of bytes the Transport Protocol can handle
static constexpr std::uint32_t MAX_ETP_MESSAGE_SIZE_BYTES = 117440505; ///< The max number of bytes the Extended Transport Protocol can handle
static constexpr std::uint32_t MAX_MESSAGE_SIZE_BYTES = 1000000; ///< The max number of bytes we will test sending
static std::atomic_bool running = { true };

void signal_handler(int)
{
	running = false;
}

using namespace isobus;

// Forward declarations
void check_can_message(const CANMessage &message, void *);
void print_progress_bar(const std::shared_ptr<TransportProtocolSessionBase> session);

// The example sends a series of CAN messages of incrementing length to itself and checks that the data is correct
int main()
{
	std::signal(SIGINT, signal_handler);

#ifndef ISOBUS_VIRTUALCAN_AVAILABLE
	std::cout << "This example requires the VirtualCAN plugin to be available. If using CMake, set the `-DCAN_DRIVER=VirtualCAN`." << std::endl;
	return -1;
#else

	std::shared_ptr<CANHardwarePlugin> originatorDriver = std::make_shared<VirtualCANPlugin>("test-channel");
	std::shared_ptr<CANHardwarePlugin> recipientDriver = std::make_shared<VirtualCANPlugin>("test-channel");

	CANHardwareInterface::set_number_of_can_channels(2);
	CANHardwareInterface::assign_can_channel_frame_handler(0, originatorDriver);
	CANHardwareInterface::assign_can_channel_frame_handler(1, recipientDriver);

	if ((!CANHardwareInterface::start()) || (!originatorDriver->get_is_valid()) || (!recipientDriver->get_is_valid()))
	{
		std::cout << "Failed to start hardware interface. The CAN driver might be invalid." << std::endl;
		return -2;
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	NAME originatorNAME(0);
	originatorNAME.set_arbitrary_address_capable(true);
	originatorNAME.set_industry_group(1);
	originatorNAME.set_device_class(0);
	originatorNAME.set_function_code(static_cast<std::uint8_t>(NAME::Function::SteeringControl));
	originatorNAME.set_identity_number(2);
	originatorNAME.set_ecu_instance(0);
	originatorNAME.set_function_instance(0);
	originatorNAME.set_device_class_instance(0);
	originatorNAME.set_manufacturer_code(1407);

	NAME recipientNAME(0);
	recipientNAME.set_arbitrary_address_capable(true);
	recipientNAME.set_industry_group(1);
	recipientNAME.set_device_class(0);
	recipientNAME.set_function_code(static_cast<std::uint8_t>(NAME::Function::VirtualTerminal));
	recipientNAME.set_identity_number(1);
	recipientNAME.set_ecu_instance(0);
	recipientNAME.set_function_instance(0);
	recipientNAME.set_device_class_instance(0);
	recipientNAME.set_manufacturer_code(1407);

	const NAMEFilter filterOriginator(NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(NAME::Function::SteeringControl));
	const NAMEFilter filterRecipient(NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(NAME::Function::VirtualTerminal));

	auto originatorECU = CANNetworkManager::CANNetwork.create_internal_control_function(originatorNAME, 0, 0x1C);
	auto originatorPartner = CANNetworkManager::CANNetwork.create_partnered_control_function(1, { filterOriginator });
	auto recipientECU = CANNetworkManager::CANNetwork.create_internal_control_function(recipientNAME, 1, 0x1D);
	auto recipientPartner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, { filterRecipient });

	// We want to make sure address claiming is successful before continuing
	auto addressClaimedFuture = std::async(std::launch::async, [&originatorECU, &recipientECU, &originatorPartner, &recipientPartner]() {
		while ((!originatorECU->get_address_valid()) || (!recipientECU->get_address_valid()) ||
		       (!originatorPartner->get_address_valid()) || (!recipientPartner->get_address_valid()))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	});
	if (addressClaimedFuture.wait_for(std::chrono::seconds(5)) == std::future_status::timeout)
	{
		std::cout << "Address claiming failed. Please make sure that your internal control function can claim a valid address." << std::endl;
		return -3;
	}

	// Set up a message buffer for testing
	std::vector<std::uint8_t> sendBuffer;
	sendBuffer.reserve(MAX_MESSAGE_SIZE_BYTES);

	for (uint32_t i = 0; i < MAX_MESSAGE_SIZE_BYTES; i++)
	{
		sendBuffer.push_back(i % 0xFF); // Fill buffer with incrementing values
	}

	// Register a callback for receiving CAN messages
	CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(PARAMETER_GROUP_NUMBER, check_can_message, nullptr);
	originatorPartner->add_parameter_group_number_callback(PARAMETER_GROUP_NUMBER, check_can_message, nullptr, recipientECU);

	// Send a classic CAN message to a specific destination(8 bytes or less)
	if (running && CANNetworkManager::CANNetwork.send_can_message(PARAMETER_GROUP_NUMBER, sendBuffer.data(), CAN_DATA_LENGTH, originatorECU, recipientPartner))
	{
		std::cout << "Sent a normal CAN Message with length 8" << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(4)); // Arbitrary
	}

	// Send a classic CAN message to global (0xFF) (8 bytes or less)
	if (running && CANNetworkManager::CANNetwork.send_can_message(PARAMETER_GROUP_NUMBER, sendBuffer.data(), CAN_DATA_LENGTH, originatorECU))
	{
		std::cout << "Sent a broadcast CAN Message with length 8" << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(4)); // Arbitrary
	}

	// Send (Extended) Transport Protocol destination-destination specific messages of exponentially increasing size
	// This will take a while to complete
	std::uint32_t message_length = 9; // Arbitrary starting point
	std::shared_ptr<TransportProtocolSessionBase> session = nullptr;
	while (running && (message_length <= MAX_MESSAGE_SIZE_BYTES) && (message_length <= MAX_ETP_MESSAGE_SIZE_BYTES))
	{
		if (session == nullptr)
		{
			if (CANNetworkManager::CANNetwork.send_can_message(PARAMETER_GROUP_NUMBER, sendBuffer.data(), message_length, originatorECU, recipientPartner))
			{
				std::cout << "Sending a Transport Protocol Message with length " << message_length << std::endl;
				message_length *= 2;
				session = CANNetworkManager::CANNetwork.get_active_transport_protocol_sessions(0).front();
			}
		}
		else
		{
			print_progress_bar(session);
			if (session.use_count() == 1) // We are the only ones holding a reference to the session, so it must be done/failed
			{
				std::cout << std::endl; // End the progress bar
				session = nullptr;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(4));
	}

	// Send Broadcast Transport Protocol messages (BAM) of exponentially increasing size
	// This will take a while to complete
	message_length = 11; // Arbitrary starting point
	while (running && (message_length <= MAX_MESSAGE_SIZE_BYTES) && (message_length <= MAX_TP_MESSAGE_SIZE_BYTES))
	{
		if (session == nullptr)
		{
			if (CANNetworkManager::CANNetwork.send_can_message(PARAMETER_GROUP_NUMBER, sendBuffer.data(), message_length, originatorECU))
			{
				std::cout << "Sending a Broadcast Transport Protocol Message with length " << message_length << std::endl;
				message_length *= 2;
				session = CANNetworkManager::CANNetwork.get_active_transport_protocol_sessions(0).front();
			}
		}
		else
		{
			print_progress_bar(session);
			if (session.use_count() == 1) // We are the only ones holding a reference to the session, so it must be done/failed
			{
				std::cout << std::endl; // End the progress bar
				session = nullptr;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(4));
	}
#endif
	CANHardwareInterface::stop();
	return 0;
}

void check_can_message(const CANMessage &message, void *)
{
	for (std::uint32_t i = 0; i < message.get_data_length(); i++)
	{
		if (message.get_data()[i] != (i % 0xFF))
		{
			std::cerr << std::endl // End the progress bar
			          << "Received CAN with incorrect data!!!" << std::endl;
			return;
		}
	}
}

void print_progress_bar(const std::shared_ptr<TransportProtocolSessionBase> session)
{
	constexpr std::uint8_t width = 50;
	float percentage = session->get_percentage_bytes_transferred();

	std::cout << "[";
	for (std::uint8_t i = 0; i < width; i++)
	{
		if (i < static_cast<std::uint8_t>(percentage / 100 * width))
		{
			std::cout << "=";
		}
		else if (i == static_cast<std::uint8_t>(percentage / 100 * width))
		{
			std::cout << ">";
		}
		else
		{
			std::cout << " ";
		}
	}
	std::cout << "] " << static_cast<int>(percentage) << "% (" << session->get_total_bytes_transferred() << "/" << session->get_message_length() << " bytes)\r";
	std::cout.flush();
}
