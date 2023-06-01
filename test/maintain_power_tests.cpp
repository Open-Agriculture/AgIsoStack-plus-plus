#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_maintain_power_interface.hpp"
#include "isobus/utility/system_timing.hpp"

#include <cmath>

using namespace isobus;

class TestMaintainPowerInterface : public MaintainPowerInterface
{
public:
	TestMaintainPowerInterface(std::shared_ptr<InternalControlFunction> source) :
	  MaintainPowerInterface(source){

	  };

	bool test_wrapper_send_maintain_power() const
	{
		return send_maintain_power();
	}

	void test_wrapper_set_flag(std::uint32_t flag)
	{
		txFlags.set_flag(flag);
	}

	static bool wasCallbackHit;
};

bool TestMaintainPowerInterface::wasCallbackHit = false;

TEST(MAINTAIN_POWER_TESTS, MessageParsing)
{
	TestMaintainPowerInterface interfaceUnderTest(nullptr);
	CANMessageFrame testFrame = { 0 };
	testFrame.isExtendedFrame = true;

	ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_maintain_power());
}

TEST(MAINTAIN_POWER_TESTS, MessageEncoding)
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
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::FanDriveControl));
	TestDeviceNAME.set_identity_number(8);
	TestDeviceNAME.set_ecu_instance(5);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);

	auto testECU = std::make_shared<isobus::InternalControlFunction>(TestDeviceNAME, 0x48, 0);
	std::uint32_t waitingTimestamp_ms = SystemTiming::get_timestamp_ms();

	while ((!testECU->get_address_valid()) &&
	       (!SystemTiming::time_expired_ms(waitingTimestamp_ms, 2000)))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	ASSERT_TRUE(testECU->get_address_valid());

	CANMessageFrame testFrame = { 0 };
	testFrame.isExtendedFrame = true;

	// Get the virtual CAN plugin back to a known state
	while (!testPlugin.get_queue_empty())
	{
		testPlugin.read_frame(testFrame);
	}
	ASSERT_TRUE(testPlugin.get_queue_empty());

	TestMaintainPowerInterface interfaceUnderTest(testECU);

	// Test fresh state
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::ImplementInWorkState::NotAvailable, interfaceUnderTest.maintainPowerTransmitData.get_implement_in_work_state());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::ImplementParkState::NotAvailable, interfaceUnderTest.maintainPowerTransmitData.get_implement_park_state());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::ImplementReadyToWorkState::NotAvailable, interfaceUnderTest.maintainPowerTransmitData.get_implement_ready_to_work_state());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::ImplementTransportState::NotAvailable, interfaceUnderTest.maintainPowerTransmitData.get_implement_transport_state());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::MaintainActuatorPower::DontCare, interfaceUnderTest.maintainPowerTransmitData.get_maintain_actuator_power());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::MaintainECUPower::DontCare, interfaceUnderTest.maintainPowerTransmitData.get_maintain_ecu_power());
	EXPECT_NE(nullptr, interfaceUnderTest.maintainPowerTransmitData.get_sender_control_function());
	EXPECT_EQ(0, interfaceUnderTest.maintainPowerTransmitData.get_timestamp_ms());
	interfaceUnderTest.test_wrapper_set_flag(0);
	interfaceUnderTest.update(); // Nothing should happen, since not initialized yet
	EXPECT_TRUE(testPlugin.get_queue_empty());

	EXPECT_EQ(false, interfaceUnderTest.get_initialized());
	interfaceUnderTest.initialize();
	EXPECT_EQ(true, interfaceUnderTest.get_initialized());
}
