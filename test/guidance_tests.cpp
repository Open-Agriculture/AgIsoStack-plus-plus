#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/isobus_guidance_interface.hpp"
#include "isobus/utility/system_timing.hpp"

using namespace isobus;

TEST(GUIDANCE_TESTS, GuidanceMessages)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	isobus::NAME TestDeviceNAME(0);
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(3);
	TestDeviceNAME.set_device_class(4);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::AdaptiveFrontLightingSystem));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(4);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);

	auto testECU = std::make_shared<isobus::InternalControlFunction>(TestDeviceNAME, 0x44, 0);

	HardwareInterfaceCANFrame testFrame;

	std::uint32_t waitingTimestamp_ms = SystemTiming::get_timestamp_ms();

	while ((!testECU->get_address_valid()) &&
	       (!SystemTiming::time_expired_ms(waitingTimestamp_ms, 2000)))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	ASSERT_TRUE(testECU->get_address_valid());

	// Get the virtual CAN plugin back to a known state
	while (!testPlugin.get_queue_empty())
	{
		testPlugin.read_frame(testFrame);
	}
	ASSERT_TRUE(testPlugin.get_queue_empty());

	GuidanceInterface interfaceUnderTest(testECU, nullptr); // Configured for broadcasts

	CANHardwareInterface::stop();
	testPlugin.close();
}

TEST(GUIDANCE_TESTS, ListenOnlyMode)
{
	GuidanceInterface interfaceUnderTest(nullptr, nullptr);

	EXPECT_FALSE(interfaceUnderTest.send_guidance_system_command());
	EXPECT_FALSE(interfaceUnderTest.send_agricultural_guidance_machine_info());
}
