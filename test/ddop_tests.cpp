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
	SectionCondensedWorkState1_16

};

TEST(DDOP_TESTS, CreateSprayerDDOP)
{
	DeviceDescriptorObjectPool testDDOP;

	// Build up a sprayer's DDOP
	LanguageCommandInterface testLanguageInterface(nullptr, nullptr);

	// Make a test pool, don't care about our ISO NAME, Localization label, or extended structure label for this test
	EXPECT_EQ(true, testDDOP.AddDevice("Isobus++ UnitTest", "1.0.0", "123", "I++1.0", testLanguageInterface.get_localization_raw_data(), std::vector<std::uint8_t>(), 0));
	EXPECT_EQ(true, testDDOP.AddDeviceElement("Sprayer", static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement), 0, task_controller_object::DeviceElementObject::Type::Device, static_cast<std::uint16_t>(SprayerDDOPObjectIDs::MainDeviceElement)));
	EXPECT_EQ(true, testDDOP.AddDeviceProcessData("Actual Work State", static_cast<std::uint16_t>(DataDescriptionIndex::ActualWorkState), task_controller_object::Object::NULL_OBJECT_ID, static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::PropertiesBit::MemberOfDefaultSet), static_cast<std::uint8_t>(task_controller_object::DeviceProcessDataObject::AvailableTriggerMethods::OnChange), static_cast<std::uint16_t>(SprayerDDOPObjectIDs::DeviceActualWorkState)));
}
