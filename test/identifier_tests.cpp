#include <gtest/gtest.h>

#include "isobus/isobus/can_identifier.hpp"

using namespace isobus;

TEST(IDENTIFIER_TESTS, RawIdentifierConstuction)
{
	CANIdentifier testID(CANIdentifier::Type::Extended,
	                     0xEF00,
	                     CANIdentifier::CANPriority::PriorityDefault6,
	                     0x1C,
	                     0x80);
	EXPECT_EQ(CANIdentifier::Type::Extended, testID.get_identifier_type());
	EXPECT_EQ(CANIdentifier::CANPriority::PriorityDefault6, testID.get_priority());
	EXPECT_EQ(0xEF00, testID.get_parameter_group_number());
	EXPECT_EQ(0x1C, testID.get_destination_address());
	EXPECT_EQ(0x80, testID.get_source_address());
}
