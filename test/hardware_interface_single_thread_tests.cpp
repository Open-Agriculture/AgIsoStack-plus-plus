//================================================================================================
/// @file hardware_interface_single_thread_tests.cpp
///
/// @brief Unit tests for the CAN hardware interface when the stack runs without threads
/// (CAN_STACK_DISABLE_THREADS), where update() polls the drivers itself.
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/can_hardware_plugin.hpp"

#include <deque>

using namespace isobus;

/// @brief A driver that hands out frames queued by the test, like a controller with received frames waiting
class QueuedFramesPlugin : public CANHardwarePlugin
{
public:
	std::string get_name() const override
	{
		return "Queued frames";
	}

	bool get_is_valid() const override
	{
		return true;
	}

	void close() override
	{
	}

	void open() override
	{
	}

	bool read_frame(CANMessageFrame &canFrame) override
	{
		if (frames.empty())
		{
			return false;
		}
		canFrame = frames.front();
		frames.pop_front();
		return true;
	}

	bool write_frame(const CANMessageFrame &) override
	{
		return true;
	}

	void queue_frames(std::size_t count)
	{
		for (std::size_t i = 0; i < count; i++)
		{
			CANMessageFrame frame = {};
			frame.identifier = 0x18EF8081;
			frame.isExtendedFrame = true;
			frame.dataLength = 8;
			frame.data[0] = static_cast<std::uint8_t>(i);
			frames.push_back(frame);
		}
	}

	std::deque<CANMessageFrame> frames;
};

TEST(HARDWARE_INTERFACE_SINGLE_THREAD_TESTS, UpdateTakesEveryFrameTheDriverHolds)
{
	auto driver = std::make_shared<QueuedFramesPlugin>();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, driver);
	CANHardwareInterface::start();

	std::vector<std::uint8_t> received;
	auto listener = CANHardwareInterface::get_can_frame_received_event_dispatcher().add_listener([&received](const CANMessageFrame &frame) {
		received.push_back(frame.data[0]);
	});

	// A burst of back-to-back frames waiting in the driver when the application calls update()
	driver->queue_frames(7);
	CANHardwareInterface::update();

	EXPECT_EQ(std::vector<std::uint8_t>({ 0, 1, 2, 3, 4, 5, 6 }), received);
	EXPECT_TRUE(driver->frames.empty());

	CANHardwareInterface::stop();
	CANHardwareInterface::set_number_of_can_channels(0);
}

TEST(HARDWARE_INTERFACE_SINGLE_THREAD_TESTS, UpdateTakesNoMoreThanTheReceiveQueueHolds)
{
	auto driver = std::make_shared<QueuedFramesPlugin>();
	CANHardwareInterface::set_number_of_can_channels(1, 4);
	CANHardwareInterface::assign_can_channel_frame_handler(0, driver);
	CANHardwareInterface::start();

	std::vector<std::uint8_t> received;
	auto listener = CANHardwareInterface::get_can_frame_received_event_dispatcher().add_listener([&received](const CANMessageFrame &frame) {
		received.push_back(frame.data[0]);
	});

	// More frames than the receive queue holds: the rest waits in the driver for the next update
	driver->queue_frames(6);
	CANHardwareInterface::update();
	EXPECT_EQ(std::vector<std::uint8_t>({ 0, 1, 2, 3 }), received);
	EXPECT_EQ(2u, driver->frames.size());

	CANHardwareInterface::update();
	EXPECT_EQ(std::vector<std::uint8_t>({ 0, 1, 2, 3, 4, 5 }), received);
	EXPECT_TRUE(driver->frames.empty());

	CANHardwareInterface::stop();
	CANHardwareInterface::set_number_of_can_channels(0);
}
