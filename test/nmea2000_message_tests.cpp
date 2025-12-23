#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/nmea2000_message_definitions.hpp"
#include "isobus/isobus/nmea2000_message_interface.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"

using namespace isobus;
using namespace NMEA2000Messages;

static bool wasCourseOverGroundSpeedOverGroundRapidUpdateCallbackHit = false;

static void test_cog_sog_callback(const std::shared_ptr<CourseOverGroundSpeedOverGroundRapidUpdate>, bool)
{
	wasCourseOverGroundSpeedOverGroundRapidUpdateCallbackHit = true;
}

static bool wasDatumCallbackHit = false;

static void test_datum_callback(const std::shared_ptr<Datum>, bool)
{
	wasDatumCallbackHit = true;
}

static bool wasGNSSPositionDataCallbackHit = false;

static void test_gnss_position_data_callback(const std::shared_ptr<GNSSPositionData>, bool)
{
	wasGNSSPositionDataCallbackHit = true;
}

static bool wasPositionRapidUpdateCallbackHit = false;

static void test_position_rapid_update_callback(const std::shared_ptr<PositionRapidUpdate>, bool)
{
	wasPositionRapidUpdateCallbackHit = true;
}

static bool wasPositionDeltaHighSpeedRapidUpdateCallbackHit = false;

static void test_position_delta_high_speed_rapid_update_callback(const std::shared_ptr<PositionDeltaHighPrecisionRapidUpdate>, bool)
{
	wasPositionDeltaHighSpeedRapidUpdateCallbackHit = true;
}

static bool wasRateOfTurnCallbackHit = false;

static void test_rate_of_turn_callback(const std::shared_ptr<RateOfTurn>, bool)
{
	wasRateOfTurnCallbackHit = true;
}

static bool wasVesselHeadingCallbackHit = false;

static void test_vessel_heading_callback(const std::shared_ptr<VesselHeading>, bool)
{
	wasVesselHeadingCallbackHit = true;
}

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

	std::int16_t tempVariation = static_cast<std::int16_t>(serializationBuffer.at(5));
	tempVariation |= (static_cast<std::int16_t>(serializationBuffer.at(6)) << 8);
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

	std::int32_t rateOfTurn = static_cast<std::int32_t>(serializationBuffer.at(1));
	rateOfTurn |= (static_cast<std::int32_t>(serializationBuffer.at(2)) << 8);
	rateOfTurn |= (static_cast<std::int32_t>(serializationBuffer.at(3)) << 16);
	rateOfTurn |= (static_cast<std::int32_t>(serializationBuffer.at(4)) << 24);
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
	EXPECT_NEAR(1000 * 1E-7, messageDataUnderTest.get_latitude(), 0.000001);
	EXPECT_NEAR(2000 * 1E-7, messageDataUnderTest.get_longitude(), 0.000001);
	EXPECT_EQ(3000, messageDataUnderTest.get_timestamp());
	EXPECT_EQ(nullptr, messageDataUnderTest.get_control_function());

	std::vector<std::uint8_t> serializationBuffer;
	EXPECT_NO_THROW(messageDataUnderTest.serialize(serializationBuffer));

	ASSERT_EQ(CAN_DATA_LENGTH, serializationBuffer.size());

	std::int32_t latitude = static_cast<std::int32_t>(serializationBuffer.at(0)) |
	  (static_cast<std::int32_t>(serializationBuffer.at(1)) << 8) |
	  (static_cast<std::int32_t>(serializationBuffer.at(2)) << 16) |
	  (static_cast<std::int32_t>(serializationBuffer.at(3)) << 24);

	std::int32_t longitude = static_cast<std::int32_t>(serializationBuffer.at(4)) |
	  (static_cast<std::int32_t>(serializationBuffer.at(5)) << 8) |
	  (static_cast<std::int32_t>(serializationBuffer.at(6)) << 16) |
	  (static_cast<std::int32_t>(serializationBuffer.at(7)) << 24);

	EXPECT_EQ(latitude, 1000);
	EXPECT_EQ(longitude, 2000);
}

TEST(NMEA2000_TESTS, CourseOverGroundSpeedOverGroundRapidUpdateDataInterface)
{
	CourseOverGroundSpeedOverGroundRapidUpdate messageDataUnderTest(nullptr);

	EXPECT_TRUE(messageDataUnderTest.set_course_over_ground(50));
	EXPECT_TRUE(messageDataUnderTest.set_course_over_ground_reference(CourseOverGroundSpeedOverGroundRapidUpdate::CourseOverGroundReference::Magnetic));
	EXPECT_TRUE(messageDataUnderTest.set_sequence_id(9));
	EXPECT_TRUE(messageDataUnderTest.set_speed_over_ground(75));
	EXPECT_TRUE(messageDataUnderTest.set_timestamp(87));

	EXPECT_FALSE(messageDataUnderTest.set_course_over_ground(50));
	EXPECT_FALSE(messageDataUnderTest.set_course_over_ground_reference(CourseOverGroundSpeedOverGroundRapidUpdate::CourseOverGroundReference::Magnetic));
	EXPECT_FALSE(messageDataUnderTest.set_sequence_id(9));
	EXPECT_FALSE(messageDataUnderTest.set_speed_over_ground(75));
	EXPECT_FALSE(messageDataUnderTest.set_timestamp(87));

	EXPECT_EQ(50, messageDataUnderTest.get_raw_course_over_ground());
	EXPECT_NEAR(50 * 1E-4f, messageDataUnderTest.get_course_over_ground(), 0.00005);
	EXPECT_EQ(CourseOverGroundSpeedOverGroundRapidUpdate::CourseOverGroundReference::Magnetic, messageDataUnderTest.get_course_over_ground_reference());
	EXPECT_EQ(9, messageDataUnderTest.get_sequence_id());
	EXPECT_EQ(75, messageDataUnderTest.get_raw_speed_over_ground());
	EXPECT_EQ(87, messageDataUnderTest.get_timestamp());
	EXPECT_EQ(nullptr, messageDataUnderTest.get_control_function());

	std::vector<std::uint8_t> serializationBuffer;
	EXPECT_NO_THROW(messageDataUnderTest.serialize(serializationBuffer));

	ASSERT_EQ(CAN_DATA_LENGTH, serializationBuffer.size());

	EXPECT_EQ(9, serializationBuffer.at(0)); // Seq
	EXPECT_EQ(1, serializationBuffer.at(1) & 0x03); // Ref

	std::uint16_t course = static_cast<std::uint16_t>(serializationBuffer.at(2)) |
	  (static_cast<std::uint16_t>(serializationBuffer.at(3)) << 8);
	EXPECT_EQ(course, 50);

	std::uint16_t speed = static_cast<std::uint16_t>(serializationBuffer.at(4)) |
	  (static_cast<std::uint16_t>(serializationBuffer.at(5)) << 8);
	EXPECT_EQ(speed, 75);

	EXPECT_EQ(0xFF, serializationBuffer.at(6));
	EXPECT_EQ(0xFF, serializationBuffer.at(7));
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
	EXPECT_EQ(-5000 * 1E-6, messageDataUnderTest.get_latitude_delta());
	EXPECT_EQ(-9000 * 1E-6, messageDataUnderTest.get_longitude_delta());
	EXPECT_EQ(-5000, messageDataUnderTest.get_raw_latitude_delta());
	EXPECT_EQ(-9000, messageDataUnderTest.get_raw_longitude_delta());
	EXPECT_EQ(7, messageDataUnderTest.get_raw_time_delta());
	EXPECT_NEAR(0.007 * 5, messageDataUnderTest.get_time_delta(), 0.0001);
	EXPECT_EQ(49, messageDataUnderTest.get_sequence_id());

	std::vector<std::uint8_t> messageBuffer;
	EXPECT_NO_THROW(messageDataUnderTest.serialize(messageBuffer));

	ASSERT_EQ(CAN_DATA_LENGTH, messageBuffer.size());
	EXPECT_EQ(49, messageBuffer.at(0));
	EXPECT_EQ(7, messageBuffer.at(1));

	std::int32_t deltaLatitude = static_cast<std::int32_t>(messageBuffer.at(2)) |
	  (static_cast<std::int32_t>(messageBuffer.at(3)) << 8) |
	  (static_cast<std::int32_t>(messageBuffer.at(4)) << 16);

	// Need to sign extend the value...
	if (messageBuffer.at(4) & 0x80)
	{
		deltaLatitude |= static_cast<std::int32_t>(0xFF000000);
	}
	EXPECT_EQ(-5000, deltaLatitude);

	std::int32_t deltaLongitude = static_cast<std::int32_t>(messageBuffer.at(5)) |
	  (static_cast<std::int32_t>(messageBuffer.at(6)) << 8) |
	  (static_cast<std::int32_t>(messageBuffer.at(7)) << 16);

	// Need to sign extend the value...
	if (messageBuffer.at(7) & 0x80)
	{
		deltaLongitude |= static_cast<std::int32_t>(0xFF000000);
	}
	EXPECT_EQ(-9000, deltaLongitude);
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
	EXPECT_TRUE(messageDataUnderTest.set_altitude(5820000000));
	EXPECT_TRUE(messageDataUnderTest.set_latitude(-72057594037298808));
	EXPECT_TRUE(messageDataUnderTest.set_longitude(720575));
	EXPECT_TRUE(messageDataUnderTest.set_position_date(19551));
	EXPECT_TRUE(messageDataUnderTest.set_position_time(86400));
	EXPECT_TRUE(messageDataUnderTest.set_reference_station(0, 4, GNSSPositionData::TypeOfSystem::Galileo, 100));

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
	EXPECT_FALSE(messageDataUnderTest.set_altitude(5820000000));
	EXPECT_FALSE(messageDataUnderTest.set_latitude(-72057594037298808));
	EXPECT_FALSE(messageDataUnderTest.set_longitude(720575));
	EXPECT_FALSE(messageDataUnderTest.set_position_date(19551));
	EXPECT_FALSE(messageDataUnderTest.set_position_time(86400));
	EXPECT_FALSE(messageDataUnderTest.set_reference_station(0, 4, GNSSPositionData::TypeOfSystem::Galileo, 100));

	EXPECT_EQ(nullptr, messageDataUnderTest.get_control_function());
	EXPECT_EQ(10000, messageDataUnderTest.get_raw_geoidal_separation());
	EXPECT_EQ(GNSSPositionData::GNSSMethod::RTKFixedInteger, messageDataUnderTest.get_gnss_method());
	EXPECT_EQ(-10, messageDataUnderTest.get_raw_horizontal_dilution_of_precision());
	EXPECT_EQ(GNSSPositionData::Integrity::Safe, messageDataUnderTest.get_integrity());
	EXPECT_EQ(1, messageDataUnderTest.get_number_of_reference_stations());
	EXPECT_EQ(4, messageDataUnderTest.get_number_of_space_vehicles());
	EXPECT_EQ(-894, messageDataUnderTest.get_raw_positional_dilution_of_precision());
	EXPECT_EQ(5, messageDataUnderTest.get_sequence_id());
	EXPECT_EQ(50, messageDataUnderTest.get_timestamp());
	EXPECT_EQ(GNSSPositionData::TypeOfSystem::GPSPlusSBASPlusGLONASS, messageDataUnderTest.get_type_of_system());
	EXPECT_EQ(5820000000, messageDataUnderTest.get_raw_altitude());
	EXPECT_EQ(-72057594037298808, messageDataUnderTest.get_raw_latitude());
	EXPECT_EQ(720575, messageDataUnderTest.get_raw_longitude());
	EXPECT_NEAR(5820000000.0 * 1E-6, messageDataUnderTest.get_altitude(), 10E-4);
	EXPECT_NEAR(-72057594037298808.0 * 1E-16, messageDataUnderTest.get_latitude(), 10E-4);
	EXPECT_NEAR(720575.0 * 1E-16, messageDataUnderTest.get_longitude(), 10E-4);
	EXPECT_EQ(19551, messageDataUnderTest.get_position_date());
	EXPECT_EQ(86400, messageDataUnderTest.get_raw_position_time());
	EXPECT_EQ(4, messageDataUnderTest.get_reference_station_id(0));
	EXPECT_EQ(GNSSPositionData::TypeOfSystem::Galileo, messageDataUnderTest.get_reference_station_system_type(0));
	EXPECT_EQ(100, messageDataUnderTest.get_raw_reference_station_corrections_age(0));
	EXPECT_NEAR(100, messageDataUnderTest.get_geoidal_separation(), 0.001);

	std::vector<std::uint8_t> messageBuffer;
	EXPECT_NO_THROW(messageDataUnderTest.serialize(messageBuffer));

	ASSERT_EQ(47, messageBuffer.size());

	EXPECT_EQ(5, messageBuffer.at(0)); // Sequence

	std::uint16_t date = static_cast<std::uint16_t>(messageBuffer.at(1)) | (static_cast<std::uint16_t>(messageBuffer.at(2)) << 8);
	EXPECT_EQ(19551, date);

	std::uint32_t time = static_cast<std::uint32_t>(messageBuffer.at(3)) |
	  (static_cast<std::uint32_t>(messageBuffer.at(4)) << 8) |
	  (static_cast<std::uint32_t>(messageBuffer.at(5)) << 16) |
	  (static_cast<std::uint32_t>(messageBuffer.at(6)) << 24);
	EXPECT_EQ(86400, time);

	std::int64_t latitude = static_cast<std::int64_t>(messageBuffer.at(7)) |
	  (static_cast<std::int64_t>(messageBuffer.at(8)) << 8) |
	  (static_cast<std::int64_t>(messageBuffer.at(9)) << 16) |
	  (static_cast<std::int64_t>(messageBuffer.at(10)) << 24) |
	  (static_cast<std::int64_t>(messageBuffer.at(11)) << 32) |
	  (static_cast<std::int64_t>(messageBuffer.at(12)) << 40) |
	  (static_cast<std::int64_t>(messageBuffer.at(13)) << 48) |
	  (static_cast<std::int64_t>(messageBuffer.at(14)) << 56);
	EXPECT_EQ(latitude, -72057594037298808);

	std::int64_t longitude = static_cast<std::int64_t>(messageBuffer.at(15)) |
	  (static_cast<std::int64_t>(messageBuffer.at(16)) << 8) |
	  (static_cast<std::int64_t>(messageBuffer.at(17)) << 16) |
	  (static_cast<std::int64_t>(messageBuffer.at(18)) << 24) |
	  (static_cast<std::int64_t>(messageBuffer.at(19)) << 32) |
	  (static_cast<std::int64_t>(messageBuffer.at(20)) << 40) |
	  (static_cast<std::int64_t>(messageBuffer.at(21)) << 48) |
	  (static_cast<std::int64_t>(messageBuffer.at(22)) << 56);
	EXPECT_EQ(longitude, 720575);

	std::int64_t altitude = static_cast<std::int64_t>(messageBuffer.at(23)) |
	  (static_cast<std::int64_t>(messageBuffer.at(24)) << 8) |
	  (static_cast<std::int64_t>(messageBuffer.at(25)) << 16) |
	  (static_cast<std::int64_t>(messageBuffer.at(26)) << 24) |
	  (static_cast<std::int64_t>(messageBuffer.at(27)) << 32) |
	  (static_cast<std::int64_t>(messageBuffer.at(28)) << 40) |
	  (static_cast<std::int64_t>(messageBuffer.at(29)) << 48) |
	  (static_cast<std::int64_t>(messageBuffer.at(30)) << 56);
	EXPECT_EQ(altitude, 5820000000);

	EXPECT_EQ(messageBuffer.at(31) & 0x0F, 4); // System type
	EXPECT_EQ((messageBuffer.at(31) >> 4) & 0x0F, 4); // Method
	EXPECT_EQ(messageBuffer.at(32) & 0x03, 1); // Integrity
	EXPECT_EQ(messageBuffer.at(32) & 0xFC, 0xFC); // Integrity byte's reserved bits
	EXPECT_EQ(messageBuffer.at(33), 4); // Number of SVs

	std::int16_t hdop = static_cast<std::int16_t>(messageBuffer.at(34)) | (static_cast<std::int16_t>(messageBuffer.at(35)) << 8);
	EXPECT_EQ(-10, hdop);

	std::int16_t pdop = static_cast<std::int16_t>(messageBuffer.at(36)) | (static_cast<std::int16_t>(messageBuffer.at(37)) << 8);
	EXPECT_EQ(-894, pdop);

	std::int32_t geoidalSep = static_cast<std::int32_t>(messageBuffer.at(38)) |
	  (static_cast<std::int32_t>(messageBuffer.at(39)) << 8) |
	  (static_cast<std::int32_t>(messageBuffer.at(40)) << 16) |
	  (static_cast<std::int32_t>(messageBuffer.at(41)) << 24);
	EXPECT_EQ(10000, geoidalSep);

	EXPECT_EQ(1, messageBuffer.at(42));
	EXPECT_EQ(8, messageBuffer.at(43) & 0x0F);

	std::uint16_t stationID = static_cast<std::uint16_t>(messageBuffer.at(43) >> 4) | (static_cast<std::uint16_t>(messageBuffer.at(44)) << 4);
	EXPECT_EQ(stationID, 4);
	EXPECT_EQ(100, messageBuffer.at(45));
	EXPECT_EQ(0, messageBuffer.at(46));
}

TEST(NMEA2000_Tests, NMEA2KInterface)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	isobus::NAME TestDeviceNAME(0);
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(3);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::GaugeSmall));
	TestDeviceNAME.set_identity_number(245);
	TestDeviceNAME.set_ecu_instance(4);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(1407);

	auto testECU = test_helpers::claim_internal_control_function(0x51, 0);
	test_helpers::force_claim_partnered_control_function(0x52, 0);

	// Get the virtual CAN plugin back to a known state
	CANMessageFrame testFrame = {};
	while (!testPlugin.get_queue_empty())
	{
		testPlugin.read_frame(testFrame);
	}
	ASSERT_TRUE(testPlugin.get_queue_empty());

	{
		// Test COG/SOG
		NMEA2000MessageInterface interfaceUnderTest(testECU, true, false, false, false, false, false, false);

		EXPECT_FALSE(interfaceUnderTest.get_initialized());
		interfaceUnderTest.initialize();
		EXPECT_TRUE(interfaceUnderTest.get_initialized());

		EXPECT_TRUE(interfaceUnderTest.get_enable_sending_cog_sog_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_datum_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_gnss_position_data_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_position_delta_high_precision_rapid_update_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_rate_of_turn_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_vessel_heading_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_position_rapid_update_cyclically());

		interfaceUnderTest.set_enable_sending_cog_sog_cyclically(false);
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_cog_sog_cyclically());
		interfaceUnderTest.set_enable_sending_cog_sog_cyclically(true);
		EXPECT_TRUE(interfaceUnderTest.get_enable_sending_cog_sog_cyclically());

		auto &message = interfaceUnderTest.get_cog_sog_transmit_message();

		EXPECT_EQ(testECU, message.get_control_function());
		EXPECT_EQ(0, message.get_course_over_ground());
		EXPECT_EQ(CourseOverGroundSpeedOverGroundRapidUpdate::CourseOverGroundReference::NotApplicableOrNull, message.get_course_over_ground_reference());
		EXPECT_EQ(0, message.get_raw_course_over_ground());
		EXPECT_EQ(0, message.get_raw_speed_over_ground());
		EXPECT_EQ(0, message.get_sequence_id());
		EXPECT_EQ(0, message.get_speed_over_ground());
		EXPECT_EQ(250, message.get_timeout());
		EXPECT_EQ(0, message.get_timestamp());

		message.set_course_over_ground(10000);
		message.set_course_over_ground_reference(CourseOverGroundSpeedOverGroundRapidUpdate::CourseOverGroundReference::True);
		message.set_sequence_id(155);
		message.set_speed_over_ground(544);

		EXPECT_NEAR(10000 * 1E-4f, message.get_course_over_ground(), 0.001);
		EXPECT_EQ(CourseOverGroundSpeedOverGroundRapidUpdate::CourseOverGroundReference::True, message.get_course_over_ground_reference());
		EXPECT_EQ(10000, message.get_raw_course_over_ground());
		EXPECT_EQ(544, message.get_raw_speed_over_ground());
		EXPECT_EQ(155, message.get_sequence_id());
		EXPECT_NEAR(544 * 1E-2f, message.get_speed_over_ground(), 0.001);

		interfaceUnderTest.update();
		ASSERT_TRUE(testPlugin.read_frame(testFrame));

		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(155, testFrame.data[0]);
		EXPECT_EQ(0, testFrame.data[1] & 0x03);

		std::uint16_t course = static_cast<std::uint16_t>(testFrame.data[2]) | (static_cast<std::uint16_t>(testFrame.data[3]) << 8);

		EXPECT_EQ(10000, course);

		std::uint16_t speed = static_cast<std::uint16_t>(testFrame.data[4]) | (static_cast<std::uint16_t>(testFrame.data[5]) << 8);

		EXPECT_EQ(544, speed);
		EXPECT_EQ(0xFF, testFrame.data[6]);
		EXPECT_EQ(0xFF, testFrame.data[7]);

		EXPECT_EQ(0, interfaceUnderTest.get_number_received_course_speed_over_ground_message_sources());
		EXPECT_EQ(nullptr, interfaceUnderTest.get_received_course_speed_over_ground_message(0));

		interfaceUnderTest.get_course_speed_over_ground_rapid_update_event_publisher().add_listener(test_cog_sog_callback);

		// Pass the frame back in but as an RX message
		testFrame.identifier = 0x19F80252;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();

		EXPECT_EQ(1, interfaceUnderTest.get_number_received_course_speed_over_ground_message_sources());
		EXPECT_NE(nullptr, interfaceUnderTest.get_received_course_speed_over_ground_message(0));

		EXPECT_TRUE(wasCourseOverGroundSpeedOverGroundRapidUpdateCallbackHit);

		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();

		// Make sure duplicate messages don't make more instances of the message's class
		EXPECT_EQ(1, interfaceUnderTest.get_number_received_course_speed_over_ground_message_sources());
		EXPECT_NE(nullptr, interfaceUnderTest.get_received_course_speed_over_ground_message(0));
	}

	{
		// Test Datum
		NMEA2000MessageInterface interfaceUnderTest(testECU, false, true, false, false, false, false, false);

		EXPECT_FALSE(interfaceUnderTest.get_initialized());
		interfaceUnderTest.initialize();
		EXPECT_TRUE(interfaceUnderTest.get_initialized());

		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_cog_sog_cyclically());
		EXPECT_TRUE(interfaceUnderTest.get_enable_sending_datum_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_gnss_position_data_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_position_delta_high_precision_rapid_update_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_rate_of_turn_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_vessel_heading_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_position_rapid_update_cyclically());

		interfaceUnderTest.set_enable_sending_datum_cyclically(false);
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_datum_cyclically());
		interfaceUnderTest.set_enable_sending_datum_cyclically(true);
		EXPECT_TRUE(interfaceUnderTest.get_enable_sending_datum_cyclically());

		auto &message = interfaceUnderTest.get_datum_transmit_message();

		EXPECT_TRUE(message.set_delta_altitude(25000));
		EXPECT_TRUE(message.set_delta_latitude(12345));
		EXPECT_TRUE(message.set_delta_longitude(6789));
		EXPECT_TRUE(message.set_local_datum("abc1"));
		EXPECT_TRUE(message.set_reference_datum("def2"));

		EXPECT_EQ(25000, message.get_raw_delta_altitude());
		EXPECT_NEAR(25000 * 1E-2f, message.get_delta_altitude(), 0.1);

		EXPECT_EQ(12345, message.get_raw_delta_latitude());
		EXPECT_NEAR(12345 * 1E-7, message.get_delta_latitude(), 0.001);

		EXPECT_EQ(6789, message.get_raw_delta_longitude());
		EXPECT_NEAR(6789 * 1E-7, message.get_delta_longitude(), 0.001);

		EXPECT_EQ("abc1", message.get_local_datum());
		EXPECT_EQ("def2", message.get_reference_datum());

		while (SystemTiming::get_timestamp_ms() < message.get_timeout())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		interfaceUnderTest.update();
		CANNetworkManager::CANNetwork.update();
		ASSERT_TRUE(testPlugin.read_frame(testFrame));

		// Message encoding tested elsewhere, just verify PGN in the Fast packet
		EXPECT_EQ(0x1F814, (testFrame.identifier >> 8) & 0x1FFFF);

		std::vector<std::uint8_t> lastFastPacketPayload;
		lastFastPacketPayload.resize(20);
		memcpy(lastFastPacketPayload.data(), &testFrame.data[2], 6);

		// wait for the rest of the FP...
		ASSERT_TRUE(testPlugin.read_frame(testFrame)); // FP Payload 2
		memcpy(lastFastPacketPayload.data() + 6, &testFrame.data[1], 7);
		ASSERT_TRUE(testPlugin.read_frame(testFrame)); // FP Payload 3
		memcpy(lastFastPacketPayload.data() + 13, &testFrame.data[1], 7);

		std::vector<std::uint8_t> comparisonBuffer;
		ASSERT_NO_THROW(message.serialize(comparisonBuffer));

		for (std::uint8_t i = 0; i < 20; i++)
		{
			EXPECT_EQ(comparisonBuffer.at(i), lastFastPacketPayload.at(i));
		}

		EXPECT_EQ(0, interfaceUnderTest.get_number_received_datum_message_sources());
		EXPECT_EQ(nullptr, interfaceUnderTest.get_received_datum_message(0));

		interfaceUnderTest.get_datum_event_publisher().add_listener(test_datum_callback);

		// Pass the fast packet back in to simulate receiving
		testFrame.identifier = 0x19F81452;
		testFrame.data[0] = 0x00;
		testFrame.data[1] = 0x14;
		memcpy(&testFrame.data[2], lastFastPacketPayload.data(), 6);
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);

		testFrame.data[0] = 0x01;
		memcpy(&testFrame.data[1], lastFastPacketPayload.data() + 6, 7);
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);

		testFrame.data[0] = 0x02;
		memcpy(&testFrame.data[1], lastFastPacketPayload.data() + 13, 7);
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(wasDatumCallbackHit);
		EXPECT_EQ(1, interfaceUnderTest.get_number_received_datum_message_sources());
		EXPECT_NE(nullptr, interfaceUnderTest.get_received_datum_message(0));
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();

		// Make sure duplicate messages don't make more instances of the message's class
		EXPECT_EQ(1, interfaceUnderTest.get_number_received_datum_message_sources());
		EXPECT_NE(nullptr, interfaceUnderTest.get_received_datum_message(0));
	}

	{
		// Test GNSS Position Data
		NMEA2000MessageInterface interfaceUnderTest(testECU, false, false, true, false, false, false, false);

		EXPECT_FALSE(interfaceUnderTest.get_initialized());
		interfaceUnderTest.initialize();
		EXPECT_TRUE(interfaceUnderTest.get_initialized());

		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_cog_sog_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_datum_cyclically());
		EXPECT_TRUE(interfaceUnderTest.get_enable_sending_gnss_position_data_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_position_delta_high_precision_rapid_update_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_rate_of_turn_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_vessel_heading_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_position_rapid_update_cyclically());

		interfaceUnderTest.set_enable_sending_gnss_position_data_cyclically(false);
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_gnss_position_data_cyclically());
		interfaceUnderTest.set_enable_sending_gnss_position_data_cyclically(true);
		EXPECT_TRUE(interfaceUnderTest.get_enable_sending_gnss_position_data_cyclically());

		auto &message = interfaceUnderTest.get_gnss_position_data_transmit_message();

		EXPECT_TRUE(message.set_sequence_id(15));
		EXPECT_TRUE(message.set_geoidal_separation(10000));
		EXPECT_TRUE(message.set_gnss_method(GNSSPositionData::GNSSMethod::RTKFixedInteger));
		EXPECT_TRUE(message.set_horizontal_dilution_of_precision(-10));
		EXPECT_TRUE(message.set_integrity(GNSSPositionData::Integrity::Caution));
		EXPECT_TRUE(message.set_number_of_reference_stations(1));
		EXPECT_TRUE(message.set_number_of_space_vehicles(4));
		EXPECT_TRUE(message.set_positional_dilution_of_precision(-894));
		EXPECT_TRUE(message.set_timestamp(50));
		EXPECT_TRUE(message.set_type_of_system(GNSSPositionData::TypeOfSystem::GPSPlusSBASPlusGLONASS));
		EXPECT_TRUE(message.set_altitude(582000000));
		EXPECT_TRUE(message.set_latitude(-7205759403729808));
		EXPECT_TRUE(message.set_longitude(720575));
		EXPECT_TRUE(message.set_position_date(19551));
		EXPECT_TRUE(message.set_position_time(8400));
		EXPECT_TRUE(message.set_reference_station(0, 4, GNSSPositionData::TypeOfSystem::GLONASS, 100));

		std::vector<std::uint8_t> comparisonBuffer;
		ASSERT_NO_THROW(message.serialize(comparisonBuffer));

		std::vector<std::uint8_t> lastFastPacketPayload;
		lastFastPacketPayload.resize(47);

		interfaceUnderTest.update();
		CANNetworkManager::CANNetwork.update();
		ASSERT_TRUE(testPlugin.read_frame(testFrame));

		// Message encoding tested elsewhere, just verify PGN in the Fast packet
		EXPECT_EQ(0x1F805, (testFrame.identifier >> 8) & 0x1FFFF);
		memcpy(lastFastPacketPayload.data(), &testFrame.data[2], 6);

		// wait for the rest of the FP to complete
		ASSERT_TRUE(testPlugin.read_frame(testFrame)); // FP Payload 2
		memcpy(lastFastPacketPayload.data() + 6, &testFrame.data[1], 7);
		ASSERT_TRUE(testPlugin.read_frame(testFrame)); // FP Payload 3
		memcpy(lastFastPacketPayload.data() + 13, &testFrame.data[1], 7);
		ASSERT_TRUE(testPlugin.read_frame(testFrame)); // FP Payload 4
		memcpy(lastFastPacketPayload.data() + 20, &testFrame.data[1], 7);
		ASSERT_TRUE(testPlugin.read_frame(testFrame)); // FP Payload 5
		memcpy(lastFastPacketPayload.data() + 27, &testFrame.data[1], 7);
		ASSERT_TRUE(testPlugin.read_frame(testFrame)); // FP Payload 6
		memcpy(lastFastPacketPayload.data() + 34, &testFrame.data[1], 7);
		ASSERT_TRUE(testPlugin.read_frame(testFrame)); // FP Payload 7
		memcpy(lastFastPacketPayload.data() + 41, &testFrame.data[1], 5);

		for (std::uint8_t i = 0; i < 47; i++)
		{
			EXPECT_EQ(comparisonBuffer.at(i), lastFastPacketPayload.at(i));
		}

		interfaceUnderTest.get_gnss_position_data_event_publisher().add_listener(test_gnss_position_data_callback);

		// Pass the fast packet back in to simulate receiving
		testFrame.identifier = 0x19F80552;
		testFrame.data[0] = 0x00;
		testFrame.data[1] = 0x2F;
		memcpy(&testFrame.data[2], lastFastPacketPayload.data(), 6);
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);

		testFrame.data[0] = 0x01;
		memcpy(&testFrame.data[1], lastFastPacketPayload.data() + 6, 7);
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);

		testFrame.data[0] = 0x02;
		memcpy(&testFrame.data[1], lastFastPacketPayload.data() + 13, 7);
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);

		testFrame.data[0] = 0x03;
		memcpy(&testFrame.data[1], lastFastPacketPayload.data() + 20, 7);
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);

		testFrame.data[0] = 0x04;
		memcpy(&testFrame.data[1], lastFastPacketPayload.data() + 27, 7);
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);

		testFrame.data[0] = 0x05;
		memcpy(&testFrame.data[1], lastFastPacketPayload.data() + 34, 7);
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);

		testFrame.data[0] = 0x06;
		testFrame.data[7] = 0xFF;
		memcpy(&testFrame.data[1], lastFastPacketPayload.data() + 41, 7);
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(wasGNSSPositionDataCallbackHit);
		EXPECT_EQ(1, interfaceUnderTest.get_number_received_gnss_position_data_message_sources());
		EXPECT_NE(nullptr, interfaceUnderTest.get_received_gnss_position_data_message(0));

		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();

		// Make sure duplicate messages don't make more instances of the message's class
		EXPECT_EQ(1, interfaceUnderTest.get_number_received_gnss_position_data_message_sources());
		EXPECT_NE(nullptr, interfaceUnderTest.get_received_gnss_position_data_message(0));
	}

	{
		// Test position delta hs rapid update
		NMEA2000MessageInterface interfaceUnderTest(testECU, false, false, false, true, false, false, false);

		EXPECT_FALSE(interfaceUnderTest.get_initialized());
		interfaceUnderTest.initialize();
		EXPECT_TRUE(interfaceUnderTest.get_initialized());

		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_cog_sog_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_datum_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_gnss_position_data_cyclically());
		EXPECT_TRUE(interfaceUnderTest.get_enable_sending_position_delta_high_precision_rapid_update_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_rate_of_turn_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_vessel_heading_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_position_rapid_update_cyclically());

		interfaceUnderTest.set_enable_sending_position_delta_high_precision_rapid_update_cyclically(false);
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_position_delta_high_precision_rapid_update_cyclically());
		interfaceUnderTest.set_enable_sending_position_delta_high_precision_rapid_update_cyclically(true);
		EXPECT_TRUE(interfaceUnderTest.get_enable_sending_position_delta_high_precision_rapid_update_cyclically());

		auto &message = interfaceUnderTest.get_position_delta_high_precision_rapid_update_transmit_message();

		EXPECT_TRUE(message.set_latitude_delta(-5000));
		EXPECT_TRUE(message.set_longitude_delta(-9000));
		EXPECT_TRUE(message.set_sequence_id(49));
		EXPECT_TRUE(message.set_time_delta(7));

		interfaceUnderTest.update();
		CANNetworkManager::CANNetwork.update();
		ASSERT_TRUE(testPlugin.read_frame(testFrame));

		// Message encoding tested elsewhere, just verify PGN
		EXPECT_EQ(0x1F803, (testFrame.identifier >> 8) & 0x1FFFF);

		EXPECT_EQ(0, interfaceUnderTest.get_number_received_position_delta_high_precision_rapid_update_message_sources());
		EXPECT_EQ(nullptr, interfaceUnderTest.get_received_position_delta_high_precision_rapid_update_message(0));

		interfaceUnderTest.get_position_delta_high_precision_rapid_update_event_publisher().add_listener(test_position_delta_high_speed_rapid_update_callback);

		// Pass the message back in
		testFrame.identifier = 0x19F80352;

		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(wasPositionDeltaHighSpeedRapidUpdateCallbackHit);
		EXPECT_EQ(1, interfaceUnderTest.get_number_received_position_delta_high_precision_rapid_update_message_sources());
		EXPECT_NE(nullptr, interfaceUnderTest.get_received_position_delta_high_precision_rapid_update_message(0));

		// Update with a known message
		testFrame.data[0] = 0xC2;
		testFrame.data[1] = 0xBE;
		testFrame.data[2] = 0x02;
		testFrame.data[3] = 0x00;
		testFrame.data[4] = 0x00;
		testFrame.data[5] = 0x17;
		testFrame.data[6] = 0x00;
		testFrame.data[7] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		auto delta = interfaceUnderTest.get_received_position_delta_high_precision_rapid_update_message(0);

		EXPECT_NEAR(delta->get_latitude_delta(), 2E-6, 0.0001);
		EXPECT_NEAR(delta->get_longitude_delta(), 2.3E-5, 0.0001);
		EXPECT_NEAR(delta->get_time_delta(), 0.95, 0.001);
	}

	{
		// Test position rapid update
		NMEA2000MessageInterface interfaceUnderTest(testECU, false, false, false, false, true, false, false);

		EXPECT_FALSE(interfaceUnderTest.get_initialized());
		interfaceUnderTest.initialize();
		EXPECT_TRUE(interfaceUnderTest.get_initialized());

		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_cog_sog_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_datum_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_gnss_position_data_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_position_delta_high_precision_rapid_update_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_rate_of_turn_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_vessel_heading_cyclically());
		EXPECT_TRUE(interfaceUnderTest.get_enable_sending_position_rapid_update_cyclically());

		interfaceUnderTest.set_enable_sending_position_rapid_update_cyclically(false);
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_position_rapid_update_cyclically());
		interfaceUnderTest.set_enable_sending_position_rapid_update_cyclically(true);
		EXPECT_TRUE(interfaceUnderTest.get_enable_sending_position_rapid_update_cyclically());

		auto &message = interfaceUnderTest.get_position_rapid_update_transmit_message();

		EXPECT_TRUE(message.set_latitude(1000));
		EXPECT_TRUE(message.set_longitude(2000));

		interfaceUnderTest.update();
		CANNetworkManager::CANNetwork.update();
		ASSERT_TRUE(testPlugin.read_frame(testFrame));

		// Message encoding tested elsewhere, just verify PGN in the Fast packet
		EXPECT_EQ(0x1F801, (testFrame.identifier >> 8) & 0x1FFFF);

		EXPECT_EQ(0, interfaceUnderTest.get_number_received_datum_message_sources());
		EXPECT_EQ(nullptr, interfaceUnderTest.get_received_datum_message(0));

		interfaceUnderTest.get_position_rapid_update_event_publisher().add_listener(test_position_rapid_update_callback);

		// Pass the message back in
		testFrame.identifier = 0x19F80152;

		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(wasPositionRapidUpdateCallbackHit);
		EXPECT_EQ(1, interfaceUnderTest.get_number_received_position_rapid_update_message_sources());
		EXPECT_NE(nullptr, interfaceUnderTest.get_received_position_rapid_update_message(0));

		// Validate duplicates don't make more instances
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(wasPositionRapidUpdateCallbackHit);
		EXPECT_EQ(1, interfaceUnderTest.get_number_received_position_rapid_update_message_sources());
		EXPECT_NE(nullptr, interfaceUnderTest.get_received_position_rapid_update_message(0));
	}

	{
		// Test rate of turn
		NMEA2000MessageInterface interfaceUnderTest(testECU, false, false, false, false, false, true, false);

		EXPECT_FALSE(interfaceUnderTest.get_initialized());
		interfaceUnderTest.initialize();
		EXPECT_TRUE(interfaceUnderTest.get_initialized());

		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_cog_sog_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_datum_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_gnss_position_data_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_position_delta_high_precision_rapid_update_cyclically());
		EXPECT_TRUE(interfaceUnderTest.get_enable_sending_rate_of_turn_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_vessel_heading_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_position_rapid_update_cyclically());

		interfaceUnderTest.set_enable_sending_rate_of_turn_cyclically(false);
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_rate_of_turn_cyclically());
		interfaceUnderTest.set_enable_sending_rate_of_turn_cyclically(true);
		EXPECT_TRUE(interfaceUnderTest.get_enable_sending_rate_of_turn_cyclically());

		auto &message = interfaceUnderTest.get_rate_of_turn_transmit_message();

		EXPECT_TRUE(message.set_rate_of_turn(100));
		EXPECT_TRUE(message.set_sequence_id(200));

		interfaceUnderTest.update();
		CANNetworkManager::CANNetwork.update();
		ASSERT_TRUE(testPlugin.read_frame(testFrame));

		// Message encoding tested elsewhere, just verify PGN
		EXPECT_EQ(0x1F113, (testFrame.identifier >> 8) & 0x1FFFF);

		EXPECT_EQ(0, interfaceUnderTest.get_number_received_rate_of_turn_message_sources());
		EXPECT_EQ(nullptr, interfaceUnderTest.get_received_rate_of_turn_message(0));

		// Pass the message back in
		testFrame.identifier = 0x19F11352;

		interfaceUnderTest.get_rate_of_turn_event_publisher().add_listener(test_rate_of_turn_callback);

		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(wasRateOfTurnCallbackHit);
		EXPECT_EQ(1, interfaceUnderTest.get_number_received_rate_of_turn_message_sources());
		EXPECT_NE(nullptr, interfaceUnderTest.get_received_rate_of_turn_message(0));

		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(wasRateOfTurnCallbackHit);
		EXPECT_EQ(1, interfaceUnderTest.get_number_received_rate_of_turn_message_sources());
		EXPECT_NE(nullptr, interfaceUnderTest.get_received_rate_of_turn_message(0));
	}

	{
		// Test vessel heading
		NMEA2000MessageInterface interfaceUnderTest(testECU, false, false, false, false, false, false, true);

		EXPECT_FALSE(interfaceUnderTest.get_initialized());
		interfaceUnderTest.initialize();
		EXPECT_TRUE(interfaceUnderTest.get_initialized());

		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_cog_sog_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_datum_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_gnss_position_data_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_position_delta_high_precision_rapid_update_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_rate_of_turn_cyclically());
		EXPECT_TRUE(interfaceUnderTest.get_enable_sending_vessel_heading_cyclically());
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_position_rapid_update_cyclically());

		interfaceUnderTest.set_enable_sending_vessel_heading_cyclically(false);
		EXPECT_FALSE(interfaceUnderTest.get_enable_sending_vessel_heading_cyclically());
		interfaceUnderTest.set_enable_sending_vessel_heading_cyclically(true);
		EXPECT_TRUE(interfaceUnderTest.get_enable_sending_vessel_heading_cyclically());

		auto &message = interfaceUnderTest.get_vessel_heading_transmit_message();

		EXPECT_TRUE(message.set_heading(1));
		EXPECT_TRUE(message.set_magnetic_deviation(2));
		EXPECT_TRUE(message.set_magnetic_variation(-3));
		EXPECT_TRUE(message.set_sensor_reference(VesselHeading::HeadingSensorReference::True));
		EXPECT_TRUE(message.set_sequence_id(4));

		interfaceUnderTest.update();
		CANNetworkManager::CANNetwork.update();
		ASSERT_TRUE(testPlugin.read_frame(testFrame));

		// Message encoding tested elsewhere, just verify PGN in the Fast packet
		EXPECT_EQ(0x1F112, (testFrame.identifier >> 8) & 0x1FFFF);

		EXPECT_EQ(0, interfaceUnderTest.get_number_received_vessel_heading_message_sources());
		EXPECT_EQ(nullptr, interfaceUnderTest.get_received_vessel_heading_message(0));

		// Pass the message back in
		testFrame.identifier = 0x19F11252;

		interfaceUnderTest.get_vessel_heading_event_publisher().add_listener(test_vessel_heading_callback);

		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(wasVesselHeadingCallbackHit);
		EXPECT_EQ(1, interfaceUnderTest.get_number_received_vessel_heading_message_sources());
		EXPECT_NE(nullptr, interfaceUnderTest.get_received_vessel_heading_message(0));

		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		EXPECT_TRUE(wasVesselHeadingCallbackHit);
		EXPECT_EQ(1, interfaceUnderTest.get_number_received_vessel_heading_message_sources());
		EXPECT_NE(nullptr, interfaceUnderTest.get_received_vessel_heading_message(0));
	}

	CANHardwareInterface::stop();
}
