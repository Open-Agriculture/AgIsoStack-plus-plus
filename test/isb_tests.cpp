#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_shortcut_button_interface.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"
#include "helpers/test_fixture.hpp"

#include <vector>

using namespace isobus;

using State = ShortcutButtonInterface::StopAllImplementOperationsState;

static ShortcutButtonInterface::StopAllImplementOperationsState lastCallbackValue = ShortcutButtonInterface::StopAllImplementOperationsState::Error;

static void testCallback(ShortcutButtonInterface::StopAllImplementOperationsState testState)
{
	lastCallbackValue = testState;
}

static void receive_isb(std::uint8_t sourceAddress, std::uint8_t transitionCount, ShortcutButtonInterface::StopAllImplementOperationsState state)
{
	const std::uint32_t identifier = (3U << 26) | (0xFD02U << 8) | sourceAddress;
	auto frame = test_helpers::create_message_frame_raw(identifier, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, transitionCount, static_cast<std::uint8_t>(0xFC | static_cast<std::uint8_t>(state)) });
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(frame);
	CANNetworkManager::CANNetwork.update();
}

static void pump_isb_frames(ShortcutButtonInterface &interfaceUnderTest, test_helpers::TestTimeSource &timeSource, VirtualCANPlugin &observer, std::uint32_t durationMs, std::vector<CANMessageFrame> &isbFrames)
{
	CANMessageFrame frame = {};

	for (std::uint32_t i = 0; i < durationMs; i++)
	{
		interfaceUnderTest.update();
		timeSource.update_for_ms(1);

		while (!observer.get_queue_empty())
		{
			observer.read_frame(frame);
			if (0xFD02 == CANIdentifier(frame.identifier).get_parameter_group_number())
			{
				isbFrames.push_back(frame);
			}
		}
	}
}

class IsobusShortcutButtonTest : public AgIsoStackTestFixture
{
	// Wrapper to give tests a more meaningful name - no content.
};

TEST_F(IsobusShortcutButtonTest, ShortcutButtonRxTests)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x97, 0, time_source);
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
	testFrame.data[6] = 0x00;
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
	testFrame.data[6] = 0x08;
	testFrame.data[7] = 0x01;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::PermitAllImplementsToOperationOn, interfaceUnderTest.get_state());

	// A lower counter is also an error
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
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::StopImplementOperations, interfaceUnderTest.get_state());

	// Test state as counter is back to normal
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

	// Set up to test roll over at 255
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

	// A stop that wraps the counter from 255 to 0 is a valid transition
	testFrame.identifier = 0x18FD0274;
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

	time_source.simulate_delay_ms(3100);
	interfaceUnderTest.update();
	EXPECT_EQ(ShortcutButtonInterface::StopAllImplementOperationsState::PermitAllImplementsToOperationOn, interfaceUnderTest.get_state());
	CANHardwareInterface::stop();

	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(IsobusShortcutButtonTest, ShortcutButtonTxTests)
{
	VirtualCANPlugin serverPlugin;
	serverPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x98, 0, time_source);
	test_helpers::force_claim_partnered_control_function(0x74, 0);

	time_source.update_for_ms(50);

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
	time_source.update_for_ms(5);
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

TEST_F(IsobusShortcutButtonTest, ShortcutButtonNotAvailableSenderCountIsIgnored)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x97, 0, time_source);
	test_helpers::force_claim_partnered_control_function(0x75, 0);
	// End boilerplate **********************************

	ShortcutButtonInterface interfaceUnderTest(internalECU, false);
	interfaceUnderTest.initialize();
	std::vector<State> events;
	interfaceUnderTest.get_stop_all_implement_operations_state_event_dispatcher().add_listener([&events](State state) {
		events.push_back(state);
	});

	receive_isb(0x75, 10, State::NotAvailable);
	receive_isb(0x75, 11, State::NotAvailable);
	receive_isb(0x75, 12, State::NotAvailable);
	receive_isb(0x75, 12, State::PermitAllImplementsToOperationOn);
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest.get_state());
	EXPECT_TRUE(events.empty());

	receive_isb(0x75, 13, State::StopImplementOperations);
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest.get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations }), events);

	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(IsobusShortcutButtonTest, ShortcutButtonNotAvailableAfterPermitIsChecked)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x97, 0, time_source);
	test_helpers::force_claim_partnered_control_function(0x76, 0);
	// End boilerplate **********************************

	ShortcutButtonInterface interfaceUnderTest(internalECU, false);
	interfaceUnderTest.initialize();
	std::vector<State> events;
	interfaceUnderTest.get_stop_all_implement_operations_state_event_dispatcher().add_listener([&events](State state) {
		events.push_back(state);
	});

	receive_isb(0x76, 10, State::PermitAllImplementsToOperationOn);
	receive_isb(0x76, 11, State::NotAvailable);
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest.get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations }), events);

	receive_isb(0x76, 11, State::PermitAllImplementsToOperationOn);
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest.get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations, State::PermitAllImplementsToOperationOn }), events);

	receive_isb(0x76, 11, State::NotAvailable);
	receive_isb(0x76, 12, State::NotAvailable);
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest.get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations, State::PermitAllImplementsToOperationOn, State::StopImplementOperations }), events);

	receive_isb(0x76, 12, State::PermitAllImplementsToOperationOn);
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest.get_state());

	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(IsobusShortcutButtonTest, ShortcutButtonServerCountsPermitToStop)
{
	VirtualCANPlugin serverPlugin;
	serverPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x98, 0, time_source);
	test_helpers::force_claim_partnered_control_function(0x74, 0);

	time_source.update_for_ms(50);

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
	std::vector<CANMessageFrame> isbFrames;

	interfaceUnderTest.set_stop_all_implement_operations_state(State::PermitAllImplementsToOperationOn);
	pump_isb_frames(interfaceUnderTest, time_source, serverPlugin, 5, isbFrames);
	ASSERT_FALSE(isbFrames.empty());
	EXPECT_EQ(0, isbFrames.back().data[6]);

	interfaceUnderTest.set_stop_all_implement_operations_state(State::StopImplementOperations);
	pump_isb_frames(interfaceUnderTest, time_source, serverPlugin, 5, isbFrames);
	ASSERT_FALSE(isbFrames.empty());
	EXPECT_EQ(1, isbFrames.back().data[6]);

	isbFrames.clear();
	pump_isb_frames(interfaceUnderTest, time_source, serverPlugin, 2500, isbFrames);
	ASSERT_GE(isbFrames.size(), 2U);
	for (const auto &isbFrame : isbFrames)
	{
		EXPECT_EQ(1, isbFrame.data[6]);
	}

	interfaceUnderTest.set_stop_all_implement_operations_state(State::PermitAllImplementsToOperationOn);
	pump_isb_frames(interfaceUnderTest, time_source, serverPlugin, 5, isbFrames);
	EXPECT_EQ(1, isbFrames.back().data[6]);

	interfaceUnderTest.set_stop_all_implement_operations_state(State::StopImplementOperations);
	pump_isb_frames(interfaceUnderTest, time_source, serverPlugin, 5, isbFrames);
	EXPECT_EQ(2, isbFrames.back().data[6]);

	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}
