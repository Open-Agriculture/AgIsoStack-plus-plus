//================================================================================================
/// @file time_date_tests.cpp
///
/// @brief Unit tests for the TimeDateInterface class.
/// @author Adrian Del Grosso
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#include <gtest/gtest.h>

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_time_date_interface.hpp"

using namespace isobus;

static TimeDateInterface::TimeAndDateInformation testTimeDateInformation;
static bool isRxCallbackCalled = false;
void test_time_date_rx_callback(TimeDateInterface::TimeAndDateInformation timeDate)
{
	testTimeDateInformation = timeDate;
	isRxCallbackCalled = true;
}

TEST(TIME_DATE_TESTS, ReceivingMessages)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	TimeDateInterface timeDateInterfaceUnderTest;

	EXPECT_FALSE(timeDateInterfaceUnderTest.is_initialized());
	timeDateInterfaceUnderTest.initialize();
	EXPECT_TRUE(timeDateInterfaceUnderTest.is_initialized());

	// Test receiving a time and date message
	auto partner = test_helpers::force_claim_partnered_control_function(0x47, 0);

	CANMessageFrame testFrame;
	memset(&testFrame, 0, sizeof(testFrame));
	testFrame.isExtendedFrame = true;

	// Register with the event dispatcher
	timeDateInterfaceUnderTest.get_event_dispatcher().add_listener(test_time_date_rx_callback);
	CANNetworkManager::CANNetwork.update(); // Extra update to help when running under ctest

	// Construct a message that says the following:
	// 1. The year is 2023
	// 2. The month is August
	// 3. 7 Days into the month
	// 4. 22 hours into the day
	// 5. 49 minutes into the hour
	// 6. 41.000 seconds into the minute
	// 7. Local hour offset is -5 (Eastern Standard Time, which implies the stuff above is UTC time/date)
	// 8. Local minute offset is 0
	testFrame.identifier = 0x18FEE647;
	testFrame.dataLength = 8;
	testFrame.data[0] = 0xA4;
	testFrame.data[1] = 0x31;
	testFrame.data[2] = 0x16;
	testFrame.data[3] = 0x08;
	testFrame.data[4] = 0x1C;
	testFrame.data[5] = 0x26;
	testFrame.data[6] = 0x7D;
	testFrame.data[7] = 0x78;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	EXPECT_TRUE(isRxCallbackCalled);
	EXPECT_EQ(testTimeDateInformation.timeAndDate.year, 2023);
	EXPECT_EQ(testTimeDateInformation.timeAndDate.month, 8);
	EXPECT_EQ(testTimeDateInformation.timeAndDate.day, 7);
	EXPECT_EQ(testTimeDateInformation.timeAndDate.quarterDays, 0);
	EXPECT_EQ(testTimeDateInformation.timeAndDate.hours, 22);
	EXPECT_EQ(testTimeDateInformation.timeAndDate.minutes, 49);
	EXPECT_EQ(testTimeDateInformation.timeAndDate.seconds, 41);
	EXPECT_EQ(testTimeDateInformation.timeAndDate.milliseconds, 0);
	EXPECT_EQ(testTimeDateInformation.timeAndDate.localHourOffset, -5);
	EXPECT_EQ(testTimeDateInformation.timeAndDate.localMinuteOffset, 0);

	isRxCallbackCalled = false;
	// Send a message with wrong length, make sure it's rejected
	testFrame.dataLength = 7;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_FALSE(isRxCallbackCalled);

	CANHardwareInterface::stop();
}

TEST(TIME_DATE_TESTS, TransmitMessages)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	auto testInternalControlFunction = test_helpers::claim_internal_control_function(0x44, 0);
	test_helpers::force_claim_partnered_control_function(0x25, 0);

	// To test transmitting, we need to provide a callback that will populate the time and date information
	// that will be sent out on the bus.
	// This is so that the PGN request protocol can ask for the time and date information at any time.
	TimeDateInterface timeDateInterfaceUnderTest(testInternalControlFunction, [](TimeDateInterface::TimeAndDate &timeAndDateToPopulate) -> bool {
		timeAndDateToPopulate.year = 2023;
		timeAndDateToPopulate.month = 8;
		timeAndDateToPopulate.day = 7;
		timeAndDateToPopulate.quarterDays = 0;
		timeAndDateToPopulate.hours = 22;
		timeAndDateToPopulate.minutes = 49;
		timeAndDateToPopulate.seconds = 41;
		timeAndDateToPopulate.milliseconds = 0;
		timeAndDateToPopulate.localHourOffset = -5;
		timeAndDateToPopulate.localMinuteOffset = 0;
		return true;
	});

	EXPECT_FALSE(timeDateInterfaceUnderTest.is_initialized());
	timeDateInterfaceUnderTest.initialize();
	EXPECT_TRUE(timeDateInterfaceUnderTest.is_initialized());

	EXPECT_EQ(timeDateInterfaceUnderTest.get_control_function(), testInternalControlFunction);

	// Get the virtual CAN plugin back to a known state
	CANMessageFrame testFrame;
	while (!testPlugin.get_queue_empty())
	{
		testPlugin.read_frame(testFrame);
	}
	ASSERT_TRUE(testPlugin.get_queue_empty());

	// Now, we can see if it works by receiving a PGN request for the time and date information PGN
	testFrame.isExtendedFrame = true;
	testFrame.channel = 0;
	testFrame.dataLength = 3;
	testFrame.identifier = 0x18EAFF25;
	testFrame.data[0] = 0xE6;
	testFrame.data[1] = 0xFE;
	testFrame.data[2] = 0x00;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	// This data should match the data we provided in the callback, and the one we processed in the other unit test
	EXPECT_TRUE(testPlugin.read_frame(testFrame));
	EXPECT_EQ(0x18FEE644, testFrame.identifier);
	EXPECT_EQ(0x08, testFrame.dataLength);
	EXPECT_EQ(0xA4, testFrame.data[0]);
	EXPECT_EQ(0x31, testFrame.data[1]);
	EXPECT_EQ(0x16, testFrame.data[2]);
	EXPECT_EQ(0x08, testFrame.data[3]);
	EXPECT_EQ(0x1C, testFrame.data[4]);
	EXPECT_EQ(0x26, testFrame.data[5]);
	EXPECT_EQ(0x7D, testFrame.data[6]);
	EXPECT_EQ(0x78, testFrame.data[7]);

	// Test emitting a request for the time and date information
	timeDateInterfaceUnderTest.request_time_and_date(testInternalControlFunction, nullptr);
	CANNetworkManager::CANNetwork.update();
	EXPECT_TRUE(testPlugin.read_frame(testFrame));
	EXPECT_EQ(0x18EAFF44, testFrame.identifier);
	EXPECT_EQ(0x03, testFrame.dataLength);
	EXPECT_EQ(0xE6, testFrame.data[0]);
	EXPECT_EQ(0xFE, testFrame.data[1]);
	EXPECT_EQ(0x00, testFrame.data[2]);

	CANNetworkManager::CANNetwork.deactivate_control_function(testInternalControlFunction);
	CANHardwareInterface::stop();
}

TEST(TIME_DATE_TESTS, MiscTests)
{
	// Test rejection of invalid parameters
	TimeDateInterface timeDateInterfaceUnderTest;

	TimeDateInterface::TimeAndDate dataToSend;

	// Valid state
	dataToSend.year = 2023;
	dataToSend.month = 8;
	dataToSend.day = 7;
	dataToSend.quarterDays = 0;
	dataToSend.hours = 22;
	dataToSend.minutes = 49;
	dataToSend.seconds = 41;
	dataToSend.milliseconds = 0;
	dataToSend.localHourOffset = -5;
	dataToSend.localMinuteOffset = 0;

	// Test invalid year
	dataToSend.year = 1984;
	EXPECT_DEATH(timeDateInterfaceUnderTest.send_time_and_date(dataToSend), "");

	// Test year high limit
	dataToSend.year = 2236;
	EXPECT_DEATH(timeDateInterfaceUnderTest.send_time_and_date(dataToSend), "");

	// Test invalid month
	dataToSend.year = 2023;
	dataToSend.month = 0;
	EXPECT_DEATH(timeDateInterfaceUnderTest.send_time_and_date(dataToSend), "");

	// Test month high limit
	dataToSend.month = 13;
	EXPECT_DEATH(timeDateInterfaceUnderTest.send_time_and_date(dataToSend), "");

	// Test invalid day
	dataToSend.month = 8;
	dataToSend.day = 90;
	EXPECT_DEATH(timeDateInterfaceUnderTest.send_time_and_date(dataToSend), "");

	// Test invalid quarter days
	dataToSend.day = 7;
	dataToSend.quarterDays = 4;
	EXPECT_DEATH(timeDateInterfaceUnderTest.send_time_and_date(dataToSend), "");

	// Test invalid hours
	dataToSend.quarterDays = 0;
	dataToSend.hours = 24;
	EXPECT_DEATH(timeDateInterfaceUnderTest.send_time_and_date(dataToSend), "");

	// Test invalid minutes
	dataToSend.hours = 22;
	dataToSend.minutes = 60;
	EXPECT_DEATH(timeDateInterfaceUnderTest.send_time_and_date(dataToSend), "");

	// Test invalid seconds
	dataToSend.minutes = 49;
	dataToSend.seconds = 60;
	EXPECT_DEATH(timeDateInterfaceUnderTest.send_time_and_date(dataToSend), "");

	// Test invalid milliseconds
	dataToSend.seconds = 41;
	dataToSend.milliseconds = 134;
	EXPECT_DEATH(timeDateInterfaceUnderTest.send_time_and_date(dataToSend), "");

	// Test invalid local hour offset
	dataToSend.milliseconds = 0;
	dataToSend.localHourOffset = -24;
	EXPECT_DEATH(timeDateInterfaceUnderTest.send_time_and_date(dataToSend), "");

	dataToSend.localHourOffset = 24;
	EXPECT_DEATH(timeDateInterfaceUnderTest.send_time_and_date(dataToSend), "");

	// Test invalid local minute offset
	dataToSend.localHourOffset = -5;
	dataToSend.localMinuteOffset = 60;
	EXPECT_DEATH(timeDateInterfaceUnderTest.send_time_and_date(dataToSend), "");

	dataToSend.localMinuteOffset = -60;
	EXPECT_DEATH(timeDateInterfaceUnderTest.send_time_and_date(dataToSend), "");
}
