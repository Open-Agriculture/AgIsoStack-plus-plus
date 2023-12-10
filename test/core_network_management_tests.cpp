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

#include "helpers/control_function_helpers.hpp"

using namespace isobus;

static std::shared_ptr<ControlFunction> testControlFunction = nullptr;
static ControlFunctionState testControlFunctionState = ControlFunctionState::Offline;
static bool wasTestStateCallbackHit = false;
void test_control_function_state_callback(std::shared_ptr<ControlFunction> controlFunction, ControlFunctionState state)
{
	testControlFunction = controlFunction;
	testControlFunctionState = state;
	wasTestStateCallbackHit = true;
}

TEST(CORE_TESTS, TestCreateAndDestroyPartners)
{
	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));

	auto testPartner1 = isobus::PartneredControlFunction::create(0, vtNameFilters);
	auto testPartner2 = isobus::PartneredControlFunction::create(0, vtNameFilters);
	EXPECT_TRUE(testPartner2->destroy());
	auto TestPartner3 = isobus::PartneredControlFunction::create(0, vtNameFilters);

	EXPECT_TRUE(testPartner1->destroy());
	EXPECT_TRUE(TestPartner3->destroy());
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
	TestDeviceNAME.set_manufacturer_code(1407);

	auto testICF1 = isobus::InternalControlFunction::create(TestDeviceNAME, 0x1C, 0);

	TestDeviceNAME.set_ecu_instance(1);
	auto testICF2 = isobus::InternalControlFunction::create(TestDeviceNAME, 0x80, 0);
	ASSERT_TRUE(testICF2->destroy());

	TestDeviceNAME.set_ecu_instance(2);
	auto testICF3 = isobus::InternalControlFunction::create(TestDeviceNAME, 0x81, 0);

	ASSERT_TRUE(testICF1->destroy());
	ASSERT_TRUE(testICF3->destroy());
}

TEST(CORE_TESTS, BusloadTest)
{
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
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	auto internalECU = test_helpers::claim_internal_control_function(0x43, 0);
	auto externalECU = test_helpers::force_claim_partnered_control_function(0xF8, 0);

	// Let's construct a short BAM session for commanded address
	// We'll ignore the 50ms timing for the unit test

	CANMessageFrame testFrame = {};
	testFrame.identifier = 0x18ECFFF8; // TP Command broadcast
	testFrame.isExtendedFrame = true;
	testFrame.dataLength = 8;
	testFrame.data[0] = 0x20; // BAM Mux
	testFrame.data[1] = 9; // Data Length
	testFrame.data[2] = 0; // Data Length MSB
	testFrame.data[3] = 2; // Packet count
	testFrame.data[4] = 0xFF; // Reserved
	testFrame.data[5] = 0xD8; // PGN LSB
	testFrame.data[6] = 0xFE; // PGN middle byte
	testFrame.data[7] = 0x00; // PGN MSB
	CANNetworkManager::process_receive_can_message_frame(testFrame);

	std::uint64_t rawNAME = internalECU->get_NAME().get_full_name();

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
	EXPECT_EQ(0x04, internalECU->get_address());

	EXPECT_TRUE(internalECU->destroy());
	EXPECT_TRUE(externalECU->destroy());
	CANHardwareInterface::stop();
}

TEST(CORE_TESTS, InvalidatingControlFunctions)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	// Request the address claim PGN
	CANMessageFrame testFrame = {};
	testFrame.identifier = 0x18EAFFFE;
	testFrame.isExtendedFrame = true;
	testFrame.dataLength = 3;
	const auto PGN = static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim);
	testFrame.data[0] = (PGN & 0xFF);
	testFrame.data[1] = ((PGN >> 8) & 0xFF);
	testFrame.data[2] = ((PGN >> 16) & 0xFF);
	CANNetworkManager::process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	// Simulate waiting for some contention
	std::this_thread::sleep_for(std::chrono::milliseconds(15));
	CANNetworkManager::CANNetwork.update();

	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(test_control_function_state_callback);
	EXPECT_FALSE(wasTestStateCallbackHit);
	EXPECT_EQ(testControlFunction, nullptr);
	EXPECT_EQ(testControlFunctionState, ControlFunctionState::Offline);

	auto testPartner = test_helpers::force_claim_partnered_control_function(0x79, 0);
	EXPECT_TRUE(wasTestStateCallbackHit);
	EXPECT_NE(testControlFunction, nullptr);
	EXPECT_EQ(testControlFunctionState, ControlFunctionState::Online);
	wasTestStateCallbackHit = false;

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

	EXPECT_FALSE(testPartner->get_address_valid());
	EXPECT_TRUE(wasTestStateCallbackHit);
	EXPECT_NE(testControlFunction, nullptr);
	EXPECT_EQ(testControlFunctionState, ControlFunctionState::Offline);

	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(test_control_function_state_callback);
	testControlFunction.reset();
	EXPECT_TRUE(testPartner->destroy());
	CANHardwareInterface::stop();
}

TEST(CORE_TESTS, SimilarControlFunctions)
{
	CANMessageFrame testFrame;
	testFrame.channel = 0;
	testFrame.isExtendedFrame = true;
	CANNetworkManager::CANNetwork.update();

	// Make a partner that is a fuel system
	// Using a less common function to avoid interfering with other tests when not running under CTest
	const isobus::NAMEFilter filterFuelSystem(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::FuelSystem));
	const std::vector<isobus::NAMEFilter> nameFilters = { filterFuelSystem };
	auto TestPartner = isobus::PartneredControlFunction::create(0, nameFilters);

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

	std::uint64_t rawNAME = 0xa0000F000425e9f8;

	// Force claim some kind of TC
	testFrame.identifier = 0x18EEFF7A;
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

	// Partner should be valid with that same NAME
	EXPECT_EQ(TestPartner->get_NAME().get_full_name(), rawNAME);

	// Now, claim something else that matches a TC. The original partner should remain the same.
	auto testOtherTCNAME = NAME(rawNAME);
	testOtherTCNAME.set_ecu_instance(1);
	testOtherTCNAME.set_function_instance(1);
	rawNAME = testOtherTCNAME.get_full_name();
	testFrame.identifier = 0x18EEFF7B;
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

	// Partner should never change
	EXPECT_EQ(TestPartner->get_NAME().get_full_name(), 0xa0000F000425e9f8);
	EXPECT_TRUE(TestPartner->destroy());
}
