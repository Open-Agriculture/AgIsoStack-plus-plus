#include <gtest/gtest.h>

#include "isobus/isobus/isobus_device_descriptor_object_pool.hpp"
#include "isobus/isobus/isobus_language_command_interface.hpp"
#include "isobus/isobus/isobus_standard_data_description_indices.hpp"

using namespace isobus;

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
	SectionCondensedWorkState1_16,

	TimePresentation,
	ShortWidthPresentation,
	LongWidthPresentation
};

TEST(DDOP_TESTS, CreateSprayerDDOP)
{
	DeviceDescriptorObjectPool testDDOP;

	// Build up a sprayer's DDOP
	LanguageCommandInterface testLanguageInterface(nullptr, nullptr);

	// Make a test pool, don't care about our ISO NAME, Localization label, or extended structure label for this test
	EXPECT_EQ(true, testDDOP.add_device("Isobus++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(true, testDDOP.add_device_element("Sprayer", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), 0, task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Actual Work State", static_cast<std::uint16_t>(DataDescriptionIndex::ActualWorkState), task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceActualWorkState)));
	EXPECT_EQ(true, testDDOP.add_device_value_presentation("min", 0, 1.0f, 1, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::TimePresentation)));
	EXPECT_EQ(true, testDDOP.add_device_element("Connector", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Connector), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), task_controller_object::DeviceElementObject::Type::Connector, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::Connector)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Connector X", static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetX), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorXOffset)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Connector Y", static_cast<std::uint16_t>(DataDescriptionIndex::DeviceElementOffsetY), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::Settable), 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorYOffset)));
	EXPECT_EQ(true, testDDOP.add_device_property("Type", 6, static_cast<std::uint16_t>(DataDescriptionIndex::ConnectorType), task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ConnectorType)));
	EXPECT_EQ(true, testDDOP.add_device_element("Boom", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom), 0, task_controller_object::DeviceElementObject::Type::Function, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::SprayBoom)));
	EXPECT_EQ(true, testDDOP.add_device_process_data("Actual Working Width", static_cast<std::uint16_t>(DataDescriptionIndex::ActualWorkingWidth), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ActualWorkingWidth)));
	EXPECT_EQ(true, testDDOP.add_device_value_presentation("mm", 0, 1.0f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::ShortWidthPresentation)));
	EXPECT_EQ(true, testDDOP.add_device_value_presentation("m", 0, 1.0f, 0, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::LongWidthPresentation)));

	std::vector<std::uint8_t> binaryDDOP;

	EXPECT_EQ(true, testDDOP.generate_binary_object_pool(binaryDDOP));
}
