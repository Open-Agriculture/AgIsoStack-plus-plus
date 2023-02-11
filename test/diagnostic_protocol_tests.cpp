#include <gtest/gtest.h>

#include "isobus/isobus/can_managed_message.hpp"
#include "isobus/isobus/isobus_diagnostic_protocol.hpp"

using namespace isobus;

TEST(DIAGNOSTIC_PROTOCOL_TESTS, DM13TestNetworkParsing)
{
	std::uint32_t testNetworkStates = 0;
	CANIdentifier testID(CANIdentifier::Type::Extended,
	                     0xDF00,
	                     CANIdentifier::CANPriority::PriorityDefault6,
	                     0xFF,
	                     0x80);
	CANLibManagedMessage testDM13Message(0);
	testDM13Message.set_identifier(testID);
	testDM13Message.set_data_size(8);
	EXPECT_EQ(true, DiagnosticProtocol::parse_j1939_network_states(&testDM13Message, testNetworkStates));
}

TEST(DIAGNOSTIC_PROTOCOL_TESTS, TestInvalidDM13Rejection)
{
	std::uint32_t testNetworkStates = 0;
	CANIdentifier testID(CANIdentifier::Type::Extended,
	                     0xDF00,
	                     CANIdentifier::CANPriority::PriorityDefault6,
	                     0xFF,
	                     0x80);
	CANLibManagedMessage testDM13Message(0);
	testDM13Message.set_identifier(testID);
	testDM13Message.set_data_size(4);
	EXPECT_EQ(false, DiagnosticProtocol::parse_j1939_network_states(&testDM13Message, testNetworkStates));
}

TEST(DIAGNOSTIC_PROTOCOL_TESTS, CreateAndDestroyProtocolObjects)
{
	NAME TestDeviceNAME(0);

	auto TestInternalECU = std::make_shared<InternalControlFunction>(TestDeviceNAME, 0x1C, 0);

	DiagnosticProtocol::assign_diagnostic_protocol_to_internal_control_function(TestInternalECU);
	DiagnosticProtocol *diagnosticProtocol = DiagnosticProtocol::get_diagnostic_protocol_by_internal_control_function(TestInternalECU);
	EXPECT_NE(nullptr, diagnosticProtocol);

	if (nullptr != diagnosticProtocol)
	{
		EXPECT_NO_THROW(DiagnosticProtocol::deassign_all_diagnostic_protocol_to_internal_control_functions());
		EXPECT_EQ(nullptr, DiagnosticProtocol::get_diagnostic_protocol_by_internal_control_function(TestInternalECU));
	}
}
