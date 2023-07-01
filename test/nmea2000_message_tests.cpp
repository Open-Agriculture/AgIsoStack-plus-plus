#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/nmea2000_message_definitions.hpp"
#include "isobus/utility/system_timing.hpp"

using namespace isobus;
using namespace NMEA2000Messages;

TEST(NMEA2000_TESTS, VesselHeadingDataInterface)
{
	VesselHeading messageDataUnderTest(nullptr);

	EXPECT_TRUE(messageDataUnderTest.set_heading(1));
	EXPECT_TRUE(messageDataUnderTest.set_magnetic_deviation(2));
	EXPECT_TRUE(messageDataUnderTest.set_magnetic_variation(3));
	EXPECT_TRUE(messageDataUnderTest.set_sensor_reference(VesselHeading::HeadingSensorReference::True));
	EXPECT_TRUE(messageDataUnderTest.set_sequence_id(4));
	EXPECT_TRUE(messageDataUnderTest.set_timestamp(5));

	EXPECT_FALSE(messageDataUnderTest.set_heading(1));
	EXPECT_FALSE(messageDataUnderTest.set_magnetic_deviation(2));
	EXPECT_FALSE(messageDataUnderTest.set_magnetic_variation(3));
	EXPECT_FALSE(messageDataUnderTest.set_sensor_reference(VesselHeading::HeadingSensorReference::True));
	EXPECT_FALSE(messageDataUnderTest.set_sequence_id(4));
	EXPECT_FALSE(messageDataUnderTest.set_timestamp(5));

	EXPECT_EQ(1, messageDataUnderTest.get_heading());
	EXPECT_EQ(2, messageDataUnderTest.get_magnetic_deviation());
	EXPECT_EQ(3, messageDataUnderTest.get_magnetic_variation());
	EXPECT_EQ(VesselHeading::HeadingSensorReference::True, messageDataUnderTest.get_sensor_reference());
	EXPECT_EQ(4, messageDataUnderTest.get_sequence_id());
	EXPECT_EQ(5, messageDataUnderTest.get_timestamp());
}

TEST(NMEA2000_TESTS, RateOfTurnDataInterface)
{
	RateOfTurn messageDataUnderTest(nullptr);

	EXPECT_TRUE(messageDataUnderTest.set_rate_of_turn(100));
	EXPECT_TRUE(messageDataUnderTest.set_sequence_id(200));
	EXPECT_TRUE(messageDataUnderTest.set_timestamp(300));

	EXPECT_FALSE(messageDataUnderTest.set_rate_of_turn(100));
	EXPECT_FALSE(messageDataUnderTest.set_sequence_id(200));
	EXPECT_FALSE(messageDataUnderTest.set_timestamp(300));

	EXPECT_EQ(100, messageDataUnderTest.get_rate_of_turn());
	EXPECT_EQ(200, messageDataUnderTest.get_sequence_id());
	EXPECT_EQ(300, messageDataUnderTest.get_timestamp());
}

TEST(NMEA2000_TESTS, PositionRapidUpdateDataInterface)
{
	PositionRapidUpdate messageDataUnderTest(nullptr);

	EXPECT_TRUE(messageDataUnderTest.set_latitude(1000));
	EXPECT_TRUE(messageDataUnderTest.set_longitude(2000));
	EXPECT_TRUE(messageDataUnderTest.set_timestamp(3000));

	EXPECT_FALSE(messageDataUnderTest.set_latitude(1000));
	EXPECT_FALSE(messageDataUnderTest.set_longitude(2000));
	EXPECT_FALSE(messageDataUnderTest.set_timestamp(3000));

	EXPECT_EQ(1000, messageDataUnderTest.get_raw_latitude());
	EXPECT_EQ(2000, messageDataUnderTest.get_raw_longitude());
	EXPECT_EQ(3000, messageDataUnderTest.get_timestamp());
}
