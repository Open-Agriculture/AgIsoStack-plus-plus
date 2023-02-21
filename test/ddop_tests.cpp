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

	// Make a test pool, don't care about our ISO NAME, Localization label, or extended structure label for this test
	// Set up device
	EXPECT_EQ(true, testDDOP.add_device("Isobus++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
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
}

TEST(DDOP_TESTS, DDOPDetectDuplicateID)
{
	DeviceDescriptorObjectPool testDDOP;
	LanguageCommandInterface testLanguageInterface(nullptr, nullptr);

	EXPECT_EQ(true, testDDOP.add_device("Isobus++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(false, testDDOP.add_device("Isobus++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));

	EXPECT_EQ(true, testDDOP.add_device_value_presentation("m", 0, 0.001f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation)));
	EXPECT_EQ(false, testDDOP.add_device_value_presentation("m", 0, 0.001f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation)));

	EXPECT_EQ(true, testDDOP.add_device_element("Product", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LiquidProduct), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom), task_controller_object::DeviceElementObject::Type::Bin, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LiquidProduct)));
	EXPECT_EQ(false, testDDOP.add_device_element("Product", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LiquidProduct), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom), task_controller_object::DeviceElementObject::Type::Bin, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LiquidProduct)));

	EXPECT_EQ(true, testDDOP.add_device_process_data("Tank Capacity", static_cast<std::uint16_t>(DataDescriptionIndex::MaximumVolumeContent), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::VolumePresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TankCapacity)));
	EXPECT_EQ(false, testDDOP.add_device_process_data("Tank Capacity", static_cast<std::uint16_t>(DataDescriptionIndex::MaximumVolumeContent), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::VolumePresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TankCapacity)));
}
