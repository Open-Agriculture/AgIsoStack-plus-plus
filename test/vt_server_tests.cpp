//================================================================================================
/// @file vt_server_tests.cpp
///
/// @brief Unit tests for the VirtualTerminalServer class.
/// @author Open-Agriculture
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"
#include "helpers/test_fixture.hpp"

using namespace isobus;

class DerivedTestVTServer : public VirtualTerminalServer
{
public:
	explicit DerivedTestVTServer(std::shared_ptr<InternalControlFunction> controlFunctionToUse) :
	  VirtualTerminalServer(controlFunctionToUse)
	{
	}

	bool get_is_enough_memory(std::uint32_t) const override
	{
		return true;
	}

	VTVersion get_version() const override
	{
		return VTVersion::Version3;
	}

	std::uint8_t get_number_of_navigation_soft_keys() const override
	{
		return 0;
	}

	std::uint8_t get_soft_key_descriptor_x_pixel_width() const override
	{
		return 0;
	}

	std::uint8_t get_soft_key_descriptor_y_pixel_height() const override
	{
		return 0;
	}

	std::uint8_t get_number_of_possible_virtual_soft_keys_in_soft_key_mask() const override
	{
		return 0;
	}

	std::uint8_t get_number_of_physical_soft_keys() const override
	{
		return 0;
	}

	std::uint16_t get_data_mask_area_size_x_pixels() const override
	{
		return 0;
	}

	std::uint16_t get_data_mask_area_size_y_pixels() const override
	{
		return 0;
	}

	void suspend_working_set(std::shared_ptr<VirtualTerminalServerManagedWorkingSet>) override
	{
	}

	SupportedWideCharsErrorCode get_supported_wide_chars(std::uint8_t, std::uint16_t, std::uint16_t, std::uint8_t &numberOfRanges, std::vector<std::uint8_t> &) override
	{
		numberOfRanges = 0;
		return static_cast<SupportedWideCharsErrorCode>(0);
	}

	std::vector<std::array<std::uint8_t, 7>> get_versions(NAME) override
	{
		return {};
	}

	std::vector<std::uint8_t> get_supported_objects() const override
	{
		return {};
	}

	std::vector<std::uint8_t> load_version(const std::vector<std::uint8_t> &, NAME) override
	{
		return {};
	}

	bool save_version(const std::vector<std::uint8_t> &, const std::vector<std::uint8_t> &, NAME) override
	{
		return true;
	}

	bool delete_version(const std::vector<std::uint8_t> &, NAME) override
	{
		return true;
	}

	bool delete_all_versions(NAME) override
	{
		return true;
	}

	bool delete_object_pool(NAME) override
	{
		return true;
	}

	void run_update()
	{
		update();
	}
};

static void receive_from_client(std::shared_ptr<InternalControlFunction> server,
                                std::shared_ptr<ControlFunction> client,
                                std::initializer_list<std::uint8_t> data)
{
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(7,
	                                                                                                   static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
	                                                                                                   server,
	                                                                                                   client,
	                                                                                                   data));
	CANNetworkManager::CANNetwork.update();
}

static bool poll_for_end_of_object_pool_response(DerivedTestVTServer &server,
                                                 VirtualCANPlugin &plugin,
                                                 test_helpers::TestTimeSource &timeSource,
                                                 CANMessageFrame &responseFrame)
{
	bool foundResponse = false;

	// The pool is parsed on a worker thread, so the response only goes out once that thread finishes
	for (std::uint_fast8_t attempt = 0; (attempt < 50) && (!foundResponse); attempt++)
	{
		server.run_update();
		timeSource.update_for_ms(5);

		while ((!foundResponse) && plugin.read_frame(responseFrame, 10))
		{
			foundResponse = (0x12 == responseFrame.data[0]);
		}
	}
	return foundResponse;
}

class VirtualTerminalServerMessagingTest : public AgIsoStackTestFixture
{
	// Wrapper to give tests a more meaningful name - no content.
};

TEST_F(VirtualTerminalServerMessagingTest, NackForUnmanagedWorkingSetIsSentToTheWorkingSetMasterNotGlobally)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x26, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x81, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();
	EXPECT_TRUE(serverUnderTest.get_initialized());
	testPlugin.clear_queue();

	// An End of Object Pool message from a client that never registered with the server should be
	// NACKed (it is connection-dependent, unlike the stateless Get Memory/Hardware/etc. messages),
	// and per ISO 11783-6:2014 4.6.9 that NACK must be sent to the working set master, not broadcast.
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

	CANMessageFrame nackFrame = {};
	time_source.update_for_ms(5);
	ASSERT_TRUE(testPlugin.read_frame(nackFrame));

	const std::uint32_t expectedUnicastIdentifier = test_helpers::create_ext_can_id(static_cast<std::uint8_t>(CANIdentifier::CANPriority::PriorityLowest7),
	                                                                                static_cast<std::uint32_t>(CANLibParameterGroupNumber::Acknowledge),
	                                                                                client,
	                                                                                internalECU);
	EXPECT_EQ(expectedUnicastIdentifier, nackFrame.identifier);
	EXPECT_EQ(0x01, nackFrame.data[0]); // Negative Acknowledgement
	EXPECT_EQ(client->get_address(), nackFrame.data[4]); // Address of the CF the NACK is about

	CANHardwareInterface::stop();
}

TEST_F(VirtualTerminalServerMessagingTest, EndOfObjectPoolResponseReportsAnErrorWhenTheObjectPoolFailsToParse)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x27, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x82, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	// A working set maintenance message with the initiate bit set connects this client to the server
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

	// Object pool data that cannot be parsed into any object
	receive_from_client(internalECU, client, { 0x11, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA });

	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_end_of_object_pool_response(serverUnderTest, testPlugin, time_source, responseFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x01, responseFrame.data[1]); // Error in object pool
	EXPECT_EQ(0x04, responseFrame.data[6]); // Any other error
}

TEST_F(VirtualTerminalServerMessagingTest, EndOfObjectPoolResponseReportsAnErrorWhenTheObjectPoolHasNoWorkingSetObject)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x28, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x83, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

	// A single number variable object, which parses without error but leaves the pool with no working set object
	receive_from_client(internalECU, client, { 0x11, 0x01, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00 });

	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_end_of_object_pool_response(serverUnderTest, testPlugin, time_source, responseFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x01, responseFrame.data[1]); // Error in object pool
	EXPECT_EQ(0x04, responseFrame.data[6]); // Any other error
}
