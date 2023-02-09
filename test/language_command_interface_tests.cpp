#include <gtest/gtest.h>

#include "isobus/isobus/can_NAME_filter.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_managed_message.hpp"
#include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_language_command_interface.hpp"

using namespace isobus;

// A log sink for the CAN stack
class CustomLogger : public isobus::CANStackLogger
{
public:
	void sink_CAN_stack_log(CANStackLogger::LoggingLevel, const std::string &) override
	{
		// Do nothing
	}
};

static CustomLogger logger;

TEST(LANGUAGE_COMMAND_INTERFACE_TESTS, BasicConstructionAndInit)
{
	NAME clientNAME(0);
	auto internalECU = std::make_shared<InternalControlFunction>(clientNAME, 0x26, 0);
	LanguageCommandInterface interfaceUnderTest(internalECU);
	EXPECT_EQ(false, interfaceUnderTest.get_initialized());
	EXPECT_EQ(false, interfaceUnderTest.send_request_language_command());
	interfaceUnderTest.initialize();
	EXPECT_EQ(true, interfaceUnderTest.get_initialized());
	EXPECT_EQ(0, interfaceUnderTest.get_language_command_timestamp());
	interfaceUnderTest.initialize(); // double init
	EXPECT_EQ(true, interfaceUnderTest.get_initialized());
	interfaceUnderTest.send_request_language_command();
}

TEST(LANGUAGE_COMMAND_INTERFACE_TESTS, InvalidICF)
{
	LanguageCommandInterface interfaceUnderTest(nullptr);
	interfaceUnderTest.initialize();
	interfaceUnderTest.send_request_language_command();
}

TEST(LANGUAGE_COMMAND_INTERFACE_TESTS, ValidPartner)
{
	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	vtNameFilters.push_back(testFilter);

	NAME clientNAME(0);
	auto internalECU = std::make_shared<InternalControlFunction>(clientNAME, 0x26, 0);

	auto vtPartner = std::make_shared<PartneredControlFunction>(0, vtNameFilters);
	LanguageCommandInterface interfaceUnderTest(internalECU, vtPartner);
	ParameterGroupNumberRequestProtocol::assign_pgn_request_protocol_to_internal_control_function(internalECU); // Pre-assign our ICF
	interfaceUnderTest.initialize();
	interfaceUnderTest.send_request_language_command();
}

TEST(LANGUAGE_COMMAND_INTERFACE_TESTS, Uninitialized)
{
	LanguageCommandInterface interfaceUnderTest(nullptr);
}

TEST(LANGUAGE_COMMAND_INTERFACE_TESTS, MessageContentParsing)
{
	NAME clientNAME(0);
	auto internalECU = std::make_shared<InternalControlFunction>(clientNAME, 0x80, 0);
	LanguageCommandInterface interfaceUnderTest(internalECU, nullptr);

	interfaceUnderTest.initialize();

	CANLibManagedMessage testMessage(0);
	testMessage.set_identifier(CANIdentifier(CANIdentifier::Type::Extended, 0xFE0F, CANIdentifier::PriorityDefault6, 0x80, 0x81));

	// Make a message that is too short
	std::uint8_t shortMessage[] = { 'r', 'u' };
	testMessage.set_data(shortMessage, 2);
	interfaceUnderTest.process_rx_message(&testMessage, &interfaceUnderTest);

	// Should still be default values
	EXPECT_EQ("", interfaceUnderTest.get_language_code());

	// This contains: "en", Comma, 24 hour time, yyyymmdd, imperial, imperial, US, US, Metric, Metric, Imperial, Metric, one junk byte at the end
	std::uint8_t testData[] = { 'e', 'n', 0b00001111, 0x04, 0b01011010, 0b00000100, 0xFF, 0xFF, 0xFF };

	testMessage.set_data_size(0); // Resets the CAN message data vector
	testMessage.set_data(testData, 9);

	interfaceUnderTest.process_rx_message(&testMessage, &interfaceUnderTest);
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

	interfaceUnderTest.process_rx_message(&testMessage, &interfaceUnderTest);
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
	EXPECT_LT(interfaceUnderTest.get_language_command_timestamp(), 100000);

	testData2[6] = 75;
	testData2[7] = 37;
	testMessage.set_data_size(0); // Resets the CAN message data vector
	testMessage.set_data(testData2, 8);
	// Cover bad reserved bytes
	interfaceUnderTest.process_rx_message(&testMessage, &interfaceUnderTest);

	testData2[6] = 0xFF;
	testData2[7] = 37;
	testMessage.set_data_size(0); // Resets the CAN message data vector
	testMessage.set_data(testData2, 8);
	// Cover bad one bad reserved byte
	interfaceUnderTest.process_rx_message(&testMessage, &interfaceUnderTest);

	testData2[6] = 43;
	testData2[7] = 0xFF;
	testMessage.set_data_size(0); // Resets the CAN message data vector
	testMessage.set_data(testData2, 8);
	// Cover bad one bad reserved byte
	interfaceUnderTest.process_rx_message(&testMessage, &interfaceUnderTest);

	// Cover null message
	interfaceUnderTest.process_rx_message(nullptr, &interfaceUnderTest);

	// Cover null parent
	interfaceUnderTest.process_rx_message(&testMessage, nullptr);

	// Cover all null parameters
	interfaceUnderTest.process_rx_message(nullptr, nullptr);

	// Cover some logger conditions
	CANStackLogger::set_can_stack_logger_sink(&logger);
	interfaceUnderTest.process_rx_message(nullptr, nullptr);
}
