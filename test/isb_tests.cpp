#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_shortcut_button_interface.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"

using namespace isobus;

static ShortcutButtonInterface::StopAllImplementOperationsState lastCallbackValue = ShortcutButtonInterface::StopAllImplementOperationsState::Error;

static void testCallback(ShortcutButtonInterface::StopAllImplementOperationsState testState)
{
	lastCallbackValue = testState;
}

TEST(ISB_TESTS, ShortcutButtonRxTests)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	auto internalECU = test_helpers::claim_internal_control_function(0x97, 0);
	test_helpers::force_claim_partnered_control_function(0x74, 0);
	// End boilerplate **********************************

	ShortcutButtonInterface interfaceUnderTest(internalECU, false);
	EXPECT_EQ(false, interfaceUnderTest.get_is_initialized());
	interfaceUnderTest.initialize();
	EXPECT_EQ(true, interfaceUnderTest.get_is_initialized());
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::PermitAllImplementsToOperationOn, interfaceUnderTest.get_state());

	// Since we're not acting as a server, make sure the public setter doesn't do anything
	interfaceUnderTest.set_stop_all_implement_operations_state(ShortcutButtonInterface::StopAllImplementOperationsState::StopImplementOperations);
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::PermitAllImplementsToOperationOn, interfaceUnderTest.get_state());

	// Send a valid message to stop
	CANMessageFrame testFrame = {};
	testFrame.identifier = 0x18FD0274;
	testFrame.isExtendedFrame = true;
	testFrame.dataLength = 8;
	testFrame.data[0] = 0xFF;
	testFrame.data[1] = 0xFF;
	testFrame.data[2] = 0xFF;
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0xFF;
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0x00;
	testFrame.data[7] = 0x00;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::StopImplementOperations, interfaceUnderTest.get_state());

	// Set back to permit state
	testFrame.identifier = 0x18FD0274;
	testFrame.data[0] = 0xFF;
	testFrame.data[1] = 0xFF;
	testFrame.data[2] = 0xFF;
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0xFF;
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0x01;
	testFrame.data[7] = 0x01;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::PermitAllImplementsToOperationOn, interfaceUnderTest.get_state());

	// Send increased, incorrect transition count
	testFrame.identifier = 0x18FD0274;
	testFrame.data[0] = 0xFF;
	testFrame.data[1] = 0xFF;
	testFrame.data[2] = 0xFF;
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0xFF;
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0x08;
	testFrame.data[7] = 0x01;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::StopImplementOperations, interfaceUnderTest.get_state());

	// Test reset of state as counter is back to normal
	testFrame.identifier = 0x18FD0274;
	testFrame.data[0] = 0xFF;
	testFrame.data[1] = 0xFF;
	testFrame.data[2] = 0xFF;
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0xFF;
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0x09;
	testFrame.data[7] = 0x01;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::PermitAllImplementsToOperationOn, interfaceUnderTest.get_state());

	// Test reset to zero counter, which should be fine
	testFrame.identifier = 0x18FD0274;
	testFrame.data[0] = 0xFF;
	testFrame.data[1] = 0xFF;
	testFrame.data[2] = 0xFF;
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0xFF;
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0x00;
	testFrame.data[7] = 0x01;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::PermitAllImplementsToOperationOn, interfaceUnderTest.get_state());

	// Test state as counter is back to normal
	testFrame.identifier = 0x18FD0274;
	testFrame.data[0] = 0xFF;
	testFrame.data[1] = 0xFF;
	testFrame.data[2] = 0xFF;
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0xFF;
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0x01;
	testFrame.data[7] = 0x01;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::PermitAllImplementsToOperationOn, interfaceUnderTest.get_state());

	// Set up to test roll over at 255
	testFrame.identifier = 0x18FD0274;
	testFrame.data[0] = 0xFF;
	testFrame.data[1] = 0xFF;
	testFrame.data[2] = 0xFF;
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0xFF;
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0xFE;
	testFrame.data[7] = 0x01;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::StopImplementOperations, interfaceUnderTest.get_state());
	// Go to 255
	testFrame.identifier = 0x18FD0274;
	testFrame.data[0] = 0xFF;
	testFrame.data[1] = 0xFF;
	testFrame.data[2] = 0xFF;
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0xFF;
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0xFF;
	testFrame.data[7] = 0x01;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::PermitAllImplementsToOperationOn, interfaceUnderTest.get_state());

	// Rollover should stay at "permit"
	testFrame.identifier = 0x18FD0274;
	testFrame.data[0] = 0xFF;
	testFrame.data[1] = 0xFF;
	testFrame.data[2] = 0xFF;
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0xFF;
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0x00;
	testFrame.data[7] = 0x01;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::PermitAllImplementsToOperationOn, interfaceUnderTest.get_state());

	interfaceUnderTest.get_stop_all_implement_operations_state_event_dispatcher().add_listener(testCallback);

	// Test callback
	// Set up to test roll over at 255
	testFrame.identifier = 0x18FD0274;
	testFrame.data[0] = 0xFF;
	testFrame.data[1] = 0xFF;
	testFrame.data[2] = 0xFF;
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0xFF;
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0xF0;
	testFrame.data[7] = 0x00;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	interfaceUnderTest.update();
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::StopImplementOperations, interfaceUnderTest.get_state());
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::StopImplementOperations, lastCallbackValue);

	std::this_thread::sleep_for(std::chrono::milliseconds(3100));
	interfaceUnderTest.update();
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::PermitAllImplementsToOperationOn, interfaceUnderTest.get_state());
	CANHardwareInterface::stop();

	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST(ISB_TESTS, ShortcutButtonTxTests)
{
	VirtualCANPlugin serverPlugin;
	serverPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	auto internalECU = test_helpers::claim_internal_control_function(0x98, 0);
	test_helpers::force_claim_partnered_control_function(0x74, 0);

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	// Get the virtual CAN plugin back to a known state
	CANMessageFrame testFrame = {};
	while (!serverPlugin.get_queue_empty())
	{
		serverPlugin.read_frame(testFrame);
	}
	ASSERT_TRUE(serverPlugin.get_queue_empty());
	ASSERT_TRUE(internalECU->get_address_valid());
	// End boilerplate **********************************

	ShortcutButtonInterface interfaceUnderTest(internalECU, true);
	CANNetworkManager::CANNetwork.update();
	interfaceUnderTest.initialize();
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::PermitAllImplementsToOperationOn, interfaceUnderTest.get_state());

	interfaceUnderTest.set_stop_all_implement_operations_state(ShortcutButtonInterface::StopAllImplementOperationsState::StopImplementOperations);
	interfaceUnderTest.update();
	EXPECT_TRUE(serverPlugin.read_frame(testFrame));

	ASSERT_TRUE(testFrame.isExtendedFrame);
	ASSERT_EQ(testFrame.dataLength, 8);
	EXPECT_EQ(CANIdentifier(testFrame.identifier).get_parameter_group_number(), 0xFD02);
	EXPECT_EQ(testFrame.data[0], 0xFF);
	EXPECT_EQ(testFrame.data[1], 0xFF);
	EXPECT_EQ(testFrame.data[2], 0xFF);
	EXPECT_EQ(testFrame.data[3], 0xFF);
	EXPECT_EQ(testFrame.data[4], 0xFF);
	EXPECT_EQ(testFrame.data[5], 0xFF);
	EXPECT_EQ(testFrame.data[6], 0x00);
	EXPECT_EQ(testFrame.data[7], 0xFC);

	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::StopImplementOperations, interfaceUnderTest.get_state());

	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}
