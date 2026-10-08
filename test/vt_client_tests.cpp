#include <gtest/gtest.h>

#include <algorithm>

#include <map>
#include <set>
#include <tuple>

#include "isobus//utility/iop_file_interface.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"
#include "helpers/test_fixture.hpp"

using namespace isobus;

class DerivedTestVTClient : public VirtualTerminalClient
{
public:
	DerivedTestVTClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource) :
	  VirtualTerminalClient(partner, clientSource) {
		  // Does nothing
	  };

	void test_wrapper_process_rx_message(const CANMessage &message, void *parentPointer)
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

	void test_wrapper_set_state(VirtualTerminalClient::StateMachineState value)
	{
		if (StateMachineState::SendAuxiliaryPreferredAssignment == value)
		{
			transition_to_connected_or_auxiliary_assignment();
		}
		else
		{
			VirtualTerminalClient::set_state(value);
		}
	}

	void test_wrapper_set_vt_version(std::uint8_t version)
	{
		connectedVTVersion = version;
	}
	void test_wrapper_refresh_vt_status()
	{
		lastVTStatusTimestamp_ms = SystemTiming::get_timestamp_ms();
	}
	bool test_wrapper_auxiliary_enabled() const
	{
		return auxiliaryFunctionsEnabled;
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

	void test_wrapper_process_command_queue()
	{
		VirtualTerminalClient::process_command_queue();
	}
};

std::vector<std::uint8_t> DerivedTestVTClient::staticTestPool;

class VirtualTerminalTest : public AgIsoStackTestFixture
{
	// Wrapper to give tests a more meaningful name - no content.
};

TEST_F(VirtualTerminalTest, InitializeAndInitialState)
{
	NAME clientNAME(0);
	auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(clientNAME, 0, 0x26);

	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	vtNameFilters.push_back(testFilter);

	auto vtPartner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);

	EXPECT_EQ(false, clientUnderTest.get_is_initialized());
	EXPECT_EQ(false, clientUnderTest.get_is_connected());

	clientUnderTest.initialize(false);

	EXPECT_EQ(true, clientUnderTest.get_is_initialized());

	clientUnderTest.initialize(false);

	EXPECT_EQ(true, clientUnderTest.get_is_initialized()); // Double init should be at least tolerated

	EXPECT_EQ(false, clientUnderTest.get_has_adjustable_volume_output());
	EXPECT_EQ(false, clientUnderTest.get_multiple_frequency_audio_output());
	EXPECT_EQ(false, clientUnderTest.get_support_pointing_device_with_pointing_message());
	EXPECT_EQ(false, clientUnderTest.get_support_touchscreen_with_pointing_message());
	EXPECT_EQ(false, clientUnderTest.get_support_intermediate_coordinates_during_drag_operations());
	EXPECT_EQ(0, clientUnderTest.get_number_y_pixels());
	EXPECT_EQ(0, clientUnderTest.get_number_x_pixels());
	EXPECT_EQ(VirtualTerminalClient::VTVersion::ReservedOrUnknown, clientUnderTest.get_connected_vt_version());

	EXPECT_NE(nullptr, clientUnderTest.get_internal_control_function());
	EXPECT_NE(nullptr, clientUnderTest.get_partner_control_function());

	clientUnderTest.terminate();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

// Annex J regression scenarios use the existing receive boundary to avoid simulating
// the unrelated object-pool handshake. Runtime behavior goes through update(), CAN and events.
struct AuxiliaryPreferences
{
	std::map<std::uint64_t, std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction>> byName;
};

static std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction> load_auxiliary_preferences(std::uint64_t name, std::uint16_t, void *context)
{
	return static_cast<AuxiliaryPreferences *>(context)->byName[name];
}

static void store_auxiliary_preferences(std::uint64_t name, std::uint16_t, const std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction> &assignments, void *context)
{
	static_cast<AuxiliaryPreferences *>(context)->byName[name] = assignments;
}

// Only shared CAN setup/cleanup; the client is destroyed before this local fixture.
struct AuxiliaryTestCAN
{
	VirtualCANPlugin vt;
	std::shared_ptr<InternalControlFunction> ecu;
	std::shared_ptr<PartneredControlFunction> partner;

	explicit AuxiliaryTestCAN(test_helpers::TestTimeSource &clock)
	{
		vt.open();
		CANHardwareInterface::set_number_of_can_channels(1);
		CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
		CANHardwareInterface::start(false);
		ecu = test_helpers::claim_internal_control_function(0x37, 0, clock);
		partner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	}
	~AuxiliaryTestCAN()
	{
		vt.close();
		CANHardwareInterface::stop();
		CANNetworkManager::CANNetwork.deactivate_control_function(partner);
		CANNetworkManager::CANNetwork.deactivate_control_function(ecu);
	}
};

static std::shared_ptr<ControlFunction> auxiliary_device(std::uint64_t name, std::uint8_t address)
{
	return std::make_shared<ControlFunction>(NAME(name), address, 0);
}

static void receive_auxiliary(DerivedTestVTClient &client, const std::shared_ptr<ControlFunction> &source, const std::vector<std::uint8_t> &data)
{
	const bool broadcast = (data[0] == 0x23) || (data[0] == 0x26) || (data[0] == 0xFE);
	const auto pgn = (data[0] == 0x23) ? 0xE700 : 0xE600;
	const auto message = broadcast ? test_helpers::create_message_broadcast(6, pgn, source, data.data(), static_cast<std::uint32_t>(data.size())) : test_helpers::create_message(6, pgn, client.get_internal_control_function(), source, data.data(), static_cast<std::uint32_t>(data.size()));
	client.test_wrapper_process_rx_message(message, &client);
}

static std::vector<std::uint8_t> auxiliary_function_pool(std::initializer_list<std::uint16_t> functions)
{
	std::vector<std::uint8_t> pool;
	for (const auto id : functions)
	{
		pool.insert(pool.end(), { static_cast<std::uint8_t>(id), static_cast<std::uint8_t>(id >> 8), 31, 0, 4, 0 });
	}
	return pool;
}

static void auxiliary_maintenance(DerivedTestVTClient &client, const std::shared_ptr<ControlFunction> &source, std::uint16_t model, std::uint8_t ready = 1)
{
	receive_auxiliary(client, source, { 0x23, static_cast<std::uint8_t>(model), static_cast<std::uint8_t>(model >> 8), ready, 0xFF, 0xFF, 0xFF, 0xFF });
}

static void auxiliary_assignment(DerivedTestVTClient &client, const std::shared_ptr<ControlFunction> &vt, std::uint64_t name, std::uint16_t function, std::uint16_t input, std::uint8_t flags = 0x84)
{
	std::vector<std::uint8_t> bytes(14, 0xFF);
	bytes[0] = 0x24;
	for (std::size_t i = 0; i < 8; ++i)
	{
		bytes[i + 1] = static_cast<std::uint8_t>(name >> (8 * i));
	}
	bytes[9] = flags;
	bytes[10] = static_cast<std::uint8_t>(input);
	bytes[11] = static_cast<std::uint8_t>(input >> 8);
	bytes[12] = static_cast<std::uint8_t>(function);
	bytes[13] = static_cast<std::uint8_t>(function >> 8);
	receive_auxiliary(client, vt, bytes);
}

static void auxiliary_status(DerivedTestVTClient &client, const std::shared_ptr<ControlFunction> &source, std::uint16_t input)
{
	receive_auxiliary(client, source, { 0x26, static_cast<std::uint8_t>(input), static_cast<std::uint8_t>(input >> 8), 0x56, 0, 0x78, 0, 0 });
}

static void auxiliary_response(DerivedTestVTClient &client, const std::shared_ptr<ControlFunction> &vt, std::uint8_t error = 0)
{
	receive_auxiliary(client, vt, { 0x22, error, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
}

static void update_auxiliary_client(DerivedTestVTClient &client)
{
	client.test_wrapper_refresh_vt_status(); // Keep the unrelated VT connection alive during time advances.
	client.update();
}

static std::vector<CANMessageFrame> auxiliary_frames(VirtualCANPlugin &vt)
{
	CANHardwareInterface::update();
	std::vector<CANMessageFrame> frames;
	CANMessageFrame frame{};
	while (vt.read_frame(frame, 0))
	{
		const auto pgn = CANIdentifier(frame.identifier).get_parameter_group_number();
		if ((pgn == 0xE700) || (pgn == 0xEC00) || (pgn == 0xEB00))
		{
			frames.push_back(frame);
		}
	}
	return frames;
}

// Complete TP only to inspect a logical Preferred Assignment command, not to retest TP.
static std::vector<std::uint8_t> receive_preferred_payload(AuxiliaryTestCAN &can, test_helpers::TestTimeSource &clock)
{
	CANNetworkManager::CANNetwork.update();
	clock.update_for_ms(10);
	std::uint16_t length = 0;
	std::vector<std::uint8_t> payload;
	for (const auto &frame : auxiliary_frames(can.vt))
	{
		const auto pgn = CANIdentifier(frame.identifier).get_parameter_group_number();
		if ((pgn == 0xE700) && (frame.data[0] == 0x22))
		{
			return { frame.data, frame.data + frame.dataLength };
		}
		if ((pgn == 0xEC00) && (frame.data[0] == 0x10) && (frame.data[6] == 0xE7))
		{
			length = static_cast<std::uint16_t>(frame.data[1] | (frame.data[2] << 8));
		}
	}
	if (length == 0)
	{
		ADD_FAILURE() << "No Preferred Assignment command transmitted";
		return {};
	}
	const auto packets = static_cast<std::uint8_t>((length + 6) / 7);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  6, 0xEC00, can.ecu, can.partner, { 0x11, packets, 1, 0xFF, 0xFF, 0, 0xE7, 0 }));
	CANNetworkManager::CANNetwork.update();
	clock.update_for_ms(10);
	for (const auto &packet : auxiliary_frames(can.vt))
	{
		if (CANIdentifier(packet.identifier).get_parameter_group_number() == 0xEB00)
		{
			for (std::size_t i = 1; (i < CAN_DATA_LENGTH) && (payload.size() < length); ++i)
			{
				payload.push_back(packet.data[i]);
			}
		}
	}
	EXPECT_EQ(length, payload.size());
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  6, 0xEC00, can.ecu, can.partner, { 0x13, static_cast<std::uint8_t>(length), static_cast<std::uint8_t>(length >> 8), packets, 0xFF, 0, 0xE7, 0 }));
	CANNetworkManager::CANNetwork.update();
	return payload;
}

using PreferredRecord = std::tuple<std::uint64_t, std::uint16_t, std::uint16_t, std::uint16_t>;
static void expect_preferred_records(const std::vector<std::uint8_t> &payload, std::vector<PreferredRecord> expected, std::uint64_t optionalEmptyDevice = UINT64_MAX)
{
	ASSERT_GE(payload.size(), 2U);
	ASSERT_EQ(0x22, payload[0]);
	std::vector<PreferredRecord> records;
	std::set<std::uint64_t> names;
	std::size_t offset = 2;
	for (unsigned int device = 0; device < payload[1]; ++device)
	{
		ASSERT_LE(offset + 11, payload.size());
		std::uint64_t name = 0;
		for (int byte = 0; byte < 8; ++byte)
		{
			name |= static_cast<std::uint64_t>(payload[offset++]) << (8 * byte);
		}
		EXPECT_TRUE(names.insert(name).second);
		const auto model = static_cast<std::uint16_t>(payload[offset] | (payload[offset + 1] << 8));
		offset += 2;
		const auto count = payload[offset++];
		if (count == 0)
		{
			EXPECT_EQ(optionalEmptyDevice, name);
		}
		for (unsigned int assignment = 0; assignment < count; ++assignment)
		{
			ASSERT_LE(offset + 4, payload.size());
			const auto function = static_cast<std::uint16_t>(payload[offset] | (payload[offset + 1] << 8));
			const auto input = static_cast<std::uint16_t>(payload[offset + 2] | (payload[offset + 3] << 8));
			offset += 4;
			records.emplace_back(name, model, function, input);
		}
	}
	// Single-frame commands are padded; multi-packet commands use their logical length.
	if (offset < CAN_DATA_LENGTH)
	{
		for (; offset < payload.size(); ++offset)
		{
			EXPECT_EQ(0xFF, payload[offset]);
		}
	}
	EXPECT_EQ(payload.size(), offset);
	std::sort(records.begin(), records.end());
	std::sort(expected.begin(), expected.end());
	EXPECT_EQ(expected, records);
}

static void expect_assignment_response(AuxiliaryTestCAN &can, std::uint16_t function, std::uint8_t error)
{
	const auto frames = auxiliary_frames(can.vt);
	const auto response = std::find_if(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700 && frame.data[0] == 0x24;
	});
	ASSERT_NE(frames.end(), response);
	const CANIdentifier id(response->identifier);
	EXPECT_EQ(can.ecu->get_address(), id.get_source_address());
	EXPECT_EQ(can.partner->get_address(), id.get_destination_address());
	ASSERT_EQ(CAN_DATA_LENGTH, response->dataLength);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0x24, static_cast<std::uint8_t>(function), static_cast<std::uint8_t>(function >> 8), error, 0xFF, 0xFF, 0xFF, 0xFF }),
	          (std::vector<std::uint8_t>(response->data, response->data + response->dataLength)));
}

TEST_F(VirtualTerminalTest, AuxiliaryConfigurationIsPreInitializationOnly)
{
	AuxiliaryTestCAN can(time_source);
	AuxiliaryPreferences preferences;
	DerivedTestVTClient client(can.partner, can.ecu);
	EXPECT_FALSE(client.test_wrapper_auxiliary_enabled());
	client.set_auxiliary_functions_enabled(true);
	EXPECT_TRUE(client.test_wrapper_auxiliary_enabled());
	client.set_auxiliary_functions_enabled(false);
	client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, nullptr, &preferences);
	EXPECT_TRUE(client.test_wrapper_auxiliary_enabled());
	const auto device = auxiliary_device(0x8123456789ABCDEF, 0x21);
	preferences.byName[device->get_NAME().get_full_name()] = { { 0x0101, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } };
	client.initialize(false);
	client.set_auxiliary_functions_enabled(false);
	client.set_auxiliary_assignment_callbacks(nullptr, nullptr, nullptr);
	EXPECT_TRUE(client.test_wrapper_auxiliary_enabled());
	auxiliary_maintenance(client, device, 0x3344);
	client.test_wrapper_set_vt_version(3);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::SendAuxiliaryPreferredAssignment);
	update_auxiliary_client(client);
	expect_preferred_records(receive_preferred_payload(can, time_source), { { device->get_NAME().get_full_name(), 0x3344, 0x0101, 0x1234 } });
}

TEST_F(VirtualTerminalTest, PreferredAssignmentStartupSerializesTheCompleteReadySetForVersionThree)
{
	AuxiliaryTestCAN can(time_source);
	AuxiliaryPreferences preferences;
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, nullptr, &preferences);
	client.test_wrapper_set_vt_version(2);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::SendAuxiliaryPreferredAssignment);
	update_auxiliary_client(client);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());
	client.test_wrapper_set_vt_version(3);
	const auto pool = auxiliary_function_pool({ 0x0101 }); // Provides a function with no stored preferences.
	client.set_object_pool(0, pool.data(), static_cast<std::uint32_t>(pool.size()));
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForEndOfObjectPoolResponse);
	receive_auxiliary(client, can.partner, { 0x12, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0, 0xFF });
	update_auxiliary_client(client);
	expect_preferred_records(receive_preferred_payload(can, time_source), {});
	auxiliary_response(client, can.partner);

	const auto first = auxiliary_device(0x8123456789ABCDEF, 0x21);
	const auto second = auxiliary_device(0x923456789ABCDEF0, 0x22);
	const auto empty = auxiliary_device(0xA3456789ABCDEF01, 0x23);
	preferences.byName[first->get_NAME().get_full_name()] = { { 0x1023, 0x4567, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } };
	preferences.byName[second->get_NAME().get_full_name()] = { { 0x89AB, 0xCDEF, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } };
	auxiliary_maintenance(client, first, 0x1357);
	auxiliary_maintenance(client, second, 0x2468, 0);
	auxiliary_maintenance(client, empty, 0x9ABC);
	update_auxiliary_client(client);
	expect_preferred_records(receive_preferred_payload(can, time_source), { { first->get_NAME().get_full_name(), 0x1357, 0x1023, 0x4567 } }, empty->get_NAME().get_full_name());
	auxiliary_response(client, can.partner);
	auxiliary_maintenance(client, second, 0x2468);
	update_auxiliary_client(client);
	expect_preferred_records(receive_preferred_payload(can, time_source), { { first->get_NAME().get_full_name(), 0x1357, 0x1023, 0x4567 }, { second->get_NAME().get_full_name(), 0x2468, 0x89AB, 0xCDEF } }, empty->get_NAME().get_full_name());
}

TEST_F(VirtualTerminalTest, PreferredAssignmentWaitsForPartnerResponseAndRetriesThreeTransmittedAttempts)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.test_wrapper_set_vt_version(3);
	std::vector<VirtualTerminalClient::AuxiliaryAssignmentFailureEvent> failures;
	client.get_auxiliary_assignment_failure_event_dispatcher().add_listener([&](const auto &event) { failures.push_back(event); });
	const auto start = [&]() {
		client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::SendAuxiliaryPreferredAssignment);
		update_auxiliary_client(client);
		expect_preferred_records(receive_preferred_payload(can, time_source), {});
	};
	start();
	auxiliary_response(client, auxiliary_device(99, 0x22));
	update_auxiliary_client(client);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());
	EXPECT_FALSE(client.get_is_connected());
	auxiliary_response(client, can.partner);
	EXPECT_TRUE(client.get_is_connected());
	EXPECT_TRUE(failures.empty());
	update_auxiliary_client(client);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());

	start();
	auxiliary_response(client, can.partner, 1);
	EXPECT_FALSE(failures.empty());
	failures.clear();
	start();
	// Capture three commands on the bus; an unanswered third command reports failure.
	for (int attempt = 1; attempt < 3; ++attempt)
	{
		time_source.update_for_ms(1989); // Payload capture advanced the test clock by 10 ms.
		update_auxiliary_client(client);
		EXPECT_TRUE(auxiliary_frames(can.vt).empty());
		time_source.update_for_ms(1);
		update_auxiliary_client(client);
		expect_preferred_records(receive_preferred_payload(can, time_source), {});
		EXPECT_TRUE(failures.empty());
	}
	time_source.update_for_ms(1990);
	update_auxiliary_client(client);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());
	EXPECT_FALSE(failures.empty());
	time_source.update_for_ms(2000);
	update_auxiliary_client(client);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());
	// With no local CF, no attempt can reach CAN. Even repeated send failures
	// must not trigger the operator failure indication reserved for failed transactions.
	DerivedTestVTClient unsendable(can.partner, nullptr);
	unsendable.set_auxiliary_functions_enabled(true);
	unsendable.test_wrapper_set_vt_version(3);
	unsendable.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::SendAuxiliaryPreferredAssignment);
	bool sendFailureReported = false;
	unsendable.get_auxiliary_assignment_failure_event_dispatcher().add_listener([&](const auto &) { sendFailureReported = true; });
	for (int retry = 0; retry < 4; ++retry)
	{
		update_auxiliary_client(unsendable);
		time_source.update_for_ms(2000);
	}
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());
	EXPECT_FALSE(sendFailureReported);
}

TEST_F(VirtualTerminalTest, AssignmentsAcknowledgeMappingsAndPreservePreferencesAcrossTemporaryChanges)
{
	AuxiliaryTestCAN can(time_source);
	AuxiliaryPreferences preferences;
	DerivedTestVTClient client(can.partner, can.ecu);
	DerivedTestVTClient::staticTestPool.assign(34, 0); // Graphics Context before the functions.
	DerivedTestVTClient::staticTestPool[2] = 36;
	DerivedTestVTClient::staticTestPool.insert(DerivedTestVTClient::staticTestPool.end(), { 0, 0x70, 33, 0, 0xFF, 0xFF }); // AUX Designator.
	DerivedTestVTClient::staticTestPool.insert(DerivedTestVTClient::staticTestPool.end(), { 1, 0x70, 1, 0, 0xFF, 0xFF, 0, 1, 0xFF, 2, 3, 0x70, 2, 0x70, 28, 0, 0 }); // Data Mask with an extended macro reference, then its Macro.
	DerivedTestVTClient::staticTestPool.insert(DerivedTestVTClient::staticTestPool.end(), { 3, 0x70, 38, 0, 1, 0, 1, 0, 0, 0xFF, 0xFF }); // One extended character range.
	const auto functions = auxiliary_function_pool({ 0x0101, 0x0102 });
	DerivedTestVTClient::staticTestPool.insert(DerivedTestVTClient::staticTestPool.end(), functions.begin(), functions.end());
	client.register_object_pool_data_chunk_callback(0, static_cast<std::uint32_t>(DerivedTestVTClient::staticTestPool.size()), [](auto index, auto offset, auto size, auto *buffer, void *context) {
		return size <= 38 && DerivedTestVTClient::testWrapperDataChunkCallback(index, offset, size, buffer, context);
	});
	client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, store_auxiliary_preferences, &preferences);
	client.test_wrapper_set_vt_version(6);
	const auto a = auxiliary_device(101, 0x21), b = auxiliary_device(102, 0x22);
	auxiliary_maintenance(client, a, 0x3344);
	auxiliary_maintenance(client, b, 0x5566);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::SendAuxiliaryPreferredAssignment);
	update_auxiliary_client(client);
	expect_preferred_records(receive_preferred_payload(can, time_source), {});
	auxiliary_response(client, can.partner);
	DerivedTestVTClient::staticTestPool.clear(); // Runtime validation must retain metadata, not borrow upload storage.
	std::vector<std::uint16_t> events;
	client.get_auxiliary_function_event_dispatcher().add_listener([&](const auto &event) { events.push_back(event.function.functionObjectID); });
	const auto assign = [&](std::uint64_t name, std::uint16_t function, std::uint16_t input, std::uint8_t flags, std::uint8_t error = 0) {
		auxiliary_assignment(client, can.partner, name, function, input, flags);
		expect_assignment_response(can, function, error);
		update_auxiliary_client(client); // Allow the application to commit preferences; no ordering/timing contract.
	};
	auxiliary_maintenance(client, auxiliary_device(103, 0x23), 0x7788); // Not yet loaded; must not block assignment.
	assign(101, 0x0101, 0x1111, 0x04);
	assign(102, 0x0101, 0x2222, 0x84); // Temporary reassignment does not replace the preference.
	auxiliary_status(client, a, 0x1111);
	EXPECT_TRUE(events.empty());
	auxiliary_status(client, b, 0x2222);
	EXPECT_EQ((std::vector<std::uint16_t>{ 0x0101 }), events);
	ASSERT_EQ(1U, preferences.byName[101].size());
	EXPECT_EQ(0x1111, preferences.byName[101][0].inputObjectID);
	assign(102, 0x0101, 0x2222, 0x04); // Persistent reassignment transfers preferred ownership too.
	EXPECT_TRUE(preferences.byName[101].empty());
	ASSERT_EQ(1U, preferences.byName[102].size());
	EXPECT_EQ(0x0101, preferences.byName[102][0].functionObjectID);
	assign(UINT64_MAX, 0x0101, 0xFFFF, 0x84, 1); // Malformed removal.
	EXPECT_EQ(1U, preferences.byName[102].size());
	assign(102, 0x9999, 0x2222, 0x04, 1); // Nonexistent function.
	assign(102, 0x0101, 0x2222, 0x02, 1); // Incompatible function type.
	assign(999, 0x0101, 0x1234, 0x04, 1); // Rejected reassignment leaves the working mapping intact.
	events.clear();
	auxiliary_status(client, b, 0x2222);
	EXPECT_EQ((std::vector<std::uint16_t>{ 0x0101 }), events);
	assign(UINT64_MAX, 0x0101, 0xFFFF, 0x9F); // Remove one without storing.
	events.clear();
	auxiliary_status(client, b, 0x2222);
	EXPECT_TRUE(events.empty());
	EXPECT_EQ(1U, preferences.byName[102].size());
	assign(102, 0x0102, 0x2222, 0x04);
	assign(UINT64_MAX, 0xFFFF, 0xFFFF, 0x1F); // Remove all and store the empty preference set.
	events.clear();
	auxiliary_status(client, b, 0x2222);
	EXPECT_TRUE(events.empty());
	EXPECT_TRUE(preferences.byName[101].empty());
	EXPECT_TRUE(preferences.byName[102].empty());
}

TEST_F(VirtualTerminalTest, IdenticalAssignmentsSetOnlyTheLegacyAlreadyAssignedBit)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	const auto pool = auxiliary_function_pool({ 0x0103, 0x0104, 0x0105, 0x0106 });
	client.set_object_pool(0, &pool);
	client.set_auxiliary_functions_enabled(true);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::SendAuxiliaryPreferredAssignment);
	const auto device = auxiliary_device(201, 0x21);
	auxiliary_maintenance(client, device, 0x3344);
	for (const std::uint8_t version : { 3, 4, 5, 6 })
	{
		SCOPED_TRACE(version);
		client.test_wrapper_set_vt_version(version);
		const auto function = static_cast<std::uint16_t>(0x0100 + version);
		auxiliary_assignment(client, can.partner, 201, function, 0x1234);
		expect_assignment_response(can, function, 0);
		auxiliary_assignment(client, can.partner, 201, function, 0x1234);
		expect_assignment_response(can, function, version <= 5 ? 2 : 0);
	}
}

TEST_F(VirtualTerminalTest, AuxiliaryStatusUsesSourceNameAndDispatchesEveryMatchingFunction)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	const auto pool = auxiliary_function_pool({ 0x0101, 0x0102, 0x0201 });
	client.set_object_pool(0, &pool);
	client.set_auxiliary_functions_enabled(true);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::SendAuxiliaryPreferredAssignment);
	const auto a = auxiliary_device(301, 0x21), b = auxiliary_device(302, 0x22);
	auxiliary_maintenance(client, a, 0x3344);
	auxiliary_maintenance(client, b, 0x3344);
	auxiliary_assignment(client, can.partner, 301, 0x0101, 0x1234);
	auxiliary_assignment(client, can.partner, 301, 0x0102, 0x1234);
	auxiliary_assignment(client, can.partner, 302, 0x0201, 0x1234);
	std::vector<std::uint16_t> events;
	client.get_auxiliary_function_event_dispatcher().add_listener([&](const auto &event) {
		events.push_back(event.function.functionObjectID);
		EXPECT_EQ(0x56, event.value1);
		EXPECT_EQ(0x78, event.value2);
	});
	auxiliary_status(client, a, 0x1234);
	std::sort(events.begin(), events.end());
	EXPECT_EQ((std::vector<std::uint16_t>{ 0x0101, 0x0102 }), events);
	events.clear();
	auxiliary_status(client, b, 0x1234);
	EXPECT_EQ((std::vector<std::uint16_t>{ 0x0201 }), events);
	events.clear();
	auxiliary_status(client, auxiliary_device(999, 0x23), 0x1234);
	EXPECT_TRUE(events.empty());
	receive_auxiliary(client, can.partner, { 0xFE, 0x37, 0xFF, 0xFF, 0xFF, 0xFF, 0x40, 0xFF });
	auxiliary_status(client, a, 0x1234);
	EXPECT_TRUE(events.empty()); // J.8: partnered VT learn mode suppresses function events.
	receive_auxiliary(client, can.partner, { 0xFE, 0x37, 0xFF, 0xFF, 0xFF, 0xFF, 0, 0xFF });
	auxiliary_status(client, a, 0x1234);
	EXPECT_EQ(2U, events.size());
}

TEST_F(VirtualTerminalTest, MaintenanceAvailabilityClearsOnlyAffectedMappingsAndRetainsPreferences)
{
	AuxiliaryTestCAN can(time_source);
	AuxiliaryPreferences preferences;
	DerivedTestVTClient client(can.partner, can.ecu);
	const auto pool = auxiliary_function_pool({ 0x0101, 0x0102, 0x0201 });
	client.set_object_pool(0, &pool);
	client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, store_auxiliary_preferences, &preferences);
	client.test_wrapper_set_vt_version(3);
	const auto a = auxiliary_device(401, 0x21), b = auxiliary_device(402, 0x22);
	auxiliary_maintenance(client, a, 0x3344, 0xFF);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::SendAuxiliaryPreferredAssignment);
	update_auxiliary_client(client);
	expect_preferred_records(receive_preferred_payload(can, time_source), {});
	auxiliary_response(client, can.partner);
	auxiliary_assignment(client, can.partner, 401, 0x0101, 0x1234, 0x04);
	expect_assignment_response(can, 0x0101, 1); // Reserved Ready value does not make the pool assignable.
	auxiliary_maintenance(client, a, 0x3344);
	auxiliary_maintenance(client, b, 0x3344);
	update_auxiliary_client(client); // Load newly discovered preferences before persistent assignment.
	auxiliary_assignment(client, can.partner, 401, 0x0101, 0x1234, 0x04);
	auxiliary_assignment(client, can.partner, 401, 0x0102, 0x1234, 0x04);
	auxiliary_assignment(client, can.partner, 402, 0x0201, 0x1234, 0x04);
	update_auxiliary_client(client);
	std::vector<std::uint16_t> events;
	client.get_auxiliary_function_event_dispatcher().add_listener([&](const auto &event) { events.push_back(event.function.functionObjectID); });
	const auto expectFunctions = [&](const std::shared_ptr<ControlFunction> &device, std::vector<std::uint16_t> expected) {
		events.clear();
		auxiliary_status(client, device, 0x1234);
		std::sort(events.begin(), events.end());
		EXPECT_EQ(expected, events);
	};
	expectFunctions(a, { 0x0101, 0x0102 });
	expectFunctions(b, { 0x0201 });
	time_source.update_for_ms(299);
	auxiliary_maintenance(client, b, 0x3344);
	update_auxiliary_client(client);
	expectFunctions(a, { 0x0101, 0x0102 });
	time_source.update_for_ms(1);
	update_auxiliary_client(client);
	expectFunctions(a, {});
	expectFunctions(b, { 0x0201 });
	ASSERT_EQ(2U, preferences.byName[401].size());

	auxiliary_maintenance(client, b, 0x3344, 0);
	expectFunctions(b, {});
	auxiliary_maintenance(client, b, 0x3344);
	auxiliary_assignment(client, can.partner, 402, 0x0201, 0x1234);
	expectFunctions(b, { 0x0201 });
	auxiliary_maintenance(client, b, 0x5566); // Same NAME with a different model also invalidates the mapping.
	expectFunctions(b, {});
	update_auxiliary_client(client);
	ASSERT_EQ(1U, preferences.byName[402].size());
	EXPECT_EQ(0x0201, preferences.byName[402][0].functionObjectID);
}

TEST_F(VirtualTerminalTest, PreferredAssignmentResynchronizesCompleteChangesAfterAcknowledgement)
{
	AuxiliaryTestCAN can(time_source);
	AuxiliaryPreferences preferences;
	DerivedTestVTClient client(can.partner, can.ecu);
	const auto pool = auxiliary_function_pool({ 0x0101, 0x0201 });
	client.set_object_pool(0, &pool);
	client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, store_auxiliary_preferences, &preferences);
	client.test_wrapper_set_vt_version(3);
	const auto a = auxiliary_device(501, 0x21), b = auxiliary_device(502, 0x22), unrelated = auxiliary_device(503, 0x23);
	preferences.byName[501] = { { 0x0101, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } };
	preferences.byName[502] = { { 0x0201, 0x5678, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } };
	auxiliary_maintenance(client, a, 0x3344);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::SendAuxiliaryPreferredAssignment);
	update_auxiliary_client(client);
	const std::vector<PreferredRecord> first = { { 501, 0x3344, 0x0101, 0x1234 } };
	expect_preferred_records(receive_preferred_payload(can, time_source), first);
	bool failed = false;
	client.get_auxiliary_assignment_failure_event_dispatcher().add_listener([&](const auto &) { failed = true; });
	preferences.byName[503] = { { 0xFFFF, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } };
	auxiliary_maintenance(client, unrelated, 0x7788); // Bad unrelated stored data must not cancel an outstanding command.
	auxiliary_maintenance(client, b, 0x5566);
	update_auxiliary_client(client);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty()); // Wait for the current transaction's acknowledgement.
	EXPECT_FALSE(failed);
	preferences.byName[503].clear();
	auxiliary_response(client, can.partner);
	update_auxiliary_client(client);
	const std::vector<PreferredRecord> complete = { { 501, 0x3344, 0x0101, 0x1234 }, { 502, 0x5566, 0x0201, 0x5678 } };
	expect_preferred_records(receive_preferred_payload(can, time_source), complete);
	auxiliary_response(client, can.partner);
	auxiliary_maintenance(client, unrelated, 0x7788);
	update_auxiliary_client(client);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());
	auxiliary_maintenance(client, a, 0x3344, 0);
	auxiliary_maintenance(client, a, 0x3344);
	update_auxiliary_client(client);
	expect_preferred_records(receive_preferred_payload(can, time_source), complete);
	// Change a preference while this complete-set transaction awaits its response.
	auxiliary_assignment(client, can.partner, 501, 0x0101, 0x9ABC, 0x04);
	expect_assignment_response(can, 0x0101, 0);
	update_auxiliary_client(client);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());
	auxiliary_response(client, can.partner);
	update_auxiliary_client(client);
	expect_preferred_records(receive_preferred_payload(can, time_source), { { 501, 0x3344, 0x0101, 0x9ABC }, { 502, 0x5566, 0x0201, 0x5678 } });
	auxiliary_response(client, can.partner);
	auxiliary_maintenance(client, a, 0x3344, 0);
	auxiliary_maintenance(client, b, 0x5566, 0);
	update_auxiliary_client(client);
	expect_preferred_records(receive_preferred_payload(can, time_source), {}); // Complete set after both preferred units disappear.
}

TEST_F(VirtualTerminalTest, VTStatusMessage)
{
	NAME clientNAME(0);
	auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(clientNAME, 0, 0x26);

	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	vtNameFilters.push_back(testFilter);

	auto vtPartner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);

	EXPECT_EQ(NULL_OBJECT_ID, clientUnderTest.get_visible_data_mask());
	EXPECT_EQ(NULL_OBJECT_ID, clientUnderTest.get_visible_soft_key_mask());

	CANIdentifier identifier(CANIdentifier::Type::Extended, static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), CANIdentifier::CANPriority::PriorityDefault6, 0, 0);
	CANMessage testMessage(CANMessage::Type::Receive,
	                       identifier,
	                       {
	                         0xFE, // VT Status message function code
	                         0x26, // Working set master address
	                         1234 & 0xFF, // Data mask active
	                         1234 >> 8, // Data mask active
	                         4567 & 0xFF, // Soft key mask active
	                         4567 >> 8, // Soft key mask active
	                         0xFF, // Busy codes
	                         1, // VT Function code that is being executed
	                       },
	                       vtPartner,
	                       internalECU,
	                       0);

	clientUnderTest.test_wrapper_process_rx_message(testMessage, &clientUnderTest);

	EXPECT_EQ(1234, clientUnderTest.get_visible_data_mask());
	EXPECT_EQ(4567, clientUnderTest.get_visible_soft_key_mask());
	EXPECT_EQ(0xFE, clientUnderTest.get_active_working_set_master_address()); // Expect null address since not in the connected state

	// Test the master address is correct when in the connected state
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	EXPECT_EQ(0x26, clientUnderTest.get_active_working_set_master_address());

	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, FullPoolAutoscalingWithVector)
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

	auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(clientNAME, 0, 0x26);

	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	vtNameFilters.push_back(testFilter);

	auto vtPartner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);

	// Actual tests start here
	std::vector<std::uint8_t> testPool = isobus::IOPFileInterface::read_iop_file("../../examples/virtual_terminal/version3_object_pool/VT3TestPool.iop");

	if (0 == testPool.size())
	{
		// Try a different path to mitigate differences between how IDEs run the unit test
		testPool = isobus::IOPFileInterface::read_iop_file("../examples/virtual_terminal/version3_object_pool/VT3TestPool.iop");
	}

	EXPECT_NE(0, testPool.size());

	clientUnderTest.set_object_pool(0, &testPool);

	EXPECT_EQ(false, clientUnderTest.test_wrapper_get_any_pool_needs_scaling());

	// Test invalid soft key width
	clientUnderTest.set_object_pool_scaling(0, 64, 0);

	EXPECT_EQ(false, clientUnderTest.test_wrapper_get_any_pool_needs_scaling());

	clientUnderTest.set_object_pool_scaling(0, 240, 240);

	// Check functionality of get_any_pool_needs_scaling
	EXPECT_EQ(true, clientUnderTest.test_wrapper_get_any_pool_needs_scaling());

	// Full scaling test using the example pool
	EXPECT_EQ(true, clientUnderTest.test_wrapper_scale_object_pools());

	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, FullPoolAutoscalingWithDataChunkCallbacks)
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

	auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(clientNAME, 0, 0x26);

	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	vtNameFilters.push_back(testFilter);

	auto vtPartner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);

	// Actual tests start here
	DerivedTestVTClient::staticTestPool = isobus::IOPFileInterface::read_iop_file("../../examples/virtual_terminal/version3_object_pool/VT3TestPool.iop");

	if (0 == DerivedTestVTClient::staticTestPool.size())
	{
		// Try a different path to mitigate differences between how IDEs run the unit test
		DerivedTestVTClient::staticTestPool = isobus::IOPFileInterface::read_iop_file("../examples/virtual_terminal/version3_object_pool/VT3TestPool.iop");
	}

	EXPECT_NE(0, DerivedTestVTClient::staticTestPool.size());

	clientUnderTest.register_object_pool_data_chunk_callback(0, DerivedTestVTClient::staticTestPool.size(), DerivedTestVTClient::testWrapperDataChunkCallback);

	clientUnderTest.set_object_pool_scaling(0, 240, 240);

	// Check functionality of get_any_pool_needs_scaling
	EXPECT_EQ(true, clientUnderTest.test_wrapper_get_any_pool_needs_scaling());

	// Full scaling test using the example pool
	EXPECT_EQ(true, clientUnderTest.test_wrapper_scale_object_pools());

	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, FullPoolAutoscalingWithPointer)
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

	auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(clientNAME, 0, 0x26);

	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	vtNameFilters.push_back(testFilter);

	auto vtPartner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);

	// Actual tests start here
	std::vector<std::uint8_t> testPool = isobus::IOPFileInterface::read_iop_file("../../examples/virtual_terminal/version3_object_pool/VT3TestPool.iop");

	if (0 == testPool.size())
	{
		// Try a different path to mitigate differences between how IDEs run the unit test
		testPool = isobus::IOPFileInterface::read_iop_file("../examples/virtual_terminal/version3_object_pool/VT3TestPool.iop");
	}

	EXPECT_NE(0, testPool.size());

	clientUnderTest.set_object_pool(0, testPool.data(), testPool.size());

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

	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, ObjectMetadataTests)
{
	NAME clientNAME(0);
	auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(clientNAME, 0, 0x26);

	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	vtNameFilters.push_back(testFilter);

	auto vtPartner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

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

	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, FontRemapping)
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
	clientUnderTest.test_wrapper_set_supported_fonts(0x55, 0x55); // 0x55 = 01010101

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

	clientUnderTest.test_wrapper_set_supported_fonts(0xAA, 0xAA); // 0xAA = 10101010
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

TEST_F(VirtualTerminalTest, ResizeOutputArchedBarGraph)
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

TEST_F(VirtualTerminalTest, ResizeOutputLinearBarGraph)
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

TEST_F(VirtualTerminalTest, ResizeOutputMeter)
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

TEST_F(VirtualTerminalTest, ResizeOutputPolygon)
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

TEST_F(VirtualTerminalTest, ResizeOutputEllipse)
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

TEST_F(VirtualTerminalTest, ResizeOutputLine)
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

TEST_F(VirtualTerminalTest, ResizeOutputList)
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

TEST_F(VirtualTerminalTest, ResizeInputBoolean)
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

TEST_F(VirtualTerminalTest, TestNumberBytesInInvalidObjects)
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

TEST_F(VirtualTerminalTest, MessageConstruction)
{
	VirtualCANPlugin serverVT;
	serverVT.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);

	DerivedTestVTClient interfaceUnderTest(vtPartner, internalECU);
	interfaceUnderTest.initialize(false);

	time_source.update_for_ms(50);

	// Get the virtual CAN plugin back to a known state
	CANMessageFrame testFrame = {};
	while (!serverVT.get_queue_empty())
	{
		serverVT.read_frame(testFrame);
	}
	ASSERT_TRUE(serverVT.get_queue_empty());

	// Test send change active mask command while not connected queues the command
	ASSERT_TRUE(interfaceUnderTest.send_change_active_mask(123, 456));
	ASSERT_TRUE(serverVT.get_queue_empty());
	interfaceUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	interfaceUnderTest.test_wrapper_process_command_queue();
	time_source.update_for_ms(5);

	ASSERT_TRUE(serverVT.read_frame(testFrame));
	EXPECT_EQ(0, testFrame.channel);
	EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
	EXPECT_TRUE(testFrame.isExtendedFrame);
	EXPECT_EQ(0x14E72637, testFrame.identifier);

	EXPECT_EQ(173, testFrame.data[0]); // VT Function

	std::uint16_t workingSetObjectID = (static_cast<std::uint16_t>(testFrame.data[1]) | (static_cast<std::uint16_t>(testFrame.data[2]) << 8));
	EXPECT_EQ(123, workingSetObjectID);

	std::uint16_t newActiveMaskObjectID = (static_cast<std::uint16_t>(testFrame.data[3]) | (static_cast<std::uint16_t>(testFrame.data[4]) << 8));
	EXPECT_EQ(456, newActiveMaskObjectID);

	// Test send_hide_show_object, but since we have not yet sent a response to the change active mask command, it should queue the command
	ASSERT_TRUE(interfaceUnderTest.send_hide_show_object(1234, VirtualTerminalClient::HideShowObjectCommand::HideObject));
	ASSERT_TRUE(serverVT.get_queue_empty());
	interfaceUnderTest.test_wrapper_process_command_queue();
	time_source.update_for_ms(5);
	ASSERT_FALSE(serverVT.read_frame(testFrame));

	// Send a response to the change active mask command
	testFrame.identifier = 0x14E63726; // VT->ECU
	testFrame.data[0] = 173; // VT Function
	testFrame.data[1] = 123 & 0xFF;
	testFrame.data[2] = (123 >> 8) & 0xFF;
	testFrame.data[3] = 0; // Success
	testFrame.data[4] = 0xFF; // Reserved
	testFrame.data[5] = 0xFF; // Reserved
	testFrame.data[6] = 0xFF; // Reserved
	testFrame.data[7] = 0xFF; // Reserved
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	interfaceUnderTest.test_wrapper_process_command_queue();
	time_source.update_for_ms(5);

	ASSERT_TRUE(serverVT.read_frame(testFrame));
	EXPECT_EQ(0, testFrame.channel);
	EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
	EXPECT_TRUE(testFrame.isExtendedFrame);
	EXPECT_EQ(0x14E72637, testFrame.identifier);
	EXPECT_EQ(160, testFrame.data[0]); // VT function

	std::uint16_t objectID = (static_cast<std::uint16_t>(testFrame.data[1]) | (static_cast<std::uint16_t>(testFrame.data[2]) << 8));
	EXPECT_EQ(1234, objectID);
	EXPECT_EQ(0, testFrame.data[3]); // Hide
	EXPECT_EQ(0xFF, testFrame.data[4]); // Reserved
	EXPECT_EQ(0xFF, testFrame.data[5]); // Reserved
	EXPECT_EQ(0xFF, testFrame.data[6]); // Reserved
	EXPECT_EQ(0xFF, testFrame.data[7]); // Reserved

	// Send a response to the hide object command
	testFrame.identifier = 0x14E63726; // VT->ECU
	testFrame.data[0] = 160; // VT Function
	testFrame.data[1] = 1234 & 0xFF;
	testFrame.data[2] = (1234 >> 8) & 0xFF;
	testFrame.data[3] = 0; // Hide
	testFrame.data[4] = 0xFF; // Reserved
	testFrame.data[5] = 0xFF; // Reserved
	testFrame.data[6] = 0xFF; // Reserved
	testFrame.data[7] = 0xFF; // Reserved
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	ASSERT_TRUE(serverVT.get_queue_empty());
	ASSERT_TRUE(interfaceUnderTest.send_enable_disable_object(1234, VirtualTerminalClient::EnableDisableObjectCommand::DisableObject));
	time_source.update_for_ms(5);
	ASSERT_TRUE(serverVT.read_frame(testFrame));
	EXPECT_EQ(0, testFrame.channel);
	EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
	EXPECT_TRUE(testFrame.isExtendedFrame);
	EXPECT_EQ(0x14E72637, testFrame.identifier);
	EXPECT_EQ(161, testFrame.data[0]); // VT function
	objectID = (static_cast<std::uint16_t>(testFrame.data[1]) | (static_cast<std::uint16_t>(testFrame.data[2]) << 8));
	EXPECT_EQ(1234, objectID);
	EXPECT_EQ(0, testFrame.data[3]); // Disable

	// Send a response to the disable object command
	testFrame.identifier = 0x14E63726; // VT->ECU
	testFrame.data[0] = 161; // VT Function
	testFrame.data[1] = 1234 & 0xFF;
	testFrame.data[2] = (1234 >> 8) & 0xFF;
	testFrame.data[3] = 0; // Disable
	testFrame.data[4] = 0xFF; // Reserved
	testFrame.data[5] = 0xFF; // Reserved
	testFrame.data[6] = 0xFF; // Reserved
	testFrame.data[7] = 0xFF; // Reserved
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	// Test draw text
	const std::string testString = "a";
	ASSERT_TRUE(serverVT.get_queue_empty());
	ASSERT_TRUE(interfaceUnderTest.send_draw_text(123, true, 1, testString.data()));
	time_source.update_for_ms(5);
	ASSERT_TRUE(serverVT.read_frame(testFrame));
	EXPECT_EQ(0, testFrame.channel);
	EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
	EXPECT_TRUE(testFrame.isExtendedFrame);
	EXPECT_EQ(0x14E72637, testFrame.identifier);
	EXPECT_EQ(184, testFrame.data[0]); // VT function (graphics context command)
	objectID = (static_cast<std::uint16_t>(testFrame.data[1]) | (static_cast<std::uint16_t>(testFrame.data[2]) << 8));
	EXPECT_EQ(123, objectID);
	EXPECT_EQ(1, testFrame.data[4]); // Transparent
	EXPECT_EQ(1, testFrame.data[5]); // Length
	EXPECT_EQ('a', testFrame.data[6]);

	serverVT.close();
	CANHardwareInterface::stop();

	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}
