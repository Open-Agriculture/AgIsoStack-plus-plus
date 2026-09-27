#include <gtest/gtest.h>

#include "isobus/isobus/isobus_virtual_terminal_server_managed_working_set.hpp"

using namespace isobus;

TEST(VirtualTerminalServerTest, AlreadyParsedIOPSegmentIsNotReparsed)
{
	VirtualTerminalServerManagedWorkingSet workingSet;

	// Container 0x1234:
	// ID          = 0x1234
	// Type        = Container
	// Width       = 100
	// Height      = 100
	// Hidden      = false
	// Children    = 0
	// Macros      = 0
	std::vector<std::uint8_t> firstSegment = {
		0x34, 0x12, static_cast<std::uint8_t>(VirtualTerminalObjectType::Container), 0x64, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00
	};

	workingSet.add_iop_raw_data(firstSegment);

	workingSet.start_parsing_thread();
	workingSet.join_parsing_thread();

	auto firstObject = workingSet.get_object_by_id(0x1234);
	ASSERT_NE(nullptr, firstObject);
	ASSERT_EQ(VirtualTerminalObjectType::Container, firstObject->get_object_type());

	auto firstContainer = std::static_pointer_cast<Container>(firstObject);

	EXPECT_FALSE(firstContainer->get_hidden());

	// Simulate a Change Child Position / Hide-Show related runtime state
	// change after the initial IOP has been parsed.
	firstContainer->set_hidden(true);

	EXPECT_TRUE(firstContainer->get_hidden());

	// A second dynamically transferred IOP segment.
	std::vector<std::uint8_t> secondSegment = {
		0x78, 0x56, static_cast<std::uint8_t>(VirtualTerminalObjectType::Container), 0x32, 0x00, 0x32, 0x00, 0x00, 0x00, 0x00
	};

	workingSet.add_iop_raw_data(secondSegment);

	workingSet.start_parsing_thread();
	workingSet.join_parsing_thread();

	// The new segment must have been parsed.
	auto secondObject = workingSet.get_object_by_id(0x5678);
	ASSERT_NE(nullptr, secondObject);
	EXPECT_EQ(VirtualTerminalObjectType::Container,
	          secondObject->get_object_type());

	// Most important regression check:
	// runtime state of an object from an already parsed segment must survive.
	auto firstObjectAfterSecondParse =
	  workingSet.get_object_by_id(0x1234);

	ASSERT_NE(nullptr, firstObjectAfterSecondParse);

	auto firstContainerAfterSecondParse =
	  std::static_pointer_cast<Container>(firstObjectAfterSecondParse);

	EXPECT_TRUE(firstContainerAfterSecondParse->get_hidden());

	// Make sure that it was not reconstructed/replaced at all.
	EXPECT_EQ(firstObject.get(), firstObjectAfterSecondParse.get());
}
