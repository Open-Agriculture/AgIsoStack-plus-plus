#include <gtest/gtest.h>

#include "isobus/isobus/can_NAME_filter.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_language_command_interface.hpp"
#include "isobus/utility/system_timing.hpp"

using namespace isobus;

TEST(LANGUAGE_COMMAND_INTERFACE_TESTS, BasicConstructionAndInit)
{
	NAME clientNAME(0);
	auto internalECU = InternalControlFunction::create(clientNAME, 0x26, 0);
	LanguageCommandInterface interfaceUnderTest(internalECU);
	EXPECT_EQ(false, interfaceUnderTest.get_initialized());
	EXPECT_EQ(false, interfaceUnderTest.send_request_language_command());
	interfaceUnderTest.initialize();
	EXPECT_EQ(true, interfaceUnderTest.get_initialized());
	EXPECT_EQ(0, interfaceUnderTest.get_language_command_timestamp());
	interfaceUnderTest.initialize(); // double init
	EXPECT_EQ(true, interfaceUnderTest.get_initialized());

	//! @todo try to reduce the reference count, such that that we don't use a control function after it is destroyed
	ASSERT_TRUE(internalECU->destroy(3));
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
	auto internalECU = InternalControlFunction::create(clientNAME, 0x26, 0);

	auto vtPartner = PartneredControlFunction::create(0, vtNameFilters);
	LanguageCommandInterface interfaceUnderTest(internalECU, vtPartner);
	interfaceUnderTest.initialize();
	ASSERT_TRUE(interfaceUnderTest.get_initialized());
	// Technically our address is bad, so this should still not send
	//! @todo Test with valid address
	ASSERT_FALSE(interfaceUnderTest.send_request_language_command());

	//! @todo try to reduce the reference count, such that that we don't use a control function after it is destroyed
	ASSERT_TRUE(vtPartner->destroy(2));
	ASSERT_TRUE(internalECU->destroy(3));
}

TEST(LANGUAGE_COMMAND_INTERFACE_TESTS, Uninitialized)
{
	LanguageCommandInterface interfaceUnderTest(nullptr);
	ASSERT_FALSE(interfaceUnderTest.get_initialized());
}

TEST(LANGUAGE_COMMAND_INTERFACE_TESTS, MessageContentParsing)
{
	NAME clientNAME(0);
	auto internalECU = InternalControlFunction::create(clientNAME, 0x80, 0);
	LanguageCommandInterface interfaceUnderTest(internalECU, nullptr);

	interfaceUnderTest.initialize();

	CANMessage testMessage(0);
	testMessage.set_identifier(CANIdentifier(CANIdentifier::Type::Extended, 0xFE0F, CANIdentifier::PriorityDefault6, 0x80, 0x81));

	// Make a message that is too short
	std::uint8_t shortMessage[] = { 'r', 'u' };
	testMessage.set_data(shortMessage, 2);
	interfaceUnderTest.process_rx_message(testMessage, &interfaceUnderTest);

	// Should still be default values
	EXPECT_EQ("", interfaceUnderTest.get_language_code());

	// This contains: "en", Comma, 24 hour time, yyyymmdd, imperial, imperial, US, US, Metric, Metric, Imperial, Metric, one junk byte at the end
	std::uint8_t testData[] = { 'e', 'n', 0b00001111, 0x04, 0b01011010, 0b00000100, 0xFF, 0xFF, 0xFF };

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

	// This contains: "de", point, 12 hour time, ddmmyyyy, metric, no action, US, Metric, Reserved, Reserved, Imperial, Metric
	std::uint8_t testData2[] = { 'd', 'e', 0b01011000, 0x00, 0b00111000, 0b10100100, 0xFF, 0xFF };

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
	EXPECT_LT(SystemTiming::get_timestamp_ms() - interfaceUnderTest.get_language_command_timestamp(), 1);

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

	//! @todo try to reduce the reference count, such that that we don't use a control function after it is destroyed
	ASSERT_TRUE(internalECU->destroy(3));
}
