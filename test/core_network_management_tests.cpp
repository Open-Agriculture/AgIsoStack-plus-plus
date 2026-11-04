#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_message_frame.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"
#include "helpers/test_fixture.hpp"

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

class CoreTest : public AgIsoStackTestFixture
{
	// Wrapper to give tests a more meaningful name - no content.
};

static CANMessageFrame create_address_claim_frame(std::uint8_t channel, std::uint8_t address, const NAME &name)
{
	CANMessageFrame addressClaim = {};
	addressClaim.channel = channel;
	addressClaim.identifier = 0x18EEFF00 | address;
	addressClaim.isExtendedFrame = true;
	addressClaim.dataLength = 8;

	const std::uint64_t fullName = name.get_full_name();
	for (std::uint8_t byteIndex = 0; byteIndex < addressClaim.dataLength; byteIndex++)
	{
		addressClaim.data[byteIndex] = static_cast<std::uint8_t>(fullName >> (8 * byteIndex));
	}
	return addressClaim;
}

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

	// The partner doesn't claim again, but one missed request is not enough to take it offline
	time_source.update_for_ms(2000);
	CANNetworkManager::CANNetwork.update();

	EXPECT_TRUE(testPartner->get_address_valid());
	EXPECT_FALSE(wasTestStateCallbackHit && (testControlFunction == testPartner));
	wasTestStateCallbackHit = false;

	// It misses the next request as well, so now it should be invalid.
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_pgn_request(
	  0xEE00, // Address Claim PGN
	  nullptr,
	  nullptr));
	CANNetworkManager::CANNetwork.update();
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

TEST_F(CoreTest, RestoredControlFunctionSurvivesTheNextPrune)
{
	constexpr std::uint8_t TEST_CHANNEL = 3;
	constexpr std::uint8_t DEPARTED_ADDRESS = 0x98;
	constexpr std::uint8_t RETURNING_ADDRESS = 0x99;
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(250); // A request timestamp of 0 would mean "no request pending"

	const NAME departedName = test_helpers::find_available_name(TEST_CHANNEL);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, DEPARTED_ADDRESS, departedName));
	const NAME returningName = test_helpers::find_available_name(TEST_CHANNEL);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, RETURNING_ADDRESS, returningName));
	CANNetworkManager::CANNetwork.update();
	auto returningCF = CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, RETURNING_ADDRESS);
	ASSERT_NE(nullptr, returningCF);
	ASSERT_EQ(returningName, returningCF->get_NAME());

	// Neither CF answers two requests for address claim in a row, so both are pruned, the departed one first
	CANMessageFrame request = test_helpers::create_message_frame_pgn_request(0xEE00, nullptr, nullptr);
	request.channel = TEST_CHANNEL;
	for (std::uint8_t i = 0; i < 2; i++)
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
		CANNetworkManager::CANNetwork.update();
		time_source.update_for_ms(1500);
		CANNetworkManager::CANNetwork.update();
	}
	ASSERT_FALSE(returningCF->get_address_valid());

	// The returning CF answers the next request, which restores it to the table
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, RETURNING_ADDRESS, returningName));
	CANNetworkManager::CANNetwork.update();
	ASSERT_TRUE(returningCF->get_address_valid());

	// It claimed after that request, so it must survive the prune when the request's timer runs out
	time_source.update_for_ms(1500);
	CANNetworkManager::CANNetwork.update();
	EXPECT_TRUE(returningCF->get_address_valid());
	EXPECT_EQ(returningCF, CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, RETURNING_ADDRESS));

	// Restored with a clean record: one missed request after that does not take it offline again
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(1500);
	CANNetworkManager::CANNetwork.update();
	EXPECT_TRUE(returningCF->get_address_valid());
}

TEST_F(CoreTest, ControlFunctionThatSkipsOneRequestStaysOnline)
{
	constexpr std::uint8_t TEST_CHANNEL = 3;
	constexpr std::uint8_t SKIPPING_ADDRESS = 0x9B;
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(250); // A request timestamp of 0 would mean "no request pending"

	const NAME skippingName = test_helpers::find_available_name(TEST_CHANNEL);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, SKIPPING_ADDRESS, skippingName));
	CANNetworkManager::CANNetwork.update();
	auto skippingCF = CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, SKIPPING_ADDRESS);
	ASSERT_NE(nullptr, skippingCF);

	CANMessageFrame request = test_helpers::create_message_frame_pgn_request(0xEE00, nullptr, nullptr);
	request.channel = TEST_CHANNEL;
	auto missRequest = [&]() {
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
		CANNetworkManager::CANNetwork.update();
		time_source.update_for_ms(1500);
		CANNetworkManager::CANNetwork.update();
	};
	auto answerRequest = [&]() {
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, SKIPPING_ADDRESS, skippingName));
		CANNetworkManager::CANNetwork.update();
		time_source.update_for_ms(1500);
		CANNetworkManager::CANNetwork.update();
	};

	// Skipping every other request never takes it offline: each claim clears the missed request
	missRequest();
	EXPECT_TRUE(skippingCF->get_address_valid());
	answerRequest();
	EXPECT_TRUE(skippingCF->get_address_valid());
	missRequest();
	EXPECT_TRUE(skippingCF->get_address_valid());
	answerRequest();
	EXPECT_TRUE(skippingCF->get_address_valid());

	// Two misses in a row do
	missRequest();
	EXPECT_TRUE(skippingCF->get_address_valid());
	missRequest();
	EXPECT_FALSE(skippingCF->get_address_valid());
	EXPECT_EQ(nullptr, CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, SKIPPING_ADDRESS));
}

TEST_F(CoreTest, AnswerWithinTheResponseTimeoutCounts)
{
	constexpr std::uint8_t TEST_CHANNEL = 3;
	constexpr std::uint8_t SLOW_ADDRESS = 0x9C;
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(250); // A request timestamp of 0 would mean "no request pending"

	const NAME slowName = test_helpers::find_available_name(TEST_CHANNEL);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, SLOW_ADDRESS, slowName));
	CANNetworkManager::CANNetwork.update();
	auto slowCF = CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, SLOW_ADDRESS);
	ASSERT_NE(nullptr, slowCF);

	CANMessageFrame request = test_helpers::create_message_frame_pgn_request(0xEE00, nullptr, nullptr);
	request.channel = TEST_CHANNEL;

	// One missed request
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(1500);
	CANNetworkManager::CANNetwork.update();
	ASSERT_TRUE(slowCF->get_address_valid());

	// The next one is answered after 1 s: later than the 755 ms address claim timing, but within
	// the 1.25 s a CF waits for a response (ISO 11783-3 T3), so it is not a second miss
	wasTestStateCallbackHit = false;
	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(test_control_function_state_callback);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(1000);
	CANNetworkManager::CANNetwork.update();
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, SLOW_ADDRESS, slowName));
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(500);
	CANNetworkManager::CANNetwork.update();

	EXPECT_TRUE(slowCF->get_address_valid());
	EXPECT_FALSE(wasTestStateCallbackHit && (testControlFunction == slowCF));
	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(test_control_function_state_callback);
	testControlFunction.reset();
}

TEST_F(CoreTest, PartnerCreatedAfterItsDeviceAnsweredARequestSurvivesThePrune)
{
	constexpr std::uint8_t TEST_CHANNEL = 3;
	constexpr std::uint8_t CLAIMED_ADDRESS = 0x9A;
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(250); // A request timestamp of 0 would mean "no request pending"

	const NAME claimedName = test_helpers::find_available_name(TEST_CHANNEL);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, CLAIMED_ADDRESS, claimedName));
	CANNetworkManager::CANNetwork.update();

	// The device answers a request for address claim before the application creates a partner for it
	CANMessageFrame request = test_helpers::create_message_frame_pgn_request(0xEE00, nullptr, nullptr);
	request.channel = TEST_CHANNEL;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, CLAIMED_ADDRESS, claimedName));
	CANNetworkManager::CANNetwork.update();

	const std::vector<NAMEFilter> nameFilters{
		NAMEFilter(NAME::NAMEParameters::IdentityNumber, claimedName.get_identity_number()),
		NAMEFilter(NAME::NAMEParameters::ManufacturerCode, claimedName.get_manufacturer_code()),
		NAMEFilter(NAME::NAMEParameters::FunctionCode, claimedName.get_function_code()),
		NAMEFilter(NAME::NAMEParameters::EcuInstance, claimedName.get_ecu_instance())
	};
	auto partner = CANNetworkManager::CANNetwork.create_partnered_control_function(TEST_CHANNEL, nameFilters);
	ASSERT_TRUE(partner->get_address_valid());

	// The device claimed after that request, so its partner must survive the prune when the request's timer runs out
	time_source.update_for_ms(1000);
	CANNetworkManager::CANNetwork.update();
	EXPECT_TRUE(partner->get_address_valid());
	EXPECT_EQ(partner, CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, CLAIMED_ADDRESS));

	CANNetworkManager::CANNetwork.deactivate_control_function(partner);
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

TEST_F(CoreTest, DeactivatingAPartnerWhileMessagesAreProcessed)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x8C, 0, time_source);
	auto firstPartner = test_helpers::force_claim_partnered_control_function(0x8D, 0);
	CANHardwareInterface::stop(); // From here the thread below is the only one that updates the stack
	const NAME partnerName = firstPartner->get_NAME();
	const std::vector<NAMEFilter> nameFilters = {
		NAMEFilter(NAME::NAMEParameters::IdentityNumber, partnerName.get_identity_number()),
		NAMEFilter(NAME::NAMEParameters::ManufacturerCode, partnerName.get_manufacturer_code()),
		NAMEFilter(NAME::NAMEParameters::FunctionCode, partnerName.get_function_code())
	};
	// Leaves an external control function with that NAME at 0x8D, for the partners below to match
	CANNetworkManager::CANNetwork.deactivate_control_function(firstPartner);

	// The stack runs on one thread, receiving messages from 0x8D for our control function, while the
	// application creates and deactivates a partner for 0x8D on another: the receive path walks the
	// partner list and reads the address table that deactivate_control_function() changes.
	std::atomic_bool done = { false };
	std::thread stack([&]() {
		const auto source = test_helpers::create_mock_control_function(0x8D);
		while (!done)
		{
			CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(6, 0xEF00, internalECU, source, { 1, 2, 3, 4, 5, 6, 7, 8 }));
			CANNetworkManager::CANNetwork.update();
		}
	});
	for (int round = 0; round < 2000; round++)
	{
		auto partner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, nameFilters);
		std::this_thread::yield();
		CANNetworkManager::CANNetwork.deactivate_control_function(partner);
	}
	done = true;
	stack.join();

	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANHardwareInterface::set_number_of_can_channels(0);
}

// A callback that deadlocks never returns, so the work runs on a thread the test can abandon.
static bool returns_within_a_second(std::function<void()> work)
{
	auto finished = std::make_shared<std::promise<void>>();
	std::future<void> result = finished->get_future();
	std::thread([work, finished]() {
		work();
		finished->set_value();
	}).detach();
	return std::future_status::ready == result.wait_for(std::chrono::seconds(1));
}

static std::uint32_t laterCallbackHits = 0;

static void later_callback(const CANMessage &, void *)
{
	laterCallbackHits++;
}

static void reregistering_any_control_function_callback(const CANMessage &, void *)
{
	CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(0xFF00, later_callback, nullptr);
	CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(0xFF00, reregistering_any_control_function_callback, nullptr);
}

TEST_F(CoreTest, AnyControlFunctionCallbackCanChangeTheCallbacks)
{
	laterCallbackHits = 0;
	CANNetworkManager::CANNetwork.update(); // Make sure the network manager is initialized
	auto source = test_helpers::force_claim_partnered_control_function(0x90, 0);
	CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(0xFF00, reregistering_any_control_function_callback, nullptr);
	const CANMessageFrame frame = test_helpers::create_message_frame_broadcast(6, 0xFF00, source, { 1, 2, 3, 4, 5, 6, 7, 8 });

	ASSERT_TRUE(returns_within_a_second([frame]() {
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(frame);
		CANNetworkManager::CANNetwork.update();
	}));
	EXPECT_EQ(0u, laterCallbackHits);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(frame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1u, laterCallbackHits);

	CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(0xFF00, later_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(source);
}

static std::uint32_t removingStateCallbackHits = 0;

static void removing_state_callback(std::shared_ptr<ControlFunction>, ControlFunctionState)
{
	removingStateCallbackHits++;
	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(removing_state_callback);
}

TEST_F(CoreTest, ControlFunctionStateCallbackCanRemoveItself)
{
	removingStateCallbackHits = 0;
	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(removing_state_callback);
	auto partners = std::make_shared<std::vector<std::shared_ptr<PartneredControlFunction>>>();

	ASSERT_TRUE(returns_within_a_second([partners]() {
		partners->push_back(test_helpers::force_claim_partnered_control_function(0x92, 0));
	}));
	EXPECT_EQ(1u, removingStateCallbackHits);

	partners->push_back(test_helpers::force_claim_partnered_control_function(0x93, 0));
	EXPECT_EQ(1u, removingStateCallbackHits);

	for (const auto &partner : *partners)
	{
		CANNetworkManager::CANNetwork.deactivate_control_function(partner);
	}
}

static bool later_pgn_request_callback(std::uint32_t, std::shared_ptr<ControlFunction>, bool &, AcknowledgementType &, void *)
{
	laterCallbackHits++;
	return false;
}

static bool reregistering_pgn_request_callback(std::uint32_t, std::shared_ptr<ControlFunction>, bool &, AcknowledgementType &, void *parent)
{
	auto protocol = static_cast<ParameterGroupNumberRequestProtocol *>(parent);
	protocol->register_pgn_request_callback(0xFF02, later_pgn_request_callback, nullptr);
	protocol->remove_pgn_request_callback(0xFF02, reregistering_pgn_request_callback, parent);
	return false;
}

TEST_F(CoreTest, PgnRequestCallbackCanChangeTheCallbacks)
{
	laterCallbackHits = 0;
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x94, 0, time_source);
	auto requester = test_helpers::force_claim_partnered_control_function(0x95, 0);
	CANHardwareInterface::stop();
	auto protocol = internalECU->get_pgn_request_protocol().lock();
	ASSERT_NE(nullptr, protocol);
	protocol->register_pgn_request_callback(0xFF02, reregistering_pgn_request_callback, protocol.get());
	const CANMessageFrame request = test_helpers::create_message_frame_pgn_request(0xFF02, requester, internalECU);

	ASSERT_TRUE(returns_within_a_second([request]() {
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
		CANNetworkManager::CANNetwork.update();
	}));
	EXPECT_EQ(0u, laterCallbackHits);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1u, laterCallbackHits);

	protocol->remove_pgn_request_callback(0xFF02, later_pgn_request_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(requester);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANHardwareInterface::set_number_of_can_channels(0);
}

static std::shared_ptr<InternalControlFunction> otherInternalControlFunction = nullptr;

static bool deactivating_pgn_request_callback(std::uint32_t, std::shared_ptr<ControlFunction>, bool &, AcknowledgementType &, void *)
{
	// Destroys its ParameterGroupNumberRequestProtocol, which removes that protocol's network manager callbacks
	CANNetworkManager::CANNetwork.deactivate_control_function(otherInternalControlFunction);
	return false;
}

TEST_F(CoreTest, PgnRequestCallbackCanDeactivateAnotherInternalControlFunction)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x96, 0, time_source);
	auto requester = test_helpers::force_claim_partnered_control_function(0x97, 0);
	CANHardwareInterface::stop();
	otherInternalControlFunction = CANNetworkManager::CANNetwork.create_internal_control_function(test_helpers::find_available_name(0), 0, 0x98);
	auto protocol = internalECU->get_pgn_request_protocol().lock();
	ASSERT_NE(nullptr, protocol);
	protocol->register_pgn_request_callback(0xFF01, deactivating_pgn_request_callback, nullptr);
	const CANMessageFrame request = test_helpers::create_message_frame_pgn_request(0xFF01, requester, internalECU);

	ASSERT_TRUE(returns_within_a_second([request]() {
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
		CANNetworkManager::CANNetwork.update();
	}));
	EXPECT_EQ(nullptr, otherInternalControlFunction->get_pgn_request_protocol().lock());

	protocol->remove_pgn_request_callback(0xFF01, deactivating_pgn_request_callback, nullptr);
	otherInternalControlFunction = nullptr;
	CANNetworkManager::CANNetwork.deactivate_control_function(requester);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANHardwareInterface::set_number_of_can_channels(0);
}

static bool later_repetition_rate_callback(std::uint32_t, std::shared_ptr<ControlFunction>, std::shared_ptr<ControlFunction>, std::uint32_t, void *)
{
	laterCallbackHits++;
	return false;
}

static bool reregistering_repetition_rate_callback(std::uint32_t, std::shared_ptr<ControlFunction>, std::shared_ptr<ControlFunction>, std::uint32_t, void *parent)
{
	auto protocol = static_cast<ParameterGroupNumberRequestProtocol *>(parent);
	protocol->register_request_for_repetition_rate_callback(0xFF04, later_repetition_rate_callback, nullptr);
	protocol->remove_request_for_repetition_rate_callback(0xFF04, reregistering_repetition_rate_callback, parent);
	return false;
}

TEST_F(CoreTest, RepetitionRateCallbackCanChangeTheCallbacks)
{
	laterCallbackHits = 0;
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x9C, 0, time_source);
	auto requester = test_helpers::force_claim_partnered_control_function(0x9D, 0);
	CANHardwareInterface::stop();
	auto protocol = internalECU->get_pgn_request_protocol().lock();
	ASSERT_NE(nullptr, protocol);
	protocol->register_request_for_repetition_rate_callback(0xFF04, reregistering_repetition_rate_callback, protocol.get());
	const CANMessageFrame request = test_helpers::create_message_frame(6,
	                                                                   static_cast<std::uint32_t>(CANLibParameterGroupNumber::RequestForRepetitionRate),
	                                                                   internalECU,
	                                                                   requester,
	                                                                   { 0x04, 0xFF, 0x00, 0x64, 0x00, 0xFF, 0xFF, 0xFF });

	ASSERT_TRUE(returns_within_a_second([request]() {
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
		CANNetworkManager::CANNetwork.update();
	}));
	EXPECT_EQ(0u, laterCallbackHits);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1u, laterCallbackHits);

	protocol->remove_request_for_repetition_rate_callback(0xFF04, later_repetition_rate_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(requester);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANHardwareInterface::set_number_of_can_channels(0);
}

static CANMessageFrame address_claim_frame(std::uint8_t address, NAME name)
{
	const std::uint64_t fullName = name.get_full_name();
	return test_helpers::create_message_frame_raw(0x18EEFF00 | address,
	                                              { static_cast<std::uint8_t>(fullName),
	                                                static_cast<std::uint8_t>(fullName >> 8),
	                                                static_cast<std::uint8_t>(fullName >> 16),
	                                                static_cast<std::uint8_t>(fullName >> 24),
	                                                static_cast<std::uint8_t>(fullName >> 32),
	                                                static_cast<std::uint8_t>(fullName >> 40),
	                                                static_cast<std::uint8_t>(fullName >> 48),
	                                                static_cast<std::uint8_t>(fullName >> 56) });
}

static void deactivating_partner_state_callback(std::shared_ptr<ControlFunction> controlFunction, ControlFunctionState state)
{
	if ((ControlFunctionState::Online == state) && (ControlFunction::Type::Partnered == controlFunction->get_type()))
	{
		CANNetworkManager::CANNetwork.deactivate_control_function(std::static_pointer_cast<PartneredControlFunction>(controlFunction));
	}
}

TEST_F(CoreTest, PartnerOnlineCallbackCanDeactivateThePartner)
{
	CANNetworkManager::CANNetwork.update();
	const NAME name = test_helpers::find_available_name(0);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(address_claim_frame(0x99, name));
	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(deactivating_partner_state_callback);
	auto partner = std::make_shared<std::shared_ptr<PartneredControlFunction>>();

	ASSERT_TRUE(returns_within_a_second([name, partner]() {
		*partner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, { NAMEFilter(NAME::NAMEParameters::IdentityNumber, name.get_identity_number()) });
		CANNetworkManager::CANNetwork.update();
	}));
	auto partners = CANNetworkManager::CANNetwork.get_partnered_control_functions();
	EXPECT_EQ(partners.end(), std::find(partners.begin(), partners.end(), *partner));

	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(deactivating_partner_state_callback);
}

static std::mutex applicationMutex;
static std::atomic_bool applicationLockingCallbackEntered{ false };

static void application_locking_state_callback(std::shared_ptr<ControlFunction>, ControlFunctionState)
{
	applicationLockingCallbackEntered = true;
	const std::lock_guard<std::mutex> applicationLock(applicationMutex);
}

TEST_F(CoreTest, StateCallbackCanWaitForAThreadThatUsesTheNetworkManager)
{
	applicationLockingCallbackEntered = false;
	CANNetworkManager::CANNetwork.update();
	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(application_locking_state_callback);
	const CANMessageFrame addressClaim = address_claim_frame(0x9A, test_helpers::find_available_name(0));

	ASSERT_TRUE(returns_within_a_second([addressClaim]() {
		std::unique_lock<std::mutex> applicationLock(applicationMutex);
		std::thread stackThread([addressClaim]() {
			CANNetworkManager::CANNetwork.process_receive_can_message_frame(addressClaim);
		});
		while (!applicationLockingCallbackEntered)
		{
			std::this_thread::yield();
		}
		CANNetworkManager::CANNetwork.get_partnered_control_functions();
		applicationLock.unlock();
		stackThread.join();
	}));

	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(application_locking_state_callback);
}

static std::atomic_bool blockingCallbackStarted{ false };
static std::atomic_bool blockingCallbackReleased{ false };
static std::atomic<std::uint32_t> blockingCallbackHits{ 0 };

static void blocking_callback(const CANMessage &, void *)
{
	blockingCallbackHits++;
	blockingCallbackStarted = true;
	while (!blockingCallbackReleased)
	{
		std::this_thread::yield();
	}
}

TEST_F(CoreTest, RemovingACallbackDoesNotWaitForACallInProgressOnAnotherThread)
{
	blockingCallbackStarted = false;
	blockingCallbackReleased = false;
	blockingCallbackHits = 0;
	CANNetworkManager::CANNetwork.update();
	auto source = test_helpers::force_claim_partnered_control_function(0x9B, 0);
	CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(0xFF03, blocking_callback, nullptr);
	const CANMessageFrame frame = test_helpers::create_message_frame_broadcast(6, 0xFF03, source, { 1, 2, 3, 4, 5, 6, 7, 8 });

	std::thread stackThread([frame]() {
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(frame);
		CANNetworkManager::CANNetwork.update();
	});
	const bool started = returns_within_a_second([]() {
		while (!blockingCallbackStarted)
		{
			std::this_thread::yield();
		}
	});
	const bool removedWhileTheCallbackRan = returns_within_a_second([]() {
		CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(0xFF03, blocking_callback, nullptr);
	});
	blockingCallbackReleased = true;
	stackThread.join();
	ASSERT_TRUE(started);
	EXPECT_TRUE(removedWhileTheCallbackRan);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(frame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1u, blockingCallbackHits);

	CANNetworkManager::CANNetwork.deactivate_control_function(source);
}

static std::shared_ptr<InternalControlFunction> selfDeactivatingInternalControlFunction = nullptr;

static bool self_deactivating_pgn_request_callback(std::uint32_t, std::shared_ptr<ControlFunction>, bool &shouldAck, AcknowledgementType &ackType, void *)
{
	// Destroys the ParameterGroupNumberRequestProtocol that is running this callback, which then sends the ACK
	CANNetworkManager::CANNetwork.deactivate_control_function(selfDeactivatingInternalControlFunction);
	shouldAck = true;
	ackType = AcknowledgementType::Positive;
	return true;
}

TEST_F(CoreTest, PgnRequestCallbackCanDeactivateItsOwnInternalControlFunction)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	selfDeactivatingInternalControlFunction = test_helpers::claim_internal_control_function(0x96, 0, time_source);
	auto requester = test_helpers::force_claim_partnered_control_function(0x97, 0);
	CANHardwareInterface::stop();
	auto protocol = selfDeactivatingInternalControlFunction->get_pgn_request_protocol().lock();
	ASSERT_NE(nullptr, protocol);
	protocol->register_pgn_request_callback(0xFF05, self_deactivating_pgn_request_callback, nullptr);
	std::weak_ptr<ParameterGroupNumberRequestProtocol> protocolLifetime = protocol;
	protocol = nullptr;
	const CANMessageFrame request = test_helpers::create_message_frame_pgn_request(0xFF05, requester, selfDeactivatingInternalControlFunction);

	ASSERT_TRUE(returns_within_a_second([request]() {
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
		CANNetworkManager::CANNetwork.update();
	}));
	EXPECT_TRUE(protocolLifetime.expired());

	selfDeactivatingInternalControlFunction = nullptr;
	CANNetworkManager::CANNetwork.deactivate_control_function(requester);
	CANHardwareInterface::set_number_of_can_channels(0);
}

static std::uint32_t standaloneRequestHits = 0;

static bool counting_pgn_request_callback(std::uint32_t, std::shared_ptr<ControlFunction>, bool &, AcknowledgementType &, void *)
{
	standaloneRequestHits++;
	return false;
}

TEST_F(CoreTest, PgnRequestProtocolOfAStandaloneInternalControlFunctionHandlesRequests)
{
	standaloneRequestHits = 0;
	CANNetworkManager::CANNetwork.update();
	auto requester = test_helpers::force_claim_partnered_control_function(0x94, 0);
	auto internalControlFunction = std::make_shared<InternalControlFunction>(test_helpers::find_available_name(0), 0x95, 0);
	ParameterGroupNumberRequestProtocol protocol(internalControlFunction);
	protocol.register_pgn_request_callback(0xFF06, counting_pgn_request_callback, nullptr);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_pgn_request(0xFF06, requester, nullptr));
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1u, standaloneRequestHits);

	CANNetworkManager::CANNetwork.deactivate_control_function(requester);
}

static std::shared_ptr<PartneredControlFunction> deactivatingPartner = nullptr;
static std::shared_ptr<PartneredControlFunction> partnerToDeactivate = nullptr;
static std::uint32_t deactivatedPartnerOnlineCalls = 0;

static void deactivating_other_partner_state_callback(std::shared_ptr<ControlFunction> controlFunction, ControlFunctionState state)
{
	if ((ControlFunctionState::Online == state) && (deactivatingPartner == controlFunction))
	{
		CANNetworkManager::CANNetwork.deactivate_control_function(partnerToDeactivate);
	}
	else if ((ControlFunctionState::Online == state) && (partnerToDeactivate == controlFunction))
	{
		deactivatedPartnerOnlineCalls++;
	}
}

TEST_F(CoreTest, PartnerDeactivatedByAnEarlierOnlineCallbackIsNotReportedOnline)
{
	deactivatedPartnerOnlineCalls = 0;
	CANNetworkManager::CANNetwork.update();
	const NAME firstName = test_helpers::find_available_name(0);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(address_claim_frame(0x92, firstName));
	const NAME secondName = test_helpers::find_available_name(0);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(address_claim_frame(0x93, secondName));
	deactivatingPartner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, { NAMEFilter(NAME::NAMEParameters::IdentityNumber, firstName.get_identity_number()) });
	partnerToDeactivate = CANNetworkManager::CANNetwork.create_partnered_control_function(0, { NAMEFilter(NAME::NAMEParameters::IdentityNumber, secondName.get_identity_number()) });
	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(deactivating_other_partner_state_callback);

	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(0u, deactivatedPartnerOnlineCalls);

	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(deactivating_other_partner_state_callback);
	CANNetworkManager::CANNetwork.deactivate_control_function(deactivatingPartner);
	deactivatingPartner = nullptr;
	partnerToDeactivate = nullptr;
}
