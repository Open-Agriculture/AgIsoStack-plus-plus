#include <gtest/gtest.h>

#include "isobus//utility/iop_file_interface.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"

using namespace isobus;

class DerivedTestVtClient : public VirtualTerminalClient
{
public:
	DerivedTestVtClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource) :
	  VirtualTerminalClient(partner, clientSource){};

	bool test_wrapper_get_any_pool_needs_scaling() const
	{
		return VirtualTerminalClient::get_any_pool_needs_scaling();
	};

	bool test_wrapper_scale_object_pools()
	{
		return VirtualTerminalClient::scale_object_pools();
	}

	bool test_wrapper_get_is_object_scalable(VirtualTerminalObjectType type) const
	{
		return VirtualTerminalClient::get_is_object_scalable(type);
	}

	FontSize test_wrapper_get_font_or_next_smallest_font(FontSize originalFont) const
	{
		return VirtualTerminalClient::get_font_or_next_smallest_font(originalFont);
	}

	FontSize test_wrapper_remap_font_to_scale(FontSize originalFont, float scaleFactor) const
	{
		return VirtualTerminalClient::remap_font_to_scale(originalFont, scaleFactor);
	}

	std::uint32_t test_wrapper_get_minimum_object_length(VirtualTerminalObjectType type) const
	{
		return VirtualTerminalClient::get_minimum_object_length(type);
	}

	std::uint32_t test_wrapper_get_number_bytes_in_object(std::uint8_t *buffer)
	{
		return VirtualTerminalClient::get_number_bytes_in_object(buffer);
	}

	bool test_wrapper_resize_object(std::uint8_t *buffer, float scaleFactor, VirtualTerminalObjectType type)
	{
		return resize_object(buffer, scaleFactor, type);
	}
};

TEST(VIRTUAL_TERMINAL_TESTS, FullPoolAutoscaling)
{
	NAME clientNAME(0);
	clientNAME.set_arbitrary_address_capable(true);
	clientNAME.set_industry_group(1);
	clientNAME.set_device_class(0);
	clientNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::OilSystemMonitor));
	clientNAME.set_identity_number(1);
	clientNAME.set_ecu_instance(1);
	clientNAME.set_function_instance(0);
	clientNAME.set_device_class_instance(0);
	clientNAME.set_manufacturer_code(69);

	auto internalECU = std::make_shared<InternalControlFunction>(clientNAME, 0x26, 0);

	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	vtNameFilters.push_back(testFilter);

	auto vtPartner = std::make_shared<PartneredControlFunction>(0, vtNameFilters);

	DerivedTestVtClient clientUnderTest(vtPartner, internalECU);

	// Actual tests start here
	std::vector<std::uint8_t> testPool = isobus::IOPFileInterface::read_iop_file("../examples/vt_version_3_object_pool/VT3TestPool.iop");
	EXPECT_NE(0, testPool.size());

	clientUnderTest.set_object_pool(0, VirtualTerminalClient::VTVersion::Version3, &testPool);

	EXPECT_EQ(false, clientUnderTest.test_wrapper_get_any_pool_needs_scaling());

	// Test invalid pool index
	clientUnderTest.set_object_pool_scaling(36, 64, 99);

	EXPECT_EQ(false, clientUnderTest.test_wrapper_get_any_pool_needs_scaling());

	clientUnderTest.set_object_pool_scaling(0, 240, 240);

	// Check functionality of get_any_pool_needs_scaling
	EXPECT_EQ(true, clientUnderTest.test_wrapper_get_any_pool_needs_scaling());

	// Full scaling test using the example pool
	EXPECT_EQ(true, clientUnderTest.test_wrapper_scale_object_pools());
}

TEST(VIRTUAL_TERMINAL_TESTS, ObjectMetadataTests)
{
	NAME clientNAME(0);
	auto internalECU = std::make_shared<InternalControlFunction>(clientNAME, 0x26, 0);

	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	vtNameFilters.push_back(testFilter);

	auto vtPartner = std::make_shared<PartneredControlFunction>(0, vtNameFilters);

	DerivedTestVtClient clientUnderTest(vtPartner, internalECU);

	// These values come from the ISO standard directly
	EXPECT_EQ(10, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::WorkingSet));
	EXPECT_EQ(8, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::DataMask));
	EXPECT_EQ(10, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::AlarmMask));
	EXPECT_EQ(10, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::Container));
	EXPECT_EQ(6, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::SoftKeyMask));
	EXPECT_EQ(7, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::Key));
	EXPECT_EQ(13, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::Button));
	EXPECT_EQ(13, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::InputBoolean));
	EXPECT_EQ(19, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::InputString));
	EXPECT_EQ(38, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::InputNumber));
	EXPECT_EQ(13, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::InputList));
	EXPECT_EQ(17, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::OutputString));
	EXPECT_EQ(29, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::OutputNumber));
	EXPECT_EQ(12, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::OutputList));
	EXPECT_EQ(11, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::OutputLine));
	EXPECT_EQ(13, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::OutputRectangle));
	EXPECT_EQ(15, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::OutputEllipse));
	EXPECT_EQ(14, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::OutputPolygon));
	EXPECT_EQ(21, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::OutputMeter));
	EXPECT_EQ(24, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::OutputLinearBarGraph));
	EXPECT_EQ(27, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::OutputArchedBarGraph));
	EXPECT_EQ(17, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::PictureGraphic));
	EXPECT_EQ(7, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::NumberVariable));
	EXPECT_EQ(5, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::StringVariable));
	EXPECT_EQ(8, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::FontAttributes));
	EXPECT_EQ(8, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::LineAttributes));
	EXPECT_EQ(8, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::FillAttributes));
	EXPECT_EQ(7, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::InputAttributes));
	EXPECT_EQ(5, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::ExtendedInputAttributes));
	EXPECT_EQ(5, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::ObjectPointer));
	EXPECT_EQ(5, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::Macro));
	EXPECT_EQ(6, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::ColourMap));
	EXPECT_EQ(34, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::GraphicsContext));
	EXPECT_EQ(17, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::WindowMask));
	EXPECT_EQ(10, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::KeyGroup));
	EXPECT_EQ(12, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::ObjectLabelRefrenceList));
	EXPECT_EQ(13, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::ExternalObjectDefinition));
	EXPECT_EQ(12, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::ExternalReferenceNAME));
	EXPECT_EQ(9, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::ExternalObjectPointer));
	EXPECT_EQ(17, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::Animation));
}

TEST(VIRTUAL_TERMINAL_TESTS, FontRemapping)
{
	DerivedTestVtClient clientUnderTest(nullptr, nullptr);

	// Check some easy 50% scaling cases
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size128x128, 0.5f), VirtualTerminalClient::FontSize::Size64x64);
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size64x64, 0.5f), VirtualTerminalClient::FontSize::Size32x32);
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size32x32, 0.5f), VirtualTerminalClient::FontSize::Size16x16);
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size16x16, 0.5f), VirtualTerminalClient::FontSize::Size8x8);

	// Ensure the floor of font sizes is 6x8
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size16x16, 0.00005f), VirtualTerminalClient::FontSize::Size6x8);
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size6x8, 0.00005f), VirtualTerminalClient::FontSize::Size6x8);

	// Check some easy 200% scaling cases
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size8x8, 2.0f), VirtualTerminalClient::FontSize::Size16x16);
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size16x16, 2.0f), VirtualTerminalClient::FontSize::Size32x32);
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size32x32, 2.0f), VirtualTerminalClient::FontSize::Size64x64);
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size64x64, 2.0f), VirtualTerminalClient::FontSize::Size128x128);

	// Ensure the size is capped at 196x128
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size32x32, 800.0f), VirtualTerminalClient::FontSize::Size128x192);

	// Check some partial upscaling
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size16x16, 1.5f), VirtualTerminalClient::FontSize::Size16x24);
}

TEST(VIRTUAL_TERMINAL_TESTS, ResizeOutputArchedBarGraph)
{
	constexpr std::uint16_t testWidth = 200;
	constexpr std::uint16_t testHeight = 100;
	std::uint8_t testObject[] = {
		0x00,
		0x01,
		0x13,
		testWidth & 0xFF,
		(testWidth >> 8),
		testHeight & 0xFF,
		(testHeight >> 8),
		0x07,
		0x00,
		0x03,
		0x00,
		0xB4,
		0x30,
		0x00,
		0x00,
		0x00,
		0xFF,
		0x00,
		0xFF,
		0xFF,
		0x00,
		0x00,
		0xFF,
		0xFF,
		0x10,
		0x00,
		0x00
	};

	DerivedTestVtClient clientUnderTest(nullptr, nullptr);

	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 0.5f, VirtualTerminalObjectType::OutputArchedBarGraph));
	EXPECT_EQ(testWidth / 2, static_cast<std::uint16_t>(testObject[3]) | (static_cast<std::uint16_t>(testObject[4]) << 8));
	EXPECT_EQ(testHeight / 2, static_cast<std::uint16_t>(testObject[5]) | (static_cast<std::uint16_t>(testObject[6]) << 8));

	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 2.0f, VirtualTerminalObjectType::OutputArchedBarGraph));
	EXPECT_EQ(testWidth, static_cast<std::uint16_t>(testObject[3]) | (static_cast<std::uint16_t>(testObject[4]) << 8));
	EXPECT_EQ(testHeight, static_cast<std::uint16_t>(testObject[5]) | (static_cast<std::uint16_t>(testObject[6]) << 8));
}

TEST(VIRTUAL_TERMINAL_TESTS, ResizeOutputLinearBarGraph)
{
	constexpr std::uint16_t testWidth = 200;
	constexpr std::uint16_t testHeight = 100;
	std::uint8_t testObject[] = {
		0x00,
		0x01,
		0x12,
		testWidth & 0xFF,
		(testWidth >> 8),
		testHeight & 0xFF,
		(testHeight >> 8),
		0x07,
		0x00,
		0x03,
		0x00,
		0xB4,
		0x30,
		0x00,
		0x00,
		0x00,
		0xFF,
		0x00,
		0xFF,
		0xFF,
		0x00,
		0x00,
		0xFF,
		0x00
	};

	DerivedTestVtClient clientUnderTest(nullptr, nullptr);

	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 0.5f, VirtualTerminalObjectType::OutputLinearBarGraph));
	EXPECT_EQ(testWidth / 2, static_cast<std::uint16_t>(testObject[3]) | (static_cast<std::uint16_t>(testObject[4]) << 8));
	EXPECT_EQ(testHeight / 2, static_cast<std::uint16_t>(testObject[5]) | (static_cast<std::uint16_t>(testObject[6]) << 8));

	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 2.0f, VirtualTerminalObjectType::OutputLinearBarGraph));
	EXPECT_EQ(testWidth, static_cast<std::uint16_t>(testObject[3]) | (static_cast<std::uint16_t>(testObject[4]) << 8));
	EXPECT_EQ(testHeight, static_cast<std::uint16_t>(testObject[5]) | (static_cast<std::uint16_t>(testObject[6]) << 8));
}

TEST(VIRTUAL_TERMINAL_TESTS, ResizeOutputMeter)
{
	constexpr std::uint16_t testWidth = 200;
	constexpr std::uint16_t testHeight = 100;
	std::uint8_t testObject[] = {
		0x00,
		0x01,
		0x11,
		testWidth & 0xFF,
		(testWidth >> 8),
		0x00,
		0x00,
		0x07,
		0x00,
		0x03,
		0x00,
		0xB4,
		0x30,
		0x00,
		0x00,
		0x00,
		0xFF,
		0x00,
		0xFF,
		0xFF,
		0x00
	};

	DerivedTestVtClient clientUnderTest(nullptr, nullptr);

	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 0.5f, VirtualTerminalObjectType::OutputMeter));
	EXPECT_EQ(testWidth / 2, static_cast<std::uint16_t>(testObject[3]) | (static_cast<std::uint16_t>(testObject[4]) << 8));

	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 2.0f, VirtualTerminalObjectType::OutputMeter));
	EXPECT_EQ(testWidth, static_cast<std::uint16_t>(testObject[3]) | (static_cast<std::uint16_t>(testObject[4]) << 8));
}

TEST(VIRTUAL_TERMINAL_TESTS, ResizeOutputPolygon)
{
	constexpr std::uint16_t testWidth = 200;
	constexpr std::uint16_t testHeight = 100;
	std::uint8_t testObject[] = {
		0x00,
		0x01,
		0x10,
		testWidth & 0xFF,
		(testWidth >> 8),
		testHeight & 0xFF,
		(testHeight >> 8),
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0x00,
		0x00
	};

	DerivedTestVtClient clientUnderTest(nullptr, nullptr);

	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 0.5f, VirtualTerminalObjectType::OutputPolygon));
	EXPECT_EQ(testWidth / 2, static_cast<std::uint16_t>(testObject[3]) | (static_cast<std::uint16_t>(testObject[4]) << 8));
	EXPECT_EQ(testHeight / 2, static_cast<std::uint16_t>(testObject[5]) | (static_cast<std::uint16_t>(testObject[6]) << 8));

	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 2.0f, VirtualTerminalObjectType::OutputPolygon));
	EXPECT_EQ(testWidth, static_cast<std::uint16_t>(testObject[3]) | (static_cast<std::uint16_t>(testObject[4]) << 8));
	EXPECT_EQ(testHeight, static_cast<std::uint16_t>(testObject[5]) | (static_cast<std::uint16_t>(testObject[6]) << 8));
}

TEST(VIRTUAL_TERMINAL_TESTS, ResizeOutputEllipse)
{
	constexpr std::uint16_t testWidth = 200;
	constexpr std::uint16_t testHeight = 100;
	std::uint8_t testObject[] = {
		0x00,
		0x01,
		0x0F,
		0xFF,
		0xFF,
		testWidth & 0xFF,
		(testWidth >> 8),
		testHeight & 0xFF,
		(testHeight >> 8),
		0x00,
		0x00,
		0xFF,
		0xFF,
		0xFF,
		0x00
	};

	DerivedTestVtClient clientUnderTest(nullptr, nullptr);

	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 0.5f, VirtualTerminalObjectType::OutputEllipse));
	EXPECT_EQ(testWidth / 2, static_cast<std::uint16_t>(testObject[5]) | (static_cast<std::uint16_t>(testObject[6]) << 8));
	EXPECT_EQ(testHeight / 2, static_cast<std::uint16_t>(testObject[7]) | (static_cast<std::uint16_t>(testObject[8]) << 8));

	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 2.0f, VirtualTerminalObjectType::OutputEllipse));
	EXPECT_EQ(testWidth, static_cast<std::uint16_t>(testObject[5]) | (static_cast<std::uint16_t>(testObject[6]) << 8));
	EXPECT_EQ(testHeight, static_cast<std::uint16_t>(testObject[7]) | (static_cast<std::uint16_t>(testObject[8]) << 8));
}

TEST(VIRTUAL_TERMINAL_TESTS, ResizeOutputLine)
{
	constexpr std::uint16_t testWidth = 200;
	constexpr std::uint16_t testHeight = 100;
	std::uint8_t testObject[] = {
		0x00,
		0x01,
		0x0D,
		0xFF,
		0xFF,
		testWidth & 0xFF,
		(testWidth >> 8),
		testHeight & 0xFF,
		(testHeight >> 8),
		0xFF,
		0xFF
	};

	DerivedTestVtClient clientUnderTest(nullptr, nullptr);

	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 0.5f, VirtualTerminalObjectType::OutputLine));
	EXPECT_EQ(testWidth / 2, static_cast<std::uint16_t>(testObject[5]) | (static_cast<std::uint16_t>(testObject[6]) << 8));
	EXPECT_EQ(testHeight / 2, static_cast<std::uint16_t>(testObject[7]) | (static_cast<std::uint16_t>(testObject[8]) << 8));

	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 2.0f, VirtualTerminalObjectType::OutputLine));
	EXPECT_EQ(testWidth, static_cast<std::uint16_t>(testObject[5]) | (static_cast<std::uint16_t>(testObject[6]) << 8));
	EXPECT_EQ(testHeight, static_cast<std::uint16_t>(testObject[7]) | (static_cast<std::uint16_t>(testObject[8]) << 8));
}

TEST(VIRTUAL_TERMINAL_TESTS, ResizeOutputList)
{
	constexpr std::uint16_t testWidth = 200;
	constexpr std::uint16_t testHeight = 100;
	std::uint8_t testObject[] = {
		0x00,
		0x01,
		0x25,
		testWidth & 0xFF,
		(testWidth >> 8),
		testHeight & 0xFF,
		(testHeight >> 8),
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0x00
	};

	DerivedTestVtClient clientUnderTest(nullptr, nullptr);

	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 0.5f, VirtualTerminalObjectType::OutputList));
	EXPECT_EQ(testWidth / 2, static_cast<std::uint16_t>(testObject[3]) | (static_cast<std::uint16_t>(testObject[4]) << 8));
	EXPECT_EQ(testHeight / 2, static_cast<std::uint16_t>(testObject[5]) | (static_cast<std::uint16_t>(testObject[6]) << 8));

	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 2.0f, VirtualTerminalObjectType::OutputList));
	EXPECT_EQ(testWidth, static_cast<std::uint16_t>(testObject[3]) | (static_cast<std::uint16_t>(testObject[4]) << 8));
	EXPECT_EQ(testHeight, static_cast<std::uint16_t>(testObject[5]) | (static_cast<std::uint16_t>(testObject[6]) << 8));
}
