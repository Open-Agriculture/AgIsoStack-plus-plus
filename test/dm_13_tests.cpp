#include <gtest/gtest.h>

#include "isobus/isobus/can_managed_message.hpp"
#include "isobus/isobus/isobus_diagnostic_protocol.hpp"

using namespace isobus;

TEST(DM13_TESTS, TestNetworkParsing)
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

TEST(DM13_TESTS, TestInvalidDM13Rejection)
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
