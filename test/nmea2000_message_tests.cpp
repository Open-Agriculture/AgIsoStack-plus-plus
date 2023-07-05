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
	EXPECT_TRUE(messageDataUnderTest.set_magnetic_variation(-3));
	EXPECT_TRUE(messageDataUnderTest.set_sensor_reference(VesselHeading::HeadingSensorReference::True));
	EXPECT_TRUE(messageDataUnderTest.set_sequence_id(4));
	EXPECT_TRUE(messageDataUnderTest.set_timestamp(5));

	EXPECT_FALSE(messageDataUnderTest.set_heading(1));
	EXPECT_FALSE(messageDataUnderTest.set_magnetic_deviation(2));
	EXPECT_FALSE(messageDataUnderTest.set_magnetic_variation(-3));
	EXPECT_FALSE(messageDataUnderTest.set_sensor_reference(VesselHeading::HeadingSensorReference::True));
	EXPECT_FALSE(messageDataUnderTest.set_sequence_id(4));
	EXPECT_FALSE(messageDataUnderTest.set_timestamp(5));

	EXPECT_NEAR(0.0001f, messageDataUnderTest.get_heading(), 0.00005f);
	EXPECT_EQ(1, messageDataUnderTest.get_raw_heading());
	EXPECT_EQ(2, messageDataUnderTest.get_raw_magnetic_deviation());
	EXPECT_NEAR(0.0002f, messageDataUnderTest.get_magnetic_deviation(), 0.00005f);
	EXPECT_EQ(-3, messageDataUnderTest.get_raw_magnetic_variation());
	EXPECT_NEAR(-0.0003f, messageDataUnderTest.get_magnetic_variation(), 0.00005f);
	EXPECT_EQ(VesselHeading::HeadingSensorReference::True, messageDataUnderTest.get_sensor_reference());
	EXPECT_EQ(4, messageDataUnderTest.get_sequence_id());
	EXPECT_EQ(5, messageDataUnderTest.get_timestamp());
	EXPECT_EQ(nullptr, messageDataUnderTest.get_control_function());

	std::vector<std::uint8_t> serializationBuffer;
	EXPECT_NO_THROW(messageDataUnderTest.serialize(serializationBuffer));

	ASSERT_EQ(CAN_DATA_LENGTH, serializationBuffer.size());
	EXPECT_EQ(4, serializationBuffer.at(0)); // Sequence ID
	EXPECT_EQ(1, serializationBuffer.at(1)); // Reading
	EXPECT_EQ(0, serializationBuffer.at(2)); // Reading
	EXPECT_EQ(2, serializationBuffer.at(3)); // Deviation
	EXPECT_EQ(0, serializationBuffer.at(4)); // Deviation

	std::int16_t tempVariation = static_cast<int16_t>(serializationBuffer.at(5));
	tempVariation |= (static_cast<int16_t>(serializationBuffer.at(6)) << 8);
	EXPECT_EQ(-3, tempVariation); // Variation
	EXPECT_EQ(0, serializationBuffer.at(7) & 0x03); // True Reference Source
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

	EXPECT_EQ(100, messageDataUnderTest.get_raw_rate_of_turn());
	EXPECT_NEAR(100 * ((1.0 / 32.0) * 10E-6), messageDataUnderTest.get_rate_of_turn(), 0.0005);
	EXPECT_EQ(200, messageDataUnderTest.get_sequence_id());
	EXPECT_EQ(300, messageDataUnderTest.get_timestamp());
	EXPECT_EQ(nullptr, messageDataUnderTest.get_control_function());

	std::vector<std::uint8_t> serializationBuffer;
	EXPECT_NO_THROW(messageDataUnderTest.serialize(serializationBuffer));

	ASSERT_EQ(CAN_DATA_LENGTH, serializationBuffer.size());
	EXPECT_EQ(200, serializationBuffer.at(0));

	std::int32_t rateOfTurn = static_cast<int32_t>(serializationBuffer.at(1));
	rateOfTurn |= (static_cast<int32_t>(serializationBuffer.at(2)) << 8);
	rateOfTurn |= (static_cast<int32_t>(serializationBuffer.at(3)) << 16);
	rateOfTurn |= (static_cast<int32_t>(serializationBuffer.at(4)) << 24);
	EXPECT_EQ(rateOfTurn, 100);
	EXPECT_EQ(0xFF, serializationBuffer.at(5));
	EXPECT_EQ(0xFF, serializationBuffer.at(6));
	EXPECT_EQ(0xFF, serializationBuffer.at(7));
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
	EXPECT_EQ(nullptr, messageDataUnderTest.get_control_function());

	std::vector<std::uint8_t> serializationBuffer;
	EXPECT_NO_THROW(messageDataUnderTest.serialize(serializationBuffer));
}

TEST(NMEA2000_TESTS, CourseOverGroundSpeedOverGroundRapidUpdateDataInterface)
{
	CourseOverGroundSpeedOverGroundRapidUpdate messageDataUnderTest(nullptr);

	EXPECT_TRUE(messageDataUnderTest.set_course_over_ground(50));
	EXPECT_TRUE(messageDataUnderTest.set_course_over_ground_reference(CourseOverGroundSpeedOverGroundRapidUpdate::CourseOverGroudReference::Magnetic));
	EXPECT_TRUE(messageDataUnderTest.set_sequence_id(9));
	EXPECT_TRUE(messageDataUnderTest.set_speed_over_ground(75));
	EXPECT_TRUE(messageDataUnderTest.set_timestamp(87));

	EXPECT_FALSE(messageDataUnderTest.set_course_over_ground(50));
	EXPECT_FALSE(messageDataUnderTest.set_course_over_ground_reference(CourseOverGroundSpeedOverGroundRapidUpdate::CourseOverGroudReference::Magnetic));
	EXPECT_FALSE(messageDataUnderTest.set_sequence_id(9));
	EXPECT_FALSE(messageDataUnderTest.set_speed_over_ground(75));
	EXPECT_FALSE(messageDataUnderTest.set_timestamp(87));

	EXPECT_EQ(50, messageDataUnderTest.get_raw_course_over_ground());
	EXPECT_NEAR(50 * 0.0001, messageDataUnderTest.get_course_over_ground(), 0.00005);
	EXPECT_EQ(CourseOverGroundSpeedOverGroundRapidUpdate::CourseOverGroudReference::Magnetic, messageDataUnderTest.get_course_over_ground_reference());
	EXPECT_EQ(9, messageDataUnderTest.get_sequence_id());
	EXPECT_EQ(75, messageDataUnderTest.get_raw_speed_over_ground());
	EXPECT_EQ(87, messageDataUnderTest.get_timestamp());
	EXPECT_EQ(nullptr, messageDataUnderTest.get_control_function());
}

TEST(NMEA2000_Tests, PositionDeltaHighPrecisionRapidUpdateDataInterface)
{
	PositionDeltaHighPrecisionRapidUpdate messageDataUnderTest(nullptr);

	EXPECT_TRUE(messageDataUnderTest.set_latitude_delta(-5000));
	EXPECT_TRUE(messageDataUnderTest.set_longitude_delta(-9000));
	EXPECT_TRUE(messageDataUnderTest.set_sequence_id(49));
	EXPECT_TRUE(messageDataUnderTest.set_timestamp(1500));
	EXPECT_TRUE(messageDataUnderTest.set_time_delta(7));

	EXPECT_FALSE(messageDataUnderTest.set_latitude_delta(-5000));
	EXPECT_FALSE(messageDataUnderTest.set_longitude_delta(-9000));
	EXPECT_FALSE(messageDataUnderTest.set_sequence_id(49));
	EXPECT_FALSE(messageDataUnderTest.set_timestamp(1500));
	EXPECT_FALSE(messageDataUnderTest.set_time_delta(7));

	EXPECT_EQ(nullptr, messageDataUnderTest.get_control_function());
	EXPECT_EQ(-5000 * 10E-16, messageDataUnderTest.get_latitude_delta());
	EXPECT_EQ(-9000 * 10E-16, messageDataUnderTest.get_longitude_delta());
	EXPECT_EQ(-5000, messageDataUnderTest.get_raw_latitude_delta());
	EXPECT_EQ(-9000, messageDataUnderTest.get_raw_longitude_delta());
	EXPECT_EQ(7, messageDataUnderTest.get_raw_time_delta());
	EXPECT_EQ(7 * (5 * 10E-3), messageDataUnderTest.get_time_delta());

	std::vector<std::uint8_t> messageBuffer;
	EXPECT_NO_THROW(messageDataUnderTest.serialize(messageBuffer));
}

TEST(NMEA2000_Tests, GNSSPositionDataDataInterface)
{
	GNSSPositionData messageDataUnderTest(nullptr);

	EXPECT_TRUE(messageDataUnderTest.set_geoidal_separation(10000));
	EXPECT_TRUE(messageDataUnderTest.set_gnss_method(GNSSPositionData::GNSSMethod::RTKFixedInteger));
	EXPECT_TRUE(messageDataUnderTest.set_horizontal_dilution_of_precision(-10));
	EXPECT_TRUE(messageDataUnderTest.set_integrity(GNSSPositionData::Integrity::Safe));
	EXPECT_TRUE(messageDataUnderTest.set_number_of_reference_stations(1));
	EXPECT_TRUE(messageDataUnderTest.set_number_of_space_vehicles(4));
	EXPECT_TRUE(messageDataUnderTest.set_positional_dilution_of_precision(-894));
	EXPECT_TRUE(messageDataUnderTest.set_sequence_id(5));
	EXPECT_TRUE(messageDataUnderTest.set_timestamp(50));
	EXPECT_TRUE(messageDataUnderTest.set_type_of_system(GNSSPositionData::TypeOfSystem::GPSPlusSBASPlusGLONASS));

	EXPECT_FALSE(messageDataUnderTest.set_geoidal_separation(10000));
	EXPECT_FALSE(messageDataUnderTest.set_gnss_method(GNSSPositionData::GNSSMethod::RTKFixedInteger));
	EXPECT_FALSE(messageDataUnderTest.set_horizontal_dilution_of_precision(-10));
	EXPECT_FALSE(messageDataUnderTest.set_integrity(GNSSPositionData::Integrity::Safe));
	EXPECT_FALSE(messageDataUnderTest.set_number_of_reference_stations(1));
	EXPECT_FALSE(messageDataUnderTest.set_number_of_space_vehicles(4));
	EXPECT_FALSE(messageDataUnderTest.set_positional_dilution_of_precision(-894));
	EXPECT_FALSE(messageDataUnderTest.set_sequence_id(5));
	EXPECT_FALSE(messageDataUnderTest.set_timestamp(50));
	EXPECT_FALSE(messageDataUnderTest.set_type_of_system(GNSSPositionData::TypeOfSystem::GPSPlusSBASPlusGLONASS));

	EXPECT_EQ(nullptr, messageDataUnderTest.get_control_function());
	EXPECT_EQ(10000, messageDataUnderTest.get_geoidal_separation());
	EXPECT_EQ(GNSSPositionData::GNSSMethod::RTKFixedInteger, messageDataUnderTest.get_gnss_method());
	EXPECT_EQ(-10, messageDataUnderTest.get_horizontal_dilution_of_precision());
	EXPECT_EQ(GNSSPositionData::Integrity::Safe, messageDataUnderTest.get_integrity());
	EXPECT_EQ(1, messageDataUnderTest.get_number_of_reference_stations());
	EXPECT_EQ(4, messageDataUnderTest.get_number_of_space_vehicles());
	EXPECT_EQ(-894, messageDataUnderTest.get_positional_dilution_of_precision());
	EXPECT_EQ(5, messageDataUnderTest.get_sequence_id());
	EXPECT_EQ(50, messageDataUnderTest.get_timestamp());
	EXPECT_EQ(GNSSPositionData::TypeOfSystem::GPSPlusSBASPlusGLONASS, messageDataUnderTest.get_type_of_system());

	std::vector<std::uint8_t> messageBuffer;
	EXPECT_NO_THROW(messageDataUnderTest.serialize(messageBuffer));
}
