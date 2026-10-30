#include <gtest/gtest.h>

#include "isobus/isobus/isobus_virtual_terminal_base.hpp"
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

TEST(VirtualTerminalServerTest, ParsingStateIsNotIdleOnceTheParsingThreadHasStarted)
{
	VirtualTerminalServerManagedWorkingSet workingSet;
	std::vector<std::uint8_t> segment = {
		0x34, 0x12, static_cast<std::uint8_t>(VirtualTerminalObjectType::Container), 0x64, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00
	};

	workingSet.add_iop_raw_data(segment);
	workingSet.start_parsing_thread();

	// The status bit and the pool command guard both read this before the worker thread runs
	const auto state = workingSet.get_object_pool_processing_state();
	EXPECT_NE(VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::None, state);
	EXPECT_NE(VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined, state);

	workingSet.join_parsing_thread();
}

TEST(VirtualTerminalServerTest, WorkingSetJoinsItsParsingThreadWhenDestroyed)
{
	// No assertion: a joinable std::thread left in the destructor aborts the binary
	VirtualTerminalServerManagedWorkingSet workingSet;
	workingSet.start_parsing_thread();
}

TEST(VirtualTerminalServerTest, MacroSkipsThePaddingOfAShortChangeStringValue)
{
	VirtualTerminalServerManagedWorkingSet workingSet;

	// Macro 5 holds a Change String Value of 2 bytes, padded with 0xFF to 8 bytes, then a Hide/Show Object
	std::vector<std::uint8_t> segment = {
		0x05, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::Macro), 0x10, 0x00, static_cast<std::uint8_t>(Macro::Command::ChangeStringValue), 0x34, 0x12, 0x02, 0x00, 'O', 'K', 0xFF, static_cast<std::uint8_t>(Macro::Command::HideShowObject), 0x34, 0x12, 0x01, 0xFF, 0xFF, 0xFF, 0xFF
	};

	workingSet.add_iop_raw_data(segment);

	workingSet.start_parsing_thread();
	workingSet.join_parsing_thread();

	auto object = workingSet.get_object_by_id(5);
	ASSERT_NE(nullptr, object);
	ASSERT_EQ(VirtualTerminalObjectType::Macro, object->get_object_type());

	auto macro = std::static_pointer_cast<Macro>(object);
	ASSERT_EQ(2, macro->get_number_of_commands());

	std::vector<std::uint8_t> command;
	ASSERT_TRUE(macro->get_command_packet(1, command));
	EXPECT_EQ(static_cast<std::uint8_t>(Macro::Command::HideShowObject), command.at(0));
}

TEST(VirtualTerminalServerTest, MacroReadsEachGraphicsContextCommandAtItsOwnLength)
{
	VirtualTerminalServerManagedWorkingSet workingSet;

	// Macro 5 holds a Draw Text of 1 character padded with 0xFF to 8 bytes, a 12 byte Pan and Zoom Viewport, then a Hide/Show Object
	std::vector<std::uint8_t> segment = {
		0x05, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::Macro), 0x1C, 0x00, static_cast<std::uint8_t>(Macro::Command::GraphicsContextCommand), 0x34, 0x12, static_cast<std::uint8_t>(VirtualTerminalBase::GraphicsContextSubCommandID::DrawText), 0x00, 0x01, 'A', 0xFF, static_cast<std::uint8_t>(Macro::Command::GraphicsContextCommand), 0x34, 0x12, static_cast<std::uint8_t>(VirtualTerminalBase::GraphicsContextSubCommandID::PanAndZoomViewport), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, static_cast<std::uint8_t>(Macro::Command::HideShowObject), 0x34, 0x12, 0x01, 0xFF, 0xFF, 0xFF, 0xFF
	};

	workingSet.add_iop_raw_data(segment);

	workingSet.start_parsing_thread();
	workingSet.join_parsing_thread();

	auto object = workingSet.get_object_by_id(5);
	ASSERT_NE(nullptr, object);
	ASSERT_EQ(VirtualTerminalObjectType::Macro, object->get_object_type());

	auto macro = std::static_pointer_cast<Macro>(object);
	ASSERT_EQ(3, macro->get_number_of_commands());

	std::vector<std::uint8_t> command;
	ASSERT_TRUE(macro->get_command_packet(1, command));
	EXPECT_EQ(12, command.size());
	ASSERT_TRUE(macro->get_command_packet(2, command));
	EXPECT_EQ(static_cast<std::uint8_t>(Macro::Command::HideShowObject), command.at(0));
}

TEST(VirtualTerminalServerTest, ObjectPoolTransferDropsThePaddingOfASingleFrame)
{
	VirtualTerminalServerManagedWorkingSet workingSet;

	// Object Pointer 0x1234 to the NULL object, padded with 0xFF to fill the 8 byte frame after the mux byte
	workingSet.add_iop_raw_data({ 0x34, 0x12, static_cast<std::uint8_t>(VirtualTerminalObjectType::ObjectPointer), 0xFF, 0xFF, 0xFF, 0xFF });

	workingSet.start_parsing_thread();
	workingSet.join_parsing_thread();

	auto object = workingSet.get_object_by_id(0x1234);
	ASSERT_NE(nullptr, object);
	EXPECT_EQ(NULL_OBJECT_ID, std::static_pointer_cast<ObjectPointer>(object)->get_value());

	std::vector<std::uint8_t> withoutPadding = { 0x34, 0x12, static_cast<std::uint8_t>(VirtualTerminalObjectType::ObjectPointer), 0xFF, 0xFF };
	EXPECT_EQ(withoutPadding, workingSet.get_iop_raw_data(0));
}

TEST(VirtualTerminalServerTest, ObjectPoolTransferAcceptsAtMostSevenBytesOfPadding)
{
	// Object Pointer 0x1234 to the NULL object, then 7 bytes of 0xFF
	std::vector<std::uint8_t> segment = {
		0x34, 0x12, static_cast<std::uint8_t>(VirtualTerminalObjectType::ObjectPointer), 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};
	VirtualTerminalServerManagedWorkingSet sevenBytes;
	EXPECT_TRUE(sevenBytes.parse_iop_into_objects(segment.data(), static_cast<std::uint32_t>(segment.size())));

	segment.push_back(0xFF);
	VirtualTerminalServerManagedWorkingSet eightBytes;
	EXPECT_FALSE(eightBytes.parse_iop_into_objects(segment.data(), static_cast<std::uint32_t>(segment.size())));
}

TEST(VirtualTerminalServerTest, MacroRejectsACommandLongerThanTheMacro)
{
	// Macro 5 of 8 bytes holding a Draw Polygon of 255 points
	std::vector<std::uint8_t> polygon = {
		0x05, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::Macro), 0x08, 0x00, static_cast<std::uint8_t>(Macro::Command::GraphicsContextCommand), 0x34, 0x12, static_cast<std::uint8_t>(VirtualTerminalBase::GraphicsContextSubCommandID::DrawPolygon), 0xFF, 0xFF, 0xFF, 0xFF
	};
	VirtualTerminalServerManagedWorkingSet polygonWorkingSet;
	EXPECT_FALSE(polygonWorkingSet.parse_iop_into_objects(polygon.data(), static_cast<std::uint32_t>(polygon.size())));

	// Macro 5 of 8 bytes holding a Change String Value of 65535 bytes
	std::vector<std::uint8_t> string = {
		0x05, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::Macro), 0x08, 0x00, static_cast<std::uint8_t>(Macro::Command::ChangeStringValue), 0x34, 0x12, 0xFF, 0xFF, 'O', 'K', 0xFF
	};
	VirtualTerminalServerManagedWorkingSet stringWorkingSet;
	EXPECT_FALSE(stringWorkingSet.parse_iop_into_objects(string.data(), static_cast<std::uint32_t>(string.size())));
}

TEST(VirtualTerminalServerTest, AlarmMaskKeepsItsAcousticSignal)
{
	// Alarm Mask 0x1234: white, no soft key mask, low priority, the acoustic signal under test,
	// no children, no macros. The signal is 0 highest, 1 medium, 2 lowest, 3 none.
	const auto alarmMask = [](std::uint8_t signal) {
		return std::vector<std::uint8_t>{ 0x34, 0x12, static_cast<std::uint8_t>(VirtualTerminalObjectType::AlarmMask), 0x01, 0xFF, 0xFF, 0x02, signal, 0x00, 0x00 };
	};
	const AlarmMask::AcousticSignal expected[] = { AlarmMask::AcousticSignal::Highest, AlarmMask::AcousticSignal::Medium, AlarmMask::AcousticSignal::Lowest, AlarmMask::AcousticSignal::None };

	for (std::uint8_t signal = 0; signal < 4; signal++)
	{
		auto segment = alarmMask(signal);
		VirtualTerminalServerManagedWorkingSet workingSet;
		ASSERT_TRUE(workingSet.parse_iop_into_objects(segment.data(), static_cast<std::uint32_t>(segment.size())));
		auto object = std::static_pointer_cast<AlarmMask>(workingSet.get_object_by_id(0x1234));
		ASSERT_NE(nullptr, object);
		EXPECT_EQ(expected[signal], object->get_signal_priority());
		EXPECT_EQ(signal, static_cast<std::uint8_t>(object->get_signal_priority()));
	}

	auto outOfRange = alarmMask(4);
	VirtualTerminalServerManagedWorkingSet workingSet;
	EXPECT_FALSE(workingSet.parse_iop_into_objects(outOfRange.data(), static_cast<std::uint32_t>(outOfRange.size())));
}
