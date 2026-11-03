#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/utility/system_timing.hpp"

#ifdef ISOBUS_SOCKETCAN_AVAILABLE
#include "isobus/hardware_integration/socket_can_interface.hpp"
#endif

#include <atomic>
#include <chrono>
#include <future>
#include <thread>
#include <vector>

using namespace isobus;

TEST(HARDWARE_INTERFACE_TESTS, SendMessageToHardware)
{
	auto sender = std::make_shared<VirtualCANPlugin>();
	auto receiver = std::make_shared<VirtualCANPlugin>();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, sender);
	CANHardwareInterface::start();

	CANMessageFrame fakeFrame;
	memset(&fakeFrame, 0, sizeof(CANMessageFrame));
	fakeFrame.identifier = 0x613;
	fakeFrame.isExtendedFrame = false;
	fakeFrame.dataLength = 1;
	fakeFrame.data[0] = 0x01;
	fakeFrame.channel = 0;

	CANMessageFrame receiveFrame;
	memset(&receiveFrame, 0, sizeof(CANMessageFrame));
	auto future = std::async(std::launch::async, [&] { receiver->read_frame(receiveFrame); });

	isobus::send_can_message_frame_to_hardware(fakeFrame);

	EXPECT_TRUE(future.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

	EXPECT_EQ(fakeFrame.identifier, 0x613);
	EXPECT_EQ(fakeFrame.isExtendedFrame, false);
	EXPECT_EQ(fakeFrame.dataLength, 1);
	EXPECT_EQ(fakeFrame.data[0], 0x01);

	CANHardwareInterface::stop();
}

#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
TEST(HARDWARE_INTERFACE_TESTS, MultipleThreadsCanTransmitWithoutLosingFrames)
{
	constexpr std::uint32_t PRODUCER_COUNT = 4;
	constexpr std::uint32_t FRAMES_PER_PRODUCER = 2000;

	auto sender = std::make_shared<VirtualCANPlugin>();
	auto receiver = std::make_shared<VirtualCANPlugin>();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, sender);
	ASSERT_TRUE(CANHardwareInterface::start());

	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);

	// Every producer sends numbered frames, using its index as the identifier
	std::vector<std::thread> producers;
	for (std::uint32_t producer = 0; producer < PRODUCER_COUNT; producer++)
	{
		producers.emplace_back([producer, FRAMES_PER_PRODUCER, deadline]() {
			for (std::uint32_t sequence = 0; sequence < FRAMES_PER_PRODUCER; sequence++)
			{
				CANMessageFrame frame;
				memset(&frame, 0, sizeof(CANMessageFrame));
				frame.identifier = producer;
				frame.isExtendedFrame = true;
				frame.dataLength = 4;
				memcpy(frame.data, &sequence, sizeof(sequence));
				frame.channel = 0;

				// The queue is bounded, so a full queue is expected. Wait for the hardware thread to drain it.
				while (!send_can_message_frame_to_hardware(frame))
				{
					if (std::chrono::steady_clock::now() > deadline)
					{
						return;
					}
					std::this_thread::yield();
				}
			}
		});
	}

	// The queue keeps the order of each producer, so the sequence numbers must arrive one by one
	std::vector<std::uint32_t> expectedSequence(PRODUCER_COUNT, 0);
	std::uint32_t received = 0;
	bool framesArrivedIntact = true;
	while ((received < PRODUCER_COUNT * FRAMES_PER_PRODUCER) && (std::chrono::steady_clock::now() < deadline))
	{
		CANMessageFrame frame;
		if (receiver->read_frame(frame, 100))
		{
			std::uint32_t sequence = 0;
			memcpy(&sequence, frame.data, sizeof(sequence));
			if ((frame.identifier >= PRODUCER_COUNT) || (4 != frame.dataLength) || (sequence != expectedSequence[frame.identifier]))
			{
				framesArrivedIntact = false;
				break;
			}
			expectedSequence[frame.identifier]++;
			received++;
		}
	}

	for (auto &producer : producers)
	{
		producer.join();
	}
	CANHardwareInterface::stop();

	EXPECT_TRUE(framesArrivedIntact);
	EXPECT_EQ(received, PRODUCER_COUNT * FRAMES_PER_PRODUCER);
}
#endif

TEST(HARDWARE_INTERFACE_TESTS, ReceiveMessageFromHardware)
{
	auto device = std::make_shared<VirtualCANPlugin>();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, device);
	CANHardwareInterface::start();

	CANMessageFrame fakeFrame;
	memset(&fakeFrame, 0, sizeof(CANMessageFrame));
	fakeFrame.identifier = 0x613;
	fakeFrame.isExtendedFrame = false;
	fakeFrame.dataLength = 1;
	fakeFrame.data[0] = 0x01;
	fakeFrame.channel = 0;

	int messageCount = 0;
	std::function<void(const CANMessageFrame &)> receivedCallback = [&messageCount](const CANMessageFrame &frame) {
		messageCount += 1;

		EXPECT_EQ(frame.identifier, 0x613);
		EXPECT_EQ(frame.isExtendedFrame, false);
		EXPECT_EQ(frame.dataLength, 1);
		EXPECT_EQ(frame.data[0], 0x01);
	};

	CANHardwareInterface::get_can_frame_received_event_dispatcher().add_listener(receivedCallback);

	device->write_frame_as_if_received(fakeFrame);

	auto future = std::async(std::launch::async, [&messageCount] { while (messageCount == 0 && CANHardwareInterface::is_running()); });
	EXPECT_TRUE(future.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

	CANHardwareInterface::stop();
}

TEST(HARDWARE_INTERFACE_TESTS, MessageFrameSentEventListener)
{
	auto receiver = std::make_shared<VirtualCANPlugin>();
	auto sender = std::make_shared<VirtualCANPlugin>();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, sender);
	CANHardwareInterface::start();

	CANMessageFrame fakeFrame;
	memset(&fakeFrame, 0, sizeof(CANMessageFrame));
	fakeFrame.identifier = 0x613;
	fakeFrame.isExtendedFrame = false;
	fakeFrame.dataLength = 1;
	fakeFrame.data[0] = 0x01;
	fakeFrame.channel = 0;

	CANMessageFrame receiveFrame;
	memset(&receiveFrame, 0, sizeof(CANMessageFrame));

	int messageCount = 0;
	std::function<void(const CANMessageFrame &)> sendCallback = [&messageCount](const CANMessageFrame &frame) {
		messageCount += 1;

		EXPECT_EQ(frame.identifier, 0x613);
		EXPECT_EQ(frame.isExtendedFrame, false);
		EXPECT_EQ(frame.dataLength, 1);
		EXPECT_EQ(frame.data[0], 0x01);
	};

	CANHardwareInterface::get_can_frame_transmitted_event_dispatcher().add_listener(sendCallback);

	isobus::send_can_message_frame_to_hardware(fakeFrame);

	auto future = std::async(std::launch::async, [&messageCount] { while (messageCount == 0 && CANHardwareInterface::is_running()); });
	EXPECT_TRUE(future.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

	CANHardwareInterface::stop();
}

TEST(HARDWARE_INTERFACE_TESTS, PeriodicUpdateEventListener)
{
	CANHardwareInterface::start();

	int updateCount = 0;
	std::function<void()> periodicCallback = [&updateCount]() {
		updateCount += 1;
	};

	CANHardwareInterface::get_periodic_update_event_dispatcher().add_listener(periodicCallback);

	auto future = std::async(std::launch::async, [&updateCount] { while (updateCount == 0 && CANHardwareInterface::is_running()); });
	EXPECT_TRUE(future.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

	CANHardwareInterface::stop();
}

TEST(HARDWARE_INTERFACE_TESTS, AddRemoveHardwareFrameHandler)
{
	//! @todo Implement this test
	// We probably want CANNetworkManager to not use the singleton pattern first
}

TEST(HARDWARE_INTERFACE_TESTS, PeriodicUpdateIntervalSetting)
{
	std::uint32_t lastUpdateTime = 0;
	std::uint32_t intervalTime = 0;
	std::function<void()> periodicCallback = [&]() {
		if (lastUpdateTime != 0)
		{
			intervalTime = isobus::SystemTiming::get_time_elapsed_ms(lastUpdateTime);
		}
		lastUpdateTime = isobus::SystemTiming::get_timestamp_ms();
	};

	CANHardwareInterface::get_periodic_update_event_dispatcher().add_listener(periodicCallback);

	CANHardwareInterface::set_periodic_update_interval(10);
	EXPECT_EQ(CANHardwareInterface::get_periodic_update_interval(), 10);

	CANHardwareInterface::start();
	std::future<void> future = std::async(std::launch::async, [&]() {
		while ((intervalTime == 0) &&
		       (intervalTime - CANHardwareInterface::get_periodic_update_interval() < 5) &&
		       (CANHardwareInterface::is_running()))
			;
	});
	EXPECT_TRUE(future.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

	CANHardwareInterface::set_periodic_update_interval(50);
	EXPECT_EQ(CANHardwareInterface::get_periodic_update_interval(), 50);

	EXPECT_TRUE(future.wait_for(std::chrono::seconds(5)) != std::future_status::timeout);

	CANHardwareInterface::stop();
}

TEST(HARDWARE_INTERFACE_TESTS, StopSetsStartedFalseInNonThreadingMode)
{
	// This test verifies that the started flag is properly set to false
	// when stop() is called, even when threading is disabled.
	// This addresses the bug where started remained true when CAN_STACK_DISABLE_THREADS is defined.

	auto device = std::make_shared<VirtualCANPlugin>();

	// Set up the hardware interface
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, device);

	// Verify initial state
	EXPECT_FALSE(CANHardwareInterface::is_running());

	// Start the interface
	CANHardwareInterface::start();
	EXPECT_TRUE(CANHardwareInterface::is_running());

	// Test that we CANNOT unassign frame handlers while running (this should fail)
	bool unassignWhileRunning = CANHardwareInterface::unassign_can_channel_frame_handler(0);
	EXPECT_FALSE(unassignWhileRunning); // Should fail because interface is running

	// Stop the interface - this should set started = false regardless of threading mode
	CANHardwareInterface::stop();

	EXPECT_FALSE(CANHardwareInterface::is_running());

	// Now we should be able to unassign frame handlers after stopping
	// (The frame handler was automatically unassigned during stop(), so this should return false
	// but NOT because of the started check - that was the original bug)
	bool unassignAfterStop = CANHardwareInterface::unassign_can_channel_frame_handler(0);

	// The frame handler was already unassigned during stop(), so this should return false
	// but NOT because of the started check (which was the original bug)
	EXPECT_FALSE(unassignAfterStop);

	// Verify the frame handler was actually unassigned
	EXPECT_EQ(nullptr, CANHardwareInterface::get_assigned_can_channel_frame_handler(0));

	// Test reinitialization - this should work properly now
	CANHardwareInterface::assign_can_channel_frame_handler(0, device);
	EXPECT_NE(nullptr, CANHardwareInterface::get_assigned_can_channel_frame_handler(0));

	// Start again to verify the system can be brought up and down multiple times
	CANHardwareInterface::start();
	EXPECT_TRUE(CANHardwareInterface::is_running());

	CANHardwareInterface::stop();
	EXPECT_FALSE(CANHardwareInterface::is_running());
}

TEST(HARDWARE_INTERFACE_TESTS, VerifyStartedFlagBehaviorInNonThreadingMode)
{
	// This test specifically verifies the started flag behavior
	// and can be used to demonstrate the fix works in non-threading mode

	auto device = std::make_shared<VirtualCANPlugin>();

	// Set up the hardware interface
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, device);

	// Verify initial state - started should be false
	EXPECT_FALSE(CANHardwareInterface::is_running());

	// Start the interface - started should become true
	CANHardwareInterface::start();
	EXPECT_TRUE(CANHardwareInterface::is_running());

	// This is the critical test: when threading is disabled,
	// the original bug would leave started = true after stop()
	// because stop_threads() is not called, and stop_threads()
	// was the only place that set started = false
	CANHardwareInterface::stop();

	// With the fix, started should be false regardless of threading mode
	EXPECT_FALSE(CANHardwareInterface::is_running());

	// The key test: unassign_can_channel_frame_handler checks if started is true
	// and returns false if it is. With the original bug, this would fail
	// in non-threading mode because started would still be true
	// Note: The frame handler was automatically unassigned during stop(), so this should return false
	// but NOT because of the started check (which was the original bug)
	EXPECT_FALSE(CANHardwareInterface::unassign_can_channel_frame_handler(0));

	// Clean up
	CANHardwareInterface::stop();
}

TEST(HARDWARE_INTERFACE_TESTS, StopClosesTheDriver)
{
	auto device = std::make_shared<VirtualCANPlugin>();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, device);
	CANHardwareInterface::start();
	CANHardwareInterface::stop();

	EXPECT_FALSE(device->get_is_valid());

	// Closing on stop is what lets a user switch hardware or recover a bad connection without restarting the application
	CANHardwareInterface::assign_can_channel_frame_handler(0, device);
	CANHardwareInterface::start();

	EXPECT_TRUE(device->get_is_valid());

	CANHardwareInterface::stop();
}

TEST(HARDWARE_INTERFACE_TESTS, OneDriverCannotBeAssignedToTwoChannels)
{
	auto device = std::make_shared<VirtualCANPlugin>();
	CANHardwareInterface::set_number_of_can_channels(2);

	// Both channels would close it, so stopping channel 0 would take down the hardware channel 1 is still reading
	EXPECT_TRUE(CANHardwareInterface::assign_can_channel_frame_handler(0, device));
	EXPECT_FALSE(CANHardwareInterface::assign_can_channel_frame_handler(1, device));

	CANHardwareInterface::unassign_can_channel_frame_handler(0);
	CANHardwareInterface::set_number_of_can_channels(1);
}

class NeverValidPlugin : public CANHardwarePlugin
{
public:
	std::string get_name() const override
	{
		return "NeverValid";
	}
	bool get_is_valid() const override
	{
		return false;
	}
	void close() override
	{
	}
	void open() override
	{
	}
	bool read_frame(CANMessageFrame &) override
	{
		return false;
	}
	bool write_frame(const CANMessageFrame &) override
	{
		return false;
	}
	std::string get_last_error() const override
	{
		return "";
	}
};

TEST(HARDWARE_INTERFACE_TESTS, StartFailsWhenADriverDoesNotOpen)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<NeverValidPlugin>());

	EXPECT_FALSE(CANHardwareInterface::start());

	// Nothing is rolled back, the caller decides whether to stop
	EXPECT_TRUE(CANHardwareInterface::is_running());

	CANHardwareInterface::stop();
}

TEST(HARDWARE_INTERFACE_TESTS, OpenChannelKeepsWorkingWhenAnotherFailsToStart)
{
	auto device = std::make_shared<VirtualCANPlugin>("partial-start");
	auto peer = std::make_shared<VirtualCANPlugin>("partial-start");
	peer->open();
	CANHardwareInterface::set_number_of_can_channels(2);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<NeverValidPlugin>());
	CANHardwareInterface::assign_can_channel_frame_handler(1, device);

	EXPECT_FALSE(CANHardwareInterface::start());
	EXPECT_TRUE(device->get_is_valid());

	CANMessageFrame frame;
	memset(&frame, 0, sizeof(CANMessageFrame));
	frame.identifier = 0x612;
	frame.dataLength = 1;
	frame.channel = 1;
	EXPECT_TRUE(isobus::send_can_message_frame_to_hardware(frame));

	CANMessageFrame peerFrame;
	memset(&peerFrame, 0, sizeof(CANMessageFrame));
	EXPECT_TRUE(peer->read_frame(peerFrame, 1000));
	EXPECT_EQ(0x612u, peerFrame.identifier);

	CANHardwareInterface::stop();
	CANHardwareInterface::set_number_of_can_channels(1);
}

TEST(HARDWARE_INTERFACE_TESTS, ChannelWithoutADriverIsNotAStartFailure)
{
	CANHardwareInterface::set_number_of_can_channels(2);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());

	EXPECT_TRUE(CANHardwareInterface::start());

	CANHardwareInterface::stop();
	CANHardwareInterface::set_number_of_can_channels(1);
}

#ifdef ISOBUS_SOCKETCAN_AVAILABLE
TEST(HARDWARE_INTERFACE_TESTS, SocketCANMissingInterfaceIsNotValid)
{
	SocketCANInterface missing("agisonotreal0");
	missing.open();

	EXPECT_FALSE(missing.get_is_valid());
	EXPECT_NE(std::string::npos, missing.get_last_error().find("agisonotreal0"));
}

TEST(HARDWARE_INTERFACE_TESTS, SocketCANNameOver15CharactersIsRefused)
{
	SocketCANInterface tooLong("agisonotreal0123");
	tooLong.open();

	EXPECT_FALSE(tooLong.get_is_valid());
	EXPECT_NE(std::string::npos, tooLong.get_last_error().find("longer than 15 characters"));
}
#endif
