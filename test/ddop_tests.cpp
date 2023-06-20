#include <gtest/gtest.h>

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
	EXPECT_EQ(true, testDDOP.add_device_process_data("Actual Work State", static_cast<std::uint16_t>(DataDescriptionIndex::ActualWorkState), task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceActualWorkState)));
	EXPECT_EQ(true, testDDOP.add_device_value_presentation("minutes", 0, 1.0f, 1, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TimePresentation)));
	EXPECT_EQ(true, testDDOP.add_device_element("Connector", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Connector), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), task_controller_object::DeviceElementObject::Type::Connector, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Connector)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Connector X", static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetX), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorXOffset)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Connector Y", static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetY), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorYOffset)));
	EXPECT_EQ(true, testDDOP.add_device_property("Type", 6, static_cast<std::uint16_t>(DataDescriptionIndex::ConnectorType), task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorType)));

	// Set up Boom
	EXPECT_EQ(true, testDDOP.add_device_element("Boom", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom), 0, task_controller_object::DeviceElementObject::Type::Function, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom)));
	EXPECT_EQ(true, testDDOP.add_device_property("Offset X", 0, static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetX), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::BoomXOffset)));
	EXPECT_EQ(true, testDDOP.add_device_property("Offset Y", 0, static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetY), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::BoomYOffset)));
	EXPECT_EQ(true, testDDOP.add_device_property("Offset Z", 0, static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetZ), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::BoomZOffset)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Actual Working Width", static_cast<std::uint16_t>(DataDescriptionIndex::ActualWorkingWidth), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ActualWorkingWidth)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Setpoint Work State", static_cast<std::uint16_t>(DataDescriptionIndex::SetpointWorkState), task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SetpointWorkState)));
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
	EXPECT_EQ(true, testDDOP.add_device_process_data("Actual Work State 1-16", static_cast<std::uint16_t>(DataDescriptionIndex::ActualCondensedWorkState1_16), task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ActualCondensedWorkingState)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Setpoint Work State", static_cast<std::uint16_t>(DataDescriptionIndex::SetpointCondensedWorkState1_16), task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable) | static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SetpointCondensedWorkingState)));

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
}

TEST(DDOP_TESTS, ProcessDataTests)
{
	DeviceDescriptorObjectPool testDDOPVersion3(3);
	DeviceDescriptorObjectPool testDDOPVersion4(4);
	LanguageCommandInterface testLanguageInterface(nullptr, nullptr);

	EXPECT_EQ(true, testDDOPVersion3.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(true, testDDOPVersion3.add_device_element("Sprayer", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), 0, task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement)));
	EXPECT_EQ(true, testDDOPVersion3.add_device_process_data("This is a very long designator that should get truncated", static_cast<std::uint16_t>(DataDescriptionIndex::ActualWorkState), task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceActualWorkState)));

	// Test that the PD Designator was truncated to 32
	auto tempPD = testDDOPVersion3.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceActualWorkState));
	ASSERT_NE(nullptr, tempPD);
	EXPECT_EQ(32, tempPD->get_designator().size());

	EXPECT_EQ(true, testDDOPVersion4.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(true, testDDOPVersion4.add_device_element("Sprayer", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), 0, task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement)));
	EXPECT_EQ(true, testDDOPVersion4.add_device_process_data("This is an even longer designator that should get truncated ideally to 128 characters in length but in reality not very many TCs will support this kind of long designator", static_cast<std::uint16_t>(DataDescriptionIndex::ActualWorkState), task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceActualWorkState)));

	// Version 4+ designators can be 128 long, mostly for utf-8 support, not ascii, but testing it with chars
	tempPD = testDDOPVersion4.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceActualWorkState));
	ASSERT_NE(nullptr, tempPD);
	EXPECT_EQ(128, tempPD->get_designator().size());

	EXPECT_EQ(tempPD->get_table_id(), "DPD");
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
	EXPECT_EQ(true, testDDOPVersion3.add_device_property("Type123456789123456789123456789000111222333", 9, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ConnectorType), isobus::task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorType)));

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
	EXPECT_EQ(true, testDDOPVersion4.add_device_property("Type123456789123456789123456789000111222333", 9, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ConnectorType), isobus::task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorType)));

	tempProperty = testDDOPVersion4.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorType));
	ASSERT_NE(nullptr, tempProperty);
	EXPECT_EQ(tempProperty->get_designator().size(), 43);

	EXPECT_EQ(true, testDDOPVersion4_2.add_device("AgIsoStack++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(true, testDDOPVersion4_2.add_device_element("Sprayer", 1, 0, isobus::task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement)));
	EXPECT_EQ(true, testDDOPVersion4_2.add_device_process_data("Total Time", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::EffectiveTotalTime), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TimePresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet) | static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::Total), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceTotalTime)));
	EXPECT_EQ(true, testDDOPVersion4_2.add_device_element("Connector", 2, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), isobus::task_controller_object::DeviceElementObject::Type::Connector, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Connector)));
	EXPECT_EQ(true, testDDOPVersion4_2.add_device_process_data("Connector X", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetX), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorXOffset)));
	EXPECT_EQ(true, testDDOPVersion4_2.add_device_process_data("Connector Y", static_cast<std::uint16_t>(isobus::DataDescriptionIndex::DeviceElementOffsetY), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(isobus::task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorYOffset)));
	EXPECT_EQ(true, testDDOPVersion4_2.add_device_property("Type123456789123456789123456789000111222333aksjdhflkajhdfasdfasdfasdfasdfasdfasdfiouhsidlfhalksjdhlkajshdflkasdfhlhasdfhalksjdflkasjhflkjashdfl", 9, static_cast<std::uint16_t>(isobus::DataDescriptionIndex::ConnectorType), isobus::task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorType)));

	tempProperty = testDDOPVersion4_2.get_object_by_id(static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorType));
	ASSERT_NE(nullptr, tempProperty);
	EXPECT_EQ(tempProperty->get_designator().size(), 128);
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
}
