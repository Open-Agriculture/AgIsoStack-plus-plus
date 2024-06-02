//================================================================================================
/// @file language_command_interface_tests.cpp
///
/// @brief Unit tests for the LanguageCommandInterface class
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_NAME_filter.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_language_command_interface.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"

using namespace isobus;

TEST(LANGUAGE_COMMAND_INTERFACE_TESTS, BasicConstructionAndInit)
{
	NAME clientNAME(0);
	auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(clientNAME, 0, 0x26);
	LanguageCommandInterface interfaceUnderTest(internalECU);
	EXPECT_EQ(false, interfaceUnderTest.get_initialized());
	EXPECT_EQ(false, interfaceUnderTest.send_request_language_command());
	interfaceUnderTest.initialize();
	EXPECT_EQ(true, interfaceUnderTest.get_initialized());
	EXPECT_EQ(0, interfaceUnderTest.get_language_command_timestamp());
	interfaceUnderTest.initialize(); // double init
	EXPECT_EQ(true, interfaceUnderTest.get_initialized());

	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST(LANGUAGE_COMMAND_INTERFACE_TESTS, InvalidICF)
{
	LanguageCommandInterface interfaceUnderTest(nullptr);
	interfaceUnderTest.initialize();
	ASSERT_FALSE(interfaceUnderTest.send_request_language_command());
}

TEST(LANGUAGE_COMMAND_INTERFACE_TESTS, ValidPartner)
{
	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	vtNameFilters.push_back(testFilter);

	NAME clientNAME(0);
	auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(clientNAME, 0, 0x26);

	auto vtPartner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);
	LanguageCommandInterface interfaceUnderTest(internalECU, vtPartner);
	interfaceUnderTest.initialize();
	ASSERT_TRUE(interfaceUnderTest.get_initialized());
	// Technically our address is bad, so this should still not send
	//! @todo Test with valid address
	ASSERT_FALSE(interfaceUnderTest.send_request_language_command());

	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST(LANGUAGE_COMMAND_INTERFACE_TESTS, Uninitialized)
{
	LanguageCommandInterface interfaceUnderTest(nullptr);
	ASSERT_FALSE(interfaceUnderTest.get_initialized());
}

TEST(LANGUAGE_COMMAND_INTERFACE_TESTS, MessageContentParsing)
{
	NAME clientNAME(0);
	auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(clientNAME, 0, 0x80);
	LanguageCommandInterface interfaceUnderTest(internalECU, nullptr);

	interfaceUnderTest.initialize();

	// Make a message that is too short
	CANIdentifier identifier(CANIdentifier::Type::Extended, 0xFE0F, CANIdentifier::CANPriority::PriorityDefault6, 0x80, 0x81);
	CANMessage testMessage(CANMessage::Type::Receive, identifier, { 'r', 'u' }, nullptr, nullptr, 0);

	interfaceUnderTest.process_rx_message(testMessage, &interfaceUnderTest);

	// Should still be default values
	EXPECT_EQ("", interfaceUnderTest.get_language_code());

	// This contains: "en", Comma, 24 hour time, yyyymmdd, imperial, imperial, US, US, Metric, Metric, Imperial, Metric, "US", one junk byte at the end
	std::uint8_t testData[] = { 'e', 'n', 0x0F, 0x04, 0x5A, 0x04, 'U', 'S', 0xFF };

	testMessage.set_data_size(0); // Resets the CAN message data vector
	testMessage.set_data(testData, 9);

	interfaceUnderTest.process_rx_message(testMessage, &interfaceUnderTest);
	EXPECT_EQ("en", interfaceUnderTest.get_language_code());
	EXPECT_EQ(LanguageCommandInterface::DecimalSymbols::Comma, interfaceUnderTest.get_commanded_decimal_symbol());
	EXPECT_EQ(LanguageCommandInterface::TimeFormats::TwentyFourHour, interfaceUnderTest.get_commanded_time_format());
	EXPECT_EQ(LanguageCommandInterface::DateFormats::yyyymmdd, interfaceUnderTest.get_commanded_date_format());
	EXPECT_EQ(LanguageCommandInterface::DistanceUnits::ImperialUS, interfaceUnderTest.get_commanded_distance_units());
	EXPECT_EQ(LanguageCommandInterface::AreaUnits::ImperialUS, interfaceUnderTest.get_commanded_area_units());
	EXPECT_EQ(LanguageCommandInterface::VolumeUnits::US, interfaceUnderTest.get_commanded_volume_units());
	EXPECT_EQ(LanguageCommandInterface::MassUnits::US, interfaceUnderTest.get_commanded_mass_units());
	EXPECT_EQ(LanguageCommandInterface::TemperatureUnits::Metric, interfaceUnderTest.get_commanded_temperature_units());
	EXPECT_EQ(LanguageCommandInterface::PressureUnits::Metric, interfaceUnderTest.get_commanded_pressure_units());
	EXPECT_EQ(LanguageCommandInterface::ForceUnits::ImperialUS, interfaceUnderTest.get_commanded_force_units());
	EXPECT_EQ(LanguageCommandInterface::UnitSystem::Metric, interfaceUnderTest.get_commanded_generic_units());
	EXPECT_EQ("US", interfaceUnderTest.get_country_code());

	// This contains: "de", point, 12 hour time, ddmmyyyy, metric, no action, US, Metric, Reserved, Reserved, Imperial, Metric, No country code
	std::uint8_t testData2[] = { 'd', 'e', 0x58, 0x00, 0x38, 0xA4, 0xFF, 0xFF };

	testMessage.set_data_size(0); // Resets the CAN message data vector
	testMessage.set_data(testData2, 8);

	interfaceUnderTest.process_rx_message(testMessage, &interfaceUnderTest);
	EXPECT_EQ("de", interfaceUnderTest.get_language_code());
	EXPECT_EQ(LanguageCommandInterface::DecimalSymbols::Point, interfaceUnderTest.get_commanded_decimal_symbol());
	EXPECT_EQ(LanguageCommandInterface::TimeFormats::TwelveHourAmPm, interfaceUnderTest.get_commanded_time_format());
	EXPECT_EQ(LanguageCommandInterface::DateFormats::ddmmyyyy, interfaceUnderTest.get_commanded_date_format());
	EXPECT_EQ(LanguageCommandInterface::DistanceUnits::Metric, interfaceUnderTest.get_commanded_distance_units());
	EXPECT_EQ(LanguageCommandInterface::AreaUnits::NoAction, interfaceUnderTest.get_commanded_area_units());
	EXPECT_EQ(LanguageCommandInterface::VolumeUnits::US, interfaceUnderTest.get_commanded_volume_units());
	EXPECT_EQ(LanguageCommandInterface::MassUnits::Metric, interfaceUnderTest.get_commanded_mass_units());
	EXPECT_EQ(LanguageCommandInterface::TemperatureUnits::Reserved, interfaceUnderTest.get_commanded_temperature_units());
	EXPECT_EQ(LanguageCommandInterface::PressureUnits::Reserved, interfaceUnderTest.get_commanded_pressure_units());
	EXPECT_EQ(LanguageCommandInterface::ForceUnits::ImperialUS, interfaceUnderTest.get_commanded_force_units());
	EXPECT_EQ(LanguageCommandInterface::UnitSystem::Metric, interfaceUnderTest.get_commanded_generic_units());
	EXPECT_LT(SystemTiming::get_timestamp_ms() - interfaceUnderTest.get_language_command_timestamp(), 2);
	EXPECT_EQ("", interfaceUnderTest.get_country_code());

	// Use the language code as a way to assert against if we processed the message.
	// In other words, if it stays "de" then we didn't accept the message, and if it changed, we did
	testData2[0] = 'f';
	testData2[1] = 'r';
	testData2[6] = 75;
	testData2[7] = 37;
	testMessage.set_data_size(0); // Resets the CAN message data vector
	testMessage.set_data(testData2, 8);
	// Cover bad reserved bytes
	interfaceUnderTest.process_rx_message(testMessage, &interfaceUnderTest);
	// We still accept the message with strange reserved bytes, but would have printed an error
	EXPECT_EQ("fr", interfaceUnderTest.get_language_code());
	//! @todo assert that an warning log message came through

	testData2[0] = 'u';
	testData2[1] = 's';
	testData2[6] = 0xFF;
	testData2[7] = 37;
	testMessage.set_data_size(0); // Resets the CAN message data vector
	testMessage.set_data(testData2, 8);
	// Cover bad one bad reserved byte
	interfaceUnderTest.process_rx_message(testMessage, &interfaceUnderTest);
	EXPECT_EQ("us", interfaceUnderTest.get_language_code());
	//! @todo assert that an warning log message came through

	testData2[0] = 'p';
	testData2[1] = 'l';
	testData2[6] = 43;
	testData2[7] = 0xFF;
	testMessage.set_data_size(0); // Resets the CAN message data vector
	testMessage.set_data(testData2, 8);
	// Cover bad one bad reserved byte
	interfaceUnderTest.process_rx_message(testMessage, &interfaceUnderTest);
	EXPECT_EQ("pl", interfaceUnderTest.get_language_code());
	//! @todo assert that an warning log message came through

	// Cover null parent
	testData2[0] = 'r';
	testData2[1] = 'u';
	testData2[6] = 0xFF;
	testData2[7] = 0xFF;
	testMessage.set_data_size(0); // Resets the CAN message data vector
	testMessage.set_data(testData2, 8);
	interfaceUnderTest.process_rx_message(testMessage, nullptr);
	// Message should have been discarded
	EXPECT_EQ("pl", interfaceUnderTest.get_language_code());

	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST(LANGUAGE_COMMAND_INTERFACE_TESTS, SettersAndTransmitting)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	auto testECU = test_helpers::claim_internal_control_function(0x49, 0);

	CANMessageFrame testFrame;
	memset(&testFrame, 0, sizeof(testFrame));
	testFrame.isExtendedFrame = true;

	// Get the virtual CAN plugin back to a known state
	while (!testPlugin.get_queue_empty())
	{
		testPlugin.read_frame(testFrame);
	}
	ASSERT_TRUE(testPlugin.get_queue_empty());

	LanguageCommandInterface interfaceUnderTest(testECU, true);

	interfaceUnderTest.initialize();

	// Sending a request without setting the various string parameters should not emit a message
	EXPECT_FALSE(interfaceUnderTest.send_language_command());

	interfaceUnderTest.set_language_code("en");
	interfaceUnderTest.set_commanded_decimal_symbol(LanguageCommandInterface::DecimalSymbols::Comma);
	interfaceUnderTest.set_commanded_time_format(LanguageCommandInterface::TimeFormats::TwentyFourHour);
	interfaceUnderTest.set_commanded_date_format(LanguageCommandInterface::DateFormats::yyyymmdd);
	interfaceUnderTest.set_commanded_distance_units(LanguageCommandInterface::DistanceUnits::ImperialUS);
	interfaceUnderTest.set_commanded_area_units(LanguageCommandInterface::AreaUnits::ImperialUS);
	interfaceUnderTest.set_commanded_volume_units(LanguageCommandInterface::VolumeUnits::US);
	interfaceUnderTest.set_commanded_mass_units(LanguageCommandInterface::MassUnits::US);
	interfaceUnderTest.set_commanded_temperature_units(LanguageCommandInterface::TemperatureUnits::ImperialUS);
	interfaceUnderTest.set_commanded_pressure_units(LanguageCommandInterface::PressureUnits::ImperialUS);
	interfaceUnderTest.set_commanded_force_units(LanguageCommandInterface::ForceUnits::ImperialUS);
	interfaceUnderTest.set_commanded_generic_units(LanguageCommandInterface::UnitSystem::US);
	interfaceUnderTest.set_country_code("US");

	EXPECT_EQ("en", interfaceUnderTest.get_language_code());
	EXPECT_EQ(LanguageCommandInterface::DecimalSymbols::Comma, interfaceUnderTest.get_commanded_decimal_symbol());
	EXPECT_EQ(LanguageCommandInterface::TimeFormats::TwentyFourHour, interfaceUnderTest.get_commanded_time_format());
	EXPECT_EQ(LanguageCommandInterface::DateFormats::yyyymmdd, interfaceUnderTest.get_commanded_date_format());
	EXPECT_EQ(LanguageCommandInterface::DistanceUnits::ImperialUS, interfaceUnderTest.get_commanded_distance_units());
	EXPECT_EQ(LanguageCommandInterface::AreaUnits::ImperialUS, interfaceUnderTest.get_commanded_area_units());
	EXPECT_EQ(LanguageCommandInterface::VolumeUnits::US, interfaceUnderTest.get_commanded_volume_units());
	EXPECT_EQ(LanguageCommandInterface::MassUnits::US, interfaceUnderTest.get_commanded_mass_units());
	EXPECT_EQ(LanguageCommandInterface::TemperatureUnits::ImperialUS, interfaceUnderTest.get_commanded_temperature_units());
	EXPECT_EQ(LanguageCommandInterface::PressureUnits::ImperialUS, interfaceUnderTest.get_commanded_pressure_units());
	EXPECT_EQ(LanguageCommandInterface::ForceUnits::ImperialUS, interfaceUnderTest.get_commanded_force_units());
	EXPECT_EQ(LanguageCommandInterface::UnitSystem::US, interfaceUnderTest.get_commanded_generic_units());
	EXPECT_EQ("US", interfaceUnderTest.get_country_code());

	interfaceUnderTest.set_language_code("de");
	interfaceUnderTest.set_commanded_decimal_symbol(LanguageCommandInterface::DecimalSymbols::Reserved);
	interfaceUnderTest.set_commanded_time_format(LanguageCommandInterface::TimeFormats::TwelveHourAmPm);
	interfaceUnderTest.set_commanded_date_format(LanguageCommandInterface::DateFormats::mmddyyyy);
	interfaceUnderTest.set_commanded_distance_units(LanguageCommandInterface::DistanceUnits::Metric);
	interfaceUnderTest.set_commanded_area_units(LanguageCommandInterface::AreaUnits::Metric);
	interfaceUnderTest.set_commanded_volume_units(LanguageCommandInterface::VolumeUnits::Metric);
	interfaceUnderTest.set_commanded_mass_units(LanguageCommandInterface::MassUnits::Metric);
	interfaceUnderTest.set_commanded_temperature_units(LanguageCommandInterface::TemperatureUnits::Metric);
	interfaceUnderTest.set_commanded_pressure_units(LanguageCommandInterface::PressureUnits::Metric);
	interfaceUnderTest.set_commanded_force_units(LanguageCommandInterface::ForceUnits::Metric);
	interfaceUnderTest.set_commanded_generic_units(LanguageCommandInterface::UnitSystem::Metric);
	interfaceUnderTest.set_country_code("DE");

	EXPECT_EQ("de", interfaceUnderTest.get_language_code());
	EXPECT_EQ(LanguageCommandInterface::DecimalSymbols::Reserved, interfaceUnderTest.get_commanded_decimal_symbol());
	EXPECT_EQ(LanguageCommandInterface::TimeFormats::TwelveHourAmPm, interfaceUnderTest.get_commanded_time_format());
	EXPECT_EQ(LanguageCommandInterface::DateFormats::mmddyyyy, interfaceUnderTest.get_commanded_date_format());
	EXPECT_EQ(LanguageCommandInterface::DistanceUnits::Metric, interfaceUnderTest.get_commanded_distance_units());
	EXPECT_EQ(LanguageCommandInterface::AreaUnits::Metric, interfaceUnderTest.get_commanded_area_units());
	EXPECT_EQ(LanguageCommandInterface::VolumeUnits::Metric, interfaceUnderTest.get_commanded_volume_units());
	EXPECT_EQ(LanguageCommandInterface::MassUnits::Metric, interfaceUnderTest.get_commanded_mass_units());
	EXPECT_EQ(LanguageCommandInterface::TemperatureUnits::Metric, interfaceUnderTest.get_commanded_temperature_units());
	EXPECT_EQ(LanguageCommandInterface::PressureUnits::Metric, interfaceUnderTest.get_commanded_pressure_units());
	EXPECT_EQ(LanguageCommandInterface::ForceUnits::Metric, interfaceUnderTest.get_commanded_force_units());
	EXPECT_EQ(LanguageCommandInterface::UnitSystem::Metric, interfaceUnderTest.get_commanded_generic_units());
	EXPECT_EQ("DE", interfaceUnderTest.get_country_code());

	// Change settings back to the one that is trickier to encode/decode
	interfaceUnderTest.set_language_code("en");
	interfaceUnderTest.set_commanded_decimal_symbol(LanguageCommandInterface::DecimalSymbols::Comma);
	interfaceUnderTest.set_commanded_time_format(LanguageCommandInterface::TimeFormats::TwelveHourAmPm);
	interfaceUnderTest.set_commanded_date_format(LanguageCommandInterface::DateFormats::yyyymmdd);
	interfaceUnderTest.set_commanded_distance_units(LanguageCommandInterface::DistanceUnits::ImperialUS);
	interfaceUnderTest.set_commanded_area_units(LanguageCommandInterface::AreaUnits::ImperialUS);
	interfaceUnderTest.set_commanded_volume_units(LanguageCommandInterface::VolumeUnits::US);
	interfaceUnderTest.set_commanded_mass_units(LanguageCommandInterface::MassUnits::US);
	interfaceUnderTest.set_commanded_temperature_units(LanguageCommandInterface::TemperatureUnits::ImperialUS);
	interfaceUnderTest.set_commanded_pressure_units(LanguageCommandInterface::PressureUnits::ImperialUS);
	interfaceUnderTest.set_commanded_force_units(LanguageCommandInterface::ForceUnits::ImperialUS);
	interfaceUnderTest.set_commanded_generic_units(LanguageCommandInterface::UnitSystem::US);
	interfaceUnderTest.set_country_code("US");

	ASSERT_TRUE(interfaceUnderTest.send_language_command());

	testPlugin.read_frame(testFrame);

	EXPECT_EQ(8, testFrame.dataLength);
	EXPECT_TRUE(testFrame.isExtendedFrame);
	EXPECT_EQ(0x18FE0F49, testFrame.identifier);
	EXPECT_EQ('e', testFrame.data[0]);
	EXPECT_EQ('n', testFrame.data[1]);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::TimeFormats::TwelveHourAmPm), (testFrame.data[2] >> 4) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::DecimalSymbols::Comma), (testFrame.data[2] >> 6) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::DateFormats::yyyymmdd), testFrame.data[3]);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::MassUnits::US), (testFrame.data[4]) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::VolumeUnits::US), (testFrame.data[4] >> 2) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::AreaUnits::ImperialUS), (testFrame.data[4] >> 4) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::DistanceUnits::ImperialUS), (testFrame.data[4] >> 6) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::UnitSystem::US), (testFrame.data[5]) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::ForceUnits::ImperialUS), (testFrame.data[5] >> 2) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::PressureUnits::ImperialUS), (testFrame.data[5] >> 4) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::TemperatureUnits::ImperialUS), (testFrame.data[5] >> 6) & 0x03);
	EXPECT_EQ('U', testFrame.data[6]);
	EXPECT_EQ('S', testFrame.data[7]);

	// Test bad values for country and language
	interfaceUnderTest.set_language_code("r");
	interfaceUnderTest.set_country_code("");

	ASSERT_TRUE(interfaceUnderTest.send_language_command());

	testPlugin.read_frame(testFrame);

	EXPECT_EQ(8, testFrame.dataLength);
	EXPECT_TRUE(testFrame.isExtendedFrame);
	EXPECT_EQ(0x18FE0F49, testFrame.identifier);
	EXPECT_EQ('r', testFrame.data[0]);
	EXPECT_EQ(' ', testFrame.data[1]);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::TimeFormats::TwelveHourAmPm), (testFrame.data[2] >> 4) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::DecimalSymbols::Comma), (testFrame.data[2] >> 6) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::DateFormats::yyyymmdd), testFrame.data[3]);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::MassUnits::US), (testFrame.data[4]) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::VolumeUnits::US), (testFrame.data[4] >> 2) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::AreaUnits::ImperialUS), (testFrame.data[4] >> 4) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::DistanceUnits::ImperialUS), (testFrame.data[4] >> 6) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::UnitSystem::US), (testFrame.data[5]) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::ForceUnits::ImperialUS), (testFrame.data[5] >> 2) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::PressureUnits::ImperialUS), (testFrame.data[5] >> 4) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::TemperatureUnits::ImperialUS), (testFrame.data[5] >> 6) & 0x03);
	EXPECT_EQ(' ', testFrame.data[6]);
	EXPECT_EQ(' ', testFrame.data[7]);

	interfaceUnderTest.set_language_code("ThisIsWayTooLong");
	interfaceUnderTest.set_country_code("AndShouldBeTruncatedWhenSent");

	ASSERT_TRUE(interfaceUnderTest.send_language_command());

	testPlugin.read_frame(testFrame);

	EXPECT_EQ(8, testFrame.dataLength);
	EXPECT_TRUE(testFrame.isExtendedFrame);
	EXPECT_EQ(0x18FE0F49, testFrame.identifier);
	EXPECT_EQ('T', testFrame.data[0]);
	EXPECT_EQ('h', testFrame.data[1]);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::TimeFormats::TwelveHourAmPm), (testFrame.data[2] >> 4) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::DecimalSymbols::Comma), (testFrame.data[2] >> 6) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::DateFormats::yyyymmdd), testFrame.data[3]);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::MassUnits::US), (testFrame.data[4]) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::VolumeUnits::US), (testFrame.data[4] >> 2) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::AreaUnits::ImperialUS), (testFrame.data[4] >> 4) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::DistanceUnits::ImperialUS), (testFrame.data[4] >> 6) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::UnitSystem::US), (testFrame.data[5]) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::ForceUnits::ImperialUS), (testFrame.data[5] >> 2) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::PressureUnits::ImperialUS), (testFrame.data[5] >> 4) & 0x03);
	EXPECT_EQ(static_cast<std::uint8_t>(LanguageCommandInterface::TemperatureUnits::ImperialUS), (testFrame.data[5] >> 6) & 0x03);
	EXPECT_EQ('A', testFrame.data[6]);
	EXPECT_EQ('n', testFrame.data[7]);

	testPlugin.close();

	CANNetworkManager::CANNetwork.deactivate_control_function(testECU);
	CANHardwareInterface::stop();
}
