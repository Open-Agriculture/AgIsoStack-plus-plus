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

TEST(VirtualTerminalServerTest, ObjectThatRunsPastTheEndOfTheTransferIsRejected)
{
	const std::uint8_t eightBitColour = static_cast<std::uint8_t>(PictureGraphic::Format::EightBitColour);
	const std::vector<std::vector<std::uint8_t>> truncatedObjects = {
		// Container with no macro count byte
		{ 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::Container), 0x0A, 0x00, 0x0A, 0x00, 0x00, 0x00 },
		// Key Group with one child, cut off after its first byte
		{ 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::KeyGroup), 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x34 },
		// Output String whose value ends the transfer, with no macro count byte
		{ 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::OutputString), 0x0A, 0x00, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x02, 0x00, 'O', 'K' },
		// Input Attributes whose validation string ends the transfer, with no macro count byte
		{ 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::InputAttributes), 0x00, 0x02, 'A', 'B' },
		// Colour Map of 2 indexes with only 1 present
		{ 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::ColourMap), 0x02, 0x00, 0x01 },
		// Macro whose Change String Value is 9 bytes long in an 8 byte macro
		{ 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::Macro), 0x08, 0x00, static_cast<std::uint8_t>(Macro::Command::ChangeStringValue), 0x34, 0x12, 0x04, 0x00, 'A', 'B', 'C' },
		// 1x1 Picture Graphic with an odd RLE byte count
		{ 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::PictureGraphic), 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, eightBitColour, 0x04, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF },
		// 1x1 Picture Graphic with one macro reference and no bytes for it
		{ 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::PictureGraphic), 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, eightBitColour, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x07 },
		// Container of 1 macro grouping whose extended reference ends the transfer
		{ 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::Container), 0x0A, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x01, 0xFF, 0x02 },
		// Working Set Special Controls cut off in its header
		{ 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::WorkingSetSpecialControls), 0x05 },
		// Working Set Special Controls declaring one more byte than the transfer holds
		{ 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::WorkingSetSpecialControls), 0x06, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 },
		// Auxiliary Function Type 1 declaring 43 children (258 bytes, which wraps a uint8) with only one present
		{ 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::AuxiliaryFunctionType1), 0x00, 0x00, 0x2B, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00 },
	};

	for (std::size_t i = 0; i < truncatedObjects.size(); i++)
	{
		SCOPED_TRACE(i);
		const std::vector<std::uint8_t> &object = truncatedObjects[i];

		// Without a sanitizer, an over-read only shows if these bytes get parsed into an accepted object
		std::vector<std::uint8_t> guarded = object;
		guarded.insert(guarded.end(), { 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

		VirtualTerminalServerManagedWorkingSet guardedWorkingSet;
		std::uint32_t objectsLength = 0;
		EXPECT_FALSE(guardedWorkingSet.parse_iop_into_objects(guarded.data(), static_cast<std::uint32_t>(object.size()), &objectsLength));
		EXPECT_LE(objectsLength, object.size());
		EXPECT_EQ(nullptr, guardedWorkingSet.get_object_by_id(1));

		// Under ASan, any read past the transfer faults here
		std::vector<std::uint8_t> exact = object;
		VirtualTerminalServerManagedWorkingSet exactWorkingSet;
		EXPECT_FALSE(exactWorkingSet.parse_iop_into_objects(exact.data(), static_cast<std::uint32_t>(exact.size())));
	}
}

TEST(VirtualTerminalServerTest, ObjectThatEndsTheTransferIsAccepted)
{
	const std::uint8_t eightBitColour = static_cast<std::uint8_t>(PictureGraphic::Format::EightBitColour);
	const std::vector<std::vector<std::uint8_t>> objects = {
		// Input Attributes with an empty validation string
		{ 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::InputAttributes), 0x00, 0x00, 0x00 },
		// Input String with an empty value
		{ 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::InputString), 0x0A, 0x00, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00 },
		// 1x1 Picture Graphic with an extra pixel byte
		{ 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::PictureGraphic), 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, eightBitColour, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x07, 0x08 },
		// Picture Graphic with an actual width of 0
		{ 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::PictureGraphic), 0x0A, 0x00, 0x00, 0x00, 0x01, 0x00, eightBitColour, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	};

	for (std::size_t i = 0; i < objects.size(); i++)
	{
		SCOPED_TRACE(i);
		std::vector<std::uint8_t> object = objects[i];
		VirtualTerminalServerManagedWorkingSet workingSet;
		EXPECT_TRUE(workingSet.parse_iop_into_objects(object.data(), static_cast<std::uint32_t>(object.size())));
		EXPECT_NE(nullptr, workingSet.get_object_by_id(1));
	}
}

TEST(VirtualTerminalServerTest, KeyGroupReadsBothCountsBeforeItsChildren)
{
	// 1 child (0x1234), 1 macro (5 on show)
	std::vector<std::uint8_t> object = { 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::KeyGroup), 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x01, 0x34, 0x12, 0x03, 0x05 };

	VirtualTerminalServerManagedWorkingSet workingSet;
	ASSERT_TRUE(workingSet.parse_iop_into_objects(object.data(), static_cast<std::uint32_t>(object.size())));

	auto keyGroup = workingSet.get_object_by_id(1);
	ASSERT_NE(nullptr, keyGroup);
	ASSERT_EQ(1u, keyGroup->get_number_children());
	EXPECT_EQ(0x1234u, keyGroup->get_child_id(0));
	ASSERT_EQ(1u, keyGroup->get_number_macros());
	EXPECT_EQ(EventID::OnShow, keyGroup->get_macro(0).event);
	EXPECT_EQ(5u, keyGroup->get_macro(0).macroID);
}

TEST(VirtualTerminalServerTest, SixteenBitMacroReferenceTakesTwoMacroGroupings)
{
	// 3 groupings: macro 7 on hide, macro 7000 on hide, then a second Container
	std::vector<std::uint8_t> pool = {
		0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::Container), 0x0A, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x03, 0x04, 0x07, 0xFF, 0x58, 0x04, 0x1B, 0x02, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::Container), 0x0A, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00
	};

	VirtualTerminalServerManagedWorkingSet workingSet;
	ASSERT_TRUE(workingSet.parse_iop_into_objects(pool.data(), static_cast<std::uint32_t>(pool.size())));

	auto container = workingSet.get_object_by_id(1);
	ASSERT_NE(nullptr, container);
	ASSERT_EQ(2u, container->get_number_macros());
	EXPECT_EQ(EventID::OnHide, container->get_macro(0).event);
	EXPECT_EQ(7u, container->get_macro(0).macroID);
	EXPECT_EQ(EventID::OnHide, container->get_macro(1).event);
	EXPECT_EQ(7000u, container->get_macro(1).macroID);
	EXPECT_NE(nullptr, workingSet.get_object_by_id(2));

	// 1 grouping can't hold a 16-bit reference, even with the bytes present
	std::vector<std::uint8_t> oneGrouping = { 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::Container), 0x0A, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x01, 0xFF, 0x58, 0x04, 0x1B };
	VirtualTerminalServerManagedWorkingSet oneGroupingWorkingSet;
	EXPECT_FALSE(oneGroupingWorkingSet.parse_iop_into_objects(oneGrouping.data(), static_cast<std::uint32_t>(oneGrouping.size())));
}

TEST(VirtualTerminalServerTest, WorkingSetSpecialControlsReadsEachLanguagePairOnce)
{
	std::vector<std::uint8_t> object = { 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::WorkingSetSpecialControls), 0x0D, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 'e', 'n', 'U', 'S', 'd', 'e', 'D', 'E' };

	VirtualTerminalServerManagedWorkingSet workingSet;
	ASSERT_TRUE(workingSet.parse_iop_into_objects(object.data(), static_cast<std::uint32_t>(object.size())));

	auto specialControls = std::static_pointer_cast<WorkingSetSpecialControlsObject>(workingSet.get_object_by_id(1));
	ASSERT_NE(nullptr, specialControls);
	const auto &codes = specialControls->get_language_codes();
	ASSERT_EQ(2u, codes.size());
	EXPECT_EQ("en", codes[0].languageCode);
	EXPECT_EQ("US", codes[0].countryCode);
	EXPECT_EQ("de", codes[1].languageCode);
	EXPECT_EQ("DE", codes[1].countryCode);
}
