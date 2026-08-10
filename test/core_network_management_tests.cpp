#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_message_frame.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"

#include <memory>
#include <thread>

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"
#include "helpers/test_fixture.hpp"

using namespace isobus;

static std::shared_ptr<ControlFunction> testControlFunction = nullptr;
static ControlFunctionState testControlFunctionState = ControlFunctionState::Offline;
static bool wasTestStateCallbackHit = false;
static bool wasAnyControlFunctionCallbackHit = false;

void test_control_function_state_callback(std::shared_ptr<ControlFunction> controlFunction, ControlFunctionState state)
{
	testControlFunction = controlFunction;
	testControlFunctionState = state;
	wasTestStateCallbackHit = true;
}

static void test_any_control_function_callback(const CANMessage &, void *)
{
	wasAnyControlFunctionCallbackHit = true;
}

static std::uint32_t sniffedMessageCount = 0;
static std::shared_ptr<ControlFunction> sniffedMessageSource = nullptr;
static std::shared_ptr<ControlFunction> sniffedMessageDestination = nullptr;

static void test_sniffed_message_callback(const CANMessage &message, void *)
{
	sniffedMessageCount++;
	sniffedMessageSource = message.get_source_control_function();
	sniffedMessageDestination = message.get_destination_control_function();
}

static std::uint32_t sniffedPayloadLength = 0;
static std::vector<std::uint8_t> sniffedPayload;

static void test_sniffed_payload_callback(const CANMessage &message, void *)
{
	sniffedMessageCount++;
	sniffedPayloadLength = message.get_data_length();
	sniffedPayload = message.get_data();
	sniffedMessageSource = message.get_source_control_function();
	sniffedMessageDestination = message.get_destination_control_function();
}

static void test_reentrant_sniffed_message_callback(const CANMessage &message, void *)
{
	sniffedMessageCount++;
	// Calling back into the registry from inside a callback must not deadlock
	CANNetworkManager::CANNetwork.is_sniffed_parameter_group_number_of_interest(message.get_identifier().get_parameter_group_number());
	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(message.get_identifier().get_parameter_group_number(), test_reentrant_sniffed_message_callback, nullptr);
}

class CoreTest : public AgIsoStackTestFixture
{
	// Wrapper to give tests a more meaningful name - no content.
};

TEST_F(CoreTest, TestCreateAndDestroyPartners)
{
	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));

	auto testPartner1 = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);
	auto testPartner2 = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);
	CANNetworkManager::CANNetwork.deactivate_control_function(testPartner2);
	auto TestPartner3 = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

	CANNetworkManager::CANNetwork.deactivate_control_function(testPartner1);
	CANNetworkManager::CANNetwork.deactivate_control_function(TestPartner3);
}

TEST_F(CoreTest, TestCreateAndDestroyICFs)
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

	auto testICF1 = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);

	TestDeviceNAME.set_ecu_instance(1);
	auto testICF2 = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);
	CANNetworkManager::CANNetwork.deactivate_control_function(testICF2);

	TestDeviceNAME.set_ecu_instance(2);
	auto testICF3 = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);

	CANNetworkManager::CANNetwork.deactivate_control_function(testICF1);
	CANNetworkManager::CANNetwork.deactivate_control_function(testICF3);
}

TEST_F(CoreTest, BusloadTest)
{
	EXPECT_EQ(0.0f, CANNetworkManager::CANNetwork.get_estimated_busload(200)); // Invalid channel should return zero load

#ifndef DISABLE_BUSLOAD_MONITORING
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
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame); // Send a bunch of junk messages
	}
	testFrame.isExtendedFrame = false;
	testFrame.identifier = 0x7F;
	for (std::uint_fast8_t i = 0; i < 25; i++)
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame); // Send a bunch of junk messages
	}
	time_source.simulate_delay_ms(101);
	CANNetworkManager::CANNetwork.update();

	// Bus load should be non zero, and less than 100%
	EXPECT_NE(0.0f, CANNetworkManager::CANNetwork.get_estimated_busload(0));
	EXPECT_LT(CANNetworkManager::CANNetwork.get_estimated_busload(0), 100.0f);
#else
	// When busload monitoring is disabled, get_estimated_busload should always return 0
	EXPECT_EQ(0.0f, CANNetworkManager::CANNetwork.get_estimated_busload(0));
#endif
}

TEST_F(CoreTest, CommandedAddress)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x43, 0, time_source);
	auto externalECU = test_helpers::force_claim_partnered_control_function(0xF8, 0);

	// Let's construct a short BAM session for commanded address
	// We'll ignore the 50ms timing for the unit test

	// Broadcast Announce Message
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  externalECU,
	  {
	    0x20, // BAM Mux
	    9, // Data Length
	    0, // Data Length MSB
	    2, // Packet count
	    0xFF, // Reserved
	    0xD8, // PGN LSB
	    0xFE, // PGN middle byte
	    0x00, // PGN MSB
	  }));

	std::uint64_t rawNAME = internalECU->get_NAME().get_full_name();

	// data packet 1
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  externalECU,
	  {
	    1, // Sequence Number
	    static_cast<std::uint8_t>(rawNAME),
	    static_cast<std::uint8_t>(rawNAME >> 8),
	    static_cast<std::uint8_t>(rawNAME >> 16),
	    static_cast<std::uint8_t>(rawNAME >> 24),
	    static_cast<std::uint8_t>(rawNAME >> 32),
	    static_cast<std::uint8_t>(rawNAME >> 40),
	    static_cast<std::uint8_t>(rawNAME >> 48),
	  }));

	// data packet 2
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  externalECU,
	  {
	    2, // Sequence Number
	    static_cast<std::uint8_t>(rawNAME >> 56),
	    0x04, // Address
	    0xFF,
	    0xFF,
	    0xFF,
	    0xFF,
	    0xFF,
	  }));
	CANNetworkManager::CANNetwork.update();

	time_source.update_for_ms(500);
	EXPECT_EQ(0x04, internalECU->get_address());

	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANNetworkManager::CANNetwork.deactivate_control_function(externalECU);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, InvalidatingControlFunctions)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	// Request the address claim PGN to simulate a control function starting to claim an address
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_pgn_request(
	  0xEE00, // Address Claim PGN
	  nullptr,
	  nullptr));
	CANNetworkManager::CANNetwork.update();

	// Simulate waiting for some contention
	time_source.update_for_ms(15);
	CANNetworkManager::CANNetwork.update();

	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(test_control_function_state_callback);
	EXPECT_FALSE(wasTestStateCallbackHit);
	EXPECT_EQ(testControlFunction, nullptr);
	EXPECT_EQ(testControlFunctionState, ControlFunctionState::Offline);

	// Make a control function claim it's address
	auto testPartner = test_helpers::force_claim_partnered_control_function(0x79, 0);
	EXPECT_TRUE(wasTestStateCallbackHit);
	EXPECT_NE(testControlFunction, nullptr);
	EXPECT_EQ(testControlFunctionState, ControlFunctionState::Online);
	wasTestStateCallbackHit = false;

	// Request the address claim PGN again
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_pgn_request(
	  0xEE00, // Address Claim PGN
	  nullptr,
	  nullptr));
	CANNetworkManager::CANNetwork.update();

	// Now, if we wait a while, that partner didn't claim again, so it should be invalid.
	time_source.update_for_ms(2000);
	CANNetworkManager::CANNetwork.update();

	EXPECT_FALSE(testPartner->get_address_valid());
	EXPECT_TRUE(wasTestStateCallbackHit);
	EXPECT_NE(testControlFunction, nullptr);
	EXPECT_EQ(testControlFunctionState, ControlFunctionState::Offline);

	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(test_control_function_state_callback);
	testControlFunction.reset();
	CANNetworkManager::CANNetwork.deactivate_control_function(testPartner);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, NewExternalControlFunctionTriggersStateCallback)
{
	wasTestStateCallbackHit = false;
	testControlFunction.reset();
	testControlFunctionState = ControlFunctionState::Offline;
	CANNetworkManager::CANNetwork.update();
	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(test_control_function_state_callback);

	constexpr std::uint8_t TEST_CHANNEL = 3;
	constexpr std::uint8_t INITIAL_ADDRESS = 0x91;

	isobus::CANMessageFrame addressClaim = {};
	addressClaim.channel = TEST_CHANNEL;
	addressClaim.identifier = 0x18EEFF00 | INITIAL_ADDRESS;
	addressClaim.isExtendedFrame = true;
	addressClaim.dataLength = 8;

	constexpr std::uint64_t DUMMY_NAME = 0x0123456789ABCDEFULL;
	for (std::uint8_t byteIndex = 0; byteIndex < addressClaim.dataLength; byteIndex++)
	{
		addressClaim.data[byteIndex] = static_cast<std::uint8_t>(DUMMY_NAME >> (8 * byteIndex));
	}

	isobus::CANNetworkManager::CANNetwork.process_receive_can_message_frame(addressClaim);

	EXPECT_TRUE(wasTestStateCallbackHit);
	ASSERT_NE(nullptr, testControlFunction);
	EXPECT_EQ(isobus::ControlFunctionState::Online, testControlFunctionState);
	EXPECT_EQ(INITIAL_ADDRESS, testControlFunction->get_address());
	EXPECT_EQ(TEST_CHANNEL, testControlFunction->get_can_port());

	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(test_control_function_state_callback);
	testControlFunction.reset();
	wasTestStateCallbackHit = false;
}

TEST_F(CoreTest, ControlFunctionAddressChangeTriggersStateCallback)
{
	wasTestStateCallbackHit = false;
	testControlFunction.reset();
	testControlFunctionState = ControlFunctionState::Offline;
	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(test_control_function_state_callback);

	constexpr std::uint8_t INITIAL_ADDRESS = 0x92;
	constexpr std::uint8_t NEW_ADDRESS = 0x93;
	auto partner = test_helpers::force_claim_partnered_control_function(INITIAL_ADDRESS, 0);

	// Reset after the initial Online event so we can capture the address change notification
	wasTestStateCallbackHit = false;
	testControlFunction.reset();
	testControlFunctionState = ControlFunctionState::Offline;

	isobus::CANMessageFrame addressClaim = {};
	addressClaim.channel = partner->get_can_port();
	addressClaim.identifier = 0x18EEFF00 | NEW_ADDRESS;
	addressClaim.isExtendedFrame = true;
	addressClaim.dataLength = 8;

	const std::uint64_t fullName = partner->get_NAME().get_full_name();
	for (std::uint8_t byteIndex = 0; byteIndex < addressClaim.dataLength; byteIndex++)
	{
		addressClaim.data[byteIndex] = static_cast<std::uint8_t>(fullName >> (8 * byteIndex));
	}

	isobus::CANNetworkManager::CANNetwork.process_receive_can_message_frame(addressClaim);

	EXPECT_TRUE(wasTestStateCallbackHit);
	EXPECT_EQ(isobus::ControlFunctionState::Online, testControlFunctionState);
	ASSERT_EQ(partner, testControlFunction);
	EXPECT_EQ(NEW_ADDRESS, partner->get_address());

	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(test_control_function_state_callback);
	CANNetworkManager::CANNetwork.deactivate_control_function(partner);
	testControlFunction.reset();
	wasTestStateCallbackHit = false;
}

TEST_F(CoreTest, PartnerCreatedAfterExternalClaimHasValidAddress)
{
	constexpr std::uint8_t TEST_CHANNEL = 3;
	constexpr std::uint8_t CLAIMED_ADDRESS = 0x96;

	const isobus::NAME claimedName = test_helpers::find_available_name(TEST_CHANNEL);

	isobus::CANMessageFrame addressClaim = {};
	addressClaim.channel = TEST_CHANNEL;
	addressClaim.identifier = 0x18EEFF00 | CLAIMED_ADDRESS;
	addressClaim.isExtendedFrame = true;
	addressClaim.dataLength = 8;

	const std::uint64_t fullName = claimedName.get_full_name();
	for (std::uint8_t byteIndex = 0; byteIndex < addressClaim.dataLength; byteIndex++)
	{
		addressClaim.data[byteIndex] = static_cast<std::uint8_t>(fullName >> (8 * byteIndex));
	}

	isobus::CANNetworkManager::CANNetwork.process_receive_can_message_frame(addressClaim);

	std::vector<isobus::NAMEFilter> nameFilters{
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::IdentityNumber, claimedName.get_identity_number()),
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::ManufacturerCode, claimedName.get_manufacturer_code()),
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::FunctionCode, claimedName.get_function_code()),
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::FunctionInstance, claimedName.get_function_instance()),
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::EcuInstance, claimedName.get_ecu_instance()),
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::DeviceClass, claimedName.get_device_class()),
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::DeviceClassInstance, claimedName.get_device_class_instance()),
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::IndustryGroup, claimedName.get_industry_group()),
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::ArbitraryAddressCapable, claimedName.get_arbitrary_address_capable())
	};

	auto partner = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(TEST_CHANNEL, nameFilters);

	ASSERT_NE(nullptr, partner);
	EXPECT_TRUE(partner->get_address_valid());
	EXPECT_EQ(CLAIMED_ADDRESS, partner->get_address());
	EXPECT_EQ(claimedName.get_full_name(), partner->get_NAME().get_full_name());

	isobus::CANNetworkManager::CANNetwork.deactivate_control_function(partner);
}

TEST_F(CoreTest, SimilarControlFunctions)
{
	CANNetworkManager::CANNetwork.update();

	// Make a partner that is a fuel system
	// Using a less common function to avoid interfering with other tests when not running under CTest
	const isobus::NAMEFilter filterFuelSystem(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::FuelSystem));
	const std::vector<isobus::NAMEFilter> nameFilters = { filterFuelSystem };
	auto TestPartner = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, nameFilters);

	// Quick test to make sure partner is working
	EXPECT_EQ(1, TestPartner->get_number_name_filters_with_parameter_type(isobus::NAME::NAMEParameters::FunctionCode));

	// Request the address claim PGN
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_pgn_request(
	  0xEE00, // Address Claim PGN
	  nullptr,
	  nullptr));
	CANNetworkManager::CANNetwork.update();
	CANNetworkManager::CANNetwork.update();

	// Simulate waiting for some contention
	std::this_thread::sleep_for(std::chrono::milliseconds(15));
	CANNetworkManager::CANNetwork.update();

	std::uint64_t rawNAME = 0xa0000F000425e9f8;

	// Force claim some kind of TC
	auto firstTC = test_helpers::create_mock_control_function(0x7A);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(
	  6,
	  0xEE00, // Address Claim PGN
	  firstTC,
	  {
	    static_cast<std::uint8_t>(rawNAME),
	    static_cast<std::uint8_t>(rawNAME >> 8),
	    static_cast<std::uint8_t>(rawNAME >> 16),
	    static_cast<std::uint8_t>(rawNAME >> 24),
	    static_cast<std::uint8_t>(rawNAME >> 32),
	    static_cast<std::uint8_t>(rawNAME >> 40),
	    static_cast<std::uint8_t>(rawNAME >> 48),
	    static_cast<std::uint8_t>(rawNAME >> 56),
	  }));
	CANNetworkManager::CANNetwork.update();

	// Partner should be valid with that same NAME
	EXPECT_EQ(TestPartner->get_NAME().get_full_name(), rawNAME);

	// Now, claim something else that matches a TC. The original partner should remain the same.
	auto secondTCNAME = NAME(rawNAME);
	secondTCNAME.set_ecu_instance(1);
	secondTCNAME.set_function_instance(1);
	rawNAME = secondTCNAME.get_full_name();
	auto secondTC = test_helpers::create_mock_control_function(0x7B);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(
	  6,
	  0xEE00, // Address Claim PGN
	  secondTC,
	  {
	    static_cast<std::uint8_t>(rawNAME),
	    static_cast<std::uint8_t>(rawNAME >> 8),
	    static_cast<std::uint8_t>(rawNAME >> 16),
	    static_cast<std::uint8_t>(rawNAME >> 24),
	    static_cast<std::uint8_t>(rawNAME >> 32),
	    static_cast<std::uint8_t>(rawNAME >> 40),
	    static_cast<std::uint8_t>(rawNAME >> 48),
	    static_cast<std::uint8_t>(rawNAME >> 56),
	  }));
	CANNetworkManager::CANNetwork.update();

	// Partner should never change
	EXPECT_EQ(TestPartner->get_NAME().get_full_name(), 0xa0000F000425e9f8);
	CANNetworkManager::CANNetwork.deactivate_control_function(TestPartner);
}

TEST_F(CoreTest, SniffedMessageCallbacksSeeExternalToExternalTraffic)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	// The network manager discards received frames until update() has initialized it
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x7A, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x7B, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	sniffedMessageSource = nullptr;
	sniffedMessageDestination = nullptr;
	wasAnyControlFunctionCallbackHit = false;

	EXPECT_FALSE(CANNetworkManager::CANNetwork.is_sniffed_parameter_group_number_of_interest(0xEF00));
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xEF00, test_sniffed_message_callback, nullptr);
	EXPECT_TRUE(CANNetworkManager::CANNetwork.is_sniffed_parameter_group_number_of_interest(0xEF00));
	EXPECT_FALSE(CANNetworkManager::CANNetwork.is_sniffed_parameter_group_number_of_interest(0xFE00));

	// The legacy any-control-function callbacks must keep ignoring traffic that isn't addressed to us
	CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(0xEF00, test_any_control_function_callback, nullptr);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  6,
	  0xEF00, // Proprietary A, addressed from one external control function to another
	  listener,
	  talker,
	  { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 }));
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(1, sniffedMessageCount);
	EXPECT_EQ(talker, sniffedMessageSource);
	EXPECT_EQ(listener, sniffedMessageDestination);
	EXPECT_FALSE(wasAnyControlFunctionCallbackHit);

	// A PGN nobody registered for must not reach the sniffing callback
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  6,
	  0x9F00, // A destination specific PGN that no sniffing callback asked for
	  listener,
	  talker,
	  { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 }));
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1, sniffedMessageCount);

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xEF00, test_sniffed_message_callback, nullptr);
	EXPECT_FALSE(CANNetworkManager::CANNetwork.is_sniffed_parameter_group_number_of_interest(0xEF00));

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  6,
	  0xEF00,
	  listener,
	  talker,
	  { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 }));
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1, sniffedMessageCount);

	CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(0xEF00, test_any_control_function_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedMessageCallbackCanMutateTheRegistryFromInsideTheCallback)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x7C, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x7D, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xEF00, test_reentrant_sniffed_message_callback, nullptr);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  6,
	  0xEF00,
	  listener,
	  talker,
	  { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 }));
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(1, sniffedMessageCount);
	// The callback removed itself, so a second identical message must not reach it
	EXPECT_FALSE(CANNetworkManager::CANNetwork.is_sniffed_parameter_group_number_of_interest(0xEF00));

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  6,
	  0xEF00,
	  listener,
	  talker,
	  { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 }));
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1, sniffedMessageCount);

	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedMessageCallbacksSeeAssembledBroadcastMessages)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x7E, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	sniffedPayloadLength = 0;
	sniffedMessageSource = nullptr;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	// Broadcast announce message for a 17 byte payload sent as 3 data frames
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  talker,
	  {
	    32, // BAM Mux
	    17, // Data Length
	    0, // Data Length MSB
	    3, // Packet count
	    0xFF, // Reserved
	    0xEC, // PGN LSB
	    0xFE, // PGN middle byte
	    0x00, // PGN MSB
	  }));
	CANNetworkManager::CANNetwork.update();

	for (std::uint8_t sequenceNumber = 1; sequenceNumber <= 3; sequenceNumber++)
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(
		  7,
		  0xEB00, // Transport Protocol Data Transfer
		  talker,
		  { sequenceNumber, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 }));
		CANNetworkManager::CANNetwork.update();
	}

	EXPECT_EQ(1, sniffedMessageCount);
	EXPECT_EQ(17, sniffedPayloadLength);
	EXPECT_EQ(talker, sniffedMessageSource);

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANHardwareInterface::stop();
}

// Sends the connection management frame that opens a 17 byte, 3 packet connection mode transfer
static void send_sniffed_request_to_send(std::shared_ptr<ControlFunction> talker, std::shared_ptr<ControlFunction> listener, std::uint32_t parameterGroupNumber)
{
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  listener,
	  talker,
	  {
	    16, // Request To Send Mux
	    17, // Data Length
	    0, // Data Length MSB
	    3, // Packet count
	    0xFF, // Packets per CTS
	    static_cast<std::uint8_t>(parameterGroupNumber & 0xFF),
	    static_cast<std::uint8_t>((parameterGroupNumber >> 8) & 0xFF),
	    static_cast<std::uint8_t>((parameterGroupNumber >> 16) & 0xFF),
	  }));
	CANNetworkManager::CANNetwork.update();
}

// Sends a connection management frame that is not a request to send. Only the multiplexor and the
// PGN mean anything to a passive observer, so the bytes between them are left at the filler value
static void send_sniffed_connection_management(std::shared_ptr<ControlFunction> sender,
                                               std::shared_ptr<ControlFunction> receiver,
                                               std::uint8_t multiplexor,
                                               std::uint32_t parameterGroupNumber)
{
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  receiver,
	  sender,
	  {
	    multiplexor,
	    3,
	    0xFF,
	    0xFF,
	    0xFF,
	    static_cast<std::uint8_t>(parameterGroupNumber & 0xFF),
	    static_cast<std::uint8_t>((parameterGroupNumber >> 8) & 0xFF),
	    static_cast<std::uint8_t>((parameterGroupNumber >> 16) & 0xFF),
	  }));
	CANNetworkManager::CANNetwork.update();
}

// The byte a correctly reassembled 17 byte message holds at the given position of the given packet,
// or the padding the standard asks for once the payload runs out
static std::uint8_t sniffed_payload_byte(std::uint8_t sequenceNumber, std::uint8_t indexInPacket)
{
	const std::uint16_t payloadIndex = ((sequenceNumber - 1) * 7) + indexInPacket;
	return (payloadIndex < 17) ? static_cast<std::uint8_t>(payloadIndex + 1) : 0xFF;
}

// Sends one data transfer frame of the transfer opened by send_sniffed_request_to_send
static void send_sniffed_data_transfer(std::shared_ptr<ControlFunction> talker, std::shared_ptr<ControlFunction> listener, std::uint8_t sequenceNumber)
{
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  listener,
	  talker,
	  { sequenceNumber,
	    sniffed_payload_byte(sequenceNumber, 0),
	    sniffed_payload_byte(sequenceNumber, 1),
	    sniffed_payload_byte(sequenceNumber, 2),
	    sniffed_payload_byte(sequenceNumber, 3),
	    sniffed_payload_byte(sequenceNumber, 4),
	    sniffed_payload_byte(sequenceNumber, 5),
	    sniffed_payload_byte(sequenceNumber, 6) }));
	CANNetworkManager::CANNetwork.update();
}

TEST_F(CoreTest, SniffedMessageCallbacksSeeAssembledConnectionModeMessages)
{
	// A second device on the same virtual bus, so the test can see everything the stack transmits
	VirtualCANPlugin busObserver;
	busObserver.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x80, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x81, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	sniffedPayloadLength = 0;
	sniffedPayload.clear();
	sniffedMessageSource = nullptr;
	sniffedMessageDestination = nullptr;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	CANMessageFrame transmittedFrame = {};
	while (!busObserver.get_queue_empty())
	{
		busObserver.read_frame(transmittedFrame);
	}

	send_sniffed_request_to_send(talker, listener, 0xFEEC);

	// Answering the request to send would make the sender transmit to us as well as to its receiver
	EXPECT_TRUE(busObserver.get_queue_empty());

	// Out of order and repeated packets are both normal once a receiver asks for a retransmission
	send_sniffed_data_transfer(talker, listener, 2);
	send_sniffed_data_transfer(talker, listener, 2);
	EXPECT_EQ(0, sniffedMessageCount);
	send_sniffed_data_transfer(talker, listener, 1);
	send_sniffed_data_transfer(talker, listener, 3);

	EXPECT_EQ(1, sniffedMessageCount);
	EXPECT_EQ(17, sniffedPayloadLength);
	EXPECT_EQ(talker, sniffedMessageSource);
	EXPECT_EQ(listener, sniffedMessageDestination);
	ASSERT_EQ(17, sniffedPayload.size());
	for (std::uint8_t i = 0; i < 17; i++)
	{
		EXPECT_EQ(i + 1, sniffedPayload[i]);
	}

	// The end of message acknowledgement belongs to the real receiver, and must not produce a second delivery
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  7,
	  0xEC00,
	  talker,
	  listener,
	  { 19, 17, 0, 3, 0xFF, 0xEC, 0xFE, 0x00 }));
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1, sniffedMessageCount);
	EXPECT_TRUE(busObserver.get_queue_empty());

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedConnectionModeSessionSurvivesRemovingTheCallbackMidSession)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x84, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x85, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	send_sniffed_request_to_send(talker, listener, 0xFEEC);
	send_sniffed_data_transfer(talker, listener, 1);

	// Interest is decided once, at the request to send. Unregistering half way through stops the
	// callbacks, but the session it already started is left to finish rather than torn out
	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	send_sniffed_data_transfer(talker, listener, 2);
	send_sniffed_data_transfer(talker, listener, 3);
	EXPECT_EQ(0, sniffedMessageCount);

	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedConnectionModeSessionIsDroppedWhenTheConnectionAborts)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x82, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x83, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	send_sniffed_request_to_send(talker, listener, 0xFEEC);
	send_sniffed_data_transfer(talker, listener, 1);

	// The real receiver gives up, so the rest of the packets are never going to arrive
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  7,
	  0xEC00,
	  talker,
	  listener,
	  {
	    255, // Connection Abort Mux
	    3, // Timeout
	    0xFF,
	    0xFF,
	    0xFF,
	    0xEC, // PGN LSB
	    0xFE, // PGN middle byte
	    0x00, // PGN MSB
	  }));
	CANNetworkManager::CANNetwork.update();

	// A sender that ignores the abort must not be able to complete the message we already gave up on
	send_sniffed_data_transfer(talker, listener, 2);
	send_sniffed_data_transfer(talker, listener, 3);
	EXPECT_EQ(0, sniffedMessageCount);

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedConnectionModeSessionIsReplacedByAnUninterestedRequestToSend)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x86, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x87, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	send_sniffed_request_to_send(talker, listener, 0xFEEC);
	send_sniffed_data_transfer(talker, listener, 1);

	// The same pair now starts a transfer nobody asked for. Leaving the half finished session in place
	// would let these packets land in it, stitching the callback a payload from two different messages
	send_sniffed_request_to_send(talker, listener, 0xEF00);

	send_sniffed_data_transfer(talker, listener, 2);
	send_sniffed_data_transfer(talker, listener, 3);
	EXPECT_EQ(0, sniffedMessageCount);

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedConnectionModeEndOfMessageAcknowledgeLeavesTheReverseSessionAlone)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x88, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x89, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	// Both control functions send the same PGN to each other at the same time, which the standard allows
	send_sniffed_request_to_send(talker, listener, 0xFEEC);
	send_sniffed_request_to_send(listener, talker, 0xFEEC);

	send_sniffed_data_transfer(talker, listener, 1);
	send_sniffed_data_transfer(talker, listener, 2);
	send_sniffed_data_transfer(talker, listener, 3);
	EXPECT_EQ(1, sniffedMessageCount);

	send_sniffed_data_transfer(listener, talker, 1);

	// This closes the transfer that just finished. Looking both ways would instead find the unrelated
	// transfer still running in reverse and throw it away
	send_sniffed_connection_management(listener, talker, 19, 0xFEEC);

	send_sniffed_data_transfer(listener, talker, 2);
	send_sniffed_data_transfer(listener, talker, 3);
	EXPECT_EQ(2, sniffedMessageCount);
	EXPECT_EQ(listener, sniffedMessageSource);
	EXPECT_EQ(talker, sniffedMessageDestination);

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedConnectionModeClearToSendIsCheckedAgainstTheSessionPgn)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x8A, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x8B, 0);
	auto otherTalker = test_helpers::force_claim_partnered_control_function(0x8C, 0);
	auto otherListener = test_helpers::force_claim_partnered_control_function(0x8D, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	// A clear to send for the PGN we follow is the receiver pacing the transfer, so the session lives
	send_sniffed_request_to_send(talker, listener, 0xFEEC);
	send_sniffed_data_transfer(talker, listener, 1);
	send_sniffed_connection_management(listener, talker, 17, 0xFEEC);
	send_sniffed_data_transfer(talker, listener, 2);
	send_sniffed_data_transfer(talker, listener, 3);
	EXPECT_EQ(1, sniffedMessageCount);

	// A clear to send for a different PGN means we missed a request to send, so our session is stale
	send_sniffed_request_to_send(otherTalker, otherListener, 0xFEEC);
	send_sniffed_data_transfer(otherTalker, otherListener, 1);
	send_sniffed_connection_management(otherListener, otherTalker, 17, 0xEF00);
	send_sniffed_data_transfer(otherTalker, otherListener, 2);
	send_sniffed_data_transfer(otherTalker, otherListener, 3);
	EXPECT_EQ(1, sniffedMessageCount);

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANNetworkManager::CANNetwork.deactivate_control_function(otherTalker);
	CANNetworkManager::CANNetwork.deactivate_control_function(otherListener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedConnectionModeSessionIsDroppedAfterTheProtocolTimeout)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x8E, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x8F, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	send_sniffed_request_to_send(talker, listener, 0xFEEC);
	send_sniffed_data_transfer(talker, listener, 1);

	// Nothing arrives for longer than the protocol allows the connection to go quiet
	time_source.simulate_delay_ms(1300); // T2/T3 = 1250ms
	CANNetworkManager::CANNetwork.update();

	// Late packets belong to a connection that is over, and must not be able to complete the message
	send_sniffed_data_transfer(talker, listener, 2);
	send_sniffed_data_transfer(talker, listener, 3);
	EXPECT_EQ(0, sniffedMessageCount);

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedConnectionModeCapacityIsNotSpentOnUninterestedConnections)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x90, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x91, 0);
	auto otherTalker = test_helpers::force_claim_partnered_control_function(0x92, 0);
	auto otherListener = test_helpers::force_claim_partnered_control_function(0x93, 0);
	CANNetworkManager::CANNetwork.update();

	const auto previousSessionLimit = CANNetworkManager::CANNetwork.get_configuration().get_max_number_transport_protocol_sessions();
	CANNetworkManager::CANNetwork.get_configuration().set_max_number_transport_protocol_sessions(1);

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	// With room for only one observed session, a transfer nobody asked for must not be the one to take it
	send_sniffed_request_to_send(otherTalker, otherListener, 0xEF00);
	send_sniffed_data_transfer(otherTalker, otherListener, 1);

	send_sniffed_request_to_send(talker, listener, 0xFEEC);
	send_sniffed_data_transfer(talker, listener, 1);
	send_sniffed_data_transfer(talker, listener, 2);
	send_sniffed_data_transfer(talker, listener, 3);
	EXPECT_EQ(1, sniffedMessageCount);
	EXPECT_EQ(talker, sniffedMessageSource);

	CANNetworkManager::CANNetwork.get_configuration().set_max_number_transport_protocol_sessions(previousSessionLimit);
	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANNetworkManager::CANNetwork.deactivate_control_function(otherTalker);
	CANNetworkManager::CANNetwork.deactivate_control_function(otherListener);
	CANHardwareInterface::stop();
}
