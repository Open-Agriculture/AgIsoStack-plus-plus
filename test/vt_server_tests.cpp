//================================================================================================
/// @file vt_server_tests.cpp
///
/// @brief Unit tests for the VirtualTerminalServer class.
/// @author Open-Agriculture
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include <gtest/gtest.h>

#include <algorithm>
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

	std::vector<std::array<std::uint8_t, 32>> get_extended_versions(NAME) override
	{
		return extendedVersions;
	}

	std::vector<std::uint8_t> get_supported_objects() const override
	{
		return {};
	}

	std::vector<std::uint8_t> load_version(const std::vector<std::uint8_t> &versionLabel, NAME) override
	{
		lastVersionLabel = versionLabel;
		return versionToLoad;
	}

	bool save_version(const std::vector<std::uint8_t> &, const std::vector<std::uint8_t> &versionLabel, NAME) override
	{
		lastVersionLabel = versionLabel;
		return saveSucceeds;
	}

	bool delete_version(const std::vector<std::uint8_t> &versionLabel, NAME) override
	{
		lastVersionLabel = versionLabel;
		return deleteSucceeds;
	}

	bool delete_all_versions(std::uint8_t versionLabelLength, NAME) override
	{
		deletedAllVersionsLabelLength = versionLabelLength;
		return deleteSucceeds;
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
	using VirtualTerminalServer::managedWorkingSetList;
	using VirtualTerminalServer::send_status_message;

	std::vector<std::uint8_t> versionToLoad;
	VTVersion version = VTVersion::Version3;
	bool saveSucceeds = true;
	bool deleteSucceeds = true;
	std::uint8_t deletedAllVersionsLabelLength = 0;
	std::vector<std::array<std::uint8_t, 32>> extendedVersions;
	std::vector<std::uint8_t> lastVersionLabel;
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

	static void receive_tp_from_client(std::shared_ptr<InternalControlFunction> server,
	                                   std::shared_ptr<ControlFunction> client,
	                                   const std::vector<std::uint8_t> &payload)
	{
		const auto size = static_cast<std::uint16_t>(payload.size());
		const auto numberOfPackets = static_cast<std::uint8_t>((size + 6) / 7);

		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(7,
		                                                                                                   static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolConnectionManagement),
		                                                                                                   server,
		                                                                                                   client,
		                                                                                                   { 0x10, static_cast<std::uint8_t>(size & 0xFF), static_cast<std::uint8_t>(size >> 8), numberOfPackets, 0xFF, 0x00, 0xE7, 0x00 }));
		CANNetworkManager::CANNetwork.update();

		for (std::uint8_t packet = 0; packet < numberOfPackets; packet++)
		{
			auto frame = test_helpers::create_message_frame(7,
			                                                static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolDataTransfer),
			                                                server,
			                                                client,
			                                                { static_cast<std::uint8_t>(packet + 1), 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
			for (std::size_t i = 0; (i < 7) && ((packet * 7U + i) < size); i++)
			{
				frame.data[i + 1] = payload[packet * 7U + i];
			}
			CANNetworkManager::CANNetwork.process_receive_can_message_frame(frame);
			CANNetworkManager::CANNetwork.update();
		}
	}

	static std::vector<std::uint8_t> extended_version_command(std::uint8_t function, const std::string &label)
	{
		std::vector<std::uint8_t> command(33, ' ');
		command[0] = function;
		std::copy(label.begin(), label.end(), command.begin() + 1);
		return command;
	}

	static std::uint8_t get_pdu_format(const CANMessageFrame &frame)
	{
		return static_cast<std::uint8_t>(frame.identifier >> 16);
	}

	bool read_tp_from_server(VirtualCANPlugin &plugin,
	                         std::shared_ptr<InternalControlFunction> server,
	                         std::shared_ptr<ControlFunction> client,
	                         std::vector<std::uint8_t> &payload)
	{
		CANMessageFrame frame = {};
		bool foundRequestToSend = false;
		std::uint16_t size = 0;

		for (std::uint_fast8_t attempt = 0; (attempt < 50) && (!foundRequestToSend); attempt++)
		{
			CANNetworkManager::CANNetwork.update();
			time_source.update_for_ms(5);
			while ((!foundRequestToSend) && plugin.read_frame(frame, 10))
			{
				foundRequestToSend = (0xEC == get_pdu_format(frame)) && (0x10 == frame.data[0]);
			}
		}

		if (foundRequestToSend)
		{
			size = static_cast<std::uint16_t>(frame.data[1] | (frame.data[2] << 8));
			CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(7,
			                                                                                                   static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolConnectionManagement),
			                                                                                                   server,
			                                                                                                   client,
			                                                                                                   { 0x11, frame.data[3], 0x01, 0xFF, 0xFF, frame.data[5], frame.data[6], frame.data[7] }));

			for (std::uint_fast8_t attempt = 0; (attempt < 50) && (payload.size() < size); attempt++)
			{
				CANNetworkManager::CANNetwork.update();
				time_source.update_for_ms(5);
				while ((payload.size() < size) && plugin.read_frame(frame, 10))
				{
					if (0xEB == get_pdu_format(frame))
					{
						payload.insert(payload.end(), frame.data + 1, frame.data + 8);
					}
				}
			}
			payload.resize(std::min<std::size_t>(payload.size(), size));
		}
		return foundRequestToSend && (payload.size() == size);
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
				// Address claims are on the bus too, and a NAME can start with the function code
				foundResponse = (0xE6 == get_pdu_format(responseFrame)) && (function == responseFrame.data[0]);
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

TEST_F(VirtualTerminalServerMessagingTest, ObjectPoolCommandsAreIgnoredUntilThePreviousObjectPoolIsAnswered)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x32, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x8D, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.versionToLoad = read_test_pool();
	ASSERT_FALSE(serverUnderTest.versionToLoad.empty());
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	serverUnderTest.wait_for_object_pool_to_parse();
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	receive_from_client(internalECU, client, { 0x11, 0x00, 0xF0, 0x15, 0x00, 0x00, 0x00, 0x00 });
	const auto numberOfPoolSegments = serverUnderTest.managedWorkingSetList.front()->get_number_iop_files();
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundLoadVersionResponse = poll_for_response(serverUnderTest, testPlugin, 0xD1, responseFrame);
	CANHardwareInterface::stop();

	EXPECT_EQ(1U, numberOfPoolSegments);
	ASSERT_TRUE(foundLoadVersionResponse);
	EXPECT_EQ(0x00, responseFrame.data[5]);
}

TEST_F(VirtualTerminalServerMessagingTest, StoreVersionResponseReportsAnyOtherErrorWhenTheObjectPoolCannotBeStored)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x33, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x8E, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.saveSucceeds = false;
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x11, 0x00, 0xF0, 0x15, 0x00, 0x00, 0x00, 0x00 });
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD0, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });

	CANMessageFrame responseFrame = {};
	const bool foundStoreVersionResponse = poll_for_response(serverUnderTest, testPlugin, 0xD0, responseFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundStoreVersionResponse);
	EXPECT_EQ(0x08, responseFrame.data[5]);
}

TEST_F(VirtualTerminalServerMessagingTest, ExtendedGetVersionsResponseWithNoVersionsIsPaddedToOneFrame)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x34, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x8F, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(serverUnderTest, testPlugin, 0xD3, responseFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(8U, responseFrame.dataLength);
	EXPECT_EQ(0x00, responseFrame.data[1]);
	for (std::uint8_t i = 2; i < 8; i++)
	{
		EXPECT_EQ(0xFF, responseFrame.data[i]);
	}
}

TEST_F(VirtualTerminalServerMessagingTest, ExtendedGetVersionsResponseCarriesThe32ByteLabels)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x35, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x90, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	std::array<std::uint8_t, 32> label;
	label.fill(' ');
	std::copy_n("EXTENDED VERSION", 16, label.begin());
	serverUnderTest.extendedVersions = { label };
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

	std::vector<std::uint8_t> response;
	const bool foundResponse = read_tp_from_server(testPlugin, internalECU, client, response);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundResponse);
	ASSERT_EQ(34U, response.size());
	EXPECT_EQ(0xD3, response[0]);
	EXPECT_EQ(1, response[1]);
	EXPECT_TRUE(std::equal(label.begin(), label.end(), response.begin() + 2));
}

TEST_F(VirtualTerminalServerMessagingTest, ExtendedLoadVersionResponseIsSentWhenARestoredObjectPoolParses)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x36, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x91, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.versionToLoad = read_test_pool();
	ASSERT_FALSE(serverUnderTest.versionToLoad.empty());
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const auto command = extended_version_command(0xD5, "EXTENDED VERSION");
	receive_tp_from_client(internalECU, client, command);
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(serverUnderTest, testPlugin, 0xD5, responseFrame);
	CANHardwareInterface::stop();

	EXPECT_EQ(std::vector<std::uint8_t>(command.begin() + 1, command.end()), serverUnderTest.lastVersionLabel);
	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x00, responseFrame.data[5]);
}

TEST_F(VirtualTerminalServerMessagingTest, ExtendedLoadVersionResponseReportsAnUnknownLabelWhenNoPoolIsStored)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x92, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	receive_tp_from_client(internalECU, client, extended_version_command(0xD5, "UNKNOWN"));

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(serverUnderTest, testPlugin, 0xD5, responseFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x02, responseFrame.data[5]); // Version label not correct or unknown
}

TEST_F(VirtualTerminalServerMessagingTest, ExtendedLoadVersionResponseReportsPoolDataCorruptionWhenARestoredObjectPoolFailsToParse)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x38, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x93, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.version = VirtualTerminalBase::VTVersion::Version5;
	serverUnderTest.versionToLoad = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_tp_from_client(internalECU, client, extended_version_command(0xD5, "EXTENDED VERSION"));
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(serverUnderTest, testPlugin, 0xD5, responseFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x01, responseFrame.data[5]); // File system error or pool data corruption
}

TEST_F(VirtualTerminalServerMessagingTest, ExtendedLoadVersionIsIgnoredUntilThePreviousObjectPoolIsAnswered)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x39, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x94, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.versionToLoad = read_test_pool();
	ASSERT_FALSE(serverUnderTest.versionToLoad.empty());
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_tp_from_client(internalECU, client, extended_version_command(0xD5, "EXTENDED VERSION"));
	serverUnderTest.wait_for_object_pool_to_parse();
	receive_tp_from_client(internalECU, client, extended_version_command(0xD5, "EXTENDED VERSION"));
	const auto numberOfPoolSegments = serverUnderTest.managedWorkingSetList.front()->get_number_iop_files();
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(serverUnderTest, testPlugin, 0xD5, responseFrame);
	CANHardwareInterface::stop();

	EXPECT_EQ(1U, numberOfPoolSegments);
	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x00, responseFrame.data[5]);
}

TEST_F(VirtualTerminalServerMessagingTest, ExtendedVersionCommandTooShortForItsLabelIsIgnored)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x3A, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x95, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD5, 'S', 'H', 'O', 'R', 'T', ' ', ' ' });

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(serverUnderTest, testPlugin, 0xD5, responseFrame);
	CANHardwareInterface::stop();

	EXPECT_FALSE(foundResponse);
	EXPECT_TRUE(serverUnderTest.lastVersionLabel.empty());
}

TEST_F(VirtualTerminalServerMessagingTest, ExtendedStoreVersionStoresThePoolUnderThe32ByteLabel)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x3B, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x96, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x11, 0x00, 0xF0, 0x15, 0x00, 0x00, 0x00, 0x00 });
	testPlugin.clear_queue();
	receive_tp_from_client(internalECU, client, extended_version_command(0xD4, "EXTENDED VERSION"));

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(serverUnderTest, testPlugin, 0xD4, responseFrame);
	CANHardwareInterface::stop();

	EXPECT_EQ(32U, serverUnderTest.lastVersionLabel.size());
	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x00, responseFrame.data[5]);
}

TEST_F(VirtualTerminalServerMessagingTest, ExtendedDeleteVersionResponseReportsAnUnknownLabel)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x3C, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x97, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	receive_tp_from_client(internalECU, client, extended_version_command(0xD6, "EXTENDED VERSION"));

	CANMessageFrame deletedFrame = {};
	const bool foundDeletedResponse = poll_for_response(serverUnderTest, testPlugin, 0xD6, deletedFrame);

	serverUnderTest.deleteSucceeds = false;
	testPlugin.clear_queue();
	receive_tp_from_client(internalECU, client, extended_version_command(0xD6, "UNKNOWN"));

	CANMessageFrame unknownFrame = {};
	const bool foundUnknownResponse = poll_for_response(serverUnderTest, testPlugin, 0xD6, unknownFrame);
	CANHardwareInterface::stop();

	EXPECT_EQ(32U, serverUnderTest.lastVersionLabel.size());
	ASSERT_TRUE(foundDeletedResponse);
	EXPECT_EQ(0x00, deletedFrame.data[5]);
	ASSERT_TRUE(foundUnknownResponse);
	EXPECT_EQ(0x02, unknownFrame.data[5]); // Version label not correct or unknown
}

TEST_F(VirtualTerminalServerMessagingTest, DeleteVersionWildcardDeletesAllVersionsOnlyOnVersion6)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x3D, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x99, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	serverUnderTest.version = VirtualTerminalBase::VTVersion::Version5;
	testPlugin.clear_queue();
	receive_tp_from_client(internalECU, client, extended_version_command(0xD6, "*"));

	CANMessageFrame version5Frame = {};
	const bool foundVersion5Response = poll_for_response(serverUnderTest, testPlugin, 0xD6, version5Frame);
	const auto version5DeletedAllLength = serverUnderTest.deletedAllVersionsLabelLength;
	const auto version5Label = serverUnderTest.lastVersionLabel;

	serverUnderTest.version = VirtualTerminalBase::VTVersion::Version6;
	serverUnderTest.lastVersionLabel.clear();
	testPlugin.clear_queue();
	receive_tp_from_client(internalECU, client, extended_version_command(0xD6, "*"));

	CANMessageFrame version6Frame = {};
	const bool foundVersion6Response = poll_for_response(serverUnderTest, testPlugin, 0xD6, version6Frame);
	const auto version6DeletedAllLength = serverUnderTest.deletedAllVersionsLabelLength;
	const bool version6CalledDeleteVersion = !serverUnderTest.lastVersionLabel.empty();

	serverUnderTest.deletedAllVersionsLabelLength = 0;
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD2, '*', ' ', ' ', ' ', ' ', ' ', ' ' });

	CANMessageFrame shortWildcardFrame = {};
	const bool foundShortWildcardResponse = poll_for_response(serverUnderTest, testPlugin, 0xD2, shortWildcardFrame);
	const auto shortWildcardDeletedAllLength = serverUnderTest.deletedAllVersionsLabelLength;
	const bool shortWildcardCalledDeleteVersion = !serverUnderTest.lastVersionLabel.empty();

	serverUnderTest.deletedAllVersionsLabelLength = 0;
	testPlugin.clear_queue();
	receive_tp_from_client(internalECU, client, extended_version_command(0xD6, "A"));

	CANMessageFrame otherLabelFrame = {};
	const bool foundOtherLabelResponse = poll_for_response(serverUnderTest, testPlugin, 0xD6, otherLabelFrame);
	const auto otherLabelDeletedAllLength = serverUnderTest.deletedAllVersionsLabelLength;
	const auto otherLabelSize = serverUnderTest.lastVersionLabel.size();

	serverUnderTest.deleteSucceeds = false;
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD2, '*', ' ', ' ', ' ', ' ', ' ', ' ' });

	CANMessageFrame failedWildcardFrame = {};
	const bool foundFailedWildcardResponse = poll_for_response(serverUnderTest, testPlugin, 0xD2, failedWildcardFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundVersion5Response);
	EXPECT_EQ(0, version5DeletedAllLength);
	EXPECT_EQ(32U, version5Label.size());
	ASSERT_TRUE(foundVersion6Response);
	EXPECT_EQ(0x00, version6Frame.data[5]);
	EXPECT_EQ(32, version6DeletedAllLength);
	EXPECT_FALSE(version6CalledDeleteVersion);
	ASSERT_TRUE(foundShortWildcardResponse);
	EXPECT_EQ(0x00, shortWildcardFrame.data[5]);
	EXPECT_EQ(7, shortWildcardDeletedAllLength);
	EXPECT_FALSE(shortWildcardCalledDeleteVersion);
	ASSERT_TRUE(foundOtherLabelResponse);
	EXPECT_EQ(0, otherLabelDeletedAllLength);
	EXPECT_EQ(32U, otherLabelSize);
	ASSERT_TRUE(foundFailedWildcardResponse);
	EXPECT_EQ(0x08, failedWildcardFrame.data[5]);
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
