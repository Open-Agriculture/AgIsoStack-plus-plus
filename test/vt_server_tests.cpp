//================================================================================================
/// @file vt_server_tests.cpp
///
/// @brief Unit tests for the VirtualTerminalServer class.
/// @author Open-Agriculture
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server.hpp"
#include "isobus/utility/iop_file_interface.hpp"
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
		return version;
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
		return versionToLoad;
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

	void wait_for_object_pool_to_parse()
	{
		for (std::uint_fast8_t attempt = 0; attempt < 200; attempt++)
		{
			const auto state = managedWorkingSetList.front()->get_object_pool_processing_state();

			if ((VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Success == state) ||
			    (VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Fail == state))
			{
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
	}

	void add_managed_working_set(std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet)
	{
		managedWorkingSetList.push_back(workingSet);
	}

	// helpers to get access to protected stuff
	bool execute_macro_for_test(std::uint16_t objectID, std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet)
	{
		return execute_macro(objectID, workingSet);
	}

	using VirtualTerminalServer::busyCodesBitfield;
	using VirtualTerminalServer::send_status_message;

	std::vector<std::uint8_t> versionToLoad;
	VTVersion version = VTVersion::Version3;
};

class VirtualTerminalServerMessagingTest : public AgIsoStackTestFixture
{
protected:
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

	bool poll_for_response(DerivedTestVTServer &server,
	                       VirtualCANPlugin &plugin,
	                       std::uint8_t function,
	                       CANMessageFrame &responseFrame)
	{
		bool foundResponse = false;

		// The pool is parsed on a worker thread, so the response only goes out once that thread finishes
		for (std::uint_fast8_t attempt = 0; (attempt < 50) && (!foundResponse); attempt++)
		{
			server.run_update();
			time_source.update_for_ms(5);

			while ((!foundResponse) && plugin.read_frame(responseFrame, 10))
			{
				foundResponse = (function == responseFrame.data[0]);
			}
		}
		return foundResponse;
	}

	bool read_status_busy_codes(const DerivedTestVTServer &server, VirtualCANPlugin &plugin, std::uint8_t &busyCodes)
	{
		bool foundStatus = false;
		CANMessageFrame statusFrame = {};

		plugin.clear_queue();
		server.send_status_message();
		time_source.update_for_ms(5);
		while ((!foundStatus) && plugin.read_frame(statusFrame, 10))
		{
			foundStatus = (0xFE == statusFrame.data[0]);
		}
		busyCodes = statusFrame.data[6];
		return foundStatus;
	}

	static std::vector<std::uint8_t> read_test_pool()
	{
		std::vector<std::uint8_t> testPool = IOPFileInterface::read_iop_file("../../examples/virtual_terminal/version3_object_pool/VT3TestPool.iop");

		if (testPool.empty())
		{
			// Try a different path to mitigate differences between how IDEs run the unit test
			testPool = IOPFileInterface::read_iop_file("../examples/virtual_terminal/version3_object_pool/VT3TestPool.iop");
		}
		return testPool;
	}
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
	const bool foundResponse = poll_for_response(serverUnderTest, testPlugin, 0x12, responseFrame);
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
	const bool foundResponse = poll_for_response(serverUnderTest, testPlugin, 0x12, responseFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x01, responseFrame.data[1]); // Error in object pool
	EXPECT_EQ(0x04, responseFrame.data[6]); // Any other error
}

TEST_F(VirtualTerminalServerMessagingTest, LoadVersionResponseIsSentWhenARestoredObjectPoolParses)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x29, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x84, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.versionToLoad = read_test_pool();
	ASSERT_FALSE(serverUnderTest.versionToLoad.empty());
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundLoadVersionResponse = poll_for_response(serverUnderTest, testPlugin, 0xD1, responseFrame);
	const std::uint8_t loadVersionErrorCodes = responseFrame.data[5];

	// Objects transferred on top of the restored pool get the normal End of Object Pool response
	receive_from_client(internalECU, client, { 0x11, 0x00, 0xF0, 0x15, 0x00, 0x00, 0x00, 0x00 });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	const bool foundEndOfObjectPoolResponse = poll_for_response(serverUnderTest, testPlugin, 0x12, responseFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundLoadVersionResponse);
	EXPECT_EQ(0x00, loadVersionErrorCodes);
	ASSERT_TRUE(foundEndOfObjectPoolResponse);
	EXPECT_EQ(0x00, responseFrame.data[1]); // No error in object pool
}

TEST_F(VirtualTerminalServerMessagingTest, LoadVersionResponseReportsPoolDataCorruptionWhenARestoredObjectPoolFailsToParse)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x2A, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x85, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.version = VirtualTerminalBase::VTVersion::Version4;
	serverUnderTest.versionToLoad = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(serverUnderTest, testPlugin, 0xD1, responseFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x01, responseFrame.data[5]); // File system error or pool data corruption
}

TEST_F(VirtualTerminalServerMessagingTest, LoadVersionResponseReportsAnyOtherErrorWhenARestoredObjectPoolFailsToParseOnVersion3)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x2C, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x87, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.versionToLoad = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(serverUnderTest, testPlugin, 0xD1, responseFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x08, responseFrame.data[5]); // Any other error
}

TEST_F(VirtualTerminalServerMessagingTest, LoadVersionResponseReportsAnUnknownLabelWhenNoPoolIsStored)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x2B, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x86, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(serverUnderTest, testPlugin, 0xD1, responseFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x02, responseFrame.data[5]); // Version label is not correct or unknown
}

TEST_F(VirtualTerminalServerMessagingTest, LoadVersionResponseIsSentWhenObjectsArriveBeforeItIsAnswered)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x2D, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x88, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.versionToLoad = read_test_pool();
	ASSERT_FALSE(serverUnderTest.versionToLoad.empty());
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	serverUnderTest.wait_for_object_pool_to_parse();
	receive_from_client(internalECU, client, { 0x11, 0x00, 0xF0, 0x15, 0x00, 0x00, 0x00, 0x00 });
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundLoadVersionResponse = poll_for_response(serverUnderTest, testPlugin, 0xD1, responseFrame);
	const std::uint8_t loadVersionErrorCodes = responseFrame.data[5];

	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	const bool foundEndOfObjectPoolResponse = poll_for_response(serverUnderTest, testPlugin, 0x12, responseFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundLoadVersionResponse);
	EXPECT_EQ(0x00, loadVersionErrorCodes);
	ASSERT_TRUE(foundEndOfObjectPoolResponse);
	EXPECT_EQ(0x00, responseFrame.data[1]); // No error in object pool
}

TEST_F(VirtualTerminalServerMessagingTest, LoadVersionWithAnUnknownLabelDoesNotAnswerForAnExistingObjectPool)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x2E, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x89, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x11, 0x00, 0xF0, 0x15, 0x00, 0x00, 0x00, 0x00 });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

	CANMessageFrame responseFrame = {};
	const bool foundEndOfObjectPoolResponse = poll_for_response(serverUnderTest, testPlugin, 0x12, responseFrame);

	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });

	const bool foundLoadVersionResponse = poll_for_response(serverUnderTest, testPlugin, 0xD1, responseFrame);
	const std::uint8_t loadVersionErrorCodes = responseFrame.data[5];
	const bool foundSecondEndOfObjectPoolResponse = poll_for_response(serverUnderTest, testPlugin, 0x12, responseFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundEndOfObjectPoolResponse);
	ASSERT_TRUE(foundLoadVersionResponse);
	EXPECT_EQ(0x02, loadVersionErrorCodes); // Version label is not correct or unknown
	EXPECT_FALSE(foundSecondEndOfObjectPoolResponse);
}

TEST_F(VirtualTerminalServerMessagingTest, StatusMessageReportsParsingUntilTheLoadVersionResponseIsSent)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x2F, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x8A, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.versionToLoad = read_test_pool();
	ASSERT_FALSE(serverUnderTest.versionToLoad.empty());
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	serverUnderTest.wait_for_object_pool_to_parse();

	std::uint8_t busyCodesBeforeResponse = 0;
	const bool foundStatusBeforeResponse = read_status_busy_codes(serverUnderTest, testPlugin, busyCodesBeforeResponse);

	CANMessageFrame responseFrame = {};
	const bool foundLoadVersionResponse = poll_for_response(serverUnderTest, testPlugin, 0xD1, responseFrame);

	std::uint8_t busyCodesAfterResponse = 0xFF;
	const bool foundStatusAfterResponse = read_status_busy_codes(serverUnderTest, testPlugin, busyCodesAfterResponse);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundStatusBeforeResponse);
	EXPECT_EQ(0x10, busyCodesBeforeResponse); // VT is busy parsing an object pool
	ASSERT_TRUE(foundLoadVersionResponse);
	ASSERT_TRUE(foundStatusAfterResponse);
	EXPECT_EQ(0x00, busyCodesAfterResponse);
}

TEST_F(VirtualTerminalServerMessagingTest, StatusMessageReportsParsingUntilTheEndOfObjectPoolResponseIsSent)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x30, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x8B, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.busyCodesBitfield = 0x40; // VT is in auxiliary learn mode
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x11, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	serverUnderTest.wait_for_object_pool_to_parse();

	std::uint8_t busyCodesBeforeResponse = 0;
	const bool foundStatusBeforeResponse = read_status_busy_codes(serverUnderTest, testPlugin, busyCodesBeforeResponse);

	CANMessageFrame responseFrame = {};
	const bool foundEndOfObjectPoolResponse = poll_for_response(serverUnderTest, testPlugin, 0x12, responseFrame);

	std::uint8_t busyCodesAfterResponse = 0xFF;
	const bool foundStatusAfterResponse = read_status_busy_codes(serverUnderTest, testPlugin, busyCodesAfterResponse);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundStatusBeforeResponse);
	EXPECT_EQ(0x50, busyCodesBeforeResponse); // Parsing on top of auxiliary learn mode
	ASSERT_TRUE(foundEndOfObjectPoolResponse);
	ASSERT_TRUE(foundStatusAfterResponse);
	EXPECT_EQ(0x40, busyCodesAfterResponse);
}

TEST_F(VirtualTerminalServerMessagingTest, StatusMessageDoesNotReportParsingOnVersion2OrOlder)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x31, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x8C, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.version = VirtualTerminalBase::VTVersion::Version2OrOlder;
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x11, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	serverUnderTest.wait_for_object_pool_to_parse();

	std::uint8_t busyCodes = 0xFF;
	const bool foundStatus = read_status_busy_codes(serverUnderTest, testPlugin, busyCodes);
	serverUnderTest.run_update(); // Joins the parsing thread, which must not outlive the working set
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundStatus);
	EXPECT_EQ(0x00, busyCodes);
}

class TestWorkingSet : public VirtualTerminalServerManagedWorkingSet
{
public:
	explicit TestWorkingSet(std::shared_ptr<ControlFunction> controlFunction) :
	  VirtualTerminalServerManagedWorkingSet(controlFunction)
	{
	}

	bool add_object(std::shared_ptr<VTObject> object)
	{
		return add_or_replace_object(object);
	}
};

TEST_F(VirtualTerminalServerMessagingTest, MacroExecutesLongMacro)
{
	auto serverControlFunction = test_helpers::create_mock_internal_control_function(0x80);
	auto clientControlFunction = test_helpers::create_mock_control_function(0x81);
	DerivedTestVTServer server(serverControlFunction);
	auto workingSet = std::make_shared<TestWorkingSet>(clientControlFunction);

	auto stringVariable = std::make_shared<StringVariable>();
	stringVariable->set_id(0x1234);
	stringVariable->set_value("Open-Agriculture");
	ASSERT_TRUE(workingSet->add_object(stringVariable));

	auto macro = std::make_shared<Macro>();
	macro->set_id(0x5678);
	ASSERT_TRUE(macro->add_command_packet({ static_cast<std::uint8_t>(isobus::Macro::Command::ChangeStringValue),
	                                        0x34,
	                                        0x12,
	                                        0x10,
	                                        0x00,
	                                        'S',
	                                        'u',
	                                        'c',
	                                        'c',
	                                        'e',
	                                        's',
	                                        's',
	                                        ' ',
	                                        ' ',
	                                        ' ',
	                                        ' ',
	                                        ' ',
	                                        ' ',
	                                        ' ',
	                                        ' ',
	                                        ' ' }));
	ASSERT_TRUE(workingSet->add_object(macro));
	server.add_managed_working_set(workingSet);

	EXPECT_TRUE(server.execute_macro_for_test(macro->get_id(), workingSet));
	EXPECT_EQ("Success         ", stringVariable->get_value());
}
