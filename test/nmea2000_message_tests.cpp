#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/nmea2000_message_definitions.hpp"
#include "isobus/isobus/nmea2000_message_interface.hpp"
#include "isobus/utility/system_timing.hpp"

using namespace isobus;
using namespace NMEA2000Messages;

static bool wasCourseOverGroundSpeedOverGroundRapidUpdateCallbackHit = false;

static void test_cog_sog_callback(const std::shared_ptr<CourseOverGroundSpeedOverGroundRapidUpdate>, bool)
{
	wasCourseOverGroundSpeedOverGroundRapidUpdateCallbackHit = true;
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
	EXPECT_TRUE(messageDataUnderTest.set_altitude(5820000000));
	EXPECT_TRUE(messageDataUnderTest.set_latitude(-72057594037298808));
	EXPECT_TRUE(messageDataUnderTest.set_longitude(720575));

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
	EXPECT_EQ(5820000000, messageDataUnderTest.get_raw_altitude());
	EXPECT_EQ(-72057594037298808, messageDataUnderTest.get_raw_latitude());
	EXPECT_EQ(720575, messageDataUnderTest.get_raw_longitude());
	EXPECT_NEAR(5820000000.0 * 10E-6, messageDataUnderTest.get_altitude(), 10E-4);
	EXPECT_NEAR(-72057594037298808.0 * 10E-16, messageDataUnderTest.get_latitude(), 10E-4);
	EXPECT_NEAR(720575.0 * 10E-16, messageDataUnderTest.get_longitude(), 10E-4);

	std::vector<std::uint8_t> messageBuffer;
	EXPECT_NO_THROW(messageDataUnderTest.serialize(messageBuffer));
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
	TestDeviceNAME.set_manufacturer_code(64);

	auto testECU = isobus::InternalControlFunction::create(TestDeviceNAME, 0x51, 0);

	CANMessageFrame testFrame;
	testFrame.timestamp_us = 0;
	testFrame.identifier = 0;
	testFrame.channel = 0;
	std::memset(testFrame.data, 0, sizeof(testFrame.data));
	testFrame.dataLength = 0;
	testFrame.isExtendedFrame = true;

	std::uint32_t waitingTimestamp_ms = SystemTiming::get_timestamp_ms();

	while ((!testECU->get_address_valid()) &&
	       (!SystemTiming::time_expired_ms(waitingTimestamp_ms, 2000)))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	ASSERT_TRUE(testECU->get_address_valid());

	// Force claim some other ECU
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(2);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::ExhaustEmissionControl));
	TestDeviceNAME.set_identity_number(275);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);
	testFrame.dataLength = 8;
	testFrame.channel = 0;
	testFrame.isExtendedFrame = true;
	testFrame.identifier = 0x18EEFF52;
	testFrame.data[0] = static_cast<std::uint8_t>(TestDeviceNAME.get_full_name() & 0xFF);
	testFrame.data[1] = static_cast<std::uint8_t>((TestDeviceNAME.get_full_name() >> 8) & 0xFF);
	testFrame.data[2] = static_cast<std::uint8_t>((TestDeviceNAME.get_full_name() >> 16) & 0xFF);
	testFrame.data[3] = static_cast<std::uint8_t>((TestDeviceNAME.get_full_name() >> 24) & 0xFF);
	testFrame.data[4] = static_cast<std::uint8_t>((TestDeviceNAME.get_full_name() >> 32) & 0xFF);
	testFrame.data[5] = static_cast<std::uint8_t>((TestDeviceNAME.get_full_name() >> 40) & 0xFF);
	testFrame.data[6] = static_cast<std::uint8_t>((TestDeviceNAME.get_full_name() >> 48) & 0xFF);
	testFrame.data[7] = static_cast<std::uint8_t>((TestDeviceNAME.get_full_name() >> 56) & 0xFF);
	CANNetworkManager::process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	// Get the virtual CAN plugin back to a known state
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
		EXPECT_EQ(CourseOverGroundSpeedOverGroundRapidUpdate::CourseOverGroudReference::NotApplicableOrNull, message.get_course_over_ground_reference());
		EXPECT_EQ(0, message.get_raw_course_over_ground());
		EXPECT_EQ(0, message.get_raw_speed_over_ground());
		EXPECT_EQ(0, message.get_sequence_id());
		EXPECT_EQ(0, message.get_speed_over_ground());
		EXPECT_EQ(250, message.get_timeout());
		EXPECT_EQ(0, message.get_timestamp());

		message.set_course_over_ground(10000);
		message.set_course_over_ground_reference(CourseOverGroundSpeedOverGroundRapidUpdate::CourseOverGroudReference::True);
		message.set_sequence_id(155);
		message.set_speed_over_ground(544);

		EXPECT_NEAR(1.0, message.get_course_over_ground(), 0.001);
		EXPECT_EQ(CourseOverGroundSpeedOverGroundRapidUpdate::CourseOverGroudReference::True, message.get_course_over_ground_reference());
		EXPECT_EQ(10000, message.get_raw_course_over_ground());
		EXPECT_EQ(544, message.get_raw_speed_over_ground());
		EXPECT_EQ(155, message.get_sequence_id());
		EXPECT_NEAR(5.44, message.get_speed_over_ground(), 0.001);

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

		auto listenerHandle = interfaceUnderTest.get_course_speed_over_ground_rapid_update_event_publisher().add_listener(test_cog_sog_callback);

		// Pass the frame back in but as an RX message
		testFrame.identifier = 0x19F80252;
		CANNetworkManager::process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();

		EXPECT_EQ(1, interfaceUnderTest.get_number_received_course_speed_over_ground_message_sources());
		EXPECT_NE(nullptr, interfaceUnderTest.get_received_course_speed_over_ground_message(0));

		EXPECT_TRUE(wasCourseOverGroundSpeedOverGroundRapidUpdateCallbackHit);
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
	}

	CANHardwareInterface::stop();
}
