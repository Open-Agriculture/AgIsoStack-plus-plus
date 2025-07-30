#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_NAME_filter.hpp"
#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"

#include <chrono>
#include <thread>

using namespace isobus;

TEST(ADDRESS_CLAIM_TESTS, PartneredClaim)
{
	auto firstDevice = std::make_shared<VirtualCANPlugin>();
	auto secondDevice = std::make_shared<VirtualCANPlugin>();
	CANHardwareInterface::set_number_of_can_channels(2);
	CANHardwareInterface::assign_can_channel_frame_handler(0, firstDevice);
	CANHardwareInterface::assign_can_channel_frame_handler(1, secondDevice);
	CANHardwareInterface::start();

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	NAME firstName(0);
	firstName.set_arbitrary_address_capable(true);
	firstName.set_industry_group(1);
	firstName.set_device_class(0);
	firstName.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::CabClimateControl));
	firstName.set_identity_number(1);
	firstName.set_ecu_instance(0);
	firstName.set_function_instance(0);
	firstName.set_device_class_instance(0);
	firstName.set_manufacturer_code(69);
	auto firstInternalECU = CANNetworkManager::CANNetwork.create_internal_control_function(firstName, 0, 0x1C);

	isobus::NAME secondName(0);
	secondName.set_arbitrary_address_capable(true);
	secondName.set_industry_group(1);
	secondName.set_device_class(0);
	secondName.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SeatControl));
	secondName.set_identity_number(2);
	secondName.set_ecu_instance(0);
	secondName.set_function_instance(0);
	secondName.set_device_class_instance(0);
	secondName.set_manufacturer_code(69);
	auto secondInternalECU2 = CANNetworkManager::CANNetwork.create_internal_control_function(secondName, 1);

	const NAMEFilter filterSecond(NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(NAME::Function::SeatControl));
	auto firstPartneredSecondECU = CANNetworkManager::CANNetwork.create_partnered_control_function(0, { filterSecond });
	const isobus::NAMEFilter filterFirst(NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(NAME::Function::CabClimateControl));
	auto secondPartneredFirstEcu = CANNetworkManager::CANNetwork.create_partnered_control_function(1, { filterFirst });

	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	EXPECT_TRUE(firstInternalECU->get_address_valid());
	EXPECT_TRUE(secondInternalECU2->get_address_valid());
	EXPECT_TRUE(firstPartneredSecondECU->get_address_valid());
	EXPECT_TRUE(secondPartneredFirstEcu->get_address_valid());

	// Test sending a forced request for address claim
	EXPECT_TRUE(CANNetworkManager::CANNetwork.send_request_for_address_claim(0));

	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(firstPartneredSecondECU);
	CANNetworkManager::CANNetwork.deactivate_control_function(secondPartneredFirstEcu);
	CANNetworkManager::CANNetwork.deactivate_control_function(firstInternalECU);
	CANNetworkManager::CANNetwork.deactivate_control_function(secondInternalECU2);
}

TEST(ADDRESS_CLAIM_TESTS, CannotClaim)
{
	VirtualCANPlugin plugin;
	plugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	// Claim a very low name on every address
	NAME firstName(0);
	firstName.set_arbitrary_address_capable(true);
	firstName.set_industry_group(0);
	firstName.set_device_class(0);
	firstName.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::Engine));

	firstName.set_ecu_instance(0);
	firstName.set_function_instance(0);
	firstName.set_device_class_instance(0);
	firstName.set_manufacturer_code(1);

	// Force claim message
	CANMessageFrame testFrame = {};
	testFrame.channel = 0;

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	for (std::uint8_t i = 0; i < 0xFE; i++)
	{
		firstName.set_identity_number(i);

		std::uint64_t fullName = firstName.get_full_name();
		testFrame.identifier = 0x18EEFF00 | i;
		testFrame.isExtendedFrame = true;
		testFrame.dataLength = 8;
		testFrame.data[0] = static_cast<std::uint8_t>(fullName);
		testFrame.data[1] = static_cast<std::uint8_t>(fullName >> 8);
		testFrame.data[2] = static_cast<std::uint8_t>(fullName >> 16);
		testFrame.data[3] = static_cast<std::uint8_t>(fullName >> 24);
		testFrame.data[4] = static_cast<std::uint8_t>(fullName >> 32);
		testFrame.data[5] = static_cast<std::uint8_t>(fullName >> 40);
		testFrame.data[6] = static_cast<std::uint8_t>(fullName >> 48);
		testFrame.data[7] = static_cast<std::uint8_t>(fullName >> 56);

		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	}

	// Verify an internal control function with higher name cannot claim
	isobus::NAME secondName(0);
	secondName.set_arbitrary_address_capable(true);
	secondName.set_industry_group(1);
	secondName.set_device_class(6);
	secondName.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::RateControl));
	secondName.set_identity_number(65534);
	secondName.set_ecu_instance(1);
	secondName.set_function_instance(2);
	secondName.set_device_class_instance(0);
	secondName.set_manufacturer_code(1407);

	// Get the virtual CAN plugin back to a known state
	while (!plugin.get_queue_empty())
	{
		plugin.read_frame(testFrame);
	}
	ASSERT_TRUE(plugin.get_queue_empty());

	auto secondInternalECU2 = CANNetworkManager::CANNetwork.create_internal_control_function(secondName, 0);

	std::this_thread::sleep_for(std::chrono::milliseconds(1500));

	bool cannot_claim_message_seen = false;
	while (!plugin.get_queue_empty())
	{
		plugin.read_frame(testFrame);

		if ((8 == testFrame.dataLength) && (0xEEFF == ((testFrame.identifier >> 8) & 0x3FFFF)))
		{
			EXPECT_EQ(0xFE, (testFrame.identifier & 0xFF));
			cannot_claim_message_seen = true;
			break;
		}
	}
	EXPECT_TRUE(cannot_claim_message_seen);
	EXPECT_FALSE(secondInternalECU2->get_address_valid());
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(secondInternalECU2);
}
