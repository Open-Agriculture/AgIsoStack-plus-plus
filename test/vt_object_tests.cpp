//================================================================================================
/// @file vt_object_tests.cpp
///
/// @brief Unit tests for the various VT objects
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include <gtest/gtest.h>

#include "isobus/isobus/isobus_virtual_terminal_objects.hpp"

using namespace isobus;

static void run_baseline_tests(VTObject *objectUnderTest)
{
	ASSERT_NE(nullptr, objectUnderTest);
	EXPECT_EQ(objectUnderTest->get_background_color(), 0);
	EXPECT_EQ(objectUnderTest->get_number_children(), 0);
	EXPECT_EQ(objectUnderTest->get_id(), NULL_OBJECT_ID);
	EXPECT_EQ(objectUnderTest->get_height(), 0);
	EXPECT_EQ(objectUnderTest->get_width(), 0);
	EXPECT_NE(objectUnderTest->get_minumum_object_length(), 0);

	objectUnderTest->set_background_color(9);
	EXPECT_EQ(objectUnderTest->get_background_color(), 9);
	objectUnderTest->set_height(100);
	EXPECT_EQ(objectUnderTest->get_height(), 100);
	objectUnderTest->set_width(200);
	EXPECT_EQ(objectUnderTest->get_width(), 200);
	objectUnderTest->add_child(300, 0, 0);
	EXPECT_EQ(objectUnderTest->get_number_children(), 1);
	ASSERT_NO_THROW(objectUnderTest->get_child_id(0));
	EXPECT_EQ(objectUnderTest->get_child_id(0), 300);
	objectUnderTest->remove_child(300, 0, 0);
	EXPECT_EQ(objectUnderTest->get_number_children(), 0);
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, WorkingSetTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto ws = std::make_shared<WorkingSet>(objects, colourTable);

	run_baseline_tests(ws.get());
	EXPECT_EQ(ws->get_active_mask(), 0xFFFF);
	EXPECT_EQ(ws->get_active_mask(), 0xFFFF);
	EXPECT_FALSE(ws->get_selectable());
	EXPECT_EQ(ws->get_object_type(), VirtualTerminalObjectType::WorkingSet);

	ws->set_active_mask(1234);
	EXPECT_EQ(ws->get_active_mask(), 1234);
	ws->set_selectable(true);
	EXPECT_TRUE(ws->get_selectable());

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;
	EXPECT_TRUE(ws->set_attribute(static_cast<std::uint8_t>(WorkingSet::AttributeName::ActiveMask), 4321, error));
	EXPECT_EQ(ws->get_active_mask(), 4321);

	EXPECT_TRUE(ws->set_attribute(static_cast<std::uint8_t>(WorkingSet::AttributeName::Selectable), 1, error));
	EXPECT_EQ(ws->get_selectable(), true);
	EXPECT_TRUE(ws->set_attribute(static_cast<std::uint8_t>(WorkingSet::AttributeName::Selectable), 0, error));
	EXPECT_EQ(ws->get_selectable(), false);

	EXPECT_TRUE(ws->set_attribute(static_cast<std::uint8_t>(WorkingSet::AttributeName::BackgroundColour), 41, error));
	EXPECT_EQ(ws->get_background_color(), 41);
	EXPECT_TRUE(ws->set_attribute(static_cast<std::uint8_t>(WorkingSet::AttributeName::BackgroundColour), 0, error));
	EXPECT_EQ(ws->get_background_color(), 0);

	// Setting the type attribute should always fail
	EXPECT_FALSE(ws->set_attribute(static_cast<std::uint8_t>(WorkingSet::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(ws->set_attribute(static_cast<std::uint8_t>(WorkingSet::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test the validity checker
	EXPECT_FALSE(ws->get_is_valid());
	ws->set_id(10);
	objects[ws->get_id()] = ws;
	EXPECT_TRUE(ws->get_is_valid());

	// Add a valid object, a container
	auto container = std::make_shared<Container>(objects, colourTable);
	container->set_id(20);
	objects[container->get_id()] = container;
	ws->add_child(container->get_id(), 0, 0);
	EXPECT_TRUE(ws->get_is_valid());

	// Add an invalid object, a Key
	auto key = std::make_shared<Key>(objects, colourTable);
	key->set_id(30);
	objects[key->get_id()] = key;
	ws->add_child(key->get_id(), 0, 0);
	EXPECT_FALSE(ws->get_is_valid());

	// Test some basic colour table stuff
	auto white = colourTable.get_colour(1);
	EXPECT_NEAR(white.r, 1.0f, 0.0001f);
	EXPECT_NEAR(white.g, 1.0f, 0.0001f);
	EXPECT_NEAR(white.b, 1.0f, 0.0001f);

	// Change white to be some other random colour
	colourTable.set_colour(1, { 0.5f, 0.5f, 0.5f });
	white = colourTable.get_colour(1);
	EXPECT_NEAR(white.r, 0.5f, 0.0001f);
	EXPECT_NEAR(white.g, 0.5f, 0.0001f);
	EXPECT_NEAR(white.b, 0.5f, 0.0001f);

	// Test setting and getting all attributes
	uint32_t testValue = 0;
	EXPECT_TRUE(ws->set_attribute(static_cast<std::uint8_t>(WorkingSet::AttributeName::ActiveMask), 1234, error));
	EXPECT_TRUE(ws->get_attribute(static_cast<std::uint8_t>(WorkingSet::AttributeName::ActiveMask), testValue));
	EXPECT_EQ(testValue, 1234);

	EXPECT_TRUE(ws->set_attribute(static_cast<std::uint8_t>(WorkingSet::AttributeName::Selectable), 1, error));
	EXPECT_TRUE(ws->get_attribute(static_cast<std::uint8_t>(WorkingSet::AttributeName::Selectable), testValue));
	EXPECT_EQ(testValue, 1);

	EXPECT_TRUE(ws->set_attribute(static_cast<std::uint8_t>(WorkingSet::AttributeName::BackgroundColour), 41, error));
	EXPECT_TRUE(ws->get_attribute(static_cast<std::uint8_t>(WorkingSet::AttributeName::BackgroundColour), testValue));
	EXPECT_EQ(testValue, 41);

	EXPECT_TRUE(ws->get_attribute(static_cast<std::uint8_t>(WorkingSet::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::WorkingSet));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, DataMaskTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	DataMask mask(objects, colourTable);

	run_baseline_tests(&mask);
	EXPECT_EQ(mask.get_object_type(), VirtualTerminalObjectType::DataMask);

	// Test data mask background colour
	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;
	mask.set_background_color(10);
	EXPECT_EQ(mask.get_background_color(), 10);
	EXPECT_TRUE(mask.set_attribute(static_cast<std::uint8_t>(DataMask::AttributeName::BackgroundColour), 20, error));
	EXPECT_EQ(mask.get_background_color(), 20);

	// Setting the type attribute should always fail
	EXPECT_FALSE(mask.set_attribute(static_cast<std::uint8_t>(DataMask::AttributeName::Type), 4, error));

	// We expect there to normally be some kind of object that goes around
	// and adds things to the object map.  We'll simulate that here.
	//
	// Test adding a soft key mask and changing it
	// We'll make a new shared pointer to an data mask
	auto dataMask2 = std::make_shared<DataMask>(objects, colourTable);
	dataMask2->set_id(1); // Arbitrary ID
	objects[dataMask2->get_id()] = dataMask2;

	// Let's add a soft key mask to the alarm mask
	auto softKeyMask = std::make_shared<SoftKeyMask>(objects, colourTable);
	softKeyMask->set_id(100);
	dataMask2->add_child(softKeyMask->get_id(), 0, 0);
	objects[softKeyMask->get_id()] = softKeyMask;

	// now let's make a different soft key mask that we'll use to replace the old one
	auto softKeyMask2 = std::make_shared<SoftKeyMask>(objects, colourTable);
	softKeyMask2->set_id(200);
	objects[softKeyMask2->get_id()] = softKeyMask2;

	EXPECT_TRUE(dataMask2->get_is_valid());

	// Add an invalid object, another data mask
	auto dataMask3 = std::make_shared<DataMask>(objects, colourTable);
	dataMask3->set_id(2); // Arbitrary ID
	objects[dataMask3->get_id()] = dataMask3;
	dataMask2->add_child(dataMask3->get_id(), 0, 0);
	EXPECT_FALSE(dataMask2->get_is_valid());

	// Take this opportunity to check that getting an object by ID works in the base class
	auto testObject = softKeyMask2->get_object_by_id(200);
	ASSERT_NE(nullptr, testObject);
	EXPECT_EQ(200, testObject->get_id());

	EXPECT_TRUE(dataMask2->set_attribute(static_cast<std::uint8_t>(DataMask::AttributeName::SoftKeyMask), 200, error));
	EXPECT_EQ(dataMask2->get_child_id(1), 200);
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Try changing the soft key mask to the other one, which is ID 100
	EXPECT_TRUE(dataMask2->change_soft_key_mask(100));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(dataMask2->set_attribute(static_cast<std::uint8_t>(DataMask::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test setting and getting all attributes
	uint32_t testValue = 0;
	EXPECT_TRUE(dataMask2->set_attribute(static_cast<std::uint8_t>(DataMask::AttributeName::BackgroundColour), 41, error));
	EXPECT_TRUE(dataMask2->get_attribute(static_cast<std::uint8_t>(DataMask::AttributeName::BackgroundColour), testValue));
	EXPECT_EQ(testValue, 41);

	EXPECT_FALSE(dataMask2->set_attribute(static_cast<std::uint8_t>(DataMask::AttributeName::SoftKeyMask), 50, error));
	EXPECT_TRUE(dataMask2->get_attribute(static_cast<std::uint8_t>(DataMask::AttributeName::SoftKeyMask), testValue));
	EXPECT_EQ(testValue, NULL_OBJECT_ID);

	EXPECT_TRUE(dataMask2->get_attribute(static_cast<std::uint8_t>(DataMask::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::DataMask));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, ContainerTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	Container container(objects, colourTable);

	run_baseline_tests(&container);
	EXPECT_EQ(container.get_object_type(), VirtualTerminalObjectType::Container);

	EXPECT_EQ(container.get_hidden(), false);
	container.set_hidden(true);
	EXPECT_EQ(container.get_hidden(), true);

	// Check read only attributes
	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;
	container.set_attribute(static_cast<std::uint8_t>(Container::AttributeName::Hidden), 0, error);
	EXPECT_NE(0, static_cast<std::uint8_t>(error));
	container.set_attribute(static_cast<std::uint8_t>(Container::AttributeName::Height), 50, error);
	EXPECT_NE(0, static_cast<std::uint8_t>(error));
	container.set_attribute(static_cast<std::uint8_t>(Container::AttributeName::Width), 50, error);
	EXPECT_NE(0, static_cast<std::uint8_t>(error));
	EXPECT_TRUE(container.get_hidden());
	EXPECT_NE(50, container.get_width());
	EXPECT_NE(50, container.get_height());

	// Setting the type attribute should always fail
	EXPECT_FALSE(container.set_attribute(static_cast<std::uint8_t>(Container::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(container.set_attribute(static_cast<std::uint8_t>(Container::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Take this opportunity to test child object offsets and pop_child
	container.add_child(100, 10, 20);
	container.set_child_x(0, 50);
	container.set_child_y(0, 60);
	EXPECT_EQ(container.get_child_x(0), 50);
	EXPECT_EQ(container.get_child_y(0), 60);
	container.pop_child();
	EXPECT_EQ(container.get_number_children(), 0);

	container.set_id(100);

	// Add a valid child object, a Button
	auto button = std::make_shared<Button>(objects, colourTable);
	button->set_id(200);
	objects[button->get_id()] = button;
	container.add_child(button->get_id(), 0, 0);
	EXPECT_TRUE(container.get_is_valid());

	// Add an invalid object, a data mask
	auto dataMask = std::make_shared<DataMask>(objects, colourTable);
	dataMask->set_id(300);
	objects[dataMask->get_id()] = dataMask;
	container.add_child(dataMask->get_id(), 0, 0);
	EXPECT_FALSE(container.get_is_valid());

	uint32_t testValue = 0;
	EXPECT_TRUE(container.get_attribute(static_cast<std::uint8_t>(Container::AttributeName::Hidden), testValue));
	EXPECT_EQ(testValue, 1);

	EXPECT_TRUE(container.get_attribute(static_cast<std::uint8_t>(Container::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 200);

	EXPECT_TRUE(container.get_attribute(static_cast<std::uint8_t>(Container::AttributeName::Height), testValue));
	EXPECT_EQ(testValue, 100);

	EXPECT_TRUE(container.get_attribute(static_cast<std::uint8_t>(Container::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::Container));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, AlarmMaskTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	AlarmMask alarmMask(objects, colourTable);

	run_baseline_tests(&alarmMask);
	EXPECT_EQ(alarmMask.get_object_type(), VirtualTerminalObjectType::AlarmMask);

	// We expect there to normally be some kind of objec that goes around
	// and adds things to the object map.  We'll simulate that here.

	// We'll make a new shared pointer to an alarm mask
	auto alarmMask2 = std::make_shared<AlarmMask>(objects, colourTable);
	alarmMask2->set_id(1); // Arbitrary ID
	objects[alarmMask2->get_id()] = alarmMask2;

	// Let's add a soft key mask to the alarm mask
	auto softKeyMask = std::make_shared<SoftKeyMask>(objects, colourTable);
	softKeyMask->set_id(100);
	alarmMask2->add_child(softKeyMask->get_id(), 0, 0);
	objects[softKeyMask->get_id()] = softKeyMask;

	// now let's make a different soft key mask that we'll use to replace the old one
	auto softKeyMask2 = std::make_shared<SoftKeyMask>(objects, colourTable);
	softKeyMask2->set_id(200);
	objects[softKeyMask2->get_id()] = softKeyMask2;

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;
	EXPECT_TRUE(alarmMask2->set_attribute(static_cast<std::uint8_t>(AlarmMask::AttributeName::SoftKeyMask), 200, error));
	EXPECT_EQ(alarmMask2->get_child_id(0), 200);

	// Test alarm mask priority
	alarmMask2->set_mask_priority(AlarmMask::Priority::Medium);
	EXPECT_EQ(alarmMask2->get_mask_priority(), AlarmMask::Priority::Medium);
	EXPECT_TRUE(alarmMask2->set_attribute(static_cast<std::uint8_t>(AlarmMask::AttributeName::Priority), static_cast<std::uint8_t>(AlarmMask::Priority::High), error));
	EXPECT_EQ(alarmMask2->get_mask_priority(), AlarmMask::Priority::High);

	// Test alarm mask acoustic signal
	alarmMask2->set_signal_priority(AlarmMask::AcousticSignal::Medium);
	EXPECT_EQ(alarmMask2->get_signal_priority(), AlarmMask::AcousticSignal::Medium);
	EXPECT_TRUE(alarmMask2->set_attribute(static_cast<std::uint8_t>(AlarmMask::AttributeName::AcousticSignal), static_cast<std::uint8_t>(AlarmMask::AcousticSignal::Highest), error));
	EXPECT_EQ(alarmMask2->get_signal_priority(), AlarmMask::AcousticSignal::Highest);

	// Test alarm mask acoustic signal with an invalid value
	alarmMask2->set_signal_priority(AlarmMask::AcousticSignal::Medium);
	EXPECT_EQ(alarmMask2->get_signal_priority(), AlarmMask::AcousticSignal::Medium);
	EXPECT_FALSE(alarmMask2->set_attribute(static_cast<std::uint8_t>(AlarmMask::AttributeName::AcousticSignal), 999, error));
	EXPECT_EQ(alarmMask2->get_signal_priority(), AlarmMask::AcousticSignal::Medium);

	// Test alarm mask background colour
	alarmMask2->set_background_color(10);
	EXPECT_EQ(alarmMask2->get_background_color(), 10);
	EXPECT_TRUE(alarmMask2->set_attribute(static_cast<std::uint8_t>(AlarmMask::AttributeName::BackgroundColour), 20, error));
	EXPECT_EQ(alarmMask2->get_background_color(), 20);

	// Test the validator
	EXPECT_TRUE(alarmMask2->get_is_valid());

	// Add an invalid object, another Alarm Mask
	auto alarmMask3 = std::make_shared<AlarmMask>(objects, colourTable);
	alarmMask3->set_id(2); // Arbitrary ID
	objects[alarmMask3->get_id()] = alarmMask3;
	alarmMask2->add_child(alarmMask3->get_id(), 0, 0);
	EXPECT_FALSE(alarmMask2->get_is_valid());

	// Setting the type attribute should always fail
	EXPECT_FALSE(alarmMask2->set_attribute(static_cast<std::uint8_t>(AlarmMask::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(alarmMask2->set_attribute(static_cast<std::uint8_t>(AlarmMask::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Attempt to change the soft key mask to the other one, which is ID 100
	EXPECT_TRUE(alarmMask2->change_soft_key_mask(100));

	uint32_t testValue = 0;
	EXPECT_TRUE(alarmMask2->get_attribute(static_cast<std::uint8_t>(AlarmMask::AttributeName::SoftKeyMask), testValue));
	EXPECT_EQ(testValue, 100);

	EXPECT_TRUE(alarmMask2->get_attribute(static_cast<std::uint8_t>(AlarmMask::AttributeName::Priority), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(AlarmMask::Priority::High));

	EXPECT_TRUE(alarmMask2->get_attribute(static_cast<std::uint8_t>(AlarmMask::AttributeName::AcousticSignal), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(AlarmMask::AcousticSignal::Medium));

	EXPECT_TRUE(alarmMask2->get_attribute(static_cast<std::uint8_t>(AlarmMask::AttributeName::BackgroundColour), testValue));
	EXPECT_EQ(testValue, 20);

	EXPECT_TRUE(alarmMask2->get_attribute(static_cast<std::uint8_t>(AlarmMask::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::AlarmMask));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, SoftKeyMaskTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto softKeyMask = std::make_shared<SoftKeyMask>(objects, colourTable);

	run_baseline_tests(softKeyMask.get());
	EXPECT_EQ(softKeyMask->get_object_type(), VirtualTerminalObjectType::SoftKeyMask);

	// Test soft key mask background colour
	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;
	softKeyMask->set_background_color(10);
	EXPECT_EQ(softKeyMask->get_background_color(), 10);
	EXPECT_TRUE(softKeyMask->set_attribute(static_cast<std::uint8_t>(SoftKeyMask::AttributeName::BackgroundColour), 20, error));
	EXPECT_EQ(softKeyMask->get_background_color(), 20);

	// Setting the type attribute should always fail
	EXPECT_FALSE(softKeyMask->set_attribute(static_cast<std::uint8_t>(SoftKeyMask::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(softKeyMask->set_attribute(static_cast<std::uint8_t>(SoftKeyMask::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	softKeyMask->set_id(100);
	objects[softKeyMask->get_id()] = softKeyMask;

	EXPECT_TRUE(softKeyMask->get_is_valid());

	// Add an invalid object, a container
	auto container = std::make_shared<Container>(objects, colourTable);
	container->set_id(200);
	objects[container->get_id()] = container;
	softKeyMask->add_child(container->get_id(), 0, 0);
	EXPECT_FALSE(softKeyMask->get_is_valid());
	softKeyMask->remove_child(200, 0, 0);

	// Add a valid object, a Key
	auto key = std::make_shared<Key>(objects, colourTable);
	key->set_id(300);
	objects[key->get_id()] = key;
	softKeyMask->add_child(key->get_id(), 0, 0);
	EXPECT_TRUE(softKeyMask->get_is_valid());

	uint32_t testValue = 0;
	EXPECT_TRUE(softKeyMask->get_attribute(static_cast<std::uint8_t>(SoftKeyMask::AttributeName::BackgroundColour), testValue));
	EXPECT_EQ(testValue, 20);

	EXPECT_TRUE(softKeyMask->get_attribute(static_cast<std::uint8_t>(SoftKeyMask::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::SoftKeyMask));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, SoftKeyTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto softKey = std::make_shared<Key>(objects, colourTable);

	run_baseline_tests(softKey.get());
	EXPECT_EQ(softKey->get_object_type(), VirtualTerminalObjectType::Key);

	softKey->set_key_code(46);
	EXPECT_EQ(softKey->get_key_code(), 46);

	// Test key background colour
	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;
	softKey->set_background_color(10);
	EXPECT_EQ(softKey->get_background_color(), 10);
	EXPECT_TRUE(softKey->set_attribute(static_cast<std::uint8_t>(Key::AttributeName::BackgroundColour), 20, error));
	EXPECT_EQ(softKey->get_background_color(), 20);

	// Test key code attribute
	EXPECT_TRUE(softKey->set_attribute(static_cast<std::uint8_t>(Key::AttributeName::KeyCode), 16, error));
	EXPECT_EQ(softKey->get_key_code(), 16);

	// Setting the type attribute should always fail
	EXPECT_FALSE(softKey->set_attribute(static_cast<std::uint8_t>(Key::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(softKey->set_attribute(static_cast<std::uint8_t>(Key::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	softKey->set_id(100);
	objects[softKey->get_id()] = softKey;

	// Add a valid child, a picture graphic
	auto pictureGraphic = std::make_shared<PictureGraphic>(objects, colourTable);
	pictureGraphic->set_id(200);
	objects[pictureGraphic->get_id()] = pictureGraphic;
	softKey->add_child(pictureGraphic->get_id(), 0, 0);
	EXPECT_TRUE(softKey->get_is_valid());

	// Add an invalid object, a data mask
	auto dataMask = std::make_shared<DataMask>(objects, colourTable);
	dataMask->set_id(300);
	objects[dataMask->get_id()] = dataMask;
	softKey->add_child(dataMask->get_id(), 0, 0);
	EXPECT_FALSE(softKey->get_is_valid());

	uint32_t testValue = 0;
	EXPECT_TRUE(softKey->get_attribute(static_cast<std::uint8_t>(Key::AttributeName::BackgroundColour), testValue));
	EXPECT_EQ(testValue, 20);

	EXPECT_TRUE(softKey->get_attribute(static_cast<std::uint8_t>(Key::AttributeName::KeyCode), testValue));
	EXPECT_EQ(testValue, 16);

	EXPECT_TRUE(softKey->get_attribute(static_cast<std::uint8_t>(Key::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::Key));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, ButtonTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto button = std::make_shared<Button>(objects, colourTable);

	run_baseline_tests(button.get());
	EXPECT_EQ(button->get_object_type(), VirtualTerminalObjectType::Button);

	// Test button background colour
	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;
	button->set_background_color(10);
	EXPECT_EQ(button->get_background_color(), 10);
	EXPECT_TRUE(button->set_attribute(static_cast<std::uint8_t>(Button::AttributeName::BackgroundColour), 20, error));
	EXPECT_EQ(button->get_background_color(), 20);

	// Test button code attribute
	EXPECT_TRUE(button->set_attribute(static_cast<std::uint8_t>(Button::AttributeName::KeyCode), 16, error));
	EXPECT_EQ(button->get_key_code(), 16);

	// Setting the type attribute should always fail
	EXPECT_FALSE(button->set_attribute(static_cast<std::uint8_t>(Button::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(button->set_attribute(static_cast<std::uint8_t>(Button::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test button width attribute
	EXPECT_TRUE(button->set_attribute(static_cast<std::uint8_t>(Button::AttributeName::Width), 50, error));
	EXPECT_EQ(button->get_width(), 50);

	// Test Button height attribute
	EXPECT_TRUE(button->set_attribute(static_cast<std::uint8_t>(Button::AttributeName::Height), 50, error));
	EXPECT_EQ(button->get_height(), 50);

	// Test Button border colour attribute
	EXPECT_TRUE(button->set_attribute(static_cast<std::uint8_t>(Button::AttributeName::BorderColour), 75, error));
	EXPECT_EQ(button->get_border_colour(), 75);

	// Test Options attribute
	EXPECT_TRUE(button->set_attribute(static_cast<std::uint8_t>(Button::AttributeName::Options), 0x01, error));
	EXPECT_EQ(button->get_option(Button::Options::Latchable), true);

	button->set_option(Button::Options::NoBorder, true);
	EXPECT_TRUE(button->get_option(Button::Options::NoBorder));
	button->set_option(Button::Options::NoBorder, false);
	EXPECT_FALSE(button->get_option(Button::Options::NoBorder));

	button->set_id(100);
	objects[button->get_id()] = button;

	// Add a valid child, a picture graphic
	auto pictureGraphic = std::make_shared<PictureGraphic>(objects, colourTable);
	pictureGraphic->set_id(200);
	objects[pictureGraphic->get_id()] = pictureGraphic;
	button->add_child(pictureGraphic->get_id(), 0, 0);
	EXPECT_TRUE(button->get_is_valid());

	// Add an invalid object, a data mask
	auto dataMask = std::make_shared<DataMask>(objects, colourTable);
	dataMask->set_id(300);
	objects[dataMask->get_id()] = dataMask;
	button->add_child(dataMask->get_id(), 0, 0);
	EXPECT_FALSE(button->get_is_valid());

	uint32_t testValue = 0;
	EXPECT_TRUE(button->get_attribute(static_cast<std::uint8_t>(Button::AttributeName::BackgroundColour), testValue));
	EXPECT_EQ(testValue, 20);

	EXPECT_TRUE(button->get_attribute(static_cast<std::uint8_t>(Button::AttributeName::KeyCode), testValue));
	EXPECT_EQ(testValue, 16);

	EXPECT_TRUE(button->get_attribute(static_cast<std::uint8_t>(Button::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(button->get_attribute(static_cast<std::uint8_t>(Button::AttributeName::Height), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(button->get_attribute(static_cast<std::uint8_t>(Button::AttributeName::BorderColour), testValue));
	EXPECT_EQ(testValue, 75);

	EXPECT_TRUE(button->get_attribute(static_cast<std::uint8_t>(Button::AttributeName::Options), testValue));
	EXPECT_EQ(testValue, 0x01);

	EXPECT_TRUE(button->get_attribute(static_cast<std::uint8_t>(Button::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::Button));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, KeyGroupTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto keyGroup = std::make_shared<KeyGroup>(objects, colourTable);
	auto testName = std::make_shared<OutputString>(objects, colourTable);

	run_baseline_tests(keyGroup.get());
	EXPECT_EQ(keyGroup->get_object_type(), VirtualTerminalObjectType::KeyGroup);

	keyGroup->set_id(100);
	objects[keyGroup->get_id()] = keyGroup;
	EXPECT_EQ(100, keyGroup->get_id());

	testName->set_id(200);
	objects[testName->get_id()] = testName;
	keyGroup->set_name_object_id(200);

	keyGroup->set_key_group_icon(500);
	EXPECT_EQ(500, keyGroup->get_key_group_icon());

	keyGroup->set_option(KeyGroup::Options::Available, true);
	EXPECT_TRUE(keyGroup->get_option(KeyGroup::Options::Available));
	keyGroup->set_options(0);
	EXPECT_FALSE(keyGroup->get_option(KeyGroup::Options::Available));
	keyGroup->set_options(1);
	EXPECT_TRUE(keyGroup->get_option(KeyGroup::Options::Available));
	keyGroup->set_option(KeyGroup::Options::Available, false);
	EXPECT_FALSE(keyGroup->get_option(KeyGroup::Options::Available));

	EXPECT_TRUE(keyGroup->get_is_valid());

	// Add a key
	auto key = std::make_shared<Key>(objects, colourTable);
	key->set_id(300);
	objects[key->get_id()] = key;
	keyGroup->add_child(key->get_id(), 0, 0);

	// It should still be valid
	EXPECT_TRUE(keyGroup->get_is_valid());

	// Add an object pointer that isn't a key
	auto objectPointer = std::make_shared<ObjectPointer>(objects, colourTable);
	objectPointer->set_id(400);
	objects[objectPointer->get_id()] = objectPointer;
	objectPointer->add_child(key->get_id(), 0, 0);
	keyGroup->add_child(objectPointer->get_id(), 0, 0);

	// It should still be valid
	EXPECT_TRUE(keyGroup->get_is_valid());

	// Change the object pointer to some random thing
	auto container = std::make_shared<Container>(objects, colourTable);
	container->set_id(500);
	objects[container->get_id()] = container;
	objectPointer->remove_child(key->get_id(), 0, 0);
	objectPointer->add_child(container->get_id(), 0, 0);

	// It should be invalid
	EXPECT_FALSE(keyGroup->get_is_valid());

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	EXPECT_TRUE(keyGroup->set_attribute(static_cast<std::uint8_t>(KeyGroup::AttributeName::Options), 1, error));
	EXPECT_TRUE(keyGroup->get_option(KeyGroup::Options::Available));

	// Make an output string we can use to test the name of the key group
	auto outputString = std::make_shared<OutputString>(objects, colourTable);
	outputString->set_id(600);
	objects[outputString->get_id()] = outputString;
	keyGroup->add_child(outputString->get_id(), 0, 0);

	// Now let's change the name of the key group
	EXPECT_TRUE(keyGroup->set_attribute(static_cast<std::uint8_t>(KeyGroup::AttributeName::Name), 600, error));
	EXPECT_EQ(keyGroup->get_name_object_id(), 600);

	// Setting the type attribute should always fail
	EXPECT_FALSE(keyGroup->set_attribute(static_cast<std::uint8_t>(KeyGroup::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(keyGroup->set_attribute(static_cast<std::uint8_t>(KeyGroup::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	uint32_t testValue = 0;
	EXPECT_TRUE(keyGroup->get_attribute(static_cast<std::uint8_t>(KeyGroup::AttributeName::Options), testValue));
	EXPECT_EQ(testValue, 1);

	EXPECT_TRUE(keyGroup->get_attribute(static_cast<std::uint8_t>(KeyGroup::AttributeName::Name), testValue));
	EXPECT_EQ(testValue, 600);

	EXPECT_TRUE(keyGroup->get_attribute(static_cast<std::uint8_t>(KeyGroup::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::KeyGroup));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, InputBooleanTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto inputBoolean = std::make_shared<InputBoolean>(objects, colourTable);

	run_baseline_tests(inputBoolean.get());
	EXPECT_EQ(inputBoolean->get_object_type(), VirtualTerminalObjectType::InputBoolean);

	// Test input boolean background colour
	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;
	inputBoolean->set_background_color(10);
	EXPECT_EQ(inputBoolean->get_background_color(), 10);
	EXPECT_TRUE(inputBoolean->set_attribute(static_cast<std::uint8_t>(InputBoolean::AttributeName::BackgroundColour), 20, error));
	EXPECT_EQ(inputBoolean->get_background_color(), 20);

	// Setting the type attribute should always fail
	EXPECT_FALSE(inputBoolean->set_attribute(static_cast<std::uint8_t>(InputBoolean::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(inputBoolean->set_attribute(static_cast<std::uint8_t>(InputBoolean::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test input boolean width attribute
	EXPECT_TRUE(inputBoolean->set_attribute(static_cast<std::uint8_t>(InputBoolean::AttributeName::Width), 50, error));
	EXPECT_EQ(inputBoolean->get_width(), 50);

	inputBoolean->set_value(true);
	EXPECT_TRUE(inputBoolean->get_value());
	inputBoolean->set_value(false);
	EXPECT_FALSE(inputBoolean->get_value());
	EXPECT_TRUE(inputBoolean->set_attribute(static_cast<std::uint8_t>(InputBoolean::AttributeName::Value), 1, error));
	EXPECT_TRUE(inputBoolean->get_value());

	// Lets do some tests using a font attributes object as the foreground colour
	// First, let's make a font attributes object
	auto fontAttribute = std::make_shared<FontAttributes>(objects, colourTable);
	fontAttribute->set_id(1); // Arbitrary
	objects[fontAttribute->get_id()] = fontAttribute;

	// Add it as a child object
	inputBoolean->add_child(fontAttribute->get_id(), 0, 0);

	// Now lets replace it with a different font attributes object using set_attribute
	auto fontAttribute2 = std::make_shared<FontAttributes>(objects, colourTable);
	fontAttribute2->set_id(2); // Arbitrary
	objects[fontAttribute2->get_id()] = fontAttribute2;

	EXPECT_TRUE(inputBoolean->set_attribute(static_cast<std::uint8_t>(InputBoolean::AttributeName::ForegroundColour), fontAttribute2->get_id(), error));
	ASSERT_NO_THROW(inputBoolean->get_child_id(0));
	EXPECT_EQ(inputBoolean->get_child_id(0), fontAttribute2->get_id()); // Now the 2nd font attribute should be used for the foreground colour

	inputBoolean->set_enabled(true);
	EXPECT_TRUE(inputBoolean->get_enabled());
	EXPECT_TRUE(inputBoolean->set_attribute(static_cast<std::uint8_t>(InputBoolean::AttributeName::Enabled), 0, error));
	EXPECT_FALSE(inputBoolean->get_enabled());

	EXPECT_TRUE(inputBoolean->set_attribute(static_cast<std::uint8_t>(InputBoolean::AttributeName::VariableReference), 0xFFFF, error));

	inputBoolean->set_id(100);
	objects[inputBoolean->get_id()] = inputBoolean;

	// Add a valid child object, a number variable
	auto numberVariable = std::make_shared<NumberVariable>(objects, colourTable);
	numberVariable->set_id(200);
	objects[numberVariable->get_id()] = numberVariable;
	inputBoolean->add_child(numberVariable->get_id(), 0, 0);
	EXPECT_TRUE(inputBoolean->get_is_valid());

	// Add an invalid child, a container
	auto container = std::make_shared<Container>(objects, colourTable);
	container->set_id(300);
	objects[container->get_id()] = container;
	inputBoolean->add_child(container->get_id(), 0, 0);
	EXPECT_FALSE(inputBoolean->get_is_valid());

	uint32_t testValue = 0;
	EXPECT_TRUE(inputBoolean->get_attribute(static_cast<std::uint8_t>(InputBoolean::AttributeName::BackgroundColour), testValue));
	EXPECT_EQ(testValue, 20);

	EXPECT_TRUE(inputBoolean->get_attribute(static_cast<std::uint8_t>(InputBoolean::AttributeName::ForegroundColour), testValue));
	EXPECT_EQ(testValue, fontAttribute2->get_id());

	EXPECT_TRUE(inputBoolean->get_attribute(static_cast<std::uint8_t>(InputBoolean::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(inputBoolean->get_attribute(static_cast<std::uint8_t>(InputBoolean::AttributeName::Value), testValue));
	EXPECT_EQ(testValue, 1);

	EXPECT_TRUE(inputBoolean->get_attribute(static_cast<std::uint8_t>(InputBoolean::AttributeName::Enabled), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(inputBoolean->get_attribute(static_cast<std::uint8_t>(InputBoolean::AttributeName::VariableReference), testValue));
	EXPECT_EQ(testValue, 200);

	EXPECT_TRUE(inputBoolean->get_attribute(static_cast<std::uint8_t>(InputBoolean::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::InputBoolean));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, InputStringTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto inputString = std::make_shared<InputString>(objects, colourTable);

	run_baseline_tests(inputString.get());
	EXPECT_EQ(inputString->get_object_type(), VirtualTerminalObjectType::InputString);

	// Test input string background colour
	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;
	inputString->set_background_color(10);
	EXPECT_EQ(inputString->get_background_color(), 10);
	EXPECT_TRUE(inputString->set_attribute(static_cast<std::uint8_t>(InputString::AttributeName::BackgroundColour), 20, error));
	EXPECT_EQ(inputString->get_background_color(), 20);

	// Setting the type attribute should always fail
	EXPECT_FALSE(inputString->set_attribute(static_cast<std::uint8_t>(InputString::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(inputString->set_attribute(static_cast<std::uint8_t>(InputString::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test input string width attribute
	EXPECT_TRUE(inputString->set_attribute(static_cast<std::uint8_t>(InputString::AttributeName::Width), 50, error));
	EXPECT_EQ(inputString->get_width(), 50);

	// Test input string height attribute
	EXPECT_TRUE(inputString->set_attribute(static_cast<std::uint8_t>(InputString::AttributeName::Height), 50, error));
	EXPECT_EQ(inputString->get_height(), 50);

	// Test enabled attribute
	inputString->set_enabled(true);
	EXPECT_TRUE(inputString->get_enabled());
	inputString->set_enabled(false);
	EXPECT_FALSE(inputString->get_enabled());
	EXPECT_TRUE(inputString->set_attribute(static_cast<std::uint8_t>(InputString::AttributeName::Enabled), 1, error));
	EXPECT_TRUE(inputString->get_enabled());

	// Test one of the option bits
	inputString->set_option(InputString::Options::AutoWrap, true);
	EXPECT_TRUE(inputString->get_option(InputString::Options::AutoWrap));
	inputString->set_option(InputString::Options::AutoWrap, false);
	EXPECT_FALSE(inputString->get_option(InputString::Options::AutoWrap));

	// Test the value
	inputString->set_value("Test");
	EXPECT_EQ(inputString->get_value(), "Test");

	// Test input string font attribute
	auto fontAttribute = std::make_shared<FontAttributes>(objects, colourTable);
	fontAttribute->set_id(1); // Arbitrary
	objects[fontAttribute->get_id()] = fontAttribute;

	// Test input string input attributes
	auto inputAttribute = std::make_shared<InputAttributes>(objects, colourTable);
	inputAttribute->set_id(5); // Arbitrary
	objects[inputAttribute->get_id()] = inputAttribute;

	// Add it as a child object
	inputString->add_child(fontAttribute->get_id(), 0, 0);

	// Now lets replace it with a different font attributes object using set_attribute
	auto fontAttribute2 = std::make_shared<FontAttributes>(objects, colourTable);
	fontAttribute2->set_id(2); // Arbitrary
	objects[fontAttribute2->get_id()] = fontAttribute2;

	EXPECT_TRUE(inputString->set_attribute(static_cast<std::uint8_t>(InputString::AttributeName::FontAttributes), fontAttribute2->get_id(), error));
	EXPECT_TRUE(inputString->set_attribute(static_cast<std::uint8_t>(InputString::AttributeName::InputAttributes), inputAttribute->get_id(), error));

	inputString->set_justification_bitfield(static_cast<std::uint8_t>(InputString::HorizontalJustification::PositionMiddle) | (static_cast<std::uint8_t>(InputString::VerticalJustification::PositionBottom) << 2));
	EXPECT_EQ(inputString->get_horizontal_justification(), InputString::HorizontalJustification::PositionMiddle);
	EXPECT_EQ(inputString->get_vertical_justification(), InputString::VerticalJustification::PositionBottom);
	EXPECT_TRUE(inputString->set_attribute(static_cast<std::uint8_t>(InputString::AttributeName::Justification), 0, error));
	EXPECT_EQ(inputString->get_horizontal_justification(), InputString::HorizontalJustification::PositionLeft);
	EXPECT_EQ(inputString->get_vertical_justification(), InputString::VerticalJustification::PositionTop);

	EXPECT_TRUE(inputString->set_attribute(static_cast<std::uint8_t>(InputString::AttributeName::VariableReference), 0xFFFF, error));
	EXPECT_TRUE(inputString->set_attribute(static_cast<std::uint8_t>(InputString::AttributeName::Options), 1, error));

	EXPECT_TRUE(inputString->get_option(InputString::Options::Transparent));

	inputString->set_id(100);
	objects[inputString->get_id()] = inputString;
	EXPECT_TRUE(inputString->get_is_valid());

	// Add an invalid object, a picture graphic
	auto pictureGraphic = std::make_shared<PictureGraphic>(objects, colourTable);
	pictureGraphic->set_id(200);
	objects[pictureGraphic->get_id()] = pictureGraphic;
	inputString->add_child(pictureGraphic->get_id(), 0, 0);
	EXPECT_FALSE(inputString->get_is_valid());

	uint32_t testValue = 0;
	EXPECT_TRUE(inputString->get_attribute(static_cast<std::uint8_t>(InputString::AttributeName::BackgroundColour), testValue));
	EXPECT_EQ(testValue, 20);

	EXPECT_TRUE(inputString->get_attribute(static_cast<std::uint8_t>(InputString::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(inputString->get_attribute(static_cast<std::uint8_t>(InputString::AttributeName::Height), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(inputString->get_attribute(static_cast<std::uint8_t>(InputString::AttributeName::Enabled), testValue));
	EXPECT_EQ(testValue, 1);

	EXPECT_TRUE(inputString->get_attribute(static_cast<std::uint8_t>(InputString::AttributeName::FontAttributes), testValue));
	EXPECT_EQ(testValue, fontAttribute2->get_id());

	EXPECT_TRUE(inputString->get_attribute(static_cast<std::uint8_t>(InputString::AttributeName::InputAttributes), testValue));
	EXPECT_EQ(testValue, inputAttribute->get_id());

	EXPECT_TRUE(inputString->get_attribute(static_cast<std::uint8_t>(InputString::AttributeName::Justification), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(inputString->get_attribute(static_cast<std::uint8_t>(InputString::AttributeName::VariableReference), testValue));
	EXPECT_EQ(testValue, 0xFFFF);

	EXPECT_TRUE(inputString->get_attribute(static_cast<std::uint8_t>(InputString::AttributeName::Options), testValue));
	EXPECT_EQ(testValue, 1);

	EXPECT_TRUE(inputString->get_attribute(static_cast<std::uint8_t>(InputString::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::InputString));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, InputNumberTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto inputNumber = std::make_shared<InputNumber>(objects, colourTable);

	run_baseline_tests(inputNumber.get());
	EXPECT_EQ(inputNumber->get_object_type(), VirtualTerminalObjectType::InputNumber);

	// Test input number background colour attribute
	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;
	inputNumber->set_background_color(10);
	EXPECT_EQ(inputNumber->get_background_color(), 10);
	EXPECT_TRUE(inputNumber->set_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::BackgroundColour), 20, error));
	EXPECT_EQ(inputNumber->get_background_color(), 20);

	// Setting the type attribute should always fail
	EXPECT_FALSE(inputNumber->set_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(inputNumber->set_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test input number width attribute
	EXPECT_TRUE(inputNumber->set_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Width), 50, error));
	EXPECT_EQ(inputNumber->get_width(), 50);

	// Test input number height attribute
	EXPECT_TRUE(inputNumber->set_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Height), 50, error));
	EXPECT_EQ(inputNumber->get_height(), 50);

	// Test min/max attribute
	inputNumber->set_maximum_value(5000);
	EXPECT_EQ(inputNumber->get_maximum_value(), 5000);
	inputNumber->set_minimum_value(2000);
	EXPECT_EQ(inputNumber->get_minimum_value(), 2000);

	EXPECT_TRUE(inputNumber->set_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::MaxValue), 6000, error));
	EXPECT_EQ(inputNumber->get_maximum_value(), 6000);

	EXPECT_TRUE(inputNumber->set_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::MinValue), 1000, error));
	EXPECT_EQ(inputNumber->get_minimum_value(), 1000);

	inputNumber->set_value(8000);
	EXPECT_EQ(inputNumber->get_value(), 8000);

	inputNumber->set_scale(4.0f);
	EXPECT_NEAR(inputNumber->get_scale(), 4.0f, 0.0001f);
	EXPECT_TRUE(inputNumber->set_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Scale), 0, error));
	EXPECT_NEAR(inputNumber->get_scale(), 0.0f, 0.0001f);

	inputNumber->set_number_of_decimals(2);
	EXPECT_EQ(inputNumber->get_number_of_decimals(), 2);
	EXPECT_TRUE(inputNumber->set_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::NumberOfDecimals), 0, error));
	EXPECT_EQ(inputNumber->get_number_of_decimals(), 0);

	inputNumber->set_format(true);
	EXPECT_TRUE(inputNumber->get_format());
	EXPECT_TRUE(inputNumber->set_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Format), 0, error));
	EXPECT_FALSE(inputNumber->get_format());

	inputNumber->set_offset(-1234);
	EXPECT_EQ(inputNumber->get_offset(), -1234);
	EXPECT_TRUE(inputNumber->set_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Offset), 567, error));
	EXPECT_EQ(inputNumber->get_offset(), 567);

	inputNumber->set_justification_bitfield(static_cast<std::uint8_t>(InputNumber::HorizontalJustification::PositionMiddle) | (static_cast<std::uint8_t>(InputNumber::VerticalJustification::PositionBottom) << 2));
	EXPECT_EQ(inputNumber->get_horizontal_justification(), InputNumber::HorizontalJustification::PositionMiddle);
	EXPECT_EQ(inputNumber->get_vertical_justification(), InputNumber::VerticalJustification::PositionBottom);

	inputNumber->set_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::VariableReference), 0xFFFF, error);

	inputNumber->set_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Justification), 0, error);
	EXPECT_EQ(inputNumber->get_horizontal_justification(), InputNumber::HorizontalJustification::PositionLeft);
	EXPECT_EQ(inputNumber->get_vertical_justification(), InputNumber::VerticalJustification::PositionTop);

	// Test some of the option bits
	inputNumber->set_option(InputNumber::Options::DisplayLeadingZeros, true);
	EXPECT_TRUE(inputNumber->get_option(InputNumber::Options::DisplayLeadingZeros));
	inputNumber->set_option(InputNumber::Options::DisplayZeroAsBlank, true);
	EXPECT_TRUE(inputNumber->get_option(InputNumber::Options::DisplayZeroAsBlank));
	inputNumber->set_option(InputNumber::Options::DisplayLeadingZeros, false);
	EXPECT_FALSE(inputNumber->get_option(InputNumber::Options::DisplayLeadingZeros));
	inputNumber->set_option(InputNumber::Options::DisplayZeroAsBlank, false);
	EXPECT_FALSE(inputNumber->get_option(InputNumber::Options::DisplayZeroAsBlank));

	EXPECT_TRUE(inputNumber->set_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Options), 4, error));
	EXPECT_TRUE(inputNumber->get_option(InputNumber::Options::DisplayZeroAsBlank));

	// Test Options2
	inputNumber->set_option2(InputNumber::Options2::RealTimeEditing, true);
	EXPECT_TRUE(inputNumber->get_option2(InputNumber::Options2::RealTimeEditing));
	inputNumber->set_options2(1);
	EXPECT_TRUE(inputNumber->get_option2(InputNumber::Options2::Enabled));
	inputNumber->set_option2(InputNumber::Options2::Enabled, false);
	EXPECT_FALSE(inputNumber->get_option2(InputNumber::Options2::Enabled));

	// Test input number font attribute
	auto fontAttribute = std::make_shared<FontAttributes>(objects, colourTable);
	fontAttribute->set_id(1); // Arbitrary
	objects[fontAttribute->get_id()] = fontAttribute;

	// Add it as a child object
	inputNumber->add_child(fontAttribute->get_id(), 0, 0);

	// Now lets replace it with a different font attributes object using set_attribute
	auto fontAttribute2 = std::make_shared<FontAttributes>(objects, colourTable);
	fontAttribute2->set_id(2); // Arbitrary
	objects[fontAttribute2->get_id()] = fontAttribute2;
	EXPECT_TRUE(inputNumber->set_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::FontAttributes), fontAttribute2->get_id(), error));

	inputNumber->set_id(100);
	objects[inputNumber->get_id()] = inputNumber;

	EXPECT_TRUE(inputNumber->get_is_valid());

	// Add an invalid object, a FillAttributes object
	auto fillAttributes = std::make_shared<FillAttributes>(objects, colourTable);
	fillAttributes->set_id(200);
	objects[fillAttributes->get_id()] = fillAttributes;
	inputNumber->add_child(fillAttributes->get_id(), 0, 0);
	EXPECT_FALSE(inputNumber->get_is_valid());

	uint32_t testValue = 0;
	EXPECT_TRUE(inputNumber->get_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::BackgroundColour), testValue));
	EXPECT_EQ(testValue, 20);

	EXPECT_TRUE(inputNumber->get_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(inputNumber->get_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Height), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(inputNumber->get_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::MaxValue), testValue));
	EXPECT_EQ(testValue, 6000);

	EXPECT_TRUE(inputNumber->get_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::MinValue), testValue));
	EXPECT_EQ(testValue, 1000);

	EXPECT_TRUE(inputNumber->get_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Value), testValue));
	EXPECT_EQ(testValue, 8000);

	EXPECT_TRUE(inputNumber->get_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Scale), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(inputNumber->get_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::NumberOfDecimals), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(inputNumber->get_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Format), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(inputNumber->get_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Offset), testValue));
	EXPECT_EQ(testValue, 567);

	EXPECT_TRUE(inputNumber->get_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Justification), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(inputNumber->get_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::VariableReference), testValue));
	EXPECT_EQ(testValue, 0xFFFF);

	EXPECT_TRUE(inputNumber->get_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Options), testValue));
	EXPECT_EQ(testValue, 4);

	EXPECT_TRUE(inputNumber->get_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Options2), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(inputNumber->get_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::FontAttributes), testValue));
	EXPECT_EQ(testValue, fontAttribute2->get_id());

	EXPECT_TRUE(inputNumber->get_attribute(static_cast<std::uint8_t>(InputNumber::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::InputNumber));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, InputListTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto inputList = std::make_shared<InputList>(objects, colourTable);

	run_baseline_tests(inputList.get());
	EXPECT_EQ(inputList->get_object_type(), VirtualTerminalObjectType::InputList);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	// Setting the type attribute should always fail
	EXPECT_FALSE(inputList->set_attribute(static_cast<std::uint8_t>(InputList::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(inputList->set_attribute(static_cast<std::uint8_t>(InputList::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test input list width attribute
	EXPECT_TRUE(inputList->set_attribute(static_cast<std::uint8_t>(InputList::AttributeName::Width), 50, error));
	EXPECT_EQ(inputList->get_width(), 50);

	// Test input list height attribute
	EXPECT_TRUE(inputList->set_attribute(static_cast<std::uint8_t>(InputList::AttributeName::Height), 50, error));
	EXPECT_EQ(inputList->get_height(), 50);

	// Test input list value attribute
	EXPECT_TRUE(inputList->set_attribute(static_cast<std::uint8_t>(InputList::AttributeName::Value), 4, error));
	EXPECT_EQ(inputList->get_value(), 4);

	// Test input list variable reference attribute
	EXPECT_TRUE(inputList->set_attribute(static_cast<std::uint8_t>(InputList::AttributeName::VariableReference), 0xFFFF, error));
	EXPECT_EQ(inputList->get_variable_reference(), 0xFFFF);

	// Test options attribute
	inputList->set_option(InputList::Options::RealTimeEditing, true);
	EXPECT_TRUE(inputList->get_option(InputList::Options::RealTimeEditing));
	inputList->set_option(InputList::Options::RealTimeEditing, false);
	EXPECT_FALSE(inputList->get_option(InputList::Options::RealTimeEditing));

	EXPECT_TRUE(inputList->set_attribute(static_cast<std::uint8_t>(InputList::AttributeName::Options), 1, error));
	EXPECT_TRUE(inputList->get_option(InputList::Options::Enabled));

	// Test value
	inputList->set_value(6);
	EXPECT_EQ(inputList->get_value(), 6);
	inputList->set_value(4);
	EXPECT_EQ(inputList->get_value(), 4);

	inputList->set_variable_reference(456);
	EXPECT_EQ(inputList->get_variable_reference(), 456);
	inputList->set_variable_reference(386);
	EXPECT_EQ(inputList->get_variable_reference(), 386);

	inputList->set_id(100);
	objects[inputList->get_id()] = inputList;

	// Add a valid child object, an output string
	auto outputString = std::make_shared<OutputString>(objects, colourTable);
	outputString->set_id(200);
	objects[outputString->get_id()] = outputString;
	inputList->add_child(outputString->get_id(), 0, 0);
	EXPECT_TRUE(inputList->get_is_valid());

	inputList->set_number_of_list_items(1);
	EXPECT_EQ(1, inputList->get_number_of_list_items());

	// Test changing the child to be object id 0xFFFF (the null id)
	EXPECT_TRUE(inputList->change_list_item(0, 0xFFFF));

	// Add an invalid object, a Soft Key Mask
	auto softKeyMask = std::make_shared<SoftKeyMask>(objects, colourTable);
	softKeyMask->set_id(300);
	objects[softKeyMask->get_id()] = softKeyMask;
	inputList->add_child(softKeyMask->get_id(), 0, 0);
	EXPECT_FALSE(inputList->get_is_valid());

	uint32_t testValue = 0;
	EXPECT_TRUE(inputList->get_attribute(static_cast<std::uint8_t>(InputList::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(inputList->get_attribute(static_cast<std::uint8_t>(InputList::AttributeName::Height), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(inputList->get_attribute(static_cast<std::uint8_t>(InputList::AttributeName::Value), testValue));
	EXPECT_EQ(testValue, 4);

	EXPECT_TRUE(inputList->get_attribute(static_cast<std::uint8_t>(InputList::AttributeName::VariableReference), testValue));
	EXPECT_EQ(testValue, 386);

	EXPECT_TRUE(inputList->get_attribute(static_cast<std::uint8_t>(InputList::AttributeName::Options), testValue));
	EXPECT_EQ(testValue, 1);

	EXPECT_TRUE(inputList->get_attribute(static_cast<std::uint8_t>(InputList::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::InputList));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, OutputStringTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto outputString = std::make_shared<OutputString>(objects, colourTable);

	run_baseline_tests(outputString.get());
	EXPECT_EQ(outputString->get_object_type(), VirtualTerminalObjectType::OutputString);

	// Test output string background colour attribute
	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;
	outputString->set_background_color(10);
	EXPECT_EQ(outputString->get_background_color(), 10);
	EXPECT_TRUE(outputString->set_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::BackgroundColour), 20, error));
	EXPECT_EQ(outputString->get_background_color(), 20);

	// Setting the type attribute should always fail
	EXPECT_FALSE(outputString->set_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(outputString->set_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test output string width attribute
	EXPECT_TRUE(outputString->set_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::Width), 50, error));
	EXPECT_EQ(outputString->get_width(), 50);

	// Test output string height attribute
	EXPECT_TRUE(outputString->set_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::Height), 50, error));
	EXPECT_EQ(outputString->get_height(), 50);

	// Test value
	outputString->set_value("Test");
	EXPECT_EQ(outputString->get_value(), "Test");

	// Test options attribute
	outputString->set_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::Options), 1, error);
	EXPECT_TRUE(outputString->get_option(OutputString::Options::Transparent));
	outputString->set_option(OutputString::Options::Transparent, false);
	EXPECT_FALSE(outputString->get_option(OutputString::Options::Transparent));
	outputString->set_option(OutputString::Options::Transparent, true);
	EXPECT_TRUE(outputString->get_option(OutputString::Options::Transparent));

	// Test variable reference
	EXPECT_TRUE(outputString->set_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::VariableReference), 0xFFFF, error));

	// Test output string font attribute
	auto fontAttribute = std::make_shared<FontAttributes>(objects, colourTable);
	fontAttribute->set_id(1); // Arbitrary
	objects[fontAttribute->get_id()] = fontAttribute;

	// Add it as a child object
	outputString->add_child(fontAttribute->get_id(), 0, 0);

	// Now lets replace it with a different font attributes object using set_attribute
	auto fontAttribute2 = std::make_shared<FontAttributes>(objects, colourTable);
	fontAttribute2->set_id(2); // Arbitrary
	objects[fontAttribute2->get_id()] = fontAttribute2;
	EXPECT_TRUE(outputString->set_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::FontAttributes), fontAttribute2->get_id(), error));

	// Test output string justification attribute
	outputString->set_justification_bitfield(static_cast<std::uint8_t>(OutputString::HorizontalJustification::PositionMiddle) | (static_cast<std::uint8_t>(OutputString::VerticalJustification::PositionBottom) << 2));
	EXPECT_EQ(outputString->get_horizontal_justification(), OutputString::HorizontalJustification::PositionMiddle);
	EXPECT_EQ(outputString->get_vertical_justification(), OutputString::VerticalJustification::PositionBottom);
	EXPECT_TRUE(outputString->set_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::Justification), 0, error));
	EXPECT_EQ(outputString->get_horizontal_justification(), OutputString::HorizontalJustification::PositionLeft);
	EXPECT_EQ(outputString->get_vertical_justification(), OutputString::VerticalJustification::PositionTop);

	outputString->set_id(100);
	objects[outputString->get_id()] = outputString;

	EXPECT_EQ(outputString->get_is_valid(), true);

	// Add an invalid child, an Input String
	auto inputString = std::make_shared<InputString>(objects, colourTable);
	inputString->set_id(200);
	objects[inputString->get_id()] = inputString;
	outputString->add_child(inputString->get_id(), 0, 0);
	EXPECT_FALSE(outputString->get_is_valid());

	uint32_t testValue = 0;
	EXPECT_TRUE(outputString->get_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::BackgroundColour), testValue));
	EXPECT_EQ(testValue, 20);

	EXPECT_TRUE(outputString->get_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputString->get_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::Height), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputString->get_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::Options), testValue));
	EXPECT_EQ(testValue, 1);

	EXPECT_TRUE(outputString->get_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::VariableReference), testValue));
	EXPECT_EQ(testValue, 0xFFFF);

	EXPECT_TRUE(outputString->get_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::Justification), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(outputString->get_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::FontAttributes), testValue));
	EXPECT_EQ(testValue, fontAttribute2->get_id());

	EXPECT_TRUE(outputString->get_attribute(static_cast<std::uint8_t>(OutputString::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::OutputString));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, OutputNumberTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto outputNumber = std::make_shared<OutputNumber>(objects, colourTable);

	run_baseline_tests(outputNumber.get());
	EXPECT_EQ(outputNumber->get_object_type(), VirtualTerminalObjectType::OutputNumber);

	// Test output number background colour attribute
	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;
	outputNumber->set_background_color(10);
	EXPECT_EQ(outputNumber->get_background_color(), 10);
	EXPECT_TRUE(outputNumber->set_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::BackgroundColour), 20, error));
	EXPECT_EQ(outputNumber->get_background_color(), 20);

	// Setting the type attribute should always fail
	EXPECT_FALSE(outputNumber->set_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(outputNumber->set_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test output number width attribute
	EXPECT_TRUE(outputNumber->set_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::Width), 50, error));
	EXPECT_EQ(outputNumber->get_width(), 50);

	// Test output number height attribute
	EXPECT_TRUE(outputNumber->set_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::Height), 50, error));
	EXPECT_EQ(outputNumber->get_height(), 50);

	// Test output number offset attribute
	EXPECT_TRUE(outputNumber->set_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::Offset), 99, error));
	EXPECT_EQ(outputNumber->get_offset(), 99);

	// Test output number decimals attribute
	EXPECT_TRUE(outputNumber->set_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::NumberOfDecimals), 4, error));
	EXPECT_EQ(outputNumber->get_number_of_decimals(), 4);

	// Test output number format attribute
	EXPECT_TRUE(outputNumber->set_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::Format), 1, error));
	EXPECT_EQ(outputNumber->get_format(), true);

	// Test output number options attribute
	EXPECT_TRUE(outputNumber->set_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::Options), 1, error));
	EXPECT_TRUE(outputNumber->get_option(OutputNumber::Options::Transparent));
	outputNumber->set_option(OutputNumber::Options::Transparent, false);
	EXPECT_FALSE(outputNumber->get_option(OutputNumber::Options::Transparent));
	outputNumber->set_option(OutputNumber::Options::Transparent, true);
	EXPECT_TRUE(outputNumber->get_option(OutputNumber::Options::Transparent));

	// Test output number font attribute
	auto fontAttribute = std::make_shared<FontAttributes>(objects, colourTable);
	fontAttribute->set_id(1); // Arbitrary
	objects[fontAttribute->get_id()] = fontAttribute;

	// Add it as a child object
	outputNumber->add_child(fontAttribute->get_id(), 0, 0);

	// Now lets replace it with a different font attributes object using set_attribute
	auto fontAttribute2 = std::make_shared<FontAttributes>(objects, colourTable);
	fontAttribute2->set_id(2); // Arbitrary
	objects[fontAttribute2->get_id()] = fontAttribute2;
	EXPECT_TRUE(outputNumber->set_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::FontAttributes), fontAttribute2->get_id(), error));

	// Test output number justification attribute
	outputNumber->set_justification_bitfield(static_cast<std::uint8_t>(OutputNumber::HorizontalJustification::PositionMiddle) | (static_cast<std::uint8_t>(OutputNumber::VerticalJustification::PositionBottom) << 2));
	EXPECT_EQ(outputNumber->get_horizontal_justification(), OutputNumber::HorizontalJustification::PositionMiddle);
	EXPECT_EQ(outputNumber->get_vertical_justification(), OutputNumber::VerticalJustification::PositionBottom);
	EXPECT_TRUE(outputNumber->set_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::Justification), 0, error));
	EXPECT_EQ(outputNumber->get_horizontal_justification(), OutputNumber::HorizontalJustification::PositionLeft);
	EXPECT_EQ(outputNumber->get_vertical_justification(), OutputNumber::VerticalJustification::PositionTop);
	EXPECT_TRUE(outputNumber->set_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::Justification), static_cast<std::uint32_t>(OutputNumber::HorizontalJustification::PositionMiddle) | (static_cast<std::uint32_t>(OutputNumber::VerticalJustification::PositionBottom) << 2), error));
	EXPECT_EQ(outputNumber->get_horizontal_justification(), OutputNumber::HorizontalJustification::PositionMiddle);
	EXPECT_EQ(outputNumber->get_vertical_justification(), OutputNumber::VerticalJustification::PositionBottom);

	// Test format
	outputNumber->set_format(true);
	EXPECT_TRUE(outputNumber->get_format());
	outputNumber->set_format(false);
	EXPECT_FALSE(outputNumber->get_format());

	// Test scale
	outputNumber->set_scale(4.0f);
	EXPECT_NEAR(outputNumber->get_scale(), 4.0f, 0.0001f);

	// Test value
	outputNumber->set_value(6);
	EXPECT_EQ(outputNumber->get_value(), 6);

	outputNumber->set_id(100);
	objects[outputNumber->get_id()] = outputNumber;

	EXPECT_TRUE(outputNumber->get_is_valid());

	// Add an invalid child, an Input Attributes
	auto inputAttributes = std::make_shared<InputAttributes>(objects, colourTable);
	inputAttributes->set_id(200);
	objects[inputAttributes->get_id()] = inputAttributes;
	outputNumber->add_child(inputAttributes->get_id(), 0, 0);
	EXPECT_FALSE(outputNumber->get_is_valid());

	uint32_t testValue = 0;
	EXPECT_TRUE(outputNumber->get_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::BackgroundColour), testValue));
	EXPECT_EQ(testValue, 20);

	EXPECT_TRUE(outputNumber->get_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputNumber->get_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::Height), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputNumber->get_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::Offset), testValue));
	EXPECT_EQ(testValue, 99);

	EXPECT_TRUE(outputNumber->get_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::NumberOfDecimals), testValue));
	EXPECT_EQ(testValue, 4);

	EXPECT_TRUE(outputNumber->get_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::Format), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(outputNumber->get_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::Options), testValue));
	EXPECT_EQ(testValue, 1);

	EXPECT_TRUE(outputNumber->get_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::Scale), testValue));
	EXPECT_NE(testValue, 0);

	EXPECT_TRUE(outputNumber->get_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::Justification), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(OutputNumber::HorizontalJustification::PositionMiddle) | (static_cast<std::uint8_t>(OutputNumber::VerticalJustification::PositionBottom) << 2));

	EXPECT_TRUE(outputNumber->get_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::FontAttributes), testValue));
	EXPECT_EQ(testValue, fontAttribute2->get_id());

	EXPECT_TRUE(outputNumber->get_attribute(static_cast<std::uint8_t>(OutputNumber::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::OutputNumber));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, OutputListTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto outputList = std::make_shared<OutputList>(objects, colourTable);

	run_baseline_tests(outputList.get());
	EXPECT_EQ(outputList->get_object_type(), VirtualTerminalObjectType::OutputList);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	// Setting the type attribute should always fail
	EXPECT_FALSE(outputList->set_attribute(static_cast<std::uint8_t>(OutputList::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(outputList->set_attribute(static_cast<std::uint8_t>(OutputList::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test output list width attribute
	EXPECT_TRUE(outputList->set_attribute(static_cast<std::uint8_t>(OutputList::AttributeName::Width), 50, error));
	EXPECT_EQ(outputList->get_width(), 50);

	// Test output list height attribute
	EXPECT_TRUE(outputList->set_attribute(static_cast<std::uint8_t>(OutputList::AttributeName::Height), 50, error));
	EXPECT_EQ(outputList->get_height(), 50);

	EXPECT_EQ(outputList->get_number_of_list_items(), 0); // This is not the number of children!

	outputList->add_child(1, 0, 0);
	outputList->add_child(2, 0, 0);
	outputList->add_child(3, 0, 0);
	outputList->add_child(4, 0, 0);

	EXPECT_EQ(outputList->get_number_of_list_items(), 0); // This is not the number of children!
	EXPECT_EQ(outputList->get_number_children(), 4);

	outputList->set_value(6);
	EXPECT_EQ(outputList->get_value(), 6);
	outputList->set_value(4);
	EXPECT_EQ(outputList->get_value(), 4);

	EXPECT_TRUE(outputList->change_list_item(2, 0xFFFF));

	outputList->remove_child(1, 0, 0);
	outputList->remove_child(2, 0, 0); // In theory this is no longer present, but just in case
	outputList->remove_child(3, 0, 0);
	outputList->remove_child(4, 0, 0);
	outputList->remove_child(0xFFFF, 0, 0);

	// Test validity with some real objects
	outputList->set_id(100);
	objects[outputList->get_id()] = outputList;

	// Create 4 output strings
	auto outputString1 = std::make_shared<OutputString>(objects, colourTable);
	outputString1->set_id(1);
	objects[outputString1->get_id()] = outputString1;
	auto outputString2 = std::make_shared<OutputString>(objects, colourTable);
	outputString2->set_id(2);
	objects[outputString2->get_id()] = outputString2;
	auto outputString3 = std::make_shared<OutputString>(objects, colourTable);
	outputString3->set_id(3);
	objects[outputString3->get_id()] = outputString3;
	auto outputString4 = std::make_shared<OutputString>(objects, colourTable);
	outputString4->set_id(4);
	objects[outputString4->get_id()] = outputString4;

	// Add the valid children and test validity
	outputList->add_child(outputString1->get_id(), 0, 0);
	outputList->add_child(outputString2->get_id(), 0, 0);
	outputList->add_child(outputString3->get_id(), 0, 0);
	outputList->add_child(outputString4->get_id(), 0, 0);
	outputList->set_number_of_list_items(4);
	EXPECT_EQ(outputList->get_number_of_list_items(), 4);
	EXPECT_EQ(outputList->get_number_children(), 4);

	EXPECT_TRUE(outputList->get_is_valid());

	// Add an invalid obejct, a Data Mask object
	auto dataMask = std::make_shared<DataMask>(objects, colourTable);
	dataMask->set_id(200);
	objects[dataMask->get_id()] = dataMask;
	outputList->add_child(dataMask->get_id(), 0, 0);
	EXPECT_FALSE(outputList->get_is_valid());

	// Test variable reference attribute
	EXPECT_TRUE(outputList->set_attribute(static_cast<std::uint8_t>(OutputList::AttributeName::VariableReference), 0xFFFF, error));
	EXPECT_EQ(0xFFFF, outputList->get_variable_reference());
	outputList->set_variable_reference(1234);
	EXPECT_EQ(1234, outputList->get_variable_reference());
	outputList->set_variable_reference(0xFFFF);

	// Test value attribute
	EXPECT_TRUE(outputList->set_attribute(static_cast<std::uint8_t>(OutputList::AttributeName::Value), 4, error));
	EXPECT_EQ(outputList->get_value(), 4);

	uint32_t testValue = 0;
	EXPECT_TRUE(outputList->get_attribute(static_cast<std::uint8_t>(OutputList::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputList->get_attribute(static_cast<std::uint8_t>(OutputList::AttributeName::Height), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputList->get_attribute(static_cast<std::uint8_t>(OutputList::AttributeName::Value), testValue));
	EXPECT_EQ(testValue, 4);

	EXPECT_TRUE(outputList->get_attribute(static_cast<std::uint8_t>(OutputList::AttributeName::VariableReference), testValue));
	EXPECT_EQ(testValue, 0xFFFF);

	EXPECT_TRUE(outputList->get_attribute(static_cast<std::uint8_t>(OutputList::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::OutputList));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, OutputLineTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto outputLine = std::make_shared<OutputLine>(objects, colourTable);

	run_baseline_tests(outputLine.get());
	EXPECT_EQ(outputLine->get_object_type(), VirtualTerminalObjectType::OutputLine);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	// Setting the type attribute should always fail
	EXPECT_FALSE(outputLine->set_attribute(static_cast<std::uint8_t>(OutputLine::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(outputLine->set_attribute(static_cast<std::uint8_t>(OutputLine::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test output line width attribute
	EXPECT_TRUE(outputLine->set_attribute(static_cast<std::uint8_t>(OutputLine::AttributeName::Width), 50, error));
	EXPECT_EQ(outputLine->get_width(), 50);

	// Test output line height attribute
	EXPECT_TRUE(outputLine->set_attribute(static_cast<std::uint8_t>(OutputLine::AttributeName::Height), 50, error));
	EXPECT_EQ(outputLine->get_height(), 50);

	// Test Line Direction attribute
	EXPECT_TRUE(outputLine->set_attribute(static_cast<std::uint8_t>(OutputLine::AttributeName::LineDirection), 1, error));
	EXPECT_EQ(OutputLine::LineDirection::BottomLeftToTopRight, outputLine->get_line_direction());

	// Test output line line attribute
	auto lineAttribute = std::make_shared<LineAttributes>(objects, colourTable);
	lineAttribute->set_id(1); // Arbitrary
	objects[lineAttribute->get_id()] = lineAttribute;

	// Add it as a child object
	outputLine->add_child(lineAttribute->get_id(), 0, 0);

	// Now lets replace it with a different line attributes object using set_attribute
	auto lineAttribute2 = std::make_shared<LineAttributes>(objects, colourTable);
	lineAttribute2->set_id(2); // Arbitrary
	objects[lineAttribute2->get_id()] = lineAttribute2;
	EXPECT_TRUE(outputLine->set_attribute(static_cast<std::uint8_t>(OutputLine::AttributeName::LineAttributes), lineAttribute2->get_id(), error));

	ASSERT_NO_THROW(outputLine->get_child_id(0));
	EXPECT_EQ(outputLine->get_child_id(0), lineAttribute2->get_id()); // Now the 2nd line attribute should be used for the line attributes

	outputLine->set_line_direction(OutputLine::LineDirection::BottomLeftToTopRight);
	EXPECT_EQ(OutputLine::LineDirection::BottomLeftToTopRight, outputLine->get_line_direction());

	outputLine->set_id(100);
	objects[outputLine->get_id()] = outputLine;

	EXPECT_TRUE(outputLine->get_is_valid());

	// Add an invalid child object, an Input Attributes object
	auto inputAttributes = std::make_shared<InputAttributes>(objects, colourTable);
	inputAttributes->set_id(200);
	objects[inputAttributes->get_id()] = inputAttributes;
	outputLine->add_child(inputAttributes->get_id(), 0, 0);
	EXPECT_FALSE(outputLine->get_is_valid());

	uint32_t testValue = 0;
	EXPECT_TRUE(outputLine->get_attribute(static_cast<std::uint8_t>(OutputLine::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputLine->get_attribute(static_cast<std::uint8_t>(OutputLine::AttributeName::Height), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputLine->get_attribute(static_cast<std::uint8_t>(OutputLine::AttributeName::LineDirection), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(OutputLine::LineDirection::BottomLeftToTopRight));

	EXPECT_TRUE(outputLine->get_attribute(static_cast<std::uint8_t>(OutputLine::AttributeName::LineAttributes), testValue));
	EXPECT_EQ(testValue, lineAttribute2->get_id());

	EXPECT_TRUE(outputLine->get_attribute(static_cast<std::uint8_t>(OutputLine::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::OutputLine));

	EXPECT_TRUE(outputLine->get_attribute(static_cast<std::uint8_t>(OutputLine::AttributeName::LineAttributes), testValue));
	EXPECT_EQ(testValue, lineAttribute2->get_id());

	EXPECT_TRUE(outputLine->get_attribute(static_cast<std::uint8_t>(OutputLine::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::OutputLine));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, OutputRectangleTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto outputRectangle = std::make_shared<OutputRectangle>(objects, colourTable);

	run_baseline_tests(outputRectangle.get());
	EXPECT_EQ(outputRectangle->get_object_type(), VirtualTerminalObjectType::OutputRectangle);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	// Setting the type attribute should always fail
	EXPECT_FALSE(outputRectangle->set_attribute(static_cast<std::uint8_t>(OutputRectangle::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(outputRectangle->set_attribute(static_cast<std::uint8_t>(OutputRectangle::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test output rectangle width attribute
	EXPECT_TRUE(outputRectangle->set_attribute(static_cast<std::uint8_t>(OutputRectangle::AttributeName::Width), 50, error));
	EXPECT_EQ(outputRectangle->get_width(), 50);

	// Test output rectangle height attribute
	EXPECT_TRUE(outputRectangle->set_attribute(static_cast<std::uint8_t>(OutputRectangle::AttributeName::Height), 50, error));
	EXPECT_EQ(outputRectangle->get_height(), 50);

	// Test output rectangle line attribute
	auto lineAttribute = std::make_shared<LineAttributes>(objects, colourTable);
	lineAttribute->set_id(1); // Arbitrary
	objects[lineAttribute->get_id()] = lineAttribute;

	// Add it as a child object
	outputRectangle->add_child(lineAttribute->get_id(), 0, 0);

	// Now lets replace it with a different line attributes object using set_attribute
	auto lineAttribute2 = std::make_shared<LineAttributes>(objects, colourTable);
	lineAttribute2->set_id(2); // Arbitrary
	objects[lineAttribute2->get_id()] = lineAttribute2;
	EXPECT_TRUE(outputRectangle->set_attribute(static_cast<std::uint8_t>(OutputRectangle::AttributeName::LineAttributes), lineAttribute2->get_id(), error));
	ASSERT_NO_THROW(outputRectangle->get_child_id(0));
	EXPECT_EQ(outputRectangle->get_child_id(0), lineAttribute2->get_id()); // Now the 2nd line attribute should be used for the line attributes

	outputRectangle->set_line_suppression_bitfield(2);
	EXPECT_EQ(outputRectangle->get_line_suppression_bitfield(), 2);

	outputRectangle->add_child(1, 10, 10);
	outputRectangle->add_child(2, 20, 50);
	outputRectangle->offset_all_children_with_id(1, 5, 6);

	EXPECT_EQ(15, outputRectangle->get_child_x(1));
	EXPECT_EQ(16, outputRectangle->get_child_y(1));

	outputRectangle->set_id(100);
	objects[outputRectangle->get_id()] = outputRectangle;

	EXPECT_TRUE(outputRectangle->get_is_valid());

	// Add an invalid object, a Data Mask
	auto dataMask = std::make_shared<DataMask>(objects, colourTable);
	dataMask->set_id(200);
	objects[dataMask->get_id()] = dataMask;
	outputRectangle->add_child(dataMask->get_id(), 0, 0);
	EXPECT_FALSE(outputRectangle->get_is_valid());

	// Test line suppression
	EXPECT_TRUE(outputRectangle->set_attribute(static_cast<std::uint8_t>(OutputRectangle::AttributeName::LineSuppression), 1, error));
	EXPECT_EQ(outputRectangle->get_line_suppression_bitfield(), 1);

	// Test fill attributes
	EXPECT_TRUE(outputRectangle->set_attribute(static_cast<std::uint8_t>(OutputRectangle::AttributeName::FillAttributes), 0xFFFF, error));

	uint32_t testValue = 0;
	EXPECT_TRUE(outputRectangle->get_attribute(static_cast<std::uint8_t>(OutputRectangle::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputRectangle->get_attribute(static_cast<std::uint8_t>(OutputRectangle::AttributeName::Height), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputRectangle->get_attribute(static_cast<std::uint8_t>(OutputRectangle::AttributeName::LineAttributes), testValue));
	EXPECT_EQ(testValue, lineAttribute2->get_id());

	EXPECT_TRUE(outputRectangle->get_attribute(static_cast<std::uint8_t>(OutputRectangle::AttributeName::LineSuppression), testValue));
	EXPECT_EQ(testValue, 1);

	EXPECT_TRUE(outputRectangle->get_attribute(static_cast<std::uint8_t>(OutputRectangle::AttributeName::FillAttributes), testValue));
	EXPECT_EQ(testValue, 0xFFFF);

	EXPECT_TRUE(outputRectangle->get_attribute(static_cast<std::uint8_t>(OutputRectangle::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::OutputRectangle));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, OutputEllipseTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto outputEllipse = std::make_shared<OutputEllipse>(objects, colourTable);

	run_baseline_tests(outputEllipse.get());
	EXPECT_EQ(outputEllipse->get_object_type(), VirtualTerminalObjectType::OutputEllipse);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	// Setting the type attribute should always fail
	EXPECT_FALSE(outputEllipse->set_attribute(static_cast<std::uint8_t>(OutputEllipse::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(outputEllipse->set_attribute(static_cast<std::uint8_t>(OutputEllipse::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test output ellipse width attribute
	EXPECT_TRUE(outputEllipse->set_attribute(static_cast<std::uint8_t>(OutputEllipse::AttributeName::Width), 50, error));
	EXPECT_EQ(outputEllipse->get_width(), 50);

	// Test output ellipse height attribute
	EXPECT_TRUE(outputEllipse->set_attribute(static_cast<std::uint8_t>(OutputEllipse::AttributeName::Height), 50, error));
	EXPECT_EQ(outputEllipse->get_height(), 50);

	outputEllipse->set_start_angle(180);
	EXPECT_EQ(outputEllipse->get_start_angle(), 180);
	outputEllipse->set_start_angle(90);
	EXPECT_EQ(outputEllipse->get_start_angle(), 90);

	outputEllipse->set_end_angle(180);
	EXPECT_EQ(outputEllipse->get_end_angle(), 180);
	outputEllipse->set_end_angle(90);
	EXPECT_EQ(outputEllipse->get_end_angle(), 90);

	outputEllipse->set_attribute(static_cast<std::uint8_t>(OutputEllipse::AttributeName::StartAngle), 35, error);
	EXPECT_EQ(outputEllipse->get_start_angle(), 35);
	outputEllipse->set_attribute(static_cast<std::uint8_t>(OutputEllipse::AttributeName::EndAngle), 45, error);
	EXPECT_EQ(outputEllipse->get_end_angle(), 45);

	// Test output ellipse line attribute
	auto lineAttribute = std::make_shared<LineAttributes>(objects, colourTable);
	lineAttribute->set_id(1); // Arbitrary
	objects[lineAttribute->get_id()] = lineAttribute;

	// Add it as a child object
	outputEllipse->add_child(lineAttribute->get_id(), 0, 0);

	// Now lets replace it with a different line attributes object using set_attribute
	auto lineAttribute2 = std::make_shared<LineAttributes>(objects, colourTable);
	lineAttribute2->set_id(2); // Arbitrary
	objects[lineAttribute2->get_id()] = lineAttribute2;
	EXPECT_TRUE(outputEllipse->set_attribute(static_cast<std::uint8_t>(OutputEllipse::AttributeName::LineAttributes), lineAttribute2->get_id(), error));
	ASSERT_NO_THROW(outputEllipse->get_child_id(0));
	EXPECT_EQ(outputEllipse->get_child_id(0), lineAttribute2->get_id()); // Now the 2nd line attribute should be used for the line attributes

	// Test output ellipse fill attribute
	auto fillAttribute = std::make_shared<FillAttributes>(objects, colourTable);
	fillAttribute->set_id(3); // Arbitrary
	objects[fillAttribute->get_id()] = fillAttribute;

	// Add it as a child object
	outputEllipse->add_child(fillAttribute->get_id(), 0, 0);

	// Now lets replace it with a different fill attributes object using set_attribute
	auto fillAttribute2 = std::make_shared<FillAttributes>(objects, colourTable);
	fillAttribute2->set_id(4); // Arbitrary
	objects[fillAttribute2->get_id()] = fillAttribute2;
	EXPECT_TRUE(outputEllipse->set_attribute(static_cast<std::uint8_t>(OutputEllipse::AttributeName::FillAttributes), fillAttribute2->get_id(), error));
	ASSERT_NO_THROW(outputEllipse->get_child_id(1));
	EXPECT_EQ(outputEllipse->get_child_id(1), fillAttribute2->get_id()); // Now the 2nd fill attribute should be used for the line attributes

	outputEllipse->set_id(100);
	objects[outputEllipse->get_id()] = outputEllipse;

	EXPECT_TRUE(outputEllipse->get_is_valid());

	// Add an invalid object, an alarm mask
	auto alarmMask = std::make_shared<AlarmMask>(objects, colourTable);
	alarmMask->set_id(200);
	objects[alarmMask->get_id()] = alarmMask;
	outputEllipse->add_child(alarmMask->get_id(), 0, 0);
	EXPECT_FALSE(outputEllipse->get_is_valid());

	// Test ellipse type attribute
	EXPECT_TRUE(outputEllipse->set_attribute(static_cast<std::uint8_t>(OutputEllipse::AttributeName::EllipseType), 1, error));
	EXPECT_EQ(outputEllipse->get_ellipse_type(), OutputEllipse::EllipseType::OpenDefinedByStartEndAngles);
	outputEllipse->set_ellipse_type(OutputEllipse::EllipseType::Closed);
	EXPECT_EQ(outputEllipse->get_ellipse_type(), OutputEllipse::EllipseType::Closed);

	uint32_t testValue = 0;
	EXPECT_TRUE(outputEllipse->get_attribute(static_cast<std::uint8_t>(OutputEllipse::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputEllipse->get_attribute(static_cast<std::uint8_t>(OutputEllipse::AttributeName::Height), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputEllipse->get_attribute(static_cast<std::uint8_t>(OutputEllipse::AttributeName::StartAngle), testValue));
	EXPECT_EQ(testValue, 35);

	EXPECT_TRUE(outputEllipse->get_attribute(static_cast<std::uint8_t>(OutputEllipse::AttributeName::EndAngle), testValue));
	EXPECT_EQ(testValue, 45);

	EXPECT_TRUE(outputEllipse->get_attribute(static_cast<std::uint8_t>(OutputEllipse::AttributeName::LineAttributes), testValue));
	EXPECT_EQ(testValue, lineAttribute2->get_id());

	EXPECT_TRUE(outputEllipse->get_attribute(static_cast<std::uint8_t>(OutputEllipse::AttributeName::FillAttributes), testValue));
	EXPECT_EQ(testValue, fillAttribute2->get_id());

	EXPECT_TRUE(outputEllipse->get_attribute(static_cast<std::uint8_t>(OutputEllipse::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::OutputEllipse));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, OutputPolygonTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto outputPolygon = std::make_shared<OutputPolygon>(objects, colourTable);

	run_baseline_tests(outputPolygon.get());
	EXPECT_EQ(outputPolygon->get_object_type(), VirtualTerminalObjectType::OutputPolygon);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	// Setting the type attribute should always fail
	EXPECT_FALSE(outputPolygon->set_attribute(static_cast<std::uint8_t>(OutputPolygon::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(outputPolygon->set_attribute(static_cast<std::uint8_t>(OutputPolygon::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test output polygon width attribute
	EXPECT_TRUE(outputPolygon->set_attribute(static_cast<std::uint8_t>(OutputPolygon::AttributeName::Width), 50, error));
	EXPECT_EQ(outputPolygon->get_width(), 50);

	// Test output polygon height attribute
	EXPECT_TRUE(outputPolygon->set_attribute(static_cast<std::uint8_t>(OutputPolygon::AttributeName::Height), 50, error));
	EXPECT_EQ(outputPolygon->get_height(), 50);

	// Test output polygon line attribute
	auto lineAttribute = std::make_shared<LineAttributes>(objects, colourTable);
	lineAttribute->set_id(1); // Arbitrary
	objects[lineAttribute->get_id()] = lineAttribute;

	// Add it as a child object
	outputPolygon->add_child(lineAttribute->get_id(), 0, 0);

	// Now lets replace it with a different line attributes object using set_attribute
	auto lineAttribute2 = std::make_shared<LineAttributes>(objects, colourTable);
	lineAttribute2->set_id(2); // Arbitrary
	objects[lineAttribute2->get_id()] = lineAttribute2;
	EXPECT_TRUE(outputPolygon->set_attribute(static_cast<std::uint8_t>(OutputPolygon::AttributeName::LineAttributes), lineAttribute2->get_id(), error));
	ASSERT_NO_THROW(outputPolygon->get_child_id(0));
	EXPECT_EQ(outputPolygon->get_child_id(0), lineAttribute2->get_id()); // Now the 2nd line attribute should be used for the line attributes

	// Test output polygon fill attribute
	auto fillAttribute = std::make_shared<FillAttributes>(objects, colourTable);
	fillAttribute->set_id(3); // Arbitrary
	objects[fillAttribute->get_id()] = fillAttribute;

	// Add it as a child object
	outputPolygon->add_child(fillAttribute->get_id(), 0, 0);

	// Now lets replace it with a different fill attributes object using set_attribute
	auto fillAttribute2 = std::make_shared<FillAttributes>(objects, colourTable);
	fillAttribute2->set_id(4); // Arbitrary
	objects[fillAttribute2->get_id()] = fillAttribute2;
	EXPECT_TRUE(outputPolygon->set_attribute(static_cast<std::uint8_t>(OutputPolygon::AttributeName::FillAttributes), fillAttribute2->get_id(), error));
	ASSERT_NO_THROW(outputPolygon->get_child_id(1));
	EXPECT_EQ(outputPolygon->get_child_id(1), fillAttribute2->get_id()); // Now the 2nd fill attribute should be used for the line attributes

	outputPolygon->set_id(100);
	objects[outputPolygon->get_id()] = outputPolygon;

	EXPECT_TRUE(outputPolygon->get_is_valid());

	// Add an invalid object, an alarm mask
	auto alarmMask = std::make_shared<AlarmMask>(objects, colourTable);
	alarmMask->set_id(200);
	objects[alarmMask->get_id()] = alarmMask;
	outputPolygon->add_child(alarmMask->get_id(), 0, 0);
	EXPECT_FALSE(outputPolygon->get_is_valid());

	// Test points
	EXPECT_EQ(0, outputPolygon->get_number_of_points());

	uint32_t testValue = 0;
	EXPECT_TRUE(outputPolygon->get_attribute(static_cast<std::uint8_t>(OutputPolygon::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputPolygon->get_attribute(static_cast<std::uint8_t>(OutputPolygon::AttributeName::Height), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputPolygon->get_attribute(static_cast<std::uint8_t>(OutputPolygon::AttributeName::LineAttributes), testValue));
	EXPECT_EQ(testValue, lineAttribute2->get_id());

	EXPECT_TRUE(outputPolygon->get_attribute(static_cast<std::uint8_t>(OutputPolygon::AttributeName::FillAttributes), testValue));
	EXPECT_EQ(testValue, fillAttribute2->get_id());

	EXPECT_TRUE(outputPolygon->get_attribute(static_cast<std::uint8_t>(OutputPolygon::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::OutputPolygon));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, OutputMeterTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto outputMeter = std::make_shared<OutputMeter>(objects, colourTable);

	run_baseline_tests(outputMeter.get());
	EXPECT_EQ(outputMeter->get_object_type(), VirtualTerminalObjectType::OutputMeter);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	// Setting the type attribute should always fail
	EXPECT_FALSE(outputMeter->set_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(outputMeter->set_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test output meter width attribute
	EXPECT_TRUE(outputMeter->set_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::Width), 50, error));
	EXPECT_EQ(outputMeter->get_width(), 50);

	outputMeter->set_arc_and_tick_colour(40);
	EXPECT_EQ(outputMeter->get_arc_and_tick_colour(), 40);
	EXPECT_TRUE(outputMeter->set_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::ArcAndTickColour), 0, error));
	EXPECT_EQ(outputMeter->get_arc_and_tick_colour(), 0);

	outputMeter->set_border_colour(5);
	EXPECT_EQ(outputMeter->get_border_colour(), 5);
	EXPECT_TRUE(outputMeter->set_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::BorderColour), 0, error));
	EXPECT_EQ(outputMeter->get_border_colour(), 0);

	outputMeter->set_number_of_ticks(12);
	EXPECT_EQ(outputMeter->get_number_of_ticks(), 12);
	EXPECT_TRUE(outputMeter->set_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::NumberOfTicks), 0, error));
	EXPECT_EQ(outputMeter->get_number_of_ticks(), 0);

	outputMeter->set_start_angle(90);
	EXPECT_EQ(outputMeter->get_start_angle(), 90);

	outputMeter->set_end_angle(90);
	EXPECT_EQ(outputMeter->get_end_angle(), 90);

	outputMeter->set_needle_colour(6);
	EXPECT_EQ(outputMeter->get_needle_colour(), 6);
	EXPECT_TRUE(outputMeter->set_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::NeedleColour), 0, error));
	EXPECT_EQ(outputMeter->get_needle_colour(), 0);

	outputMeter->set_min_value(7);
	EXPECT_EQ(outputMeter->get_min_value(), 7);
	EXPECT_TRUE(outputMeter->set_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::MinValue), 11, error));
	EXPECT_EQ(outputMeter->get_min_value(), 11);

	outputMeter->set_max_value(8);
	EXPECT_EQ(outputMeter->get_max_value(), 8);
	EXPECT_TRUE(outputMeter->set_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::MaxValue), 12, error));
	EXPECT_EQ(outputMeter->get_max_value(), 12);

	outputMeter->set_value(9);
	EXPECT_EQ(outputMeter->get_value(), 9);

	outputMeter->set_option(OutputMeter::Options::DeflectionDirection, true);
	EXPECT_TRUE(outputMeter->get_option(OutputMeter::Options::DeflectionDirection));

	outputMeter->set_id(100);
	objects[outputMeter->get_id()] = outputMeter;

	EXPECT_TRUE(outputMeter->get_is_valid());

	// Add an invalid object, a container
	auto container = std::make_shared<Container>(objects, colourTable);
	container->set_id(200);
	objects[container->get_id()] = container;
	outputMeter->add_child(container->get_id(), 0, 0);
	EXPECT_FALSE(outputMeter->get_is_valid());

	uint32_t testValue = 0;
	EXPECT_TRUE(outputMeter->get_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputMeter->get_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::ArcAndTickColour), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(outputMeter->get_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::BorderColour), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(outputMeter->get_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::NumberOfTicks), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(outputMeter->get_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::StartAngle), testValue));
	EXPECT_EQ(testValue, 90);

	EXPECT_TRUE(outputMeter->get_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::EndAngle), testValue));
	EXPECT_EQ(testValue, 90);

	EXPECT_TRUE(outputMeter->get_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::NeedleColour), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(outputMeter->get_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::MinValue), testValue));
	EXPECT_EQ(testValue, 11);

	EXPECT_TRUE(outputMeter->get_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::MaxValue), testValue));
	EXPECT_EQ(testValue, 12);

	EXPECT_TRUE(outputMeter->get_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::OutputMeter));

	EXPECT_TRUE(outputMeter->get_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::Value), testValue));
	EXPECT_EQ(testValue, 9);

	EXPECT_TRUE(outputMeter->get_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::VariableReference), testValue));
	EXPECT_EQ(testValue, 0xFFFF);

	EXPECT_TRUE(outputMeter->get_attribute(static_cast<std::uint8_t>(OutputMeter::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::OutputMeter));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, OutputLinearBarGraphTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	OutputLinearBarGraph outputLinearBarGraph(objects, colourTable);

	run_baseline_tests(&outputLinearBarGraph);
	EXPECT_EQ(outputLinearBarGraph.get_object_type(), VirtualTerminalObjectType::OutputLinearBarGraph);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	// Setting the type attribute should always fail
	EXPECT_FALSE(outputLinearBarGraph.set_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(outputLinearBarGraph.set_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test output linear bar graph width attribute
	EXPECT_TRUE(outputLinearBarGraph.set_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::Width), 50, error));
	EXPECT_EQ(outputLinearBarGraph.get_width(), 50);

	outputLinearBarGraph.set_colour(9);
	EXPECT_EQ(outputLinearBarGraph.get_colour(), 9);
	EXPECT_TRUE(outputLinearBarGraph.set_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::Colour), 0, error));
	EXPECT_EQ(outputLinearBarGraph.get_colour(), 0);

	outputLinearBarGraph.set_max_value(65500);
	EXPECT_EQ(outputLinearBarGraph.get_max_value(), 65500);
	EXPECT_TRUE(outputLinearBarGraph.set_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::MaxValue), 12, error));
	EXPECT_EQ(outputLinearBarGraph.get_max_value(), 12);

	outputLinearBarGraph.set_number_of_ticks(12);
	EXPECT_EQ(outputLinearBarGraph.get_number_of_ticks(), 12);

	outputLinearBarGraph.set_min_value(3200);
	EXPECT_EQ(outputLinearBarGraph.get_min_value(), 3200);
	EXPECT_TRUE(outputLinearBarGraph.set_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::MinValue), 11, error));
	EXPECT_EQ(outputLinearBarGraph.get_min_value(), 11);

	outputLinearBarGraph.set_option(OutputLinearBarGraph::Options::BarGraphType, true);
	EXPECT_TRUE(outputLinearBarGraph.get_option(OutputLinearBarGraph::Options::BarGraphType));

	outputLinearBarGraph.set_target_value(120);
	EXPECT_EQ(outputLinearBarGraph.get_target_value(), 120);
	EXPECT_TRUE(outputLinearBarGraph.set_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::TargetValue), 8, error));
	EXPECT_EQ(outputLinearBarGraph.get_target_value(), 8);

	outputLinearBarGraph.set_target_value_reference(130);
	EXPECT_EQ(outputLinearBarGraph.get_target_value_reference(), 130);
	EXPECT_TRUE(outputLinearBarGraph.set_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::TargetValueVariableReference), 9, error));
	EXPECT_EQ(outputLinearBarGraph.get_target_value_reference(), 9);

	uint32_t testValue = 0;
	EXPECT_TRUE(outputLinearBarGraph.get_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputLinearBarGraph.set_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::Height), 26, error));
	EXPECT_EQ(outputLinearBarGraph.get_height(), 26);
	EXPECT_TRUE(outputLinearBarGraph.get_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::Height), testValue));
	EXPECT_EQ(testValue, 26);

	EXPECT_TRUE(outputLinearBarGraph.get_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::Colour), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(outputLinearBarGraph.get_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::MaxValue), testValue));
	EXPECT_EQ(testValue, 12);

	EXPECT_TRUE(outputLinearBarGraph.get_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::MinValue), testValue));
	EXPECT_EQ(testValue, 11);

	EXPECT_TRUE(outputLinearBarGraph.set_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::TargetLineColour), 3, error));
	EXPECT_EQ(outputLinearBarGraph.get_target_line_colour(), 3);
	EXPECT_TRUE(outputLinearBarGraph.get_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::TargetLineColour), testValue));
	EXPECT_EQ(testValue, 3);

	EXPECT_TRUE(outputLinearBarGraph.set_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::Options), 4, error));
	EXPECT_TRUE(outputLinearBarGraph.get_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::Options), testValue));
	EXPECT_EQ(testValue, 4);

	EXPECT_TRUE(outputLinearBarGraph.set_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::NumberOfTicks), 9, error));
	EXPECT_TRUE(outputLinearBarGraph.get_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::NumberOfTicks), testValue));
	EXPECT_EQ(testValue, 9);

	// Create and add a number variable so that the test for setting the variable reference passes
	auto numberVariable = std::make_shared<NumberVariable>(objects, colourTable);
	numberVariable->set_id(100);
	objects[numberVariable->get_id()] = numberVariable;

	EXPECT_TRUE(outputLinearBarGraph.set_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::VariableReference), 100, error));
	EXPECT_TRUE(outputLinearBarGraph.get_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::VariableReference), testValue));
	EXPECT_EQ(testValue, 100);

	EXPECT_TRUE(outputLinearBarGraph.set_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::TargetValue), 51, error));
	EXPECT_TRUE(outputLinearBarGraph.get_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::TargetValue), testValue));
	EXPECT_EQ(testValue, 51);

	EXPECT_TRUE(outputLinearBarGraph.get_attribute(static_cast<std::uint8_t>(OutputLinearBarGraph::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::OutputLinearBarGraph));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, OutputArchedBarGraphTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	OutputArchedBarGraph outputArchedBarGraph(objects, colourTable);

	run_baseline_tests(&outputArchedBarGraph);
	EXPECT_EQ(outputArchedBarGraph.get_object_type(), VirtualTerminalObjectType::OutputArchedBarGraph);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	// Setting the type attribute should always fail
	EXPECT_FALSE(outputArchedBarGraph.set_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(outputArchedBarGraph.set_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Test output arched bar graph width attribute
	EXPECT_TRUE(outputArchedBarGraph.set_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::Width), 50, error));
	EXPECT_EQ(outputArchedBarGraph.get_width(), 50);

	outputArchedBarGraph.set_colour(9);
	EXPECT_EQ(outputArchedBarGraph.get_colour(), 9);
	EXPECT_TRUE(outputArchedBarGraph.set_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::Colour), 0, error));
	EXPECT_EQ(outputArchedBarGraph.get_colour(), 0);

	outputArchedBarGraph.set_max_value(65500);
	EXPECT_EQ(outputArchedBarGraph.get_max_value(), 65500);
	EXPECT_TRUE(outputArchedBarGraph.set_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::MaxValue), 6500, error));
	EXPECT_EQ(outputArchedBarGraph.get_max_value(), 6500);

	outputArchedBarGraph.set_bar_graph_width(12);
	EXPECT_EQ(outputArchedBarGraph.get_bar_graph_width(), 12);

	outputArchedBarGraph.set_min_value(3200);
	EXPECT_EQ(outputArchedBarGraph.get_min_value(), 3200);
	EXPECT_TRUE(outputArchedBarGraph.set_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::MinValue), 4000, error));
	EXPECT_EQ(outputArchedBarGraph.get_min_value(), 4000);

	outputArchedBarGraph.set_option(OutputArchedBarGraph::Options::BarGraphType, true);
	EXPECT_TRUE(outputArchedBarGraph.get_option(OutputArchedBarGraph::Options::BarGraphType));

	outputArchedBarGraph.set_end_angle(60);
	EXPECT_EQ(outputArchedBarGraph.get_end_angle(), 60);
	EXPECT_TRUE(outputArchedBarGraph.set_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::EndAngle), 10, error));
	EXPECT_EQ(outputArchedBarGraph.get_end_angle(), 10);

	outputArchedBarGraph.set_start_angle(30);
	EXPECT_EQ(outputArchedBarGraph.get_start_angle(), 30);
	EXPECT_TRUE(outputArchedBarGraph.set_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::StartAngle), 9, error));
	EXPECT_EQ(outputArchedBarGraph.get_start_angle(), 9);

	outputArchedBarGraph.set_target_value(120);
	EXPECT_EQ(outputArchedBarGraph.get_target_value(), 120);
	EXPECT_TRUE(outputArchedBarGraph.set_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::TargetValue), 8, error));
	EXPECT_EQ(outputArchedBarGraph.get_target_value(), 8);

	outputArchedBarGraph.set_target_value_reference(130);
	EXPECT_EQ(outputArchedBarGraph.get_target_value_reference(), 130);
	EXPECT_TRUE(outputArchedBarGraph.set_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::TargetValueVariableReference), 7, error));
	EXPECT_EQ(outputArchedBarGraph.get_target_value_reference(), 7);

	outputArchedBarGraph.set_value(4);
	EXPECT_EQ(outputArchedBarGraph.get_value(), 4);

	outputArchedBarGraph.set_target_line_colour(1);
	EXPECT_EQ(outputArchedBarGraph.get_target_line_colour(), 1);
	EXPECT_TRUE(outputArchedBarGraph.set_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::TargetLineColour), 12, error));
	EXPECT_EQ(outputArchedBarGraph.get_target_line_colour(), 12);

	outputArchedBarGraph.set_options(1);
	EXPECT_TRUE(outputArchedBarGraph.get_option(OutputArchedBarGraph::Options::DrawBorder));

	uint32_t testValue = 0;
	EXPECT_TRUE(outputArchedBarGraph.get_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(outputArchedBarGraph.set_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::Height), 26, error));
	EXPECT_EQ(outputArchedBarGraph.get_height(), 26);

	EXPECT_TRUE(outputArchedBarGraph.get_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::Colour), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(outputArchedBarGraph.get_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::MaxValue), testValue));
	EXPECT_EQ(testValue, 6500);

	EXPECT_TRUE(outputArchedBarGraph.get_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::MinValue), testValue));
	EXPECT_EQ(testValue, 4000);

	EXPECT_TRUE(outputArchedBarGraph.get_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::BarGraphWidth), testValue));
	EXPECT_EQ(testValue, 12);

	EXPECT_TRUE(outputArchedBarGraph.get_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::StartAngle), testValue));
	EXPECT_EQ(testValue, 9);

	EXPECT_TRUE(outputArchedBarGraph.get_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::EndAngle), testValue));
	EXPECT_EQ(testValue, 10);

	EXPECT_TRUE(outputArchedBarGraph.get_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::TargetLineColour), testValue));
	EXPECT_EQ(testValue, 12);

	EXPECT_TRUE(outputArchedBarGraph.get_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::Options), testValue));
	EXPECT_EQ(testValue, 1);

	EXPECT_TRUE(outputArchedBarGraph.get_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::VariableReference), testValue));
	EXPECT_EQ(testValue, 0xFFFF);

	EXPECT_TRUE(outputArchedBarGraph.get_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::TargetValue), testValue));
	EXPECT_EQ(testValue, 8);

	EXPECT_TRUE(outputArchedBarGraph.get_attribute(static_cast<std::uint8_t>(OutputArchedBarGraph::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::OutputArchedBarGraph));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, PictureGraphicTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	PictureGraphic pictureGraphic(objects, colourTable);

	run_baseline_tests(&pictureGraphic);
	EXPECT_EQ(pictureGraphic.get_object_type(), VirtualTerminalObjectType::PictureGraphic);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	// Setting the type attribute should always fail
	EXPECT_FALSE(pictureGraphic.set_attribute(static_cast<std::uint8_t>(PictureGraphic::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(pictureGraphic.set_attribute(static_cast<std::uint8_t>(PictureGraphic::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	pictureGraphic.set_transparency_colour(10);
	EXPECT_EQ(pictureGraphic.get_transparency_colour(), 10);

	pictureGraphic.set_number_of_bytes_in_raw_data(1024);
	EXPECT_EQ(1024, pictureGraphic.get_number_of_bytes_in_raw_data());

	std::vector<std::uint8_t> rawData(1024);

	for (std::size_t i = 0; i < 1024; i++)
	{
		rawData.at(i) = static_cast<std::uint8_t>(i % 255);
	}
	pictureGraphic.set_raw_data(rawData.data(), rawData.size());

	EXPECT_EQ(rawData, pictureGraphic.get_raw_data());

	pictureGraphic.set_actual_height(50);
	EXPECT_EQ(50, pictureGraphic.get_actual_height());

	pictureGraphic.set_actual_width(40);
	EXPECT_EQ(40, pictureGraphic.get_actual_width());

	pictureGraphic.set_format(PictureGraphic::Format::FourBitColour);
	EXPECT_EQ(PictureGraphic::Format::FourBitColour, pictureGraphic.get_format());

	EXPECT_FALSE(pictureGraphic.set_attribute(static_cast<std::uint8_t>(PictureGraphic::AttributeName::Type), static_cast<std::uint16_t>(PictureGraphic::Format::EightBitColour), error));
	EXPECT_TRUE(pictureGraphic.set_attribute(static_cast<std::uint8_t>(PictureGraphic::AttributeName::Width), 90, error));
	EXPECT_EQ(90, pictureGraphic.get_width());

	// Test an option (RLE in this case)
	EXPECT_TRUE(pictureGraphic.set_attribute(static_cast<std::uint8_t>(PictureGraphic::AttributeName::Options), (1 << 2), error));
	EXPECT_TRUE(pictureGraphic.get_option(PictureGraphic::Options::RunLengthEncoded));

	pictureGraphic.set_option(PictureGraphic::Options::RunLengthEncoded, false);
	EXPECT_FALSE(pictureGraphic.get_option(PictureGraphic::Options::RunLengthEncoded));
	pictureGraphic.set_option(PictureGraphic::Options::RunLengthEncoded, true);
	EXPECT_TRUE(pictureGraphic.get_option(PictureGraphic::Options::RunLengthEncoded));

	EXPECT_NO_THROW(pictureGraphic.add_raw_data(45));

	EXPECT_TRUE(pictureGraphic.set_attribute(static_cast<std::uint8_t>(PictureGraphic::AttributeName::TransparencyColour), 90, error));
	EXPECT_EQ(pictureGraphic.get_transparency_colour(), 90);

	uint32_t testValue = 0;
	pictureGraphic.set_width(50);
	EXPECT_TRUE(pictureGraphic.get_attribute(static_cast<std::uint8_t>(PictureGraphic::AttributeName::Width), testValue));
	EXPECT_EQ(testValue, 50);

	pictureGraphic.set_options(18);
	EXPECT_TRUE(pictureGraphic.get_attribute(static_cast<std::uint8_t>(PictureGraphic::AttributeName::Options), testValue));
	EXPECT_EQ(testValue, 18);

	pictureGraphic.set_transparency_colour(70);
	EXPECT_TRUE(pictureGraphic.get_attribute(static_cast<std::uint8_t>(PictureGraphic::AttributeName::TransparencyColour), testValue));
	EXPECT_EQ(testValue, 70);

	pictureGraphic.set_actual_width(100);
	EXPECT_TRUE(pictureGraphic.get_attribute(static_cast<std::uint8_t>(PictureGraphic::AttributeName::ActualWidth), testValue));
	EXPECT_EQ(testValue, 100);

	pictureGraphic.set_actual_height(200);
	EXPECT_TRUE(pictureGraphic.get_attribute(static_cast<std::uint8_t>(PictureGraphic::AttributeName::ActualHeight), testValue));
	EXPECT_EQ(testValue, 200);

	pictureGraphic.set_format(PictureGraphic::Format::EightBitColour);
	EXPECT_TRUE(pictureGraphic.get_attribute(static_cast<std::uint8_t>(PictureGraphic::AttributeName::Format), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(PictureGraphic::Format::EightBitColour));

	EXPECT_TRUE(pictureGraphic.get_attribute(static_cast<std::uint8_t>(PictureGraphic::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::PictureGraphic));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, NumberVariableTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	NumberVariable numberVariable(objects, colourTable);

	run_baseline_tests(&numberVariable);
	EXPECT_EQ(numberVariable.get_object_type(), VirtualTerminalObjectType::NumberVariable);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	// Setting the type attribute should always fail
	EXPECT_FALSE(numberVariable.set_attribute(static_cast<std::uint8_t>(NumberVariable::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(numberVariable.set_attribute(static_cast<std::uint8_t>(NumberVariable::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	numberVariable.set_value(2000000);
	EXPECT_EQ(2000000, numberVariable.get_value());

	uint32_t testValue = 0;
	EXPECT_TRUE(numberVariable.get_attribute(static_cast<std::uint8_t>(NumberVariable::AttributeName::Value), testValue));
	EXPECT_EQ(testValue, 2000000);

	numberVariable.set_value(1000000);
	EXPECT_FALSE(numberVariable.set_attribute(static_cast<std::uint8_t>(NumberVariable::AttributeName::Value), 1000000, error));
	EXPECT_EQ(1000000, numberVariable.get_value());

	EXPECT_TRUE(numberVariable.get_attribute(static_cast<std::uint8_t>(NumberVariable::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::NumberVariable));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, StringVariableTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	StringVariable stringVariable(objects, colourTable);

	run_baseline_tests(&stringVariable);
	EXPECT_EQ(stringVariable.get_object_type(), VirtualTerminalObjectType::StringVariable);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	// Setting the type attribute should always fail
	EXPECT_FALSE(stringVariable.set_attribute(static_cast<std::uint8_t>(StringVariable::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(stringVariable.set_attribute(static_cast<std::uint8_t>(StringVariable::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	stringVariable.set_value("Hello World");
	EXPECT_EQ("Hello World", stringVariable.get_value());

	uint32_t testValue = 0;
	EXPECT_TRUE(stringVariable.get_attribute(static_cast<std::uint8_t>(StringVariable::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::StringVariable));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, FontAttributesTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	FontAttributes fontAttributes(objects, colourTable);

	run_baseline_tests(&fontAttributes);
	EXPECT_EQ(fontAttributes.get_object_type(), VirtualTerminalObjectType::FontAttributes);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	fontAttributes.set_id(10);
	EXPECT_EQ(10, fontAttributes.get_id());

	fontAttributes.set_height(12);
	EXPECT_EQ(12, fontAttributes.get_height());

	fontAttributes.set_type(FontAttributes::FontType::ISO8859_4);
	EXPECT_EQ(FontAttributes::FontType::ISO8859_4, fontAttributes.get_type());

	fontAttributes.set_size(FontAttributes::FontSize::Size24x32);
	EXPECT_EQ(FontAttributes::FontSize::Size24x32, fontAttributes.get_size());

	// For a 24 x 32 font, the width should be 24 and the height should be 32
	EXPECT_EQ(24, fontAttributes.get_font_width_pixels());
	EXPECT_EQ(32, fontAttributes.get_font_height_pixels());

	// Test other font size heights
	fontAttributes.set_size(FontAttributes::FontSize::Size12x16);
	EXPECT_EQ(16, fontAttributes.get_font_height_pixels());
	fontAttributes.set_size(FontAttributes::FontSize::Size16x24);
	EXPECT_EQ(24, fontAttributes.get_font_height_pixels());
	fontAttributes.set_size(FontAttributes::FontSize::Size24x32);
	EXPECT_EQ(32, fontAttributes.get_font_height_pixels());
	fontAttributes.set_size(FontAttributes::FontSize::Size32x48);
	EXPECT_EQ(48, fontAttributes.get_font_height_pixels());
	fontAttributes.set_size(FontAttributes::FontSize::Size48x64);
	EXPECT_EQ(64, fontAttributes.get_font_height_pixels());
	fontAttributes.set_size(FontAttributes::FontSize::Size64x96);
	EXPECT_EQ(96, fontAttributes.get_font_height_pixels());
	fontAttributes.set_size(FontAttributes::FontSize::Size96x128);
	EXPECT_EQ(128, fontAttributes.get_font_height_pixels());
	fontAttributes.set_size(FontAttributes::FontSize::Size128x192);
	EXPECT_EQ(192, fontAttributes.get_font_height_pixels());

	// Test other font size widths
	fontAttributes.set_size(FontAttributes::FontSize::Size12x16);
	EXPECT_EQ(12, fontAttributes.get_font_width_pixels());
	fontAttributes.set_size(FontAttributes::FontSize::Size16x24);
	EXPECT_EQ(16, fontAttributes.get_font_width_pixels());
	fontAttributes.set_size(FontAttributes::FontSize::Size24x32);
	EXPECT_EQ(24, fontAttributes.get_font_width_pixels());
	fontAttributes.set_size(FontAttributes::FontSize::Size32x48);
	EXPECT_EQ(32, fontAttributes.get_font_width_pixels());
	fontAttributes.set_size(FontAttributes::FontSize::Size48x64);
	EXPECT_EQ(48, fontAttributes.get_font_width_pixels());
	fontAttributes.set_size(FontAttributes::FontSize::Size64x96);
	EXPECT_EQ(64, fontAttributes.get_font_width_pixels());
	fontAttributes.set_size(FontAttributes::FontSize::Size96x128);
	EXPECT_EQ(96, fontAttributes.get_font_width_pixels());
	fontAttributes.set_size(FontAttributes::FontSize::Size128x192);
	EXPECT_EQ(128, fontAttributes.get_font_width_pixels());

	fontAttributes.set_style(FontAttributes::FontStyleBits::Italic, true);
	EXPECT_TRUE(fontAttributes.get_style(FontAttributes::FontStyleBits::Italic));

	fontAttributes.set_style(FontAttributes::FontStyleBits::Bold, true);
	EXPECT_TRUE(fontAttributes.get_style(FontAttributes::FontStyleBits::Bold));

	EXPECT_TRUE(fontAttributes.set_attribute(static_cast<std::uint8_t>(FontAttributes::AttributeName::FontStyle), 0, error));
	EXPECT_FALSE(fontAttributes.get_style(FontAttributes::FontStyleBits::Bold));
	EXPECT_FALSE(fontAttributes.get_style(FontAttributes::FontStyleBits::Italic));

	// Setting the type attribute should always fail
	EXPECT_FALSE(fontAttributes.set_attribute(static_cast<std::uint8_t>(FontAttributes::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(fontAttributes.set_attribute(static_cast<std::uint8_t>(FontAttributes::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	EXPECT_TRUE(fontAttributes.set_attribute(static_cast<std::uint8_t>(FontAttributes::AttributeName::FontSize), 4, error));
	EXPECT_EQ(FontAttributes::FontSize::Size16x16, fontAttributes.get_size());

	fontAttributes.set_colour(4);
	EXPECT_EQ(4, fontAttributes.get_colour());
	EXPECT_TRUE(fontAttributes.set_attribute(static_cast<std::uint8_t>(FontAttributes::AttributeName::FontColour), 0, error));
	EXPECT_EQ(0, fontAttributes.get_colour());

	uint32_t testValue = 0;
	EXPECT_TRUE(fontAttributes.get_attribute(static_cast<std::uint8_t>(FontAttributes::AttributeName::FontColour), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(fontAttributes.get_attribute(static_cast<std::uint8_t>(FontAttributes::AttributeName::FontStyle), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(fontAttributes.get_attribute(static_cast<std::uint8_t>(FontAttributes::AttributeName::FontType), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(FontAttributes::FontType::ISO8859_4));

	EXPECT_TRUE(fontAttributes.get_attribute(static_cast<std::uint8_t>(FontAttributes::AttributeName::FontSize), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(FontAttributes::FontSize::Size16x16));

	EXPECT_TRUE(fontAttributes.get_attribute(static_cast<std::uint8_t>(FontAttributes::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::FontAttributes));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, LineAttributesTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	LineAttributes lineAttributes(objects, colourTable);

	run_baseline_tests(&lineAttributes);
	EXPECT_EQ(lineAttributes.get_object_type(), VirtualTerminalObjectType::LineAttributes);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	lineAttributes.set_id(10);
	EXPECT_EQ(10, lineAttributes.get_id());

	lineAttributes.set_line_art_bit_pattern(0xF00F);
	EXPECT_EQ(0xF00F, lineAttributes.get_line_art_bit_pattern());

	EXPECT_TRUE(lineAttributes.set_attribute(static_cast<std::uint8_t>(LineAttributes::AttributeName::LineArt), 0, error));
	EXPECT_EQ(0, lineAttributes.get_line_art_bit_pattern());

	EXPECT_TRUE(lineAttributes.set_attribute(static_cast<std::uint8_t>(LineAttributes::AttributeName::LineColour), 4, error));
	EXPECT_EQ(4, lineAttributes.get_background_color());

	EXPECT_TRUE(lineAttributes.set_attribute(static_cast<std::uint8_t>(LineAttributes::AttributeName::LineWidth), 16, error));
	EXPECT_EQ(16, lineAttributes.get_width());

	// Setting the type attribute should always fail
	EXPECT_FALSE(lineAttributes.set_attribute(static_cast<std::uint8_t>(LineAttributes::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(lineAttributes.set_attribute(static_cast<std::uint8_t>(LineAttributes::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	uint32_t testValue = 0;
	EXPECT_TRUE(lineAttributes.get_attribute(static_cast<std::uint8_t>(LineAttributes::AttributeName::LineArt), testValue));
	EXPECT_EQ(testValue, 0);

	EXPECT_TRUE(lineAttributes.get_attribute(static_cast<std::uint8_t>(LineAttributes::AttributeName::LineColour), testValue));
	EXPECT_EQ(testValue, 4);

	EXPECT_TRUE(lineAttributes.get_attribute(static_cast<std::uint8_t>(LineAttributes::AttributeName::LineWidth), testValue));
	EXPECT_EQ(testValue, 16);

	EXPECT_TRUE(lineAttributes.get_attribute(static_cast<std::uint8_t>(LineAttributes::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::LineAttributes));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, FillAttributesTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	FillAttributes fillAttributes(objects, colourTable);

	run_baseline_tests(&fillAttributes);
	EXPECT_EQ(fillAttributes.get_object_type(), VirtualTerminalObjectType::FillAttributes);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	fillAttributes.set_id(10);
	EXPECT_EQ(10, fillAttributes.get_id());

	EXPECT_TRUE(fillAttributes.set_attribute(static_cast<std::uint8_t>(FillAttributes::AttributeName::FillPattern), 3, error));
	EXPECT_EQ(3, fillAttributes.get_fill_pattern());

	EXPECT_TRUE(fillAttributes.set_attribute(static_cast<std::uint8_t>(FillAttributes::AttributeName::FillType), 2, error));
	EXPECT_EQ(FillAttributes::FillType::FillWithSpecifiedColorInFillColorAttribute, fillAttributes.get_type());

	EXPECT_TRUE(fillAttributes.set_attribute(static_cast<std::uint8_t>(FillAttributes::AttributeName::FillColour), 4, error));
	EXPECT_EQ(4, fillAttributes.get_background_color());

	// Setting the type attribute should always fail
	EXPECT_FALSE(fillAttributes.set_attribute(static_cast<std::uint8_t>(FillAttributes::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(fillAttributes.set_attribute(static_cast<std::uint8_t>(FillAttributes::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	uint32_t testValue = 0;
	EXPECT_TRUE(fillAttributes.get_attribute(static_cast<std::uint8_t>(FillAttributes::AttributeName::FillPattern), testValue));
	EXPECT_EQ(testValue, 3);

	EXPECT_TRUE(fillAttributes.get_attribute(static_cast<std::uint8_t>(FillAttributes::AttributeName::FillColour), testValue));
	EXPECT_EQ(testValue, 4);

	EXPECT_TRUE(fillAttributes.get_attribute(static_cast<std::uint8_t>(FillAttributes::AttributeName::FillType), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(FillAttributes::FillType::FillWithSpecifiedColorInFillColorAttribute));

	EXPECT_TRUE(fillAttributes.get_attribute(static_cast<std::uint8_t>(FillAttributes::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::FillAttributes));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, InputAttributesTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	InputAttributes inputAttributes(objects, colourTable);

	run_baseline_tests(&inputAttributes);
	EXPECT_EQ(inputAttributes.get_object_type(), VirtualTerminalObjectType::InputAttributes);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	inputAttributes.set_id(10);
	EXPECT_EQ(10, inputAttributes.get_id());

	const std::string testValidationString = "123456789";
	inputAttributes.set_validation_string(testValidationString);
	EXPECT_EQ(testValidationString, inputAttributes.get_validation_string());

	inputAttributes.set_validation_type(InputAttributes::ValidationType::InvalidCharactersAreListed);
	EXPECT_EQ(InputAttributes::ValidationType::InvalidCharactersAreListed, inputAttributes.get_validation_type());
	inputAttributes.set_validation_type(InputAttributes::ValidationType::ValidCharactersAreListed);
	EXPECT_EQ(InputAttributes::ValidationType::ValidCharactersAreListed, inputAttributes.get_validation_type());

	// Setting the type attribute should always fail
	EXPECT_FALSE(inputAttributes.set_attribute(static_cast<std::uint8_t>(InputAttributes::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(inputAttributes.set_attribute(static_cast<std::uint8_t>(InputAttributes::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	uint32_t testValue = 0;
	EXPECT_TRUE(inputAttributes.get_attribute(static_cast<std::uint8_t>(InputAttributes::AttributeName::ValidationType), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(InputAttributes::ValidationType::ValidCharactersAreListed));

	EXPECT_TRUE(inputAttributes.get_attribute(static_cast<std::uint8_t>(InputAttributes::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::InputAttributes));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, ExtendedInputAttributesTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	ExtendedInputAttributes extendedInputAttributes(objects, colourTable);

	run_baseline_tests(&extendedInputAttributes);
	EXPECT_EQ(extendedInputAttributes.get_object_type(), VirtualTerminalObjectType::ExtendedInputAttributes);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	extendedInputAttributes.set_number_of_code_planes(3);
	EXPECT_EQ(3, extendedInputAttributes.get_number_of_code_planes());

	extendedInputAttributes.set_validation_type(ExtendedInputAttributes::ValidationType::InvalidCharactersAreListed);
	EXPECT_EQ(ExtendedInputAttributes::ValidationType::InvalidCharactersAreListed, extendedInputAttributes.get_validation_type());

	// Setting the type attribute should always fail
	EXPECT_FALSE(extendedInputAttributes.set_attribute(static_cast<std::uint8_t>(ExtendedInputAttributes::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(extendedInputAttributes.set_attribute(static_cast<std::uint8_t>(ExtendedInputAttributes::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	uint32_t testValue = 0;
	EXPECT_TRUE(extendedInputAttributes.get_attribute(static_cast<std::uint8_t>(ExtendedInputAttributes::AttributeName::ValidationType), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(ExtendedInputAttributes::ValidationType::InvalidCharactersAreListed));

	EXPECT_TRUE(extendedInputAttributes.get_attribute(static_cast<std::uint8_t>(ExtendedInputAttributes::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::ExtendedInputAttributes));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, MacroTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	Macro macro(objects, colourTable);

	run_baseline_tests(&macro);
	EXPECT_EQ(macro.get_object_type(), VirtualTerminalObjectType::Macro);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	const std::array<std::uint8_t, CAN_DATA_LENGTH> TEST_PACKET = { static_cast<std::uint8_t>(Macro::Command::ChangeSize), 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
	macro.add_command_packet(TEST_PACKET);
	EXPECT_EQ(1, macro.get_number_of_commands());

	EXPECT_TRUE(macro.get_is_valid());

	std::array<std::uint8_t, CAN_DATA_LENGTH> returnedCommand = { 0 };
	macro.get_command_packet(0, returnedCommand);
	EXPECT_EQ(returnedCommand, TEST_PACKET);

	EXPECT_TRUE(macro.remove_command_packet(0));
	EXPECT_FALSE(macro.remove_command_packet(0));

	// Add an invalid nonsense packet
	std::array<std::uint8_t, CAN_DATA_LENGTH> nonsensePacket = { 0 };
	macro.add_command_packet(nonsensePacket);
	EXPECT_FALSE(macro.get_is_valid());

	// Setting the type attribute should always fail
	EXPECT_FALSE(macro.set_attribute(static_cast<std::uint8_t>(Macro::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(macro.set_attribute(static_cast<std::uint8_t>(Macro::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	uint32_t testValue = 0;
	EXPECT_TRUE(macro.get_attribute(static_cast<std::uint8_t>(Macro::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::Macro));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, ColourMapTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	ColourMap colourMap(objects, colourTable);

	run_baseline_tests(&colourMap);
	EXPECT_EQ(colourMap.get_object_type(), VirtualTerminalObjectType::ColourMap);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	// Setting the type attribute should always fail
	EXPECT_FALSE(colourMap.set_attribute(static_cast<std::uint8_t>(ColourMap::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(colourMap.set_attribute(static_cast<std::uint8_t>(ColourMap::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	EXPECT_TRUE(colourMap.set_number_of_colour_indexes(256));
	EXPECT_EQ(256, colourMap.get_number_of_colour_indexes());

	// Only values of 256, 16, and 2 are valid
	EXPECT_FALSE(colourMap.set_number_of_colour_indexes(67));
	EXPECT_EQ(256, colourMap.get_number_of_colour_indexes());

	// Check that the default indexes are correct
	EXPECT_EQ(0, colourMap.get_colour_map_index(0));
	EXPECT_EQ(16, colourMap.get_colour_map_index(16));
	EXPECT_TRUE(colourMap.set_colour_map_index(16, 32));
	EXPECT_EQ(32, colourMap.get_colour_map_index(16));

	uint32_t testValue = 0;
	EXPECT_TRUE(colourMap.get_attribute(static_cast<std::uint8_t>(ColourMap::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::ColourMap));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, WindowMaskTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto windowMask = std::make_shared<WindowMask>(objects, colourTable);

	run_baseline_tests(windowMask.get());
	EXPECT_EQ(windowMask->get_object_type(), VirtualTerminalObjectType::WindowMask);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	windowMask->set_window_type(WindowMask::WindowType::NumericInputValueNoUnits1x1);
	EXPECT_EQ(WindowMask::WindowType::NumericInputValueNoUnits1x1, windowMask->get_window_type());

	// Setting the type attribute should always fail
	EXPECT_FALSE(windowMask->set_attribute(static_cast<std::uint8_t>(WindowMask::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(windowMask->set_attribute(static_cast<std::uint8_t>(WindowMask::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	EXPECT_TRUE(windowMask->set_attribute(static_cast<std::uint8_t>(WindowMask::AttributeName::Name), 65534, error));
	EXPECT_EQ(65534, windowMask->get_name_object_id());

	EXPECT_TRUE(windowMask->set_attribute(static_cast<std::uint8_t>(WindowMask::AttributeName::BackgroundColour), 13, error));
	EXPECT_EQ(13, windowMask->get_background_color());

	EXPECT_TRUE(windowMask->set_attribute(static_cast<std::uint8_t>(WindowMask::AttributeName::Options), 1, error));
	EXPECT_TRUE(windowMask->get_option(WindowMask::Options::Available));

	windowMask->set_options(2);
	EXPECT_TRUE(windowMask->get_option(WindowMask::Options::Transparent));
	EXPECT_FALSE(windowMask->get_option(WindowMask::Options::Available));

	windowMask->set_option(WindowMask::Options::Available, true);
	EXPECT_TRUE(windowMask->get_option(WindowMask::Options::Transparent));
	windowMask->set_option(WindowMask::Options::Transparent, false);
	EXPECT_FALSE(windowMask->get_option(WindowMask::Options::Transparent));

	windowMask->set_title_object_id(65535);
	EXPECT_EQ(65535, windowMask->get_title_object_id());

	windowMask->set_icon_object_id(12345);
	EXPECT_EQ(12345, windowMask->get_icon_object_id());
	windowMask->set_icon_object_id(0xFFFF);

	EXPECT_FALSE(windowMask->get_is_valid());

	windowMask->set_id(50);
	objects[windowMask->get_id()] = windowMask;

	// Add a valid title object
	auto title = std::make_shared<OutputString>(objects, colourTable);
	title->set_id(100);
	objects[title->get_id()] = title;
	windowMask->set_title_object_id(100);

	// Should still be invalid because we have no name
	EXPECT_FALSE(windowMask->get_is_valid());

	// Add a name
	auto name = std::make_shared<OutputString>(objects, colourTable);
	name->set_id(101);
	objects[name->get_id()] = name;
	EXPECT_TRUE(windowMask->set_attribute(static_cast<std::uint8_t>(WindowMask::AttributeName::Name), name->get_id(), error));

	// Should still be invalid because we have no icon
	EXPECT_FALSE(windowMask->get_is_valid());

	// Add an icon
	auto icon = std::make_shared<PictureGraphic>(objects, colourTable);
	icon->set_id(102);
	objects[icon->get_id()] = icon;
	windowMask->set_icon_object_id(102);

	// Because this is an input number window mask, it should still be invalid until we add an input number as a child
	EXPECT_FALSE(windowMask->get_is_valid());

	// Add an input number
	auto inputNumber = std::make_shared<InputNumber>(objects, colourTable);
	inputNumber->set_id(103);
	objects[inputNumber->get_id()] = inputNumber;
	windowMask->add_child(inputNumber->get_id(), 0, 0);

	// Now it should be valid
	EXPECT_TRUE(windowMask->get_is_valid());

	// Now let's change the type to NumericInputValueWithUnits1x1
	windowMask->set_window_type(WindowMask::WindowType::NumericInputValueWithUnits1x1);

	// Now it should be invalid again because we don't have a units object
	EXPECT_FALSE(windowMask->get_is_valid());

	// Add a units object
	auto units = std::make_shared<OutputString>(objects, colourTable);
	units->set_id(104);
	objects[units->get_id()] = units;
	windowMask->add_child(104, 0, 0);

	// Now it should be valid again
	EXPECT_TRUE(windowMask->get_is_valid());

	uint32_t testValue = 0;
	EXPECT_TRUE(windowMask->get_attribute(static_cast<std::uint8_t>(WindowMask::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::WindowMask));

	EXPECT_TRUE(windowMask->get_attribute(static_cast<std::uint8_t>(WindowMask::AttributeName::Name), testValue));
	EXPECT_EQ(testValue, 101);

	EXPECT_TRUE(windowMask->get_attribute(static_cast<std::uint8_t>(WindowMask::AttributeName::BackgroundColour), testValue));
	EXPECT_EQ(testValue, 13);

	EXPECT_TRUE(windowMask->get_attribute(static_cast<std::uint8_t>(WindowMask::AttributeName::Options), testValue));
	EXPECT_EQ(testValue, 1);
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, ExternalObjectPointerTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto externalObject = std::make_shared<ExternalObjectPointer>(objects, colourTable);

	run_baseline_tests(externalObject.get());
	EXPECT_EQ(externalObject->get_object_type(), VirtualTerminalObjectType::ExternalObjectPointer);

	// Test default object ID
	externalObject->set_default_object_id(10);
	EXPECT_EQ(10, externalObject->get_default_object_id());

	externalObject->set_external_reference_name_id(20);
	EXPECT_EQ(20, externalObject->get_external_reference_name_id());

	externalObject->set_external_object_id(30);
	EXPECT_EQ(30, externalObject->get_external_object_id());

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	EXPECT_TRUE(externalObject->set_attribute(static_cast<std::uint8_t>(ExternalObjectPointer::AttributeName::DefaultObjectID), 0xFFFF, error));
	EXPECT_EQ(0xFFFF, externalObject->get_default_object_id());

	// We shouldn't allow changing the default object to an object that isn't the null id or an extant object
	EXPECT_FALSE(externalObject->set_attribute(static_cast<std::uint8_t>(ExternalObjectPointer::AttributeName::DefaultObjectID), 1234, error));
	EXPECT_EQ(0xFFFF, externalObject->get_default_object_id());

	EXPECT_TRUE(externalObject->set_attribute(static_cast<std::uint8_t>(ExternalObjectPointer::AttributeName::ExternalReferenceNAMEID), 50, error));
	EXPECT_EQ(50, externalObject->get_external_reference_name_id());

	EXPECT_TRUE(externalObject->set_attribute(static_cast<std::uint8_t>(ExternalObjectPointer::AttributeName::ExternalObjectID), 60, error));
	EXPECT_EQ(60, externalObject->get_external_object_id());

	// Setting the type attribute should always fail
	EXPECT_FALSE(externalObject->set_attribute(static_cast<std::uint8_t>(ExternalObjectPointer::AttributeName::Type), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	// Setting the number of attributes should always fail
	EXPECT_FALSE(externalObject->set_attribute(static_cast<std::uint8_t>(ExternalObjectPointer::AttributeName::NumberOfAttributes), 4, error));
	EXPECT_NE(0, static_cast<std::uint8_t>(error));

	uint32_t testValue = 0;
	EXPECT_TRUE(externalObject->get_attribute(static_cast<std::uint8_t>(ExternalObjectPointer::AttributeName::DefaultObjectID), testValue));
	EXPECT_EQ(testValue, 0xFFFF);

	EXPECT_TRUE(externalObject->get_attribute(static_cast<std::uint8_t>(ExternalObjectPointer::AttributeName::ExternalReferenceNAMEID), testValue));
	EXPECT_EQ(testValue, 50);

	EXPECT_TRUE(externalObject->get_attribute(static_cast<std::uint8_t>(ExternalObjectPointer::AttributeName::ExternalObjectID), testValue));
	EXPECT_EQ(testValue, 60);

	EXPECT_TRUE(externalObject->get_attribute(static_cast<std::uint8_t>(ExternalObjectPointer::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::ExternalObjectPointer));
}

TEST(VIRTUAL_TERMINAL_OBJECT_TESTS, ObjectPointerTests)
{
	std::map<std::uint16_t, std::shared_ptr<VTObject>> objects;
	VTColourTable colourTable;
	auto externalObject = std::make_shared<ObjectPointer>(objects, colourTable);

	run_baseline_tests(externalObject.get());
	EXPECT_EQ(externalObject->get_object_type(), VirtualTerminalObjectType::ObjectPointer);

	VTObject::AttributeError error = VTObject::AttributeError::AnyOtherError;

	// Test all attributes are read only
	EXPECT_FALSE(externalObject->set_attribute(static_cast<std::uint8_t>(ObjectPointer::AttributeName::Value), 0xFFFF, error));

	uint32_t testValue = 0;
	EXPECT_TRUE(externalObject->get_attribute(static_cast<std::uint8_t>(ObjectPointer::AttributeName::Value), testValue));
	EXPECT_EQ(testValue, 0xFFFF);

	EXPECT_TRUE(externalObject->get_attribute(static_cast<std::uint8_t>(ObjectPointer::AttributeName::Type), testValue));
	EXPECT_EQ(testValue, static_cast<std::uint8_t>(VirtualTerminalObjectType::ObjectPointer));
}
