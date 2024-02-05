#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/utility/system_timing.hpp"

#include <chrono>
#include <future>
#include <thread>

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
