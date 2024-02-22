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
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
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

TEST(HEARTBEAT_TESTS, HeartBeat)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	NAME clientNAME(0);
	clientNAME.set_industry_group(2);
	clientNAME.set_device_class(4);
	clientNAME.set_function_code(static_cast<std::uint8_t>(NAME::Function::EnduranceBraking));
	auto internalECU = test_helpers::claim_internal_control_function(0x41, 0);
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

	ASSERT_TRUE(testPlugin.read_frame(testFrame));
	EXPECT_EQ(testFrame.identifier, 0x0CF0E441);
	EXPECT_EQ(testFrame.dataLength, 1);
	EXPECT_EQ(testFrame.data[0], 251);

	// Wait for the next one. Sequence should now be 0
	std::this_thread::sleep_for(std::chrono::milliseconds(80));
	ASSERT_TRUE(testPlugin.read_frame(testFrame));
	EXPECT_EQ(testFrame.identifier, 0x0CF0E441);
	EXPECT_EQ(testFrame.dataLength, 1);
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
	std::this_thread::sleep_for(std::chrono::milliseconds(400));
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

	// No message should be sent
	EXPECT_FALSE(testPlugin.read_frame(testFrame));

	CANHardwareInterface::stop();
}
