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
#include <atomic>
#include <chrono>
#include <cstdio>
#include <functional>
#include <string>
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
	struct Suspension
	{
		std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet;
		std::uint8_t errorCodes;
		std::uint16_t parentIDOfFaultingObject;
		std::uint16_t faultingObjectID;
		std::uint8_t objectPoolErrorCodes;
	};

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

	void suspend_working_set(std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet,
	                         std::uint8_t errorCodes,
	                         std::uint16_t parentIDOfFaultingObject,
	                         std::uint16_t faultingObjectID,
	                         std::uint8_t objectPoolErrorCodes) override
	{
		suspensions.push_back({ workingSet, errorCodes, parentIDOfFaultingObject, faultingObjectID, objectPoolErrorCodes });
		if (onSuspendWorkingSet)
		{
			onSuspendWorkingSet(workingSet);
		}
	}

	SupportedWideCharsErrorCode get_supported_wide_chars(std::uint8_t, std::uint16_t, std::uint16_t, std::uint8_t &numberOfRanges, std::vector<std::uint8_t> &) override
	{
		numberOfRanges = 0;
		return static_cast<SupportedWideCharsErrorCode>(0);
	}

	std::vector<std::array<std::uint8_t, 7>> get_versions(NAME) override
	{
		return versions;
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

			// A version stored again moves to the end, as get_versions() lists them in the order they were stored
			if (versionLabel.size() == VERSION_LABEL_SIZE)
			{
				std::array<std::uint8_t, VERSION_LABEL_SIZE> label;
				std::copy(versionLabel.begin(), versionLabel.end(), label.begin());
				versions.erase(std::remove(versions.begin(), versions.end(), label), versions.end());
				versions.push_back(label);
			}
		}
		return saveSucceeds;
	}

	bool delete_version(const std::vector<std::uint8_t> &versionLabel, NAME) override
	{
		lastVersionLabel = versionLabel;
		deletedVersionLabels.push_back(versionLabel);
		storedVersion.clear();
		if (deleteSucceeds)
		{
			versions.erase(std::remove_if(versions.begin(), versions.end(), [&versionLabel](const std::array<std::uint8_t, VERSION_LABEL_SIZE> &label) {
				               return std::equal(label.begin(), label.end(), versionLabel.begin(), versionLabel.end());
			               }),
			               versions.end());
		}
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

	std::uint16_t abort_input(std::shared_ptr<VirtualTerminalServerManagedWorkingSet> ws) override
	{
		abortInputCount++;
		std::uint16_t id = ws->get_object_open_for_input();

		if (NULL_OBJECT_ID == id)
		{
			id = inputOpenOnlyInApp;
		}
		inputOpenOnlyInApp = NULL_OBJECT_ID;

		if (NULL_OBJECT_ID != id)
		{
			ws->set_object_open_for_input(NULL_OBJECT_ID);
			send_select_input_object_message(id, false, false, ws->get_control_function());
		}
		return id;
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

	void on_rx_messages_pending() override
	{
		rxMessagesPendingCalls++;
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

	bool execute_macro_for_test(std::uint16_t objectID, std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet)
	{
		return execute_macro(objectID, workingSet);
	}

	using VirtualTerminalServer::busyCodesBitfield;
	using VirtualTerminalServer::DEFAULT_MAX_RX_QUEUE_SIZE_BYTES;
	using VirtualTerminalServer::managedWorkingSetList;
	using VirtualTerminalServer::process_rx_messages;
	using VirtualTerminalServer::send_status_message;
	using VirtualTerminalServer::store_rx_message;

	std::vector<std::uint8_t> versionToLoad;
	VTVersion version = VTVersion::Version3;
	bool saveSucceeds = true;
	bool deleteSucceeds = true;
	std::uint8_t deletedAllVersionsLabelLength = 0;
	std::vector<std::array<std::uint8_t, 32>> extendedVersions;
	static constexpr std::size_t VERSION_LABEL_SIZE = 7;
	std::vector<std::uint8_t> lastVersionLabel;
	std::vector<std::vector<std::uint8_t>> storedVersion;
	std::vector<std::array<std::uint8_t, VERSION_LABEL_SIZE>> versions; ///< The 7 byte labels stored, in the order they were stored
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
	std::vector<Suspension> suspensions;
	std::uint_fast8_t abortInputCount = 0;
	std::uint16_t inputOpenOnlyInApp = NULL_OBJECT_ID; ///< An input the app has open without calling set_object_open_for_input(), like a VT built against an older stack
	std::function<void(std::shared_ptr<VirtualTerminalServerManagedWorkingSet>)> onObjectPoolParseFinished;
	std::function<void(std::shared_ptr<VirtualTerminalServerManagedWorkingSet>)> onObjectPoolDeleted;
	std::function<void(std::shared_ptr<VirtualTerminalServerManagedWorkingSet>)> onSuspendWorkingSet;
	std::uint_fast8_t objectPoolDeletionRequests = 0;
	std::vector<std::uint8_t> supportedObjects;
	std::vector<std::array<std::uint16_t, 4>> audioSignals;
	std::vector<std::shared_ptr<ControlFunction>> audioSignalSources;
	std::vector<std::uint8_t> audioVolumes;
	std::vector<std::shared_ptr<ControlFunction>> audioVolumeSources;
	std::atomic<std::uint32_t> rxMessagesPendingCalls = { 0 };
};

class VirtualTerminalServerMessagingTest : public AgIsoStackTestFixture
{
protected:
	/// @brief Injects a message from the client and lets the network manager deliver it, without the server processing it
	static void receive_from_client_without_processing(std::shared_ptr<InternalControlFunction> server,
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

	void receive_from_client(std::shared_ptr<InternalControlFunction> server,
	                         std::shared_ptr<ControlFunction> client,
	                         std::initializer_list<std::uint8_t> data)
	{
		receive_from_client_without_processing(server, client, data);
		serverToDrain->process_rx_messages();
	}

	// Version 2 and prior clients have to send this before they can be managed
	void receive_working_set_master_from_client(std::shared_ptr<ControlFunction> client)
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(5,
		                                                                                                             static_cast<std::uint32_t>(CANLibParameterGroupNumber::WorkingSetMaster),
		                                                                                                             client,
		                                                                                                             { 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }));
		CANNetworkManager::CANNetwork.update();
		serverToDrain->process_rx_messages();
	}

	/// @brief Sends a payload through the transport protocol and lets the network manager deliver it, without the server processing it
	static void receive_tp_from_client_without_processing(std::shared_ptr<InternalControlFunction> server,
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

	void receive_tp_from_client(std::shared_ptr<InternalControlFunction> server,
	                            std::shared_ptr<ControlFunction> client,
	                            const std::vector<std::uint8_t> &payload)
	{
		receive_tp_from_client_without_processing(server, client, payload);
		serverToDrain->process_rx_messages();
	}

	static std::vector<std::uint8_t> extended_version_command(std::uint8_t function, const std::string &label)
	{
		std::vector<std::uint8_t> command(33, ' ');
		command[0] = function;
		std::copy(label.begin(), label.end(), command.begin() + 1);
		return command;
	}

	/// @brief Returns the 8 data bytes of a frame
	static std::vector<std::uint8_t> payload_of(const CANMessageFrame &frame)
	{
		return std::vector<std::uint8_t>(frame.data, frame.data + 8);
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

	void receive_working_set_member_from_client(std::shared_ptr<ControlFunction> master, std::shared_ptr<ControlFunction> member)
	{
		const std::uint64_t memberName = member->get_NAME().get_full_name();
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(5,
		                                                                                                             static_cast<std::uint32_t>(CANLibParameterGroupNumber::WorkingSetMember),
		                                                                                                             master,
		                                                                                                             { static_cast<std::uint8_t>(memberName),
		                                                                                                               static_cast<std::uint8_t>(memberName >> 8),
		                                                                                                               static_cast<std::uint8_t>(memberName >> 16),
		                                                                                                               static_cast<std::uint8_t>(memberName >> 24),
		                                                                                                               static_cast<std::uint8_t>(memberName >> 32),
		                                                                                                               static_cast<std::uint8_t>(memberName >> 40),
		                                                                                                               static_cast<std::uint8_t>(memberName >> 48),
		                                                                                                               static_cast<std::uint8_t>(memberName >> 56) }));
		CANNetworkManager::CANNetwork.update();
		serverToDrain->process_rx_messages();
	}

	void receive_working_set_master_with_members_from_client(std::shared_ptr<ControlFunction> master, std::uint8_t workingSetSize)
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(5,
		                                                                                                             static_cast<std::uint32_t>(CANLibParameterGroupNumber::WorkingSetMaster),
		                                                                                                             master,
		                                                                                                             { workingSetSize, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }));
		CANNetworkManager::CANNetwork.update();
		serverToDrain->process_rx_messages();
	}

	// A master announces one member and starts its working set with the given version, then the member asks for the number of soft keys
	bool read_soft_keys_response_destination_for_member(std::uint8_t serverAddress,
	                                                    VirtualTerminalBase::VTVersion vtVersion,
	                                                    std::uint8_t workingSetVersion,
	                                                    bool announceMember,
	                                                    std::uint8_t &masterAddress,
	                                                    std::uint8_t &memberAddress,
	                                                    std::uint8_t &destination,
	                                                    std::function<void(DerivedTestVTServer &, std::shared_ptr<ControlFunction>, std::shared_ptr<ControlFunction>)> beforeRequest = nullptr)
	{
		VirtualCANPlugin testPlugin;
		testPlugin.open();

		CANHardwareInterface::set_number_of_can_channels(1);
		CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
		CANHardwareInterface::start(false);

		auto internalECU = test_helpers::claim_internal_control_function(serverAddress, 0, time_source);
		auto master = test_helpers::force_claim_partnered_control_function(serverAddress + 1, 0);
		auto member = test_helpers::force_claim_partnered_control_function(serverAddress + 2, 0);
		masterAddress = master->get_address();
		memberAddress = member->get_address();

		DerivedTestVTServer serverUnderTest(internalECU);
		serverUnderTest.version = vtVersion;
		serverUnderTest.initialize();
		serverToDrain = &serverUnderTest;

		receive_working_set_master_with_members_from_client(master, 0x02);
		if (announceMember)
		{
			receive_working_set_member_from_client(master, member);
		}
		receive_from_client(internalECU, master, { 0xFF, 0x01, workingSetVersion, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		if (beforeRequest)
		{
			beforeRequest(serverUnderTest, master, member);
		}

		testPlugin.clear_queue();
		receive_from_client(internalECU, member, { 0xC2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

		CANMessageFrame response = {};
		const bool foundResponse = poll_for_response(serverUnderTest, testPlugin, 0xC2, response);
		destination = static_cast<std::uint8_t>(response.identifier >> 8);
		CANHardwareInterface::stop();
		serverToDrain = nullptr;
		return foundResponse;
	}

	bool read_status_busy_codes(const DerivedTestVTServer &server, VirtualCANPlugin &plugin, std::uint8_t &busyCodes)
	{
		CANMessageFrame statusFrame = {};
		const bool foundStatus = read_status_message(server, plugin, statusFrame);
		busyCodes = statusFrame.data[6];
		return foundStatus;
	}

	DerivedTestVTServer *serverToDrain = nullptr; ///< The server the receive helpers drain, set by fixtures that own one
};

/// @brief An object pool with a selectable Working Set
static const std::vector<std::uint8_t> SELECTABLE_OBJECT_POOL = { 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF };

/// @brief The selectable pool with Output Polygon 2 (100x50, points (0,0) (100,0) (33,17)), Output Line 3 (10x20, direction 0)
/// and Macro 5, which Line 3 runs On Change End Point to resize Polygon 2 to 7x8
static const std::vector<std::uint8_t> POLYGON_AND_LINE_OBJECT_POOL = { 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF, 0x02, 0x00, 0x10, 0x64, 0x00, 0x32, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x21, 0x00, 0x11, 0x00, 0x03, 0x00, 0x0D, 0xFF, 0xFF, 0x0A, 0x00, 0x14, 0x00, 0x00, 0x01, 0x12, 0x05, 0x05, 0x00, 0x1C, 0x08, 0x00, 0xA6, 0x02, 0x00, 0x07, 0x00, 0x08, 0x00, 0xFF };

/// @brief A VT server on a virtual bus with one connected client
class VirtualTerminalServerBusTest : public VirtualTerminalServerMessagingTest
{
protected:
	void SetUp() override
	{
		VirtualTerminalServerMessagingTest::SetUp();
		testPlugin.open();

		CANHardwareInterface::set_number_of_can_channels(1);
		CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
		CANHardwareInterface::start(false);

		internalECU = test_helpers::claim_internal_control_function(0x26, 0, time_source);
		client = test_helpers::force_claim_partnered_control_function(0x81, 0);
		server.reset(new DerivedTestVTServer(internalECU));
		server->initialize();
		serverToDrain = server.get();
		testPlugin.clear_queue();
	}

	void TearDown() override
	{
		CANHardwareInterface::stop();
		serverToDrain = nullptr;
		server.reset();
		VirtualTerminalServerMessagingTest::TearDown();
	}

	/// @brief Connects the uploader and sends it the pool, returns true when the End of Object Pool response reports no error
	bool upload_pool(const std::vector<std::uint8_t> &pool, std::shared_ptr<ControlFunction> uploader = nullptr)
	{
		CANMessageFrame response = {};

		if (nullptr == uploader)
		{
			uploader = client;
		}
		receive_from_client(internalECU, uploader, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		receive_tp_from_client(internalECU, uploader, pool);
		receive_from_client(internalECU, uploader, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		testPlugin.clear_queue();
		return poll_for_response(*server, testPlugin, 0x12, response) && (0x00 == response.data[1]);
	}

	/// @brief Makes the server answer Load Version with the VT3 test pool
	void load_vt3_pool_into_server()
	{
		server->versionToLoad = test_helpers::read_vt3_test_pool();
		ASSERT_FALSE(server->versionToLoad.empty());
	}

	/// @brief Sends a command from the client and polls for the VT's response, by default the one carrying the command's own function code
	bool send_and_poll(std::initializer_list<std::uint8_t> command, CANMessageFrame &response, std::uint8_t responseFunction = 0)
	{
		receive_from_client(internalECU, client, command);
		return poll_for_response(*server, testPlugin, (0 == responseFunction) ? *command.begin() : responseFunction, response);
	}

	/// @brief Runs update() a few times and counts the VT to ECU frames with this function code and the NACKs the server sent
	void count_responses_and_nacks(std::uint8_t function, std::uint32_t &responses, std::uint32_t &nacks)
	{
		CANMessageFrame frame = {};

		responses = 0;
		nacks = 0;
		for (std::uint_fast8_t attempt = 0; attempt < 10; attempt++)
		{
			server->run_update();
			time_source.update_for_ms(5);
			while (testPlugin.read_frame(frame, 10))
			{
				if ((0xE6 == get_pdu_format(frame)) && (function == frame.data[0]))
				{
					responses++;
				}
				else if ((0xE8 == get_pdu_format(frame)) && (0x01 == frame.data[0]))
				{
					nacks++;
				}
			}
		}
	}

	/// @brief Connects the client and drops its object pool transfer, as if the queue was full
	void drop_object_pool_transfer()
	{
		receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		server->set_max_rx_queue_size(1);
		receive_from_client_without_processing(internalECU, client, { 0xC0, 0xFF, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF });
		receive_tp_from_client_without_processing(internalECU, client, SELECTABLE_OBJECT_POOL);
		server->process_rx_messages();
		server->set_max_rx_queue_size(DerivedTestVTServer::DEFAULT_MAX_RX_QUEUE_SIZE_BYTES);
	}

	/// @brief Checks that suspend_working_set() was called once, for the only working set, with the given response fields
	void expect_single_suspension(std::uint8_t errorCodes, std::uint16_t parentIDOfFaultingObject, std::uint16_t faultingObjectID, std::uint8_t objectPoolErrorCodes) const
	{
		ASSERT_EQ(1U, server->suspensions.size());
		EXPECT_EQ(server->managedWorkingSetList.front(), server->suspensions.front().workingSet);
		EXPECT_EQ(errorCodes, server->suspensions.front().errorCodes);
		EXPECT_EQ(parentIDOfFaultingObject, server->suspensions.front().parentIDOfFaultingObject);
		EXPECT_EQ(faultingObjectID, server->suspensions.front().faultingObjectID);
		EXPECT_EQ(objectPoolErrorCodes, server->suspensions.front().objectPoolErrorCodes);
	}

	/// @brief Names a hook call for the order log, like "deleted:82" for the working set at address 0x82
	static std::string describe_hook_call(const char *hook, const std::shared_ptr<VirtualTerminalServerManagedWorkingSet> &workingSet)
	{
		char text[32];

		std::snprintf(text, sizeof(text), "%s:%x", hook, static_cast<unsigned>(workingSet->get_control_function()->get_address()));
		return text;
	}

	VirtualCANPlugin testPlugin;
	std::shared_ptr<InternalControlFunction> internalECU;
	std::shared_ptr<PartneredControlFunction> client;
	std::unique_ptr<DerivedTestVTServer> server;
};

TEST_F(VirtualTerminalServerBusTest, NackForUnmanagedWorkingSetIsSentToTheWorkingSetMasterNotGlobally)
{
	EXPECT_TRUE(server->get_initialized());

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
}

TEST_F(VirtualTerminalServerBusTest, BroadcastFromUnmanagedClientIsNotNacked)
{
	// An input unit broadcasts its Auxiliary Input Type 2 Maintenance message before it connects (ISO 11783-6 J.7.10).
	// A NACK makes it drop the connection it is starting.
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(3,
	                                                                                                             static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
	                                                                                                             client,
	                                                                                                             { 0x23, 0x34, 0x12, 0x00, 0xFF, 0xFF, 0xFF, 0xFF }));
	CANNetworkManager::CANNetwork.update();
	server->process_rx_messages();
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
}

TEST_F(VirtualTerminalServerBusTest, CommandsAreDroppedWhenTheQueueIsFull)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	server->set_max_rx_queue_size(1);
	testPlugin.clear_queue();

	receive_from_client_without_processing(internalECU, client, { 0xC0, 0xFF, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF });
	receive_from_client_without_processing(internalECU, client, { 0xC0, 0xFF, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF });

	std::uint32_t responses = 0;
	std::uint32_t nacks = 0;
	count_responses_and_nacks(0xC0, responses, nacks);

	EXPECT_EQ(1U, responses);
	EXPECT_EQ(0U, nacks);
}

TEST_F(VirtualTerminalServerBusTest, AMessageLargerThanTheLimitIsQueuedWhenTheQueueIsEmpty)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	server->set_max_rx_queue_size(1);
	testPlugin.clear_queue();

	receive_from_client_without_processing(internalECU, client, { 0xC0, 0xFF, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF });

	std::uint32_t responses = 0;
	std::uint32_t nacks = 0;
	count_responses_and_nacks(0xC0, responses, nacks);

	EXPECT_EQ(1U, responses);
	EXPECT_EQ(0U, nacks);
}

TEST_F(VirtualTerminalServerBusTest, DroppedObjectPoolTransferFailsEndOfObjectPool)
{
	drop_object_pool_transfer();
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	CANMessageFrame response = {};
	ASSERT_TRUE(poll_for_response(*server, testPlugin, 0x12, response));
	EXPECT_EQ(0x02, response.data[1]); // VT ran out of memory during transfer
	EXPECT_EQ(0x08, response.data[6]); // Object pool was deleted from volatile memory
	EXPECT_EQ(1U, server->deletedObjectPools.size());
	EXPECT_TRUE(server->parsedWorkingSets.empty());
	expect_single_suspension(0x02, NULL_OBJECT_ID, NULL_OBJECT_ID, 0x08);
}

TEST_F(VirtualTerminalServerBusTest, UploadWorksAgainAfterADroppedTransferFailed)
{
	drop_object_pool_transfer();
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	CANMessageFrame response = {};
	ASSERT_TRUE(poll_for_response(*server, testPlugin, 0x12, response));

	receive_tp_from_client(internalECU, client, SELECTABLE_OBJECT_POOL);
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	ASSERT_TRUE(poll_for_response(*server, testPlugin, 0x12, response));
	EXPECT_EQ(0x00, response.data[1]);
	EXPECT_EQ(1U, server->suspensions.size());
}

TEST_F(VirtualTerminalServerBusTest, DeleteObjectPoolAfterADroppedTransferAllowsANewUpload)
{
	drop_object_pool_transfer();
	ASSERT_EQ(1U, server->managedWorkingSetList.size());
	EXPECT_TRUE(server->managedWorkingSetList.front()->get_object_pool_transfer_dropped());

	receive_from_client(internalECU, client, { 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	EXPECT_FALSE(server->managedWorkingSetList.front()->get_object_pool_transfer_dropped());

	receive_tp_from_client(internalECU, client, SELECTABLE_OBJECT_POOL);
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	CANMessageFrame response = {};
	ASSERT_TRUE(poll_for_response(*server, testPlugin, 0x12, response));
	EXPECT_EQ(0x00, response.data[1]);
}

TEST_F(VirtualTerminalServerBusTest, AClientsMessagesAfterItsDroppedTransferAreDroppedUntilTheDrain)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	server->set_max_rx_queue_size(1);
	receive_from_client_without_processing(internalECU, client, { 0xC0, 0xFF, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF });
	receive_tp_from_client_without_processing(internalECU, client, SELECTABLE_OBJECT_POOL);
	server->set_max_rx_queue_size(DerivedTestVTServer::DEFAULT_MAX_RX_QUEUE_SIZE_BYTES);
	receive_from_client_without_processing(internalECU, client, { 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	server->process_rx_messages();

	ASSERT_EQ(1U, server->managedWorkingSetList.size());
	EXPECT_TRUE(server->managedWorkingSetList.front()->get_object_pool_transfer_dropped());

	CANMessageFrame response = {};
	EXPECT_FALSE(poll_for_response(*server, testPlugin, 0xB2, response));
}

TEST_F(VirtualTerminalServerBusTest, OtherClientsMessagesAreQueuedAfterADroppedTransfer)
{
	auto otherClient = test_helpers::force_claim_partnered_control_function(0x82, 0);
	receive_from_client(internalECU, otherClient, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	server->set_max_rx_queue_size(1);
	receive_from_client_without_processing(internalECU, client, { 0xC0, 0xFF, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF });
	receive_tp_from_client_without_processing(internalECU, client, SELECTABLE_OBJECT_POOL);
	server->set_max_rx_queue_size(DerivedTestVTServer::DEFAULT_MAX_RX_QUEUE_SIZE_BYTES);
	receive_from_client_without_processing(internalECU, otherClient, { 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	server->process_rx_messages();

	CANMessageFrame response = {};
	EXPECT_TRUE(poll_for_response(*server, testPlugin, 0xB2, response));
}

TEST_F(VirtualTerminalServerBusTest, DroppedObjectPoolTransferFromAnUnmanagedSourceChangesNothing)
{
	server->set_max_rx_queue_size(1);

	receive_from_client_without_processing(internalECU, client, { 0xC0, 0xFF, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF });
	receive_tp_from_client_without_processing(internalECU, client, SELECTABLE_OBJECT_POOL);
	testPlugin.clear_queue();

	std::uint32_t responses = 0;
	std::uint32_t nacks = 0;
	count_responses_and_nacks(0x12, responses, nacks);

	EXPECT_EQ(0U, responses);
	EXPECT_EQ(0U, nacks);
	EXPECT_TRUE(server->managedWorkingSetList.empty());
}

TEST_F(VirtualTerminalServerBusTest, PendingHookIsCalledOncePerDrain)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const std::uint32_t before = server->rxMessagesPendingCalls;

	receive_from_client_without_processing(internalECU, client, { 0xC0, 0xFF, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF });
	receive_from_client_without_processing(internalECU, client, { 0xC0, 0xFF, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF });
	EXPECT_EQ(before + 1, server->rxMessagesPendingCalls);

	server->process_rx_messages();
	receive_from_client_without_processing(internalECU, client, { 0xC0, 0xFF, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF });
	EXPECT_EQ(before + 2, server->rxMessagesPendingCalls);
}

TEST_F(VirtualTerminalServerBusTest, EndOfObjectPoolResponseReportsAnErrorWhenTheObjectPoolFailsToParse)
{
	// A working set maintenance message with the initiate bit set connects this client to the server
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

	// Object pool data that cannot be parsed into any object
	receive_from_client(internalECU, client, { 0x11, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA });

	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	std::vector<std::string> order;
	server->onObjectPoolParseFinished = [&](std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet) {
		order.push_back(describe_hook_call("parsed", workingSet));
	};
	server->onSuspendWorkingSet = [&](std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet) {
		order.push_back(describe_hook_call("suspended", workingSet));
	};

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(*server, testPlugin, 0x12, responseFrame);

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x01, responseFrame.data[1]); // Error in object pool
	EXPECT_EQ(0x04, responseFrame.data[6]); // Any other error
	ASSERT_EQ(1U, server->parseResults.size());
	EXPECT_FALSE(server->parseResults.front());
	ASSERT_EQ(1U, server->suspensions.size());
	EXPECT_EQ(0x01, server->suspensions.front().errorCodes);
	EXPECT_EQ(0x04, server->suspensions.front().objectPoolErrorCodes);
	EXPECT_EQ((std::vector<std::string>{ "parsed:81", "suspended:81" }), order);
}

TEST_F(VirtualTerminalServerBusTest, EndOfObjectPoolResponseReportsAnErrorWhenTheObjectPoolHasNoWorkingSetObject)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

	// A single number variable object, which parses without error but leaves the pool with no working set object
	receive_from_client(internalECU, client, { 0x11, 0x01, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00 });

	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(*server, testPlugin, 0x12, responseFrame);

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x01, responseFrame.data[1]); // Error in object pool
	EXPECT_EQ(0x04, responseFrame.data[6]); // Any other error
	expect_single_suspension(0x01, NULL_OBJECT_ID, NULL_OBJECT_ID, 0x04);
}

TEST_F(VirtualTerminalServerBusTest, EndOfObjectPoolResponseReportsTheParentOfTheFaultyObject)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	// Working Set 258 with child 1, then object 1 of an unknown type
	receive_tp_from_client(internalECU, client, { 0x11, 0x02, 0x01, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xAA, 0xAA });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(*server, testPlugin, 0x12, responseFrame);

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x01, responseFrame.data[1]);
	EXPECT_EQ(258, responseFrame.data[2] | (responseFrame.data[3] << 8));
	EXPECT_EQ(1, responseFrame.data[4] | (responseFrame.data[5] << 8));
	expect_single_suspension(0x01, 258, 1, 0x04);
}

TEST_F(VirtualTerminalServerBusTest, EndOfObjectPoolResponseReportsNoParentWhenTheFaultyObjectHasNoID)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	// Input List 2 with one NULL item, then 3 bytes, too short to hold an object
	receive_tp_from_client(internalECU, client, { 0x11, 0x02, 0x00, 0x0A, 0x10, 0x00, 0x10, 0x00, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0x03, 0x00, 0x01 });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(*server, testPlugin, 0x12, responseFrame);

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x01, responseFrame.data[1]);
	EXPECT_EQ(NULL_OBJECT_ID, responseFrame.data[2] | (responseFrame.data[3] << 8));
	EXPECT_EQ(NULL_OBJECT_ID, responseFrame.data[4] | (responseFrame.data[5] << 8));
	EXPECT_NE(nullptr, server->managedWorkingSetList.front()->get_object_by_id(2));
}

TEST_F(VirtualTerminalServerBusTest, FirstSelectableWorkingSetIsSelectedAutomatically)
{
	auto selectableClient = test_helpers::force_claim_partnered_control_function(0x82, 0);

	CANMessageFrame statusAfterNotSelectable = {};
	CANMessageFrame statusAfterSelectable = {};

	// Working Set with Selectable (byte 5) = 0
	auto notSelectablePool = SELECTABLE_OBJECT_POOL;
	notSelectablePool[5] = 0x00;
	const bool notSelectablePoolAnswered = upload_pool(notSelectablePool);
	const bool foundStatusAfterNotSelectable = read_status_message(*server, testPlugin, statusAfterNotSelectable);

	const bool selectablePoolAnswered = upload_pool(SELECTABLE_OBJECT_POOL, selectableClient);
	const bool foundStatusAfterSelectable = read_status_message(*server, testPlugin, statusAfterSelectable);

	ASSERT_TRUE(notSelectablePoolAnswered);
	ASSERT_TRUE(foundStatusAfterNotSelectable);
	EXPECT_EQ(NULL_CAN_ADDRESS, statusAfterNotSelectable.data[1]);
	ASSERT_TRUE(selectablePoolAnswered);
	ASSERT_TRUE(foundStatusAfterSelectable);
	EXPECT_EQ(0x82, statusAfterSelectable.data[1]);
	EXPECT_EQ(0x03E8, statusAfterSelectable.data[2] | (statusAfterSelectable.data[3] << 8));
}

// The server reads the object with the Working Set's ID as the Working Set when it answers the pool
TEST_F(VirtualTerminalServerBusTest, PoolThatGivesTheWorkingSetIdToAnotherObjectTypeIsRejected)
{
	auto pool = SELECTABLE_OBJECT_POOL;
	pool.insert(pool.end(), { 0x00, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::NumberVariable), 0x2A, 0x00, 0x00, 0x00 });

	EXPECT_FALSE(upload_pool(pool));
	EXPECT_EQ(nullptr, server->get_active_working_set());
}

TEST_F(VirtualTerminalServerBusTest, ObjectPoolParseFinishedIsCalledWithTheWorkingSetAlreadyActive)
{
	const bool poolLoaded = upload_pool(SELECTABLE_OBJECT_POOL);

	ASSERT_TRUE(poolLoaded);
	ASSERT_EQ(1U, server->parsedWorkingSets.size());
	EXPECT_TRUE(server->parseResults.front());
	EXPECT_EQ(server->managedWorkingSetList.front(), server->parsedWorkingSets.front());
	EXPECT_EQ(server->parsedWorkingSets.front(), server->activeWorkingSetsWhenParsed.front());
}

TEST_F(VirtualTerminalServerBusTest, ApplicationCanLeaveAnAcceptedWorkingSetInactive)
{
	server->selectOfferedWorkingSets = false;

	CANMessageFrame statusFrame = {};

	const bool poolLoaded = upload_pool(SELECTABLE_OBJECT_POOL);
	const bool foundStatus = read_status_message(*server, testPlugin, statusFrame);

	ASSERT_TRUE(poolLoaded);
	ASSERT_EQ(1U, server->workingSetsOfferedForSelection.size());
	EXPECT_EQ(server->managedWorkingSetList.front(), server->workingSetsOfferedForSelection.front());
	ASSERT_EQ(1U, server->activeWorkingSetsWhenParsed.size());
	EXPECT_EQ(nullptr, server->activeWorkingSetsWhenParsed.front());
	ASSERT_TRUE(foundStatus);
	EXPECT_EQ(NULL_CAN_ADDRESS, statusFrame.data[1]);
}

TEST_F(VirtualTerminalServerBusTest, ExtendingTheActiveObjectPoolDoesNotSelectTheWorkingSetAgain)
{
	CANMessageFrame extensionResponse = {};

	const bool poolLoaded = upload_pool(SELECTABLE_OBJECT_POOL);
	receive_tp_from_client(internalECU, client, { 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	const bool foundExtensionResponse = poll_for_response(*server, testPlugin, 0x12, extensionResponse);

	ASSERT_TRUE(poolLoaded);
	ASSERT_TRUE(foundExtensionResponse);
	EXPECT_EQ(0x00, extensionResponse.data[1]);
	EXPECT_EQ(2U, server->parseResults.size());
	EXPECT_EQ(1U, server->workingSetsOfferedForSelection.size());
	EXPECT_EQ(server->managedWorkingSetList.front(), server->get_active_working_set());
}

TEST_F(VirtualTerminalServerBusTest, DeleteObjectPoolDeletesThePoolAndKeepsTheWorkingSetManaged)
{
	CANMessageFrame deleteResponse = {};
	CANMessageFrame statusAfterDelete = {};
	CANMessageFrame reloadResponse = {};

	const bool poolLoaded = upload_pool(SELECTABLE_OBJECT_POOL);
	const bool foundDeleteResponse = send_and_poll({ 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, deleteResponse);
	const bool foundStatusAfterDelete = read_status_message(*server, testPlugin, statusAfterDelete);
	const bool activeCleared = (nullptr == server->get_active_working_set());
	const bool stillManaged = is_managed(internalECU, client, testPlugin);
	const bool poolDeleted = stillManaged &&
	  (nullptr == server->managedWorkingSetList.front()->get_working_set_object()) &&
	  (VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::None == server->managedWorkingSetList.front()->get_object_pool_processing_state());
	receive_tp_from_client(internalECU, client, { 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	const bool foundReloadResponse = poll_for_response(*server, testPlugin, 0x12, reloadResponse);

	ASSERT_TRUE(poolLoaded);
	ASSERT_TRUE(foundDeleteResponse);
	EXPECT_EQ(0x00, deleteResponse.data[1]);
	EXPECT_EQ(1U, server->objectPoolDeletionRequests);
	ASSERT_TRUE(foundStatusAfterDelete);
	EXPECT_EQ(NULL_CAN_ADDRESS, statusAfterDelete.data[1]);
	EXPECT_TRUE(activeCleared);
	EXPECT_TRUE(stillManaged);
	EXPECT_TRUE(poolDeleted);
	ASSERT_TRUE(foundReloadResponse);
	EXPECT_EQ(0x00, reloadResponse.data[1]);
	EXPECT_EQ(2U, server->parseResults.size());
	EXPECT_TRUE(server->shutDownWorkingSets.empty());
	EXPECT_TRUE(server->suspensions.empty());
}

TEST_F(VirtualTerminalServerBusTest, ObjectPoolDeletedIsCalledFromUpdate)
{
	const bool poolLoaded = upload_pool(SELECTABLE_OBJECT_POOL);
	receive_from_client(internalECU, client, { 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const std::size_t deletionsReportedBeforeUpdate = server->deletedObjectPools.size();
	server->run_update();
	server->run_update();

	ASSERT_TRUE(poolLoaded);
	EXPECT_EQ(1U, server->objectPoolDeletionRequests);
	EXPECT_EQ(0U, deletionsReportedBeforeUpdate);
	ASSERT_EQ(1U, server->deletedObjectPools.size());
	EXPECT_EQ(server->managedWorkingSetList.front(), server->deletedObjectPools.front());
	EXPECT_EQ(nullptr, server->activeWorkingSetsWhenDeleted.front());
}

TEST_F(VirtualTerminalServerBusTest, WorkingSetIsSelectedBeforeTheParseCallbacksOfTheSameUpdate)
{
	auto selectableClient = test_helpers::force_claim_partnered_control_function(0x82, 0);

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_tp_from_client(internalECU, client, { 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, selectableClient, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_tp_from_client(internalECU, selectableClient, { 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF });
	receive_from_client(internalECU, selectableClient, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	server->wait_for_object_pool_to_parse(0);
	server->wait_for_object_pool_to_parse(1);

	server->onObjectPoolParseFinished = [&](std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet) {
		if (0x81 == workingSet->get_control_function()->get_address())
		{
			receive_from_client(internalECU, selectableClient, { 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		}
	};
	server->run_update();

	ASSERT_EQ(2U, server->parsedWorkingSets.size());
	EXPECT_EQ(0x81, server->parsedWorkingSets.front()->get_control_function()->get_address());
	ASSERT_EQ(1U, server->workingSetsOfferedForSelection.size());
	EXPECT_EQ(0x82, server->workingSetsOfferedForSelection.front()->get_control_function()->get_address());
	EXPECT_EQ(server->workingSetsOfferedForSelection.front(), server->activeWorkingSetsWhenParsed.front());
	EXPECT_EQ(nullptr, server->get_active_working_set());
}

TEST_F(VirtualTerminalServerBusTest, ObjectPoolDeletedIsCalledBeforeTheNextPoolOfThatWorkingSetIsAccepted)
{
	auto otherClient = test_helpers::force_claim_partnered_control_function(0x82, 0);
	load_vt3_pool_into_server();

	CANMessageFrame loadVersionResponse = {};

	const bool poolLoaded = upload_pool(SELECTABLE_OBJECT_POOL);
	const bool otherPoolLoaded = upload_pool(SELECTABLE_OBJECT_POOL, otherClient);
	receive_from_client(internalECU, otherClient, { 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

	server->onObjectPoolDeleted = [&](std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet) {
		if (0x82 == workingSet->get_control_function()->get_address())
		{
			receive_from_client(internalECU, client, { 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
			receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
			server->wait_for_object_pool_to_parse();
		}
	};
	testPlugin.clear_queue();
	const bool foundLoadVersionResponse = poll_for_response(*server, testPlugin, 0xD1, loadVersionResponse);
	server->run_update();

	ASSERT_TRUE(poolLoaded);
	ASSERT_TRUE(otherPoolLoaded);
	ASSERT_TRUE(foundLoadVersionResponse);
	EXPECT_EQ(0x00, loadVersionResponse.data[5]);
	ASSERT_EQ(2U, server->deletedObjectPools.size());
	EXPECT_EQ(server->managedWorkingSetList.front(), server->deletedObjectPools.back());
	EXPECT_EQ(nullptr, server->activeWorkingSetsWhenDeleted.back());
	EXPECT_EQ(server->managedWorkingSetList.front(), server->get_active_working_set());
}

TEST_F(VirtualTerminalServerBusTest, SuspendWorkingSetFollowsTheDeletionOfThePoolItReports)
{
	auto otherClient = test_helpers::force_claim_partnered_control_function(0x82, 0);
	std::vector<std::string> order;

	const bool otherPoolLoaded = upload_pool(SELECTABLE_OBJECT_POOL, otherClient);
	receive_from_client(internalECU, otherClient, { 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	server->onObjectPoolDeleted = [&](std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet) {
		order.push_back(describe_hook_call("deleted", workingSet));
		if (0x82 == workingSet->get_control_function()->get_address())
		{
			drop_object_pool_transfer();
			receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		}
	};
	server->onSuspendWorkingSet = [&](std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet) {
		order.push_back(describe_hook_call("suspended", workingSet));
	};

	server->run_update();
	const std::vector<std::string> afterFirstUpdate = order;
	server->run_update();

	ASSERT_TRUE(otherPoolLoaded);
	EXPECT_EQ((std::vector<std::string>{ "deleted:82" }), afterFirstUpdate);
	EXPECT_EQ((std::vector<std::string>{ "deleted:82", "deleted:81", "suspended:81" }), order);
}

TEST_F(VirtualTerminalServerBusTest, SuspendWorkingSetIsNotRaisedForTheDeletionOfAnEarlierPoolOfTheSameWorkingSet)
{
	ASSERT_TRUE(upload_pool(SELECTABLE_OBJECT_POOL));
	const auto workingSet = server->managedWorkingSetList.front();
	server->onObjectPoolDeleted = [&](std::shared_ptr<VirtualTerminalServerManagedWorkingSet> deleted) {
		EXPECT_EQ(workingSet, deleted);
		if (1U == server->deletedObjectPools.size())
		{
			server->set_max_rx_queue_size(1);
			receive_from_client_without_processing(internalECU, client, { 0xC0, 0xFF, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF });
			receive_tp_from_client_without_processing(internalECU, client, SELECTABLE_OBJECT_POOL);
			server->process_rx_messages();
			server->set_max_rx_queue_size(DerivedTestVTServer::DEFAULT_MAX_RX_QUEUE_SIZE_BYTES);
			receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		}
	};
	receive_from_client_without_processing(internalECU, client, { 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

	server->run_update();
	ASSERT_EQ(1U, server->deletedObjectPools.size());
	EXPECT_TRUE(server->suspensions.empty());

	server->run_update();
	ASSERT_EQ(2U, server->deletedObjectPools.size());
	ASSERT_EQ(1U, server->suspensions.size());
	EXPECT_EQ(workingSet, server->suspensions.front().workingSet);
	EXPECT_EQ(0x02, server->suspensions.front().errorCodes);
	EXPECT_EQ(0x08, server->suspensions.front().objectPoolErrorCodes);
}

TEST_F(VirtualTerminalServerBusTest, DeleteObjectPoolIsRejectedUntilThePoolIsAnswered)
{
	CANMessageFrame deleteResponse = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_tp_from_client(internalECU, client, { 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	server->wait_for_object_pool_to_parse();
	testPlugin.clear_queue();
	const bool foundDeleteResponse = send_and_poll({ 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, deleteResponse);

	ASSERT_TRUE(foundDeleteResponse);
	EXPECT_EQ(0x10, deleteResponse.data[1]); // Any other error
	EXPECT_EQ(0U, server->objectPoolDeletionRequests);
	ASSERT_EQ(1U, server->parseResults.size());
	EXPECT_NE(nullptr, server->managedWorkingSetList.front()->get_working_set_object());
}

TEST_F(VirtualTerminalServerBusTest, ChangeActiveMaskRejectsObjectsOfTheWrongType)
{
	CANMessageFrame workingSetIsObjectPointerResponse = {};
	CANMessageFrame maskIsObjectPointerResponse = {};
	CANMessageFrame maskIsDataMaskResponse = {};
	CANMessageFrame unknownMaskResponse = {};

	// Working Set 0 and Data Mask 1000, each with Object Pointer 1 as its child
	const bool poolAnswered = upload_pool({ 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF, 0xE8, 0x03, 0x01, 0x00, 0xFF, 0xFF, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 });

	const bool workingSetIsObjectPointerAnswered = send_and_poll({ 0xAD, 0x01, 0x00, 0xE8, 0x03, 0xFF, 0xFF, 0xFF }, workingSetIsObjectPointerResponse);
	const bool maskIsObjectPointerAnswered = send_and_poll({ 0xAD, 0x00, 0x00, 0x01, 0x00, 0xFF, 0xFF, 0xFF }, maskIsObjectPointerResponse);
	const bool maskIsDataMaskAnswered = send_and_poll({ 0xAD, 0x00, 0x00, 0xE8, 0x03, 0xFF, 0xFF, 0xFF }, maskIsDataMaskResponse);
	const auto objectCountBeforeUnknownMask = server->managedWorkingSetList.front()->get_object_tree().size();
	const bool unknownMaskAnswered = send_and_poll({ 0xAD, 0x01, 0x00, 0xD0, 0x07, 0xFF, 0xFF, 0xFF }, unknownMaskResponse);

	ASSERT_TRUE(poolAnswered);
	ASSERT_TRUE(workingSetIsObjectPointerAnswered);
	EXPECT_EQ(0x01, workingSetIsObjectPointerResponse.data[3]);
	ASSERT_TRUE(maskIsObjectPointerAnswered);
	EXPECT_EQ(0x02, maskIsObjectPointerResponse.data[3]);
	ASSERT_TRUE(maskIsDataMaskAnswered);
	EXPECT_EQ(0x00, maskIsDataMaskResponse.data[3]);
	ASSERT_TRUE(unknownMaskAnswered);
	EXPECT_EQ(0x01, unknownMaskResponse.data[3]);
	EXPECT_EQ(objectCountBeforeUnknownMask, server->managedWorkingSetList.front()->get_object_tree().size());
}

TEST_F(VirtualTerminalServerBusTest, ChangePolygonScaleRescalesThePointsAndRepaints)
{
	std::uint32_t repaints = 0;
	server->get_on_repaint_event_dispatcher().add_listener([&repaints](std::shared_ptr<VirtualTerminalServerManagedWorkingSet>) { repaints++; });

	CANMessageFrame response = {};

	const bool connected = upload_pool(POLYGON_AND_LINE_OBJECT_POOL);
	const std::uint32_t repaintsBeforeScale = repaints;
	const bool answered = send_and_poll({ 0xB7, 0x02, 0x00, 0x96, 0x00, 0x19, 0x00, 0xFF }, response);

	ASSERT_TRUE(connected);
	ASSERT_TRUE(answered);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0xB7, 0x02, 0x00, 0x96, 0x00, 0x19, 0x00, 0x00 }), payload_of(response));
	EXPECT_EQ(repaintsBeforeScale + 1, repaints);

	auto polygon = std::static_pointer_cast<OutputPolygon>(server->managedWorkingSetList.front()->get_object_by_id(2));
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

TEST_F(VirtualTerminalServerBusTest, ChangePolygonScaleRejectsUnknownAndWrongObjectsAndAZeroOldWidth)
{
	CANMessageFrame unknownObjectResponse = {};
	CANMessageFrame lineResponse = {};
	CANMessageFrame zeroWidthResponse = {};
	CANMessageFrame zeroOldWidthResponse = {};

	const bool connected = upload_pool(POLYGON_AND_LINE_OBJECT_POOL);
	const bool unknownObjectAnswered = send_and_poll({ 0xB7, 0xD0, 0x07, 0x96, 0x00, 0x19, 0x00, 0xFF }, unknownObjectResponse);
	const bool lineAnswered = send_and_poll({ 0xB7, 0x03, 0x00, 0x96, 0x00, 0x19, 0x00, 0xFF }, lineResponse);
	const bool zeroWidthAnswered = send_and_poll({ 0xB7, 0x02, 0x00, 0x00, 0x00, 0x19, 0x00, 0xFF }, zeroWidthResponse);
	const bool zeroOldWidthAnswered = send_and_poll({ 0xB7, 0x02, 0x00, 0x96, 0x00, 0x19, 0x00, 0xFF }, zeroOldWidthResponse);

	ASSERT_TRUE(connected);
	ASSERT_TRUE(unknownObjectAnswered);
	EXPECT_EQ(0x01, unknownObjectResponse.data[7]);
	ASSERT_TRUE(lineAnswered);
	EXPECT_EQ(0x01, lineResponse.data[7]);
	EXPECT_EQ(10, server->managedWorkingSetList.front()->get_object_by_id(3)->get_width());
	ASSERT_TRUE(zeroWidthAnswered);
	EXPECT_EQ(0x00, zeroWidthResponse.data[7]);
	ASSERT_TRUE(zeroOldWidthAnswered);
	EXPECT_EQ(0x10, zeroOldWidthResponse.data[7]);
	EXPECT_EQ(0, server->managedWorkingSetList.front()->get_object_by_id(2)->get_width());
}

TEST_F(VirtualTerminalServerBusTest, ChangePolygonScaleRejectsAPointThatWouldNotFitIn16Bits)
{
	std::uint32_t repaints = 0;
	server->get_on_repaint_event_dispatcher().add_listener([&repaints](std::shared_ptr<VirtualTerminalServerManagedWorkingSet>) { repaints++; });
	CANMessageFrame pointResponse = {};
	CANMessageFrame scaleResponse = {};

	const bool connected = upload_pool(POLYGON_AND_LINE_OBJECT_POOL);
	const bool pointAnswered = send_and_poll({ 0xB6, 0x02, 0x00, 0x02, 0xC8, 0x00, 0x11, 0x00 }, pointResponse);
	const std::uint32_t repaintsBeforeScale = repaints;
	// Point 2 is now at x = 200, so (200 * 65535 + 50) / 100 needs 17 bits, while point 1 before it would fit
	const bool scaleAnswered = send_and_poll({ 0xB7, 0x02, 0x00, 0xFF, 0xFF, 0x19, 0x00, 0xFF }, scaleResponse);

	ASSERT_TRUE(connected);
	ASSERT_TRUE(pointAnswered);
	EXPECT_EQ(0x00, pointResponse.data[3]);
	ASSERT_TRUE(scaleAnswered);
	EXPECT_EQ(0x10, scaleResponse.data[7]);
	EXPECT_EQ(repaintsBeforeScale, repaints);

	auto polygon = std::static_pointer_cast<OutputPolygon>(server->managedWorkingSetList.front()->get_object_by_id(2));
	EXPECT_EQ(100, polygon->get_width());
	EXPECT_EQ(50, polygon->get_height());
	EXPECT_EQ(100, polygon->get_point(1).xValue);
	EXPECT_EQ(200, polygon->get_point(2).xValue);
	EXPECT_EQ(17, polygon->get_point(2).yValue);
}

TEST_F(VirtualTerminalServerBusTest, ChangePolygonPointMovesThePointAndRepaints)
{
	std::uint32_t repaints = 0;
	server->get_on_repaint_event_dispatcher().add_listener([&repaints](std::shared_ptr<VirtualTerminalServerManagedWorkingSet>) { repaints++; });

	CANMessageFrame response = {};

	const bool connected = upload_pool(POLYGON_AND_LINE_OBJECT_POOL);
	const std::uint32_t repaintsBeforeChange = repaints;
	const bool answered = send_and_poll({ 0xB6, 0x02, 0x00, 0x02, 0x28, 0x00, 0x1E, 0x00 }, response);

	ASSERT_TRUE(connected);
	ASSERT_TRUE(answered);
	EXPECT_EQ(0x00, response.data[3]);
	EXPECT_EQ(repaintsBeforeChange + 1, repaints);

	auto polygon = std::static_pointer_cast<OutputPolygon>(server->managedWorkingSetList.front()->get_object_by_id(2));
	EXPECT_EQ(40, polygon->get_point(2).xValue);
	EXPECT_EQ(30, polygon->get_point(2).yValue);
}

TEST_F(VirtualTerminalServerBusTest, ChangePolygonScaleInAShortFrameIsIgnored)
{
	CANMessageFrame response = {};

	const bool connected = upload_pool(POLYGON_AND_LINE_OBJECT_POOL);
	const bool answered = send_and_poll({ 0xB7, 0x02, 0x00, 0x96, 0x00, 0x19, 0x00 }, response);

	ASSERT_TRUE(connected);
	EXPECT_FALSE(answered);
	EXPECT_EQ(100, server->managedWorkingSetList.front()->get_object_by_id(2)->get_width());
}

TEST_F(VirtualTerminalServerBusTest, ChangeEndPointChangesTheLineRepaintsAndRunsItsMacro)
{
	std::uint32_t repaints = 0;
	server->get_on_repaint_event_dispatcher().add_listener([&repaints](std::shared_ptr<VirtualTerminalServerManagedWorkingSet>) { repaints++; });

	CANMessageFrame response = {};

	const bool connected = upload_pool(POLYGON_AND_LINE_OBJECT_POOL);
	const std::uint32_t repaintsBeforeChange = repaints;
	const bool answered = send_and_poll({ 0xA9, 0x03, 0x00, 0x28, 0x00, 0x1E, 0x00, 0x01 }, response);

	ASSERT_TRUE(connected);
	ASSERT_TRUE(answered);
	EXPECT_EQ(0x03, response.data[1]);
	EXPECT_EQ(0x00, response.data[2]);
	EXPECT_EQ(0x00, response.data[3]);
	EXPECT_EQ(0xFF, response.data[4]);
	EXPECT_EQ(0xFF, response.data[7]);
	// One for the end point, one for the Change Size the macro runs
	EXPECT_EQ(repaintsBeforeChange + 2, repaints);

	auto line = std::static_pointer_cast<OutputLine>(server->managedWorkingSetList.front()->get_object_by_id(3));
	EXPECT_EQ(40, line->get_width());
	EXPECT_EQ(30, line->get_height());
	EXPECT_EQ(OutputLine::LineDirection::BottomLeftToTopRight, line->get_line_direction());
	EXPECT_EQ(7, server->managedWorkingSetList.front()->get_object_by_id(2)->get_width());
	EXPECT_EQ(8, server->managedWorkingSetList.front()->get_object_by_id(2)->get_height());
}

TEST_F(VirtualTerminalServerBusTest, CommandsRunByAMacroSendNoResponse)
{
	CANMessageFrame response = {};

	const bool connected = upload_pool(POLYGON_AND_LINE_OBJECT_POOL);
	// Changing Line 3's end point runs Macro 5, whose Change Size of Polygon 2 must not be answered
	const bool answered = send_and_poll({ 0xA9, 0x03, 0x00, 0x28, 0x00, 0x1E, 0x00, 0x01 }, response, 0xA6);

	ASSERT_TRUE(connected);
	EXPECT_FALSE(answered);
	EXPECT_EQ(7, server->managedWorkingSetList.front()->get_object_by_id(2)->get_width());
}

TEST_F(VirtualTerminalServerBusTest, ExecuteMacroCommandIsStillAnswered)
{
	CANMessageFrame response = {};

	const bool connected = upload_pool(POLYGON_AND_LINE_OBJECT_POOL);
	// The working set's own Execute Macro is answered once Macro 5 has finished
	const bool answered = send_and_poll({ 0xBE, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, response);

	ASSERT_TRUE(connected);
	ASSERT_TRUE(answered);
	EXPECT_EQ(0x05, response.data[1]);
	EXPECT_EQ(0x00, response.data[3]);
	EXPECT_EQ(7, server->managedWorkingSetList.front()->get_object_by_id(2)->get_width());
}

TEST_F(VirtualTerminalServerBusTest, ThrowingMacroCommandDoesNotSilenceLaterResponses)
{
	CANMessageFrame response = {};

	ASSERT_TRUE(upload_pool(POLYGON_AND_LINE_OBJECT_POOL));

	// Macro 5's Change Size repaints, so this throws out of execute_macro
	const auto listener = server->get_on_repaint_event_dispatcher().add_listener([](std::shared_ptr<VirtualTerminalServerManagedWorkingSet>) { throw std::runtime_error("repaint failed"); });
	EXPECT_THROW(server->execute_macro_for_test(5, server->managedWorkingSetList.front()), std::runtime_error);
	server->get_on_repaint_event_dispatcher().remove_listener(listener);

	const bool answered = send_and_poll({ 0xA6, 0x02, 0x00, 0x09, 0x00, 0x0A, 0x00, 0xFF }, response);

	EXPECT_TRUE(answered);
}

TEST_F(VirtualTerminalServerBusTest, ChangeEndPointRejectsUnknownAndWrongObjectsAndAnInvalidLineDirection)
{
	CANMessageFrame unknownObjectResponse = {};
	CANMessageFrame polygonResponse = {};
	CANMessageFrame invalidDirectionResponse = {};

	const bool connected = upload_pool(POLYGON_AND_LINE_OBJECT_POOL);
	const bool unknownObjectAnswered = send_and_poll({ 0xA9, 0xD0, 0x07, 0x28, 0x00, 0x1E, 0x00, 0x01 }, unknownObjectResponse);
	const bool polygonAnswered = send_and_poll({ 0xA9, 0x02, 0x00, 0x28, 0x00, 0x1E, 0x00, 0x01 }, polygonResponse);
	const bool invalidDirectionAnswered = send_and_poll({ 0xA9, 0x03, 0x00, 0x28, 0x00, 0x1E, 0x00, 0x02 }, invalidDirectionResponse);

	ASSERT_TRUE(connected);
	ASSERT_TRUE(unknownObjectAnswered);
	EXPECT_EQ(0x01, unknownObjectResponse.data[3]);
	ASSERT_TRUE(polygonAnswered);
	EXPECT_EQ(0x01, polygonResponse.data[3]);
	EXPECT_EQ(100, server->managedWorkingSetList.front()->get_object_by_id(2)->get_width());
	ASSERT_TRUE(invalidDirectionAnswered);
	EXPECT_EQ(0x02, invalidDirectionResponse.data[3]);
	EXPECT_EQ(10, server->managedWorkingSetList.front()->get_object_by_id(3)->get_width());
}

TEST_F(VirtualTerminalServerBusTest, ChangeEndPointInAShortFrameIsIgnored)
{
	CANMessageFrame response = {};

	const bool connected = upload_pool(POLYGON_AND_LINE_OBJECT_POOL);
	const bool answered = send_and_poll({ 0xA9, 0x03, 0x00, 0x28, 0x00, 0x1E, 0x00 }, response);

	ASSERT_TRUE(connected);
	EXPECT_FALSE(answered);
	EXPECT_EQ(10, server->managedWorkingSetList.front()->get_object_by_id(3)->get_width());
}

TEST_F(VirtualTerminalServerBusTest, CommandsToObjectsOfTheWrongTypeReportTheErrorBitOfThatCommand)
{
	server->version = VirtualTerminalBase::VTVersion::Version4;

	const auto errorByteOf = [&](std::uint8_t function, std::size_t errorByte) {
		CANMessageFrame response = {};
		return poll_for_response(*server, testPlugin, function, response) ? static_cast<int>(response.data[errorByte]) : -1;
	};
	const auto errorCodeOf = [&](std::initializer_list<std::uint8_t> command, std::size_t errorByte) {
		receive_from_client(internalECU, client, command);
		return errorByteOf(*command.begin(), errorByte);
	};

	// Working Set 0 holds Object Pointer 1; Data Mask 1000 holds disabled Input Boolean 4 and hidden Container 2 (Input Boolean 3); Input Boolean 5 is on no mask
	const bool connected = upload_pool({ 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1B, 0xFF, 0xFF, 0xE8, 0x03, 0x01, 0x00, 0xFF, 0xFF, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x03, 0x0A, 0x00, 0x0A, 0x00, 0x01, 0x01, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x07, 0x00, 0x0A, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x04, 0x00, 0x07, 0x00, 0x0A, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x05, 0x00, 0x07, 0x00, 0x0A, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0x00 });

	const int changeSize = errorCodeOf({ 0xA6, 0x01, 0x00, 0x0A, 0x00, 0x0A, 0x00, 0xFF }, 3);
	const int changeBackgroundColour = errorCodeOf({ 0xA7, 0x01, 0x00, 0x05, 0xFF, 0xFF, 0xFF, 0xFF }, 4);
	const int changeSoftKeyMask = errorCodeOf({ 0xAE, 0x01, 0x01, 0x00, 0xFF, 0xFF, 0xFF, 0xFF }, 5);
	const int changePriority = errorCodeOf({ 0xB0, 0x01, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF }, 4);
	const int changeListItem = errorCodeOf({ 0xB1, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF }, 6);
	const int changePolygonPoint = errorCodeOf({ 0xB6, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 3);

	const int childLocationOfANonParent = errorCodeOf({ 0xA5, 0x04, 0x00, 0x01, 0x00, 0x80, 0x80, 0xFF }, 5);
	const int childLocationOfANonChild = errorCodeOf({ 0xA5, 0x00, 0x00, 0x04, 0x00, 0x80, 0x80, 0xFF }, 5);
	const int childLocationOfAMissingChild = errorCodeOf({ 0xA5, 0x00, 0x00, 0xD0, 0x07, 0x80, 0x80, 0xFF }, 5);
	const int childLocationOfAChild = errorCodeOf({ 0xA5, 0x00, 0x00, 0x01, 0x00, 0x80, 0x80, 0xFF }, 5);
	receive_tp_from_client(internalECU, client, { 0xB4, 0x04, 0x00, 0x01, 0x00, 0x05, 0x00, 0x05, 0x00 });
	const int childPositionOfANonParent = errorByteOf(0xB4, 5);
	receive_tp_from_client(internalECU, client, { 0xB4, 0x00, 0x00, 0x04, 0x00, 0x05, 0x00, 0x05, 0x00 });
	const int childPositionOfANonChild = errorByteOf(0xB4, 5);

	const int selectNonInput = errorCodeOf({ 0xA2, 0x01, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, 4);
	const int selectDisabled = errorCodeOf({ 0xA2, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, 4);
	const int selectNotOnActiveMask = errorCodeOf({ 0xA2, 0x05, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, 4);
	const int selectInHiddenContainer = errorCodeOf({ 0xA2, 0x03, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, 4);
	const int enable = errorCodeOf({ 0xA1, 0x04, 0x00, 0x01, 0xFF, 0xFF, 0xFF, 0xFF }, 4);
	const int selectWithInvalidOption = errorCodeOf({ 0xA2, 0x04, 0x00, 0x05, 0xFF, 0xFF, 0xFF, 0xFF }, 4);
	CANMessageFrame selectEnabled = {};
	const bool selectEnabledAnswered = send_and_poll({ 0xA2, 0x04, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, selectEnabled);
	server->version = VirtualTerminalBase::VTVersion::Version3;
	CANMessageFrame openOnVersion3 = {};
	const bool openOnVersion3Answered = send_and_poll({ 0xA2, 0x04, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF }, openOnVersion3);

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

TEST_F(VirtualTerminalServerBusTest, GetSupportedObjectsResponseIsASortedListOfVariableLength)
{
	CANMessageFrame emptyResponse = {};
	CANMessageFrame shortResponse = {};
	CANMessageFrame sixTypesResponse = {};
	std::vector<std::uint8_t> sevenTypesResponse;
	std::vector<std::uint8_t> longResponse;

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	server->supportedObjects = {};
	const bool emptyAnswered = send_and_poll({ 0xC5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, emptyResponse);

	server->supportedObjects = { 3, 1 };
	const bool shortAnswered = send_and_poll({ 0xC5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, shortResponse);

	server->supportedObjects = { 6, 5, 4, 3, 2, 1 };
	const bool sixTypesAnswered = send_and_poll({ 0xC5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, sixTypesResponse);

	server->supportedObjects = { 7, 6, 5, 4, 3, 2, 1 };
	receive_from_client(internalECU, client, { 0xC5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool sevenTypesAnswered = read_tp_from_server(testPlugin, internalECU, client, sevenTypesResponse);

	server->supportedObjects = { 39, 37, 7, 6, 5, 4, 3, 2, 1, 0 };
	receive_from_client(internalECU, client, { 0xC5, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool longAnswered = read_tp_from_server(testPlugin, internalECU, client, longResponse);

	ASSERT_TRUE(emptyAnswered);
	EXPECT_EQ(8, emptyResponse.dataLength);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0xC5, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }), payload_of(emptyResponse));
	ASSERT_TRUE(shortAnswered);
	EXPECT_EQ(8, shortResponse.dataLength);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0xC5, 2, 1, 3, 0xFF, 0xFF, 0xFF, 0xFF }), payload_of(shortResponse));
	ASSERT_TRUE(sixTypesAnswered);
	EXPECT_EQ(8, sixTypesResponse.dataLength);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0xC5, 6, 1, 2, 3, 4, 5, 6 }), payload_of(sixTypesResponse));
	ASSERT_TRUE(sevenTypesAnswered);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0xC5, 7, 1, 2, 3, 4, 5, 6, 7 }), sevenTypesResponse);
	ASSERT_TRUE(longAnswered);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0xC5, 10, 0, 1, 2, 3, 4, 5, 6, 7, 37, 39 }), longResponse);
}

TEST_F(VirtualTerminalServerBusTest, LoadVersionResponseIsSentWhenARestoredObjectPoolParses)
{
	load_vt3_pool_into_server();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundLoadVersionResponse = poll_for_response(*server, testPlugin, 0xD1, responseFrame);
	const std::uint8_t loadVersionErrorCodes = responseFrame.data[5];

	// Objects transferred on top of the restored pool get the normal End of Object Pool response
	receive_from_client(internalECU, client, { 0x11, 0x00, 0xF0, 0x15, 0x00, 0x00, 0x00, 0x00 });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	const bool foundEndOfObjectPoolResponse = poll_for_response(*server, testPlugin, 0x12, responseFrame);

	ASSERT_TRUE(foundLoadVersionResponse);
	EXPECT_EQ(0x00, loadVersionErrorCodes);
	ASSERT_TRUE(foundEndOfObjectPoolResponse);
	EXPECT_EQ(0x00, responseFrame.data[1]); // No error in object pool
}

TEST_F(VirtualTerminalServerBusTest, LoadVersionResponseReportsPoolDataCorruptionWhenARestoredObjectPoolFailsToParse)
{
	server->version = VirtualTerminalBase::VTVersion::Version4;
	server->versionToLoad = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(*server, testPlugin, 0xD1, responseFrame);

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x01, responseFrame.data[5]); // File system error or pool data corruption
}

TEST_F(VirtualTerminalServerBusTest, LoadVersionResponseReportsAnyOtherErrorWhenARestoredObjectPoolFailsToParseOnVersion3)
{
	server->versionToLoad = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(*server, testPlugin, 0xD1, responseFrame);

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x08, responseFrame.data[5]); // Any other error
}

TEST_F(VirtualTerminalServerBusTest, LoadVersionResponseReportsAnUnknownLabelWhenNoPoolIsStored)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(*server, testPlugin, 0xD1, responseFrame);

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x02, responseFrame.data[5]); // Version label is not correct or unknown
}

TEST_F(VirtualTerminalServerBusTest, LoadVersionResponseIsSentWhenObjectsArriveBeforeItIsAnswered)
{
	load_vt3_pool_into_server();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	server->wait_for_object_pool_to_parse();
	receive_from_client(internalECU, client, { 0x11, 0x00, 0xF0, 0x15, 0x00, 0x00, 0x00, 0x00 });
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundLoadVersionResponse = poll_for_response(*server, testPlugin, 0xD1, responseFrame);
	const std::uint8_t loadVersionErrorCodes = responseFrame.data[5];

	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	const bool foundEndOfObjectPoolResponse = poll_for_response(*server, testPlugin, 0x12, responseFrame);

	ASSERT_TRUE(foundLoadVersionResponse);
	EXPECT_EQ(0x00, loadVersionErrorCodes);
	ASSERT_TRUE(foundEndOfObjectPoolResponse);
	EXPECT_EQ(0x00, responseFrame.data[1]); // No error in object pool
}

TEST_F(VirtualTerminalServerBusTest, LoadVersionWithAnUnknownLabelDoesNotAnswerForAnExistingObjectPool)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x11, 0x00, 0xF0, 0x15, 0x00, 0x00, 0x00, 0x00 });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

	CANMessageFrame responseFrame = {};
	const bool foundEndOfObjectPoolResponse = poll_for_response(*server, testPlugin, 0x12, responseFrame);

	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });

	const bool foundLoadVersionResponse = poll_for_response(*server, testPlugin, 0xD1, responseFrame);
	const std::uint8_t loadVersionErrorCodes = responseFrame.data[5];
	const bool foundSecondEndOfObjectPoolResponse = poll_for_response(*server, testPlugin, 0x12, responseFrame);

	ASSERT_TRUE(foundEndOfObjectPoolResponse);
	ASSERT_TRUE(foundLoadVersionResponse);
	EXPECT_EQ(0x02, loadVersionErrorCodes); // Version label is not correct or unknown
	EXPECT_FALSE(foundSecondEndOfObjectPoolResponse);
}

TEST_F(VirtualTerminalServerBusTest, LoadVersionOverwritesTheExistingObjectPool)
{
	load_vt3_pool_into_server();

	const bool poolUploaded = upload_pool({ 0x11, 0x01, 0xF0, 0x01, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x01, 0x01, 0xF0, 0x00, 0x00, 0x03, 'e', 'n', 'd', 'e', 'f', 'r' });

	CANMessageFrame responseFrame = {};

	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });

	const bool foundLoadVersionResponse = poll_for_response(*server, testPlugin, 0xD1, responseFrame);
	const std::uint8_t loadVersionErrorCodes = responseFrame.data[5];

	CANMessageFrame statusFrame = {};
	const bool foundStatus = read_status_message(*server, testPlugin, statusFrame);

	auto workingSet = server->managedWorkingSetList.front();
	ASSERT_TRUE(poolUploaded);
	ASSERT_TRUE(foundLoadVersionResponse);
	EXPECT_EQ(0x00, loadVersionErrorCodes);
	ASSERT_EQ(1U, workingSet->get_number_iop_files());
	EXPECT_EQ(server->versionToLoad, workingSet->get_iop_raw_data(0));
	EXPECT_EQ(0U, workingSet->get_object_tree().count(0xF000));

	const std::uint16_t loadedActiveMask = std::static_pointer_cast<WorkingSet>(workingSet->get_working_set_object())->get_active_mask();
	ASSERT_NE(0xF001, loadedActiveMask);
	ASSERT_TRUE(foundStatus);
	EXPECT_EQ(0x81, statusFrame.data[1]);
	EXPECT_EQ(loadedActiveMask, static_cast<std::uint16_t>(statusFrame.data[2] | (statusFrame.data[3] << 8)));
}

TEST_F(VirtualTerminalServerBusTest, StatusMessageReportsParsingUntilTheLoadVersionResponseIsSent)
{
	load_vt3_pool_into_server();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	server->wait_for_object_pool_to_parse();

	std::uint8_t busyCodesBeforeResponse = 0;
	const bool foundStatusBeforeResponse = read_status_busy_codes(*server, testPlugin, busyCodesBeforeResponse);

	CANMessageFrame responseFrame = {};
	const bool foundLoadVersionResponse = poll_for_response(*server, testPlugin, 0xD1, responseFrame);

	std::uint8_t busyCodesAfterResponse = 0xFF;
	const bool foundStatusAfterResponse = read_status_busy_codes(*server, testPlugin, busyCodesAfterResponse);

	ASSERT_TRUE(foundStatusBeforeResponse);
	EXPECT_EQ(0x10, busyCodesBeforeResponse); // VT is busy parsing an object pool
	ASSERT_TRUE(foundLoadVersionResponse);
	ASSERT_TRUE(foundStatusAfterResponse);
	EXPECT_EQ(0x00, busyCodesAfterResponse);
}

TEST_F(VirtualTerminalServerBusTest, StatusMessageReportsParsingUntilTheEndOfObjectPoolResponseIsSent)
{
	server->busyCodesBitfield = 0x40; // VT is in auxiliary learn mode

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x11, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	server->wait_for_object_pool_to_parse();

	std::uint8_t busyCodesBeforeResponse = 0;
	const bool foundStatusBeforeResponse = read_status_busy_codes(*server, testPlugin, busyCodesBeforeResponse);

	CANMessageFrame responseFrame = {};
	const bool foundEndOfObjectPoolResponse = poll_for_response(*server, testPlugin, 0x12, responseFrame);

	std::uint8_t busyCodesAfterResponse = 0xFF;
	const bool foundStatusAfterResponse = read_status_busy_codes(*server, testPlugin, busyCodesAfterResponse);

	ASSERT_TRUE(foundStatusBeforeResponse);
	EXPECT_EQ(0x50, busyCodesBeforeResponse); // Parsing on top of auxiliary learn mode
	ASSERT_TRUE(foundEndOfObjectPoolResponse);
	ASSERT_TRUE(foundStatusAfterResponse);
	EXPECT_EQ(0x40, busyCodesAfterResponse);
}

TEST_F(VirtualTerminalServerBusTest, StatusMessageDoesNotReportParsingOnVersion2OrOlder)
{
	server->version = VirtualTerminalBase::VTVersion::Version2OrOlder;

	receive_working_set_master_from_client(client);
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x11, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA });
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	server->wait_for_object_pool_to_parse();

	std::uint8_t busyCodes = 0xFF;
	const bool foundStatus = read_status_busy_codes(*server, testPlugin, busyCodes);
	server->run_update(); // Joins the parsing thread, which must not outlive the working set

	ASSERT_TRUE(foundStatus);
	EXPECT_EQ(0x00, busyCodes);
}

TEST_F(VirtualTerminalServerBusTest, ObjectPoolCommandsAreIgnoredUntilThePreviousObjectPoolIsAnswered)
{
	load_vt3_pool_into_server();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	server->wait_for_object_pool_to_parse();
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	receive_from_client(internalECU, client, { 0x11, 0x00, 0xF0, 0x15, 0x00, 0x00, 0x00, 0x00 });
	const auto numberOfPoolSegments = server->managedWorkingSetList.front()->get_number_iop_files();
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundLoadVersionResponse = poll_for_response(*server, testPlugin, 0xD1, responseFrame);

	EXPECT_EQ(1U, numberOfPoolSegments);
	ASSERT_TRUE(foundLoadVersionResponse);
	EXPECT_EQ(0x00, responseFrame.data[5]);
}

TEST_F(VirtualTerminalServerBusTest, StoreVersionResponseReportsAnyOtherErrorWhenTheObjectPoolCannotBeStored)
{
	server->saveSucceeds = false;

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x11, 0x00, 0xF0, 0x15, 0x00, 0x00, 0x00, 0x00 });
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD0, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });

	CANMessageFrame responseFrame = {};
	const bool foundStoreVersionResponse = poll_for_response(*server, testPlugin, 0xD0, responseFrame);

	ASSERT_TRUE(foundStoreVersionResponse);
	EXPECT_EQ(0x08, responseFrame.data[5]);
}

TEST_F(VirtualTerminalServerBusTest, StoreVersionReplacesTheVersionStoredUnderTheSameLabel)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 });
	receive_from_client(internalECU, client, { 0x11, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E });
	testPlugin.clear_queue();

	CANMessageFrame firstResponse = {};
	CANMessageFrame secondResponse = {};
	const bool foundFirstResponse = send_and_poll({ 0xD0, 'V', 'E', 'R', 'S', 'I', 'O', 'N' }, firstResponse);
	const bool foundSecondResponse = send_and_poll({ 0xD0, 'V', 'E', 'R', 'S', 'I', 'O', 'N' }, secondResponse);

	ASSERT_TRUE(foundFirstResponse);
	EXPECT_EQ(0x00, firstResponse.data[5]);
	ASSERT_TRUE(foundSecondResponse);
	EXPECT_EQ(0x00, secondResponse.data[5]);
	ASSERT_EQ(2U, server->deletedVersionLabels.size());
	EXPECT_EQ((std::vector<std::uint8_t>{ 'V', 'E', 'R', 'S', 'I', 'O', 'N' }), server->deletedVersionLabels.back());
	ASSERT_EQ(2U, server->storedVersion.size());
	EXPECT_EQ((std::vector<std::uint8_t>{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 }), server->storedVersion.at(0));
	EXPECT_EQ((std::vector<std::uint8_t>{ 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E }), server->storedVersion.at(1));
}

TEST_F(VirtualTerminalServerBusTest, StoreVersionSucceedsWhenNothingIsStoredUnderTheLabelYet)
{
	server->deleteSucceeds = false;

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x11, 0x00, 0xF0, 0x15, 0x00, 0x00, 0x00, 0x00 });
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD0, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });

	CANMessageFrame responseFrame = {};
	const bool foundStoreVersionResponse = poll_for_response(*server, testPlugin, 0xD0, responseFrame);

	ASSERT_TRUE(foundStoreVersionResponse);
	EXPECT_EQ(0x00, responseFrame.data[5]);
	EXPECT_EQ(1U, server->deletedVersionLabels.size());
	EXPECT_EQ(1U, server->storedVersion.size());
}

TEST_F(VirtualTerminalServerBusTest, ExtendedGetVersionsResponseWithNoVersionsIsPaddedToOneFrame)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(*server, testPlugin, 0xD3, responseFrame);

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(8U, responseFrame.dataLength);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0xD3, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }), payload_of(responseFrame));
}

TEST_F(VirtualTerminalServerBusTest, ExtendedGetVersionsResponseCarriesThe32ByteLabels)
{
	std::array<std::uint8_t, 32> label;
	label.fill(' ');
	std::copy_n("EXTENDED VERSION", 16, label.begin());
	server->extendedVersions = { label };

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

	std::vector<std::uint8_t> response;
	const bool foundResponse = read_tp_from_server(testPlugin, internalECU, client, response);

	ASSERT_TRUE(foundResponse);
	ASSERT_EQ(34U, response.size());
	EXPECT_EQ(0xD3, response[0]);
	EXPECT_EQ(1, response[1]);
	EXPECT_TRUE(std::equal(label.begin(), label.end(), response.begin() + 2));
}

TEST_F(VirtualTerminalServerBusTest, ExtendedLoadVersionResponseIsSentWhenARestoredObjectPoolParses)
{
	load_vt3_pool_into_server();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const auto command = extended_version_command(0xD5, "EXTENDED VERSION");
	receive_tp_from_client(internalECU, client, command);
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(*server, testPlugin, 0xD5, responseFrame);

	EXPECT_EQ(std::vector<std::uint8_t>(command.begin() + 1, command.end()), server->lastVersionLabel);
	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x00, responseFrame.data[5]);
}

TEST_F(VirtualTerminalServerBusTest, ExtendedLoadVersionResponseReportsAnUnknownLabelWhenNoPoolIsStored)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	receive_tp_from_client(internalECU, client, extended_version_command(0xD5, "UNKNOWN"));

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(*server, testPlugin, 0xD5, responseFrame);

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x02, responseFrame.data[5]); // Version label not correct or unknown
}

TEST_F(VirtualTerminalServerBusTest, ExtendedLoadVersionResponseReportsPoolDataCorruptionWhenARestoredObjectPoolFailsToParse)
{
	server->version = VirtualTerminalBase::VTVersion::Version5;
	server->versionToLoad = { 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA };

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_tp_from_client(internalECU, client, extended_version_command(0xD5, "EXTENDED VERSION"));
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(*server, testPlugin, 0xD5, responseFrame);

	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x01, responseFrame.data[5]); // File system error or pool data corruption
}

TEST_F(VirtualTerminalServerBusTest, ExtendedLoadVersionIsIgnoredUntilThePreviousObjectPoolIsAnswered)
{
	load_vt3_pool_into_server();

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_tp_from_client(internalECU, client, extended_version_command(0xD5, "EXTENDED VERSION"));
	server->wait_for_object_pool_to_parse();
	receive_tp_from_client(internalECU, client, extended_version_command(0xD5, "EXTENDED VERSION"));
	const auto numberOfPoolSegments = server->managedWorkingSetList.front()->get_number_iop_files();
	testPlugin.clear_queue();

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(*server, testPlugin, 0xD5, responseFrame);

	EXPECT_EQ(1U, numberOfPoolSegments);
	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x00, responseFrame.data[5]);
}

TEST_F(VirtualTerminalServerBusTest, ExtendedVersionCommandTooShortForItsLabelIsIgnored)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD5, 'S', 'H', 'O', 'R', 'T', ' ', ' ' });

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(*server, testPlugin, 0xD5, responseFrame);

	EXPECT_FALSE(foundResponse);
	EXPECT_TRUE(server->lastVersionLabel.empty());
}

TEST_F(VirtualTerminalServerBusTest, ExtendedStoreVersionStoresThePoolUnderThe32ByteLabel)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x11, 0x00, 0xF0, 0x15, 0x00, 0x00, 0x00, 0x00 });
	testPlugin.clear_queue();
	receive_tp_from_client(internalECU, client, extended_version_command(0xD4, "EXTENDED VERSION"));

	CANMessageFrame responseFrame = {};
	const bool foundResponse = poll_for_response(*server, testPlugin, 0xD4, responseFrame);

	EXPECT_EQ(32U, server->lastVersionLabel.size());
	ASSERT_EQ(1U, server->deletedVersionLabels.size());
	EXPECT_EQ(server->lastVersionLabel, server->deletedVersionLabels.front());
	EXPECT_EQ(1U, server->storedVersion.size());
	ASSERT_TRUE(foundResponse);
	EXPECT_EQ(0x00, responseFrame.data[5]);
}

TEST_F(VirtualTerminalServerBusTest, ExtendedDeleteVersionResponseReportsAnUnknownLabel)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	receive_tp_from_client(internalECU, client, extended_version_command(0xD6, "EXTENDED VERSION"));

	CANMessageFrame deletedFrame = {};
	const bool foundDeletedResponse = poll_for_response(*server, testPlugin, 0xD6, deletedFrame);

	server->deleteSucceeds = false;
	testPlugin.clear_queue();
	receive_tp_from_client(internalECU, client, extended_version_command(0xD6, "UNKNOWN"));

	CANMessageFrame unknownFrame = {};
	const bool foundUnknownResponse = poll_for_response(*server, testPlugin, 0xD6, unknownFrame);

	EXPECT_EQ(32U, server->lastVersionLabel.size());
	ASSERT_TRUE(foundDeletedResponse);
	EXPECT_EQ(0x00, deletedFrame.data[5]);
	ASSERT_TRUE(foundUnknownResponse);
	EXPECT_EQ(0x02, unknownFrame.data[5]); // Version label not correct or unknown
}

TEST_F(VirtualTerminalServerBusTest, DeleteVersionWildcardDeletesAllVersionsOnlyOnVersion6)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	server->version = VirtualTerminalBase::VTVersion::Version5;
	testPlugin.clear_queue();
	receive_tp_from_client(internalECU, client, extended_version_command(0xD6, "*"));

	CANMessageFrame version5Frame = {};
	const bool foundVersion5Response = poll_for_response(*server, testPlugin, 0xD6, version5Frame);
	const auto version5DeletedAllLength = server->deletedAllVersionsLabelLength;
	const auto version5Label = server->lastVersionLabel;

	server->version = VirtualTerminalBase::VTVersion::Version6;
	server->lastVersionLabel.clear();
	testPlugin.clear_queue();
	receive_tp_from_client(internalECU, client, extended_version_command(0xD6, "*"));

	CANMessageFrame version6Frame = {};
	const bool foundVersion6Response = poll_for_response(*server, testPlugin, 0xD6, version6Frame);
	const auto version6DeletedAllLength = server->deletedAllVersionsLabelLength;
	const bool version6CalledDeleteVersion = !server->lastVersionLabel.empty();

	server->deletedAllVersionsLabelLength = 0;
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD2, '*', ' ', ' ', ' ', ' ', ' ', ' ' });

	CANMessageFrame shortWildcardFrame = {};
	const bool foundShortWildcardResponse = poll_for_response(*server, testPlugin, 0xD2, shortWildcardFrame);
	const auto shortWildcardDeletedAllLength = server->deletedAllVersionsLabelLength;
	const bool shortWildcardCalledDeleteVersion = !server->lastVersionLabel.empty();

	server->deletedAllVersionsLabelLength = 0;
	testPlugin.clear_queue();
	receive_tp_from_client(internalECU, client, extended_version_command(0xD6, "A"));

	CANMessageFrame otherLabelFrame = {};
	const bool foundOtherLabelResponse = poll_for_response(*server, testPlugin, 0xD6, otherLabelFrame);
	const auto otherLabelDeletedAllLength = server->deletedAllVersionsLabelLength;
	const auto otherLabelSize = server->lastVersionLabel.size();

	server->deleteSucceeds = false;
	testPlugin.clear_queue();
	receive_from_client(internalECU, client, { 0xD2, '*', ' ', ' ', ' ', ' ', ' ', ' ' });

	CANMessageFrame failedWildcardFrame = {};
	const bool foundFailedWildcardResponse = poll_for_response(*server, testPlugin, 0xD2, failedWildcardFrame);

	ASSERT_TRUE(foundVersion5Response);
	EXPECT_EQ(0, version5DeletedAllLength);
	// Before VT version 6 "*" is not a wildcard, and a label can't contain it (ISO 11783-6:2018 E.1), so it is not correct
	EXPECT_TRUE(version5Label.empty());
	EXPECT_EQ(0x02, version5Frame.data[5]);
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

TEST_F(VirtualTerminalServerBusTest, WorkingSetShutsDownWhenMaintenanceMessagesStop)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	time_source.simulate_delay_ms(2000);
	receive_from_client(internalECU, client, { 0xFF, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	time_source.simulate_delay_ms(2000);
	server->run_update();
	const std::size_t shutdownsWhileMaintained = server->shutDownWorkingSets.size();
	const bool managedWhileMaintained = is_managed(internalECU, client, testPlugin);

	time_source.simulate_delay_ms(JUST_PAST_MAINTENANCE_TIMEOUT_MS);
	server->run_update();
	const bool managedAfterTimeout = is_managed(internalECU, client, testPlugin);

	EXPECT_EQ(0U, shutdownsWhileMaintained);
	EXPECT_TRUE(managedWhileMaintained);
	ASSERT_EQ(1U, server->shutdownReasons.size());
	EXPECT_EQ(VirtualTerminalServer::UnexpectedShutdownReason::MaintenanceTimeout, server->shutdownReasons.front());
	EXPECT_EQ(client, server->shutDownWorkingSets.front()->get_control_function());
	EXPECT_FALSE(managedAfterTimeout);
}

TEST_F(VirtualTerminalServerBusTest, OtherMessagesDoNotKeepAWorkingSetAlive)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	time_source.simulate_delay_ms(2000);
	receive_from_client(internalECU, client, { 0xC0, 0xFF, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF }); // Get Memory
	receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }); // End of Object Pool
	time_source.simulate_delay_ms(1001);
	server->run_update();

	ASSERT_EQ(1U, server->shutdownReasons.size());
	EXPECT_EQ(VirtualTerminalServer::UnexpectedShutdownReason::MaintenanceTimeout, server->shutdownReasons.front());
}

TEST_F(VirtualTerminalServerBusTest, TimedOutWorkingSetIsManagedAgainAfterAnInitiatingMessage)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	time_source.simulate_delay_ms(JUST_PAST_MAINTENANCE_TIMEOUT_MS);
	server->run_update();

	// Until it initiates again, even its maintenance messages are NACKed
	const bool maintenanceNacked = is_nacked(internalECU, client, testPlugin, { 0xFF, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool managedAfterInitiating = is_managed(internalECU, client, testPlugin);

	EXPECT_EQ(1U, server->shutdownReasons.size());
	EXPECT_TRUE(maintenanceNacked);
	EXPECT_TRUE(managedAfterInitiating);
}

TEST_F(VirtualTerminalServerBusTest, ActiveWorkingSetIsClearedWhenItTimesOut)
{
	CANMessageFrame statusWhileConnected = {};
	CANMessageFrame statusAfterTimeout = {};

	const bool poolLoaded = upload_pool(SELECTABLE_OBJECT_POOL);
	const bool foundStatusWhileConnected = read_status_message(*server, testPlugin, statusWhileConnected);
	time_source.simulate_delay_ms(JUST_PAST_MAINTENANCE_TIMEOUT_MS);
	server->run_update();
	const bool foundStatusAfterTimeout = read_status_message(*server, testPlugin, statusAfterTimeout);

	ASSERT_TRUE(poolLoaded);
	ASSERT_TRUE(foundStatusWhileConnected);
	EXPECT_EQ(0x81, statusWhileConnected.data[1]);
	EXPECT_EQ(1U, server->shutdownReasons.size());
	ASSERT_TRUE(foundStatusAfterTimeout);
	EXPECT_EQ(NULL_CAN_ADDRESS, statusAfterTimeout.data[1]);
	EXPECT_EQ(NULL_OBJECT_ID, statusAfterTimeout.data[2] | (statusAfterTimeout.data[3] << 8));
	EXPECT_EQ(NULL_OBJECT_ID, statusAfterTimeout.data[4] | (statusAfterTimeout.data[5] << 8));
}

TEST_F(VirtualTerminalServerBusTest, RepeatedInitiatingMessageRestartsTheWorkingSet)
{
	CANMessageFrame statusAfterRestart = {};

	const bool poolLoaded = upload_pool(SELECTABLE_OBJECT_POOL);
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool foundStatusAfterRestart = read_status_message(*server, testPlugin, statusAfterRestart);
	const bool managedAfterRestart = is_managed(internalECU, client, testPlugin);
	// Store Version is NACKed when there is no object pool, so this shows the old pool was discarded
	const bool storeVersionNacked = is_nacked(internalECU, client, testPlugin, { 0xD0, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	// The application is told from update(), not from the thread that received the message
	const std::size_t shutdownsBeforeUpdate = server->shutdownReasons.size();
	server->run_update();

	ASSERT_TRUE(poolLoaded);
	EXPECT_EQ(0U, shutdownsBeforeUpdate);
	ASSERT_EQ(1U, server->shutdownReasons.size());
	EXPECT_EQ(VirtualTerminalServer::UnexpectedShutdownReason::RepeatedInitiatingMessage, server->shutdownReasons.front());
	ASSERT_TRUE(foundStatusAfterRestart);
	EXPECT_EQ(NULL_CAN_ADDRESS, statusAfterRestart.data[1]);
	EXPECT_TRUE(managedAfterRestart);
	EXPECT_TRUE(storeVersionNacked);
}

TEST_F(VirtualTerminalServerBusTest, WorkingSetThatShutsDownWhileParsingIsReportedOnceParsingFinishes)
{
	load_vt3_pool_into_server();

	// Load Version starts parsing the stored pool, then the client restarts before the VT has answered
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0xD1, 'V', 'E', 'R', 'S', 'I', 'O', 'N' });
	const auto parsingWorkingSet = server->managedWorkingSetList.front();
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	// Handling the restart must not wait for the parsing thread, so the large pool is still being parsed here
	const auto stateAfterRestart = parsingWorkingSet->get_object_pool_processing_state();
	const bool managedAfterRestart = is_managed(internalECU, client, testPlugin);

	bool loadVersionResponseSent = false;
	for (std::uint_fast16_t attempt = 0; (attempt < 400) && server->shutdownReasons.empty(); attempt++)
	{
		CANMessageFrame frame = {};
		server->run_update();
		time_source.update_for_ms(1);
		while (testPlugin.read_frame(frame, 1))
		{
			loadVersionResponseSent |= (0xE6 == get_pdu_format(frame)) && (0xD1 == frame.data[0]);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	EXPECT_EQ(VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Running, stateAfterRestart);
	EXPECT_TRUE(managedAfterRestart);
	ASSERT_EQ(1U, server->shutdownReasons.size());
	EXPECT_EQ(VirtualTerminalServer::UnexpectedShutdownReason::RepeatedInitiatingMessage, server->shutdownReasons.front());
	EXPECT_EQ(parsingWorkingSet, server->shutDownWorkingSets.front());
	EXPECT_EQ(VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined, parsingWorkingSet->get_object_pool_processing_state());
	EXPECT_FALSE(loadVersionResponseSent);
}

TEST_F(VirtualTerminalServerBusTest, Version2ClientMaintenanceMessagesDoNotRestartTheWorkingSet)
{
	auto version2Client = test_helpers::force_claim_partnered_control_function(0x82, 0);

	receive_working_set_master_from_client(client);
	receive_working_set_master_from_client(version2Client);

	// Version 2 and prior clients have no Initiating bit, so bit 0 is set in every maintenance message
	for (std::uint_fast8_t i = 0; i < 3; i++)
	{
		receive_from_client(internalECU, client, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		receive_from_client(internalECU, version2Client, { 0xFF, 0x01, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		time_source.simulate_delay_ms(1000);
		server->run_update();
	}
	const bool clientManaged = is_managed(internalECU, client, testPlugin);
	const bool version2ClientManaged = is_managed(internalECU, version2Client, testPlugin);

	EXPECT_EQ(0U, server->shutdownReasons.size());
	EXPECT_TRUE(clientManaged);
	EXPECT_TRUE(version2ClientManaged);
}

TEST_F(VirtualTerminalServerBusTest, Version2ClientIsOnlyManagedAfterAWorkingSetMasterMessage)
{
	const bool nackedBeforeWorkingSetMaster = is_nacked(internalECU, client, testPlugin, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool managedBeforeWorkingSetMaster = is_managed(internalECU, client, testPlugin);

	receive_working_set_master_from_client(client);
	receive_from_client(internalECU, client, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool managedAfterWorkingSetMaster = is_managed(internalECU, client, testPlugin);

	// A managed working set master also answers requests for this message, which must not let it back in after a timeout
	receive_working_set_master_from_client(client);
	time_source.simulate_delay_ms(JUST_PAST_MAINTENANCE_TIMEOUT_MS);
	server->run_update();

	// Its maintenance messages look like initiating ones, but its object pool is gone
	const bool nackedAfterTimeout = is_nacked(internalECU, client, testPlugin, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool managedAfterTimeout = is_managed(internalECU, client, testPlugin);

	receive_working_set_master_from_client(client);
	receive_from_client(internalECU, client, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	const bool managedAfterRestart = is_managed(internalECU, client, testPlugin);

	EXPECT_TRUE(nackedBeforeWorkingSetMaster);
	EXPECT_FALSE(managedBeforeWorkingSetMaster);
	EXPECT_TRUE(managedAfterWorkingSetMaster);
	ASSERT_EQ(1U, server->shutdownReasons.size());
	EXPECT_EQ(VirtualTerminalServer::UnexpectedShutdownReason::MaintenanceTimeout, server->shutdownReasons.front());
	EXPECT_TRUE(nackedAfterTimeout);
	EXPECT_FALSE(managedAfterTimeout);
	EXPECT_TRUE(managedAfterRestart);
}

// ISO 11783-06-003 Test #4a, configuration 1
TEST_F(VirtualTerminalServerMessagingTest, ResponseToAWorkingSetMemberGoesToTheMasterWhenBothAreVersion3)
{
	std::uint8_t master = 0;
	std::uint8_t member = 0;
	std::uint8_t destination = 0;
	ASSERT_TRUE(read_soft_keys_response_destination_for_member(0x02, VirtualTerminalBase::VTVersion::Version3, 3, true, master, member, destination));
	EXPECT_EQ(master, destination);
}

TEST_F(VirtualTerminalServerMessagingTest, ResponseToAWorkingSetMemberGoesToTheMasterWhenTheWorkingSetIsVersion3)
{
	std::uint8_t master = 0;
	std::uint8_t member = 0;
	std::uint8_t destination = 0;
	ASSERT_TRUE(read_soft_keys_response_destination_for_member(0x05, VirtualTerminalBase::VTVersion::Version4, 3, true, master, member, destination));
	EXPECT_EQ(master, destination);
}

TEST_F(VirtualTerminalServerMessagingTest, ResponseToAWorkingSetMemberGoesToTheMasterWhenTheVTIsVersion3)
{
	std::uint8_t master = 0;
	std::uint8_t member = 0;
	std::uint8_t destination = 0;
	ASSERT_TRUE(read_soft_keys_response_destination_for_member(0x08, VirtualTerminalBase::VTVersion::Version3, 4, true, master, member, destination));
	EXPECT_EQ(master, destination);
}

TEST_F(VirtualTerminalServerMessagingTest, ResponseToAWorkingSetMemberGoesToTheMemberWhenBothAreVersion4OrLater)
{
	std::uint8_t master = 0;
	std::uint8_t member = 0;
	std::uint8_t destination = 0;
	ASSERT_TRUE(read_soft_keys_response_destination_for_member(0x0B, VirtualTerminalBase::VTVersion::Version6, 5, true, master, member, destination));
	EXPECT_EQ(member, destination);
}

TEST_F(VirtualTerminalServerMessagingTest, ResponseGoesToTheSenderWhenNoMasterAnnouncedIt)
{
	std::uint8_t master = 0;
	std::uint8_t member = 0;
	std::uint8_t destination = 0;
	ASSERT_TRUE(read_soft_keys_response_destination_for_member(0x0E, VirtualTerminalBase::VTVersion::Version3, 3, false, master, member, destination));
	EXPECT_EQ(member, destination);
}

// ISO 11783-7 10.2 d: a new Working Set Master message replaces the old definition
TEST_F(VirtualTerminalServerMessagingTest, ResponseGoesToTheSenderWhenItsMasterRedefinesTheSetWithoutIt)
{
	std::uint8_t master = 0;
	std::uint8_t member = 0;
	std::uint8_t destination = 0;
	ASSERT_TRUE(read_soft_keys_response_destination_for_member(0x11, VirtualTerminalBase::VTVersion::Version3, 3, true, master, member, destination, [this](DerivedTestVTServer &, std::shared_ptr<ControlFunction> workingSetMaster, std::shared_ptr<ControlFunction>) {
		receive_working_set_master_with_members_from_client(workingSetMaster, 0x01);
	}));
	EXPECT_EQ(member, destination);
}

// ISO 11783-7 10.2 d: a control function is a member of one working set only
TEST_F(VirtualTerminalServerMessagingTest, ResponseGoesToTheNewMasterWhenAMemberMovesToAnotherWorkingSet)
{
	std::uint8_t master = 0;
	std::uint8_t member = 0;
	std::uint8_t destination = 0;
	std::uint8_t newMaster = 0;
	ASSERT_TRUE(read_soft_keys_response_destination_for_member(0x14, VirtualTerminalBase::VTVersion::Version3, 3, true, master, member, destination, [this, &newMaster](DerivedTestVTServer &, std::shared_ptr<ControlFunction>, std::shared_ptr<ControlFunction> workingSetMember) {
		auto otherMaster = test_helpers::force_claim_partnered_control_function(0x17, 0);
		newMaster = otherMaster->get_address();
		receive_working_set_master_with_members_from_client(otherMaster, 0x02);
		receive_working_set_member_from_client(otherMaster, workingSetMember);
	}));
	EXPECT_NE(master, destination);
	EXPECT_EQ(newMaster, destination);
}

TEST_F(VirtualTerminalServerMessagingTest, ResponseGoesToTheSenderWhenItsMasterShutsDown)
{
	std::uint8_t master = 0;
	std::uint8_t member = 0;
	std::uint8_t destination = 0;
	std::size_t shutdowns = 0;
	ASSERT_TRUE(read_soft_keys_response_destination_for_member(0x18, VirtualTerminalBase::VTVersion::Version3, 3, true, master, member, destination, [this, &shutdowns](DerivedTestVTServer &server, std::shared_ptr<ControlFunction>, std::shared_ptr<ControlFunction>) {
		time_source.simulate_delay_ms(JUST_PAST_MAINTENANCE_TIMEOUT_MS);
		server.run_update();
		shutdowns = server.shutdownReasons.size();
	}));
	EXPECT_EQ(1U, shutdowns);
	EXPECT_EQ(member, destination);
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

TEST_F(VirtualTerminalServerBusTest, ESCMessageIsSentToTheWorkingSet)
{
	ASSERT_TRUE(server->send_esc_message(0x1234, 0, client));

	CANMessageFrame frame = {};
	time_source.update_for_ms(5);
	ASSERT_TRUE(testPlugin.read_frame(frame));

	EXPECT_EQ(0xE6, get_pdu_format(frame)); // VT to ECU
	EXPECT_EQ(client->get_address(), static_cast<std::uint8_t>(frame.identifier >> 8));
	EXPECT_EQ((std::vector<std::uint8_t>{ 0x04, 0x34, 0x12, 0x00, 0xFF, 0xFF, 0xFF, 0xFF }), payload_of(frame));
}

TEST_F(VirtualTerminalServerBusTest, PointingEventMessageMatchesTheVTVersion)
{
	CANMessageFrame frame = {};
	const auto send_pointing_event = [&](VirtualTerminalBase::KeyActivationCode activation) {
		const bool sent = server->send_pointing_event_message(activation, 0x0123, 0x0045, 0x1234, client);
		time_source.update_for_ms(5);
		return sent && testPlugin.read_frame(frame);
	};

	// VT version 3 only reports presses, so a release or a held touch sends nothing (ISO 11783-6:2018 H.6)
	server->version = VirtualTerminalBase::VTVersion::Version3;
	EXPECT_FALSE(server->send_pointing_event_message(VirtualTerminalBase::KeyActivationCode::ButtonUnlatchedOrReleased, 0x0123, 0x0045, 0x1234, client));
	EXPECT_FALSE(server->send_pointing_event_message(VirtualTerminalBase::KeyActivationCode::ButtonStillHeld, 0x0123, 0x0045, 0x1234, client));
	time_source.update_for_ms(5);
	EXPECT_FALSE(testPlugin.read_frame(frame));

	// A press only reports the position
	ASSERT_TRUE(send_pointing_event(VirtualTerminalBase::KeyActivationCode::ButtonPressedOrLatched));
	EXPECT_EQ(0xE6, get_pdu_format(frame)); // VT to ECU
	EXPECT_EQ(client->get_address(), static_cast<std::uint8_t>(frame.identifier >> 8));
	EXPECT_EQ((std::vector<std::uint8_t>{ 0x02, 0x23, 0x01, 0x45, 0x00, 0xFF, 0xFF, 0xFF }), payload_of(frame));

	// VT version 4 and 5 add the touch state
	server->version = VirtualTerminalBase::VTVersion::Version5;
	ASSERT_TRUE(send_pointing_event(VirtualTerminalBase::KeyActivationCode::ButtonStillHeld));
	EXPECT_EQ(0x02, frame.data[5]);
	EXPECT_EQ(0xFF, frame.data[6]);
	EXPECT_EQ(0xFF, frame.data[7]);

	// VT version 6 adds the parent mask and a new transaction number for each message
	server->version = VirtualTerminalBase::VTVersion::Version6;
	ASSERT_TRUE(send_pointing_event(VirtualTerminalBase::KeyActivationCode::ButtonPressedOrLatched));
	EXPECT_EQ(0x01, frame.data[5]);
	EXPECT_EQ(0x34, frame.data[6]);
	EXPECT_EQ(0x12, frame.data[7]);

	ASSERT_TRUE(send_pointing_event(VirtualTerminalBase::KeyActivationCode::ButtonUnlatchedOrReleased));
	EXPECT_EQ(0x10, frame.data[5]);
}

TEST_F(VirtualTerminalServerBusTest, ESCCommandIsAnsweredWithTheObjectWhoseInputWasAborted)
{
	CANMessageFrame noInputOpenResponse = {};
	CANMessageFrame inputAbortedResponse = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	const bool noInputOpenAnswered = send_and_poll({ 0x92, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, noInputOpenResponse);
	server->managedWorkingSetList.front()->set_object_open_for_input(0x1234);
	const bool inputAbortedAnswered = send_and_poll({ 0x92, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, inputAbortedResponse);

	ASSERT_TRUE(noInputOpenAnswered);
	EXPECT_EQ(0xFF, noInputOpenResponse.data[1]);
	EXPECT_EQ(0xFF, noInputOpenResponse.data[2]);
	EXPECT_EQ(0x01, noInputOpenResponse.data[3]);
	ASSERT_TRUE(inputAbortedAnswered);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0x92, 0x34, 0x12, 0x00, 0xFF, 0xFF, 0xFF, 0xFF }), payload_of(inputAbortedResponse));
}

TEST_F(VirtualTerminalServerBusTest, PreferredAssignmentCommandIsAnswered)
{
	CANMessageFrame response = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	const bool answered = send_and_poll({ 0x22, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, response);

	ASSERT_TRUE(answered);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0x22, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }), payload_of(response));
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
	const std::string text = "Success         ";
	std::vector<std::uint8_t> packet = { static_cast<std::uint8_t>(isobus::Macro::Command::ChangeStringValue), 0x34, 0x12, 0x10, 0x00 };
	packet.insert(packet.end(), text.begin(), text.end());
	ASSERT_TRUE(macro->add_command_packet(packet));
	ASSERT_TRUE(workingSet->add_object(macro));
	server.add_managed_working_set(workingSet);

	EXPECT_TRUE(server.execute_macro_for_test(macro->get_id(), workingSet));
	EXPECT_EQ(text, stringVariable->get_value());
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

TEST_F(VirtualTerminalServerBusTest, UnsupportedCommandRunByAMacroIsNotAnsweredWithUnsupportedVTFunction)
{
	server->version = VirtualTerminalBase::VTVersion::Version5;

	auto workingSet = std::make_shared<TestWorkingSet>(client);
	// Macro 5 holding a Lock/Unlock Mask, which the server has no handler for
	workingSet->add_iop_raw_data({ 0x05, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::Macro), 0x08, 0x00, static_cast<std::uint8_t>(Macro::Command::LockUnlockMask), 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF });
	workingSet->start_parsing_thread();
	workingSet->join_parsing_thread();
	server->add_managed_working_set(workingSet);

	CANMessageFrame response = {};
	const bool ran = server->execute_macro_for_test(5, workingSet);
	const bool answered = poll_for_response(*server, testPlugin, 0xFD, response);

	EXPECT_TRUE(ran);
	EXPECT_FALSE(answered);
}

TEST_F(VirtualTerminalServerBusTest, MaintenanceQueuedDuringAStallKeepsTheWorkingSet)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	time_source.simulate_delay_ms(2900);
	receive_from_client_without_processing(internalECU, client, { 0xFF, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	time_source.simulate_delay_ms(600);
	server->run_update();

	EXPECT_TRUE(server->shutdownReasons.empty());
	EXPECT_TRUE(is_managed(internalECU, client, testPlugin));
}

TEST_F(VirtualTerminalServerBusTest, MaintenanceQueuedBeforeAStallTimesOut)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client_without_processing(internalECU, client, { 0xFF, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	time_source.simulate_delay_ms(5000);
	server->run_update();

	ASSERT_EQ(1U, server->shutdownReasons.size());
	EXPECT_EQ(VirtualTerminalServer::UnexpectedShutdownReason::MaintenanceTimeout, server->shutdownReasons.front());
}

TEST_F(VirtualTerminalServerBusTest, MaintenanceQueuedDuringAStallInsideUpdateKeepsTheWorkingSet)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	// The maintenance message arrives while a hook inside update() stalls, after update() drained the queue
	server->onObjectPoolDeleted = [&](std::shared_ptr<VirtualTerminalServerManagedWorkingSet>) {
		time_source.simulate_delay_ms(2900);
		receive_from_client_without_processing(internalECU, client, { 0xFF, 0x00, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		time_source.simulate_delay_ms(600);
	};
	receive_from_client_without_processing(internalECU, client, { 0xB2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	server->run_update();
	server->run_update();

	EXPECT_TRUE(server->shutdownReasons.empty());
	EXPECT_TRUE(is_managed(internalECU, client, testPlugin));
}

TEST_F(VirtualTerminalServerBusTest, MessagesCanBeStoredFromAnotherThreadWhileUpdating)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();

	const CANIdentifier identifier(CANIdentifier::Type::Extended,
	                               static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
	                               CANIdentifier::CANPriority::PriorityLowest7,
	                               internalECU->get_address(),
	                               client->get_address());
	const CANMessage message(CANMessage::Type::Receive,
	                         identifier,
	                         { 0xC0, 0xFF, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF },
	                         client,
	                         internalECU,
	                         0);
	std::atomic<bool> done = { false };

	std::thread receiver([&]() {
		for (std::uint32_t i = 0; i < 1000; i++)
		{
			DerivedTestVTServer::store_rx_message(message, server.get());
		}
		done = true;
	});

	while (!done)
	{
		server->run_update();
	}
	receiver.join();
	server->run_update();

	EXPECT_TRUE(is_managed(internalECU, client, testPlugin));
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

TEST_F(VirtualTerminalServerBusTest, AudioCommandsAreForwardedToTheApplication)
{
	CANMessageFrame audioSignalResponse = {};
	CANMessageFrame audioVolumeResponse = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	// 3 activations of 880 Hz, 200 ms on and 100 ms off
	const bool audioSignalAnswered = send_and_poll({ 0xA3, 0x03, 0x70, 0x03, 0xC8, 0x00, 0x64, 0x00 }, audioSignalResponse);
	const bool audioVolumeAnswered = send_and_poll({ 0xA4, 0x4B, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, audioVolumeResponse);
	receive_from_client(internalECU, client, { 0xA4, 0xFA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });

	ASSERT_EQ(1U, server->audioSignals.size());
	EXPECT_EQ(3, server->audioSignals.front()[0]);
	EXPECT_EQ(880, server->audioSignals.front()[1]);
	EXPECT_EQ(200, server->audioSignals.front()[2]);
	EXPECT_EQ(100, server->audioSignals.front()[3]);
	EXPECT_EQ(client, server->audioSignalSources.front());
	ASSERT_EQ(2U, server->audioVolumes.size());
	EXPECT_EQ(75, server->audioVolumes.front());
	EXPECT_EQ(100, server->audioVolumes.back());
	EXPECT_EQ(client, server->audioVolumeSources.front());
	ASSERT_TRUE(audioSignalAnswered);
	EXPECT_EQ(0x00, audioSignalResponse.data[1]);
	ASSERT_TRUE(audioVolumeAnswered);
}

TEST_F(VirtualTerminalServerBusTest, UnsupportedFunctionSentDestinationSpecificIsAnsweredWithUnsupportedVTFunction)
{
	server->version = VirtualTerminalBase::VTVersion::Version5;

	CANMessageFrame response = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	const bool answered = send_and_poll({ 0x50, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, response, 0xFD);

	ASSERT_TRUE(answered);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0xFD, 0x50, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }), payload_of(response));
}

TEST_F(VirtualTerminalServerBusTest, UnsupportedFunctionSentToTheGlobalAddressIsNotAnswered)
{
	server->version = VirtualTerminalBase::VTVersion::Version5;

	CANMessageFrame response = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(7,
	                                                                                                             static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
	                                                                                                             client,
	                                                                                                             { 0x50, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }));
	CANNetworkManager::CANNetwork.update();
	const bool answered = poll_for_response(*server, testPlugin, 0xFD, response);

	EXPECT_FALSE(answered);
}

TEST_F(VirtualTerminalServerBusTest, UnsupportedFunctionIsNotAnsweredByAVersion4VT)
{
	server->version = VirtualTerminalBase::VTVersion::Version4;

	CANMessageFrame response = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	const bool answered = send_and_poll({ 0x50, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, response, 0xFD);

	EXPECT_FALSE(answered);
}

TEST_F(VirtualTerminalServerBusTest, FunctionsTheVTAcceptsWithoutAnAnswerOrAnswersElsewhereAreNotReportedUnsupported)
{
	server->version = VirtualTerminalBase::VTVersion::Version5;

	CANMessageFrame response = {};

	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	testPlugin.clear_queue();
	for (const std::uint8_t function : { 0x07, 0x09, 0x20, 0x24, 0x25, 0x26, 0xC1, 0xC2, 0xC3, 0xC4, 0xC7, 0xFD })
	{
		receive_from_client(internalECU, client, { function, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	}
	const bool answered = poll_for_response(*server, testPlugin, 0xFD, response);

	EXPECT_FALSE(answered);
}

TEST_F(VirtualTerminalServerBusTest, StatusMessageReportsTheNewAddressOfTheActiveWorkingSetMaster)
{
	// Working Set 0 with active mask 1000 and Container 2000 as its one object, Data Mask 1000, and Container 2000
	constexpr auto WORKING_SET = static_cast<std::uint8_t>(VirtualTerminalObjectType::WorkingSet);
	constexpr auto DATA_MASK = static_cast<std::uint8_t>(VirtualTerminalObjectType::DataMask);
	constexpr auto CONTAINER = static_cast<std::uint8_t>(VirtualTerminalObjectType::Container);
	auto workingSet = std::make_shared<TestWorkingSet>(client);
	workingSet->add_iop_raw_data({ 0x00, 0x00, WORKING_SET, 0x00, 0x01, 0xE8, 0x03, 0x01, 0x00, 0x00, 0xD0, 0x07, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x03, DATA_MASK, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0xD0, 0x07, CONTAINER, 0x0A, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00 });
	workingSet->start_parsing_thread();
	workingSet->join_parsing_thread();
	ASSERT_NE(nullptr, workingSet->get_working_set_object());
	server->add_managed_working_set(workingSet);
	server->select_working_set(workingSet);

	// The working set master moves to address 0xE4, as after a Commanded Address (ISO 11783-06-005 Test #2)
	CANMessageFrame claim = {};
	claim.channel = 0;
	claim.identifier = 0x18EEFFE4;
	claim.isExtendedFrame = true;
	claim.dataLength = CAN_DATA_LENGTH;
	for (std::uint8_t i = 0; i < CAN_DATA_LENGTH; i++)
	{
		claim.data[i] = static_cast<std::uint8_t>(client->get_NAME().get_full_name() >> (8 * i));
	}
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(claim);
	CANNetworkManager::CANNetwork.update();
	const std::uint8_t clientAddress = client->get_address();

	testPlugin.clear_queue();
	time_source.update_for_ms(1001);
	workingSet->set_working_set_maintenance_message_timestamp_ms(SystemTiming::get_timestamp_ms());
	server->run_update();
	time_source.update_for_ms(5);

	bool statusSent = false;
	std::uint8_t reportedAddress = NULL_CAN_ADDRESS;
	CANMessageFrame frame = {};
	while (testPlugin.read_frame(frame))
	{
		if ((0xE6 == get_pdu_format(frame)) && (0xFE == frame.data[0]))
		{
			statusSent = true;
			reportedAddress = frame.data[1];
		}
	}

	EXPECT_EQ(0xE4, clientAddress);
	ASSERT_TRUE(statusSent);
	EXPECT_EQ(0xE4, reportedAddress);
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

class VirtualTerminalServerMaskChangeMacroTest : public VirtualTerminalServerBusTest
{
protected:
	// Uploads the pool and returns the macros that ran after the command was answered
	std::vector<std::uint16_t> run_command(bool selectWorkingSet,
	                                       std::initializer_list<std::uint8_t> command,
	                                       const std::vector<std::uint8_t> &pool = create_mask_change_macro_pool(),
	                                       std::uint32_t *lastShownDataMask = nullptr,
	                                       std::uint32_t *lastShownSoftKeyMask = nullptr)
	{
		CANMessageFrame commandResponse = {};
		std::vector<std::uint16_t> macrosThatRan;

		server->selectOfferedWorkingSets = selectWorkingSet;
		const bool poolAnswered = upload_pool(pool);
		EXPECT_TRUE(poolAnswered);
		EXPECT_EQ(selectWorkingSet, nullptr != server->get_active_working_set());

		if (poolAnswered)
		{
			const auto workingSet = server->managedWorkingSetList.front();
			reset_mask_change_macros(workingSet);
			receive_from_client(internalECU, client, command);
			EXPECT_TRUE(poll_for_response(*server, testPlugin, *command.begin(), commandResponse));
			EXPECT_EQ(0x00, (0xAD == *command.begin()) ? commandResponse.data[3] : commandResponse.data[5]);
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
		return macrosThatRan;
	}
};

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeActiveMaskHidesTheOldMaskAndShowsTheNewOne)
{
	const auto macrosThatRan = run_command(true, { 0xAD, 0x00, 0x00, 0xE9, 0x03, 0xFF, 0xFF, 0xFF });

	// On Change Active Mask, then On Hide of Data Mask 1000 and Soft Key Mask 2000, then On Show of Data Mask 1001 and Soft Key Mask 2001
	EXPECT_EQ((std::vector<std::uint16_t>{ 10, 12, 13, 16, 17 }), macrosThatRan);
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeActiveMaskToTheActiveMaskOnlyRunsOnChangeActiveMask)
{
	const auto macrosThatRan = run_command(true, { 0xAD, 0x00, 0x00, 0xE8, 0x03, 0xFF, 0xFF, 0xFF });

	EXPECT_EQ((std::vector<std::uint16_t>{ 10 }), macrosThatRan);
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeActiveMaskOfAnInactiveWorkingSetOnlyRunsOnChangeActiveMask)
{
	const auto macrosThatRan = run_command(false, { 0xAD, 0x00, 0x00, 0xE9, 0x03, 0xFF, 0xFF, 0xFF });

	EXPECT_EQ((std::vector<std::uint16_t>{ 10 }), macrosThatRan);
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeSoftKeyMaskOfTheVisibleMaskHidesTheOldSoftKeyMaskAndShowsTheNewOne)
{
	const auto macrosThatRan = run_command(true, { 0xAE, 0x01, 0xE8, 0x03, 0xD1, 0x07, 0xFF, 0xFF });

	EXPECT_EQ((std::vector<std::uint16_t>{ 16, 17 }), macrosThatRan);
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeSoftKeyMaskOfAMaskThatIsNotShownRunsNoSoftKeyMaskMacros)
{
	const auto macrosThatRan = run_command(true, { 0xAE, 0x01, 0xE9, 0x03, 0xD0, 0x07, 0xFF, 0xFF });

	EXPECT_TRUE(macrosThatRan.empty());
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeActiveMaskHidesTheSoftKeyMaskThatWasShownEvenIfAMacroChangesIt)
{
	// On Change Active Mask also gives Data Mask 1000 Soft Key Mask 2001. Soft Key Mask 2000 was the one on the display.
	const auto pool = create_mask_change_macro_pool({ 0xAE, 0x01, 0xE8, 0x03, 0xD1, 0x07, 0xFF, 0xFF });
	const auto macrosThatRan = run_command(true, { 0xAD, 0x00, 0x00, 0xE9, 0x03, 0xFF, 0xFF, 0xFF }, pool);

	EXPECT_EQ((std::vector<std::uint16_t>{ 10, 12, 13, 16, 17 }), macrosThatRan);
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, OnChangeActiveMaskMacroThatChangesTheActiveMaskAgainDoesNotRecurse)
{
	// On Change Active Mask sends Change Active Mask again, which would run the same macro again without end
	const auto pool = create_mask_change_macro_pool({ 0xAD, 0x00, 0x00, 0xE9, 0x03, 0xFF, 0xFF, 0xFF });
	const auto macrosThatRan = run_command(true, { 0xAD, 0x00, 0x00, 0xE9, 0x03, 0xFF, 0xFF, 0xFF }, pool);
	// The nested command finds macro 10 still running and skips it. Its mask is already the active one, so it hides
	// and shows nothing. Then the outer command hides Data Mask 1000 and shows Data Mask 1001.
	EXPECT_EQ((std::vector<std::uint16_t>{ 10, 12, 13, 16, 17 }), macrosThatRan);
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeActiveMaskShowsTheMaskLastThatItsMacroMakesActive)
{
	// On Change Active Mask switches straight back to Data Mask 1000
	const auto pool = create_mask_change_macro_pool({ 0xAD, 0x00, 0x00, 0xE8, 0x03, 0xFF, 0xFF, 0xFF });
	std::uint32_t lastShownDataMask = 0;
	std::uint32_t lastShownSoftKeyMask = 0;
	const auto macrosThatRan = run_command(true, { 0xAD, 0x00, 0x00, 0xE9, 0x03, 0xFF, 0xFF, 0xFF }, pool, &lastShownDataMask, &lastShownSoftKeyMask);
	EXPECT_EQ((std::vector<std::uint16_t>{ 10, 11, 12, 13, 14, 15, 16, 17, 18 }), macrosThatRan);
	// Data Mask 1000 is active again, so it and its Soft Key Mask have to be the last ones shown
	EXPECT_EQ(1000U, lastShownDataMask);
	EXPECT_EQ(2000U, lastShownSoftKeyMask);
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeSoftKeyMaskShowsTheSoftKeyMaskLastThatItsMacroSets)
{
	// On Change Soft Key Mask of Data Mask 1000 gives it Soft Key Mask 2000 back
	const auto pool = create_mask_change_macro_pool({}, { 0xAE, 0x01, 0xE8, 0x03, 0xD0, 0x07, 0xFF, 0xFF });
	std::uint32_t lastShownSoftKeyMask = 0;
	const auto macrosThatRan = run_command(true, { 0xAE, 0x01, 0xE8, 0x03, 0xD1, 0x07, 0xFF, 0xFF }, pool, nullptr, &lastShownSoftKeyMask);
	EXPECT_EQ((std::vector<std::uint16_t>{ 15, 16, 17, 18 }), macrosThatRan);
	EXPECT_EQ(2000U, lastShownSoftKeyMask);
}

TEST_F(VirtualTerminalServerMaskChangeMacroTest, ChangeSoftKeyMaskUsesTheVisibilityFromBeforeItsMacros)
{
	// On Change Soft Key Mask of Data Mask 1000 makes Data Mask 1001 the active mask
	const auto pool = create_mask_change_macro_pool({}, { 0xAD, 0x00, 0x00, 0xE9, 0x03, 0xFF, 0xFF, 0xFF });
	const auto macrosThatRan = run_command(true, { 0xAE, 0x01, 0xE8, 0x03, 0xD1, 0x07, 0xFF, 0xFF }, pool);

	// The macro's Change Active Mask runs On Change Active Mask, hides Data Mask 1000 with its new Soft Key Mask 2001 and
	// shows Data Mask 1001 with Soft Key Mask 2001. Then Soft Key Mask 2000 is hidden and 2001 shown, because Data Mask 1000
	// was on the display when the command came.
	EXPECT_EQ((std::vector<std::uint16_t>{ 10, 12, 13, 16, 17, 18 }), macrosThatRan);
}

TEST_F(VirtualTerminalServerBusTest, VersionCommandsCheckTheLabelAndUseTheLastStoredVersionForABlankLabel)
{
	receive_from_client(internalECU, client, { 0xFF, 0x01, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	receive_from_client(internalECU, client, { 0x11, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 });
	testPlugin.clear_queue();

	const std::vector<std::uint8_t> storedLabel = { 'A', 'B', '-', '1', '2', '_', '3' };
	CANMessageFrame response = {};
	const auto errorCodeOf = [&](std::uint8_t function) {
		return poll_for_response(*server, testPlugin, function, response) ? static_cast<int>(response.data[5]) : -1;
	};

	// ISO 11783-6:2018 E.4: a blank label overwrites the last stored version, and is an error when there is none
	receive_from_client(internalECU, client, { 0xD0, ' ', ' ', ' ', ' ', ' ', ' ', ' ' });
	const int blankStoreWithoutAVersion = errorCodeOf(0xD0);

	// E.1: '*' and '>' can't be used in a version label (ISO 11783-06-001 Test #5, 5y)
	receive_from_client(internalECU, client, { 0xD0, 'A', 'B', 'C', '1', '2', '*', '>' });
	const int invalidStore = errorCodeOf(0xD0);
	const auto deletesAfterInvalidStore = server->deletedVersionLabels.size();

	receive_from_client(internalECU, client, { 0xD0, 'A', 'B', '-', '1', '2', '_', '3' });
	const int store = errorCodeOf(0xD0);

	// The blank label now overwrites AB-12_3 (5t), instead of storing a second version with a blank label
	server->lastVersionLabel.clear();
	receive_from_client(internalECU, client, { 0xD0, ' ', ' ', ' ', ' ', ' ', ' ', ' ' });
	const int blankStore = errorCodeOf(0xD0);
	const auto blankStoreLabel = server->lastVersionLabel;

	// E.6: a blank label loads the last stored version
	server->lastVersionLabel.clear();
	receive_from_client(internalECU, client, { 0xD1, ' ', ' ', ' ', ' ', ' ', ' ', ' ' });
	poll_for_response(*server, testPlugin, 0xD1, response);
	const auto blankLoadLabel = server->lastVersionLabel;

	// E.8: an invalid label is not correct, and a blank one deletes the last stored version
	receive_from_client(internalECU, client, { 0xD2, 'A', 'B', ':', '1', '2', ' ', ' ' });
	const int invalidDelete = errorCodeOf(0xD2);
	server->lastVersionLabel.clear();
	receive_from_client(internalECU, client, { 0xD2, ' ', ' ', ' ', ' ', ' ', ' ', ' ' });
	const int blankDelete = errorCodeOf(0xD2);
	const auto blankDeleteLabel = server->lastVersionLabel;

	EXPECT_EQ(0x02, blankStoreWithoutAVersion); // Version label is not correct
	EXPECT_EQ(0x02, invalidStore);
	EXPECT_EQ(0U, deletesAfterInvalidStore); // Nothing was stored or replaced
	EXPECT_EQ(0x00, store);
	EXPECT_EQ(0x00, blankStore);
	EXPECT_EQ(storedLabel, blankStoreLabel);
	EXPECT_EQ(storedLabel, blankLoadLabel);
	EXPECT_EQ(0x02, invalidDelete); // Version label is not correct or unknown
	EXPECT_EQ(0x00, blankDelete);
	EXPECT_EQ(storedLabel, blankDeleteLabel);
}

TEST_F(VirtualTerminalServerMessagingTest, MacroWithMoreThan255CommandsRunsAllOfThem)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0xEC, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xED, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	// Number Variable 1, and Macro 5 with 260 Change Numeric Value commands that set it to 0, 1, ... 259.
	// 260 commands of 8 bytes fit the 65535 bytes a macro can hold (ISO 11783-6:2018 Table B.56).
	constexpr std::uint16_t NUMBER_OF_COMMANDS = 260;
	constexpr std::uint16_t MACRO_LENGTH = NUMBER_OF_COMMANDS * 8;
	std::vector<std::uint8_t> pool = { 0x01, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::NumberVariable), 0x00, 0x00, 0x00, 0x00 };
	pool.insert(pool.end(), { 0x05, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::Macro), static_cast<std::uint8_t>(MACRO_LENGTH & 0xFF), static_cast<std::uint8_t>(MACRO_LENGTH >> 8) });
	for (std::uint16_t i = 0; i < NUMBER_OF_COMMANDS; i++)
	{
		pool.insert(pool.end(), { static_cast<std::uint8_t>(Macro::Command::ChangeNumericValue), 0x01, 0x00, 0xFF, static_cast<std::uint8_t>(i & 0xFF), static_cast<std::uint8_t>(i >> 8), 0x00, 0x00 });
	}
	auto workingSet = std::make_shared<TestWorkingSet>(client);
	workingSet->add_iop_raw_data(pool);
	workingSet->start_parsing_thread();
	workingSet->join_parsing_thread();
	serverUnderTest.add_managed_working_set(workingSet);

	const auto macro = std::static_pointer_cast<Macro>(workingSet->get_object_by_id(5));
	const auto variable = std::static_pointer_cast<NumberVariable>(workingSet->get_object_by_id(1));
	const bool parsed = (nullptr != macro) && (nullptr != variable);
	const bool ran = parsed && serverUnderTest.execute_macro_for_test(5, workingSet);
	CANHardwareInterface::stop();

	ASSERT_TRUE(parsed);
	EXPECT_EQ(NUMBER_OF_COMMANDS, macro->get_number_of_commands());
	std::vector<std::uint8_t> lastCommand;
	EXPECT_TRUE(macro->get_command_packet(NUMBER_OF_COMMANDS - 1, lastCommand));
	EXPECT_TRUE(ran);
	EXPECT_EQ(NUMBER_OF_COMMANDS - 1U, variable->get_value()); // The last command ran

	CANNetworkManager::CANNetwork.deactivate_control_function(client);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
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

/// @brief A client connected with a pool of input objects, whose tests set the focus and the open input as the VT application would
/// @details Data Mask 1000 holds Container 10 (Input Boolean 11), Container 12 (Input Boolean 13) and Input Number 20 (variable 30, both value 5),
/// and has Soft Key Mask 500 (Key 501). Data Mask 1001 is empty, and Data Mask 1002 holds Input Number 20 too.
class VirtualTerminalServerOpenInputTest : public VirtualTerminalServerBusTest
{
protected:
	void SetUp() override
	{
		VirtualTerminalServerBusTest::SetUp();
		server->version = VirtualTerminalBase::VTVersion::Version4;

		ASSERT_TRUE(upload_pool({ 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0xE8, 0x03, 0x00, 0x00, 0x00, 0xE8, 0x03, 0x01, 0x00, 0xF4, 0x01, 0x03, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE9, 0x03, 0x01, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x0A, 0x00, 0x03, 0x0A, 0x00, 0x0A, 0x00, 0x00, 0x01, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x00, 0x07, 0x00, 0x0A, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x0C, 0x00, 0x03, 0x0A, 0x00, 0x0A, 0x00, 0x00, 0x01, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0D, 0x00, 0x07, 0x00, 0x0A, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x14, 0x00, 0x09, 0x0A, 0x00, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x1E, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x01, 0x00, 0x1E, 0x00, 0x15, 0x05, 0x00, 0x00, 0x00, 0xEA, 0x03, 0x01, 0x00, 0xFF, 0xFF, 0x01, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF4, 0x01, 0x04, 0x00, 0x01, 0x00, 0xF5, 0x01, 0xF5, 0x01, 0x05, 0x00, 0x01, 0x00, 0x00 }));
		workingSet = server->get_active_working_set();
		ASSERT_NE(nullptr, workingSet);
		testPlugin.clear_queue();
	}

	/// @brief Opens the object for input and gives it the focus, as the VT application does
	void open_input(std::uint16_t objectID)
	{
		workingSet->set_object_open_for_input(objectID);
		workingSet->set_object_focus(objectID);
	}

	// Everything the VT sent to the clients since the last read, in the order it was sent
	std::vector<CANMessageFrame> read_vt_frames()
	{
		std::vector<CANMessageFrame> frames;
		CANMessageFrame frame = {};

		CANNetworkManager::CANNetwork.update();
		time_source.update_for_ms(5);
		while (testPlugin.read_frame(frame, 10))
		{
			if (0xE6 == get_pdu_format(frame))
			{
				frames.push_back(frame);
			}
		}
		return frames;
	}

	std::vector<CANMessageFrame> send(std::initializer_list<std::uint8_t> command)
	{
		receive_from_client(internalECU, client, command);
		return read_vt_frames();
	}

	// The objects replace the ones with the same IDs in the pool the client already loaded
	void send_pool_update(std::vector<std::uint8_t> objects)
	{
		objects.insert(objects.begin(), 0x11);
		receive_tp_from_client(internalECU, client, objects);
		receive_from_client(internalECU, client, { 0x12, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
	}

	bool pool_update_succeeded()
	{
		CANMessageFrame response = {};
		return poll_for_response(*server, testPlugin, 0x12, response) && (0x00 == response.data[1]);
	}

	static std::vector<std::uint8_t> functions_of(const std::vector<CANMessageFrame> &frames)
	{
		std::vector<std::uint8_t> functions;
		for (const auto &frame : frames)
		{
			functions.push_back(frame.data[0]);
		}
		return functions;
	}

	std::uint32_t input_number_value() const
	{
		return std::static_pointer_cast<InputNumber>(workingSet->get_object_by_id(20))->get_value();
	}

	std::uint16_t input_number_width() const
	{
		return std::static_pointer_cast<InputNumber>(workingSet->get_object_by_id(20))->get_width();
	}

	// Replaces Input Number 20 on Data Mask 1000 by Output List 40, whose only item is Input Number 20
	void show_input_number_20_through_output_list_40()
	{
		send_pool_update({ 0x28, 0x00, 0x25, 0x0A, 0x00, 0x0A, 0x00, 0xFF, 0xFF, 0x00, 0x01, 0x00, 0x14, 0x00, // Output List 40
		                   0xE8,
		                   0x03,
		                   0x01,
		                   0x00,
		                   0xF4,
		                   0x01,
		                   0x03,
		                   0x00,
		                   0x0A,
		                   0x00,
		                   0x00,
		                   0x00,
		                   0x00,
		                   0x00,
		                   0x0C,
		                   0x00,
		                   0x00,
		                   0x00,
		                   0x00,
		                   0x00,
		                   0x28,
		                   0x00,
		                   0x00,
		                   0x00,
		                   0x00,
		                   0x00 }); // Data Mask 1000 with Output List 40 in place of Input Number 20
		ASSERT_TRUE(pool_update_succeeded());
		EXPECT_TRUE(read_vt_frames().empty());
	}

	const std::vector<std::uint8_t> inputNumber20 = { 0x14, 0x00, 0x09, 0x0A, 0x00, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x1E, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x01, 0x00 };

	std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet;
};

TEST_F(VirtualTerminalServerOpenInputTest, PoolUpdateCannotGiveTheWorkingSetIdToAnotherObjectType)
{
	// Number Variable 31, then a Number Variable with the Working Set's ID
	send_pool_update({ 0x1F, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::NumberVariable), 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, static_cast<std::uint8_t>(VirtualTerminalObjectType::NumberVariable), 0x2A, 0x00, 0x00, 0x00 });

	EXPECT_FALSE(pool_update_succeeded());
	ASSERT_NE(nullptr, workingSet->get_working_set_object());
	EXPECT_EQ(VirtualTerminalObjectType::WorkingSet, workingSet->get_working_set_object()->get_object_type());
}

TEST_F(VirtualTerminalServerOpenInputTest, DisablingTheOpenInputIsRefused)
{
	workingSet->set_object_open_for_input(20);

	const auto frames = send({ 0xA1, 0x14, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF }); // Disable Input Number 20

	ASSERT_EQ((std::vector<std::uint8_t>{ 0xA1 }), functions_of(frames));
	EXPECT_EQ(0x08, frames[0].data[4]); // Input object is being modified
	EXPECT_TRUE(std::static_pointer_cast<InputNumber>(workingSet->get_object_by_id(20))->get_option2(InputNumber::Options2::Enabled));
}

TEST_F(VirtualTerminalServerOpenInputTest, SelectingAnotherObjectWhileAnInputIsOpenIsRefused)
{
	open_input(20);

	const auto frames = send({ 0xA2, 0x0B, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }); // Select Input Boolean 11

	ASSERT_EQ((std::vector<std::uint8_t>{ 0xA2 }), functions_of(frames));
	EXPECT_EQ(0x08, frames[0].data[4]); // Another field is being modified
	EXPECT_EQ(20, workingSet->get_object_focus());
}

TEST_F(VirtualTerminalServerOpenInputTest, ChangingTheValueOfTheOpenInputIsRefused)
{
	workingSet->set_object_open_for_input(20);

	const auto frames = send({ 0xA8, 0x14, 0x00, 0xFF, 0x07, 0x00, 0x00, 0x00 }); // Set Input Number 20 to 7

	ASSERT_EQ((std::vector<std::uint8_t>{ 0xA8 }), functions_of(frames));
	EXPECT_EQ(0x04, frames[0].data[3]); // Value in use
	EXPECT_EQ(5U, input_number_value());
}

TEST_F(VirtualTerminalServerOpenInputTest, ChangingTheVariableOfTheOpenInputIsRefused)
{
	workingSet->set_object_open_for_input(20);

	const auto frames = send({ 0xA8, 0x1E, 0x00, 0xFF, 0x08, 0x00, 0x00, 0x00 }); // Set Number Variable 30 to 8

	ASSERT_EQ((std::vector<std::uint8_t>{ 0xA8 }), functions_of(frames));
	EXPECT_EQ(0x04, frames[0].data[3]); // Value in use
	EXPECT_EQ(5U, std::static_pointer_cast<NumberVariable>(workingSet->get_object_by_id(30))->get_value());
}

TEST_F(VirtualTerminalServerOpenInputTest, ChangingTheValueOfTheOpenInputIsAcceptedFromVTVersion6)
{
	server->version = VirtualTerminalBase::VTVersion::Version6;
	workingSet->set_object_open_for_input(20);

	const auto frames = send({ 0xA8, 0x14, 0x00, 0xFF, 0x07, 0x00, 0x00, 0x00 }); // Set Input Number 20 to 7

	ASSERT_EQ((std::vector<std::uint8_t>{ 0xA8 }), functions_of(frames));
	EXPECT_EQ(0x00, frames[0].data[3]);
	EXPECT_EQ(7U, input_number_value());
}

TEST_F(VirtualTerminalServerOpenInputTest, DisablingTheFocusedObjectDeselectsItAfterTheResponse)
{
	workingSet->set_object_focus(11);

	const auto frames = send({ 0xA1, 0x0B, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF }); // Disable Input Boolean 11

	ASSERT_EQ((std::vector<std::uint8_t>{ 0xA1, 0x03 }), functions_of(frames));
	EXPECT_EQ(0x0B, frames[1].data[1]);
	EXPECT_EQ(0x00, frames[1].data[2]);
	EXPECT_EQ(0x00, frames[1].data[3]); // Not selected
	EXPECT_EQ(0x00, frames[1].data[4]); // Not open for input
	EXPECT_EQ(NULL_OBJECT_ID, workingSet->get_object_focus());
}

TEST_F(VirtualTerminalServerOpenInputTest, ChangingTheActiveMaskDeselectsTheFocusedObjectBeforeTheResponse)
{
	workingSet->set_object_focus(11);

	const auto frames = send({ 0xAD, 0x00, 0x00, 0xE9, 0x03, 0xFF, 0xFF, 0xFF }); // Change the active mask to Data Mask 1001

	ASSERT_EQ((std::vector<std::uint8_t>{ 0x03, 0xAD }), functions_of(frames));
	EXPECT_EQ(0x0B, frames[0].data[1]);
	EXPECT_EQ(0x00, frames[0].data[3]);
	EXPECT_EQ(0x00, frames[1].data[3]);
	EXPECT_EQ(NULL_OBJECT_ID, workingSet->get_object_focus());
}

TEST_F(VirtualTerminalServerOpenInputTest, ChangingTheActiveMaskAbortsTheOpenInputEvenWhenTheNewMaskShowsIt)
{
	open_input(20);

	const auto frames = send({ 0xAD, 0x00, 0x00, 0xEA, 0x03, 0xFF, 0xFF, 0xFF }); // Change the active mask to Data Mask 1002, which shows Input Number 20 too

	ASSERT_EQ((std::vector<std::uint8_t>{ 0x03, 0x04, 0xAD }), functions_of(frames));
	EXPECT_EQ(20, frames[0].data[1]);
	EXPECT_EQ(0x00, frames[0].data[3]);
	EXPECT_EQ(20, frames[1].data[1]);
	EXPECT_EQ(0x00, frames[1].data[2]);
	EXPECT_EQ(0x00, frames[1].data[3]);
	EXPECT_EQ(0x00, frames[2].data[3]);
	EXPECT_EQ(1, server->abortInputCount);
	EXPECT_EQ(NULL_OBJECT_ID, workingSet->get_object_open_for_input());
	EXPECT_EQ(NULL_OBJECT_ID, workingSet->get_object_focus());
}

TEST_F(VirtualTerminalServerOpenInputTest, ChangingToTheSameMaskKeepsTheOpenInput)
{
	open_input(20);

	const auto frames = send({ 0xAD, 0x00, 0x00, 0xE8, 0x03, 0xFF, 0xFF, 0xFF }); // Change the active mask to Data Mask 1000

	ASSERT_EQ((std::vector<std::uint8_t>{ 0xAD }), functions_of(frames));
	EXPECT_EQ(0, server->abortInputCount);
	EXPECT_EQ(20, workingSet->get_object_open_for_input());
}

TEST_F(VirtualTerminalServerOpenInputTest, HidingTheContainerOfTheFocusedObjectDeselectsIt)
{
	workingSet->set_object_focus(11);

	const auto frames = send({ 0xA0, 0x0A, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF }); // Hide Container 10

	ASSERT_EQ((std::vector<std::uint8_t>{ 0xA0, 0x03 }), functions_of(frames));
	EXPECT_EQ(0x0B, frames[1].data[1]);
	EXPECT_EQ(0x00, frames[1].data[3]);
	EXPECT_EQ(NULL_OBJECT_ID, workingSet->get_object_focus());
}

TEST_F(VirtualTerminalServerOpenInputTest, HidingAContainerThatDoesNotHoldTheFocusedObjectSendsNoDeselect)
{
	workingSet->set_object_focus(11);

	const auto frames = send({ 0xA0, 0x0C, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF }); // Hide Container 12

	ASSERT_EQ((std::vector<std::uint8_t>{ 0xA0 }), functions_of(frames));
	EXPECT_EQ(11, workingSet->get_object_focus());
}

TEST_F(VirtualTerminalServerOpenInputTest, ChangingTheSoftKeyMaskDeselectsTheFocusedKey)
{
	workingSet->set_object_focus(501);

	const auto frames = send({ 0xAE, 0x01, 0xE8, 0x03, 0xFF, 0xFF, 0xFF, 0xFF }); // Remove the soft key mask of Data Mask 1000

	ASSERT_EQ((std::vector<std::uint8_t>{ 0xAE, 0x03 }), functions_of(frames));
	EXPECT_EQ(0x00, frames[0].data[5]);
	EXPECT_EQ(0xF5, frames[1].data[1]);
	EXPECT_EQ(0x01, frames[1].data[2]);
	EXPECT_EQ(0x00, frames[1].data[3]);
	EXPECT_EQ(NULL_OBJECT_ID, workingSet->get_object_focus());
}

TEST_F(VirtualTerminalServerOpenInputTest, ChangingTheSoftKeyMaskAttributeDeselectsTheFocusedKey)
{
	workingSet->set_object_focus(501);

	const auto frames = send({ 0xAF, 0xE8, 0x03, 0x02, 0xFF, 0xFF, 0x00, 0x00 }); // Set the soft key mask attribute of Data Mask 1000 to none

	ASSERT_EQ((std::vector<std::uint8_t>{ 0xAF, 0x03 }), functions_of(frames));
	EXPECT_EQ(0x00, frames[0].data[4]);
	EXPECT_EQ(0xF5, frames[1].data[1]);
	EXPECT_EQ(0x01, frames[1].data[2]);
	EXPECT_EQ(0x00, frames[1].data[3]);
	EXPECT_EQ(NULL_OBJECT_ID, workingSet->get_object_focus());
}

TEST_F(VirtualTerminalServerOpenInputTest, ReplacingTheOpenInputInAPoolUpdateAbortsIt)
{
	open_input(20);

	send_pool_update(inputNumber20);
	ASSERT_TRUE(pool_update_succeeded());
	const auto frames = read_vt_frames();

	ASSERT_EQ((std::vector<std::uint8_t>{ 0x03, 0x04 }), functions_of(frames));
	EXPECT_EQ(20, frames[0].data[1]);
	EXPECT_EQ(0x00, frames[0].data[3]);
	EXPECT_EQ(20, frames[1].data[1]);
	EXPECT_EQ(1, server->abortInputCount);
	EXPECT_EQ(NULL_OBJECT_ID, workingSet->get_object_open_for_input());
}

TEST_F(VirtualTerminalServerOpenInputTest, UpdatingThePoolWithoutTheOpenInputKeepsIt)
{
	open_input(20);

	send_pool_update({ 0x0C, 0x00, 0x03, 0x0A, 0x00, 0x0A, 0x00, 0x00, 0x01, 0x00, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00 }); // Container 12 again
	ASSERT_TRUE(pool_update_succeeded());

	EXPECT_TRUE(read_vt_frames().empty());
	EXPECT_EQ(0, server->abortInputCount);
	EXPECT_EQ(20, workingSet->get_object_open_for_input());
}

TEST_F(VirtualTerminalServerOpenInputTest, InputOpenedWhileThePoolParsesIsKept)
{
	open_input(20);

	send_pool_update(inputNumber20);
	for (std::uint_fast16_t attempt = 0; (attempt < 1000) && (VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Success != workingSet->get_object_pool_processing_state()); attempt++)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_EQ(VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Success, workingSet->get_object_pool_processing_state());

	// The operator closes Input Number 20 and opens Input Boolean 11 before the server handles the parse result
	open_input(11);
	ASSERT_TRUE(pool_update_succeeded());

	EXPECT_TRUE(read_vt_frames().empty());
	EXPECT_EQ(0, server->abortInputCount);
	EXPECT_EQ(11, workingSet->get_object_open_for_input());
	EXPECT_EQ(11, workingSet->get_object_focus());
}

TEST_F(VirtualTerminalServerOpenInputTest, AppThatDoesNotReportTheOpenInputGetsOneDeselectAndOneEsc)
{
	workingSet->set_object_focus(20);
	server->inputOpenOnlyInApp = 20;

	const auto frames = send({ 0xAD, 0x00, 0x00, 0xE9, 0x03, 0xFF, 0xFF, 0xFF }); // Change the active mask to Data Mask 1001

	ASSERT_EQ((std::vector<std::uint8_t>{ 0x03, 0x04, 0xAD }), functions_of(frames));
	EXPECT_EQ(20, frames[0].data[1]);
	EXPECT_EQ(0x00, frames[0].data[2]);
	EXPECT_EQ(0x00, frames[0].data[3]);
	EXPECT_EQ(20, frames[1].data[1]);
	EXPECT_EQ(0x00, frames[1].data[2]);
	EXPECT_EQ(1, server->abortInputCount);
	EXPECT_EQ(NULL_OBJECT_ID, workingSet->get_object_focus());
}

TEST_F(VirtualTerminalServerOpenInputTest, ChangingAnAttributeOfTheOpenInputIsRefused)
{
	workingSet->set_object_open_for_input(20);

	const auto frames = send({ 0xAF, 0x14, 0x00, 0x01, 0x0B, 0x00, 0x00, 0x00 }); // Set the width of Input Number 20 to 11

	ASSERT_EQ((std::vector<std::uint8_t>{ 0xAF }), functions_of(frames));
	EXPECT_EQ(0x08, frames[0].data[4]); // Value in use
	EXPECT_EQ(10U, input_number_width());
}

TEST_F(VirtualTerminalServerOpenInputTest, ChangingAnAttributeOfTheOpenInputIsAcceptedFromVTVersion6)
{
	server->version = VirtualTerminalBase::VTVersion::Version6;
	workingSet->set_object_open_for_input(20);

	const auto frames = send({ 0xAF, 0x14, 0x00, 0x01, 0x0B, 0x00, 0x00, 0x00 }); // Set the width of Input Number 20 to 11

	ASSERT_EQ((std::vector<std::uint8_t>{ 0xAF }), functions_of(frames));
	EXPECT_EQ(0x00, frames[0].data[4]);
	EXPECT_EQ(11U, input_number_width());
}

TEST_F(VirtualTerminalServerOpenInputTest, ReplacingTheListItemThatHoldsTheOpenInputAbortsIt)
{
	show_input_number_20_through_output_list_40();
	open_input(20);

	const auto frames = send({ 0xB1, 0x28, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF }); // Set item 0 of Output List 40 to empty

	ASSERT_EQ((std::vector<std::uint8_t>{ 0xB1, 0x03, 0x04 }), functions_of(frames));
	EXPECT_EQ(20, frames[2].data[1]);
	EXPECT_EQ(1, server->abortInputCount);
	EXPECT_EQ(NULL_OBJECT_ID, workingSet->get_object_open_for_input());
}

TEST_F(VirtualTerminalServerOpenInputTest, ReplacingAListItemThatDoesNotHoldTheOpenInputKeepsIt)
{
	show_input_number_20_through_output_list_40();
	open_input(11);

	const auto frames = send({ 0xB1, 0x28, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF }); // Set item 0 of Output List 40 to empty

	ASSERT_EQ((std::vector<std::uint8_t>{ 0xB1 }), functions_of(frames));
	EXPECT_EQ(0, server->abortInputCount);
	EXPECT_EQ(11, workingSet->get_object_open_for_input());
}

TEST_F(VirtualTerminalServerOpenInputTest, CommandsAreOnlyProcessedInUpdate)
{
	std::uint32_t repaints = 0;
	server->get_on_repaint_event_dispatcher().add_listener([&repaints](std::shared_ptr<VirtualTerminalServerManagedWorkingSet>) { repaints++; });

	receive_from_client_without_processing(internalECU, client, { 0xA8, 0x14, 0x00, 0xFF, 0x07, 0x00, 0x00, 0x00 }); // Change Numeric Value, Input Number 20 to 7
	const auto framesBefore = read_vt_frames();

	EXPECT_TRUE(functions_of(framesBefore).empty());
	EXPECT_EQ(5U, input_number_value());
	EXPECT_EQ(0U, repaints);

	server->run_update();
	const auto framesAfter = read_vt_frames();

	EXPECT_EQ((std::vector<std::uint8_t>{ 0xA8 }), functions_of(framesAfter));
	EXPECT_EQ(7U, input_number_value());
	EXPECT_GT(repaints, 0U);
}

TEST_F(VirtualTerminalServerMessagingTest, OutputMeterAndInputBooleanStaySquare)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0xE5, 0, time_source);
	auto client = test_helpers::force_claim_partnered_control_function(0xE6, 0);

	DerivedTestVTServer serverUnderTest(internalECU);
	serverUnderTest.initialize();

	// Working Set 0 with Data Mask 1000, Output Meter 6 (width 50) and Input Boolean 7 (width 20)
	constexpr auto WORKING_SET = static_cast<std::uint8_t>(VirtualTerminalObjectType::WorkingSet);
	constexpr auto DATA_MASK = static_cast<std::uint8_t>(VirtualTerminalObjectType::DataMask);
	constexpr auto OUTPUT_METER = static_cast<std::uint8_t>(VirtualTerminalObjectType::OutputMeter);
	constexpr auto INPUT_BOOLEAN = static_cast<std::uint8_t>(VirtualTerminalObjectType::InputBoolean);
	auto workingSet = std::make_shared<TestWorkingSet>(client);
	workingSet->add_iop_raw_data({ 0x00, 0x00, WORKING_SET, 0x00, 0x01, 0xE8, 0x03, 0x00, 0x00, 0x00, 0xE8, 0x03, DATA_MASK, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x06, 0x00, OUTPUT_METER, 0x32, 0x00, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0xB4, 0x00, 0x00, 0x64, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x07, 0x00, INPUT_BOOLEAN, 0x00, 0x14, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0x00 });
	workingSet->start_parsing_thread();
	workingSet->join_parsing_thread();
	serverUnderTest.add_managed_working_set(workingSet);
	const auto meter = workingSet->get_object_by_id(6);
	const auto inputBoolean = workingSet->get_object_by_id(7);
	CANMessageFrame response = {};

	const bool parsed = (nullptr != meter) && (nullptr != inputBoolean);
	std::uint16_t meterAfterAttribute[2] = { 0, 0 };
	std::uint16_t booleanAfterAttribute[2] = { 0, 0 };
	int sizeZeroError = -1;
	std::uint16_t meterAfterSizeZero[2] = { 1, 1 };
	std::uint16_t meterAfterSize[2] = { 0, 0 };
	std::uint16_t booleanAfterSize[2] = { 0, 0 };

	if (parsed)
	{
		// Change Attribute width: the height follows (ISO 11783-6:2018, B.8.2 and B.11.2)
		receive_from_client_without_processing(internalECU, client, { 0xAF, 0x06, 0x00, 0x01, 0x50, 0x00, 0x00, 0x00 });
		poll_for_response(serverUnderTest, testPlugin, 0xAF, response);
		meterAfterAttribute[0] = meter->get_width();
		meterAfterAttribute[1] = meter->get_height();
		receive_from_client_without_processing(internalECU, client, { 0xAF, 0x07, 0x00, 0x02, 0x1E, 0x00, 0x00, 0x00 });
		poll_for_response(serverUnderTest, testPlugin, 0xAF, response);
		booleanAfterAttribute[0] = inputBoolean->get_width();
		booleanAfterAttribute[1] = inputBoolean->get_height();

		// Change Size with a height of 0: the size is 0 and the object is not drawn (F.18). The command is accepted.
		receive_from_client_without_processing(internalECU, client, { 0xA6, 0x06, 0x00, 0x64, 0x00, 0x00, 0x00, 0xFF });
		if (poll_for_response(serverUnderTest, testPlugin, 0xA6, response))
		{
			sizeZeroError = response.data[3];
		}
		meterAfterSizeZero[0] = meter->get_width();
		meterAfterSizeZero[1] = meter->get_height();

		// Change Size to a size that is not square: the width is the side of the square
		receive_from_client_without_processing(internalECU, client, { 0xA6, 0x06, 0x00, 0x64, 0x00, 0x3C, 0x00, 0xFF });
		poll_for_response(serverUnderTest, testPlugin, 0xA6, response);
		meterAfterSize[0] = meter->get_width();
		meterAfterSize[1] = meter->get_height();
		receive_from_client_without_processing(internalECU, client, { 0xA6, 0x07, 0x00, 0x28, 0x00, 0x14, 0x00, 0xFF });
		poll_for_response(serverUnderTest, testPlugin, 0xA6, response);
		booleanAfterSize[0] = inputBoolean->get_width();
		booleanAfterSize[1] = inputBoolean->get_height();
	}
	CANHardwareInterface::stop();

	ASSERT_TRUE(parsed);
	EXPECT_EQ(80, meterAfterAttribute[0]);
	EXPECT_EQ(80, meterAfterAttribute[1]);
	EXPECT_EQ(30, booleanAfterAttribute[0]);
	EXPECT_EQ(30, booleanAfterAttribute[1]);
	EXPECT_EQ(0x00, sizeZeroError);
	EXPECT_EQ(0, meterAfterSizeZero[0]);
	EXPECT_EQ(0, meterAfterSizeZero[1]);
	EXPECT_EQ(100, meterAfterSize[0]);
	EXPECT_EQ(100, meterAfterSize[1]);
	EXPECT_EQ(40, booleanAfterSize[0]);
	EXPECT_EQ(40, booleanAfterSize[1]);

	CANNetworkManager::CANNetwork.deactivate_control_function(client);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}
