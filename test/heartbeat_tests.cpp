//================================================================================================
/// @file heartbeat_tests.cpp
///
/// @brief Unit tests for the ISOBUS Heartbeat Message interface.
///
/// @author Adrian Del Grosso
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#include <gtest/gtest.h>

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"
#include "helpers/test_fixture.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_heartbeat.hpp"

using namespace isobus;

static bool heartbeat_error_callback_called = false;
static bool new_heartbeat_callback_called = false;
static HeartbeatInterface::HeartBeatError error_type = HeartbeatInterface::HeartBeatError::InvalidSequenceCounter;
void error_callback(HeartbeatInterface::HeartBeatError error, std::shared_ptr<ControlFunction>)
{
	heartbeat_error_callback_called = true;
	error_type = error;
}

void new_callback(std::shared_ptr<ControlFunction>)
{
	new_heartbeat_callback_called = true;
}

static std::uint32_t sequence_counter_errors = 0;

/// Heartbeats left tracked by a previous test time out here, so only sequence counter errors are counted
static void sequence_counter_error_callback(HeartbeatInterface::HeartBeatError error, std::shared_ptr<ControlFunction>)
{
	if (HeartbeatInterface::HeartBeatError::InvalidSequenceCounter == error)
	{
		sequence_counter_errors++;
	}
}

static void receive_heartbeat(std::shared_ptr<ControlFunction> source, std::uint8_t sequenceCounter)
{
	auto frame = test_helpers::create_message_frame_broadcast(3, static_cast<std::uint32_t>(CANLibParameterGroupNumber::HeartbeatMessage), source, { sequenceCounter, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(frame);
	CANNetworkManager::CANNetwork.update();
}

class HeartbeatTest : public AgIsoStackTestFixture
{
protected:
	void TearDown() override
	{
		isobus::CANNetworkManager::CANNetwork.get_heartbeat_interface(0).set_enabled(false);
		AgIsoStackTestFixture::TearDown();
	}
};

class HeartbeatRxTest : public HeartbeatTest
{
protected:
	void SetUp() override
	{
		HeartbeatTest::SetUp();

		auto &heartbeatInterface = CANNetworkManager::CANNetwork.get_heartbeat_interface(0);
		heartbeatInterface.set_enabled(true);
		CANNetworkManager::CANNetwork.update(); // The network manager drops received frames until it has been updated once
		sequence_counter_errors = 0;
		errorListener = heartbeatInterface.get_heartbeat_error_event_dispatcher().add_listener(sequence_counter_error_callback);
	}

	void TearDown() override
	{
		CANNetworkManager::CANNetwork.get_heartbeat_interface(0).get_heartbeat_error_event_dispatcher().remove_listener(errorListener);
		HeartbeatTest::TearDown();
	}

	EventCallbackHandle errorListener = 0;
};

TEST_F(HeartbeatTest, HeartBeat)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	NAME clientNAME(0);
	clientNAME.set_industry_group(2);
	clientNAME.set_device_class(4);
	clientNAME.set_function_code(static_cast<std::uint8_t>(NAME::Function::EnduranceBraking));
	auto internalECU = test_helpers::claim_internal_control_function(0x41, 0, time_source);
	auto partner = test_helpers::force_claim_partnered_control_function(0xF4, 0);

	// Get the virtual CAN plugin back to a known state
	CANMessageFrame testFrame = {};
	while (!testPlugin.get_queue_empty())
	{
		testPlugin.read_frame(testFrame);
	}
	ASSERT_TRUE(testPlugin.get_queue_empty());

	auto &heartbeatInterface = CANNetworkManager::CANNetwork.get_heartbeat_interface(0);

	// Enabled by default
	EXPECT_TRUE(heartbeatInterface.is_enabled());

	// Register the error callback
	heartbeatInterface.get_heartbeat_error_event_dispatcher().add_listener(error_callback);

	// Register the new heartbeat callback
	heartbeatInterface.get_new_tracked_heartbeat_event_dispatcher().add_listener(new_callback);

	heartbeatInterface.request_heartbeat(internalECU, partner);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);

	// Check that the heartbeat request was sent
	ASSERT_TRUE(testPlugin.read_frame(testFrame));
	EXPECT_EQ(testFrame.identifier, 0x18CCF441);
	EXPECT_EQ(testFrame.dataLength, 8);
	EXPECT_EQ(testFrame.data[0], static_cast<std::uint8_t>(61668 & 0xFF));
	EXPECT_EQ(testFrame.data[1], static_cast<std::uint8_t>((61668 >> 8) & 0xFF));
	EXPECT_EQ(testFrame.data[2], static_cast<std::uint8_t>((61668 >> 16) & 0xFF));
	EXPECT_EQ(testFrame.data[3], static_cast<std::uint8_t>(100 & 0xFF));
	EXPECT_EQ(testFrame.data[4], static_cast<std::uint8_t>((100 >> 8) & 0xFF));
	EXPECT_EQ(testFrame.data[5], 0xFF);
	EXPECT_EQ(testFrame.data[6], 0xFF);
	EXPECT_EQ(testFrame.data[7], 0xFF);

	// Send a request for the heartbeat
	testFrame.identifier = 0x18CC41F4;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);

	ASSERT_TRUE(testPlugin.read_frame(testFrame));
	EXPECT_EQ(testFrame.identifier, 0x0CF0E441);
	EXPECT_EQ(testFrame.dataLength, 8);
	EXPECT_EQ(testFrame.data[0], 251);
	EXPECT_EQ(testFrame.data[1], 0xFF);
	EXPECT_EQ(testFrame.data[2], 0xFF);
	EXPECT_EQ(testFrame.data[3], 0xFF);
	EXPECT_EQ(testFrame.data[4], 0xFF);
	EXPECT_EQ(testFrame.data[5], 0xFF);
	EXPECT_EQ(testFrame.data[6], 0xFF);
	EXPECT_EQ(testFrame.data[7], 0xFF);

	// Wait for the next one. Sequence should now be 0
	time_source.update_for_ms(101);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);
	ASSERT_TRUE(testPlugin.read_frame(testFrame));
	EXPECT_EQ(testFrame.identifier, 0x0CF0E441);
	EXPECT_EQ(testFrame.dataLength, 8);
	EXPECT_EQ(testFrame.data[0], 0);

	// Supply a heartbeat
	EXPECT_FALSE(new_heartbeat_callback_called);
	new_heartbeat_callback_called = false;
	testFrame.identifier = 0x0CF0E4F4;
	testFrame.dataLength = 1;
	testFrame.data[0] = 251;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_TRUE(new_heartbeat_callback_called);

	// Wait to ensure that the heartbeat times out
	EXPECT_FALSE(heartbeat_error_callback_called);
	time_source.update_for_ms(400);
	CANNetworkManager::CANNetwork.update();
	EXPECT_TRUE(heartbeat_error_callback_called);
	EXPECT_EQ(error_type, HeartbeatInterface::HeartBeatError::TimedOut);

	// Get the virtual CAN plugin back to a known state
	while (!testPlugin.get_queue_empty())
	{
		testPlugin.read_frame(testFrame);
	}
	ASSERT_TRUE(testPlugin.get_queue_empty());

	// Disable the heartbeat interface
	heartbeatInterface.set_enabled(false);
	EXPECT_FALSE(heartbeatInterface.is_enabled());
	time_source.update_for_ms(5);

	// No message should be sent
	EXPECT_FALSE(testPlugin.read_frame(testFrame));

	CANHardwareInterface::stop();
}

TEST_F(HeartbeatRxTest, RxInitialSequenceCounterThenZero)
{
	auto partner = test_helpers::force_claim_partnered_control_function(0x80, 0);

	// A CF sends 251 once on initialization, then restarts the range at 0
	receive_heartbeat(partner, 251);
	receive_heartbeat(partner, 0);

	EXPECT_EQ(sequence_counter_errors, 0);
}

TEST_F(HeartbeatRxTest, RxSequenceCounterWrap)
{
	auto partner = test_helpers::force_claim_partnered_control_function(0x81, 0);

	receive_heartbeat(partner, 249);
	receive_heartbeat(partner, 250);
	receive_heartbeat(partner, 0);
	receive_heartbeat(partner, 1);

	EXPECT_EQ(sequence_counter_errors, 0);
}

TEST_F(HeartbeatRxTest, RxPerControlFunctionSequenceCounters)
{
	auto firstPartner = test_helpers::force_claim_partnered_control_function(0x82, 0);
	auto secondPartner = test_helpers::force_claim_partnered_control_function(0x83, 0);

	receive_heartbeat(firstPartner, 10);
	receive_heartbeat(secondPartner, 20);
	receive_heartbeat(firstPartner, 11);
	receive_heartbeat(secondPartner, 21);
	receive_heartbeat(firstPartner, 12);
	receive_heartbeat(secondPartner, 22);

	EXPECT_EQ(sequence_counter_errors, 0);
}

TEST_F(HeartbeatRxTest, RxInvalidSequenceCounterStillDetected)
{
	auto partner = test_helpers::force_claim_partnered_control_function(0x84, 0);

	receive_heartbeat(partner, 5);
	receive_heartbeat(partner, 6);
	EXPECT_EQ(sequence_counter_errors, 0);

	receive_heartbeat(partner, 9);
	EXPECT_EQ(sequence_counter_errors, 1);

	receive_heartbeat(partner, 9);
	EXPECT_EQ(sequence_counter_errors, 2);

	receive_heartbeat(partner, 250);
	EXPECT_EQ(sequence_counter_errors, 3);
}
