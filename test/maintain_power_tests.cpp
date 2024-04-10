#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_maintain_power_interface.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"

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

	static void test_maintain_power_callback(const std::shared_ptr<MaintainPowerData>, bool)
	{
		wasCallbackHit = true;
	}

	static void test_key_switch_callback()
	{
		wasKeySwitchTransitionCallbackHit = true;
	}

	static bool wasCallbackHit;
	static bool wasKeySwitchTransitionCallbackHit;
};

bool TestMaintainPowerInterface::wasCallbackHit = false;
bool TestMaintainPowerInterface::wasKeySwitchTransitionCallbackHit = false;

TEST(MAINTAIN_POWER_TESTS, MessageParsing)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	auto testECU = test_helpers::claim_internal_control_function(0x82, 0);
	TestMaintainPowerInterface interfaceUnderTest(testECU);

	EXPECT_FALSE(interfaceUnderTest.get_initialized());
	interfaceUnderTest.initialize();
	EXPECT_TRUE(interfaceUnderTest.get_initialized());

	CANMessageFrame testFrame;
	memset(&testFrame, 0, sizeof(testFrame));
	testFrame.isExtendedFrame = true;

	// Get the virtual CAN plugin back to a known state
	while (!testPlugin.get_queue_empty())
	{
		testPlugin.read_frame(testFrame);
	}
	ASSERT_TRUE(testPlugin.get_queue_empty());

	test_helpers::force_claim_partnered_control_function(0x49, 0);

	EXPECT_EQ(0, interfaceUnderTest.get_number_received_maintain_power_sources());
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_maintain_power(0));

	interfaceUnderTest.get_maintain_power_data_event_publisher().add_listener(TestMaintainPowerInterface::test_maintain_power_callback);
	interfaceUnderTest.get_key_switch_transition_off_event_publisher().add_listener(TestMaintainPowerInterface::test_key_switch_callback);
	EXPECT_FALSE(TestMaintainPowerInterface::wasCallbackHit);

	// Construct a maintain power message
	testFrame.identifier = 0x18FE4749;
	testFrame.data[0] = 0x5F;
	testFrame.data[1] = 0x55;
	testFrame.data[2] = 0xFF;
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0xFF;
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0xFF;
	testFrame.data[7] = 0xFF;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(1, interfaceUnderTest.get_number_received_maintain_power_sources());
	auto receivedData = interfaceUnderTest.get_received_maintain_power(0);
	EXPECT_NE(nullptr, receivedData);

	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::ImplementInWorkState::ImplementInWorkState, receivedData->get_implement_in_work_state());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::ImplementParkState::ImplementMayBeDisconnected, receivedData->get_implement_park_state());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::ImplementReadyToWorkState::ImplementReadyForFieldWork, receivedData->get_implement_ready_to_work_state());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::ImplementTransportState::ImplementMayBeTransported, receivedData->get_implement_transport_state());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::MaintainActuatorPower::RequirementFor2SecondsMoreForPWR, receivedData->get_maintain_actuator_power());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::MaintainECUPower::RequirementFor2SecondsMoreForECU_PWR, receivedData->get_maintain_ecu_power());
	EXPECT_TRUE(TestMaintainPowerInterface::wasCallbackHit);
	EXPECT_FALSE(TestMaintainPowerInterface::wasKeySwitchTransitionCallbackHit);
	TestMaintainPowerInterface::wasCallbackHit = false;

	// Retest with all zeros
	testFrame.identifier = 0x18FE4749;
	testFrame.data[0] = 0x0F;
	testFrame.data[1] = 0x00;
	testFrame.data[2] = 0xFF;
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0xFF;
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0xFF;
	testFrame.data[7] = 0xFF;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(1, interfaceUnderTest.get_number_received_maintain_power_sources());
	EXPECT_NE(nullptr, interfaceUnderTest.get_received_maintain_power(0));
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::ImplementInWorkState::ImplementNotInWorkState, receivedData->get_implement_in_work_state());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::ImplementParkState::ImplementMayNotBeDisconnected, receivedData->get_implement_park_state());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::ImplementReadyToWorkState::ImplementNotReadyForFieldWork, receivedData->get_implement_ready_to_work_state());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::ImplementTransportState::ImplementMayNotBeTransported, receivedData->get_implement_transport_state());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::MaintainActuatorPower::NoFurtherRequirementForPWR, receivedData->get_maintain_actuator_power());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::MaintainECUPower::NoFurtherRequirementForECU_PWR, receivedData->get_maintain_ecu_power());
	EXPECT_TRUE(TestMaintainPowerInterface::wasCallbackHit);
	EXPECT_FALSE(TestMaintainPowerInterface::wasKeySwitchTransitionCallbackHit);

	interfaceUnderTest.set_maintain_power_time(3000); // Send the maintain power message for 3 seconds (this will provide around 4s of extra power)
	EXPECT_EQ(3000, interfaceUnderTest.get_maintain_power_time());

	// Test detection of key state
	testFrame.identifier = 0x0CFE4849;
	std::uint32_t encodedDistance = 965742;
	std::uint16_t encodedSpeed = 4000;
	testFrame.data[0] = static_cast<std::uint8_t>(encodedSpeed & 0xFF);
	testFrame.data[1] = static_cast<std::uint8_t>((encodedSpeed >> 8) & 0xFF);
	testFrame.data[2] = static_cast<std::uint8_t>(encodedDistance & 0xFF);
	testFrame.data[3] = static_cast<std::uint8_t>((encodedDistance >> 8) & 0xFF);
	testFrame.data[4] = static_cast<std::uint8_t>((encodedDistance >> 16) & 0xFF);
	testFrame.data[5] = static_cast<std::uint8_t>((encodedDistance >> 24) & 0xFF);
	testFrame.data[6] = 200; // Max time of tractor power
	testFrame.data[7] = 0x55; // All parameters set to 1

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_FALSE(TestMaintainPowerInterface::wasKeySwitchTransitionCallbackHit);

	testFrame.data[7] = 0x00; // Turn all parameters off

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_TRUE(TestMaintainPowerInterface::wasKeySwitchTransitionCallbackHit);
	TestMaintainPowerInterface::wasKeySwitchTransitionCallbackHit = false;
	interfaceUnderTest.update();

	testPlugin.read_frame(testFrame); // This one is our wheel based speed, so discard that
	testPlugin.read_frame(testFrame);

	// Now that there's been a transition, we should be able to receive a maintain power from our interface
	EXPECT_EQ(0x18FE4782, testFrame.identifier);

	// If we wait for 1-ish second, we should get another
	std::this_thread::sleep_for(std::chrono::milliseconds(1060));
	interfaceUnderTest.update();

	EXPECT_TRUE(testPlugin.read_frame(testFrame));

	EXPECT_EQ(0x18FE4782, testFrame.identifier);
	EXPECT_TRUE(testPlugin.get_queue_empty());

	// If we wait for 1-ish second, we should get third
	std::this_thread::sleep_for(std::chrono::milliseconds(1060));
	interfaceUnderTest.update();

	EXPECT_EQ(0x18FE4782, testFrame.identifier);
	EXPECT_TRUE(testPlugin.get_queue_empty());

	// Send all errors, and ensure we don't get a callback for a transition
	testFrame.identifier = 0x0CFE4849;
	testFrame.data[0] = static_cast<std::uint8_t>(encodedSpeed & 0xFF);
	testFrame.data[1] = static_cast<std::uint8_t>((encodedSpeed >> 8) & 0xFF);
	testFrame.data[2] = static_cast<std::uint8_t>(encodedDistance & 0xFF);
	testFrame.data[3] = static_cast<std::uint8_t>((encodedDistance >> 8) & 0xFF);
	testFrame.data[4] = static_cast<std::uint8_t>((encodedDistance >> 16) & 0xFF);
	testFrame.data[5] = static_cast<std::uint8_t>((encodedDistance >> 24) & 0xFF);
	testFrame.data[6] = 200;
	testFrame.data[7] = 0xAA; // All parameters set to 0
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	EXPECT_FALSE(TestMaintainPowerInterface::wasKeySwitchTransitionCallbackHit);

	// Test that transition from any state that isn't "not off" to off doesn't cause a callback
	// Send all errors, and ensure we don't get a callback for a transition
	testFrame.data[7] = 0x55; // All parameters set to 1
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	EXPECT_FALSE(TestMaintainPowerInterface::wasKeySwitchTransitionCallbackHit);

	CANNetworkManager::CANNetwork.deactivate_control_function(testECU);
	CANHardwareInterface::stop();
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
	TestDeviceNAME.set_manufacturer_code(1407);

	auto testECU = test_helpers::claim_internal_control_function(0x48, 0);

	CANMessageFrame testFrame;
	memset(&testFrame, 0, sizeof(testFrame));
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

	EXPECT_TRUE(interfaceUnderTest.maintainPowerTransmitData.set_implement_in_work_state(MaintainPowerInterface::MaintainPowerData::ImplementInWorkState::ImplementInWorkState));
	EXPECT_TRUE(interfaceUnderTest.maintainPowerTransmitData.set_implement_park_state(MaintainPowerInterface::MaintainPowerData::ImplementParkState::ImplementMayNotBeDisconnected));
	EXPECT_TRUE(interfaceUnderTest.maintainPowerTransmitData.set_implement_ready_to_work_state(MaintainPowerInterface::MaintainPowerData::ImplementReadyToWorkState::ImplementReadyForFieldWork));
	EXPECT_TRUE(interfaceUnderTest.maintainPowerTransmitData.set_implement_transport_state(MaintainPowerInterface::MaintainPowerData::ImplementTransportState::ImplementMayNotBeTransported));
	EXPECT_TRUE(interfaceUnderTest.maintainPowerTransmitData.set_maintain_actuator_power(MaintainPowerInterface::MaintainPowerData::MaintainActuatorPower::RequirementFor2SecondsMoreForPWR));
	EXPECT_TRUE(interfaceUnderTest.maintainPowerTransmitData.set_maintain_ecu_power(MaintainPowerInterface::MaintainPowerData::MaintainECUPower::RequirementFor2SecondsMoreForECU_PWR));

	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::ImplementInWorkState::ImplementInWorkState, interfaceUnderTest.maintainPowerTransmitData.get_implement_in_work_state());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::ImplementParkState::ImplementMayNotBeDisconnected, interfaceUnderTest.maintainPowerTransmitData.get_implement_park_state());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::ImplementReadyToWorkState::ImplementReadyForFieldWork, interfaceUnderTest.maintainPowerTransmitData.get_implement_ready_to_work_state());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::ImplementTransportState::ImplementMayNotBeTransported, interfaceUnderTest.maintainPowerTransmitData.get_implement_transport_state());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::MaintainActuatorPower::RequirementFor2SecondsMoreForPWR, interfaceUnderTest.maintainPowerTransmitData.get_maintain_actuator_power());
	EXPECT_EQ(MaintainPowerInterface::MaintainPowerData::MaintainECUPower::RequirementFor2SecondsMoreForECU_PWR, interfaceUnderTest.maintainPowerTransmitData.get_maintain_ecu_power());

	interfaceUnderTest.test_wrapper_set_flag(0);
	interfaceUnderTest.update();
	testPlugin.read_frame(testFrame);

	EXPECT_EQ(8, testFrame.dataLength);
	EXPECT_TRUE(testFrame.isExtendedFrame);
	EXPECT_EQ(0x18FE4748, testFrame.identifier);
	EXPECT_EQ(0x01, (testFrame.data[0] >> 4) & 0x03);
	EXPECT_EQ(0x01, (testFrame.data[0] >> 6) & 0x03);
	EXPECT_EQ(0x01, (testFrame.data[1]) & 0x03);
	EXPECT_EQ(0x01, (testFrame.data[1] >> 2) & 0x03);
	EXPECT_EQ(0x00, (testFrame.data[1] >> 4) & 0x03);
	EXPECT_EQ(0x00, (testFrame.data[1] >> 6) & 0x03);
	EXPECT_EQ(0xFF, testFrame.data[2]);
	EXPECT_EQ(0xFF, testFrame.data[3]);
	EXPECT_EQ(0xFF, testFrame.data[4]);
	EXPECT_EQ(0xFF, testFrame.data[5]);
	EXPECT_EQ(0xFF, testFrame.data[6]);
	EXPECT_EQ(0xFF, testFrame.data[7]);

	testPlugin.close();

	CANNetworkManager::CANNetwork.deactivate_control_function(testECU);
	CANHardwareInterface::stop();
}
