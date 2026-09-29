#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/zephyr/can_plugin.hpp"
#include "isobus/zephyr/time_source.hpp"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>

#include <atomic>
#include <memory>

#ifdef CONFIG_BOARD_NATIVE_SIM
#include <posix_board_if.h>
#endif

static int finish(int result)
{
#ifdef CONFIG_BOARD_NATIVE_SIM
	posix_exit(result);
#endif
	return result;
}

int main()
{
	const device *canDevice = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));
	if (!device_is_ready(canDevice))
	{
		printk("Zephyr CAN device is not ready\n");
		return finish(1);
	}
	if (can_set_mode(canDevice, CAN_MODE_LOOPBACK) != 0)
	{
		printk("Zephyr CAN loopback mode is unavailable\n");
		return finish(1);
	}

	static isobus::ZephyrTimeSource timeSource;
	isobus::SystemTiming::override_time_source(&timeSource);
	auto canPlugin = std::make_shared<isobus::ZephyrCANPlugin>(canDevice);
	std::atomic<unsigned int> receivedFrames{ 0 };
	isobus::CANHardwareInterface::get_can_frame_received_event_dispatcher().add_listener([&receivedFrames](const isobus::CANMessageFrame &frame) {
		if ((frame.channel == 0) && (frame.dataLength == 2) && (frame.data[0] == 0xA5) && (frame.data[1] == 0x5A))
		{
			if (!frame.isExtendedFrame && (frame.identifier == 0x123))
			{
				receivedFrames.fetch_or(1);
			}
			else if (frame.isExtendedFrame && (frame.identifier == 0x1ABCDE))
			{
				receivedFrames.fetch_or(2);
			}
		}
	});
	if (!isobus::CANHardwareInterface::set_number_of_can_channels(1) ||
	    !isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, canPlugin))
	{
		printk("AgIsoStack++ CAN channel setup failed\n");
		return finish(1);
	}

	if (!isobus::CANHardwareInterface::start() || !isobus::CANHardwareInterface::is_running() || !canPlugin->get_is_valid())
	{
		printk("AgIsoStack++ startup failed\n");
		if (isobus::CANHardwareInterface::is_running())
		{
			isobus::CANHardwareInterface::stop();
		}
		return finish(1);
	}

	printk("AgIsoStack++ started with Zephyr CAN\n");
	isobus::CANMessageFrame standardFrame{};
	standardFrame.channel = 0;
	standardFrame.identifier = 0x123;
	standardFrame.dataLength = 2;
	standardFrame.data[0] = 0xA5;
	standardFrame.data[1] = 0x5A;
	auto extendedFrame = standardFrame;
	extendedFrame.identifier = 0x1ABCDE;
	extendedFrame.isExtendedFrame = true;
	const bool sent = isobus::CANHardwareInterface::transmit_can_frame(standardFrame) &&
	                  isobus::CANHardwareInterface::transmit_can_frame(extendedFrame);
	const int64_t deadline = k_uptime_get() + 1000;
	while (sent && (receivedFrames.load() != 3) && (k_uptime_get() < deadline))
	{
		k_msleep(1);
	}
	const bool loopbackPassed = sent && (receivedFrames.load() == 3);
	if (!loopbackPassed)
	{
		printk("Zephyr CAN loopback failed (sent=%d, received=0x%x)\n", sent, receivedFrames.load());
	}
	else
	{
		printk("Zephyr CAN standard and extended loopback passed\n");
	}
	if (!isobus::CANHardwareInterface::stop())
	{
		printk("AgIsoStack++ shutdown failed\n");
		return finish(1);
	}
	printk("AgIsoStack++ stopped\n");
	return finish(loopbackPassed ? 0 : 1);
}
