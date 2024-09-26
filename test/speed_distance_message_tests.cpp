#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_speed_distance_messages.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"

#include <cmath>

using namespace isobus;

class TestSpeedInterface : public SpeedMessagesInterface
{
public:
	TestSpeedInterface(std::shared_ptr<InternalControlFunction> source) :
	  SpeedMessagesInterface(source){

	  };

	TestSpeedInterface(std::shared_ptr<InternalControlFunction> source,
	                   bool sendGroundBasedSpeedPeriodically,
	                   bool sendWheelBasedSpeedPeriodically,
	                   bool sendMachineSelectedSpeedPeriodically,
	                   bool sendMachineSelectedSpeedCommandPeriodically) :
	  SpeedMessagesInterface(source, sendGroundBasedSpeedPeriodically, sendWheelBasedSpeedPeriodically, sendMachineSelectedSpeedPeriodically, sendMachineSelectedSpeedCommandPeriodically){

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

	bool test_wrapper_send_machine_selected_speed_command() const
	{
		return send_machine_selected_speed_command();
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

	static void test_command_callback(const std::shared_ptr<MachineSelectedSpeedCommandData>, bool)
	{
		wasCommandCallbackHit = true;
	}

	static bool wasMSSCallbackHit;
	static bool wasWBSCallbackHit;
	static bool wasGBSCallbackHit;
	static bool wasCommandCallbackHit;
};

bool TestSpeedInterface::wasMSSCallbackHit = false;
bool TestSpeedInterface::wasWBSCallbackHit = false;
bool TestSpeedInterface::wasGBSCallbackHit = false;
bool TestSpeedInterface::wasCommandCallbackHit = false;

TEST(SPEED_MESSAGE_TESTS, SpeedMessages)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	auto testECU = test_helpers::claim_internal_control_function(0x45, 0);
	ASSERT_TRUE(testECU->get_address_valid());

	// Get the virtual CAN plugin back to a known state
	CANMessageFrame testFrame = {};
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
		EXPECT_FALSE(interfaceUnderTest.test_wrapper_send_machine_selected_speed_command());

		// Test fresh state
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_ground_based_speed_sources());
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_machine_selected_speed_sources());
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_wheel_based_speed_sources());
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_machine_selected_speed_command_sources());
		EXPECT_EQ(nullptr, interfaceUnderTest.get_received_ground_based_speed(0));
		EXPECT_EQ(nullptr, interfaceUnderTest.get_received_machine_selected_speed(0));
		EXPECT_EQ(nullptr, interfaceUnderTest.get_received_wheel_based_speed(0));
		EXPECT_EQ(nullptr, interfaceUnderTest.get_received_machine_selected_speed_command(0));
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
		TestSpeedInterface interfaceUnderTest(testECU, false, false, true, false); // Configure MSS to be sent only

		interfaceUnderTest.machineSelectedSpeedTransmitData.set_exit_reason_code(15);
		interfaceUnderTest.machineSelectedSpeedTransmitData.set_limit_status(SpeedMessagesInterface::MachineSelectedSpeedData::LimitStatus::LimitedLow);
		interfaceUnderTest.machineSelectedSpeedTransmitData.set_machine_direction_of_travel(SpeedMessagesInterface::MachineDirection::Forward);
		interfaceUnderTest.machineSelectedSpeedTransmitData.set_machine_distance(123456);
		interfaceUnderTest.machineSelectedSpeedTransmitData.set_machine_speed(1000);
		interfaceUnderTest.machineSelectedSpeedTransmitData.set_speed_source(SpeedMessagesInterface::MachineSelectedSpeedData::SpeedSource::NavigationBasedSpeed);

		EXPECT_EQ(15, interfaceUnderTest.machineSelectedSpeedTransmitData.get_exit_reason_code());
		EXPECT_EQ(SpeedMessagesInterface::MachineSelectedSpeedData::LimitStatus::LimitedLow, interfaceUnderTest.machineSelectedSpeedTransmitData.get_limit_status());
		EXPECT_EQ(SpeedMessagesInterface::MachineDirection::Forward, interfaceUnderTest.machineSelectedSpeedTransmitData.get_machine_direction_of_travel());
		EXPECT_EQ(123456, interfaceUnderTest.machineSelectedSpeedTransmitData.get_machine_distance());
		EXPECT_EQ(1000, interfaceUnderTest.machineSelectedSpeedTransmitData.get_machine_speed());
		EXPECT_EQ(SpeedMessagesInterface::MachineSelectedSpeedData::SpeedSource::NavigationBasedSpeed, interfaceUnderTest.machineSelectedSpeedTransmitData.get_speed_source());

		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_ground_based_speed());
		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_wheel_based_speed());
		ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_machine_selected_speed());
		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_machine_selected_speed_command());
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
		EXPECT_EQ(testFrame.data[7] & 0x03, 1); // Direction == forward?
		EXPECT_EQ((testFrame.data[7] >> 2) & 0x07, 2); // Source == navigation?
		EXPECT_EQ((testFrame.data[7] >> 5) & 0x07, 3); // low limited?

		// Test above max values
		interfaceUnderTest.machineSelectedSpeedTransmitData.set_machine_distance(4211081216);
		interfaceUnderTest.machineSelectedSpeedTransmitData.set_machine_speed(65534);
		EXPECT_EQ(0, interfaceUnderTest.machineSelectedSpeedTransmitData.get_machine_distance());
		EXPECT_EQ(0, interfaceUnderTest.machineSelectedSpeedTransmitData.get_machine_speed());
	}

	{
		TestSpeedInterface interfaceUnderTest(testECU, false, true, false, false); // Configure wheel speed to be sent only

		interfaceUnderTest.wheelBasedSpeedTransmitData.set_implement_start_stop_operations_state(SpeedMessagesInterface::WheelBasedMachineSpeedData::ImplementStartStopOperations::StartEnableImplementOperations);
		interfaceUnderTest.wheelBasedSpeedTransmitData.set_key_switch_state(SpeedMessagesInterface::WheelBasedMachineSpeedData::KeySwitchState::NotOff);
		interfaceUnderTest.wheelBasedSpeedTransmitData.set_machine_direction_of_travel(SpeedMessagesInterface::MachineDirection::Reverse);
		interfaceUnderTest.wheelBasedSpeedTransmitData.set_machine_distance(5000);
		interfaceUnderTest.wheelBasedSpeedTransmitData.set_machine_speed(9876);
		interfaceUnderTest.wheelBasedSpeedTransmitData.set_maximum_time_of_tractor_power(3);
		interfaceUnderTest.wheelBasedSpeedTransmitData.set_operator_direction_reversed_state(SpeedMessagesInterface::WheelBasedMachineSpeedData::OperatorDirectionReversed::NotReversed);

		EXPECT_EQ(SpeedMessagesInterface::WheelBasedMachineSpeedData::ImplementStartStopOperations::StartEnableImplementOperations, interfaceUnderTest.wheelBasedSpeedTransmitData.get_implement_start_stop_operations_state());
		EXPECT_EQ(SpeedMessagesInterface::WheelBasedMachineSpeedData::KeySwitchState::NotOff, interfaceUnderTest.wheelBasedSpeedTransmitData.get_key_switch_state());
		EXPECT_EQ(SpeedMessagesInterface::MachineDirection::Reverse, interfaceUnderTest.wheelBasedSpeedTransmitData.get_machine_direction_of_travel());
		EXPECT_EQ(5000, interfaceUnderTest.wheelBasedSpeedTransmitData.get_machine_distance());
		EXPECT_EQ(9876, interfaceUnderTest.wheelBasedSpeedTransmitData.get_machine_speed());
		EXPECT_EQ(3, interfaceUnderTest.wheelBasedSpeedTransmitData.get_maximum_time_of_tractor_power());
		EXPECT_EQ(SpeedMessagesInterface::WheelBasedMachineSpeedData::OperatorDirectionReversed::NotReversed, interfaceUnderTest.wheelBasedSpeedTransmitData.get_operator_direction_reversed_state());

		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_ground_based_speed());
		ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_wheel_based_speed());
		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_machine_selected_speed());
		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_machine_selected_speed_command());
		ASSERT_TRUE(testPlugin.read_frame(testFrame));

		// Validate wheel-based speed encoding
		EXPECT_EQ(0, testFrame.channel);
		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(true, testFrame.isExtendedFrame);
		EXPECT_EQ(0x0CFE4845, testFrame.identifier); // Verify priority 3 and PGN is FE48

		std::uint16_t decodedSpeed_mm_s = static_cast<std::uint16_t>(testFrame.data[0]) | (static_cast<std::uint16_t>(testFrame.data[1]) << 8);
		EXPECT_EQ(9876, decodedSpeed_mm_s);

		std::uint32_t decodedDistance_mm = (static_cast<std::uint16_t>(testFrame.data[2]) |
		                                    (static_cast<std::uint16_t>(testFrame.data[3]) << 8) |
		                                    (static_cast<std::uint16_t>(testFrame.data[4]) << 16) |
		                                    (static_cast<std::uint16_t>(testFrame.data[5]) << 24));
		EXPECT_EQ(5000, decodedDistance_mm);
		EXPECT_EQ(3, testFrame.data[6]);
		EXPECT_EQ(testFrame.data[7] & 0x03, 0); // Direction == reverse?
		EXPECT_EQ((testFrame.data[7] >> 2) & 0x03, 1); // Key not off?
		EXPECT_EQ((testFrame.data[7] >> 4) & 0x03, 1); // Implement operations permitted?
		EXPECT_EQ((testFrame.data[7] >> 6) & 0x03, 0); // Not reversed?

		// Test above max values
		interfaceUnderTest.wheelBasedSpeedTransmitData.set_machine_distance(4211081216);
		interfaceUnderTest.wheelBasedSpeedTransmitData.set_machine_speed(65534);
		EXPECT_EQ(0, interfaceUnderTest.wheelBasedSpeedTransmitData.get_machine_distance());
		EXPECT_EQ(0, interfaceUnderTest.wheelBasedSpeedTransmitData.get_machine_speed());
	}

	{
		TestSpeedInterface interfaceUnderTest(testECU, true, false, false, false); // Configure ground speed to be sent only

		interfaceUnderTest.groundBasedSpeedTransmitData.set_machine_direction_of_travel(SpeedMessagesInterface::MachineDirection::Forward);
		interfaceUnderTest.groundBasedSpeedTransmitData.set_machine_distance(80000);
		interfaceUnderTest.groundBasedSpeedTransmitData.set_machine_speed(9999);

		EXPECT_EQ(SpeedMessagesInterface::MachineDirection::Forward, interfaceUnderTest.groundBasedSpeedTransmitData.get_machine_direction_of_travel());
		EXPECT_EQ(80000, interfaceUnderTest.groundBasedSpeedTransmitData.get_machine_distance());
		EXPECT_EQ(9999, interfaceUnderTest.groundBasedSpeedTransmitData.get_machine_speed());

		ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_ground_based_speed());
		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_wheel_based_speed());
		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_machine_selected_speed());
		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_machine_selected_speed_command());
		ASSERT_TRUE(testPlugin.read_frame(testFrame));

		EXPECT_EQ(0x0CFE4945, testFrame.identifier); // Verify priority 3 and PGN is FE49
		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(true, testFrame.isExtendedFrame);

		std::uint16_t decodedSpeed_mm_s = static_cast<std::uint16_t>(testFrame.data[0]) | (static_cast<std::uint16_t>(testFrame.data[1]) << 8);
		EXPECT_EQ(9999, decodedSpeed_mm_s);

		std::uint32_t decodedDistance_mm = (static_cast<std::uint16_t>(testFrame.data[2]) |
		                                    (static_cast<std::uint16_t>(testFrame.data[3]) << 8) |
		                                    (static_cast<std::uint16_t>(testFrame.data[4]) << 16) |
		                                    (static_cast<std::uint16_t>(testFrame.data[5]) << 24));
		EXPECT_EQ(80000, decodedDistance_mm);
		EXPECT_EQ(0xFF, testFrame.data[6]);
		EXPECT_EQ(static_cast<std::uint8_t>(SpeedMessagesInterface::MachineDirection::Forward), testFrame.data[7] & 0x03);

		// Test above max values
		interfaceUnderTest.groundBasedSpeedTransmitData.set_machine_distance(4211081216);
		interfaceUnderTest.groundBasedSpeedTransmitData.set_machine_speed(65534);
		EXPECT_EQ(0, interfaceUnderTest.groundBasedSpeedTransmitData.get_machine_distance());
		EXPECT_EQ(0, interfaceUnderTest.groundBasedSpeedTransmitData.get_machine_speed());
	}

	{
		TestSpeedInterface interfaceUnderTest(testECU, false, false, false, true); // Configure machine selected speed command to be sent only

		interfaceUnderTest.machineSelectedSpeedCommandTransmitData.set_machine_selected_speed_setpoint_limit(12345);
		interfaceUnderTest.machineSelectedSpeedCommandTransmitData.set_machine_speed_setpoint_command(56789);
		interfaceUnderTest.machineSelectedSpeedCommandTransmitData.set_machine_direction_of_travel(SpeedMessagesInterface::MachineDirection::Forward);

		EXPECT_EQ(12345, interfaceUnderTest.machineSelectedSpeedCommandTransmitData.get_machine_selected_speed_setpoint_limit());
		EXPECT_EQ(56789, interfaceUnderTest.machineSelectedSpeedCommandTransmitData.get_machine_speed_setpoint_command());
		EXPECT_EQ(SpeedMessagesInterface::MachineDirection::Forward, interfaceUnderTest.machineSelectedSpeedCommandTransmitData.get_machine_direction_command());

		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_ground_based_speed());
		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_wheel_based_speed());
		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_machine_selected_speed());
		ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_machine_selected_speed_command());
		ASSERT_TRUE(testPlugin.read_frame(testFrame));

		EXPECT_EQ(0x0CFD4345, testFrame.identifier); // Verify priority 3 and PGN is FD43
		EXPECT_EQ(8, testFrame.dataLength);
		EXPECT_EQ(true, testFrame.isExtendedFrame);

		std::uint16_t decodedSpeed_mm_s = static_cast<std::uint16_t>(testFrame.data[0]) | (static_cast<std::uint16_t>(testFrame.data[1]) << 8);
		EXPECT_EQ(56789, decodedSpeed_mm_s);

		std::uint16_t decodedSpeedLimit_mm_s = static_cast<std::uint16_t>(testFrame.data[2]) | (static_cast<std::uint16_t>(testFrame.data[3]) << 8);
		EXPECT_EQ(12345, decodedSpeedLimit_mm_s);

		EXPECT_EQ(0xFF, testFrame.data[4]);
		EXPECT_EQ(0xFF, testFrame.data[5]);
		EXPECT_EQ(0xFF, testFrame.data[6]);
		EXPECT_EQ(0x01, testFrame.data[7] & 0x03);
	}

	{
		// Configure all messages to be sent
		TestSpeedInterface interfaceUnderTest(testECU, true, true, true, true);
		interfaceUnderTest.initialize();
		interfaceUnderTest.update();

		std::this_thread::sleep_for(std::chrono::milliseconds(105));
		interfaceUnderTest.update();

		// Should get 4 messages every 100ms
		ASSERT_TRUE(testPlugin.read_frame(testFrame));
		ASSERT_TRUE(testPlugin.read_frame(testFrame));
		ASSERT_TRUE(testPlugin.read_frame(testFrame));
		ASSERT_TRUE(testPlugin.read_frame(testFrame));
	}

	CANNetworkManager::CANNetwork.deactivate_control_function(testECU);
	CANHardwareInterface::stop();
}

TEST(SPEED_MESSAGE_TESTS, ListenOnlyModeAndDecoding)
{
	TestSpeedInterface interfaceUnderTest(nullptr);
	CANMessageFrame testFrame = {};
	testFrame.isExtendedFrame = true;
	testFrame.dataLength = 8;

	ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_ground_based_speed());
	ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_wheel_based_speed());
	ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_machine_selected_speed());

	std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Sleep a bit for ctest to get a non zero timestamp

	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(false, interfaceUnderTest.get_initialized());
	interfaceUnderTest.initialize();
	EXPECT_EQ(true, interfaceUnderTest.get_initialized());

	EXPECT_EQ(0, interfaceUnderTest.get_number_received_ground_based_speed_sources());
	EXPECT_EQ(0, interfaceUnderTest.get_number_received_machine_selected_speed_sources());
	EXPECT_EQ(0, interfaceUnderTest.get_number_received_wheel_based_speed_sources());
	EXPECT_EQ(0, interfaceUnderTest.get_number_received_machine_selected_speed_command_sources());
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_ground_based_speed(0));
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_machine_selected_speed(0));
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_wheel_based_speed(0));
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_machine_selected_speed_command(0));

	test_helpers::force_claim_partnered_control_function(0x46, 0);

	// Register callbacks to test
	interfaceUnderTest.get_machine_selected_speed_data_event_publisher().add_listener(TestSpeedInterface::test_mss_callback);
	interfaceUnderTest.get_wheel_based_machine_speed_data_event_publisher().add_listener(TestSpeedInterface::test_wbs_callback);
	interfaceUnderTest.get_ground_based_machine_speed_data_event_publisher().add_listener(TestSpeedInterface::test_gbs_callback);
	interfaceUnderTest.get_machine_selected_speed_command_data_event_publisher().add_listener(TestSpeedInterface::test_command_callback);
	EXPECT_EQ(false, TestSpeedInterface::wasGBSCallbackHit);
	EXPECT_EQ(false, TestSpeedInterface::wasMSSCallbackHit);
	EXPECT_EQ(false, TestSpeedInterface::wasWBSCallbackHit);
	EXPECT_EQ(false, TestSpeedInterface::wasCommandCallbackHit);

	{
		// Test MSS Message
		testFrame.identifier = 0x0CF02246;

		std::uint32_t encodedDistance = 965742;
		std::uint16_t encodedSpeed = 4000;
		testFrame.data[0] = static_cast<std::uint8_t>(encodedSpeed & 0xFF);
		testFrame.data[1] = static_cast<std::uint8_t>((encodedSpeed >> 8) & 0xFF);
		testFrame.data[2] = static_cast<std::uint8_t>(encodedDistance & 0xFF);
		testFrame.data[3] = static_cast<std::uint8_t>((encodedDistance >> 8) & 0xFF);
		testFrame.data[4] = static_cast<std::uint8_t>((encodedDistance >> 16) & 0xFF);
		testFrame.data[5] = static_cast<std::uint8_t>((encodedDistance >> 24) & 0xFF);
		testFrame.data[6] = 30; // exit code
		testFrame.data[7] = 0x25; // All remaining properties set to 1

		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();

		EXPECT_EQ(false, TestSpeedInterface::wasGBSCallbackHit);
		EXPECT_EQ(true, TestSpeedInterface::wasMSSCallbackHit);
		EXPECT_EQ(false, TestSpeedInterface::wasWBSCallbackHit);
		EXPECT_EQ(false, TestSpeedInterface::wasCommandCallbackHit);
		TestSpeedInterface::wasMSSCallbackHit = false;

		EXPECT_EQ(1, interfaceUnderTest.get_number_received_machine_selected_speed_sources());
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_wheel_based_speed_sources());
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_ground_based_speed_sources());
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_machine_selected_speed_command_sources());

		auto mss = interfaceUnderTest.get_received_machine_selected_speed(0);
		ASSERT_NE(nullptr, mss);

		EXPECT_EQ(30, mss->get_exit_reason_code());
		EXPECT_EQ(965742, mss->get_machine_distance());
		EXPECT_EQ(4000, mss->get_machine_speed());
		EXPECT_EQ(SpeedMessagesInterface::MachineSelectedSpeedData::LimitStatus::OperatorLimitedControlled, mss->get_limit_status());
		EXPECT_EQ(SpeedMessagesInterface::MachineDirection::Forward, mss->get_machine_direction_of_travel());
		EXPECT_EQ(SpeedMessagesInterface::MachineSelectedSpeedData::SpeedSource::GroundBasedSpeed, mss->get_speed_source());
		EXPECT_NE(0, mss->get_timestamp_ms());
	}

	{
		// Test wheel-based speed Message
		testFrame.identifier = 0x0CFE4846;

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

		EXPECT_EQ(false, TestSpeedInterface::wasGBSCallbackHit);
		EXPECT_EQ(false, TestSpeedInterface::wasMSSCallbackHit);
		EXPECT_EQ(true, TestSpeedInterface::wasWBSCallbackHit);
		EXPECT_EQ(false, TestSpeedInterface::wasCommandCallbackHit);
		TestSpeedInterface::wasWBSCallbackHit = false;

		EXPECT_EQ(1, interfaceUnderTest.get_number_received_machine_selected_speed_sources());
		EXPECT_EQ(1, interfaceUnderTest.get_number_received_wheel_based_speed_sources());
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_ground_based_speed_sources());
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_machine_selected_speed_command_sources());

		auto wheelSpeed = interfaceUnderTest.get_received_wheel_based_speed(0);
		ASSERT_NE(nullptr, wheelSpeed);

		EXPECT_EQ(SpeedMessagesInterface::WheelBasedMachineSpeedData::ImplementStartStopOperations::StartEnableImplementOperations, wheelSpeed->get_implement_start_stop_operations_state());
		EXPECT_EQ(SpeedMessagesInterface::WheelBasedMachineSpeedData::KeySwitchState::NotOff, wheelSpeed->get_key_switch_state());
		EXPECT_EQ(SpeedMessagesInterface::WheelBasedMachineSpeedData::OperatorDirectionReversed::Reversed, wheelSpeed->get_operator_direction_reversed_state());
		EXPECT_EQ(SpeedMessagesInterface::MachineDirection::Forward, wheelSpeed->get_machine_direction_of_travel());
		EXPECT_EQ(965742, wheelSpeed->get_machine_distance());
		EXPECT_EQ(4000, wheelSpeed->get_machine_speed());
		EXPECT_EQ(200, wheelSpeed->get_maximum_time_of_tractor_power());
		EXPECT_NE(0, wheelSpeed->get_timestamp_ms());
	}

	{
		// Test ground-based speed Message
		testFrame.identifier = 0x0CFE4946;

		std::uint32_t encodedDistance = 965742;
		std::uint16_t encodedSpeed = 4000;
		testFrame.data[0] = static_cast<std::uint8_t>(encodedSpeed & 0xFF);
		testFrame.data[1] = static_cast<std::uint8_t>((encodedSpeed >> 8) & 0xFF);
		testFrame.data[2] = static_cast<std::uint8_t>(encodedDistance & 0xFF);
		testFrame.data[3] = static_cast<std::uint8_t>((encodedDistance >> 8) & 0xFF);
		testFrame.data[4] = static_cast<std::uint8_t>((encodedDistance >> 16) & 0xFF);
		testFrame.data[5] = static_cast<std::uint8_t>((encodedDistance >> 24) & 0xFF);
		testFrame.data[6] = 0xFF;
		testFrame.data[7] = 0x01; // Forward

		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();

		EXPECT_EQ(true, TestSpeedInterface::wasGBSCallbackHit);
		EXPECT_EQ(false, TestSpeedInterface::wasMSSCallbackHit);
		EXPECT_EQ(false, TestSpeedInterface::wasWBSCallbackHit);
		TestSpeedInterface::wasGBSCallbackHit = false;

		EXPECT_EQ(1, interfaceUnderTest.get_number_received_machine_selected_speed_sources());
		EXPECT_EQ(1, interfaceUnderTest.get_number_received_wheel_based_speed_sources());
		EXPECT_EQ(1, interfaceUnderTest.get_number_received_ground_based_speed_sources());
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_machine_selected_speed_command_sources());

		auto groundSpeed = interfaceUnderTest.get_received_ground_based_speed(0);
		ASSERT_NE(nullptr, groundSpeed);

		EXPECT_EQ(965742, groundSpeed->get_machine_distance());
		EXPECT_EQ(4000, groundSpeed->get_machine_speed());
		EXPECT_EQ(SpeedMessagesInterface::MachineDirection::Forward, groundSpeed->get_machine_direction_of_travel());
		EXPECT_NE(0, groundSpeed->get_timestamp_ms());
	}

	{
		// Test MSS Command
		testFrame.identifier = 0x0CFD4346;

		std::uint16_t encodedSpeed = 4000;
		std::uint16_t encodedLimit = 5000;
		testFrame.data[0] = static_cast<std::uint8_t>(encodedSpeed & 0xFF);
		testFrame.data[1] = static_cast<std::uint8_t>((encodedSpeed >> 8) & 0xFF);
		testFrame.data[2] = static_cast<std::uint8_t>(encodedLimit & 0xFF);
		testFrame.data[3] = static_cast<std::uint8_t>((encodedLimit >> 8) & 0xFF);
		testFrame.data[4] = 0xFF;
		testFrame.data[5] = 0xFF;
		testFrame.data[6] = 0xFF;
		testFrame.data[7] = 0xFC; // Direction

		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();

		EXPECT_EQ(false, TestSpeedInterface::wasGBSCallbackHit);
		EXPECT_EQ(false, TestSpeedInterface::wasMSSCallbackHit);
		EXPECT_EQ(false, TestSpeedInterface::wasWBSCallbackHit);
		EXPECT_EQ(true, TestSpeedInterface::wasCommandCallbackHit);
		TestSpeedInterface::wasCommandCallbackHit = false;

		EXPECT_EQ(1, interfaceUnderTest.get_number_received_machine_selected_speed_command_sources());

		auto command = interfaceUnderTest.get_received_machine_selected_speed_command(0);
		ASSERT_NE(nullptr, command);

		EXPECT_NE(0, command->get_timestamp_ms());
		EXPECT_EQ(SpeedMessagesInterface::MachineDirection::Reverse, command->get_machine_direction_command());
		EXPECT_EQ(5000, command->get_machine_selected_speed_setpoint_limit());
		EXPECT_EQ(4000, command->get_machine_speed_setpoint_command());
		EXPECT_NE(nullptr, command->get_sender_control_function());
	}

	{
		// Test timeouts
		interfaceUnderTest.initialize();
		interfaceUnderTest.update();

		std::this_thread::sleep_for(std::chrono::milliseconds(305));
		interfaceUnderTest.update();
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_machine_selected_speed_sources());
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_wheel_based_speed_sources());
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_ground_based_speed_sources());
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_machine_selected_speed_command_sources());
	}
}
