#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_speed_distance_messages.hpp"
#include "isobus/utility/system_timing.hpp"

#include <cmath>

using namespace isobus;

class TestSpeedInterface : public SpeedMessagesInterface
{
public:
	TestSpeedInterface(std::shared_ptr<InternalControlFunction> source) :
	  SpeedMessagesInterface(source){

	  };

	TestSpeedInterface(std::shared_ptr<InternalControlFunction> source, bool sendGroundBasedSpeedPeriodically, bool sendWheelBasedSpeedPeriodically, bool sendMachineSelectedSpeedPeriodically) :
	  SpeedMessagesInterface(source, sendGroundBasedSpeedPeriodically, sendWheelBasedSpeedPeriodically, sendMachineSelectedSpeedPeriodically){

	  };

	void test_wrapper_set_flag(std::uint32_t flag)
	{
		txFlags.set_flag(flag);
	}

	bool test_wrapper_send_machine_selected_speed() const
	{
		return send_machine_selected_speed();
	}

	bool test_wrapper_send_wheel_based_speed() const
	{
		return send_wheel_based_speed();
	}

	bool test_wrapper_send_ground_based_speed() const
	{
		return send_ground_based_speed();
	}

	static void test_mss_callback(const std::shared_ptr<MachineSelectedSpeedData>, bool)
	{
		wasMSSCallbackHit = true;
	}

	static void test_wbs_callback(const std::shared_ptr<WheelBasedMachineSpeedData>, bool)
	{
		wasWBSCallbackHit = true;
	}

	static void test_gbs_callback(const std::shared_ptr<GroundBasedSpeedData>, bool)
	{
		wasGBSCallbackHit = true;
	}

	static bool wasMSSCallbackHit;
	static bool wasWBSCallbackHit;
	static bool wasGBSCallbackHit;
};

bool TestSpeedInterface::wasMSSCallbackHit = false;
bool TestSpeedInterface::wasWBSCallbackHit = false;
bool TestSpeedInterface::wasGBSCallbackHit = false;

TEST(SPEED_MESSAGE_TESTS, SpeedMessages)
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
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::Alarm1SystemControlForMarineEngines));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(5);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);

	auto testECU = std::make_shared<isobus::InternalControlFunction>(TestDeviceNAME, 0x45, 0);

	CANMessageFrame testFrame;
	testFrame.timestamp_us = 0;
	testFrame.identifier = 0;
	testFrame.channel = 0;
	std::memset(testFrame.data, 0, sizeof(testFrame.data));
	testFrame.dataLength = 0; ///< The length of the data used in the frame
	testFrame.isExtendedFrame = true; ///< Denotes if the frame is extended format

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

	{
		TestSpeedInterface interfaceUnderTest(testECU);
		// Sends should fail because we did not configure them to be sent in this test
		EXPECT_FALSE(interfaceUnderTest.test_wrapper_send_ground_based_speed());
		EXPECT_FALSE(interfaceUnderTest.test_wrapper_send_machine_selected_speed());
		EXPECT_FALSE(interfaceUnderTest.test_wrapper_send_wheel_based_speed());

		// Test fresh state
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_ground_based_speed_sources());
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_machine_selected_speed_sources());
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_wheel_based_speed_sources());
		EXPECT_EQ(nullptr, interfaceUnderTest.get_received_ground_based_speed(0));
		EXPECT_EQ(nullptr, interfaceUnderTest.get_received_machine_selected_speed(0));
		EXPECT_EQ(nullptr, interfaceUnderTest.get_received_wheel_based_speed(0));
		interfaceUnderTest.test_wrapper_set_flag(0);
		interfaceUnderTest.update(); // Nothing should happen, since not initialized yet
		EXPECT_TRUE(testPlugin.get_queue_empty());

		EXPECT_EQ(63, interfaceUnderTest.machineSelectedSpeedTransmitData.get_exit_reason_code());
		EXPECT_EQ(SpeedMessagesInterface::MachineSelectedSpeedData::LimitStatus::NotAvailable, interfaceUnderTest.machineSelectedSpeedTransmitData.get_limit_status());
		EXPECT_EQ(SpeedMessagesInterface::MachineDirection::NotAvailable, interfaceUnderTest.machineSelectedSpeedTransmitData.get_machine_direction_of_travel());
		EXPECT_EQ(0, interfaceUnderTest.machineSelectedSpeedTransmitData.get_machine_distance());
		EXPECT_EQ(0, interfaceUnderTest.machineSelectedSpeedTransmitData.get_machine_speed());
		EXPECT_EQ(nullptr, interfaceUnderTest.machineSelectedSpeedTransmitData.get_sender_control_function());
		EXPECT_EQ(0, interfaceUnderTest.machineSelectedSpeedTransmitData.get_timestamp_ms());
		EXPECT_EQ(SpeedMessagesInterface::MachineSelectedSpeedData::SpeedSource::NotAvailable, interfaceUnderTest.machineSelectedSpeedTransmitData.get_speed_source());

		EXPECT_EQ(SpeedMessagesInterface::WheelBasedMachineSpeedData::ImplementStartStopOperations::NotAvailable, interfaceUnderTest.wheelBasedSpeedTransmitData.get_implement_start_stop_operations_state());
		EXPECT_EQ(SpeedMessagesInterface::WheelBasedMachineSpeedData::KeySwitchState::NotAvailable, interfaceUnderTest.wheelBasedSpeedTransmitData.get_key_switch_state());
		EXPECT_EQ(SpeedMessagesInterface::WheelBasedMachineSpeedData::OperatorDirectionReversed::NotAvailable, interfaceUnderTest.wheelBasedSpeedTransmitData.get_operator_direction_reversed_state());
		EXPECT_EQ(SpeedMessagesInterface::MachineDirection::NotAvailable, interfaceUnderTest.wheelBasedSpeedTransmitData.get_machine_direction_of_travel());
		EXPECT_EQ(0, interfaceUnderTest.wheelBasedSpeedTransmitData.get_machine_distance());
		EXPECT_EQ(0, interfaceUnderTest.wheelBasedSpeedTransmitData.get_timestamp_ms());
		EXPECT_EQ(0, interfaceUnderTest.wheelBasedSpeedTransmitData.get_machine_speed());
		EXPECT_EQ(0, interfaceUnderTest.wheelBasedSpeedTransmitData.get_maximum_time_of_tractor_power());
		EXPECT_EQ(nullptr, interfaceUnderTest.wheelBasedSpeedTransmitData.get_sender_control_function());

		EXPECT_EQ(0, interfaceUnderTest.groundBasedSpeedTransmitData.get_machine_distance());
		EXPECT_EQ(0, interfaceUnderTest.groundBasedSpeedTransmitData.get_timestamp_ms());
		EXPECT_EQ(0, interfaceUnderTest.groundBasedSpeedTransmitData.get_machine_speed());
		EXPECT_EQ(nullptr, interfaceUnderTest.groundBasedSpeedTransmitData.get_sender_control_function());
		EXPECT_EQ(SpeedMessagesInterface::MachineDirection::NotAvailable, interfaceUnderTest.groundBasedSpeedTransmitData.get_machine_direction_of_travel());
	}

	{
		TestSpeedInterface interfaceUnderTest(testECU, false, false, true); // Configure MSS to be sent only

		interfaceUnderTest.machineSelectedSpeedTransmitData.set_exit_reason_code(15);
		interfaceUnderTest.machineSelectedSpeedTransmitData.set_limit_status(SpeedMessagesInterface::MachineSelectedSpeedData::LimitStatus::LimitedLow);
		interfaceUnderTest.machineSelectedSpeedTransmitData.set_machine_direction_of_travel(SpeedMessagesInterface::MachineDirection::Forward);
		interfaceUnderTest.machineSelectedSpeedTransmitData.set_machine_distance(123456);
		interfaceUnderTest.machineSelectedSpeedTransmitData.set_machine_speed(1000);
		interfaceUnderTest.machineSelectedSpeedTransmitData.set_speed_source(SpeedMessagesInterface::MachineSelectedSpeedData::SpeedSource::NavigationBasedSpeed);

		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_ground_based_speed());
		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_wheel_based_speed());
		ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_machine_selected_speed());
		ASSERT_TRUE(testPlugin.read_frame(testFrame));

		// Validate MSS encoding
		EXPECT_EQ(0, testFrame.channel);
		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(0x0CF02245, testFrame.identifier); // Verify priority 3 and PGN is F022

		std::uint16_t decodedSpeed_mm_s = static_cast<std::uint16_t>(testFrame.data[0]) | (static_cast<std::uint16_t>(testFrame.data[1]) << 8);
		EXPECT_EQ(1000, decodedSpeed_mm_s);

		std::uint32_t decodedDistance_mm = (static_cast<std::uint16_t>(testFrame.data[2]) |
		                                    (static_cast<std::uint16_t>(testFrame.data[3]) << 8) |
		                                    (static_cast<std::uint16_t>(testFrame.data[4]) << 16) |
		                                    (static_cast<std::uint16_t>(testFrame.data[5]) << 24));
		EXPECT_EQ(123456, decodedDistance_mm);
		EXPECT_EQ(testFrame.data[6] & 0x3F, 15); // Reason == 15?
		EXPECT_EQ(testFrame.data[7] & 0x03, 0); // Direction == forward?
		EXPECT_EQ((testFrame.data[7] >> 2) & 0x07, 2); // Source == navigation?
		EXPECT_EQ((testFrame.data[7] >> 5) & 0x07, 3); // low limited?
	}

	{
		TestSpeedInterface interfaceUnderTest(testECU, false, true, false); // Configure wheel speed to be sent only

		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_ground_based_speed());
		ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_wheel_based_speed());
		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_machine_selected_speed());
		ASSERT_TRUE(testPlugin.read_frame(testFrame));
	}

	{
		TestSpeedInterface interfaceUnderTest(testECU, true, false, false); // Configure ground speed to be sent only

		ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_ground_based_speed());
		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_wheel_based_speed());
		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_machine_selected_speed());
		ASSERT_TRUE(testPlugin.read_frame(testFrame));
	}
}

TEST(SPEED_MESSAGE_TESTS, ListenOnlyModeAndDecoding)
{
	TestSpeedInterface interfaceUnderTest(nullptr);
	CANMessageFrame testFrame;

	testFrame.timestamp_us = 0;
	testFrame.identifier = 0;
	testFrame.channel = 0;
	std::memset(testFrame.data, 0, sizeof(testFrame.data));
	testFrame.dataLength = 0;
	testFrame.isExtendedFrame = true;

	ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_ground_based_speed());
	ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_wheel_based_speed());
	ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_machine_selected_speed());

	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(false, interfaceUnderTest.get_initialized());
	interfaceUnderTest.initialize();
	EXPECT_EQ(true, interfaceUnderTest.get_initialized());

	EXPECT_EQ(0, interfaceUnderTest.get_number_received_ground_based_speed_sources());
	EXPECT_EQ(0, interfaceUnderTest.get_number_received_machine_selected_speed_sources());
	EXPECT_EQ(0, interfaceUnderTest.get_number_received_wheel_based_speed_sources());
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_ground_based_speed(0));
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_machine_selected_speed(0));
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_wheel_based_speed(0));

	// Force claim some other ECU
	testFrame.dataLength = 8;
	testFrame.channel = 0;
	testFrame.isExtendedFrame = true;
	testFrame.identifier = 0x18EEFF46;
	testFrame.data[0] = 0x03;
	testFrame.data[1] = 0x05;
	testFrame.data[2] = 0x04;
	testFrame.data[3] = 0x12;
	testFrame.data[4] = 0x00;
	testFrame.data[5] = 0x82;
	testFrame.data[6] = 0x01;
	testFrame.data[7] = 0xA0;
	CANNetworkManager::process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	// Register callbacks to test
	auto mssListener = interfaceUnderTest.get_machine_selected_speed_data_event_publisher().add_listener(TestSpeedInterface::test_mss_callback);
	auto wheelSpeedListener = interfaceUnderTest.get_wheel_based_machine_speed_data_event_publisher().add_listener(TestSpeedInterface::test_wbs_callback);
	auto groundSpeedListener = interfaceUnderTest.get_ground_based_machine_speed_data_event_publisher().add_listener(TestSpeedInterface::test_gbs_callback);
	EXPECT_EQ(false, TestSpeedInterface::wasGBSCallbackHit);
	EXPECT_EQ(false, TestSpeedInterface::wasMSSCallbackHit);
	EXPECT_EQ(false, TestSpeedInterface::wasWBSCallbackHit);
}
