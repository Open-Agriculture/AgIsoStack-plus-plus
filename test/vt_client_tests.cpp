#include <gtest/gtest.h>

#include "isobus//utility/iop_file_interface.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"

using namespace isobus;

class DerivedTestVTClient : public VirtualTerminalClient
{
public:
	DerivedTestVTClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource) :
	  VirtualTerminalClient(partner, clientSource){};

	void test_wrapper_process_rx_message(CANMessage *message, void *parentPointer)
	{
		VirtualTerminalClient::process_rx_message(message, parentPointer);
	}

	bool test_wrapper_get_any_pool_needs_scaling() const
	{
		return VirtualTerminalClient::get_any_pool_needs_scaling();
	}

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

	void test_wrapper_set_supported_fonts(std::uint8_t smallFontsBitfield, std::uint8_t largeFontsBitfield)
	{
		smallFontSizesBitfield = smallFontsBitfield;
		largeFontSizesBitfield = largeFontsBitfield;
	}

	static std::vector<std::uint8_t> staticTestPool;

	static bool testWrapperDataChunkCallback(std::uint32_t,
	                                         std::uint32_t bytesOffset,
	                                         std::uint32_t numberOfBytesNeeded,
	                                         std::uint8_t *chunkBuffer,
	                                         void *)
	{
		memcpy(chunkBuffer, &staticTestPool.data()[bytesOffset], numberOfBytesNeeded);
		return true;
	}
};

std::vector<std::uint8_t> DerivedTestVTClient::staticTestPool;

TEST(VIRTUAL_TERMINAL_TESTS, InitializeAndInitialState)
{
	NAME clientNAME(0);
	auto internalECU = std::make_shared<InternalControlFunction>(clientNAME, 0x26, 0);

	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	vtNameFilters.push_back(testFilter);

	auto vtPartner = std::make_shared<PartneredControlFunction>(0, vtNameFilters);

	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);

	EXPECT_EQ(false, clientUnderTest.get_is_initialized());
	EXPECT_EQ(false, clientUnderTest.get_is_connected());

	clientUnderTest.initialize(false);

	EXPECT_EQ(true, clientUnderTest.get_is_initialized());

	EXPECT_EQ(false, clientUnderTest.get_has_adjustable_volume_output());
	EXPECT_EQ(false, clientUnderTest.get_multiple_frequency_audio_output());
	EXPECT_EQ(false, clientUnderTest.get_support_pointing_device_with_pointing_message());
	EXPECT_EQ(false, clientUnderTest.get_support_touchscreen_with_pointing_message());
	EXPECT_EQ(false, clientUnderTest.get_support_intermediate_coordinates_during_drag_operations());
	EXPECT_EQ(0, clientUnderTest.get_number_y_pixels());
	EXPECT_EQ(0, clientUnderTest.get_number_x_pixels());
	EXPECT_EQ(VirtualTerminalClient::VTVersion::ReservedOrUnknown, clientUnderTest.get_connected_vt_version());

	clientUnderTest.terminate();
}

TEST(VIRTUAL_TERMINAL_TESTS, VTStatusMessage)
{
	NAME clientNAME(0);
	auto internalECU = std::make_shared<InternalControlFunction>(clientNAME, 0x26, 0);

	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	vtNameFilters.push_back(testFilter);

	auto vtPartner = std::make_shared<PartneredControlFunction>(0, vtNameFilters);

	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);

	static constexpr std::uint16_t NULL_OBJECT_ID = isobus::VirtualTerminalClient::NULL_OBJECT_ID;
	EXPECT_EQ(NULL_OBJECT_ID, clientUnderTest.get_visible_data_mask());
	EXPECT_EQ(NULL_OBJECT_ID, clientUnderTest.get_visible_soft_key_mask());

	CANLibManagedMessage testMessage(0);
	testMessage.set_identifier(CANIdentifier(CANIdentifier::Type::Extended, static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), CANIdentifier::PriorityDefault6, 0, 0));

	std::uint8_t testContent[] = {
		0xFE, // VT Status message function code
		0x26, // Working set master address
		1234 & 0xFF, // Data mask active
		1234 >> 8, // Data mask active
		4567 & 0xFF, // Soft key mask active
		4567 >> 8, // Soft key mask active
		0xFF, // Busy codes
		1, // VT Function code that is being executed
	};

	testMessage.set_data(testContent, 8);
	clientUnderTest.test_wrapper_process_rx_message(&testMessage, &clientUnderTest);

	EXPECT_EQ(1234, clientUnderTest.get_visible_data_mask());
	EXPECT_EQ(4567, clientUnderTest.get_visible_soft_key_mask());
}

TEST(VIRTUAL_TERMINAL_TESTS, FullPoolAutoscalingWithVector)
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

	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);

	// Actual tests start here
	std::vector<std::uint8_t> testPool = isobus::IOPFileInterface::read_iop_file("../examples/virtual_terminal/version3_object_pool/VT3TestPool.iop");

	if (0 == testPool.size())
	{
		// Try a different path to mitigate differences between how IDEs run the unit test
		testPool = isobus::IOPFileInterface::read_iop_file("examples/virtual_terminal/version3_object_pool/VT3TestPool.iop");
	}

	EXPECT_NE(0, testPool.size());

	clientUnderTest.set_object_pool(0, VirtualTerminalClient::VTVersion::Version3, &testPool);

	EXPECT_EQ(false, clientUnderTest.test_wrapper_get_any_pool_needs_scaling());

	// Test invalid soft key width
	clientUnderTest.set_object_pool_scaling(0, 64, 0);

	EXPECT_EQ(false, clientUnderTest.test_wrapper_get_any_pool_needs_scaling());

	clientUnderTest.set_object_pool_scaling(0, 240, 240);

	// Check functionality of get_any_pool_needs_scaling
	EXPECT_EQ(true, clientUnderTest.test_wrapper_get_any_pool_needs_scaling());

	// Full scaling test using the example pool
	EXPECT_EQ(true, clientUnderTest.test_wrapper_scale_object_pools());
}

TEST(VIRTUAL_TERMINAL_TESTS, FullPoolAutoscalingWithDataChunkCallbacks)
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

	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);

	// Actual tests start here
	DerivedTestVTClient::staticTestPool = isobus::IOPFileInterface::read_iop_file("../examples/virtual_terminal/version3_object_pool/VT3TestPool.iop");

	if (0 == DerivedTestVTClient::staticTestPool.size())
	{
		// Try a different path to mitigate differences between how IDEs run the unit test
		DerivedTestVTClient::staticTestPool = isobus::IOPFileInterface::read_iop_file("examples/virtual_terminal/version3_object_pool/VT3TestPool.iop");
	}

	EXPECT_NE(0, DerivedTestVTClient::staticTestPool.size());

	clientUnderTest.register_object_pool_data_chunk_callback(0, VirtualTerminalClient::VTVersion::Version3, DerivedTestVTClient::staticTestPool.size(), DerivedTestVTClient::testWrapperDataChunkCallback);

	clientUnderTest.set_object_pool_scaling(0, 240, 240);

	// Check functionality of get_any_pool_needs_scaling
	EXPECT_EQ(true, clientUnderTest.test_wrapper_get_any_pool_needs_scaling());

	// Full scaling test using the example pool
	EXPECT_EQ(true, clientUnderTest.test_wrapper_scale_object_pools());
}

TEST(VIRTUAL_TERMINAL_TESTS, FullPoolAutoscalingWithPointer)
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

	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);

	// Actual tests start here
	std::vector<std::uint8_t> testPool = isobus::IOPFileInterface::read_iop_file("../examples/virtual_terminal/version3_object_pool/VT3TestPool.iop");

	if (0 == testPool.size())
	{
		// Try a different path to mitigate differences between how IDEs run the unit test
		testPool = isobus::IOPFileInterface::read_iop_file("examples/virtual_terminal/version3_object_pool/VT3TestPool.iop");
	}

	EXPECT_NE(0, testPool.size());

	clientUnderTest.set_object_pool(0, VirtualTerminalClient::VTVersion::Version3, testPool.data(), testPool.size());

	EXPECT_EQ(false, clientUnderTest.test_wrapper_get_any_pool_needs_scaling());

	// Test invalid soft key width
	clientUnderTest.set_object_pool_scaling(0, 64, 0);

	EXPECT_EQ(false, clientUnderTest.test_wrapper_get_any_pool_needs_scaling());

	// Test invalid data mask key width
	clientUnderTest.set_object_pool_scaling(0, 0, 64);

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

	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);

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

	// Don't support proprietary objects for autoscaling
	EXPECT_EQ(0, clientUnderTest.test_wrapper_get_minimum_object_length(VirtualTerminalObjectType::ManufacturerDefined11));
}

TEST(VIRTUAL_TERMINAL_TESTS, FontRemapping)
{
	DerivedTestVTClient clientUnderTest(nullptr, nullptr);

	// Check some easy 50% scaling cases
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size128x128, 0.5f), VirtualTerminalClient::FontSize::Size64x64);
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size64x64, 0.5f), VirtualTerminalClient::FontSize::Size32x32);
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size32x32, 0.5f), VirtualTerminalClient::FontSize::Size16x16);
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size16x16, 0.5f), VirtualTerminalClient::FontSize::Size8x8);

	// Ensure the floor of font sizes is 6x8
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size16x16, 0.00005f), VirtualTerminalClient::FontSize::Size6x8);
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size6x8, 0.00005f), VirtualTerminalClient::FontSize::Size6x8);

	// Check 75% scaling
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size128x192, 0.75f), VirtualTerminalClient::FontSize::Size96x128);

	// Check some easy 200% scaling cases
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size8x8, 2.0f), VirtualTerminalClient::FontSize::Size16x16);
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size16x16, 2.0f), VirtualTerminalClient::FontSize::Size32x32);
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size32x32, 2.0f), VirtualTerminalClient::FontSize::Size64x64);
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size64x64, 2.0f), VirtualTerminalClient::FontSize::Size128x128);

	// Ensure the size is capped at 196x128
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size32x32, 800.0f), VirtualTerminalClient::FontSize::Size128x192);

	// Check some partial upscaling
	EXPECT_EQ(clientUnderTest.test_wrapper_remap_font_to_scale(VirtualTerminalClient::FontSize::Size16x16, 1.5f), VirtualTerminalClient::FontSize::Size16x24);

	// Set and test supported Fonts
	clientUnderTest.test_wrapper_set_supported_fonts(0b01010101, 0b01010101);

	// Small fonts
	EXPECT_EQ(true, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size6x8));
	EXPECT_EQ(false, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size8x8));
	EXPECT_EQ(true, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size8x12));
	EXPECT_EQ(false, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size12x16));
	EXPECT_EQ(true, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size16x16));
	EXPECT_EQ(false, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size16x24));
	EXPECT_EQ(true, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size24x32));
	EXPECT_EQ(false, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size32x32));

	// Large fonts
	EXPECT_EQ(true, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size32x48));
	EXPECT_EQ(false, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size48x64));
	EXPECT_EQ(true, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size64x64));
	EXPECT_EQ(false, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size64x96));
	EXPECT_EQ(true, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size96x128));
	EXPECT_EQ(false, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size128x128));
	EXPECT_EQ(true, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size128x192));

	// Remapping to the available fonts
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size6x8, clientUnderTest.test_wrapper_get_font_or_next_smallest_font((VirtualTerminalClient::FontSize::Size6x8)));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size6x8, clientUnderTest.test_wrapper_get_font_or_next_smallest_font((VirtualTerminalClient::FontSize::Size8x8)));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size8x12, clientUnderTest.test_wrapper_get_font_or_next_smallest_font((VirtualTerminalClient::FontSize::Size8x12)));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size8x12, clientUnderTest.test_wrapper_get_font_or_next_smallest_font((VirtualTerminalClient::FontSize::Size12x16)));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size16x16, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size16x16));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size16x16, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size16x24));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size24x32, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size24x32));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size24x32, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size32x32));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size32x48, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size32x48));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size32x48, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size48x64));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size64x64, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size64x64));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size64x64, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size64x96));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size96x128, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size96x128));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size96x128, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size128x128));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size128x192, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size128x192));

	clientUnderTest.test_wrapper_set_supported_fonts(0b10101010, 0b10101010);
	// Small fonts
	EXPECT_EQ(false, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size6x8));
	EXPECT_EQ(true, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size8x8));
	EXPECT_EQ(false, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size8x12));
	EXPECT_EQ(true, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size12x16));
	EXPECT_EQ(false, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size16x16));
	EXPECT_EQ(true, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size16x24));
	EXPECT_EQ(false, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size24x32));
	EXPECT_EQ(true, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size32x32));

	// Large fonts
	EXPECT_EQ(false, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size32x48));
	EXPECT_EQ(true, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size48x64));
	EXPECT_EQ(false, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size64x64));
	EXPECT_EQ(true, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size64x96));
	EXPECT_EQ(false, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size96x128));
	EXPECT_EQ(true, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size128x128));
	EXPECT_EQ(false, clientUnderTest.get_font_size_supported(VirtualTerminalClient::FontSize::Size128x192));

	// Remapping to the available fonts
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size6x8, clientUnderTest.test_wrapper_get_font_or_next_smallest_font((VirtualTerminalClient::FontSize::Size6x8)));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size8x8, clientUnderTest.test_wrapper_get_font_or_next_smallest_font((VirtualTerminalClient::FontSize::Size8x8)));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size8x8, clientUnderTest.test_wrapper_get_font_or_next_smallest_font((VirtualTerminalClient::FontSize::Size8x12)));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size12x16, clientUnderTest.test_wrapper_get_font_or_next_smallest_font((VirtualTerminalClient::FontSize::Size12x16)));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size12x16, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size16x16));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size16x24, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size16x24));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size16x24, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size24x32));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size32x32, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size32x32));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size32x32, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size32x48));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size48x64, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size48x64));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size48x64, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size64x64));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size64x96, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size64x96));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size64x96, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size96x128));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size128x128, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size128x128));
	EXPECT_EQ(VirtualTerminalClient::FontSize::Size128x128, clientUnderTest.test_wrapper_get_font_or_next_smallest_font(VirtualTerminalClient::FontSize::Size128x192));

	// It doesn't really make sense to test the hardcoded scales against the same arbitrary boundaries I made up, so just loop through all remappings.
	// If we discover good scale factors from real testing we can add them here instead.
	for (std::uint8_t i = 0; i <= static_cast<std::uint8_t>(VirtualTerminalClient::FontSize::Size128x192); i++)
	{
		for (float j = 0.0f; j <= 24.0f; j += 0.05f)
		{
			clientUnderTest.test_wrapper_remap_font_to_scale(static_cast<VirtualTerminalClient::FontSize>(i), j);
		}
	}
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

	DerivedTestVTClient clientUnderTest(nullptr, nullptr);

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

	DerivedTestVTClient clientUnderTest(nullptr, nullptr);

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

	DerivedTestVTClient clientUnderTest(nullptr, nullptr);

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

	DerivedTestVTClient clientUnderTest(nullptr, nullptr);

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

	DerivedTestVTClient clientUnderTest(nullptr, nullptr);

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

	DerivedTestVTClient clientUnderTest(nullptr, nullptr);

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
		0x00,
		0x00,
		0x00,
		0x00
	};

	DerivedTestVTClient clientUnderTest(nullptr, nullptr);

	// Check object length
	EXPECT_EQ(12, clientUnderTest.test_wrapper_get_number_bytes_in_object(testObject));

	// Add a macro and re-check the length
	testObject[11] = 1;
	EXPECT_EQ(14, clientUnderTest.test_wrapper_get_number_bytes_in_object(testObject));

	// Add a full list of child objects and re-check the length
	testObject[10] = 255;
	EXPECT_EQ(524, clientUnderTest.test_wrapper_get_number_bytes_in_object(testObject));

	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 0.5f, VirtualTerminalObjectType::OutputList));
	EXPECT_EQ(testWidth / 2, static_cast<std::uint16_t>(testObject[3]) | (static_cast<std::uint16_t>(testObject[4]) << 8));
	EXPECT_EQ(testHeight / 2, static_cast<std::uint16_t>(testObject[5]) | (static_cast<std::uint16_t>(testObject[6]) << 8));

	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 2.0f, VirtualTerminalObjectType::OutputList));
	EXPECT_EQ(testWidth, static_cast<std::uint16_t>(testObject[3]) | (static_cast<std::uint16_t>(testObject[4]) << 8));
	EXPECT_EQ(testHeight, static_cast<std::uint16_t>(testObject[5]) | (static_cast<std::uint16_t>(testObject[6]) << 8));
}

TEST(VIRTUAL_TERMINAL_TESTS, ResizeInputBoolean)
{
	constexpr std::uint16_t testWidth = 50;
	std::uint8_t testObject[] = {
		0x00,
		0x01,
		0x07,
		0x00,
		testWidth & 0xFF,
		(testWidth >> 8),
		0xFF,
		0xFF,
		0xFF,
		0xFF,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00
	};

	DerivedTestVTClient clientUnderTest(nullptr, nullptr);

	// Check object length
	EXPECT_EQ(13, clientUnderTest.test_wrapper_get_number_bytes_in_object(testObject));

	// Add a macro and re-check the length
	testObject[12] = 1;
	EXPECT_EQ(15, clientUnderTest.test_wrapper_get_number_bytes_in_object(testObject));

	// Cant really resize these since the width is a max value. Should remain the same.
	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 2.0f, VirtualTerminalObjectType::InputBoolean));
	EXPECT_EQ(testWidth, static_cast<std::uint16_t>(testObject[4]) | (static_cast<std::uint16_t>(testObject[5]) << 8));
}

TEST(VIRTUAL_TERMINAL_TESTS, TestNumberBytesInInvalidObjects)
{
	DerivedTestVTClient clientUnderTest(nullptr, nullptr);

	// Test some unsupported objects
	for (auto i = static_cast<std::uint8_t>(VirtualTerminalObjectType::ManufacturerDefined1); i < static_cast<std::uint8_t>(VirtualTerminalObjectType::Reserved); i++)
	{
		std::uint8_t testObject[] = {
			0x00,
			0x01,
			i
		};
		EXPECT_EQ(0, clientUnderTest.test_wrapper_get_number_bytes_in_object(testObject));
	}
}