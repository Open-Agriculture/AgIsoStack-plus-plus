#include "isobus/hardware_integration/available_can_drivers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"

#include <string.h>
#include <csignal>
#include <iostream>

static std::atomic_bool running = { true };

void signal_handler(int)
{
	running = false;
}

int main()
{
	std::signal(SIGINT, signal_handler);

	auto physicalCAN = std::make_shared<isobus::InnoMakerUSB2CANWindowsPlugin>(0);
	auto virtualCAN = std::make_shared<isobus::NTCANPlugin>(42);

	isobus::CANHardwareInterface::set_number_of_can_channels(2);
	isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, physicalCAN);
	isobus::CANHardwareInterface::assign_can_channel_frame_handler(1, virtualCAN);

	if ((!isobus::CANHardwareInterface::start()) || (!physicalCAN->get_is_valid()) || (!virtualCAN->get_is_valid()))
	{
		std::cout << "Failed to initialize. An interface might not have started." << std::endl;
		return -1;
	}

	isobus::CANHardwareInterface::get_can_frame_received_event_dispatcher().add_listener([](const isobus::CANMessageFrame &frame) {
		isobus::CANMessageFrame newFrame = frame;

		// Relay the frame to the other channel
		if (frame.channel == 0)
		{
			newFrame.channel = 1;
		}
		else
		{
			newFrame.channel = 0;
		}
		isobus::CANHardwareInterface::transmit_can_frame(newFrame);
	});

	while (running)
	{
		// CAN stack runs in other threads. Do nothing forever.
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	isobus::CANHardwareInterface::stop();
	return 0;
}
