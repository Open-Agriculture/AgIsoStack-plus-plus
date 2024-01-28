#include <gtest/gtest.h>

#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/isobus_device_descriptor_object_pool.hpp"
#include "isobus/isobus/isobus_language_command_interface.hpp"
#include "isobus/isobus/isobus_standard_data_description_indices.hpp"
#include "isobus/utility/to_string.hpp"

using namespace isobus;

static constexpr std::size_t NUMBER_SECTIONS_TO_CREATE = 16;

enum class SprayerDDOPObjectIDs : std::uint16_t
{
	Device = 0,

	MainDeviceElement,
	DeviceActualWorkState,
	DeviceTotalTime,

	Connector,
	ConnectorXOffset,
	ConnectorYOffset,
	ConnectorType,

	SprayBoom,
	ActualWorkState,
	ActualWorkingWidth,
	AreaTotal,
	SetpointWorkState,
	SectionCondensedWorkState1_16,
	BoomXOffset,
	BoomYOffset,
	BoomZOffset,

	Section1,
	SectionMax = Section1 + (NUMBER_SECTIONS_TO_CREATE - 1),
	Section1XOffset,
	SectionXOffsetMax = Section1XOffset + (NUMBER_SECTIONS_TO_CREATE - 1),
	Section1YOffset,
	SectionYOffsetMax = Section1YOffset + (NUMBER_SECTIONS_TO_CREATE - 1),
	Section1Width,
	SectionWidthMax = Section1Width + (NUMBER_SECTIONS_TO_CREATE - 1),
	ActualCondensedWorkingState,
	SetpointCondensedWorkingState,

	LiquidProduct,
	TankCapacity,
	TankVolume,

	AreaPresentation,
	TimePresentation,
	ShortWidthPresentation,
	LongWidthPresentation,
	VolumePresentation
};

TEST(DDOP_TESTS, CreateSprayerDDOP)
{
	DeviceDescriptorObjectPool testDDOP;

	// Build up a sprayer's DDOP
	LanguageCommandInterface testLanguageInterface(nullptr, nullptr);

	// Test a nonsense TC version gets asserted
	EXPECT_DEATH(DeviceDescriptorObjectPool badDDOP(200), "");

	// Make a test pool, don't care about our ISO NAME, Localization label, or extended structure label for this test
	// Set up device
	EXPECT_EQ(true, testDDOP.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(true, testDDOP.add_device_element("Sprayer", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), 0, task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Actual Work State", static_cast<std::uint16_t>(DataDescriptionIndex::ActualWorkState), NULL_OBJECT_ID, static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceActualWorkState)));
	EXPECT_EQ(true, testDDOP.add_device_value_presentation("minutes", 0, 1.0f, 1, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TimePresentation)));
	EXPECT_EQ(true, testDDOP.add_device_element("Connector", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Connector), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), task_controller_object::DeviceElementObject::Type::Connector, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Connector)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Connector X", static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetX), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorXOffset)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Connector Y", static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetY), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorYOffset)));
	EXPECT_EQ(true, testDDOP.add_device_property("Type", 6, static_cast<std::uint16_t>(DataDescriptionIndex::ConnectorType), NULL_OBJECT_ID, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorType)));

	// Set up Boom
	EXPECT_EQ(true, testDDOP.add_device_element("Boom", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom), 0, task_controller_object::DeviceElementObject::Type::Function, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom)));
	EXPECT_EQ(true, testDDOP.add_device_property("Offset X", 0, static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetX), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::BoomXOffset)));
	EXPECT_EQ(true, testDDOP.add_device_property("Offset Y", 0, static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetY), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::BoomYOffset)));
	EXPECT_EQ(true, testDDOP.add_device_property("Offset Z", 0, static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetZ), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::BoomZOffset)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Actual Working Width", static_cast<std::uint16_t>(DataDescriptionIndex::ActualWorkingWidth), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ActualWorkingWidth)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Setpoint Work State", static_cast<std::uint16_t>(DataDescriptionIndex::SetpointWorkState), NULL_OBJECT_ID, static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SetpointWorkState)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Area Total", static_cast<std::uint16_t>(DataDescriptionIndex::TotalArea), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::AreaPresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::Total), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::AreaTotal)));

	// Set up sections for section control
	// Using 7 ft sections
	for (std::size_t i = 0; i < NUMBER_SECTIONS_TO_CREATE; i++)
	{
		EXPECT_EQ(true, testDDOP.add_device_element("Section " + isobus::to_string(static_cast<int>(i)), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Section1) + i, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom), task_controller_object::DeviceElementObject::Type::Section, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Section1) + i));
		EXPECT_EQ(true, testDDOP.add_device_property("Offset X", -20, static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetX), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Section1XOffset) + i));
		EXPECT_EQ(true, testDDOP.add_device_property("Offset Y", (1067 * i) - 18288, static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetY), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Section1YOffset) + i));
		EXPECT_EQ(true, testDDOP.add_device_property("Width", 2 * 1067, static_cast<std::uint16_t>(DataDescriptionIndex::ActualWorkingWidth), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Section1Width) + i));
	}
	EXPECT_EQ(true, testDDOP.add_device_process_data("Actual Work State 1-16", static_cast<std::uint16_t>(DataDescriptionIndex::ActualCondensedWorkState1_16), NULL_OBJECT_ID, static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ActualCondensedWorkingState)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Setpoint Work State", static_cast<std::uint16_t>(DataDescriptionIndex::SetpointCondensedWorkState1_16), NULL_OBJECT_ID, static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable) | static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SetpointCondensedWorkingState)));

	// Set up bin/tank
	EXPECT_EQ(true, testDDOP.add_device_element("Product", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LiquidProduct), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom), task_controller_object::DeviceElementObject::Type::Bin, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LiquidProduct)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Tank Capacity", static_cast<std::uint16_t>(DataDescriptionIndex::MaximumVolumeContent), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::VolumePresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TankCapacity)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Tank Volume", static_cast<std::uint16_t>(DataDescriptionIndex::ActualVolumeContent), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::VolumePresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet) | static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::TimeInterval), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TankVolume)));

	// Set up presentations
	EXPECT_EQ(true, testDDOP.add_device_value_presentation("mm", 0, 1.0f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation)));
	EXPECT_EQ(true, testDDOP.add_device_value_presentation("m", 0, 0.001f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation)));
	EXPECT_EQ(true, testDDOP.add_device_value_presentation("m^2", 0, 1.0f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::AreaPresentation)));
	EXPECT_EQ(true, testDDOP.add_device_value_presentation("L", 0, 0.001f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::VolumePresentation)));

	std::vector<std::uint8_t> binaryDDOP;

	EXPECT_EQ(true, testDDOP.generate_binary_object_pool(binaryDDOP));

	// Now attempt to reverse the DDOP we just created back into it's objects.
	testDDOP.clear();
	EXPECT_EQ(0, testDDOP.size());

	EXPECT_EQ(true, testDDOP.deserialize_binary_object_pool(binaryDDOP, NAME(0)));

	// Test some objects match the expected pool
	auto tempObject = testDDOP.get_object_by_id(0);
	ASSERT_NE(nullptr, tempObject);
	ASSERT_EQ(task_controller_object::ObjectTypes::Device, tempObject->get_object_type());
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceObject>(tempObject)->get_object_id(), 0);
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceObject>(tempObject)->get_designator(), "AgIsoStack++ UnitTest");
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceObject>(tempObject)->get_extended_structure_label().size(), 0);
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceObject>(tempObject)->get_iso_name(), 0);
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceObject>(tempObject)->get_serial_number(), "123");
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceObject>(tempObject)->get_structure_label(), "I++1.0 ");
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceObject>(tempObject)->get_localization_label(), testLanguageInterface.get_localization_raw_data());

	tempObject = testDDOP.get_object_by_id(1);
	ASSERT_NE(nullptr, tempObject);
	ASSERT_EQ(task_controller_object::ObjectTypes::DeviceElement, tempObject->get_object_type());
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceElementObject>(tempObject)->get_object_id(), 1);
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceElementObject>(tempObject)->get_designator(), "Sprayer");
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceElementObject>(tempObject)->get_element_number(), 1);
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceElementObject>(tempObject)->get_number_child_objects(), 0);
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceElementObject>(tempObject)->get_parent_object(), 0);

	tempObject = testDDOP.get_object_by_id(4);
	ASSERT_NE(nullptr, tempObject);
	ASSERT_EQ(task_controller_object::ObjectTypes::DeviceElement, tempObject->get_object_type());
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceElementObject>(tempObject)->get_object_id(), 4);
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceElementObject>(tempObject)->get_designator(), "Connector");
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceElementObject>(tempObject)->get_element_number(), 4);
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceElementObject>(tempObject)->get_number_child_objects(), 0);
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceElementObject>(tempObject)->get_parent_object(), 1);

	tempObject = testDDOP.get_object_by_id(14);
	ASSERT_NE(nullptr, tempObject);
	ASSERT_EQ(task_controller_object::ObjectTypes::DeviceProperty, tempObject->get_object_type());
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DevicePropertyObject>(tempObject)->get_object_id(), 14);
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DevicePropertyObject>(tempObject)->get_designator(), "Offset X");
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DevicePropertyObject>(tempObject)->get_ddi(), 134);
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DevicePropertyObject>(tempObject)->get_device_value_presentation_object_id(), 88);

	tempObject = testDDOP.get_object_by_id(15);
	ASSERT_NE(nullptr, tempObject);
	ASSERT_EQ(task_controller_object::ObjectTypes::DeviceProperty, tempObject->get_object_type());
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DevicePropertyObject>(tempObject)->get_object_id(), 15);
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DevicePropertyObject>(tempObject)->get_designator(), "Offset Y");
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DevicePropertyObject>(tempObject)->get_ddi(), 135);
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DevicePropertyObject>(tempObject)->get_device_value_presentation_object_id(), 88);

	tempObject = testDDOP.get_object_by_id(90);
	ASSERT_NE(nullptr, tempObject);
	ASSERT_EQ(task_controller_object::ObjectTypes::DeviceValuePresentation, tempObject->get_object_type());
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceValuePresentationObject>(tempObject)->get_designator(), "L");
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceValuePresentationObject>(tempObject)->get_number_of_decimals(), 0);
	EXPECT_NEAR(std::dynamic_pointer_cast<task_controller_object::DeviceValuePresentationObject>(tempObject)->get_scale(), 0.001, 0.001);

	tempObject = testDDOP.get_object_by_id(85);
	ASSERT_NE(nullptr, tempObject);
	ASSERT_EQ(task_controller_object::ObjectTypes::DeviceProcessData, tempObject->get_object_type());
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceProcessDataObject>(tempObject)->get_designator(), "Tank Volume");
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceProcessDataObject>(tempObject)->get_ddi(), 72);
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceProcessDataObject>(tempObject)->get_trigger_methods_bitfield(), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::TimeInterval));
	EXPECT_EQ(std::dynamic_pointer_cast<task_controller_object::DeviceProcessDataObject>(tempObject)->get_properties_bitfield(), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet) | static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable));
}

TEST(DDOP_TESTS, DDOPDetectDuplicateID)
{
	DeviceDescriptorObjectPool testDDOP;
	LanguageCommandInterface testLanguageInterface(nullptr, nullptr);

	EXPECT_EQ(true, testDDOP.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(false, testDDOP.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));

	EXPECT_EQ(true, testDDOP.add_device_value_presentation("m", 0, 0.001f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation)));
	EXPECT_EQ(false, testDDOP.add_device_value_presentation("m", 0, 0.001f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation)));

	EXPECT_EQ(true, testDDOP.add_device_element("Product", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LiquidProduct), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom), task_controller_object::DeviceElementObject::Type::Bin, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LiquidProduct)));
	EXPECT_EQ(false, testDDOP.add_device_element("Product", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LiquidProduct), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom), task_controller_object::DeviceElementObject::Type::Bin, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LiquidProduct)));

	EXPECT_EQ(true, testDDOP.add_device_process_data("Tank Capacity", static_cast<std::uint16_t>(DataDescriptionIndex::MaximumVolumeContent), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::VolumePresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TankCapacity)));
	EXPECT_EQ(false, testDDOP.add_device_process_data("Tank Capacity", static_cast<std::uint16_t>(DataDescriptionIndex::MaximumVolumeContent), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::VolumePresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TankCapacity)));
}

TEST(DDOP_TESTS, TestRemovingObjectsByID)
{
	DeviceDescriptorObjectPool testDDOP;
	LanguageCommandInterface testLanguageInterface(nullptr, nullptr);

	EXPECT_TRUE(testDDOP.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_TRUE(testDDOP.add_device_value_presentation("m", 0, 0.001f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation)));
	EXPECT_TRUE(testDDOP.add_device_element("Product", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LiquidProduct), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom), task_controller_object::DeviceElementObject::Type::Bin, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LiquidProduct)));
	EXPECT_TRUE(testDDOP.add_device_process_data("Tank Capacity", static_cast<std::uint16_t>(DataDescriptionIndex::MaximumVolumeContent), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::VolumePresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TankCapacity)));

	// Try removing in reverse order
	EXPECT_TRUE(testDDOP.remove_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TankCapacity)));
	EXPECT_TRUE(testDDOP.remove_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LiquidProduct)));
	EXPECT_TRUE(testDDOP.remove_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation)));
	EXPECT_TRUE(testDDOP.remove_object_by_id(0));
}

TEST(DDOP_TESTS, DeviceTests)
{
	DeviceDescriptorObjectPool testDDOPVersion3(3);
	DeviceDescriptorObjectPool testDDOPVersion4(4);
	DeviceDescriptorObjectPool testDDOPVersion4_2(4);
	LanguageCommandInterface testLanguageInterface(nullptr, nullptr);

	std::vector<std::uint8_t> veryLongExtendedStructureLabel;

	for (std::uint_fast8_t i = 0; i < 200; i++)
	{
		veryLongExtendedStructureLabel.push_back(rand());
	}

	EXPECT_EQ(true, testDDOPVersion3.add_device("This is a very long designator that should get truncated", "1.0.0", "123456789123456789456134987945698745631", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));

	// Test that the Device Designator was truncated to 32
	auto tempPD = testDDOPVersion3.get_object_by_id(0);
	ASSERT_NE(nullptr, tempPD);
	EXPECT_EQ(32, tempPD->get_designator().size());

	// Test that the serial number was truncated to 32
	EXPECT_EQ(std::static_pointer_cast<task_controller_object::DeviceObject>(tempPD)->get_serial_number().size(), 32);

	// Test object type
	EXPECT_EQ(tempPD->get_object_type(), task_controller_object::ObjectTypes::Device);

	// Test extended label is ignored on v3
	EXPECT_EQ(false, std::static_pointer_cast<task_controller_object::DeviceObject>(tempPD)->get_use_extended_structure_label());

	EXPECT_EQ(true, testDDOPVersion4.add_device("This is an even longer designator that should get truncated ideally to 128 characters in length but in reality not very many TCs will support this kind of long designator", "1.0.0", "198sdbfaysdfafg987egrn9a87werhiyuawn23", "I++1.0", testLanguageInterface.get_localization_raw_data(), veryLongExtendedStructureLabel, 0));

	// Test that the Device Designator was truncated to 128
	tempPD = testDDOPVersion4.get_object_by_id(0);
	ASSERT_NE(nullptr, tempPD);
	EXPECT_EQ(128, tempPD->get_designator().size());

	// Test the serial number that is longer than 32 bytes is working
	EXPECT_EQ(std::static_pointer_cast<task_controller_object::DeviceObject>(tempPD)->get_serial_number().size(), 38);

	// Test structure label is truncated and not empty
	EXPECT_EQ(std::static_pointer_cast<task_controller_object::DeviceObject>(tempPD)->get_extended_structure_label().size(), 32);

	// Add an extended structure label to this one
	std::vector<std::uint8_t> testExtendedLabel = { 'T', 'E', 'S', 'T' };

	EXPECT_EQ(true, testDDOPVersion4_2.add_device("This is a long designator that is larger than 32 but smaller than 128, which should warn the user but be tolerated", "1.0.0", "1211111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111113", "I++1.0", testLanguageInterface.get_localization_raw_data(), testExtendedLabel, 0));

	// Test that the Device Designator allowed
	tempPD = testDDOPVersion4_2.get_object_by_id(0);
	ASSERT_NE(nullptr, tempPD);
	EXPECT_EQ(114, tempPD->get_designator().size());

	// Test serial is truncated to 128
	EXPECT_EQ(std::static_pointer_cast<task_controller_object::DeviceObject>(tempPD)->get_serial_number().size(), 128);

	// Adding another device should fail
	EXPECT_NE(true, testDDOPVersion4_2.add_device("This is a long designator that is larger than 32 but smaller than 128, which should warn the user but be tolerated", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));

	EXPECT_EQ(tempPD->get_table_id(), "DVC");

	// Check extended structure label used in version 4
	EXPECT_EQ(testDDOPVersion4_2.get_task_controller_compatibility_level(), 4);
	EXPECT_EQ(true, std::static_pointer_cast<task_controller_object::DeviceObject>(tempPD)->get_use_extended_structure_label());

	// Try to lower the compatibility level to 3
	testDDOPVersion4_2.set_task_controller_compatibility_level(3);
	EXPECT_EQ(testDDOPVersion4_2.get_task_controller_compatibility_level(), 3);
	EXPECT_EQ(false, std::static_pointer_cast<task_controller_object::DeviceObject>(tempPD)->get_use_extended_structure_label());

	EXPECT_EQ(testDDOPVersion4_2.get_max_supported_task_controller_version(), 4);

	// Test Setters
	auto objectUnderTest = std::static_pointer_cast<task_controller_object::DeviceObject>(tempPD);
	objectUnderTest->set_designator("Test");
	EXPECT_EQ("Test", objectUnderTest->get_designator());
	objectUnderTest->set_iso_name(1234567);
	EXPECT_EQ(1234567, objectUnderTest->get_iso_name());
	objectUnderTest->set_serial_number("9999");
	EXPECT_EQ("9999", objectUnderTest->get_serial_number());
	objectUnderTest->set_software_version("5555");
	EXPECT_EQ("5555", objectUnderTest->get_software_version());

	std::vector<std::uint8_t> testESL = { 1, 2, 3, 4, 5, 6, 7, 87 };
	objectUnderTest->set_extended_structure_label(testESL);
	EXPECT_EQ(testESL, objectUnderTest->get_extended_structure_label());
	objectUnderTest->set_structure_label("TEST");
	EXPECT_EQ("TEST", objectUnderTest->get_structure_label());
	objectUnderTest->set_use_extended_structure_label(true);
	EXPECT_TRUE(objectUnderTest->get_use_extended_structure_label());

	std::array<std::uint8_t, 7> testLocalization = { 0, 1, 2, 3, 4, 5, 6 };
	objectUnderTest->set_localization_label(testLocalization);
	EXPECT_EQ(testLocalization, objectUnderTest->get_localization_label());
}

TEST(DDOP_TESTS, DeviceElementDesignatorTests)
{
	DeviceDescriptorObjectPool testDDOPVersion3(3);
	DeviceDescriptorObjectPool testDDOPVersion4(4);
	DeviceDescriptorObjectPool testDDOPVersion4_2(4);

	LanguageCommandInterface testLanguageInterface(nullptr, nullptr);

	EXPECT_EQ(true, testDDOPVersion3.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(true, testDDOPVersion3.add_device_element("Sprayer But like with a super long designator, just a really impractical one", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), 0, task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement)));
	EXPECT_EQ(true, testDDOPVersion4.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(true, testDDOPVersion4.add_device_element("Sprayer But like with a super long designator, just a really impractical one", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), 0, task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement)));
	EXPECT_EQ(true, testDDOPVersion4_2.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(true, testDDOPVersion4_2.add_device_element("Sprayer But like with a super long designator, just a really impractical one, it's really getting out of hand with this designator", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), 0, task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement)));

	auto tempPD = testDDOPVersion3.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement));
	ASSERT_NE(nullptr, tempPD);
	// Version 3 designator should be truncated
	EXPECT_EQ(32, tempPD->get_designator().size());

	tempPD = testDDOPVersion4.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement));
	ASSERT_NE(nullptr, tempPD);

	// Version 4 designator should be allowed
	EXPECT_EQ(76, tempPD->get_designator().size());

	tempPD = testDDOPVersion4_2.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement));
	ASSERT_NE(nullptr, tempPD);

	// Version 4 designator should truncate at 128
	EXPECT_EQ(128, tempPD->get_designator().size());

	EXPECT_EQ(tempPD->get_table_id(), "DET");

	// Now test that a parent of the device element is not null
	EXPECT_EQ(true, testDDOPVersion4_2.add_device_element("Super Junk Element", 0xFFFF, 0xFFFF, task_controller_object::DeviceElementObject::Type::Bin, 65530));
	std::vector<std::uint8_t> binaryDDOP;
	EXPECT_EQ(false, testDDOPVersion4_2.generate_binary_object_pool(binaryDDOP));

	// Test invalid parent
	EXPECT_EQ(true, testDDOPVersion4.add_device_property("asasdfasdf", 4, 5, 0xFFFF, 12347));
	EXPECT_EQ(true, testDDOPVersion4.add_device_element("asldkfy", 714, 12347, task_controller_object::DeviceElementObject::Type::Bin, 7786));
	EXPECT_EQ(false, testDDOPVersion4.generate_binary_object_pool(binaryDDOP));

	// Test missing parent
	EXPECT_EQ(true, testDDOPVersion3.add_device_property("asasdfasdf", 4, 5, 0xFFFF, 12347));
	EXPECT_EQ(true, testDDOPVersion3.add_device_element("asldkfy", 714, 8467, task_controller_object::DeviceElementObject::Type::Bin, 7786));
	EXPECT_EQ(false, testDDOPVersion3.generate_binary_object_pool(binaryDDOP));

	// Test Setters
	auto objectUnderTest = std::static_pointer_cast<task_controller_object::DeviceElementObject>(tempPD);
	objectUnderTest->set_element_number(200);
	EXPECT_EQ(200, objectUnderTest->get_element_number());
	objectUnderTest->set_object_id(3500);
	EXPECT_EQ(3500, objectUnderTest->get_object_id());
	objectUnderTest->set_parent_object(4444);
	EXPECT_EQ(4444, objectUnderTest->get_parent_object());

	objectUnderTest->add_reference_to_child_object(111);
	EXPECT_EQ(1, objectUnderTest->get_number_child_objects());
	objectUnderTest->remove_reference_to_child_object(111);
	EXPECT_EQ(0, objectUnderTest->get_number_child_objects());

	// Test that invalid child objects are rejected
	DeviceDescriptorObjectPool testDDOPWithBadChildren(3);
	EXPECT_TRUE(testDDOPWithBadChildren.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_TRUE(testDDOPWithBadChildren.add_device_element("Sprayer", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), 0, task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement)));
	EXPECT_TRUE(testDDOPWithBadChildren.add_device_element("Junk Element 1", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), 0, task_controller_object::DeviceElementObject::Type::Function, 250));
	EXPECT_TRUE(testDDOPWithBadChildren.generate_binary_object_pool(binaryDDOP));
	objectUnderTest = std::static_pointer_cast<task_controller_object::DeviceElementObject>(testDDOPWithBadChildren.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement)));
	objectUnderTest->add_reference_to_child_object(250); // Set child as a DET, which is not allowed
	EXPECT_FALSE(testDDOPWithBadChildren.generate_binary_object_pool(binaryDDOP));
}

TEST(DDOP_TESTS, ProcessDataTests)
{
	DeviceDescriptorObjectPool testDDOPVersion3(3);
	DeviceDescriptorObjectPool testDDOPVersion4(4);
	LanguageCommandInterface testLanguageInterface(nullptr, nullptr);

	EXPECT_EQ(true, testDDOPVersion3.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(true, testDDOPVersion3.add_device_element("Sprayer", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), 0, task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement)));
	EXPECT_EQ(true, testDDOPVersion3.add_device_process_data("This is a very long designator that should get truncated", static_cast<std::uint16_t>(DataDescriptionIndex::ActualWorkState), NULL_OBJECT_ID, static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceActualWorkState)));

	// Test that the PD Designator was truncated to 32
	auto tempPD = testDDOPVersion3.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceActualWorkState));
	ASSERT_NE(nullptr, tempPD);
	EXPECT_EQ(32, tempPD->get_designator().size());

	EXPECT_EQ(true, testDDOPVersion4.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(true, testDDOPVersion4.add_device_element("Sprayer", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), 0, task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement)));
	EXPECT_EQ(true, testDDOPVersion4.add_device_process_data("This is an even longer designator that should get truncated ideally to 128 characters in length but in reality not very many TCs will support this kind of long designator", static_cast<std::uint16_t>(DataDescriptionIndex::ActualWorkState), NULL_OBJECT_ID, static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceActualWorkState)));

	// Version 4+ designators can be 128 long, mostly for utf-8 support, not ascii, but testing it with chars
	tempPD = testDDOPVersion4.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceActualWorkState));
	ASSERT_NE(nullptr, tempPD);
	EXPECT_EQ(128, tempPD->get_designator().size());

	EXPECT_EQ(tempPD->get_table_id(), "DPD");

	// Test Setters
	auto objectUnderTest = std::static_pointer_cast<task_controller_object::DeviceProcessDataObject>(tempPD);
	objectUnderTest->set_ddi(45056);
	EXPECT_EQ(45056, objectUnderTest->get_ddi());
	objectUnderTest->set_device_value_presentation_object_id(25555);
	EXPECT_EQ(25555, objectUnderTest->get_device_value_presentation_object_id());
	objectUnderTest->set_object_id(3000);
	EXPECT_EQ(3000, objectUnderTest->get_object_id());
	objectUnderTest->set_properties_bitfield(0x04);
	EXPECT_EQ(0x04, objectUnderTest->get_properties_bitfield());
	objectUnderTest->set_trigger_methods_bitfield(0x08);
	EXPECT_EQ(0x08, objectUnderTest->get_trigger_methods_bitfield());
}

TEST(DDOP_TESTS, PropertyTests)
{
	DeviceDescriptorObjectPool testDDOPVersion3(3);
	DeviceDescriptorObjectPool testDDOPVersion4(4);
	DeviceDescriptorObjectPool testDDOPVersion4_2(4);
	LanguageCommandInterface testLanguageInterface(nullptr, nullptr);

	EXPECT_EQ(true, testDDOPVersion3.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(true, testDDOPVersion3.add_device_element("Sprayer", 1, 0, isobus::task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement)));
	EXPECT_EQ(true, testDDOPVersion3.add_device_process_data("Total Time", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::EffectiveTotalTime), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TimePresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::Total), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceTotalTime)));
	EXPECT_EQ(true, testDDOPVersion3.add_device_element("Connector", 2, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), isobus::task_controller_object::DeviceElementObject::Type::Connector, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Connector)));
	EXPECT_EQ(true, testDDOPVersion3.add_device_process_data("Connector X", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetX), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorXOffset)));
	EXPECT_EQ(true, testDDOPVersion3.add_device_process_data("Connector Y", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetY), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorYOffset)));
	EXPECT_EQ(true, testDDOPVersion3.add_device_property("Type123456789123456789123456789000111222333", 9, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ConnectorType), isobus::NULL_OBJECT_ID, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorType)));

	auto tempProperty = testDDOPVersion3.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorType));
	ASSERT_NE(nullptr, tempProperty);
	EXPECT_EQ(tempProperty->get_designator().size(), 32);
	EXPECT_EQ(std::static_pointer_cast<task_controller_object::DevicePropertyObject>(tempProperty)->get_ddi(), 157);
	EXPECT_EQ(std::static_pointer_cast<task_controller_object::DevicePropertyObject>(tempProperty)->get_table_id(), "DPT");
	EXPECT_EQ(std::static_pointer_cast<task_controller_object::DevicePropertyObject>(tempProperty)->get_device_value_presentation_object_id(), 65535);

	EXPECT_EQ(true, testDDOPVersion4.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(true, testDDOPVersion4.add_device_element("Sprayer", 1, 0, isobus::task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement)));
	EXPECT_EQ(true, testDDOPVersion4.add_device_process_data("Total Time", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::EffectiveTotalTime), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TimePresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::Total), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceTotalTime)));
	EXPECT_EQ(true, testDDOPVersion4.add_device_element("Connector", 2, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), isobus::task_controller_object::DeviceElementObject::Type::Connector, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Connector)));
	EXPECT_EQ(true, testDDOPVersion4.add_device_process_data("Connector X", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetX), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorXOffset)));
	EXPECT_EQ(true, testDDOPVersion4.add_device_process_data("Connector Y", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetY), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorYOffset)));
	EXPECT_EQ(true, testDDOPVersion4.add_device_property("Type123456789123456789123456789000111222333", 9, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ConnectorType), isobus::NULL_OBJECT_ID, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorType)));

	tempProperty = testDDOPVersion4.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorType));
	ASSERT_NE(nullptr, tempProperty);
	EXPECT_EQ(tempProperty->get_designator().size(), 43);

	EXPECT_EQ(true, testDDOPVersion4_2.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(true, testDDOPVersion4_2.add_device_element("Sprayer", 1, 0, isobus::task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement)));
	EXPECT_EQ(true, testDDOPVersion4_2.add_device_process_data("Total Time", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::EffectiveTotalTime), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TimePresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::Total), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceTotalTime)));
	EXPECT_EQ(true, testDDOPVersion4_2.add_device_element("Connector", 2, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), isobus::task_controller_object::DeviceElementObject::Type::Connector, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Connector)));
	EXPECT_EQ(true, testDDOPVersion4_2.add_device_process_data("Connector X", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetX), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorXOffset)));
	EXPECT_EQ(true, testDDOPVersion4_2.add_device_process_data("Connector Y", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetY), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorYOffset)));
	EXPECT_EQ(true, testDDOPVersion4_2.add_device_property("Type123456789123456789123456789000111222333aksjdhflkajhdfasdfasdfasdfasdfasdfasdfiouhsidlfhalksjdhlkajshdflkasdfhlhasdfhalksjdflkasjhflkjashdfl", 9, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ConnectorType), isobus::NULL_OBJECT_ID, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorType)));

	tempProperty = testDDOPVersion4_2.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorType));
	ASSERT_NE(nullptr, tempProperty);
	EXPECT_EQ(tempProperty->get_designator().size(), 128);

	// Test Setters
	auto objectUnderTest = std::static_pointer_cast<task_controller_object::DevicePropertyObject>(tempProperty);
	objectUnderTest->set_ddi(688);
	EXPECT_EQ(688, objectUnderTest->get_ddi());
	objectUnderTest->set_device_value_presentation_object_id(745);
	EXPECT_EQ(745, objectUnderTest->get_device_value_presentation_object_id());
	objectUnderTest->set_object_id(800);
	EXPECT_EQ(800, objectUnderTest->get_object_id());
	objectUnderTest->set_value(4000);
	EXPECT_EQ(4000, objectUnderTest->get_value());
}

TEST(DDOP_TESTS, PresentationTests)
{
	DeviceDescriptorObjectPool testDDOPVersion3(3);
	DeviceDescriptorObjectPool testDDOPVersion4(4);
	LanguageCommandInterface testLanguageInterface(nullptr, nullptr);

	EXPECT_EQ(true, testDDOPVersion3.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(true, testDDOPVersion3.add_device_value_presentation("mm", 0, 1.0f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation)));
	EXPECT_EQ(true, testDDOPVersion3.add_device_value_presentation("mm but like with an abnormally long designator to test if we handle it correctly", 0, 1.0f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation)));

	EXPECT_EQ(true, testDDOPVersion4.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(true, testDDOPVersion4.add_device_value_presentation("mm but basically an outragious designator that makes no sense and should never be used. Ideally his is always 32 chars or less, but using a long string to test byte max.", 0, 1.0f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation)));
	EXPECT_EQ(true, testDDOPVersion4.add_device_value_presentation("mm but like with an abnormally long designator to test if we handle it correctly", 0, 1.0f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation)));

	auto tempPresentation = testDDOPVersion3.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation));
	ASSERT_NE(nullptr, tempPresentation);
	EXPECT_EQ(tempPresentation->get_designator(), "mm");
	EXPECT_EQ(tempPresentation->get_table_id(), "DVP");

	tempPresentation = testDDOPVersion3.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation));
	ASSERT_NE(nullptr, tempPresentation);
	EXPECT_EQ(tempPresentation->get_designator().size(), 32);

	tempPresentation = testDDOPVersion4.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation));
	ASSERT_NE(nullptr, tempPresentation);
	EXPECT_EQ(tempPresentation->get_designator().size(), 128);

	tempPresentation = testDDOPVersion4.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation));
	ASSERT_NE(nullptr, tempPresentation);
	EXPECT_EQ(tempPresentation->get_designator().size(), 80);

	// Test Setters
	auto objectUnderTest = std::static_pointer_cast<task_controller_object::DeviceValuePresentationObject>(tempPresentation);
	objectUnderTest->set_number_of_decimals(3);
	EXPECT_EQ(3, objectUnderTest->get_number_of_decimals());
	objectUnderTest->set_object_id(400);
	EXPECT_EQ(400, objectUnderTest->get_object_id());
	objectUnderTest->set_offset(50000);
	EXPECT_EQ(50000, objectUnderTest->get_offset());
	objectUnderTest->set_scale(10.0f);
	EXPECT_NEAR(10.0f, objectUnderTest->get_scale(), 0.001f);
}

TEST(DDOP_TESTS, ISOXMLOutput)
{
	DeviceDescriptorObjectPool testDDOPVersion3(3);
	constexpr std::uint8_t testObjectPool[] = { 0x44, 0x56, 0x43, 0x00, 0x00, 0x15, 0x41, 0x67, 0x49, 0x73, 0x6f, 0x53, 0x74, 0x61, 0x63, 0x6b, 0x2b, 0x2b, 0x20, 0x55, 0x6e, 0x69, 0x74, 0x54, 0x65, 0x73, 0x74, 0x05, 0x31, 0x2e, 0x30, 0x2e, 0x30, 0x02, 0x00, 0xe0, 0xaf, 0x00, 0x80, 0x0c, 0xa0, 0x03, 0x31, 0x32, 0x33, 0x41, 0x2b, 0x2b, 0x31, 0x2e, 0x30, 0x20, 0x65, 0x6e, 0x50, 0x00, 0x55, 0x55, 0xff, 0x44, 0x45, 0x54, 0x01, 0x00, 0x01, 0x07, 0x53, 0x70, 0x72, 0x61, 0x79, 0x65, 0x72, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x04, 0x00, 0x44, 0x50, 0x44, 0x02, 0x00, 0x8d, 0x00, 0x01, 0x08, 0x11, 0x41, 0x63, 0x74, 0x75, 0x61, 0x6c, 0x20, 0x57, 0x6f, 0x72, 0x6b, 0x20, 0x53, 0x74, 0x61, 0x74, 0x65, 0xff, 0xff, 0x44, 0x50, 0x44, 0x03, 0x00, 0x03, 0x00, 0x00, 0x10, 0x12, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x20, 0x44, 0x65, 0x66, 0x61, 0x75, 0x6c, 0x74, 0x20, 0x50, 0x44, 0xff, 0xff, 0x44, 0x50, 0x44, 0x04, 0x00, 0x77, 0x00, 0x03, 0x10, 0x0a, 0x54, 0x6f, 0x74, 0x61, 0x6c, 0x20, 0x54, 0x69, 0x6d, 0x65, 0x3b, 0x04, 0x44, 0x45, 0x54, 0x05, 0x00, 0x06, 0x09, 0x43, 0x6f, 0x6e, 0x6e, 0x65, 0x63, 0x74, 0x6f, 0x72, 0x01, 0x00, 0x01, 0x00, 0x03, 0x00, 0x06, 0x00, 0x07, 0x00, 0x08, 0x00, 0x44, 0x50, 0x44, 0x06, 0x00, 0x86, 0x00, 0x02, 0x00, 0x0b, 0x43, 0x6f, 0x6e, 0x6e, 0x65, 0x63, 0x74, 0x6f, 0x72, 0x20, 0x58, 0x3c, 0x04, 0x44, 0x50, 0x44, 0x07, 0x00, 0x87, 0x00, 0x02, 0x00, 0x0b, 0x43, 0x6f, 0x6e, 0x6e, 0x65, 0x63, 0x74, 0x6f, 0x72, 0x20, 0x59, 0x3c, 0x04, 0x44, 0x50, 0x54, 0x08, 0x00, 0x9d, 0x00, 0x09, 0x00, 0x00, 0x00, 0x04, 0x54, 0x79, 0x70, 0x65, 0xff, 0xff, 0x44, 0x45, 0x54, 0x09, 0x00, 0x02, 0x04, 0x42, 0x6f, 0x6f, 0x6d, 0x02, 0x00, 0x01, 0x00, 0x07, 0x00, 0x0f, 0x00, 0x10, 0x00, 0x11, 0x00, 0x0b, 0x00, 0x0e, 0x00, 0x12, 0x04, 0x22, 0x04, 0x44, 0x50, 0x54, 0x0f, 0x00, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3c, 0x04, 0x44, 0x50, 0x54, 0x10, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3c, 0x04, 0x44, 0x50, 0x54, 0x11, 0x00, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x5a, 0x3c, 0x04, 0x44, 0x50, 0x44, 0x0b, 0x00, 0x43, 0x00, 0x01, 0x08, 0x14, 0x41, 0x63, 0x74, 0x75, 0x61, 0x6c, 0x20, 0x57, 0x6f, 0x72, 0x6b, 0x69, 0x6e, 0x67, 0x20, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x50, 0x44, 0x0d, 0x00, 0x21, 0x01, 0x03, 0x08, 0x13, 0x53, 0x65, 0x74, 0x70, 0x6f, 0x69, 0x6e, 0x74, 0x20, 0x57, 0x6f, 0x72, 0x6b, 0x20, 0x53, 0x74, 0x61, 0x74, 0x65, 0xff, 0xff, 0x44, 0x50, 0x44, 0x0c, 0x00, 0x74, 0x00, 0x01, 0x10, 0x0a, 0x41, 0x72, 0x65, 0x61, 0x20, 0x54, 0x6f, 0x74, 0x61, 0x6c, 0x3a, 0x04, 0x44, 0x50, 0x44, 0x0e, 0x00, 0xa0, 0x00, 0x03, 0x09, 0x15, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x43, 0x6f, 0x6e, 0x74, 0x72, 0x6f, 0x6c, 0x20, 0x53, 0x74, 0x61, 0x74, 0x65, 0xff, 0xff, 0x44, 0x45, 0x54, 0x32, 0x04, 0x03, 0x07, 0x50, 0x72, 0x6f, 0x64, 0x75, 0x63, 0x74, 0x03, 0x00, 0x09, 0x00, 0x07, 0x00, 0x33, 0x04, 0x34, 0x04, 0x35, 0x04, 0x36, 0x04, 0x37, 0x04, 0x38, 0x04, 0x39, 0x04, 0x44, 0x50, 0x44, 0x33, 0x04, 0x49, 0x00, 0x01, 0x09, 0x0d, 0x54, 0x61, 0x6e, 0x6b, 0x20, 0x43, 0x61, 0x70, 0x61, 0x63, 0x69, 0x74, 0x79, 0x3e, 0x04, 0x44, 0x50, 0x44, 0x34, 0x04, 0x48, 0x00, 0x03, 0x09, 0x0b, 0x54, 0x61, 0x6e, 0x6b, 0x20, 0x56, 0x6f, 0x6c, 0x75, 0x6d, 0x65, 0x3e, 0x04, 0x44, 0x50, 0x44, 0x35, 0x04, 0x45, 0x01, 0x01, 0x10, 0x15, 0x4c, 0x69, 0x66, 0x65, 0x74, 0x69, 0x6d, 0x65, 0x20, 0x54, 0x6f, 0x74, 0x61, 0x6c, 0x20, 0x56, 0x6f, 0x6c, 0x75, 0x6d, 0x65, 0x3e, 0x04, 0x44, 0x50, 0x44, 0x36, 0x04, 0x9e, 0x00, 0x03, 0x09, 0x10, 0x52, 0x78, 0x20, 0x43, 0x6f, 0x6e, 0x74, 0x72, 0x6f, 0x6c, 0x20, 0x53, 0x74, 0x61, 0x74, 0x65, 0xff, 0xff, 0x44, 0x50, 0x44, 0x38, 0x04, 0x01, 0x00, 0x03, 0x08, 0x0b, 0x54, 0x61, 0x72, 0x67, 0x65, 0x74, 0x20, 0x52, 0x61, 0x74, 0x65, 0x3f, 0x04, 0x44, 0x50, 0x44, 0x39, 0x04, 0x02, 0x00, 0x01, 0x09, 0x0b, 0x41, 0x63, 0x74, 0x75, 0x61, 0x6c, 0x20, 0x52, 0x61, 0x74, 0x65, 0x3f, 0x04, 0x44, 0x50, 0x54, 0x37, 0x04, 0xb3, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0e, 0x4f, 0x70, 0x65, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x54, 0x79, 0x70, 0x65, 0xff, 0xff, 0x44, 0x45, 0x54, 0x12, 0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x30, 0x04, 0x00, 0x09, 0x00, 0x03, 0x00, 0x12, 0x02, 0x12, 0x01, 0x12, 0x03, 0x44, 0x50, 0x54, 0x12, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x12, 0x02, 0x87, 0x00, 0x07, 0xbd, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x12, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x13, 0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x31, 0x05, 0x00, 0x09, 0x00, 0x03, 0x00, 0x13, 0x02, 0x13, 0x01, 0x13, 0x03, 0x44, 0x50, 0x54, 0x13, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x13, 0x02, 0x87, 0x00, 0xf5, 0xc5, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x13, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x14, 0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x32, 0x06, 0x00, 0x09, 0x00, 0x03, 0x00, 0x14, 0x02, 0x14, 0x01, 0x14, 0x03, 0x44, 0x50, 0x54, 0x14, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x14, 0x02, 0x87, 0x00, 0xe3, 0xce, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x14, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x15, 0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x33, 0x07, 0x00, 0x09, 0x00, 0x03, 0x00, 0x15, 0x02, 0x15, 0x01, 0x15, 0x03, 0x44, 0x50, 0x54, 0x15, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x15, 0x02, 0x87, 0x00, 0xd1, 0xd7, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x15, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x16, 0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x34, 0x08, 0x00, 0x09, 0x00, 0x03, 0x00, 0x16, 0x02, 0x16, 0x01, 0x16, 0x03, 0x44, 0x50, 0x54, 0x16, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x16, 0x02, 0x87, 0x00, 0xbf, 0xe0, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x16, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x17, 0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x35, 0x09, 0x00, 0x09, 0x00, 0x03, 0x00, 0x17, 0x02, 0x17, 0x01, 0x17, 0x03, 0x44, 0x50, 0x54, 0x17, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x17, 0x02, 0x87, 0x00, 0xad, 0xe9, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x17, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x18, 0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x36, 0x0a, 0x00, 0x09, 0x00, 0x03, 0x00, 0x18, 0x02, 0x18, 0x01, 0x18, 0x03, 0x44, 0x50, 0x54, 0x18, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x18, 0x02, 0x87, 0x00, 0x9b, 0xf2, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x18, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x19, 0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x37, 0x0b, 0x00, 0x09, 0x00, 0x03, 0x00, 0x19, 0x02, 0x19, 0x01, 0x19, 0x03, 0x44, 0x50, 0x54, 0x19, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x19, 0x02, 0x87, 0x00, 0x89, 0xfb, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x19, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x1a, 0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x38, 0x0c, 0x00, 0x09, 0x00, 0x03, 0x00, 0x1a, 0x02, 0x1a, 0x01, 0x1a, 0x03, 0x44, 0x50, 0x54, 0x1a, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1a, 0x02, 0x87, 0x00, 0x77, 0x04, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1a, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x1b, 0x00, 0x04, 0x09, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x39, 0x0d, 0x00, 0x09, 0x00, 0x03, 0x00, 0x1b, 0x02, 0x1b, 0x01, 0x1b, 0x03, 0x44, 0x50, 0x54, 0x1b, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1b, 0x02, 0x87, 0x00, 0x65, 0x0d, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1b, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x1c, 0x00, 0x04, 0x0a, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x31, 0x30, 0x0e, 0x00, 0x09, 0x00, 0x03, 0x00, 0x1c, 0x02, 0x1c, 0x01, 0x1c, 0x03, 0x44, 0x50, 0x54, 0x1c, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1c, 0x02, 0x87, 0x00, 0x53, 0x16, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1c, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x1d, 0x00, 0x04, 0x0a, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x31, 0x31, 0x0f, 0x00, 0x09, 0x00, 0x03, 0x00, 0x1d, 0x02, 0x1d, 0x01, 0x1d, 0x03, 0x44, 0x50, 0x54, 0x1d, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1d, 0x02, 0x87, 0x00, 0x41, 0x1f, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1d, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x1e, 0x00, 0x04, 0x0a, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x31, 0x32, 0x10, 0x00, 0x09, 0x00, 0x03, 0x00, 0x1e, 0x02, 0x1e, 0x01, 0x1e, 0x03, 0x44, 0x50, 0x54, 0x1e, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1e, 0x02, 0x87, 0x00, 0x2f, 0x28, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1e, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x1f, 0x00, 0x04, 0x0a, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x31, 0x33, 0x11, 0x00, 0x09, 0x00, 0x03, 0x00, 0x1f, 0x02, 0x1f, 0x01, 0x1f, 0x03, 0x44, 0x50, 0x54, 0x1f, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1f, 0x02, 0x87, 0x00, 0x1d, 0x31, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x1f, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x20, 0x00, 0x04, 0x0a, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x31, 0x34, 0x12, 0x00, 0x09, 0x00, 0x03, 0x00, 0x20, 0x02, 0x20, 0x01, 0x20, 0x03, 0x44, 0x50, 0x54, 0x20, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x20, 0x02, 0x87, 0x00, 0x0b, 0x3a, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x20, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x45, 0x54, 0x21, 0x00, 0x04, 0x0a, 0x53, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x31, 0x35, 0x13, 0x00, 0x09, 0x00, 0x03, 0x00, 0x21, 0x02, 0x21, 0x01, 0x21, 0x03, 0x44, 0x50, 0x54, 0x21, 0x01, 0x86, 0x00, 0xec, 0xff, 0xff, 0xff, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x58, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x21, 0x02, 0x87, 0x00, 0xf9, 0x42, 0x00, 0x00, 0x08, 0x4f, 0x66, 0x66, 0x73, 0x65, 0x74, 0x20, 0x59, 0x3d, 0x04, 0x44, 0x50, 0x54, 0x21, 0x03, 0x43, 0x00, 0xee, 0x08, 0x00, 0x00, 0x05, 0x57, 0x69, 0x64, 0x74, 0x68, 0x3d, 0x04, 0x44, 0x50, 0x44, 0x12, 0x04, 0xa1, 0x00, 0x01, 0x08, 0x16, 0x41, 0x63, 0x74, 0x75, 0x61, 0x6c, 0x20, 0x57, 0x6f, 0x72, 0x6b, 0x20, 0x53, 0x74, 0x61, 0x74, 0x65, 0x20, 0x31, 0x2d, 0x31, 0x36, 0xff, 0xff, 0x44, 0x50, 0x44, 0x22, 0x04, 0x22, 0x01, 0x03, 0x08, 0x18, 0x53, 0x65, 0x74, 0x70, 0x6f, 0x69, 0x6e, 0x74, 0x20, 0x57, 0x6f, 0x72, 0x6b, 0x20, 0x53, 0x74, 0x61, 0x74, 0x65, 0x20, 0x31, 0x2d, 0x31, 0x36, 0xff, 0xff, 0x44, 0x56, 0x50, 0x3c, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x02, 0x6d, 0x6d, 0x44, 0x56, 0x50, 0x3d, 0x04, 0x00, 0x00, 0x00, 0x00, 0x6f, 0x12, 0x83, 0x3a, 0x00, 0x01, 0x6d, 0x44, 0x56, 0x50, 0x3a, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x03, 0x6d, 0x5e, 0x32, 0x44, 0x56, 0x50, 0x3e, 0x04, 0x00, 0x00, 0x00, 0x00, 0x6f, 0x12, 0x83, 0x3a, 0x00, 0x01, 0x4c, 0x44, 0x56, 0x50, 0x3b, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x01, 0x07, 0x6d, 0x69, 0x6e, 0x75, 0x74, 0x65, 0x73, 0x44, 0x56, 0x50, 0x3f, 0x04, 0x00, 0x00, 0x00, 0x00, 0x6f, 0x12, 0x83, 0x3a, 0x01, 0x04, 0x4c, 0x2f, 0x68, 0x61 };
	std::string isoxml;

	ASSERT_TRUE(testDDOPVersion3.deserialize_binary_object_pool(testObjectPool, sizeof(testObjectPool)));
	ASSERT_TRUE(testDDOPVersion3.generate_task_data_iso_xml(isoxml));
	ASSERT_FALSE(isoxml.empty());

	// Validate the ISOXML against a known good file
	const std::string textXML = R"ISOXML(<?xml version="1.0" encoding="UTF-8"?>
<ISO11783_TaskData VersionMajor="3" VersionMinor="0" DataTransferOrigin="1">
<DVC A="DVC-1" B="AgIsoStack++ UnitTest" C="1.0.0" D="A00C8000AFE00002" E="123" F="20302E312B2B41" G="FF555500506E65">
	<DET A="DET-1" B="1" C="1" D="Sprayer" E="0" F="0">
		<DOR A="2"/>
		<DOR A="4"/>
	</DET>
	<DET A="DET-2" B="5" C="6" D="Connector" E="1" F="1">
		<DOR A="6"/>
		<DOR A="7"/>
		<DOR A="8"/>
	</DET>
	<DET A="DET-3" B="9" C="2" D="Boom" E="2" F="1">
		<DOR A="15"/>
		<DOR A="16"/>
		<DOR A="17"/>
		<DOR A="11"/>
		<DOR A="14"/>
		<DOR A="1042"/>
		<DOR A="1058"/>
	</DET>
	<DET A="DET-4" B="1074" C="3" D="Product" E="3" F="9">
		<DOR A="1075"/>
		<DOR A="1076"/>
		<DOR A="1077"/>
		<DOR A="1078"/>
		<DOR A="1079"/>
		<DOR A="1080"/>
		<DOR A="1081"/>
	</DET>
	<DET A="DET-5" B="18" C="4" D="Section 0" E="4" F="9">
		<DOR A="530"/>
		<DOR A="274"/>
		<DOR A="786"/>
	</DET>
	<DET A="DET-6" B="19" C="4" D="Section 1" E="5" F="9">
		<DOR A="531"/>
		<DOR A="275"/>
		<DOR A="787"/>
	</DET>
	<DET A="DET-7" B="20" C="4" D="Section 2" E="6" F="9">
		<DOR A="532"/>
		<DOR A="276"/>
		<DOR A="788"/>
	</DET>
	<DET A="DET-8" B="21" C="4" D="Section 3" E="7" F="9">
		<DOR A="533"/>
		<DOR A="277"/>
		<DOR A="789"/>
	</DET>
	<DET A="DET-9" B="22" C="4" D="Section 4" E="8" F="9">
		<DOR A="534"/>
		<DOR A="278"/>
		<DOR A="790"/>
	</DET>
	<DET A="DET-10" B="23" C="4" D="Section 5" E="9" F="9">
		<DOR A="535"/>
		<DOR A="279"/>
		<DOR A="791"/>
	</DET>
	<DET A="DET-11" B="24" C="4" D="Section 6" E="10" F="9">
		<DOR A="536"/>
		<DOR A="280"/>
		<DOR A="792"/>
	</DET>
	<DET A="DET-12" B="25" C="4" D="Section 7" E="11" F="9">
		<DOR A="537"/>
		<DOR A="281"/>
		<DOR A="793"/>
	</DET>
	<DET A="DET-13" B="26" C="4" D="Section 8" E="12" F="9">
		<DOR A="538"/>
		<DOR A="282"/>
		<DOR A="794"/>
	</DET>
	<DET A="DET-14" B="27" C="4" D="Section 9" E="13" F="9">
		<DOR A="539"/>
		<DOR A="283"/>
		<DOR A="795"/>
	</DET>
	<DET A="DET-15" B="28" C="4" D="Section 10" E="14" F="9">
		<DOR A="540"/>
		<DOR A="284"/>
		<DOR A="796"/>
	</DET>
	<DET A="DET-16" B="29" C="4" D="Section 11" E="15" F="9">
		<DOR A="541"/>
		<DOR A="285"/>
		<DOR A="797"/>
	</DET>
	<DET A="DET-17" B="30" C="4" D="Section 12" E="16" F="9">
		<DOR A="542"/>
		<DOR A="286"/>
		<DOR A="798"/>
	</DET>
	<DET A="DET-18" B="31" C="4" D="Section 13" E="17" F="9">
		<DOR A="543"/>
		<DOR A="287"/>
		<DOR A="799"/>
	</DET>
	<DET A="DET-19" B="32" C="4" D="Section 14" E="18" F="9">
		<DOR A="544"/>
		<DOR A="288"/>
		<DOR A="800"/>
	</DET>
	<DET A="DET-20" B="33" C="4" D="Section 15" E="19" F="9">
		<DOR A="545"/>
		<DOR A="289"/>
		<DOR A="801"/>
	</DET>
	<DPD A="2" B="008D" C="1" D="8" E="Actual Work State"/>
	<DPD A="3" B="0003" C="0" D="16" E="Request Default PD"/>
	<DPD A="4" B="0077" C="3" D="16" E="Total Time" F="1083"/>
	<DPD A="6" B="0086" C="2" D="0" E="Connector X" F="1084"/>
	<DPD A="7" B="0087" C="2" D="0" E="Connector Y" F="1084"/>
	<DPD A="11" B="0043" C="1" D="8" E="Actual Working Width" F="1085"/>
	<DPD A="13" B="0121" C="3" D="8" E="Setpoint Work State"/>
	<DPD A="12" B="0074" C="1" D="16" E="Area Total" F="1082"/>
	<DPD A="14" B="00A0" C="3" D="9" E="Section Control State"/>
	<DPD A="1075" B="0049" C="1" D="9" E="Tank Capacity" F="1086"/>
	<DPD A="1076" B="0048" C="3" D="9" E="Tank Volume" F="1086"/>
	<DPD A="1077" B="0145" C="1" D="16" E="Lifetime Total Volume" F="1086"/>
	<DPD A="1078" B="009E" C="3" D="9" E="Rx Control State"/>
	<DPD A="1080" B="0001" C="3" D="8" E="Target Rate" F="1087"/>
	<DPD A="1081" B="0002" C="1" D="9" E="Actual Rate" F="1087"/>
	<DPD A="1042" B="00A1" C="1" D="8" E="Actual Work State 1-16"/>
	<DPD A="1058" B="0122" C="3" D="8" E="Setpoint Work State 1-16"/>
	<DPT A="8" B="009D" C="9" D="Type"/>
	<DPT A="15" B="0086" C="0" D="Offset X" E="1084"/>
	<DPT A="16" B="0087" C="0" D="Offset Y" E="1084"/>
	<DPT A="17" B="0088" C="0" D="Offset Z" E="1084"/>
	<DPT A="1079" B="00B3" C="3" D="Operation Type"/>
	<DPT A="274" B="0086" C="-20" D="Offset X" E="1085"/>
	<DPT A="530" B="0087" C="-17145" D="Offset Y" E="1085"/>
	<DPT A="786" B="0043" C="2286" D="Width" E="1085"/>
	<DPT A="275" B="0086" C="-20" D="Offset X" E="1085"/>
	<DPT A="531" B="0087" C="-14859" D="Offset Y" E="1085"/>
	<DPT A="787" B="0043" C="2286" D="Width" E="1085"/>
	<DPT A="276" B="0086" C="-20" D="Offset X" E="1085"/>
	<DPT A="532" B="0087" C="-12573" D="Offset Y" E="1085"/>
	<DPT A="788" B="0043" C="2286" D="Width" E="1085"/>
	<DPT A="277" B="0086" C="-20" D="Offset X" E="1085"/>
	<DPT A="533" B="0087" C="-10287" D="Offset Y" E="1085"/>
	<DPT A="789" B="0043" C="2286" D="Width" E="1085"/>
	<DPT A="278" B="0086" C="-20" D="Offset X" E="1085"/>
	<DPT A="534" B="0087" C="-8001" D="Offset Y" E="1085"/>
	<DPT A="790" B="0043" C="2286" D="Width" E="1085"/>
	<DPT A="279" B="0086" C="-20" D="Offset X" E="1085"/>
	<DPT A="535" B="0087" C="-5715" D="Offset Y" E="1085"/>
	<DPT A="791" B="0043" C="2286" D="Width" E="1085"/>
	<DPT A="280" B="0086" C="-20" D="Offset X" E="1085"/>
	<DPT A="536" B="0087" C="-3429" D="Offset Y" E="1085"/>
	<DPT A="792" B="0043" C="2286" D="Width" E="1085"/>
	<DPT A="281" B="0086" C="-20" D="Offset X" E="1085"/>
	<DPT A="537" B="0087" C="-1143" D="Offset Y" E="1085"/>
	<DPT A="793" B="0043" C="2286" D="Width" E="1085"/>
	<DPT A="282" B="0086" C="-20" D="Offset X" E="1085"/>
	<DPT A="538" B="0087" C="1143" D="Offset Y" E="1085"/>
	<DPT A="794" B="0043" C="2286" D="Width" E="1085"/>
	<DPT A="283" B="0086" C="-20" D="Offset X" E="1085"/>
	<DPT A="539" B="0087" C="3429" D="Offset Y" E="1085"/>
	<DPT A="795" B="0043" C="2286" D="Width" E="1085"/>
	<DPT A="284" B="0086" C="-20" D="Offset X" E="1085"/>
	<DPT A="540" B="0087" C="5715" D="Offset Y" E="1085"/>
	<DPT A="796" B="0043" C="2286" D="Width" E="1085"/>
	<DPT A="285" B="0086" C="-20" D="Offset X" E="1085"/>
	<DPT A="541" B="0087" C="8001" D="Offset Y" E="1085"/>
	<DPT A="797" B="0043" C="2286" D="Width" E="1085"/>
	<DPT A="286" B="0086" C="-20" D="Offset X" E="1085"/>
	<DPT A="542" B="0087" C="10287" D="Offset Y" E="1085"/>
	<DPT A="798" B="0043" C="2286" D="Width" E="1085"/>
	<DPT A="287" B="0086" C="-20" D="Offset X" E="1085"/>
	<DPT A="543" B="0087" C="12573" D="Offset Y" E="1085"/>
	<DPT A="799" B="0043" C="2286" D="Width" E="1085"/>
	<DPT A="288" B="0086" C="-20" D="Offset X" E="1085"/>
	<DPT A="544" B="0087" C="14859" D="Offset Y" E="1085"/>
	<DPT A="800" B="0043" C="2286" D="Width" E="1085"/>
	<DPT A="289" B="0086" C="-20" D="Offset X" E="1085"/>
	<DPT A="545" B="0087" C="17145" D="Offset Y" E="1085"/>
	<DPT A="801" B="0043" C="2286" D="Width" E="1085"/>
	<DVP A="1084" B="0" C="1.000000" D="0" E="mm"/>
	<DVP A="1085" B="0" C="0.001000" D="0" E="m"/>
	<DVP A="1082" B="0" C="1.000000" D="0" E="m^2"/>
	<DVP A="1086" B="0" C="0.001000" D="0" E="L"/>
	<DVP A="1083" B="0" C="1.000000" D="1" E="minutes"/>
	<DVP A="1087" B="0" C="0.001000" D="1" E="L/ha"/>
</DVC>
</ISO11783_TaskData>
)ISOXML";
	EXPECT_EQ(textXML, isoxml);
}
