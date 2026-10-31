#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <thread>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"
#include "helpers/object_pool_helpers.hpp"
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

	std::uint32_t test_wrapper_get_number_bytes_in_object(std::uint8_t *buffer, std::uint32_t bufferLength)
	{
		return VirtualTerminalClient::get_number_bytes_in_object(buffer, bufferLength);
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
		VirtualTerminalClient::set_state(value);
	}

	void test_wrapper_set_vt_version(std::uint8_t version)
	{
		connectedVTVersion = version;
	}
	void test_wrapper_refresh_vt_status()
	{
		lastVTStatusTimestamp_ms = SystemTiming::get_timestamp_ms();
	}
	bool test_wrapper_send_auxiliary_maintenance() const
	{
		return send_auxiliary_input_maintenance();
	}
	bool test_wrapper_update_auxiliary_input_status(std::uint16_t objectID)
	{
		return update_auxiliary_input_status(objectID);
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

	using VirtualTerminalClient::connectedVTVersion;
	using VirtualTerminalClient::pendingExtendedVersionDeletes;
	using VirtualTerminalClient::state;
};

std::vector<std::uint8_t> DerivedTestVTClient::staticTestPool;

class VirtualTerminalTest : public AgIsoStackTestFixture
{
protected:
	static CANMessage create_vt_to_ecu_message(const std::vector<std::uint8_t> &data)
	{
		CANIdentifier identifier(CANIdentifier::Type::Extended, static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), CANIdentifier::CANPriority::PriorityDefault6, 0, 0);
		return CANMessage(CANMessage::Type::Receive, identifier, data, nullptr, nullptr, 0);
	}
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

// AUX-N protocol tests use the normal message boundary and never construct an object pool.
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

struct AuxiliaryPreferences
{
	std::map<std::uint64_t, std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction>> byName;
	unsigned int loadCalls = 0;
};

static std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction> load_auxiliary_preferences(std::uint64_t name, std::uint16_t, void *context)
{
	auto &preferences = *static_cast<AuxiliaryPreferences *>(context);
	++preferences.loadCalls;
	return preferences.byName[name];
}

static void store_auxiliary_preferences(std::uint64_t name, std::uint16_t, const std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction> &assignments, void *context)
{
	static_cast<AuxiliaryPreferences *>(context)->byName[name] = assignments;
}

static void receive_auxiliary(DerivedTestVTClient &client, const std::shared_ptr<ControlFunction> &source, const std::vector<std::uint8_t> &data)
{
	const bool broadcast = (data[0] == 0x23) || (data[0] == 0x26) || (data[0] == 0xFE);
	const auto pgn = (data[0] == 0x23) ? 0xE700 : 0xE600;
	if (broadcast)
	{
		const auto message = test_helpers::create_message_broadcast(6, pgn, source, data.data(), static_cast<std::uint32_t>(data.size()));
		client.test_wrapper_process_rx_message(message, &client);
	}
	else
	{
		auto message = test_helpers::create_message(6, pgn, client.get_internal_control_function(), source, data.data(), static_cast<std::uint32_t>(data.size()));
		client.test_wrapper_process_rx_message(message, &client);
	}
}

static void auxiliary_maintenance(DerivedTestVTClient &client, const std::shared_ptr<ControlFunction> &source, std::uint16_t model, std::uint8_t ready = 1)
{
	receive_auxiliary(client, source, { 0x23, static_cast<std::uint8_t>(model), static_cast<std::uint8_t>(model >> 8), ready, 0xFF, 0xFF, 0xFF, 0xFF });
}

static void auxiliary_assignment(DerivedTestVTClient &client, const std::shared_ptr<ControlFunction> &vt, std::uint64_t name, std::uint16_t function, std::uint16_t input, std::uint8_t flags = 0x84, std::size_t length = 16)
{
	std::vector<std::uint8_t> bytes(length, 0xFF);
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
	client.test_wrapper_refresh_vt_status();
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
		if ((pgn == 0xE600) || (pgn == 0xE700) || (pgn == 0xEC00) || (pgn == 0xEB00))
		{
			frames.push_back(frame);
		}
	}
	return frames;
}

static void auxiliary_status_enable(DerivedTestVTClient &client, const std::shared_ptr<ControlFunction> &vt, std::uint16_t input, bool enable)
{
	receive_auxiliary(client, vt, { 0x25, static_cast<std::uint8_t>(input), static_cast<std::uint8_t>(input >> 8), static_cast<std::uint8_t>(enable), 0xFF, 0xFF, 0xFF, 0xFF });
}

static void auxiliary_vt_status(DerivedTestVTClient &client, const std::shared_ptr<ControlFunction> &vt, std::uint8_t busyCodes)
{
	receive_auxiliary(client, vt, { 0xFE, 0x26, 0xFF, 0xFF, 0xFF, 0xFF, busyCodes, 0x01 });
}

static std::vector<std::uint8_t> receive_preferred_payload(AuxiliaryTestCAN &can, test_helpers::TestTimeSource &clock)
{
	CANNetworkManager::CANNetwork.update();
	clock.update_for_ms(10);
	for (const auto &frame : auxiliary_frames(can.vt))
	{
		if ((CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700) && (frame.data[0] == 0x22))
		{
			return { frame.data, frame.data + frame.dataLength };
		}
		if ((CANIdentifier(frame.identifier).get_parameter_group_number() == 0xEC00) &&
		    (frame.data[0] == 0x10) && (frame.data[5] == 0) && (frame.data[6] == 0xE7) && (frame.data[7] == 0))
		{
			const std::size_t length = frame.data[1] | (frame.data[2] << 8);
			const auto packets = frame.data[3];
			CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
			  6, 0xEC00, can.ecu, can.partner, { 0x11, packets, 1, 0xFF, 0xFF, 0, 0xE7, 0 }));
			clock.update_for_ms(10);
			CANNetworkManager::CANNetwork.update();
			std::vector<std::uint8_t> payload;
			for (const auto &packet : auxiliary_frames(can.vt))
			{
				if (CANIdentifier(packet.identifier).get_parameter_group_number() == 0xEB00)
				{
					EXPECT_EQ(payload.size() / 7 + 1, packet.data[0]);
					payload.insert(payload.end(), packet.data + 1, packet.data + 8);
				}
			}
			EXPECT_EQ(static_cast<std::size_t>(packets) * 7, payload.size());
			if (payload.size() < length)
			{
				ADD_FAILURE() << "Incomplete Preferred Assignment transport";
				return {};
			}
			payload.resize(length);
			CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
			  6, 0xEC00, can.ecu, can.partner, { 0x13, frame.data[1], frame.data[2], packets, 0xFF, 0, 0xE7, 0 }));
			clock.update_for_ms(10);
			CANNetworkManager::CANNetwork.update();
			return payload;
		}
	}
	ADD_FAILURE() << "No Preferred Assignment command transmitted";
	return {};
}

static void expect_empty_preferred_assignment(const std::vector<std::uint8_t> &payload)
{
	ASSERT_EQ(CAN_DATA_LENGTH, payload.size());
	EXPECT_EQ(0x22, payload[0]);
	EXPECT_EQ(0, payload[1]);
	for (std::size_t i = 2; i < payload.size(); ++i)
	{
		EXPECT_EQ(0xFF, payload[i]);
	}
}

static void expect_assignment_response(AuxiliaryTestCAN &can, std::uint16_t function, std::uint8_t error)
{
	const auto frames = auxiliary_frames(can.vt);
	const auto response = std::find_if(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700 && frame.data[0] == 0x24;
	});
	ASSERT_NE(frames.end(), response);
	EXPECT_EQ((std::vector<std::uint8_t>{ 0x24, static_cast<std::uint8_t>(function), static_cast<std::uint8_t>(function >> 8), error, 0xFF, 0xFF, 0xFF, 0xFF }),
	          (std::vector<std::uint8_t>(response->data, response->data + response->dataLength)));
}

TEST_F(VirtualTerminalTest, ConcurrentTerminate)
{
	NAME clientNAME(0);
	clientNAME.set_ecu_instance(2);
	auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(clientNAME, 0, 0x27);

	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	vtNameFilters.push_back(testFilter);

	auto vtPartner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

	// The owner terminates the client while another thread does too. Joining the worker twice hangs,
	// or aborts, within a few rounds.
	for (int round = 0; round < 20; round++)
	{
		DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
		clientUnderTest.initialize(true);

		std::thread other([&clientUnderTest]() { clientUnderTest.terminate(); });
		clientUnderTest.terminate();
		other.join();
		EXPECT_FALSE(clientUnderTest.get_is_initialized());
	}

	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, PopulatedPreferredAssignmentsIncludePacketPadding)
{
	for (const bool twoFunctions : { false, true })
	{
		SCOPED_TRACE(twoFunctions ? "two functions" : "one function");
		AuxiliaryTestCAN can(time_source);
		AuxiliaryPreferences preferences;
		const std::uint64_t name = 0x0807060504030201;
		preferences.byName[name] = { { 0x0101, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } };
		if (twoFunctions)
		{
			preferences.byName[name].push_back({ 0x0102, 0x5678, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary });
		}
		DerivedTestVTClient client(can.partner, can.ecu);
		client.set_auxiliary_functions_enabled(true);
		client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, store_auxiliary_preferences, &preferences);
		client.test_wrapper_set_vt_version(6);
		client.initialize(false);
		client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
		auxiliary_maintenance(client, auxiliary_device(name, 0x21), 0x3344);
		update_auxiliary_client(client);
		std::vector<std::uint8_t> expected = { 0x22, 1, 1, 2, 3, 4, 5, 6, 7, 8, 0x44, 0x33, static_cast<std::uint8_t>(twoFunctions ? 2 : 1), 1, 1, 0x34, 0x12 };
		if (twoFunctions)
		{
			expected.insert(expected.end(), { 2, 1, 0x78, 0x56 });
		}
		expected.resize(24, 0xFF);
		EXPECT_EQ(expected, receive_preferred_payload(can, time_source));
	}
}

TEST_F(VirtualTerminalTest, AuxiliaryStatusRetriesAfterTransmitQueueFull)
{
	for (const bool learnMode : { false, true })
	{
		SCOPED_TRACE(learnMode);
		AuxiliaryTestCAN can(time_source);
		DerivedTestVTClient client(can.partner, can.ecu);
		client.add_auxiliary_input_object_id(0x1234);
		client.initialize(false);
		client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
		auxiliary_status_enable(client, can.partner, 0x1234, true);
		auxiliary_vt_status(client, can.partner, learnMode ? 0x40 : 0);
		auxiliary_frames(can.vt);

		time_source.simulate_delay_ms(50);
		client.update_auxiliary_input(0x1234, 1, 2, true);
		auxiliary_frames(can.vt);
		time_source.simulate_delay_ms(50);

		// Fill the queue without servicing the hardware, then attempt an interaction status.
		CANMessageFrame filler{};
		std::size_t queued = 0;
		while ((queued < 100) && CANHardwareInterface::transmit_can_frame(filler))
		{
			++queued;
		}
		ASSERT_GT(queued, 0);
		ASSERT_LT(queued, 100);
		client.update_auxiliary_input(0x1234, 3, 4, true);
		EXPECT_FALSE(client.test_wrapper_update_auxiliary_input_status(0x1234));
		auxiliary_frames(can.vt); // Free the queue without advancing time.

		ASSERT_TRUE(client.test_wrapper_update_auxiliary_input_status(0x1234));
		const auto frames = auxiliary_frames(can.vt);
		ASSERT_EQ(1, frames.size());
		EXPECT_EQ(learnMode ? 0xE700 : 0xE600, CANIdentifier(frames.front().identifier).get_parameter_group_number());
		EXPECT_EQ(0x26, frames.front().data[0]);
		EXPECT_EQ(3, frames.front().data[3]);
		EXPECT_EQ(4, frames.front().data[5]);
		EXPECT_EQ(learnMode ? 0x0F : 0x0C, frames.front().data[7]);

		// A successful retry starts the existing 50 ms minimum interval.
		client.update_auxiliary_input(0x1234, 5, 6, true);
		time_source.simulate_delay_ms(49);
		EXPECT_FALSE(client.test_wrapper_update_auxiliary_input_status(0x1234));
		EXPECT_TRUE(auxiliary_frames(can.vt).empty());
		time_source.simulate_delay_ms(1);
		EXPECT_TRUE(client.test_wrapper_update_auxiliary_input_status(0x1234));
		auxiliary_frames(can.vt);

		// Periodic status no longer reports the successfully submitted interaction.
		time_source.simulate_delay_ms(1000);
		EXPECT_TRUE(client.test_wrapper_update_auxiliary_input_status(0x1234));
		const auto periodicFrames = auxiliary_frames(can.vt);
		ASSERT_EQ(1, periodicFrames.size());
		EXPECT_EQ(learnMode ? 0x05 : 0x04, periodicFrames.front().data[7]);
	}
}

TEST_F(VirtualTerminalTest, InputStatusEnableDisableAllAndLearnModeTiming)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	client.add_auxiliary_input_object_id(0x1234);
	client.add_auxiliary_input_object_id(0x1235);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	time_source.update_for_ms(100);
	update_auxiliary_client(client);
	auto frames = auxiliary_frames(can.vt);
	EXPECT_FALSE(std::any_of(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE600 && frame.data[0] == 0x26;
	}));

	auxiliary_status_enable(client, can.partner, 0x1234, true);
	auxiliary_status_enable(client, can.partner, 0x1235, true);
	frames = auxiliary_frames(can.vt);
	auto response = std::find_if(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700 && frame.data[0] == 0x25;
	});
	ASSERT_NE(frames.end(), response);
	EXPECT_EQ(1, response->data[3]);
	EXPECT_EQ(0, response->data[4]);
	client.update_auxiliary_input(0x1234, 1, 2);
	frames = auxiliary_frames(can.vt);
	EXPECT_TRUE(std::any_of(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE600 && frame.data[0] == 0x26;
	}));
	auxiliary_status_enable(client, can.partner, 0x1234, false);
	client.update_auxiliary_input(0x1234, 2, 3);
	frames = auxiliary_frames(can.vt);
	EXPECT_FALSE(std::any_of(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE600 && frame.data[0] == 0x26;
	}));
	client.update_auxiliary_input(0x1235, 2, 3);
	frames = auxiliary_frames(can.vt);
	EXPECT_TRUE(std::any_of(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE600 && frame.data[0] == 0x26;
	}));

	auxiliary_status_enable(client, can.partner, 0xFFFF, false);
	frames = auxiliary_frames(can.vt);
	response = std::find_if(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700 && frame.data[0] == 0x25;
	});
	ASSERT_NE(frames.end(), response);
	EXPECT_EQ(0xFF, response->data[1]);
	EXPECT_EQ(0xFF, response->data[2]);
	EXPECT_EQ(0, response->data[3]);
	EXPECT_EQ(0, response->data[4]);
	client.update_auxiliary_input(0x1235, 3, 4);
	frames = auxiliary_frames(can.vt);
	EXPECT_FALSE(std::any_of(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE600 && frame.data[0] == 0x26;
	}));
	auxiliary_status_enable(client, can.partner, 0xFFFF, true);
	frames = auxiliary_frames(can.vt);
	response = std::find_if(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700 && frame.data[0] == 0x25;
	});
	ASSERT_NE(frames.end(), response);
	EXPECT_EQ(1, response->data[4]);
	client.update_auxiliary_input(0x1234, 2, 3);
	frames = auxiliary_frames(can.vt);
	EXPECT_FALSE(std::any_of(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE600 && frame.data[0] == 0x26;
	}));

	// Learn mode sends changed values even while the input remains disabled.
	auxiliary_vt_status(client, can.partner, 0x40);
	client.update_auxiliary_input(0x1234, 3, 4);
	time_source.update_for_ms(49);
	client.update_auxiliary_input(0x1234, 4, 5);
	frames = auxiliary_frames(can.vt);
	EXPECT_FALSE(std::any_of(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700 && frame.data[0] == 0x26;
	}));
	time_source.update_for_ms(1);
	client.update_auxiliary_input(0x1234, 5, 6);
	frames = auxiliary_frames(can.vt);
	EXPECT_TRUE(std::any_of(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700 && frame.data[0] == 0x26 && (frame.data[7] & 0x03) == 0x03;
	}));
}

TEST_F(VirtualTerminalTest, PreferredAssignmentRetriesThreeTimesThenReportsFailure)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	update_auxiliary_client(client);
	std::vector<VirtualTerminalClient::AuxiliaryAssignmentFailureEvent> failures;
	client.get_auxiliary_assignment_failure_event_dispatcher().add_listener([&](const auto &event) { failures.push_back(event); });

	for (int attempt = 0; attempt < 3; ++attempt)
	{
		if (attempt > 0)
		{
			time_source.update_for_ms(1990); // Payload capture advances time by 10 ms.
			update_auxiliary_client(client);
		}
		expect_empty_preferred_assignment(receive_preferred_payload(can, time_source));
		EXPECT_TRUE(failures.empty());
	}
	time_source.update_for_ms(2000);
	update_auxiliary_client(client);
	ASSERT_EQ(1U, failures.size());
	EXPECT_EQ(&client, failures.front().parentPointer);
	EXPECT_EQ(VirtualTerminalClient::AuxiliaryAssignmentFailureReason::ResponseTimeout, failures.front().reason);
}

TEST_F(VirtualTerminalTest, PreferredAssignmentRejectionReportsAllDefinedReasons)
{
	using Reason = VirtualTerminalClient::AuxiliaryAssignmentFailureReason;
	const std::vector<std::pair<std::uint8_t, Reason>> cases = {
		{ 0x01, Reason::InvalidAuxiliaryInputUnit },
		{ 0x02, Reason::InvalidFunctionObjectID },
		{ 0x04, Reason::InvalidInputObjectID },
		{ 0x08, Reason::DuplicateFunctionObjectID },
		{ 0x10, Reason::OtherVTError },
		{ 0x1F, Reason::InvalidAuxiliaryInputUnit | Reason::InvalidFunctionObjectID | Reason::InvalidInputObjectID | Reason::DuplicateFunctionObjectID | Reason::OtherVTError },
		{ 0xE0, static_cast<Reason>(0) },
		{ 0xE2, Reason::InvalidFunctionObjectID }
	};
	for (const auto &entry : cases)
	{
		SCOPED_TRACE(static_cast<unsigned int>(entry.first));
		AuxiliaryTestCAN can(time_source);
		DerivedTestVTClient client(can.partner, can.ecu);
		client.set_auxiliary_functions_enabled(true);
		client.test_wrapper_set_vt_version(6);
		client.initialize(false);
		client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
		std::vector<VirtualTerminalClient::AuxiliaryAssignmentFailureEvent> failures;
		client.get_auxiliary_assignment_failure_event_dispatcher().add_listener([&](const auto &event) { failures.push_back(event); });
		update_auxiliary_client(client);
		expect_empty_preferred_assignment(receive_preferred_payload(can, time_source));
		auxiliary_response(client, can.partner, entry.first);
		ASSERT_EQ(1U, failures.size());
		EXPECT_EQ(&client, failures.front().parentPointer);
		EXPECT_EQ(Reason::VTRejected | entry.second, failures.front().reason);
		EXPECT_EQ(Reason::VTRejected, failures.front().reason & Reason::VTRejected);
		EXPECT_EQ(entry.second, failures.front().reason & entry.second);
		update_auxiliary_client(client);
		EXPECT_TRUE(auxiliary_frames(can.vt).empty());
		EXPECT_EQ(1U, failures.size());
	}
}

TEST_F(VirtualTerminalTest, PreferredAssignmentIgnoresInvalidAndUnsolicitedResponses)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	std::vector<VirtualTerminalClient::AuxiliaryAssignmentFailureEvent> failures;
	client.get_auxiliary_assignment_failure_event_dispatcher().add_listener([&](const auto &event) { failures.push_back(event); });
	auxiliary_response(client, can.partner, 0x01); // No active transaction.
	update_auxiliary_client(client);
	expect_empty_preferred_assignment(receive_preferred_payload(can, time_source));
	auxiliary_response(client, auxiliary_device(99, 0x22), 0x01);
	receive_auxiliary(client, can.partner, { 0x22, 0x01 });
	EXPECT_TRUE(failures.empty());
	// Neither invalid response completed the transaction: a valid rejection must still report failure.
	auxiliary_response(client, can.partner, 0x02);
	ASSERT_EQ(1U, failures.size());
	EXPECT_EQ(VirtualTerminalClient::AuxiliaryAssignmentFailureReason::VTRejected |
	            VirtualTerminalClient::AuxiliaryAssignmentFailureReason::InvalidFunctionObjectID,
	          failures.front().reason);
	auxiliary_response(client, can.partner, 0x01); // Transaction already completed.
	EXPECT_EQ(1U, failures.size());
}

TEST_F(VirtualTerminalTest, InvalidPreferredAssignmentsReportInvalidPreferences)
{
	for (bool invalidLoadedEntry : { true, false })
	{
		SCOPED_TRACE(invalidLoadedEntry ? "invalid loaded entry" : "duplicate function across devices");
		AuxiliaryTestCAN can(time_source);
		AuxiliaryPreferences preferences;
		preferences.byName[651] = { { 0x0201, static_cast<std::uint16_t>(invalidLoadedEntry ? NULL_OBJECT_ID : 0x1234), VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } };
		preferences.byName[652] = { { 0x0201, 0x2345, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } };
		DerivedTestVTClient client(can.partner, can.ecu);
		client.set_auxiliary_functions_enabled(true);
		client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, store_auxiliary_preferences, &preferences);
		client.test_wrapper_set_vt_version(6);
		client.initialize(false);
		client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
		update_auxiliary_client(client);
		expect_empty_preferred_assignment(receive_preferred_payload(can, time_source));
		auxiliary_response(client, can.partner);
		std::vector<VirtualTerminalClient::AuxiliaryAssignmentFailureEvent> failures;
		client.get_auxiliary_assignment_failure_event_dispatcher().add_listener([&](const auto &event) { failures.push_back(event); });
		auxiliary_maintenance(client, auxiliary_device(651, 0x24), 0x3344);
		if (!invalidLoadedEntry)
		{
			auxiliary_maintenance(client, auxiliary_device(652, 0x25), 0x3344);
		}
		update_auxiliary_client(client);
		ASSERT_EQ(1U, failures.size());
		EXPECT_EQ(&client, failures.front().parentPointer);
		EXPECT_EQ(VirtualTerminalClient::AuxiliaryAssignmentFailureReason::InvalidPreferences, failures.front().reason);
		EXPECT_TRUE(auxiliary_frames(can.vt).empty());
		update_auxiliary_client(client);
		EXPECT_EQ(1U, failures.size());
	}
}

TEST_F(VirtualTerminalTest, AuxiliaryMaintenanceReadinessFollowsPoolStates)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_input_model_identification_code(0x3344);
	client.add_auxiliary_input_object_id(0x1234);
	client.initialize(false);
	const auto checkScheduled = [&](std::uint32_t elapsed_ms, std::uint8_t expectedReady, std::size_t expectedCount = 1) {
		time_source.update_for_ms(elapsed_ms);
		client.update();
		const auto frames = auxiliary_frames(can.vt);
		std::size_t count = 0;
		for (const auto &frame : frames)
		{
			const CANIdentifier identifier(frame.identifier);
			if ((identifier.get_parameter_group_number() == 0xE700) && (frame.data[0] == 0x23))
			{
				++count;
				EXPECT_EQ(0xFF, identifier.get_destination_address());
				EXPECT_EQ(CANIdentifier::CANPriority::Priority3, identifier.get_priority());
				EXPECT_EQ(0x44, frame.data[1]);
				EXPECT_EQ(0x33, frame.data[2]);
				EXPECT_EQ(expectedReady, frame.data[3]);
			}
		}
		EXPECT_EQ(expectedCount, count);
	};
	// Initializing maintenance starts before pool loading and repeats every 100 ms.
	checkScheduled(100, 0);
	checkScheduled(99, 0, 0);
	checkScheduled(1, 0);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	checkScheduled(100, 1);
	// Ordinary reconnection keeps maintenance running without a >500 ms reconfiguration pause.
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Disconnected);
	checkScheduled(100, 0);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForLoadVersionResponse);
	receive_auxiliary(client, can.partner, { 0xD1, 0xFF, 0xFF, 0xFF, 0xFF, 0, 0xFF, 0xFF });
	ASSERT_TRUE(client.get_is_connected());
	checkScheduled(100, 1);

	const auto check = [&](VirtualTerminalClient::StateMachineState state, std::uint8_t expectedReady) {
		client.test_wrapper_set_state(state);
		ASSERT_TRUE(client.test_wrapper_send_auxiliary_maintenance());
		const auto frames = auxiliary_frames(can.vt);
		const auto maintenance = std::find_if(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
			return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700 && frame.data[0] == 0x23;
		});
		ASSERT_NE(frames.end(), maintenance);
		EXPECT_EQ(expectedReady, maintenance->data[3]);
	};
	check(VirtualTerminalClient::StateMachineState::SendStoreVersion, 1);
	check(VirtualTerminalClient::StateMachineState::WaitForStoreVersionResponse, 1);
	check(VirtualTerminalClient::StateMachineState::Connected, 1);
	check(VirtualTerminalClient::StateMachineState::Failed, 0);
	check(VirtualTerminalClient::StateMachineState::Disconnected, 0);

	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForLoadVersionResponse);
	receive_auxiliary(client, can.partner, { 0xD1, 0xFF, 0xFF, 0xFF, 0xFF, 0, 0xFF, 0xFF });
	EXPECT_TRUE(client.get_is_connected());
	ASSERT_TRUE(client.test_wrapper_send_auxiliary_maintenance());
	const auto frames = auxiliary_frames(can.vt);
	const auto maintenance = std::find_if(frames.rbegin(), frames.rend(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700 && frame.data[0] == 0x23;
	});
	ASSERT_NE(frames.rend(), maintenance);
	EXPECT_EQ(1, maintenance->data[3]);

	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForLoadVersionResponse);
	receive_auxiliary(client, can.partner, { 0xD1, 0xFF, 0xFF, 0xFF, 0xFF, 1, 0xFF, 0xFF });
	ASSERT_TRUE(client.test_wrapper_send_auxiliary_maintenance());
	const auto failedLoadFrames = auxiliary_frames(can.vt);
	const auto failedLoadMaintenance = std::find_if(failedLoadFrames.begin(), failedLoadFrames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700 && frame.data[0] == 0x23;
	});
	ASSERT_NE(failedLoadFrames.end(), failedLoadMaintenance);
	EXPECT_EQ(0, failedLoadMaintenance->data[3]);
}

TEST_F(VirtualTerminalTest, EmptyPreferredAssignmentWaitsForCorrectVTResponse)
{
	AuxiliaryTestCAN can(time_source);
	AuxiliaryPreferences preferences;
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, store_auxiliary_preferences, &preferences);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	std::vector<VirtualTerminalClient::AuxiliaryAssignmentFailureEvent> failures;
	client.get_auxiliary_assignment_failure_event_dispatcher().add_listener([&](const auto &event) { failures.push_back(event); });
	auxiliary_maintenance(client, auxiliary_device(600, 0x20), 0x3344, 0);
	update_auxiliary_client(client);
	EXPECT_EQ(1U, preferences.loadCalls);
	expect_empty_preferred_assignment(receive_preferred_payload(can, time_source));
	auxiliary_response(client, auxiliary_device(99, 0x22));
	update_auxiliary_client(client);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());
	auxiliary_response(client, can.partner);
	update_auxiliary_client(client);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());
	EXPECT_TRUE(failures.empty());
}

TEST_F(VirtualTerminalTest, InitializingPreferredDevicesWaitAndStaleDevicesDoNotTriggerSync)
{
	AuxiliaryTestCAN can(time_source);
	AuxiliaryPreferences preferences;
	preferences.byName[601] = { { 0x0101, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } };
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, store_auxiliary_preferences, &preferences);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	const auto initializing = auxiliary_device(601, 0x21);
	auxiliary_maintenance(client, initializing, 0x3344, 0);
	update_auxiliary_client(client);
	EXPECT_EQ(1U, preferences.loadCalls);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());
	auxiliary_maintenance(client, initializing, 0x3344, 1);
	update_auxiliary_client(client);
	const auto readyFrames = auxiliary_frames(can.vt);
	EXPECT_TRUE(std::any_of(readyFrames.begin(), readyFrames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xEC00;
	}));
	auxiliary_response(client, can.partner);
	update_auxiliary_client(client);
	auxiliary_frames(can.vt);

	// A stale initializing device with no pending J.7.7 trigger does not cause an empty command.
	const auto stale = auxiliary_device(602, 0x22);
	preferences.byName[602] = { { 0x0102, 0x2345, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } };
	auxiliary_maintenance(client, stale, 0x4455, 0);
	update_auxiliary_client(client);
	EXPECT_EQ(2U, preferences.loadCalls);
	time_source.update_for_ms(301);
	auxiliary_maintenance(client, initializing, 0x3344, 1);
	update_auxiliary_client(client);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());
}

TEST_F(VirtualTerminalTest, StaleInitializingDeviceReleasesPendingInitialSync)
{
	AuxiliaryTestCAN can(time_source);
	AuxiliaryPreferences preferences;
	preferences.byName[603] = { { 0x0103, 0x3456, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } };
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, store_auxiliary_preferences, &preferences);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	auxiliary_maintenance(client, auxiliary_device(603, 0x23), 0x5566, 0);
	update_auxiliary_client(client);
	EXPECT_EQ(1U, preferences.loadCalls);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());
	time_source.update_for_ms(301);
	update_auxiliary_client(client);
	expect_empty_preferred_assignment(receive_preferred_payload(can, time_source));
}

TEST_F(VirtualTerminalTest, ObsoletePreferredSnapshotCompletesOnACKOrTimeout)
{
	const auto run = [&](bool acknowledge) {
		SCOPED_TRACE(acknowledge ? "ACK" : "timeout");
		AuxiliaryTestCAN can(time_source);
		AuxiliaryPreferences preferences;
		preferences.byName[651] = { { 0x0201, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } };
		DerivedTestVTClient client(can.partner, can.ecu);
		client.set_auxiliary_functions_enabled(true);
		client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, store_auxiliary_preferences, &preferences);
		client.test_wrapper_set_vt_version(6);
		client.initialize(false);
		client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
		std::vector<VirtualTerminalClient::AuxiliaryAssignmentFailureEvent> failures;
		client.get_auxiliary_assignment_failure_event_dispatcher().add_listener([&](const auto &event) { failures.push_back(event); });
		update_auxiliary_client(client);
		expect_empty_preferred_assignment(receive_preferred_payload(can, time_source));
		auxiliary_response(client, can.partner);
		update_auxiliary_client(client);
		auxiliary_frames(can.vt);

		const auto device = auxiliary_device(651, 0x24);
		auxiliary_maintenance(client, device, 0x3344);
		update_auxiliary_client(client);
		const auto transactionFrames = auxiliary_frames(can.vt);
		ASSERT_TRUE(std::any_of(transactionFrames.begin(), transactionFrames.end(), [](const CANMessageFrame &frame) {
			return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xEC00;
		}));
		time_source.update_for_ms(301);
		update_auxiliary_client(client);
		EXPECT_TRUE(auxiliary_frames(can.vt).empty());

		if (acknowledge)
		{
			auxiliary_response(client, can.partner);
			update_auxiliary_client(client);
		}
		else
		{
			time_source.update_for_ms(1699);
			update_auxiliary_client(client);
		}
		const auto replacementFrames = auxiliary_frames(can.vt);
		EXPECT_FALSE(std::any_of(replacementFrames.begin(), replacementFrames.end(), [](const CANMessageFrame &frame) {
			return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xEC00;
		}));
		if (acknowledge)
		{
			EXPECT_TRUE(failures.empty());
			const auto replacement = std::find_if(replacementFrames.begin(), replacementFrames.end(), [](const CANMessageFrame &frame) {
				return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700 && frame.data[0] == 0x22;
			});
			ASSERT_NE(replacementFrames.end(), replacement);
			expect_empty_preferred_assignment({ replacement->data, replacement->data + replacement->dataLength });
		}
		else
		{
			ASSERT_EQ(1U, failures.size());
			EXPECT_EQ(&client, failures.front().parentPointer);
			EXPECT_EQ(VirtualTerminalClient::AuxiliaryAssignmentFailureReason::ObsoleteAssignment, failures.front().reason);
			EXPECT_TRUE(std::any_of(replacementFrames.begin(), replacementFrames.end(), [](const CANMessageFrame &frame) {
				return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700 && frame.data[0] == 0x22;
			}));
		}
	};
	run(true);
	run(false);
}

TEST_F(VirtualTerminalTest, PersistentAssignmentWaitsForOutstandingPreferenceLoad)
{
	AuxiliaryTestCAN can(time_source);
	AuxiliaryPreferences preferences;
	preferences.byName[701] = { { 0x0101, 0x1111, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } };
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, store_auxiliary_preferences, &preferences);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	update_auxiliary_client(client);
	expect_empty_preferred_assignment(receive_preferred_payload(can, time_source));

	const auto device = auxiliary_device(701, 0x25);
	auxiliary_maintenance(client, device, 0x3344, 1);
	auxiliary_assignment(client, can.partner, 701, 0x0102, 0x2222, 0x04);
	update_auxiliary_client(client);
	EXPECT_EQ(0U, preferences.loadCalls);
	ASSERT_EQ(1U, preferences.byName[701].size());
	EXPECT_EQ(0x0101, preferences.byName[701][0].functionObjectID);
	expect_assignment_response(can, 0x0102, 1);
	time_source.update_for_ms(1990);
	auxiliary_maintenance(client, device, 0x3344, 1);
	update_auxiliary_client(client);
	EXPECT_EQ(0U, preferences.loadCalls);
	const auto retryFrames = auxiliary_frames(can.vt);
	EXPECT_TRUE(std::any_of(retryFrames.begin(), retryFrames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700 && frame.data[0] == 0x22;
	}));

	auxiliary_response(client, can.partner);
	update_auxiliary_client(client);
	EXPECT_EQ(1U, preferences.loadCalls);
	auxiliary_frames(can.vt);
	auxiliary_assignment(client, can.partner, 701, 0x0102, 0x2222, 0x04);
	update_auxiliary_client(client);
	ASSERT_EQ(2U, preferences.byName[701].size());
	EXPECT_EQ(0x0101, preferences.byName[701][0].functionObjectID);
	EXPECT_EQ(0x0102, preferences.byName[701][1].functionObjectID);
	expect_assignment_response(can, 0x0102, 0);
}

TEST_F(VirtualTerminalTest, PaddedAndUnpaddedAssignmentsAreAcknowledgedStoredAndRemoved)
{
	for (const std::size_t length : { 14, 16 })
	{
		SCOPED_TRACE(length);
		AuxiliaryTestCAN can(time_source);
		AuxiliaryPreferences preferences;
		DerivedTestVTClient client(can.partner, can.ecu);
		client.set_auxiliary_functions_enabled(true);
		client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, store_auxiliary_preferences, &preferences);
		client.test_wrapper_set_vt_version(6);
		const auto device = auxiliary_device(501, 0x21);
		auxiliary_maintenance(client, device, 0x3344);
		auxiliary_assignment(client, can.partner, 501, 0x0101, 0x1234, 0x04, length);
		update_auxiliary_client(client);
		expect_assignment_response(can, 0x0101, 0);
		ASSERT_EQ(1U, preferences.byName[501].size());
		EXPECT_EQ(0x0101, preferences.byName[501][0].functionObjectID);
		EXPECT_EQ(0x1234, preferences.byName[501][0].inputObjectID);

		std::vector<std::uint16_t> events;
		client.get_auxiliary_function_event_dispatcher().add_listener([&](const auto &event) { events.push_back(event.function.functionObjectID); });
		auxiliary_status(client, device, 0x1234);
		EXPECT_EQ((std::vector<std::uint16_t>{ 0x0101 }), events);

		auxiliary_assignment(client, can.partner, 0xFFFFFFFFFFFFFFFFULL, 0x0101, NULL_OBJECT_ID, 0x1F, length);
		update_auxiliary_client(client);
		expect_assignment_response(can, 0x0101, 0);
		EXPECT_TRUE(preferences.byName[501].empty());
		events.clear();
		auxiliary_status(client, device, 0x1234);
		EXPECT_TRUE(events.empty());
	}
}

TEST_F(VirtualTerminalTest, AuxiliaryStatusUsesSourceNameAndDispatchesEveryMatchingFunction)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.test_wrapper_set_vt_version(6);
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
}

TEST_F(VirtualTerminalTest, LostInputExpiresActiveAssignmentButPreservesItsPreference)
{
	AuxiliaryTestCAN can(time_source);
	AuxiliaryPreferences preferences;
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, store_auxiliary_preferences, &preferences);
	client.test_wrapper_set_vt_version(6);
	const auto device = auxiliary_device(501, 0x21);
	auxiliary_maintenance(client, device, 0x3344);
	auxiliary_assignment(client, can.partner, 501, 0x0101, 0x1234, 0x04);
	update_auxiliary_client(client);
	ASSERT_EQ(1U, preferences.byName[501].size());
	EXPECT_EQ(0x1234, preferences.byName[501][0].inputObjectID);

	std::vector<std::uint16_t> events;
	client.get_auxiliary_function_event_dispatcher().add_listener([&](const auto &event) { events.push_back(event.function.functionObjectID); });
	auxiliary_status(client, device, 0x1234);
	EXPECT_EQ((std::vector<std::uint16_t>{ 0x0101 }), events);
	time_source.update_for_ms(300);
	update_auxiliary_client(client);
	events.clear();
	auxiliary_status(client, device, 0x1234);
	EXPECT_TRUE(events.empty());
	ASSERT_EQ(1U, preferences.byName[501].size());
	EXPECT_EQ(0x1234, preferences.byName[501][0].inputObjectID);
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
	std::vector<std::uint8_t> testPool = test_helpers::read_vt3_test_pool();

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
	DerivedTestVTClient::staticTestPool = test_helpers::read_vt3_test_pool();

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
	std::vector<std::uint8_t> testPool = test_helpers::read_vt3_test_pool();

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
	EXPECT_EQ(12, clientUnderTest.test_wrapper_get_number_bytes_in_object(testObject, sizeof(testObject)));

	// Add a macro and re-check the length
	testObject[11] = 1;
	EXPECT_EQ(14, clientUnderTest.test_wrapper_get_number_bytes_in_object(testObject, sizeof(testObject)));

	// Add a full list of child objects and re-check the length
	testObject[10] = 255;
	EXPECT_EQ(524, clientUnderTest.test_wrapper_get_number_bytes_in_object(testObject, sizeof(testObject)));

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
	EXPECT_EQ(13, clientUnderTest.test_wrapper_get_number_bytes_in_object(testObject, sizeof(testObject)));

	// Add a macro and re-check the length
	testObject[12] = 1;
	EXPECT_EQ(15, clientUnderTest.test_wrapper_get_number_bytes_in_object(testObject, sizeof(testObject)));

	// Cant really resize these since the width is a max value. Should remain the same.
	EXPECT_EQ(true, clientUnderTest.test_wrapper_resize_object(testObject, 2.0f, VirtualTerminalObjectType::InputBoolean));
	EXPECT_EQ(testWidth, static_cast<std::uint16_t>(testObject[4]) | (static_cast<std::uint16_t>(testObject[5]) << 8));
}

TEST_F(VirtualTerminalTest, NumberBytesInExtendedInputAttributes)
{
	// clang-format off
	std::uint8_t testPool[] = {
		0xD2, 0x04, // Object ID 1234
		0x26, // Extended Input Attributes
		0x00, // Valid characters are listed
		0x02, // 2 code planes
		0x00, // Code plane 0
		0x03, // 3 ranges
		0x30, 0x00, 0x39, 0x00, // '0'..'9'
		0x41, 0x00, 0x5A, 0x00, // 'A'..'Z'
		0x61, 0x00, 0x7A, 0x00, // 'a'..'z'
		0x01, // Code plane 1
		0x02, // 2 ranges
		0x00, 0x00, 0xFF, 0x00,
		0x00, 0x01, 0xFF, 0x01,
		0xD3, 0x04, // Object ID 1235
		0x0E, // Output Rectangle
		0x00, 0x00, 0x64, 0x00, 0x32, 0x00, 0x00, 0xFF, 0xFF, 0x00
	};
	// clang-format on

	DerivedTestVTClient clientUnderTest(nullptr, nullptr);

	const std::uint32_t extendedInputAttributesSize = clientUnderTest.test_wrapper_get_number_bytes_in_object(testPool, sizeof(testPool));
	ASSERT_EQ(29, extendedInputAttributesSize);
	EXPECT_EQ(1235, testPool[extendedInputAttributesSize] | (testPool[extendedInputAttributesSize + 1] << 8));
	EXPECT_EQ(static_cast<std::uint8_t>(VirtualTerminalObjectType::OutputRectangle), testPool[extendedInputAttributesSize + 2]);
	EXPECT_EQ(13, clientUnderTest.test_wrapper_get_number_bytes_in_object(&testPool[extendedInputAttributesSize], sizeof(testPool) - extendedInputAttributesSize));

	// Cut off inside the second code plane's ranges, then inside its header
	EXPECT_EQ(0, clientUnderTest.test_wrapper_get_number_bytes_in_object(testPool, 28));
	EXPECT_EQ(0, clientUnderTest.test_wrapper_get_number_bytes_in_object(testPool, 20));
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
		EXPECT_EQ(0, clientUnderTest.test_wrapper_get_number_bytes_in_object(testObject, sizeof(testObject)));
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

TEST_F(VirtualTerminalTest, ExtendedVersionResponsesDriveTheConnectStateMachine)
{
	NAME clientNAME(0);
	auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(clientNAME, 0, 0x40);
	std::vector<isobus::NAMEFilter> vtNameFilters = { isobus::NAMEFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal)) };
	auto vtPartner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	const std::vector<std::uint8_t> pool = { 0x00 };
	clientUnderTest.set_object_pool(0, &pool, "EXTENDED VERSION");

	const auto create_get_versions_response = [](const std::string &label) {
		std::vector<std::uint8_t> response = { 0xD3, 1 };
		response.insert(response.end(), label.begin(), label.end());
		response.resize(2 + 32, ' ');
		return create_vt_to_ecu_message(response);
	};

	// Only the 32 byte label tells these apart, the first 7 characters are the same
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForGetVersionsResponse);
	clientUnderTest.test_wrapper_process_rx_message(create_get_versions_response("EXTENDED OTHER"), &clientUnderTest);
	EXPECT_EQ(VirtualTerminalClient::StateMachineState::UploadObjectPool, clientUnderTest.state);

	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForGetVersionsResponse);
	clientUnderTest.test_wrapper_process_rx_message(create_get_versions_response("EXTENDED VERSION"), &clientUnderTest);
	EXPECT_EQ(VirtualTerminalClient::StateMachineState::SendLoadVersion, clientUnderTest.state);

	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForLoadVersionResponse);
	clientUnderTest.test_wrapper_process_rx_message(create_vt_to_ecu_message({ 0xD5, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0xFF, 0xFF }), &clientUnderTest);
	EXPECT_EQ(VirtualTerminalClient::StateMachineState::UploadObjectPool, clientUnderTest.state);

	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForLoadVersionResponse);
	clientUnderTest.test_wrapper_process_rx_message(create_vt_to_ecu_message({ 0xD5, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF }), &clientUnderTest);
	EXPECT_EQ(VirtualTerminalClient::StateMachineState::Connected, clientUnderTest.state);

	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForStoreVersionResponse);
	clientUnderTest.test_wrapper_process_rx_message(create_vt_to_ecu_message({ 0xD4, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF }), &clientUnderTest);
	EXPECT_EQ(VirtualTerminalClient::StateMachineState::Connected, clientUnderTest.state);

	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, ExtendedVersionDeletesAreSentOneAtATime)
{
	VirtualCANPlugin serverVT;
	serverVT.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x43, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x44, 0);

	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.initialize(false);
	const std::vector<std::uint8_t> pool = { 0x00 };
	clientUnderTest.set_object_pool(0, &pool, "EXTENDED VERSION");
	clientUnderTest.connectedVTVersion = 5;

	std::vector<std::uint8_t> response = { 0xD3, 2 };
	for (const std::string label : { "OLD VERSION A", "OLD VERSION B" })
	{
		response.insert(response.end(), label.begin(), label.end());
		response.resize(response.size() + 32 - label.size(), ' ');
	}
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForGetVersionsResponse);
	clientUnderTest.test_wrapper_process_rx_message(create_vt_to_ecu_message(response), &clientUnderTest);
	const auto queuedDeletes = clientUnderTest.pendingExtendedVersionDeletes.size();

	// The first delete holds the only TP session to the VT until it completes, so the second has to wait
	clientUnderTest.update();
	const auto deletesAfterFirstUpdate = clientUnderTest.pendingExtendedVersionDeletes.size();
	clientUnderTest.update();
	const auto deletesAfterSecondUpdate = clientUnderTest.pendingExtendedVersionDeletes.size();

	CANHardwareInterface::stop();

	EXPECT_EQ(2U, queuedDeletes);
	EXPECT_EQ(1U, deletesAfterFirstUpdate);
	EXPECT_EQ(1U, deletesAfterSecondUpdate);

	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, ExtendedVersionCommandsAreOnlySentForLongLabelsOnVersion5)
{
	VirtualCANPlugin serverVT;
	serverVT.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x41, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x42, 0);

	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.initialize(false);
	const std::vector<std::uint8_t> pool = { 0x00 };
	CANMessageFrame testFrame = {};

	const auto send_in_state = [&](std::uint8_t vtVersion, const std::string &label, VirtualTerminalClient::StateMachineState sendState) {
		clientUnderTest.set_object_pool(0, &pool, label);
		clientUnderTest.connectedVTVersion = vtVersion;
		time_source.update_for_ms(50);
		serverVT.clear_queue();
		clientUnderTest.test_wrapper_set_state(sendState);
		clientUnderTest.update();
		time_source.update_for_ms(5);
		testFrame = {};
		return serverVT.read_frame(testFrame);
	};

	ASSERT_TRUE(send_in_state(5, "EXTENDED VERSION", VirtualTerminalClient::StateMachineState::SendGetVersions));
	EXPECT_EQ(0xD3, testFrame.data[0]);

	ASSERT_TRUE(send_in_state(5, "VERSION", VirtualTerminalClient::StateMachineState::SendGetVersions));
	EXPECT_EQ(0xDF, testFrame.data[0]);

	ASSERT_TRUE(send_in_state(5, "EXTENDED VERSION", VirtualTerminalClient::StateMachineState::SendLoadVersion));
	EXPECT_EQ(0x10, testFrame.data[0]); // TP request to send
	EXPECT_EQ(33, testFrame.data[1]);

	ASSERT_TRUE(send_in_state(4, "EXTENDED VERSION", VirtualTerminalClient::StateMachineState::SendStoreVersion));
	EXPECT_EQ(0xD0, testFrame.data[0]);
	EXPECT_EQ(0, std::memcmp("EXTENDE", &testFrame.data[1], 7));

	CANHardwareInterface::stop();

	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, WaitForPartnerVTStatusMessageTimesOutAndRetries)
{
	NAME clientNAME(0);
	auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(clientNAME, 0, 0x26);

	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	vtNameFilters.push_back(testFilter);

	auto vtPartner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);

	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForPartnerVTStatusMessage);
	EXPECT_EQ(VirtualTerminalClient::StateMachineState::WaitForPartnerVTStatusMessage, clientUnderTest.state);

	// No VT Status message received yet, but still within the 3 s timeout
	time_source.simulate_delay_ms(2999);
	clientUnderTest.update();
	EXPECT_EQ(VirtualTerminalClient::StateMachineState::WaitForPartnerVTStatusMessage, clientUnderTest.state);

	time_source.simulate_delay_ms(2);
	clientUnderTest.update();
	EXPECT_EQ(VirtualTerminalClient::StateMachineState::Failed, clientUnderTest.state);

	// Failed retries back to Disconnected after its own timeout
	time_source.simulate_delay_ms(5001);
	clientUnderTest.update();
	EXPECT_EQ(VirtualTerminalClient::StateMachineState::Disconnected, clientUnderTest.state);

	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, ObjectPoolParsingWaitsForThreeConsecutiveIdleStatusMessages)
{
	using State = VirtualTerminalClient::StateMachineState;
	auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(NAME(0), 0, 0x26);
	std::vector<NAMEFilter> vtNameFilters = { NAMEFilter(NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(NAME::Function::VirtualTerminal)) };
	auto vtPartner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.connectedVTVersion = 3;

	const auto receive_status = [&](bool parsing) {
		CANIdentifier identifier(CANIdentifier::Type::Extended, static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), CANIdentifier::CANPriority::PriorityDefault6, 0xFF, 0x27);
		CANMessage message(CANMessage::Type::Receive,
		                   identifier,
		                   { 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, static_cast<std::uint8_t>(parsing ? 0x10 : 0x00), 0xFF },
		                   vtPartner,
		                   nullptr,
		                   0);
		clientUnderTest.test_wrapper_process_rx_message(message, &clientUnderTest);
		clientUnderTest.update();
	};

	for (const auto waitState : { State::WaitForEndOfObjectPoolResponse, State::WaitForLoadVersionResponse })
	{
		SCOPED_TRACE(static_cast<int>(waitState));
		clientUnderTest.test_wrapper_set_state(waitState);

		// Parsing can exceed the old three-second response timeout while VT status stays fresh.
		for (int seconds = 0; seconds < 4; ++seconds)
		{
			time_source.simulate_delay_ms(1000);
			receive_status(true);
			EXPECT_EQ(waitState, clientUnderTest.state);
		}

		receive_status(false);
		receive_status(false);
		EXPECT_EQ(waitState, clientUnderTest.state);
		receive_status(true); // Parsing resumes, resetting the consecutive idle count.
		receive_status(false);
		receive_status(false);
		EXPECT_EQ(waitState, clientUnderTest.state);
		receive_status(false);
		EXPECT_EQ(State::Failed, clientUnderTest.state);
	}

	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}
