#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"

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

	HardwareInterfaceCANFrame fakeFrame;
	memset(&fakeFrame, 0, sizeof(HardwareInterfaceCANFrame));
	fakeFrame.identifier = 0x613;
	fakeFrame.isExtendedFrame = false;
	fakeFrame.dataLength = 1;
	fakeFrame.data[0] = 0x01;
	fakeFrame.channel = 0;

	HardwareInterfaceCANFrame receiveFrame;
	memset(&receiveFrame, 0, sizeof(HardwareInterfaceCANFrame));
	auto future = std::async(std::launch::async, [&] { receiver->read_frame(receiveFrame); });

	isobus::send_can_message_to_hardware(fakeFrame);

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

	HardwareInterfaceCANFrame fakeFrame;
	memset(&fakeFrame, 0, sizeof(HardwareInterfaceCANFrame));
	fakeFrame.identifier = 0x613;
	fakeFrame.isExtendedFrame = false;
	fakeFrame.dataLength = 1;
	fakeFrame.data[0] = 0x01;
	fakeFrame.channel = 0;

	int message_count = 0;
	CANHardwareInterface::add_raw_can_message_rx_callback(
	  [](HardwareInterfaceCANFrame &frame, void *parent) {
		  int *count = static_cast<int *>(parent);
		  *count += 1;

		  EXPECT_EQ(frame.identifier, 0x613);
		  EXPECT_EQ(frame.isExtendedFrame, false);
		  EXPECT_EQ(frame.dataLength, 1);
		  EXPECT_EQ(frame.data[0], 0x01);
	  },
	  &message_count);

	device->write_frame_as_if_received(fakeFrame);

	//! @todo This is a hack to wait for the message to be received.  We should
	//!       have a way to wait for the message to be received.
	std::this_thread::sleep_for(std::chrono::milliseconds(250));
	EXPECT_EQ(message_count, 1);

	CANHardwareInterface::stop();
}