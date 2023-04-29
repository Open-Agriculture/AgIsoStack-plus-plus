#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/utility/system_timing.hpp"

#include <memory>
#include <thread>

using namespace isobus;

TEST(CORE_TESTS, TestCreateAndDestroyPartners)
{
	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));

	isobus::PartneredControlFunction TestPartner1(0, vtNameFilters);
	isobus::PartneredControlFunction *TestPartner2 = new isobus::PartneredControlFunction(0, vtNameFilters);
	delete TestPartner2;
	auto TestPartner3 = std::make_shared<isobus::PartneredControlFunction>(0, vtNameFilters);
}

TEST(CORE_TESTS, TestCreateAndDestroyICFs)
{
	isobus::NAME TestDeviceNAME(0);
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(0);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);

	isobus::InternalControlFunction TestIcf1(TestDeviceNAME, 0x1C, 0);
	auto TestIcf2 = new isobus::InternalControlFunction(TestDeviceNAME, 0x80, 0);
	delete TestIcf2;
	auto TestIcf3 = std::make_shared<isobus::InternalControlFunction>(TestDeviceNAME, 0x81, 0);
}

TEST(CORE_TESTS, BusloadTest)
{
	EXPECT_EQ(0.0f, CANNetworkManager::CANNetwork.get_estimated_busload(0)); // This test runs early in the testing, so load should be zero.
	EXPECT_EQ(0.0f, CANNetworkManager::CANNetwork.get_estimated_busload(200)); // Invalid channel should return zero load

	// Send a bunch of messages through the receive process
	CANMessageFrame testFrame;
	testFrame.dataLength = 8;
	testFrame.channel = 0;
	testFrame.isExtendedFrame = true;
	testFrame.identifier = 0x18EFFFFE;
	memset(testFrame.data, 0, sizeof(testFrame.data));

	CANNetworkManager::CANNetwork.update(); // Make sure the network manager is initialized
	for (std::uint_fast8_t i = 0; i < 25; i++)
	{
		CANNetworkManager::process_receive_can_message_frame(testFrame); // Send a bunch of junk messages
	}
	testFrame.isExtendedFrame = false;
	testFrame.identifier = 0x7F;
	for (std::uint_fast8_t i = 0; i < 25; i++)
	{
		CANNetworkManager::process_receive_can_message_frame(testFrame); // Send a bunch of junk messages
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(101));
	CANNetworkManager::CANNetwork.update();

	// Bus load should be non zero, and less than 100%
	EXPECT_NE(0.0f, CANNetworkManager::CANNetwork.get_estimated_busload(0));
	EXPECT_LT(CANNetworkManager::CANNetwork.get_estimated_busload(0), 100.0f);
}

TEST(CORE_TESTS, CommandedAddress)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	isobus::NAME TestDeviceNAME(0);
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(3);
	TestDeviceNAME.set_device_class(4);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::FuelPropertiesSensor));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(1);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);

	auto testECU = std::make_shared<isobus::InternalControlFunction>(TestDeviceNAME, 0x43, 0);

	CANMessageFrame testFrame;

	std::uint32_t waitingTimestamp_ms = SystemTiming::get_timestamp_ms();

	while ((!testECU->get_address_valid()) &&
	       (!SystemTiming::time_expired_ms(waitingTimestamp_ms, 2000)))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	ASSERT_TRUE(testECU->get_address_valid());

	// Force claim some other ECU
	testFrame.dataLength = 8;
	testFrame.channel = 0;
	testFrame.isExtendedFrame = true;
	testFrame.identifier = 0x18EEFFF8;
	testFrame.data[0] = 0x03;
	testFrame.data[1] = 0x04;
	testFrame.data[2] = 0x00;
	testFrame.data[3] = 0x02;
	testFrame.data[4] = 0x00;
	testFrame.data[5] = 0x89;
	testFrame.data[6] = 0x00;
	testFrame.data[7] = 0xA0;
	CANNetworkManager::process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	// Let's construct a short BAM session for commanded address
	// We'll ignore the 50ms timing for the unit test

	testFrame.identifier = 0x18ECFFF8; // TP Command broadcast
	testFrame.data[0] = 0x20; // BAM Mux
	testFrame.data[1] = 9; // Data Length
	testFrame.data[2] = 0; // Data Length MSB
	testFrame.data[3] = 2; // Packet count
	testFrame.data[4] = 0xFF; // Reserved
	testFrame.data[5] = 0xD8; // PGN LSB
	testFrame.data[6] = 0xFE; // PGN middle byte
	testFrame.data[7] = 0x00; // PGN MSB
	CANNetworkManager::process_receive_can_message_frame(testFrame);

	std::uint64_t rawNAME = testECU->get_NAME().get_full_name();

	// data packet 1
	testFrame.identifier = 0x18EBFFF8;
	testFrame.data[0] = 1;
	testFrame.data[1] = static_cast<std::uint8_t>(rawNAME & 0xFF);
	testFrame.data[2] = static_cast<std::uint8_t>((rawNAME >> 8) & 0xFF);
	testFrame.data[3] = static_cast<std::uint8_t>((rawNAME >> 16) & 0xFF);
	testFrame.data[4] = static_cast<std::uint8_t>((rawNAME >> 24) & 0xFF);
	testFrame.data[5] = static_cast<std::uint8_t>((rawNAME >> 32) & 0xFF);
	testFrame.data[6] = static_cast<std::uint8_t>((rawNAME >> 40) & 0xFF);
	testFrame.data[7] = static_cast<std::uint8_t>((rawNAME >> 48) & 0xFF);
	CANNetworkManager::process_receive_can_message_frame(testFrame);

	// data packet 2
	testFrame.data[0] = 2;
	testFrame.data[1] = static_cast<std::uint8_t>((rawNAME >> 56) & 0xFF);
	testFrame.data[2] = 0x04; // Address
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0xFF;
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0xFF;
	testFrame.data[7] = 0xFF;
	CANNetworkManager::process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	EXPECT_EQ(0x04, testECU->get_address());
	testPlugin.close();
	CANHardwareInterface::stop();
}

TEST(CORE_TESTS, InvalidatingControlFunctions)
{
	CANMessageFrame testFrame;
	testFrame.channel = 0;
	testFrame.isExtendedFrame = true;
	NAME partnerName(0);
	partnerName.set_arbitrary_address_capable(true);
	partnerName.set_device_class(6);
	partnerName.set_ecu_instance(3);
	partnerName.set_function_code(130);
	partnerName.set_industry_group(3);
	partnerName.set_identity_number(967);

	CANNetworkManager::CANNetwork.update();

	// Request the address claim PGN
	const auto PGN = static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim);
	testFrame.data[0] = (PGN & std::numeric_limits<std::uint8_t>::max());
	testFrame.data[1] = ((PGN >> 8) & std::numeric_limits<std::uint8_t>::max());
	testFrame.data[2] = ((PGN >> 16) & std::numeric_limits<std::uint8_t>::max());
	testFrame.identifier = 0x18EAFFFE;
	testFrame.dataLength = 3;
	CANNetworkManager::process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	// Simulate waiting for some contention
	std::this_thread::sleep_for(std::chrono::milliseconds(15));
	CANNetworkManager::CANNetwork.update();

	std::vector<NAMEFilter> testFilter = { NAMEFilter(NAME::NAMEParameters::IdentityNumber, 967) };
	PartneredControlFunction testPartner(0, testFilter);

	std::uint64_t rawNAME = partnerName.get_full_name();

	// Force claim some kind of partner
	testFrame.identifier = 0x18EEFF79;
	testFrame.dataLength = 8;
	testFrame.data[0] = static_cast<std::uint8_t>(rawNAME & 0xFF);
	testFrame.data[1] = static_cast<std::uint8_t>((rawNAME >> 8) & 0xFF);
	testFrame.data[2] = static_cast<std::uint8_t>((rawNAME >> 16) & 0xFF);
	testFrame.data[3] = static_cast<std::uint8_t>((rawNAME >> 24) & 0xFF);
	testFrame.data[4] = static_cast<std::uint8_t>((rawNAME >> 32) & 0xFF);
	testFrame.data[5] = static_cast<std::uint8_t>((rawNAME >> 40) & 0xFF);
	testFrame.data[6] = static_cast<std::uint8_t>((rawNAME >> 48) & 0xFF);
	testFrame.data[7] = static_cast<std::uint8_t>((rawNAME >> 56) & 0xFF);
	CANNetworkManager::process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	EXPECT_TRUE(testPartner.get_address_valid());

	// Request the address claim PGN
	testFrame.data[0] = (PGN & std::numeric_limits<std::uint8_t>::max());
	testFrame.data[1] = ((PGN >> 8) & std::numeric_limits<std::uint8_t>::max());
	testFrame.data[2] = ((PGN >> 16) & std::numeric_limits<std::uint8_t>::max());
	testFrame.identifier = 0x18EAFFFE;
	testFrame.dataLength = 3;
	CANNetworkManager::process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	// Now, if we wait a while, that partner didn't claim again, so it should be invalid.
	std::this_thread::sleep_for(std::chrono::seconds(2));
	CANNetworkManager::CANNetwork.update();

	EXPECT_FALSE(testPartner.get_address_valid());
}
