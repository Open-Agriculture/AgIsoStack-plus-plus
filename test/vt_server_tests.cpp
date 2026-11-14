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
#include <functional>
#include <thread>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_virtual_terminal_server.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"
#include "helpers/object_pool_helpers.hpp"
#include "helpers/test_fixture.hpp"

using namespace isobus;

/// @brief Just over the 3 second working set maintenance timeout of ISO 11783-6 4.6.9
static constexpr std::uint32_t JUST_PAST_MAINTENANCE_TIMEOUT_MS = 3001;

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
		return supportedObjects;
	}

	std::vector<std::uint8_t> load_version(const std::vector<std::uint8_t> &versionLabel, NAME) override
	{
		lastVersionLabel = versionLabel;
		return versionToLoad;
	}

	bool save_version(const std::vector<std::uint8_t> &objectPool, const std::vector<std::uint8_t> &versionLabel, NAME) override
	{
		lastVersionLabel = versionLabel;
		if (saveSucceeds)
		{
			storedVersion.push_back(objectPool);
		}
		return saveSucceeds;
	}

	bool delete_version(const std::vector<std::uint8_t> &versionLabel, NAME) override
	{
		lastVersionLabel = versionLabel;
		deletedVersionLabels.push_back(versionLabel);
		storedVersion.clear();
		return deleteSucceeds;
	}

	bool delete_all_versions(std::uint8_t versionLabelLength, NAME) override
	{
		deletedAllVersionsLabelLength = versionLabelLength;
		return deleteSucceeds;
	}

	bool delete_object_pool(NAME) override
	{
		objectPoolDeletionRequests++;
		return true;
	}

	void handle_unexpected_working_set_shutdown(std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet, UnexpectedShutdownReason reason) override
	{
		shutDownWorkingSets.push_back(workingSet);
		shutdownReasons.push_back(reason);
	}

	std::uint16_t abort_input(std::shared_ptr<VirtualTerminalServerManagedWorkingSet>) override
	{
		return openInputObjectID;
	}

	void object_pool_parse_finished(std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet, bool success) override
	{
		parsedWorkingSets.push_back(workingSet);
		parseResults.push_back(success);
		activeWorkingSetsWhenParsed.push_back(get_active_working_set());
		if (onObjectPoolParseFinished)
		{
			onObjectPoolParseFinished(workingSet);
		}
	}

	void object_pool_deleted(std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet) override
	{
		deletedObjectPools.push_back(workingSet);
		activeWorkingSetsWhenDeleted.push_back(get_active_working_set());
		if (onObjectPoolDeleted)
		{
			onObjectPoolDeleted(workingSet);
		}
	}

	void select_working_set(std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet) override
	{
		workingSetsOfferedForSelection.push_back(workingSet);
		if (selectOfferedWorkingSets)
		{
			VirtualTerminalServer::select_working_set(workingSet);
		}
	}

	void control_audio_signal(std::uint8_t activations, std::uint16_t frequencyHz, std::uint16_t onTimeMs, std::uint16_t offTimeMs, std::shared_ptr<ControlFunction> source) override
	{
		audioSignals.push_back({ activations, frequencyHz, onTimeMs, offTimeMs });
		audioSignalSources.push_back(source);
	}

	void set_audio_volume(std::uint8_t volumePercent, std::shared_ptr<ControlFunction> source) override
	{
		audioVolumes.push_back(volumePercent);
		audioVolumeSources.push_back(source);
	}

	void run_update()
	{
		update();
	}

	void wait_for_object_pool_to_parse(std::size_t workingSetIndex = 0)
	{
		for (std::uint_fast8_t attempt = 0; attempt < 200; attempt++)
		{
			const auto state = managedWorkingSetList.at(workingSetIndex)->get_object_pool_processing_state();

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
	std::vector<std::vector<std::uint8_t>> storedVersion;
	std::vector<std::vector<std::uint8_t>> deletedVersionLabels;
	std::vector<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>> shutDownWorkingSets;
	std::vector<UnexpectedShutdownReason> shutdownReasons;
	std::vector<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>> parsedWorkingSets;
	std::vector<bool> parseResults;
	std::vector<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>> activeWorkingSetsWhenParsed;
	std::vector<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>> workingSetsOfferedForSelection;
	bool selectOfferedWorkingSets = true;
	std::vector<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>> deletedObjectPools;
	std::vector<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>> activeWorkingSetsWhenDeleted;
	std::uint16_t openInputObjectID = NULL_OBJECT_ID;
	std::function<void(std::shared_ptr<VirtualTerminalServerManagedWorkingSet>)> onObjectPoolParseFinished;
	std::function<void(std::shared_ptr<VirtualTerminalServerManagedWorkingSet>)> onObjectPoolDeleted;
	std::uint_fast8_t objectPoolDeletionRequests = 0;
	std::vector<std::uint8_t> supportedObjects;
	std::vector<std::array<std::uint16_t, 4>> audioSignals;
	std::vector<std::shared_ptr<ControlFunction>> audioSignalSources;
	std::vector<std::uint8_t> audioVolumes;
	std::vector<std::shared_ptr<ControlFunction>> audioVolumeSources;
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

	// Version 2 and prior clients have to send this before they can be managed
	static void receive_working_set_master_from_client(std::shared_ptr<ControlFunction> client)
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(5,
		                                                                                                             static_cast<std::uint32_t>(CANLibParameterGroupNumber::WorkingSetMaster),
		                                                                                                             client,
		                                                                                                             { 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }));
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
			const CANMessageFrame requestToSend = frame;
			size = static_cast<std::uint16_t>(requestToSend.data[1] | (requestToSend.data[2] << 8));
			CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(7,
			                                                                                                   static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolConnectionManagement),
			                                                                                                   server,
			                                                                                                   client,
			                                                                                                   { 0x11, requestToSend.data[3], 0x01, 0xFF, 0xFF, requestToSend.data[5], requestToSend.data[6], requestToSend.data[7] }));

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
			// Without this ack the server refuses the next session
			CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(7,
			                                                                                                   static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolConnectionManagement),
			                                                                                                   server,
			                                                                                                   client,
			                                                                                                   { 0x13, requestToSend.data[1], requestToSend.data[2], requestToSend.data[3], 0xFF, requestToSend.data[5], requestToSend.data[6], requestToSend.data[7] }));
			CANNetworkManager::CANNetwork.update();
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

	bool read_status_message(const DerivedTestVTServer &server, VirtualCANPlugin &plugin, CANMessageFrame &statusFrame)
	{
		bool foundStatus = false;

		plugin.clear_queue();
		server.send_status_message();
		time_source.update_for_ms(5);
		while ((!foundStatus) && plugin.read_frame(statusFrame, 10))
		{
			foundStatus = (0xFE == statusFrame.data[0]);
		}
		return foundStatus;
	}

	bool is_nacked(std::shared_ptr<InternalControlFunction> server,
	               std::shared_ptr<ControlFunction> client,
	               VirtualCANPlugin &plugin,
	               std::initializer_list<std::uint8_t> data)
	{
		CANMessageFrame frame = {};
		bool foundNack = false;

		plugin.clear_queue();
		receive_from_client(server, client, data);
		time_source.update_for_ms(5);
		while ((!foundNack) && plugin.read_frame(frame, 10))
		{
			foundNack = (0xE8 == get_pdu_format(frame)) && (0x01 == frame.data[0]);
		}
		return foundNack;
	}

	// A managed working set without an object pool gets no response to an End of Object Pool message, an unmanaged one gets a NACK
	bool is_managed(std::shared_ptr<InternalControlFunction> server, std::shared_ptr<ControlFunction> client, VirtualCANPlugin &plugin)
	{
		return !is_nacked(server, client, plugin, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	}

	bool connect_with_selectable_object_pool(DerivedTestVTServer &server,
	                                         VirtualCANPlugin &plugin,
	                                         std::shared_ptr<InternalControlFunction> serverCF,
	                                         std::shared_ptr<ControlFunction> client)
	{
		CANMessageFrame response = {};

		receive_from_client(serverCF, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		receive_tp_from_client(serverCF, client, { 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF });
		receive_from_client(serverCF, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		plugin.clear_queue();
		return poll_for_response(server, plugin, 0x12, response) && (0x00 == response.data[1]);
	}

	// Also holds Output Polygon 2 (100x50, points (0,0) (100,0) (33,17)), Output Line 3 (10x20, direction 0)
	// and Macro 5, which Line 3 runs On Change End Point to resize Polygon 2 to 7x8
	bool connect_with_polygon_and_line_pool(DerivedTestVTServer &server,
	                                        VirtualCANPlugin &plugin,
	                                        std::shared_ptr<InternalControlFunction> serverCF,
	                                        std::shared_ptr<ControlFunction> client)
	{
		CANMessageFrame response = {};

		receive_from_client(serverCF, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		receive_tp_from_client(serverCF, client, { 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF, 0x02, 0x00, 0x10, 0x64, 0x00, 0x32, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x21, 0x00, 0x11, 0x00, 0x03, 0x00, 0x0D, 0xFF, 0xFF, 0x0A, 0x00, 0x14, 0x00, 0x00, 0x01, 0x12, 0x05, 0x05, 0x00, 0x1C, 0x08, 0x00, 0xA6, 0x02, 0x00, 0x07, 0x00, 0x08, 0x00, 0xFF });
		receive_from_client(serverCF, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		plugin.clear_queue();
		return poll_for_response(server, plugin, 0x12, response) && (0x00 == response.data[1]);
	}

	bool read_status_busy_codes(const DerivedTestVTServer &server, VirtualCANPlugin &plugin, std::uint8_t &busyCodes)
	{
		CANMessageFrame statusFrame = {};
		const bool foundStatus = read_status_message(server, plugin, statusFrame);
		busyCodes = statusFrame.data[6];
		return foundStatus;
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

TEST_F(VirtualTerminalServerMessagingTest, BroadcastFromUnmanagedClientIsNotNacked)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x51, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xCB, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();
	testPlugin.clear_queue();

	// An input unit broadcasts its Auxiliary Input Type 2 Maintenance message before it connects (ISO 11783-6 J.7.10).
	// A NACK makes it drop the connection it is starting.
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(3,
	                                                                                                             static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
	                                                                                                             client,
	                                                                                                             { 0x23, 0x34, 0x12, 0x00, 0xFF, 0xFF, 0xFF, 0xFF }));
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);

	CANMessageFrame frame = {};
	bool foundNack = false;
	while (testPlugin.read_frame(frame, 10))
	{
		foundNack = foundNack || ((0xE8 == get_pdu_format(frame)) && (0x01 == frame.data[0]));
	}
	EXPECT_FALSE(foundNack);

	// The same client is still NACKed for a message sent to the VT
	EXPECT_TRUE(is_nacked(internalECU, client, testPlugin, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }));

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
	ASSERT_EQ(1U, serverUnderTest.parseResults.size());
	EXPECT_FALSE(serverUnderTest.parseResults.front());
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

TEST_F(VirtualTerminalServerMessagingTest, EndOfObjectPoolResponseReportsTheParentOfTheFaultyObject)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x41, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x9E, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	// Working Set 258 with child 1, then object 1 of an unknown type
	receive_tp_from_client(internalECU, client, { 0x11, 0x02, 0x01, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xAA, 0xAA });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(serverUnderTest, testPlugin, 0x12, responseFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x01, responseFrame.data[1]);
	EXPECT_EQ(258, responseFrame.data[2] | (responseFrame.data[3] << 8));
	EXPECT_EQ(1, responseFrame.data[4] | (responseFrame.data[5] << 8));
}

TEST_F(VirtualTerminalServerMessagingTest, EndOfObjectPoolResponseReportsNoParentWhenTheFaultyObjectHasNoID)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x42, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x9F, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	// Input List 2 with one NULL item, then 3 bytes, too short to hold an object
	receive_tp_from_client(internalECU, client, { 0x11, 0x02, 0x00, 0x0A, 0x10, 0x00, 0x10, 0x00, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x01 });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(serverUnderTest, testPlugin, 0x12, responseFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x01, responseFrame.data[1]);
	EXPECT_EQ(NULL_OBJECT_ID, responseFrame.data[2] | (responseFrame.data[3] << 8));
	EXPECT_EQ(NULL_OBJECT_ID, responseFrame.data[4] | (responseFrame.data[5] << 8));
	EXPECT_NE(nullptr, serverUnderTest.managedWorkingSetList.front()->get_object_by_id(2));
}

TEST_F(VirtualTerminalServerMessagingTest, FirstSelectableWorkingSetIsSelectedAutomatically)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x3E, 0, time_source);
	auto notSelectableClient = test_helpers::force_claim_partnered_control_function(0x9A, 0);
	auto selectableClient = test_helpers::force_claim_partnered_control_function(0x9B, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	CANMessageFrame notSelectableResponse = {};
	CANMessageFrame selectableResponse = {};
	CANMessageFrame statusAfterNotSelectable = {};
	CANMessageFrame statusAfterSelectable = {};

	receive_from_client(internalECU, notSelectableClient, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	// Working Set with Selectable (byte 5) = 0
	receive_tp_from_client(internalECU, notSelectableClient, { 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF });
	receive_from_client(internalECU, notSelectableClient, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	const bool notSelectablePoolAnswered = poll_for_response(serverUnderTest, testPlugin, 0x12, notSelectableResponse);
	const bool foundStatusAfterNotSelectable = read_status_message(serverUnderTest, testPlugin, statusAfterNotSelectable);

	receive_from_client(internalECU, selectableClient, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_tp_from_client(internalECU, selectableClient, { 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF });
	receive_from_client(internalECU, selectableClient, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	const bool selectablePoolAnswered = poll_for_response(serverUnderTest, testPlugin, 0x12, selectableResponse);
	const bool foundStatusAfterSelectable = read_status_message(serverUnderTest, testPlugin, statusAfterSelectable);
	CANHardwareInterface::stop();

	ASSERT_TRUE(notSelectablePoolAnswered);
	EXPECT_EQ(0x00, notSelectableResponse.data[1]);
	ASSERT_TRUE(foundStatusAfterNotSelectable);
	EXPECT_EQ(NULL_CAN_ADDRESS, statusAfterNotSelectable.data[1]);
	ASSERT_TRUE(selectablePoolAnswered);
	EXPECT_EQ(0x00, selectableResponse.data[1]);
	ASSERT_TRUE(foundStatusAfterSelectable);
	EXPECT_EQ(0x9B, statusAfterSelectable.data[1]);
	EXPECT_EQ(0x03E8, statusAfterSelectable.data[2] | (statusAfterSelectable.data[3] << 8));
}

TEST_F(VirtualTerminalServerMessagingTest, ObjectPoolParseFinishedIsCalledWithTheWorkingSetAlreadyActive)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x68, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xB9, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	const bool poolLoaded = connect_with_selectable_object_pool(serverUnderTest, testPlugin, internalECU, client);
	CANHardwareInterface::stop();

	ASSERT_TRUE(poolLoaded);
	ASSERT_EQ(1U, serverUnderTest.parsedWorkingSets.size());
	EXPECT_TRUE(serverUnderTest.parseResults.front());
	EXPECT_EQ(serverUnderTest.managedWorkingSetList.front(), serverUnderTest.parsedWorkingSets.front());
	EXPECT_EQ(serverUnderTest.parsedWorkingSets.front(), serverUnderTest.activeWorkingSetsWhenParsed.front());
}

TEST_F(VirtualTerminalServerMessagingTest, ApplicationCanLeaveAnAcceptedWorkingSetInactive)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x6B, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xBC, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.selectOfferedWorkingSets = false;
	serverUnderTest.initialize();

	CANMessageFrame statusFrame = {};

	const bool poolLoaded = connect_with_selectable_object_pool(serverUnderTest, testPlugin, internalECU, client);
	const bool foundStatus = read_status_message(serverUnderTest, testPlugin, statusFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(poolLoaded);
	ASSERT_EQ(1U, serverUnderTest.workingSetsOfferedForSelection.size());
	EXPECT_EQ(serverUnderTest.managedWorkingSetList.front(), serverUnderTest.workingSetsOfferedForSelection.front());
	ASSERT_EQ(1U, serverUnderTest.activeWorkingSetsWhenParsed.size());
	EXPECT_EQ(nullptr, serverUnderTest.activeWorkingSetsWhenParsed.front());
	ASSERT_TRUE(foundStatus);
	EXPECT_EQ(NULL_CAN_ADDRESS, statusFrame.data[1]);
}

TEST_F(VirtualTerminalServerMessagingTest, ExtendingTheActiveObjectPoolDoesNotSelectTheWorkingSetAgain)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x6C, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xBD, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	CANMessageFrame extensionResponse = {};

	const bool poolLoaded = connect_with_selectable_object_pool(serverUnderTest, testPlugin, internalECU, client);
	receive_tp_from_client(internalECU, client, { 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	const bool foundExtensionResponse = poll_for_response(serverUnderTest, testPlugin, 0x12, extensionResponse);
	CANHardwareInterface::stop();

	ASSERT_TRUE(poolLoaded);
	ASSERT_TRUE(foundExtensionResponse);
	EXPECT_EQ(0x00, extensionResponse.data[1]);
	EXPECT_EQ(2U, serverUnderTest.parseResults.size());
	EXPECT_EQ(1U, serverUnderTest.workingSetsOfferedForSelection.size());
	EXPECT_EQ(serverUnderTest.managedWorkingSetList.front(), serverUnderTest.get_active_working_set());
}

TEST_F(VirtualTerminalServerMessagingTest, DeleteObjectPoolDeletesThePoolAndKeepsTheWorkingSetManaged)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x69, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xBA, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	CANMessageFrame deleteResponse = {};
	CANMessageFrame statusAfterDelete = {};
	CANMessageFrame reloadResponse = {};

	const bool poolLoaded = connect_with_selectable_object_pool(serverUnderTest, testPlugin, internalECU, client);
	receive_from_client(internalECU, client, { 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool foundDeleteResponse = poll_for_response(serverUnderTest, testPlugin, 0xB2, deleteResponse);
	const bool foundStatusAfterDelete = read_status_message(serverUnderTest, testPlugin, statusAfterDelete);
	const bool activeCleared = (nullptr == serverUnderTest.get_active_working_set());
	const bool stillManaged = is_managed(internalECU, client, testPlugin);
	const bool poolDeleted = stillManaged &&
	  (nullptr == serverUnderTest.managedWorkingSetList.front()->get_working_set_object()) &&
	  (VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::None == serverUnderTest.managedWorkingSetList.front()->get_object_pool_processing_state());
	receive_tp_from_client(internalECU, client, { 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	const bool foundReloadResponse = poll_for_response(serverUnderTest, testPlugin, 0x12, reloadResponse);
	CANHardwareInterface::stop();

	ASSERT_TRUE(poolLoaded);
	ASSERT_TRUE(foundDeleteResponse);
	EXPECT_EQ(0x00, deleteResponse.data[1]);
	EXPECT_EQ(1U, serverUnderTest.objectPoolDeletionRequests);
	ASSERT_TRUE(foundStatusAfterDelete);
	EXPECT_EQ(NULL_CAN_ADDRESS, statusAfterDelete.data[1]);
	EXPECT_TRUE(activeCleared);
	EXPECT_TRUE(stillManaged);
	EXPECT_TRUE(poolDeleted);
	ASSERT_TRUE(foundReloadResponse);
	EXPECT_EQ(0x00, reloadResponse.data[1]);
	EXPECT_EQ(2U, serverUnderTest.parseResults.size());
	EXPECT_TRUE(serverUnderTest.shutDownWorkingSets.empty());
}

TEST_F(VirtualTerminalServerMessagingTest, ObjectPoolDeletedIsCalledFromUpdate)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x6D, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xBE, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	const bool poolLoaded = connect_with_selectable_object_pool(serverUnderTest, testPlugin, internalECU, client);
	receive_from_client(internalECU, client, { 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const std::size_t deletionsReportedBeforeUpdate = serverUnderTest.deletedObjectPools.size();
	serverUnderTest.run_update();
	serverUnderTest.run_update();
	CANHardwareInterface::stop();

	ASSERT_TRUE(poolLoaded);
	EXPECT_EQ(1U, serverUnderTest.objectPoolDeletionRequests);
	EXPECT_EQ(0U, deletionsReportedBeforeUpdate);
	ASSERT_EQ(1U, serverUnderTest.deletedObjectPools.size());
	EXPECT_EQ(serverUnderTest.managedWorkingSetList.front(), serverUnderTest.deletedObjectPools.front());
	EXPECT_EQ(nullptr, serverUnderTest.activeWorkingSetsWhenDeleted.front());
}

TEST_F(VirtualTerminalServerMessagingTest, WorkingSetIsSelectedBeforeTheParseCallbacksOfTheSameUpdate)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x6E, 0, time_source);
	auto notSelectableClient = test_helpers::force_claim_partnered_control_function(0xBF, 0);
	auto selectableClient = test_helpers::force_claim_partnered_control_function(0xC0, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_from_client(internalECU, notSelectableClient, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_tp_from_client(internalECU, notSelectableClient, { 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF });
	receive_from_client(internalECU, notSelectableClient, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, selectableClient, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_tp_from_client(internalECU, selectableClient, { 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF });
	receive_from_client(internalECU, selectableClient, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	serverUnderTest.wait_for_object_pool_to_parse(0);
	serverUnderTest.wait_for_object_pool_to_parse(1);

	serverUnderTest.onObjectPoolParseFinished = [&](std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet) {
		if (0xBF == workingSet->get_control_function()->get_address())
		{
			receive_from_client(internalECU, selectableClient, { 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		}
	};
	serverUnderTest.run_update();
	CANHardwareInterface::stop();

	ASSERT_EQ(2U, serverUnderTest.parsedWorkingSets.size());
	EXPECT_EQ(0xBF, serverUnderTest.parsedWorkingSets.front()->get_control_function()->get_address());
	ASSERT_EQ(1U, serverUnderTest.workingSetsOfferedForSelection.size());
	EXPECT_EQ(0xC0, serverUnderTest.workingSetsOfferedForSelection.front()->get_control_function()->get_address());
	EXPECT_EQ(serverUnderTest.workingSetsOfferedForSelection.front(), serverUnderTest.activeWorkingSetsWhenParsed.front());
	EXPECT_EQ(nullptr, serverUnderTest.get_active_working_set());
}

TEST_F(VirtualTerminalServerMessagingTest, ObjectPoolDeletedIsCalledBeforeTheNextPoolOfThatWorkingSetIsAccepted)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x6F, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xC1, 0);
	auto otherClient = test_helpers::force_claim_partnered_control_function(0xC2, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.versionToLoad = test_helpers::read_vt3_test_pool();
	ASSERT_FALSE(serverUnderTest.versionToLoad.empty());
	serverUnderTest.initialize();

	CANMessageFrame loadVersionResponse = {};

	const bool poolLoaded = connect_with_selectable_object_pool(serverUnderTest, testPlugin, internalECU, client);
	const bool otherPoolLoaded = connect_with_selectable_object_pool(serverUnderTest, testPlugin, internalECU, otherClient);
	receive_from_client(internalECU, otherClient, { 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

	serverUnderTest.onObjectPoolDeleted = [&](std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet) {
		if (0xC2 == workingSet->get_control_function()->get_address())
		{
			receive_from_client(internalECU, client, { 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
			receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
			serverUnderTest.wait_for_object_pool_to_parse();
		}
	};
	testPlugin.clear_queue();
	const bool foundLoadVersionResponse = poll_for_response(serverUnderTest, testPlugin, 0xD1, loadVersionResponse);
	serverUnderTest.run_update();
	CANHardwareInterface::stop();

	ASSERT_TRUE(poolLoaded);
	ASSERT_TRUE(otherPoolLoaded);
	ASSERT_TRUE(foundLoadVersionResponse);
	EXPECT_EQ(0x00, loadVersionResponse.data[5]);
	ASSERT_EQ(2U, serverUnderTest.deletedObjectPools.size());
	EXPECT_EQ(serverUnderTest.managedWorkingSetList.front(), serverUnderTest.deletedObjectPools.back());
	EXPECT_EQ(nullptr, serverUnderTest.activeWorkingSetsWhenDeleted.back());
	EXPECT_EQ(serverUnderTest.managedWorkingSetList.front(), serverUnderTest.get_active_working_set());
}

TEST_F(VirtualTerminalServerMessagingTest, DeleteObjectPoolIsRejectedUntilThePoolIsAnswered)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x6A, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xBB, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	CANMessageFrame deleteResponse = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_tp_from_client(internalECU, client, { 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	serverUnderTest.wait_for_object_pool_to_parse();
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool foundDeleteResponse = poll_for_response(serverUnderTest, testPlugin, 0xB2, deleteResponse);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundDeleteResponse);
	EXPECT_EQ(0x10, deleteResponse.data[1]); // Any other error
	EXPECT_EQ(0U, serverUnderTest.objectPoolDeletionRequests);
	ASSERT_EQ(1U, serverUnderTest.parseResults.size());
	EXPECT_NE(nullptr, serverUnderTest.managedWorkingSetList.front()->get_working_set_object());
}

TEST_F(VirtualTerminalServerMessagingTest, ChangeActiveMaskRejectsObjectsOfTheWrongType)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x3F, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x9C, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	CANMessageFrame poolResponse = {};
	CANMessageFrame workingSetIsObjectPointerResponse = {};
	CANMessageFrame maskIsObjectPointerResponse = {};
	CANMessageFrame maskIsDataMaskResponse = {};
	CANMessageFrame unknownMaskResponse = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	// Working Set 0 and Data Mask 1000, each with Object Pointer 1 as its child
	receive_tp_from_client(internalECU, client, { 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF, 0xE8, 0x03, 0x01, 0x00, 0xFF, 0xFF, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	const bool poolAnswered = poll_for_response(serverUnderTest, testPlugin, 0x12, poolResponse);

	receive_from_client(internalECU, client, { 0xAD, 0x01, 0x00, 0xE8, 0x03, 0xFF, 0xFF, 0xFF });
	const bool workingSetIsObjectPointerAnswered = poll_for_response(serverUnderTest, testPlugin, 0xAD, workingSetIsObjectPointerResponse);
	receive_from_client(internalECU, client, { 0xAD, 0x00, 0x00, 0x01, 0x00, 0xFF, 0xFF, 0xFF });
	const bool maskIsObjectPointerAnswered = poll_for_response(serverUnderTest, testPlugin, 0xAD, maskIsObjectPointerResponse);
	receive_from_client(internalECU, client, { 0xAD, 0x00, 0x00, 0xE8, 0x03, 0xFF, 0xFF, 0xFF });
	const bool maskIsDataMaskAnswered = poll_for_response(serverUnderTest, testPlugin, 0xAD, maskIsDataMaskResponse);
	const auto objectCountBeforeUnknownMask = serverUnderTest.managedWorkingSetList.front()->get_object_tree().size();
	receive_from_client(internalECU, client, { 0xAD, 0x01, 0x00, 0xD0, 0x07, 0xFF, 0xFF, 0xFF });
	const bool unknownMaskAnswered = poll_for_response(serverUnderTest, testPlugin, 0xAD, unknownMaskResponse);
	CANHardwareInterface::stop();

	ASSERT_TRUE(poolAnswered);
	EXPECT_EQ(0x00, poolResponse.data[1]);
	ASSERT_TRUE(workingSetIsObjectPointerAnswered);
	EXPECT_EQ(0x01, workingSetIsObjectPointerResponse.data[3]);
	ASSERT_TRUE(maskIsObjectPointerAnswered);
	EXPECT_EQ(0x02, maskIsObjectPointerResponse.data[3]);
	ASSERT_TRUE(maskIsDataMaskAnswered);
	EXPECT_EQ(0x00, maskIsDataMaskResponse.data[3]);
	ASSERT_TRUE(unknownMaskAnswered);
	EXPECT_EQ(0x01, unknownMaskResponse.data[3]);
	EXPECT_EQ(objectCountBeforeUnknownMask, serverUnderTest.managedWorkingSetList.front()->get_object_tree().size());
}

TEST_F(VirtualTerminalServerMessagingTest, ChangePolygonScaleRescalesThePointsAndRepaints)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x48, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xC3, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();
	std::uint32_t repaints = 0;
	serverUnderTest.get_on_repaint_event_dispatcher().add_listener([&repaints](std::shared_ptr<VirtualTerminalServerManagedWorkingSet>) { repaints++; });

	CANMessageFrame response = {};

	const bool connected = connect_with_polygon_and_line_pool(serverUnderTest, testPlugin, internalECU, client);
	const std::uint32_t repaintsBeforeScale = repaints;
	receive_from_client(internalECU, client, { 0xB7, 0x02, 0x00, 0x96, 0x00, 0x19, 0x00, 0xFF });
	const bool answered = poll_for_response(serverUnderTest, testPlugin, 0xB7, response);
	CANHardwareInterface::stop();

	ASSERT_TRUE(connected);
	ASSERT_TRUE(answered);
	EXPECT_EQ(0x02, response.data[1]);
	EXPECT_EQ(0x00, response.data[2]);
	EXPECT_EQ(0x96, response.data[3]);
	EXPECT_EQ(0x00, response.data[4]);
	EXPECT_EQ(0x19, response.data[5]);
	EXPECT_EQ(0x00, response.data[6]);
	EXPECT_EQ(0x00, response.data[7]);
	EXPECT_EQ(repaintsBeforeScale + 1, repaints);

	auto polygon = std::static_pointer_cast<OutputPolygon>(serverUnderTest.managedWorkingSetList.front()->get_object_by_id(2));
	EXPECT_EQ(150, polygon->get_width());
	EXPECT_EQ(25, polygon->get_height());
	EXPECT_EQ(0, polygon->get_point(0).xValue);
	EXPECT_EQ(0, polygon->get_point(0).yValue);
	EXPECT_EQ(150, polygon->get_point(1).xValue);
	EXPECT_EQ(0, polygon->get_point(1).yValue);
	// (33 * 150 + 50) / 100 and (17 * 25 + 25) / 50: the formula rounds to the nearest pixel
	EXPECT_EQ(50, polygon->get_point(2).xValue);
	EXPECT_EQ(9, polygon->get_point(2).yValue);
}

TEST_F(VirtualTerminalServerMessagingTest, ChangePolygonScaleRejectsUnknownAndWrongObjectsAndAZeroOldWidth)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x49, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xC4, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	CANMessageFrame unknownObjectResponse = {};
	CANMessageFrame lineResponse = {};
	CANMessageFrame zeroWidthResponse = {};
	CANMessageFrame zeroOldWidthResponse = {};

	const bool connected = connect_with_polygon_and_line_pool(serverUnderTest, testPlugin, internalECU, client);
	receive_from_client(internalECU, client, { 0xB7, 0xD0, 0x07, 0x96, 0x00, 0x19, 0x00, 0xFF });
	const bool unknownObjectAnswered = poll_for_response(serverUnderTest, testPlugin, 0xB7, unknownObjectResponse);
	receive_from_client(internalECU, client, { 0xB7, 0x03, 0x00, 0x96, 0x00, 0x19, 0x00, 0xFF });
	const bool lineAnswered = poll_for_response(serverUnderTest, testPlugin, 0xB7, lineResponse);
	receive_from_client(internalECU, client, { 0xB7, 0x02, 0x00, 0x00, 0x00, 0x19, 0x00, 0xFF });
	const bool zeroWidthAnswered = poll_for_response(serverUnderTest, testPlugin, 0xB7, zeroWidthResponse);
	receive_from_client(internalECU, client, { 0xB7, 0x02, 0x00, 0x96, 0x00, 0x19, 0x00, 0xFF });
	const bool zeroOldWidthAnswered = poll_for_response(serverUnderTest, testPlugin, 0xB7, zeroOldWidthResponse);
	CANHardwareInterface::stop();

	ASSERT_TRUE(connected);
	ASSERT_TRUE(unknownObjectAnswered);
	EXPECT_EQ(0x01, unknownObjectResponse.data[7]);
	ASSERT_TRUE(lineAnswered);
	EXPECT_EQ(0x01, lineResponse.data[7]);
	EXPECT_EQ(10, serverUnderTest.managedWorkingSetList.front()->get_object_by_id(3)->get_width());
	ASSERT_TRUE(zeroWidthAnswered);
	EXPECT_EQ(0x00, zeroWidthResponse.data[7]);
	ASSERT_TRUE(zeroOldWidthAnswered);
	EXPECT_EQ(0x10, zeroOldWidthResponse.data[7]);
	EXPECT_EQ(0, serverUnderTest.managedWorkingSetList.front()->get_object_by_id(2)->get_width());
}

TEST_F(VirtualTerminalServerMessagingTest, ChangePolygonScaleRejectsAPointThatWouldNotFitIn16Bits)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x4F, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xC9, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	std::uint32_t repaints = 0;
	serverUnderTest.get_on_repaint_event_dispatcher().add_listener([&repaints](std::shared_ptr<VirtualTerminalServerManagedWorkingSet>) { repaints++; });
	CANMessageFrame pointResponse = {};
	CANMessageFrame scaleResponse = {};

	const bool connected = connect_with_polygon_and_line_pool(serverUnderTest, testPlugin, internalECU, client);
	receive_from_client(internalECU, client, { 0xB6, 0x02, 0x00, 0x02, 0xC8, 0x00, 0x11, 0x00 });
	const bool pointAnswered = poll_for_response(serverUnderTest, testPlugin, 0xB6, pointResponse);
	const std::uint32_t repaintsBeforeScale = repaints;
	// Point 2 is now at x = 200, so (200 * 65535 + 50) / 100 needs 17 bits, while point 1 before it would fit
	receive_from_client(internalECU, client, { 0xB7, 0x02, 0x00, 0xFF, 0xFF, 0x19, 0x00, 0xFF });
	const bool scaleAnswered = poll_for_response(serverUnderTest, testPlugin, 0xB7, scaleResponse);
	CANHardwareInterface::stop();

	ASSERT_TRUE(connected);
	ASSERT_TRUE(pointAnswered);
	EXPECT_EQ(0x00, pointResponse.data[3]);
	ASSERT_TRUE(scaleAnswered);
	EXPECT_EQ(0x10, scaleResponse.data[7]);
	EXPECT_EQ(repaintsBeforeScale, repaints);

	auto polygon = std::static_pointer_cast<OutputPolygon>(serverUnderTest.managedWorkingSetList.front()->get_object_by_id(2));
	EXPECT_EQ(100, polygon->get_width());
	EXPECT_EQ(50, polygon->get_height());
	EXPECT_EQ(100, polygon->get_point(1).xValue);
	EXPECT_EQ(200, polygon->get_point(2).xValue);
	EXPECT_EQ(17, polygon->get_point(2).yValue);
}

TEST_F(VirtualTerminalServerMessagingTest, ChangePolygonPointMovesThePointAndRepaints)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x50, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xCA, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();
	std::uint32_t repaints = 0;
	serverUnderTest.get_on_repaint_event_dispatcher().add_listener([&repaints](std::shared_ptr<VirtualTerminalServerManagedWorkingSet>) { repaints++; });

	CANMessageFrame response = {};

	const bool connected = connect_with_polygon_and_line_pool(serverUnderTest, testPlugin, internalECU, client);
	const std::uint32_t repaintsBeforeChange = repaints;
	receive_from_client(internalECU, client, { 0xB6, 0x02, 0x00, 0x02, 0x28, 0x00, 0x1E, 0x00 });
	const bool answered = poll_for_response(serverUnderTest, testPlugin, 0xB6, response);
	CANHardwareInterface::stop();

	ASSERT_TRUE(connected);
	ASSERT_TRUE(answered);
	EXPECT_EQ(0x00, response.data[3]);
	EXPECT_EQ(repaintsBeforeChange + 1, repaints);

	auto polygon = std::static_pointer_cast<OutputPolygon>(serverUnderTest.managedWorkingSetList.front()->get_object_by_id(2));
	EXPECT_EQ(40, polygon->get_point(2).xValue);
	EXPECT_EQ(30, polygon->get_point(2).yValue);
}

TEST_F(VirtualTerminalServerMessagingTest, ChangePolygonScaleInAShortFrameIsIgnored)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x4B, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xC5, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	CANMessageFrame response = {};

	const bool connected = connect_with_polygon_and_line_pool(serverUnderTest, testPlugin, internalECU, client);
	receive_from_client(internalECU, client, { 0xB7, 0x02, 0x00, 0x96, 0x00, 0x19, 0x00 });
	const bool answered = poll_for_response(serverUnderTest, testPlugin, 0xB7, response);
	CANHardwareInterface::stop();

	ASSERT_TRUE(connected);
	EXPECT_FALSE(answered);
	EXPECT_EQ(100, serverUnderTest.managedWorkingSetList.front()->get_object_by_id(2)->get_width());
}

TEST_F(VirtualTerminalServerMessagingTest, ChangeEndPointChangesTheLineRepaintsAndRunsItsMacro)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x4C, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xC6, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();
	std::uint32_t repaints = 0;
	serverUnderTest.get_on_repaint_event_dispatcher().add_listener([&repaints](std::shared_ptr<VirtualTerminalServerManagedWorkingSet>) { repaints++; });

	CANMessageFrame response = {};

	const bool connected = connect_with_polygon_and_line_pool(serverUnderTest, testPlugin, internalECU, client);
	const std::uint32_t repaintsBeforeChange = repaints;
	receive_from_client(internalECU, client, { 0xA9, 0x03, 0x00, 0x28, 0x00, 0x1E, 0x00, 0x01 });
	const bool answered = poll_for_response(serverUnderTest, testPlugin, 0xA9, response);
	CANHardwareInterface::stop();

	ASSERT_TRUE(connected);
	ASSERT_TRUE(answered);
	EXPECT_EQ(0x03, response.data[1]);
	EXPECT_EQ(0x00, response.data[2]);
	EXPECT_EQ(0x00, response.data[3]);
	EXPECT_EQ(0xFF, response.data[4]);
	EXPECT_EQ(0xFF, response.data[7]);
	// One for the end point, one for the Change Size the macro runs
	EXPECT_EQ(repaintsBeforeChange + 2, repaints);

	auto line = std::static_pointer_cast<OutputLine>(serverUnderTest.managedWorkingSetList.front()->get_object_by_id(3));
	EXPECT_EQ(40, line->get_width());
	EXPECT_EQ(30, line->get_height());
	EXPECT_EQ(OutputLine::LineDirection::BottomLeftToTopRight, line->get_line_direction());
	EXPECT_EQ(7, serverUnderTest.managedWorkingSetList.front()->get_object_by_id(2)->get_width());
	EXPECT_EQ(8, serverUnderTest.managedWorkingSetList.front()->get_object_by_id(2)->get_height());
}

TEST_F(VirtualTerminalServerMessagingTest, ChangeEndPointRejectsUnknownAndWrongObjectsAndAnInvalidLineDirection)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x4D, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xC7, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	CANMessageFrame unknownObjectResponse = {};
	CANMessageFrame polygonResponse = {};
	CANMessageFrame invalidDirectionResponse = {};

	const bool connected = connect_with_polygon_and_line_pool(serverUnderTest, testPlugin, internalECU, client);
	receive_from_client(internalECU, client, { 0xA9, 0xD0, 0x07, 0x28, 0x00, 0x1E, 0x00, 0x01 });
	const bool unknownObjectAnswered = poll_for_response(serverUnderTest, testPlugin, 0xA9, unknownObjectResponse);
	receive_from_client(internalECU, client, { 0xA9, 0x02, 0x00, 0x28, 0x00, 0x1E, 0x00, 0x01 });
	const bool polygonAnswered = poll_for_response(serverUnderTest, testPlugin, 0xA9, polygonResponse);
	receive_from_client(internalECU, client, { 0xA9, 0x03, 0x00, 0x28, 0x00, 0x1E, 0x00, 0x02 });
	const bool invalidDirectionAnswered = poll_for_response(serverUnderTest, testPlugin, 0xA9, invalidDirectionResponse);
	CANHardwareInterface::stop();

	ASSERT_TRUE(connected);
	ASSERT_TRUE(unknownObjectAnswered);
	EXPECT_EQ(0x01, unknownObjectResponse.data[3]);
	ASSERT_TRUE(polygonAnswered);
	EXPECT_EQ(0x01, polygonResponse.data[3]);
	EXPECT_EQ(100, serverUnderTest.managedWorkingSetList.front()->get_object_by_id(2)->get_width());
	ASSERT_TRUE(invalidDirectionAnswered);
	EXPECT_EQ(0x02, invalidDirectionResponse.data[3]);
	EXPECT_EQ(10, serverUnderTest.managedWorkingSetList.front()->get_object_by_id(3)->get_width());
}

TEST_F(VirtualTerminalServerMessagingTest, ChangeEndPointInAShortFrameIsIgnored)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x4E, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xC8, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	CANMessageFrame response = {};

	const bool connected = connect_with_polygon_and_line_pool(serverUnderTest, testPlugin, internalECU, client);
	receive_from_client(internalECU, client, { 0xA9, 0x03, 0x00, 0x28, 0x00, 0x1E, 0x00 });
	const bool answered = poll_for_response(serverUnderTest, testPlugin, 0xA9, response);
	CANHardwareInterface::stop();

	ASSERT_TRUE(connected);
	EXPECT_FALSE(answered);
	EXPECT_EQ(10, serverUnderTest.managedWorkingSetList.front()->get_object_by_id(3)->get_width());
}

TEST_F(VirtualTerminalServerMessagingTest, CommandsToObjectsOfTheWrongTypeReportTheErrorBitOfThatCommand)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x52, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xCC, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();
	serverUnderTest.version = VirtualTerminalBase::VTVersion::Version4;

	const auto errorCodeOf = [&](std::uint8_t function, std::size_t errorByte) {
		CANMessageFrame response = {};
		return poll_for_response(serverUnderTest, testPlugin, function, response) ? static_cast<int>(response.data[errorByte]) : -1;
	};

	// Working Set 0 holds Object Pointer 1; Data Mask 1000 holds disabled Input Boolean 4 and hidden Container 2 (Input Boolean 3); Input Boolean 5 is on no mask
	CANMessageFrame poolResponse = {};
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_tp_from_client(internalECU, client, { 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF, 0xE8, 0x03, 0x01, 0x00, 0xFF, 0xFF, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x0A, 0x00, 0x0A, 0x00, 0x01, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x07, 0x00, 0x0A, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x00, 0x07, 0x00, 0x0A, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x05, 0x00, 0x07, 0x00, 0x0A, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0x00 });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	const bool connected = poll_for_response(serverUnderTest, testPlugin, 0x12, poolResponse) && (0x00 == poolResponse.data[1]);

	receive_from_client(internalECU, client, { 0xA6, 0x01, 0x00, 0x0A, 0x00, 0x0A, 0x00, 0xFF });
	const int changeSize = errorCodeOf(0xA6, 3);
	receive_from_client(internalECU, client, { 0xA7, 0x01, 0x00, 0x05, 0xFF, 0xFF, 0xFF, 0xFF });
	const int changeBackgroundColour = errorCodeOf(0xA7, 4);
	receive_from_client(internalECU, client, { 0xAE, 0x01, 0x01, 0x00, 0xFF, 0xFF, 0xFF, 0xFF });
	const int changeSoftKeyMask = errorCodeOf(0xAE, 5);
	receive_from_client(internalECU, client, { 0xB0, 0x01, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF });
	const int changePriority = errorCodeOf(0xB0, 4);
	receive_from_client(internalECU, client, { 0xB1, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF });
	const int changeListItem = errorCodeOf(0xB1, 6);
	receive_from_client(internalECU, client, { 0xB6, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
	const int changePolygonPoint = errorCodeOf(0xB6, 3);

	receive_from_client(internalECU, client, { 0xA5, 0x04, 0x00, 0x01, 0x00, 0x80, 0x80, 0xFF });
	const int childLocationOfANonParent = errorCodeOf(0xA5, 5);
	receive_from_client(internalECU, client, { 0xA5, 0x00, 0x00, 0x04, 0x00, 0x80, 0x80, 0xFF });
	const int childLocationOfANonChild = errorCodeOf(0xA5, 5);
	receive_from_client(internalECU, client, { 0xA5, 0x00, 0x00, 0xD0, 0x07, 0x80, 0x80, 0xFF });
	const int childLocationOfAMissingChild = errorCodeOf(0xA5, 5);
	receive_from_client(internalECU, client, { 0xA5, 0x00, 0x00, 0x01, 0x00, 0x80, 0x80, 0xFF });
	const int childLocationOfAChild = errorCodeOf(0xA5, 5);
	receive_tp_from_client(internalECU, client, { 0xB4, 0x04, 0x00, 0x01, 0x00, 0x05, 0x00, 0x05, 0x00 });
	const int childPositionOfANonParent = errorCodeOf(0xB4, 5);
	receive_tp_from_client(internalECU, client, { 0xB4, 0x00, 0x00, 0x04, 0x00, 0x05, 0x00, 0x05, 0x00 });
	const int childPositionOfANonChild = errorCodeOf(0xB4, 5);

	receive_from_client(internalECU, client, { 0xA2, 0x01, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const int selectNonInput = errorCodeOf(0xA2, 4);
	receive_from_client(internalECU, client, { 0xA2, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const int selectDisabled = errorCodeOf(0xA2, 4);
	receive_from_client(internalECU, client, { 0xA2, 0x05, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const int selectNotOnActiveMask = errorCodeOf(0xA2, 4);
	receive_from_client(internalECU, client, { 0xA2, 0x03, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const int selectInHiddenContainer = errorCodeOf(0xA2, 4);
	receive_from_client(internalECU, client, { 0xA1, 0x04, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF });
	const int enable = errorCodeOf(0xA1, 4);
	receive_from_client(internalECU, client, { 0xA2, 0x04, 0x00, 0x05, 0xFF, 0xFF, 0xFF, 0xFF });
	const int selectWithInvalidOption = errorCodeOf(0xA2, 4);
	receive_from_client(internalECU, client, { 0xA2, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	CANMessageFrame selectEnabled = {};
	const bool selectEnabledAnswered = poll_for_response(serverUnderTest, testPlugin, 0xA2, selectEnabled);
	serverUnderTest.version = VirtualTerminalBase::VTVersion::Version3;
	receive_from_client(internalECU, client, { 0xA2, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF });
	CANMessageFrame openOnVersion3 = {};
	const bool openOnVersion3Answered = poll_for_response(serverUnderTest, testPlugin, 0xA2, openOnVersion3);
	CANHardwareInterface::stop();

	ASSERT_TRUE(connected);
	EXPECT_EQ(0x01, changeSize);
	EXPECT_EQ(0x01, changeBackgroundColour);
	EXPECT_EQ(0x01, changeSoftKeyMask);
	EXPECT_EQ(0x01, changePriority);
	EXPECT_EQ(0x01, changeListItem);
	EXPECT_EQ(0x01, changePolygonPoint);

	EXPECT_EQ(0x01, childLocationOfANonParent);
	EXPECT_EQ(0x01, childLocationOfANonChild);
	EXPECT_EQ(0x02, childLocationOfAMissingChild);
	EXPECT_EQ(0x00, childLocationOfAChild);
	EXPECT_EQ(0x01, childPositionOfANonParent);
	EXPECT_EQ(0x01, childPositionOfANonChild);

	EXPECT_EQ(0x02, selectNonInput);
	EXPECT_EQ(0x01, selectDisabled);
	EXPECT_EQ(0x04, selectNotOnActiveMask);
	EXPECT_EQ(0x04, selectInHiddenContainer);
	EXPECT_EQ(0x00, enable);
	EXPECT_EQ(0x20, selectWithInvalidOption);
	ASSERT_TRUE(selectEnabledAnswered);
	EXPECT_EQ(0x01, selectEnabled.data[3]);
	EXPECT_EQ(0x00, selectEnabled.data[4]);
	ASSERT_TRUE(openOnVersion3Answered);
	EXPECT_EQ(0x01, openOnVersion3.data[3]);
	EXPECT_EQ(0x00, openOnVersion3.data[4]);
}

TEST_F(VirtualTerminalServerMessagingTest, GetSupportedObjectsResponseIsASortedListOfVariableLength)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x40, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x9D, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	CANMessageFrame emptyResponse = {};
	CANMessageFrame shortResponse = {};
	CANMessageFrame sixTypesResponse = {};
	std::vector<std::uint8_t> sevenTypesResponse;
	std::vector<std::uint8_t> longResponse;

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	serverUnderTest.supportedObjects = {};
	receive_from_client(internalECU, client, { 0xC5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool emptyAnswered = poll_for_response(serverUnderTest, testPlugin, 0xC5, emptyResponse);

	serverUnderTest.supportedObjects = { 3, 1 };
	receive_from_client(internalECU, client, { 0xC5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool shortAnswered = poll_for_response(serverUnderTest, testPlugin, 0xC5, shortResponse);

	serverUnderTest.supportedObjects = { 6, 5, 4, 3, 2, 1 };
	receive_from_client(internalECU, client, { 0xC5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool sixTypesAnswered = poll_for_response(serverUnderTest, testPlugin, 0xC5, sixTypesResponse);

	serverUnderTest.supportedObjects = { 7, 6, 5, 4, 3, 2, 1 };
	receive_from_client(internalECU, client, { 0xC5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool sevenTypesAnswered = read_tp_from_server(testPlugin, internalECU, client, sevenTypesResponse);

	serverUnderTest.supportedObjects = { 39, 37, 7, 6, 5, 4, 3, 2, 1, 0 };
	receive_from_client(internalECU, client, { 0xC5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool longAnswered = read_tp_from_server(testPlugin, internalECU, client, longResponse);
	CANHardwareInterface::stop();

	ASSERT_TRUE(emptyAnswered);
	EXPECT_EQ(8, emptyResponse.dataLength);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0xC5, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }), std::vector<std::uint8_t>(emptyResponse.data, emptyResponse.data + 8));
	ASSERT_TRUE(shortAnswered);
	EXPECT_EQ(8, shortResponse.dataLength);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0xC5, 2, 1, 3, 0xFF, 0xFF, 0xFF, 0xFF }), std::vector<std::uint8_t>(shortResponse.data, shortResponse.data + 8));
	ASSERT_TRUE(sixTypesAnswered);
	EXPECT_EQ(8, sixTypesResponse.dataLength);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0xC5, 6, 1, 2, 3, 4, 5, 6 }), std::vector<std::uint8_t>(sixTypesResponse.data, sixTypesResponse.data + 8));
	ASSERT_TRUE(sevenTypesAnswered);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0xC5, 7, 1, 2, 3, 4, 5, 6, 7 }), sevenTypesResponse);
	ASSERT_TRUE(longAnswered);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0xC5, 10, 0, 1, 2, 3, 4, 5, 6, 7, 37, 39 }), longResponse);
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
	serverUnderTest.versionToLoad = test_helpers::read_vt3_test_pool();
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
	serverUnderTest.versionToLoad = test_helpers::read_vt3_test_pool();
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

TEST_F(VirtualTerminalServerMessagingTest, LoadVersionOverwritesTheExistingObjectPool)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x3F, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0x9C, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.versionToLoad = test_helpers::read_vt3_test_pool();
	ASSERT_FALSE(serverUnderTest.versionToLoad.empty());
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_tp_from_client(internalECU, client, { 0x11, 0x01, 0xF0, 0x01, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x01, 0x01, 0xF0, 0x00, 0x00, 0x03, 'e', 'n', 'd', 'e', 'f', 'r' });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

	CANMessageFrame responseFrame = {};
	const bool foundEndOfObjectPoolResponse = poll_for_response(serverUnderTest, testPlugin, 0x12, responseFrame);
	const std::uint8_t endOfObjectPoolErrorCodes = responseFrame.data[1];

	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });

	const bool foundLoadVersionResponse = poll_for_response(serverUnderTest, testPlugin, 0xD1, responseFrame);
	const std::uint8_t loadVersionErrorCodes = responseFrame.data[5];

	CANMessageFrame statusFrame = {};
	const bool foundStatus = read_status_message(serverUnderTest, testPlugin, statusFrame);
	CANHardwareInterface::stop();

	auto workingSet = serverUnderTest.managedWorkingSetList.front();
	ASSERT_TRUE(foundEndOfObjectPoolResponse);
	ASSERT_EQ(0x00, endOfObjectPoolErrorCodes);
	ASSERT_TRUE(foundLoadVersionResponse);
	EXPECT_EQ(0x00, loadVersionErrorCodes);
	ASSERT_EQ(1U, workingSet->get_number_iop_files());
	EXPECT_EQ(serverUnderTest.versionToLoad, workingSet->get_iop_raw_data(0));
	EXPECT_EQ(0U, workingSet->get_object_tree().count(0xF000));

	const std::uint16_t loadedActiveMask = std::static_pointer_cast<WorkingSet>(workingSet->get_working_set_object())->get_active_mask();
	ASSERT_NE(0xF001, loadedActiveMask);
	ASSERT_TRUE(foundStatus);
	EXPECT_EQ(0x9C, statusFrame.data[1]);
	EXPECT_EQ(loadedActiveMask, static_cast<std::uint16_t>(statusFrame.data[2] | (statusFrame.data[3] << 8)));
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
	serverUnderTest.versionToLoad = test_helpers::read_vt3_test_pool();
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

	receive_working_set_master_from_client(client);
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
	serverUnderTest.versionToLoad = test_helpers::read_vt3_test_pool();
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

TEST_F(VirtualTerminalServerMessagingTest, StoreVersionReplacesTheVersionStoredUnderTheSameLabel)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x43, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xA0, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 });
	receive_from_client(internalECU, client, { 0x11, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E });
	testPlugin.clear_queue();

	CANMessageFrame firstResponse = {};
	CANMessageFrame secondResponse = {};
	receive_from_client(internalECU, client, { 0xD0, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	const bool foundFirstResponse = poll_for_response(serverUnderTest, testPlugin, 0xD0, firstResponse);
	receive_from_client(internalECU, client, { 0xD0, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	const bool foundSecondResponse = poll_for_response(serverUnderTest, testPlugin, 0xD0, secondResponse);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundFirstResponse);
	EXPECT_EQ(0x00, firstResponse.data[5]);
	ASSERT_TRUE(foundSecondResponse);
	EXPECT_EQ(0x00, secondResponse.data[5]);
	ASSERT_EQ(2U, serverUnderTest.deletedVersionLabels.size());
	EXPECT_EQ((std::vector<std::uint8_t>{ 'V', 'E', 'R', 'S', 'I', 'O', 'N' }), serverUnderTest.deletedVersionLabels.back());
	ASSERT_EQ(2U, serverUnderTest.storedVersion.size());
	EXPECT_EQ((std::vector<std::uint8_t>{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 }), serverUnderTest.storedVersion.at(0));
	EXPECT_EQ((std::vector<std::uint8_t>{ 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E }), serverUnderTest.storedVersion.at(1));
}

TEST_F(VirtualTerminalServerMessagingTest, StoreVersionSucceedsWhenNothingIsStoredUnderTheLabelYet)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x44, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xA1, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.deleteSucceeds = false;
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x11, 0x00, 0xF0, 0x15, 0x00, 0x00, 0x00, 0x00 });
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD0, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });

	CANMessageFrame responseFrame = {};
	const bool foundStoreVersionResponse = poll_for_response(serverUnderTest, testPlugin, 0xD0, responseFrame);
	CANHardwareInterface::stop();

	ASSERT_TRUE(foundStoreVersionResponse);
	EXPECT_EQ(0x00, responseFrame.data[5]);
	EXPECT_EQ(1U, serverUnderTest.deletedVersionLabels.size());
	EXPECT_EQ(1U, serverUnderTest.storedVersion.size());
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
	serverUnderTest.versionToLoad = test_helpers::read_vt3_test_pool();
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
	serverUnderTest.versionToLoad = test_helpers::read_vt3_test_pool();
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
	ASSERT_EQ(1U, serverUnderTest.deletedVersionLabels.size());
	EXPECT_EQ(serverUnderTest.lastVersionLabel, serverUnderTest.deletedVersionLabels.front());
	EXPECT_EQ(1U, serverUnderTest.storedVersion.size());
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

TEST_F(VirtualTerminalServerMessagingTest, WorkingSetShutsDownWhenMaintenanceMessagesStop)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x60, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xB0, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	time_source.simulate_delay_ms(2000);
	receive_from_client(internalECU, client, { 0xFF, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	time_source.simulate_delay_ms(2000);
	serverUnderTest.run_update();
	const std::size_t shutdownsWhileMaintained = serverUnderTest.shutDownWorkingSets.size();
	const bool managedWhileMaintained = is_managed(internalECU, client, testPlugin);

	time_source.simulate_delay_ms(JUST_PAST_MAINTENANCE_TIMEOUT_MS);
	serverUnderTest.run_update();
	const bool managedAfterTimeout = is_managed(internalECU, client, testPlugin);
	CANHardwareInterface::stop();

	EXPECT_EQ(0U, shutdownsWhileMaintained);
	EXPECT_TRUE(managedWhileMaintained);
	ASSERT_EQ(1U, serverUnderTest.shutdownReasons.size());
	EXPECT_EQ(VirtualTerminalServer::UnexpectedShutdownReason::MaintenanceTimeout, serverUnderTest.shutdownReasons.front());
	EXPECT_EQ(client, serverUnderTest.shutDownWorkingSets.front()->get_control_function());
	EXPECT_FALSE(managedAfterTimeout);
}

TEST_F(VirtualTerminalServerMessagingTest, OtherMessagesDoNotKeepAWorkingSetAlive)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x61, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xB1, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	time_source.simulate_delay_ms(2000);
	receive_from_client(internalECU, client, { 0xC0, 0xFF, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF }); // Get Memory
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }); // End of Object Pool
	time_source.simulate_delay_ms(1001);
	serverUnderTest.run_update();
	CANHardwareInterface::stop();

	ASSERT_EQ(1U, serverUnderTest.shutdownReasons.size());
	EXPECT_EQ(VirtualTerminalServer::UnexpectedShutdownReason::MaintenanceTimeout, serverUnderTest.shutdownReasons.front());
}

TEST_F(VirtualTerminalServerMessagingTest, TimedOutWorkingSetIsManagedAgainAfterAnInitiatingMessage)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x62, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xB2, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	time_source.simulate_delay_ms(JUST_PAST_MAINTENANCE_TIMEOUT_MS);
	serverUnderTest.run_update();

	// Until it initiates again, even its maintenance messages are NACKed
	const bool maintenanceNacked = is_nacked(internalECU, client, testPlugin, { 0xFF, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool managedAfterInitiating = is_managed(internalECU, client, testPlugin);
	CANHardwareInterface::stop();

	EXPECT_EQ(1U, serverUnderTest.shutdownReasons.size());
	EXPECT_TRUE(maintenanceNacked);
	EXPECT_TRUE(managedAfterInitiating);
}

TEST_F(VirtualTerminalServerMessagingTest, ActiveWorkingSetIsClearedWhenItTimesOut)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x63, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xB3, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	CANMessageFrame statusWhileConnected = {};
	CANMessageFrame statusAfterTimeout = {};

	const bool poolLoaded = connect_with_selectable_object_pool(serverUnderTest, testPlugin, internalECU, client);
	const bool foundStatusWhileConnected = read_status_message(serverUnderTest, testPlugin, statusWhileConnected);
	time_source.simulate_delay_ms(JUST_PAST_MAINTENANCE_TIMEOUT_MS);
	serverUnderTest.run_update();
	const bool foundStatusAfterTimeout = read_status_message(serverUnderTest, testPlugin, statusAfterTimeout);
	CANHardwareInterface::stop();

	ASSERT_TRUE(poolLoaded);
	ASSERT_TRUE(foundStatusWhileConnected);
	EXPECT_EQ(0xB3, statusWhileConnected.data[1]);
	EXPECT_EQ(1U, serverUnderTest.shutdownReasons.size());
	ASSERT_TRUE(foundStatusAfterTimeout);
	EXPECT_EQ(NULL_CAN_ADDRESS, statusAfterTimeout.data[1]);
	EXPECT_EQ(NULL_OBJECT_ID, statusAfterTimeout.data[2] | (statusAfterTimeout.data[3] << 8));
	EXPECT_EQ(NULL_OBJECT_ID, statusAfterTimeout.data[4] | (statusAfterTimeout.data[5] << 8));
}

TEST_F(VirtualTerminalServerMessagingTest, RepeatedInitiatingMessageRestartsTheWorkingSet)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x64, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xB4, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	CANMessageFrame statusAfterRestart = {};

	const bool poolLoaded = connect_with_selectable_object_pool(serverUnderTest, testPlugin, internalECU, client);
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool foundStatusAfterRestart = read_status_message(serverUnderTest, testPlugin, statusAfterRestart);
	const bool managedAfterRestart = is_managed(internalECU, client, testPlugin);
	// Store Version is NACKed when there is no object pool, so this shows the old pool was discarded
	const bool storeVersionNacked = is_nacked(internalECU, client, testPlugin, { 0xD0, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	// The application is told from update(), not from the thread that received the message
	const std::size_t shutdownsBeforeUpdate = serverUnderTest.shutdownReasons.size();
	serverUnderTest.run_update();
	CANHardwareInterface::stop();

	ASSERT_TRUE(poolLoaded);
	EXPECT_EQ(0U, shutdownsBeforeUpdate);
	ASSERT_EQ(1U, serverUnderTest.shutdownReasons.size());
	EXPECT_EQ(VirtualTerminalServer::UnexpectedShutdownReason::RepeatedInitiatingMessage, serverUnderTest.shutdownReasons.front());
	ASSERT_TRUE(foundStatusAfterRestart);
	EXPECT_EQ(NULL_CAN_ADDRESS, statusAfterRestart.data[1]);
	EXPECT_TRUE(managedAfterRestart);
	EXPECT_TRUE(storeVersionNacked);
}

TEST_F(VirtualTerminalServerMessagingTest, WorkingSetThatShutsDownWhileParsingIsReportedOnceParsingFinishes)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x66, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xB7, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.versionToLoad = test_helpers::read_vt3_test_pool();
	ASSERT_FALSE(serverUnderTest.versionToLoad.empty());
	serverUnderTest.initialize();

	// Load Version starts parsing the stored pool, then the client restarts before the VT has answered
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	const auto parsingWorkingSet = serverUnderTest.managedWorkingSetList.front();
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	// Handling the restart must not wait for the parsing thread, so the large pool is still being parsed here
	const auto stateAfterRestart = parsingWorkingSet->get_object_pool_processing_state();
	const bool managedAfterRestart = is_managed(internalECU, client, testPlugin);

	bool loadVersionResponseSent = false;
	for (std::uint_fast16_t attempt = 0; (attempt < 400) && serverUnderTest.shutdownReasons.empty(); attempt++)
	{
		CANMessageFrame frame = {};
		serverUnderTest.run_update();
		time_source.update_for_ms(1);
		while (testPlugin.read_frame(frame, 1))
		{
			loadVersionResponseSent |= (0xE6 == get_pdu_format(frame)) && (0xD1 == frame.data[0]);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
	CANHardwareInterface::stop();

	EXPECT_EQ(VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Running, stateAfterRestart);
	EXPECT_TRUE(managedAfterRestart);
	ASSERT_EQ(1U, serverUnderTest.shutdownReasons.size());
	EXPECT_EQ(VirtualTerminalServer::UnexpectedShutdownReason::RepeatedInitiatingMessage, serverUnderTest.shutdownReasons.front());
	EXPECT_EQ(parsingWorkingSet, serverUnderTest.shutDownWorkingSets.front());
	EXPECT_EQ(VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined, parsingWorkingSet->get_object_pool_processing_state());
	EXPECT_FALSE(loadVersionResponseSent);
}

TEST_F(VirtualTerminalServerMessagingTest, Version2ClientMaintenanceMessagesDoNotRestartTheWorkingSet)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x65, 0, time_source);
	auto reservedVersionClient = test_helpers::force_claim_partnered_control_function(0xB5, 0);
	auto version2Client = test_helpers::force_claim_partnered_control_function(0xB6, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	receive_working_set_master_from_client(reservedVersionClient);
	receive_working_set_master_from_client(version2Client);

	// Version 2 and prior clients have no Initiating bit, so bit 0 is set in every maintenance message
	for (std::uint_fast8_t i = 0; i < 3; i++)
	{
		receive_from_client(internalECU, reservedVersionClient, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		receive_from_client(internalECU, version2Client, { 0xFF, 0x01, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		time_source.simulate_delay_ms(1000);
		serverUnderTest.run_update();
	}
	const bool reservedVersionClientManaged = is_managed(internalECU, reservedVersionClient, testPlugin);
	const bool version2ClientManaged = is_managed(internalECU, version2Client, testPlugin);
	CANHardwareInterface::stop();

	EXPECT_EQ(0U, serverUnderTest.shutdownReasons.size());
	EXPECT_TRUE(reservedVersionClientManaged);
	EXPECT_TRUE(version2ClientManaged);
}

TEST_F(VirtualTerminalServerMessagingTest, Version2ClientIsOnlyManagedAfterAWorkingSetMasterMessage)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x67, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xB8, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	const bool nackedBeforeWorkingSetMaster = is_nacked(internalECU, client, testPlugin, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool managedBeforeWorkingSetMaster = is_managed(internalECU, client, testPlugin);

	receive_working_set_master_from_client(client);
	receive_from_client(internalECU, client, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool managedAfterWorkingSetMaster = is_managed(internalECU, client, testPlugin);

	// A managed working set master also answers requests for this message, which must not let it back in after a timeout
	receive_working_set_master_from_client(client);
	time_source.simulate_delay_ms(JUST_PAST_MAINTENANCE_TIMEOUT_MS);
	serverUnderTest.run_update();

	// Its maintenance messages look like initiating ones, but its object pool is gone
	const bool nackedAfterTimeout = is_nacked(internalECU, client, testPlugin, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool managedAfterTimeout = is_managed(internalECU, client, testPlugin);

	receive_working_set_master_from_client(client);
	receive_from_client(internalECU, client, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool managedAfterRestart = is_managed(internalECU, client, testPlugin);
	CANHardwareInterface::stop();

	EXPECT_TRUE(nackedBeforeWorkingSetMaster);
	EXPECT_FALSE(managedBeforeWorkingSetMaster);
	EXPECT_TRUE(managedAfterWorkingSetMaster);
	ASSERT_EQ(1U, serverUnderTest.shutdownReasons.size());
	EXPECT_EQ(VirtualTerminalServer::UnexpectedShutdownReason::MaintenanceTimeout, serverUnderTest.shutdownReasons.front());
	EXPECT_TRUE(nackedAfterTimeout);
	EXPECT_FALSE(managedAfterTimeout);
	EXPECT_TRUE(managedAfterRestart);
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

TEST_F(VirtualTerminalServerMessagingTest, ESCMessageIsSentToTheWorkingSet)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x45, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xA2, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();
	testPlugin.clear_queue();

	ASSERT_TRUE(serverUnderTest.send_esc_message(0x1234, 0, client));

	CANMessageFrame frame = {};
	time_source.update_for_ms(5);
	ASSERT_TRUE(testPlugin.read_frame(frame));

	EXPECT_EQ(0xE6, get_pdu_format(frame)); // VT to ECU
	EXPECT_EQ(client->get_address(), static_cast<std::uint8_t>(frame.identifier >> 8));
	EXPECT_EQ(0x04, frame.data[0]);
	EXPECT_EQ(0x34, frame.data[1]);
	EXPECT_EQ(0x12, frame.data[2]);
	EXPECT_EQ(0x00, frame.data[3]);
	EXPECT_EQ(0xFF, frame.data[4]);
	EXPECT_EQ(0xFF, frame.data[5]);
	EXPECT_EQ(0xFF, frame.data[6]);
	EXPECT_EQ(0xFF, frame.data[7]);

	CANHardwareInterface::stop();
}

TEST_F(VirtualTerminalServerMessagingTest, PointingEventMessageMatchesTheVTVersion)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x52, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xCC, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();
	testPlugin.clear_queue();

	CANMessageFrame frame = {};

	// VT version 3 only reports the position
	serverUnderTest.version = VirtualTerminalBase::VTVersion::Version3;
	ASSERT_TRUE(serverUnderTest.send_pointing_event_message(VirtualTerminalBase::KeyActivationCode::ButtonUnlatchedOrReleased, 0x0123, 0x0045, 0x1234, client));
	time_source.update_for_ms(5);
	ASSERT_TRUE(testPlugin.read_frame(frame));
	EXPECT_EQ(0xE6, get_pdu_format(frame)); // VT to ECU
	EXPECT_EQ(client->get_address(), static_cast<std::uint8_t>(frame.identifier >> 8));
	EXPECT_EQ(0x02, frame.data[0]);
	EXPECT_EQ(0x23, frame.data[1]);
	EXPECT_EQ(0x01, frame.data[2]);
	EXPECT_EQ(0x45, frame.data[3]);
	EXPECT_EQ(0x00, frame.data[4]);
	EXPECT_EQ(0xFF, frame.data[5]);
	EXPECT_EQ(0xFF, frame.data[6]);
	EXPECT_EQ(0xFF, frame.data[7]);

	// VT version 4 and 5 add the touch state
	serverUnderTest.version = VirtualTerminalBase::VTVersion::Version5;
	ASSERT_TRUE(serverUnderTest.send_pointing_event_message(VirtualTerminalBase::KeyActivationCode::ButtonStillHeld, 0x0123, 0x0045, 0x1234, client));
	time_source.update_for_ms(5);
	ASSERT_TRUE(testPlugin.read_frame(frame));
	EXPECT_EQ(0x02, frame.data[5]);
	EXPECT_EQ(0xFF, frame.data[6]);
	EXPECT_EQ(0xFF, frame.data[7]);

	// VT version 6 adds the parent mask and a new transaction number for each message
	serverUnderTest.version = VirtualTerminalBase::VTVersion::Version6;
	ASSERT_TRUE(serverUnderTest.send_pointing_event_message(VirtualTerminalBase::KeyActivationCode::ButtonPressedOrLatched, 0x0123, 0x0045, 0x1234, client));
	time_source.update_for_ms(5);
	ASSERT_TRUE(testPlugin.read_frame(frame));
	EXPECT_EQ(0x01, frame.data[5]);
	EXPECT_EQ(0x34, frame.data[6]);
	EXPECT_EQ(0x12, frame.data[7]);

	ASSERT_TRUE(serverUnderTest.send_pointing_event_message(VirtualTerminalBase::KeyActivationCode::ButtonUnlatchedOrReleased, 0x0123, 0x0045, 0x1234, client));
	time_source.update_for_ms(5);
	ASSERT_TRUE(testPlugin.read_frame(frame));
	EXPECT_EQ(0x10, frame.data[5]);

	CANHardwareInterface::stop();
}

TEST_F(VirtualTerminalServerMessagingTest, ESCCommandIsAnsweredWithTheObjectWhoseInputWasAborted)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x46, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xA3, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	CANMessageFrame noInputOpenResponse = {};
	CANMessageFrame inputAbortedResponse = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0x92, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool noInputOpenAnswered = poll_for_response(serverUnderTest, testPlugin, 0x92, noInputOpenResponse);
	serverUnderTest.openInputObjectID = 0x1234;
	receive_from_client(internalECU, client, { 0x92, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool inputAbortedAnswered = poll_for_response(serverUnderTest, testPlugin, 0x92, inputAbortedResponse);
	CANHardwareInterface::stop();

	ASSERT_TRUE(noInputOpenAnswered);
	EXPECT_EQ(0xFF, noInputOpenResponse.data[1]);
	EXPECT_EQ(0xFF, noInputOpenResponse.data[2]);
	EXPECT_EQ(0x01, noInputOpenResponse.data[3]);
	ASSERT_TRUE(inputAbortedAnswered);
	EXPECT_EQ(0x34, inputAbortedResponse.data[1]);
	EXPECT_EQ(0x12, inputAbortedResponse.data[2]);
	EXPECT_EQ(0x00, inputAbortedResponse.data[3]);
	EXPECT_EQ(0xFF, inputAbortedResponse.data[4]);
	EXPECT_EQ(0xFF, inputAbortedResponse.data[5]);
	EXPECT_EQ(0xFF, inputAbortedResponse.data[6]);
	EXPECT_EQ(0xFF, inputAbortedResponse.data[7]);
}

TEST_F(VirtualTerminalServerMessagingTest, PreferredAssignmentCommandIsAnswered)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x47, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xA4, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	CANMessageFrame response = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0x22, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool answered = poll_for_response(serverUnderTest, testPlugin, 0x22, response);
	CANHardwareInterface::stop();

	ASSERT_TRUE(answered);
	EXPECT_EQ(0x00, response.data[1]);
	EXPECT_EQ(0xFF, response.data[2]);
	EXPECT_EQ(0xFF, response.data[3]);
	EXPECT_EQ(0xFF, response.data[4]);
	EXPECT_EQ(0xFF, response.data[5]);
	EXPECT_EQ(0xFF, response.data[6]);
	EXPECT_EQ(0xFF, response.data[7]);
}

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

TEST_F(VirtualTerminalServerMessagingTest, MacroBlanksAStringWithAnEmptyChangeStringValue)
{
	auto serverControlFunction = test_helpers::create_mock_internal_control_function(0x80);
	auto clientControlFunction = test_helpers::create_mock_control_function(0x81);
	DerivedTestVTServer server(serverControlFunction);
	auto workingSet = std::make_shared<TestWorkingSet>(clientControlFunction);

	// String Variable 0x1234 holding "OK", then Macro 5 holding a Change String Value of 0 bytes, padded with 0xFF to 8 bytes
	workingSet->add_iop_raw_data({ 0x34, 0x12, static_cast<std::uint8_t>(VirtualTerminalObjectType::StringVariable), 0x02, 0x00, 'O', 'K', 0x05, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::Macro), 0x08, 0x00, static_cast<std::uint8_t>(Macro::Command::ChangeStringValue), 0x34, 0x12, 0x00, 0x00, 0xFF, 0xFF, 0xFF });
	workingSet->start_parsing_thread();
	workingSet->join_parsing_thread();
	server.add_managed_working_set(workingSet);

	EXPECT_TRUE(server.execute_macro_for_test(5, workingSet));
	EXPECT_EQ("  ", std::static_pointer_cast<StringVariable>(workingSet->get_object_by_id(0x1234))->get_value());
}

/// @brief A managed working set whose macros change one string variable, so that a test can see which of them ran
class MacroNestingTest : public VirtualTerminalServerMessagingTest
{
protected:
	void SetUp() override
	{
		VirtualTerminalServerMessagingTest::SetUp();
		stringVariable->set_id(0x1234);
		stringVariable->set_value("  ");
		ASSERT_TRUE(workingSet->add_object(stringVariable));
		server.add_managed_working_set(workingSet);
	}

	void add_macro(std::uint16_t id, const std::vector<std::vector<std::uint8_t>> &commands)
	{
		auto macro = std::make_shared<Macro>();
		macro->set_id(id);
		for (const auto &command : commands)
		{
			ASSERT_TRUE(macro->add_command_packet(command));
		}
		ASSERT_TRUE(workingSet->add_object(macro));
	}

	static std::vector<std::uint8_t> execute(std::uint8_t id)
	{
		return { static_cast<std::uint8_t>(Macro::Command::ExecuteMacro), id, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	}

	static std::vector<std::uint8_t> set_string(char value)
	{
		return { static_cast<std::uint8_t>(Macro::Command::ChangeStringValue), 0x34, 0x12, 0x02, 0x00, static_cast<std::uint8_t>(value), static_cast<std::uint8_t>(value), 0xFF };
	}

	DerivedTestVTServer server{ test_helpers::create_mock_internal_control_function(0x80) };
	std::shared_ptr<TestWorkingSet> workingSet = std::make_shared<TestWorkingSet>(test_helpers::create_mock_control_function(0x81));
	std::shared_ptr<StringVariable> stringVariable = std::make_shared<StringVariable>();
};

// A circular reference must not recurse until the stack overflows, which would take the VT down for every working set
TEST_F(MacroNestingTest, MacroThatExecutesItselfDoesNotRecurse)
{
	add_macro(1, { set_string('A'), execute(1) });

	EXPECT_TRUE(server.execute_macro_for_test(1, workingSet));
	EXPECT_EQ("AA", stringVariable->get_value());
}

TEST_F(MacroNestingTest, MacrosThatExecuteEachOtherDoNotRecurse)
{
	add_macro(1, { execute(2) });
	add_macro(2, { execute(1), set_string('B') });

	EXPECT_TRUE(server.execute_macro_for_test(1, workingSet));
	EXPECT_EQ("BB", stringVariable->get_value());

	// Nothing is left marked as running, so they run again
	stringVariable->set_value("  ");
	EXPECT_TRUE(server.execute_macro_for_test(2, workingSet));
	EXPECT_EQ("BB", stringVariable->get_value());
}

TEST_F(MacroNestingTest, MacroExecutesAnotherMacro)
{
	add_macro(1, { set_string('A'), execute(2) });
	add_macro(2, { set_string('C') });

	EXPECT_TRUE(server.execute_macro_for_test(1, workingSet));
	EXPECT_EQ("CC", stringVariable->get_value());
}

/// @brief A server that runs macro 1 of another working set the first time it is told to set the audio volume
class NestingTestVTServer : public DerivedTestVTServer
{
public:
	using DerivedTestVTServer::DerivedTestVTServer;

	void set_audio_volume(std::uint8_t, std::shared_ptr<ControlFunction>) override
	{
		auto workingSet = otherWorkingSet;
		otherWorkingSet = nullptr;
		if (nullptr != workingSet)
		{
			nestedMacroRan = execute_macro_for_test(1, workingSet);
		}
	}

	std::shared_ptr<VirtualTerminalServerManagedWorkingSet> otherWorkingSet;
	bool nestedMacroRan = false;
};

TEST_F(VirtualTerminalServerMessagingTest, EqualMacroIDsInOtherWorkingSetsAreNotCircular)
{
	auto serverControlFunction = test_helpers::create_mock_internal_control_function(0x80);
	NestingTestVTServer server(serverControlFunction);
	auto workingSetA = std::make_shared<TestWorkingSet>(test_helpers::create_mock_control_function(0x81));
	auto workingSetB = std::make_shared<TestWorkingSet>(test_helpers::create_mock_control_function(0x82));

	for (const auto &workingSet : { workingSetA, workingSetB })
	{
		auto macro = std::make_shared<Macro>();
		macro->set_id(1);
		ASSERT_TRUE(macro->add_command_packet({ static_cast<std::uint8_t>(Macro::Command::SetAudioVolume), 50, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }));
		ASSERT_TRUE(workingSet->add_object(macro));
		server.add_managed_working_set(workingSet);
	}
	server.otherWorkingSet = workingSetB;

	// Macro 1 of A sets the volume, which runs macro 1 of B while A's is still running
	EXPECT_TRUE(server.execute_macro_for_test(1, workingSetA));
	EXPECT_TRUE(server.nestedMacroRan);
}

TEST_F(VirtualTerminalServerMessagingTest, AudioCommandsAreForwardedToTheApplication)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x4A, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xA6, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	CANMessageFrame audioSignalResponse = {};
	CANMessageFrame audioVolumeResponse = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	// 3 activations of 880 Hz, 200 ms on and 100 ms off
	receive_from_client(internalECU, client, { 0xA3, 0x03, 0x70, 0x03, 0xC8, 0x00, 0x64, 0x00 });
	const bool audioSignalAnswered = poll_for_response(serverUnderTest, testPlugin, 0xA3, audioSignalResponse);
	receive_from_client(internalECU, client, { 0xA4, 0x4B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool audioVolumeAnswered = poll_for_response(serverUnderTest, testPlugin, 0xA4, audioVolumeResponse);
	receive_from_client(internalECU, client, { 0xA4, 0xFA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	CANHardwareInterface::stop();

	ASSERT_EQ(1U, serverUnderTest.audioSignals.size());
	EXPECT_EQ(3, serverUnderTest.audioSignals.front()[0]);
	EXPECT_EQ(880, serverUnderTest.audioSignals.front()[1]);
	EXPECT_EQ(200, serverUnderTest.audioSignals.front()[2]);
	EXPECT_EQ(100, serverUnderTest.audioSignals.front()[3]);
	EXPECT_EQ(client, serverUnderTest.audioSignalSources.front());
	ASSERT_EQ(2U, serverUnderTest.audioVolumes.size());
	EXPECT_EQ(75, serverUnderTest.audioVolumes.front());
	EXPECT_EQ(100, serverUnderTest.audioVolumes.back());
	EXPECT_EQ(client, serverUnderTest.audioVolumeSources.front());
	ASSERT_TRUE(audioSignalAnswered);
	EXPECT_EQ(0x00, audioSignalResponse.data[1]);
	ASSERT_TRUE(audioVolumeAnswered);
}

TEST_F(VirtualTerminalServerMessagingTest, UnsupportedFunctionSentDestinationSpecificIsAnsweredWithUnsupportedVTFunction)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x46, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xA3, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.version = VirtualTerminalBase::VTVersion::Version5;
	serverUnderTest.initialize();

	CANMessageFrame response = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0x50, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool answered = poll_for_response(serverUnderTest, testPlugin, 0xFD, response);
	CANHardwareInterface::stop();

	ASSERT_TRUE(answered);
	EXPECT_EQ(0x50, response.data[1]);
	EXPECT_EQ(0xFF, response.data[2]);
	EXPECT_EQ(0xFF, response.data[3]);
	EXPECT_EQ(0xFF, response.data[4]);
	EXPECT_EQ(0xFF, response.data[5]);
	EXPECT_EQ(0xFF, response.data[6]);
	EXPECT_EQ(0xFF, response.data[7]);
}

TEST_F(VirtualTerminalServerMessagingTest, UnsupportedFunctionSentToTheGlobalAddressIsNotAnswered)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x46, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xA3, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.version = VirtualTerminalBase::VTVersion::Version5;
	serverUnderTest.initialize();

	CANMessageFrame response = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(7,
	                                                                                                             static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
	                                                                                                             client,
	                                                                                                             { 0x50, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }));
	CANNetworkManager::CANNetwork.update();
	const bool answered = poll_for_response(serverUnderTest, testPlugin, 0xFD, response);
	CANHardwareInterface::stop();

	EXPECT_FALSE(answered);
}

TEST_F(VirtualTerminalServerMessagingTest, UnsupportedFunctionIsNotAnsweredByAVersion4VT)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x46, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xA3, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.version = VirtualTerminalBase::VTVersion::Version4;
	serverUnderTest.initialize();

	CANMessageFrame response = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0x50, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool answered = poll_for_response(serverUnderTest, testPlugin, 0xFD, response);
	CANHardwareInterface::stop();

	EXPECT_FALSE(answered);
}

TEST_F(VirtualTerminalServerMessagingTest, FunctionsTheVTAcceptsWithoutAnAnswerOrAnswersElsewhereAreNotReportedUnsupported)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x46, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xA3, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.version = VirtualTerminalBase::VTVersion::Version5;
	serverUnderTest.initialize();

	CANMessageFrame response = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	for (const std::uint8_t function : { 0x07, 0x09, 0x20, 0x24, 0x25, 0x26, 0xC1, 0xC2, 0xC3, 0xC4, 0xC7, 0xFD })
	{
		receive_from_client(internalECU, client, { function, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	}
	const bool answered = poll_for_response(serverUnderTest, testPlugin, 0xFD, response);
	CANHardwareInterface::stop();

	EXPECT_FALSE(answered);
}

namespace
{
	// Macros 10 to 18 each set Number Variable 100 to 108 to 1, so a test can see which macros ran:
	// - Working Set 0 runs macro 10 On Change Active Mask
	// - Data Mask 1000 (Soft Key Mask 2000) runs 11 On Show and 12 On Hide
	// - Data Mask 1001 (Soft Key Mask 2001) runs 13 On Show and 14 On Hide
	// - Soft Key Mask 2000 runs 15 On Show and 16 On Hide
	// - Soft Key Mask 2001 runs 17 On Show and 18 On Hide
	// The On Show macros also write their mask's ID, so a test can see which mask was shown last:
	// 11 and 13 into Number Variable 109, 15 and 17 into Number Variable 110.
	// extraCommandsOfMacro10 are added to macro 10. A non-empty onChangeSoftKeyMaskMacroOfMask1000 becomes macro 19,
	// which Data Mask 1000 runs On Change Soft Key Mask.
	std::vector<std::uint8_t> create_mask_change_macro_pool(const std::vector<std::uint8_t> &extraCommandsOfMacro10 = {},
	                                                        const std::vector<std::uint8_t> &onChangeSoftKeyMaskMacroOfMask1000 = {})
	{
		constexpr auto WORKING_SET = static_cast<std::uint8_t>(VirtualTerminalObjectType::WorkingSet);
		constexpr auto DATA_MASK = static_cast<std::uint8_t>(VirtualTerminalObjectType::DataMask);
		constexpr auto SOFT_KEY_MASK = static_cast<std::uint8_t>(VirtualTerminalObjectType::SoftKeyMask);
		constexpr auto NUMBER_VARIABLE = static_cast<std::uint8_t>(VirtualTerminalObjectType::NumberVariable);
		constexpr auto MACRO = static_cast<std::uint8_t>(VirtualTerminalObjectType::Macro);
		constexpr auto CHANGE_NUMERIC_VALUE = static_cast<std::uint8_t>(Macro::Command::ChangeNumericValue);
		const bool mask1000HasMacro19 = !onChangeSoftKeyMaskMacroOfMask1000.empty();

		// Object Pool Transfer
		std::vector<std::uint8_t> pool = { 0x11 };
		// Working Set 0, active mask 1000, macro 10 On Change Active Mask
		pool.insert(pool.end(), { 0x00, 0x00, WORKING_SET, 0x00, 0x01, 0xE8, 0x03, 0x00, 0x01, 0x00, 0x07, 10 });
		// Data Mask 1000 with Soft Key Mask 2000
		pool.insert(pool.end(), { 0xE8, 0x03, DATA_MASK, 0x00, 0xD0, 0x07, 0x00, static_cast<std::uint8_t>(mask1000HasMacro19 ? 3 : 2), 0x03, 11, 0x04, 12 });
		if (mask1000HasMacro19)
		{
			pool.insert(pool.end(), { 0x08, 19 });
		}
		// Data Mask 1001 with Soft Key Mask 2001
		pool.insert(pool.end(), { 0xE9, 0x03, DATA_MASK, 0x00, 0xD1, 0x07, 0x00, 0x02, 0x03, 13, 0x04, 14 });
		// Soft Key Masks 2000 and 2001
		pool.insert(pool.end(), { 0xD0, 0x07, SOFT_KEY_MASK, 0x00, 0x00, 0x02, 0x03, 15, 0x04, 16 });
		pool.insert(pool.end(), { 0xD1, 0x07, SOFT_KEY_MASK, 0x00, 0x00, 0x02, 0x03, 17, 0x04, 18 });

		// Number Variables 109 and 110 for the last shown Data Mask and Soft Key Mask
		pool.insert(pool.end(), { 109, 0x00, NUMBER_VARIABLE, 0x00, 0x00, 0x00, 0x00 });
		pool.insert(pool.end(), { 110, 0x00, NUMBER_VARIABLE, 0x00, 0x00, 0x00, 0x00 });

		for (std::uint8_t i = 0; i < 9; i++)
		{
			const auto variableID = static_cast<std::uint8_t>(100 + i);
			const std::vector<std::uint8_t> noCommands;
			// Macros 11, 13, 15 and 17 are the On Show macros of masks 1000, 1001, 2000 and 2001
			const std::vector<std::uint8_t> showMask1000 = { CHANGE_NUMERIC_VALUE, 109, 0x00, 0xFF, 0xE8, 0x03, 0x00, 0x00 };
			const std::vector<std::uint8_t> showMask1001 = { CHANGE_NUMERIC_VALUE, 109, 0x00, 0xFF, 0xE9, 0x03, 0x00, 0x00 };
			const std::vector<std::uint8_t> showMask2000 = { CHANGE_NUMERIC_VALUE, 110, 0x00, 0xFF, 0xD0, 0x07, 0x00, 0x00 };
			const std::vector<std::uint8_t> showMask2001 = { CHANGE_NUMERIC_VALUE, 110, 0x00, 0xFF, 0xD1, 0x07, 0x00, 0x00 };
			const std::vector<std::uint8_t> *extraCommandsOfMacro = &noCommands;
			switch (i)
			{
				case 0:
					extraCommandsOfMacro = &extraCommandsOfMacro10;
					break;
				case 1:
					extraCommandsOfMacro = &showMask1000;
					break;
				case 3:
					extraCommandsOfMacro = &showMask1001;
					break;
				case 5:
					extraCommandsOfMacro = &showMask2000;
					break;
				case 7:
					extraCommandsOfMacro = &showMask2001;
					break;
				default:
					break;
			}
			const std::vector<std::uint8_t> &extraCommands = *extraCommandsOfMacro;
			const auto macroLength = static_cast<std::uint8_t>(8 + extraCommands.size());
			pool.insert(pool.end(), { variableID, 0x00, NUMBER_VARIABLE, 0x00, 0x00, 0x00, 0x00 });
			pool.insert(pool.end(), { static_cast<std::uint8_t>(10 + i), 0x00, MACRO, macroLength, 0x00, CHANGE_NUMERIC_VALUE, variableID, 0x00, 0xFF, 0x01, 0x00, 0x00, 0x00 });
			pool.insert(pool.end(), extraCommands.begin(), extraCommands.end());
		}
		if (mask1000HasMacro19)
		{
			pool.insert(pool.end(), { 19, 0x00, MACRO, static_cast<std::uint8_t>(onChangeSoftKeyMaskMacroOfMask1000.size()), 0x00 });
			pool.insert(pool.end(), onChangeSoftKeyMaskMacroOfMask1000.begin(), onChangeSoftKeyMaskMacroOfMask1000.end());
		}
		return pool;
	}

	// Returns the IDs of the macros that ran since reset_mask_change_macros()
	std::vector<std::uint16_t> get_mask_change_macros_that_ran(const std::shared_ptr<VirtualTerminalServerManagedWorkingSet> &workingSet)
	{
		std::vector<std::uint16_t> macrosThatRan;
		for (std::uint16_t i = 0; i < 9; i++)
		{
			auto variable = workingSet->get_object_by_id(static_cast<std::uint16_t>(100 + i));
			if ((nullptr != variable) && (0 != std::static_pointer_cast<NumberVariable>(variable)->get_value()))
			{
				macrosThatRan.push_back(static_cast<std::uint16_t>(10 + i));
			}
		}
		return macrosThatRan;
	}

	// Returns the value of Number Variable 109 or 110, the last shown Data Mask or Soft Key Mask
	std::uint32_t get_last_shown(const std::shared_ptr<VirtualTerminalServerManagedWorkingSet> &workingSet, std::uint16_t variableID)
	{
		return std::static_pointer_cast<NumberVariable>(workingSet->get_object_by_id(variableID))->get_value();
	}

	void reset_mask_change_macros(const std::shared_ptr<VirtualTerminalServerManagedWorkingSet> &workingSet)
	{
		for (std::uint16_t i = 0; i < 11; i++)
		{
			std::static_pointer_cast<NumberVariable>(workingSet->get_object_by_id(static_cast<std::uint16_t>(100 + i)))->set_value(0);
		}
	}
}

class VirtualTerminalServerMaskChangeMacroTest : public VirtualTerminalServerMessagingTest
{
protected:
	// Uploads the pool and returns the macros that ran after the command was answered
	std::vector<std::uint16_t> run_command(std::uint8_t serverAddress,
	                                       std::uint8_t clientAddress,
	                                       bool selectWorkingSet,
	                                       std::initializer_list<std::uint8_t> command,
	                                       std::uint8_t &errorCode,
	                                       const std::vector<std::uint8_t> &pool = create_mask_change_macro_pool(),
	                                       std::uint32_t *lastShownDataMask = nullptr,
	                                       std::uint32_t *lastShownSoftKeyMask = nullptr)
	{
		VirtualCANPlugin testPlugin;
		testPlugin.open();

		CANHardwareInterface::set_number_of_can_channels(1);
		CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
		CANHardwareInterface::start(false);

		auto internalECU = test_helpers::claim_internal_control_function(serverAddress, 0, time_source);
		auto client = test_helpers::force_claim_partnered_control_function(clientAddress, 0);

		DerivedTestVTServer serverUnderTest(internalECU);
		serverUnderTest.selectOfferedWorkingSets = selectWorkingSet;
		serverUnderTest.initialize();

		CANMessageFrame poolResponse = {};
		CANMessageFrame commandResponse = {};
		std::vector<std::uint16_t> macrosThatRan;

		receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		receive_tp_from_client(internalECU, client, pool);
		receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		testPlugin.clear_queue();
		const bool poolAnswered = poll_for_response(serverUnderTest, testPlugin, 0x12, poolResponse);
		EXPECT_TRUE(poolAnswered);
		EXPECT_EQ(0x00, poolResponse.data[1]);
		EXPECT_EQ(selectWorkingSet, nullptr != serverUnderTest.get_active_working_set());

		if (poolAnswered)
		{
			const auto workingSet = serverUnderTest.managedWorkingSetList.front();
			reset_mask_change_macros(workingSet);
			receive_from_client(internalECU, client, command);
			EXPECT_TRUE(poll_for_response(serverUnderTest, testPlugin, *command.begin(), commandResponse));
			errorCode = (0xAD == *command.begin()) ? commandResponse.data[3] : commandResponse.data[5];
			macrosThatRan = get_mask_change_macros_that_ran(workingSet);
			if (nullptr != lastShownDataMask)
			{
				*lastShownDataMask = get_last_shown(workingSet, 109);
			}
			if (nullptr != lastShownSoftKeyMask)
			{
				*lastShownSoftKeyMask = get_last_shown(workingSet, 110);
			}
		}
		CANHardwareInterface::stop();
		return macrosThatRan;
	}
};

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeActiveMaskHidesTheOldMaskAndShowsTheNewOne)
{
	std::uint8_t errorCode = 0xFF;
	const auto macrosThatRan = run_command(0x4C, 0xA8, true, { 0xAD, 0x00, 0x00, 0xE9, 0x03, 0xFF, 0xFF, 0xFF }, errorCode);

	EXPECT_EQ(0x00, errorCode);
	// On Change Active Mask, then On Hide of Data Mask 1000 and Soft Key Mask 2000, then On Show of Data Mask 1001 and Soft Key Mask 2001
	EXPECT_EQ((std::vector<std::uint16_t>{ 10, 12, 13, 16, 17 }), macrosThatRan);
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeActiveMaskToTheActiveMaskOnlyRunsOnChangeActiveMask)
{
	std::uint8_t errorCode = 0xFF;
	const auto macrosThatRan = run_command(0x4D, 0xA9, true, { 0xAD, 0x00, 0x00, 0xE8, 0x03, 0xFF, 0xFF, 0xFF }, errorCode);

	EXPECT_EQ(0x00, errorCode);
	EXPECT_EQ((std::vector<std::uint16_t>{ 10 }), macrosThatRan);
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeActiveMaskOfAnInactiveWorkingSetOnlyRunsOnChangeActiveMask)
{
	std::uint8_t errorCode = 0xFF;
	const auto macrosThatRan = run_command(0x4E, 0xAC, false, { 0xAD, 0x00, 0x00, 0xE9, 0x03, 0xFF, 0xFF, 0xFF }, errorCode);

	EXPECT_EQ(0x00, errorCode);
	EXPECT_EQ((std::vector<std::uint16_t>{ 10 }), macrosThatRan);
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeSoftKeyMaskOfTheVisibleMaskHidesTheOldSoftKeyMaskAndShowsTheNewOne)
{
	std::uint8_t errorCode = 0xFF;
	const auto macrosThatRan = run_command(0x4F, 0xAD, true, { 0xAE, 0x01, 0xE8, 0x03, 0xD1, 0x07, 0xFF, 0xFF }, errorCode);

	EXPECT_EQ(0x00, errorCode);
	EXPECT_EQ((std::vector<std::uint16_t>{ 16, 17 }), macrosThatRan);
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeSoftKeyMaskOfAMaskThatIsNotShownRunsNoSoftKeyMaskMacros)
{
	std::uint8_t errorCode = 0xFF;
	const auto macrosThatRan = run_command(0x50, 0xAE, true, { 0xAE, 0x01, 0xE9, 0x03, 0xD0, 0x07, 0xFF, 0xFF }, errorCode);

	EXPECT_EQ(0x00, errorCode);
	EXPECT_TRUE(macrosThatRan.empty());
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeActiveMaskHidesTheSoftKeyMaskThatWasShownEvenIfAMacroChangesIt)
{
	// On Change Active Mask also gives Data Mask 1000 Soft Key Mask 2001. Soft Key Mask 2000 was the one on the display.
	const auto pool = create_mask_change_macro_pool({ 0xAE, 0x01, 0xE8, 0x03, 0xD1, 0x07, 0xFF, 0xFF });
	std::uint8_t errorCode = 0xFF;
	const auto macrosThatRan = run_command(0x53, 0xAF, true, { 0xAD, 0x00, 0x00, 0xE9, 0x03, 0xFF, 0xFF, 0xFF }, errorCode, pool);

	EXPECT_EQ(0x00, errorCode);
	EXPECT_EQ((std::vector<std::uint16_t>{ 10, 12, 13, 16, 17 }), macrosThatRan);
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, OnChangeActiveMaskMacroThatChangesTheActiveMaskAgainDoesNotRecurse)
{
	// On Change Active Mask sends Change Active Mask again, which would run the same macro again without end
	const auto pool = create_mask_change_macro_pool({ 0xAD, 0x00, 0x00, 0xE9, 0x03, 0xFF, 0xFF, 0xFF });
	std::uint8_t errorCode = 0xFF;
	const auto macrosThatRan = run_command(0x55, 0xD5, true, { 0xAD, 0x00, 0x00, 0xE9, 0x03, 0xFF, 0xFF, 0xFF }, errorCode, pool);
	EXPECT_EQ(0x00, errorCode);
	// The nested command finds macro 10 still running and skips it. Its mask is already the active one, so it hides
	// and shows nothing. Then the outer command hides Data Mask 1000 and shows Data Mask 1001.
	EXPECT_EQ((std::vector<std::uint16_t>{ 10, 12, 13, 16, 17 }), macrosThatRan);
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeActiveMaskShowsTheMaskLastThatItsMacroMakesActive)
{
	// On Change Active Mask switches straight back to Data Mask 1000
	const auto pool = create_mask_change_macro_pool({ 0xAD, 0x00, 0x00, 0xE8, 0x03, 0xFF, 0xFF, 0xFF });
	std::uint8_t errorCode = 0xFF;
	std::uint32_t lastShownDataMask = 0;
	std::uint32_t lastShownSoftKeyMask = 0;
	const auto macrosThatRan = run_command(0x56, 0xD6, true, { 0xAD, 0x00, 0x00, 0xE9, 0x03, 0xFF, 0xFF, 0xFF }, errorCode, pool, &lastShownDataMask, &lastShownSoftKeyMask);
	EXPECT_EQ(0x00, errorCode);
	EXPECT_EQ((std::vector<std::uint16_t>{ 10, 11, 12, 13, 14, 15, 16, 17, 18 }), macrosThatRan);
	// Data Mask 1000 is active again, so it and its Soft Key Mask have to be the last ones shown
	EXPECT_EQ(1000U, lastShownDataMask);
	EXPECT_EQ(2000U, lastShownSoftKeyMask);
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeSoftKeyMaskShowsTheSoftKeyMaskLastThatItsMacroSets)
{
	// On Change Soft Key Mask of Data Mask 1000 gives it Soft Key Mask 2000 back
	const auto pool = create_mask_change_macro_pool({}, { 0xAE, 0x01, 0xE8, 0x03, 0xD0, 0x07, 0xFF, 0xFF });
	std::uint8_t errorCode = 0xFF;
	std::uint32_t lastShownSoftKeyMask = 0;
	const auto macrosThatRan = run_command(0x57, 0xD7, true, { 0xAE, 0x01, 0xE8, 0x03, 0xD1, 0x07, 0xFF, 0xFF }, errorCode, pool, nullptr, &lastShownSoftKeyMask);
	EXPECT_EQ(0x00, errorCode);
	EXPECT_EQ((std::vector<std::uint16_t>{ 15, 16, 17, 18 }), macrosThatRan);
	EXPECT_EQ(2000U, lastShownSoftKeyMask);
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeSoftKeyMaskUsesTheVisibilityFromBeforeItsMacros)
{
	// On Change Soft Key Mask of Data Mask 1000 makes Data Mask 1001 the active mask
	const auto pool = create_mask_change_macro_pool({}, { 0xAD, 0x00, 0x00, 0xE9, 0x03, 0xFF, 0xFF, 0xFF });
	std::uint8_t errorCode = 0xFF;
	const auto macrosThatRan = run_command(0x54, 0xC3, true, { 0xAE, 0x01, 0xE8, 0x03, 0xD1, 0x07, 0xFF, 0xFF }, errorCode, pool);

	EXPECT_EQ(0x00, errorCode);
	// The macro's Change Active Mask runs On Change Active Mask, hides Data Mask 1000 with its new Soft Key Mask 2001 and
	// shows Data Mask 1001 with Soft Key Mask 2001. Then Soft Key Mask 2000 is hidden and 2001 shown, because Data Mask 1000
	// was on the display when the command came.
	EXPECT_EQ((std::vector<std::uint16_t>{ 10, 12, 13, 16, 17, 18 }), macrosThatRan);
}
