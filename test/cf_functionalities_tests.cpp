#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_functionalities.hpp"
#include "isobus/utility/system_timing.hpp"

using namespace isobus;

TEST(CONTROL_FUNCTION_FUNCTIONALITIES_TESTS, asdf)
{
	VirtualCANPlugin requesterPlugin;
	requesterPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	NAME clientNAME(0);
	clientNAME.set_industry_group(2);
	clientNAME.set_function_instance(3);
	clientNAME.set_function_code(static_cast<std::uint8_t>(NAME::Function::TirePressureControl));
	auto internalECU = std::make_shared<InternalControlFunction>(clientNAME, 0x50, 0);

	HardwareInterfaceCANFrame testFrame;

	std::uint32_t waitingTimestamp_ms = SystemTiming::get_timestamp_ms();

	while ((!internalECU->get_address_valid()) &&
	       (!SystemTiming::time_expired_ms(waitingTimestamp_ms, 2000)))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	ASSERT_TRUE(internalECU->get_address_valid());

	// Force claim a partner
	testFrame.dataLength = 8;
	testFrame.channel = 0;
	testFrame.isExtendedFrame = true;
	testFrame.identifier = 0x18EEFFF7;
	testFrame.data[0] = 0x03;
	testFrame.data[1] = 0x04;
	testFrame.data[2] = 0x00;
	testFrame.data[3] = 0x12;
	testFrame.data[4] = 0x00;
	testFrame.data[5] = 0x82;
	testFrame.data[6] = 0x00;
	testFrame.data[7] = 0xA0;
	CANNetworkManager::process_receive_can_message_frame(testFrame);

	std::this_thread::sleep_for(std::chrono::milliseconds(50));

	// Get the virtual CAN plugin back to a known state
	while (!requesterPlugin.get_queue_empty())
	{
		requesterPlugin.read_frame(testFrame);
	}
	ASSERT_TRUE(requesterPlugin.get_queue_empty());

	ControlFunctionFunctionalities cfFunctionalitiesUnderTest(internalECU);
}
