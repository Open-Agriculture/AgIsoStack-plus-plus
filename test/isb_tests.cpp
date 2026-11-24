#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_shortcut_button_interface.hpp"

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"
#include "helpers/test_fixture.hpp"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

using namespace isobus;

using State = ShortcutButtonInterface::StopAllImplementOperationsState;

class IsobusShortcutButtonTest : public AgIsoStackTestFixture
{
protected:
	void SetUp() override
	{
		AgIsoStackTestFixture::SetUp();
		CANHardwareInterface::set_number_of_can_channels(1);
		CANHardwareInterface::assign_can_channel_frame_handler(0, hardwarePlugin);
		CANHardwareInterface::start(false);
		internalECU = test_helpers::claim_internal_control_function(0x97, 0, time_source);
	}

	std::shared_ptr<VirtualCANPlugin> hardwarePlugin = std::make_shared<VirtualCANPlugin>();
	std::shared_ptr<InternalControlFunction> internalECU;
};

TEST_F(IsobusShortcutButtonTest, ShortcutButtonInitialize)
{
	ShortcutButtonInterface interfaceUnderTest(internalECU, false);
	EXPECT_FALSE(interfaceUnderTest.get_is_initialized());
	interfaceUnderTest.initialize();
	EXPECT_TRUE(interfaceUnderTest.get_is_initialized());
}

class IsobusShortcutButtonReceiverTest : public IsobusShortcutButtonTest
{
protected:
	void SetUp() override
	{
		IsobusShortcutButtonTest::SetUp();
		interfaceUnderTest.reset(new ShortcutButtonInterface(internalECU, false));
		interfaceUnderTest->initialize();
		interfaceUnderTest->get_stop_all_implement_operations_state_event_dispatcher().add_listener([this](State state) {
			events.push_back(state);
		});
		CANNetworkManager::CANNetwork.update(); // The network manager drops received frames until it has been updated once
	}

	void queue_isb(std::uint8_t sourceAddress, std::uint8_t transitionCount, State state)
	{
		const std::uint32_t identifier = (3U << 26) | (0xFD02U << 8) | sourceAddress;
		auto frame = test_helpers::create_message_frame_raw(identifier, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, transitionCount, static_cast<std::uint8_t>(0xFC | static_cast<std::uint8_t>(state)) });
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(frame);
		CANNetworkManager::CANNetwork.update();
	}

	void receive_isb(std::uint8_t sourceAddress, std::uint8_t transitionCount, State state)
	{
		queue_isb(sourceAddress, transitionCount, state);
		interfaceUnderTest->update();
	}

	std::unique_ptr<ShortcutButtonInterface> interfaceUnderTest;
	std::vector<State> events;
};

TEST_F(IsobusShortcutButtonReceiverTest, ShortcutButtonRxTests)
{
	test_helpers::force_claim_partnered_control_function(0x74, 0);
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest->get_state());

	// Since we're not acting as a server, make sure the public setter doesn't do anything
	interfaceUnderTest->set_stop_all_implement_operations_state(State::StopImplementOperations);
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest->get_state());

	receive_isb(0x74, 0, State::StopImplementOperations);
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());

	receive_isb(0x74, 0, State::PermitAllImplementsToOperationOn);
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest->get_state());

	// Increased transition count without a Stop
	receive_isb(0x74, 8, State::PermitAllImplementsToOperationOn);
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());

	receive_isb(0x74, 8, State::PermitAllImplementsToOperationOn);
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest->get_state());

	// A lower count is also an error
	receive_isb(0x74, 0, State::PermitAllImplementsToOperationOn);
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());

	receive_isb(0x74, 0, State::PermitAllImplementsToOperationOn);
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest->get_state());

	receive_isb(0x74, 255, State::PermitAllImplementsToOperationOn);
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());

	receive_isb(0x74, 255, State::PermitAllImplementsToOperationOn);
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest->get_state());

	// A stop that wraps the count from 255 to 0 is a valid transition
	receive_isb(0x74, 0, State::StopImplementOperations);
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());

	receive_isb(0x74, 0, State::PermitAllImplementsToOperationOn);
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest->get_state());

	events.clear();
	receive_isb(0x74, 0xF0, State::StopImplementOperations);
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations }), events);

	time_source.simulate_delay_ms(2999);
	interfaceUnderTest->update();
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());

	time_source.simulate_delay_ms(1);
	interfaceUnderTest->update();
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest->get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations, State::PermitAllImplementsToOperationOn }), events);
}

TEST_F(IsobusShortcutButtonReceiverTest, ShortcutButtonNotAvailableSenderCountIsIgnored)
{
	test_helpers::force_claim_partnered_control_function(0x75, 0);

	receive_isb(0x75, 10, State::NotAvailable);
	receive_isb(0x75, 11, State::NotAvailable);
	receive_isb(0x75, 12, State::NotAvailable);
	receive_isb(0x75, 12, State::PermitAllImplementsToOperationOn);
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest->get_state());
	EXPECT_TRUE(events.empty());

	receive_isb(0x75, 13, State::StopImplementOperations);
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations }), events);
}

TEST_F(IsobusShortcutButtonReceiverTest, ShortcutButtonNotAvailableAfterPermitIsChecked)
{
	test_helpers::force_claim_partnered_control_function(0x76, 0);

	receive_isb(0x76, 10, State::PermitAllImplementsToOperationOn);
	receive_isb(0x76, 11, State::NotAvailable);
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations }), events);

	receive_isb(0x76, 11, State::PermitAllImplementsToOperationOn);
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest->get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations, State::PermitAllImplementsToOperationOn }), events);

	receive_isb(0x76, 11, State::NotAvailable);
	receive_isb(0x76, 12, State::NotAvailable);
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations, State::PermitAllImplementsToOperationOn, State::StopImplementOperations }), events);

	receive_isb(0x76, 12, State::PermitAllImplementsToOperationOn);
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest->get_state());
}

TEST_F(IsobusShortcutButtonReceiverTest, ShortcutButtonTimeoutKeepsTheTransitionCount)
{
	test_helpers::force_claim_partnered_control_function(0x77, 0);

	receive_isb(0x77, 5, State::PermitAllImplementsToOperationOn);
	time_source.simulate_delay_ms(3000);
	interfaceUnderTest->update();
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest->get_state());
	EXPECT_TRUE(events.empty());

	receive_isb(0x77, 6, State::PermitAllImplementsToOperationOn);
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations }), events);
}

TEST_F(IsobusShortcutButtonReceiverTest, ShortcutButtonReceivedStateAppliesInUpdate)
{
	test_helpers::force_claim_partnered_control_function(0x78, 0);

	queue_isb(0x78, 0, State::StopImplementOperations);
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest->get_state());
	EXPECT_TRUE(events.empty());

	interfaceUnderTest->update();
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations }), events);
}

TEST_F(IsobusShortcutButtonReceiverTest, ShortcutButtonQueuedMessageKeepsItsReceiveTime)
{
	test_helpers::force_claim_partnered_control_function(0x79, 0);

	queue_isb(0x79, 0, State::StopImplementOperations);
	time_source.simulate_delay_ms(2999);
	interfaceUnderTest->update();
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations }), events);

	time_source.simulate_delay_ms(1);
	interfaceUnderTest->update();
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest->get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations, State::PermitAllImplementsToOperationOn }), events);
}

TEST_F(IsobusShortcutButtonReceiverTest, ShortcutButtonStopThatWaitedOutTheTimeoutIsReported)
{
	test_helpers::force_claim_partnered_control_function(0x7A, 0);

	queue_isb(0x7A, 0, State::StopImplementOperations);
	time_source.simulate_delay_ms(3000);
	interfaceUnderTest->update();
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest->get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations, State::PermitAllImplementsToOperationOn }), events);
}

TEST_F(IsobusShortcutButtonReceiverTest, ShortcutButtonStopAfterABurstIsNotDropped)
{
	test_helpers::force_claim_partnered_control_function(0x7B, 0);

	receive_isb(0x7B, 5, State::PermitAllImplementsToOperationOn);

	for (std::uint8_t i = 0; i < 20; i++)
	{
		queue_isb(0x7B, 5, State::PermitAllImplementsToOperationOn);
	}
	queue_isb(0x7B, 6, State::StopImplementOperations);
	interfaceUnderTest->update();
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations }), events);
}

TEST_F(IsobusShortcutButtonReceiverTest, ShortcutButtonMessageReceivedDuringAListenerWaitsForTheNextUpdate)
{
	test_helpers::force_claim_partnered_control_function(0x7D, 0);
	interfaceUnderTest->get_stop_all_implement_operations_state_event_dispatcher().add_listener([this](State state) {
		if (State::StopImplementOperations == state)
		{
			queue_isb(0x7D, 6, State::PermitAllImplementsToOperationOn);
		}
	});

	receive_isb(0x7D, 5, State::PermitAllImplementsToOperationOn);

	queue_isb(0x7D, 6, State::StopImplementOperations);
	interfaceUnderTest->update();
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations }), events);

	interfaceUnderTest->update();
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest->get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations, State::PermitAllImplementsToOperationOn }), events);
}

TEST_F(IsobusShortcutButtonReceiverTest, ShortcutButtonSlowListenerDoesNotTimeOutAQueuedStop)
{
	test_helpers::force_claim_partnered_control_function(0x7E, 0);
	bool blocked = false;
	interfaceUnderTest->get_stop_all_implement_operations_state_event_dispatcher().add_listener([this, &blocked](State state) {
		if ((State::StopImplementOperations == state) && (!blocked))
		{
			// The listener blocks for the whole timeout while the sender keeps repeating Stop
			blocked = true;
			time_source.simulate_delay_ms(3000);
			queue_isb(0x7E, 6, State::StopImplementOperations);
		}
	});

	receive_isb(0x7E, 5, State::PermitAllImplementsToOperationOn);

	queue_isb(0x7E, 6, State::StopImplementOperations);
	interfaceUnderTest->update();
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations }), events);

	interfaceUnderTest->update();
	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());
	EXPECT_EQ(std::vector<State>({ State::StopImplementOperations }), events);
}

#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
TEST_F(IsobusShortcutButtonReceiverTest, ShortcutButtonReceiveThreadDoesNotRaceUpdate)
{
	test_helpers::force_claim_partnered_control_function(0x7C, 0);

	std::atomic<bool> done = { false };
	std::thread receiver([this, &done]() {
		for (std::uint8_t transitionCount = 1; transitionCount <= 200; transitionCount++)
		{
			queue_isb(0x7C, transitionCount, State::StopImplementOperations);
			queue_isb(0x7C, transitionCount, State::PermitAllImplementsToOperationOn);
		}
		queue_isb(0x7C, 201, State::StopImplementOperations);
		done = true;
	});

	while (!done)
	{
		interfaceUnderTest->update();
		interfaceUnderTest->get_state();
	}
	receiver.join();
	interfaceUnderTest->update();

	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest->get_state());
}
#endif

class IsobusShortcutButtonSenderTest : public IsobusShortcutButtonTest
{
protected:
	void SetUp() override
	{
		observer.open();
		IsobusShortcutButtonTest::SetUp();
		time_source.update_for_ms(50);
		observer.clear_queue();
		CANNetworkManager::CANNetwork.update();
	}

	void pump_isb_frames(ShortcutButtonInterface &interfaceUnderTest, std::uint32_t durationMs, std::vector<CANMessageFrame> &isbFrames)
	{
		CANMessageFrame frame = {};

		for (std::uint32_t i = 0; i < durationMs; i++)
		{
			interfaceUnderTest.update();
			time_source.update_for_ms(1);

			while (!observer.get_queue_empty())
			{
				observer.read_frame(frame);
				frame.timestamp_us = time_source.get_current_time_us();
				if (0xFD02 == CANIdentifier(frame.identifier).get_parameter_group_number())
				{
					isbFrames.push_back(frame);
				}
			}
		}
	}

	VirtualCANPlugin observer;
};

TEST_F(IsobusShortcutButtonSenderTest, ShortcutButtonTxTests)
{
	ShortcutButtonInterface interfaceUnderTest(internalECU, true);
	interfaceUnderTest.initialize();
	EXPECT_EQ(State::PermitAllImplementsToOperationOn, interfaceUnderTest.get_state());

	interfaceUnderTest.set_stop_all_implement_operations_state(State::StopImplementOperations);
	interfaceUnderTest.update();
	time_source.update_for_ms(5);
	CANMessageFrame testFrame = {};
	EXPECT_TRUE(observer.read_frame(testFrame));

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

	EXPECT_EQ(State::StopImplementOperations, interfaceUnderTest.get_state());
}

TEST_F(IsobusShortcutButtonSenderTest, ShortcutButtonServerCountsPermitToStop)
{
	ShortcutButtonInterface interfaceUnderTest(internalECU, true);
	interfaceUnderTest.initialize();
	std::vector<CANMessageFrame> isbFrames;

	interfaceUnderTest.set_stop_all_implement_operations_state(State::PermitAllImplementsToOperationOn);
	pump_isb_frames(interfaceUnderTest, 5, isbFrames);
	ASSERT_FALSE(isbFrames.empty());
	EXPECT_EQ(0, isbFrames.back().data[6]);

	interfaceUnderTest.set_stop_all_implement_operations_state(State::StopImplementOperations);
	pump_isb_frames(interfaceUnderTest, 5, isbFrames);
	ASSERT_FALSE(isbFrames.empty());
	EXPECT_EQ(1, isbFrames.back().data[6]);

	isbFrames.clear();
	pump_isb_frames(interfaceUnderTest, 2500, isbFrames);
	ASSERT_GE(isbFrames.size(), 2U);
	for (const auto &isbFrame : isbFrames)
	{
		EXPECT_EQ(1, isbFrame.data[6]);
	}

	interfaceUnderTest.set_stop_all_implement_operations_state(State::PermitAllImplementsToOperationOn);
	pump_isb_frames(interfaceUnderTest, 5, isbFrames);
	EXPECT_EQ(1, isbFrames.back().data[6]);

	interfaceUnderTest.set_stop_all_implement_operations_state(State::StopImplementOperations);
	pump_isb_frames(interfaceUnderTest, 5, isbFrames);
	EXPECT_EQ(2, isbFrames.back().data[6]);
}

TEST_F(IsobusShortcutButtonSenderTest, ShortcutButtonClientDoesNotSend)
{
	ShortcutButtonInterface interfaceUnderTest(internalECU, false);
	interfaceUnderTest.initialize();
	std::vector<CANMessageFrame> isbFrames;

	pump_isb_frames(interfaceUnderTest, 2500, isbFrames);
	EXPECT_TRUE(isbFrames.empty());
}

TEST_F(IsobusShortcutButtonSenderTest, ShortcutButtonStopThenPermitBeforeUpdateSendsBoth)
{
	ShortcutButtonInterface interfaceUnderTest(internalECU, true);
	interfaceUnderTest.initialize();
	std::vector<CANMessageFrame> isbFrames;

	pump_isb_frames(interfaceUnderTest, 5, isbFrames);
	isbFrames.clear();

	interfaceUnderTest.set_stop_all_implement_operations_state(State::StopImplementOperations);
	interfaceUnderTest.set_stop_all_implement_operations_state(State::PermitAllImplementsToOperationOn);
	pump_isb_frames(interfaceUnderTest, 5, isbFrames);

	ASSERT_EQ(2U, isbFrames.size());
	EXPECT_EQ(0xFC, isbFrames.at(0).data[7]);
	EXPECT_EQ(0xFD, isbFrames.at(1).data[7]);
}

TEST_F(IsobusShortcutButtonSenderTest, ShortcutButtonFailedStopIsSentBeforePermit)
{
	ShortcutButtonInterface interfaceUnderTest(internalECU, true);
	interfaceUnderTest.initialize();
	std::vector<CANMessageFrame> isbFrames;

	interfaceUnderTest.set_stop_all_implement_operations_state(State::PermitAllImplementsToOperationOn);
	pump_isb_frames(interfaceUnderTest, 5, isbFrames);
	isbFrames.clear();

	hardwarePlugin->close();
	interfaceUnderTest.set_stop_all_implement_operations_state(State::StopImplementOperations);
	hardwarePlugin->open();
	interfaceUnderTest.set_stop_all_implement_operations_state(State::PermitAllImplementsToOperationOn);
	pump_isb_frames(interfaceUnderTest, 5, isbFrames);

	ASSERT_EQ(2U, isbFrames.size());
	EXPECT_EQ(0xFC, isbFrames.at(0).data[7]);
	EXPECT_EQ(1, isbFrames.at(0).data[6]);
	EXPECT_EQ(0xFD, isbFrames.at(1).data[7]);
	EXPECT_EQ(1, isbFrames.at(1).data[6]);
}

TEST_F(IsobusShortcutButtonSenderTest, ShortcutButtonServerSendsOncePerSecond)
{
	ShortcutButtonInterface interfaceUnderTest(internalECU, true);
	interfaceUnderTest.initialize();
	std::vector<CANMessageFrame> isbFrames;

	pump_isb_frames(interfaceUnderTest, 5000, isbFrames);
	ASSERT_GE(isbFrames.size(), 4U);
	for (std::size_t i = 1; i < isbFrames.size(); i++)
	{
		EXPECT_EQ(1000000U, isbFrames.at(i).timestamp_us - isbFrames.at(i - 1).timestamp_us);
	}

	const std::uint64_t lastPeriodic_us = isbFrames.back().timestamp_us;
	isbFrames.clear();
	interfaceUnderTest.set_stop_all_implement_operations_state(State::PermitAllImplementsToOperationOn);
	pump_isb_frames(interfaceUnderTest, 1000, isbFrames);
	ASSERT_EQ(2U, isbFrames.size());
	EXPECT_EQ(1000000U, isbFrames.at(1).timestamp_us - lastPeriodic_us);
}
