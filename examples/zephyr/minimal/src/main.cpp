#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/zephyr/time_source.hpp"

#include <zephyr/kernel.h>

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
	static isobus::ZephyrTimeSource timeSource;
	isobus::SystemTiming::override_time_source(&timeSource);

	if (!isobus::CANHardwareInterface::start() || !isobus::CANHardwareInterface::is_running())
	{
		printk("AgIsoStack++ startup failed\n");
		return finish(1);
	}

	printk("AgIsoStack++ started\n");
	k_msleep(10);
	if (!isobus::CANHardwareInterface::stop())
	{
		printk("AgIsoStack++ shutdown failed\n");
		return finish(1);
	}
	printk("AgIsoStack++ stopped\n");
	return finish(0);
}
