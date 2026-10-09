#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <tuple>

#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#endif

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

class InvalidAddressTestICF : public InternalControlFunction
{
public:
	InvalidAddressTestICF() :
	  InternalControlFunction(NAME(1ULL << 63), NULL_CAN_ADDRESS, 0) {}
};

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
	void test_wrapper_set_transmit_available(bool available)
	{
		if (available)
		{
			if (nullptr != testWrapperSavedControlFunction)
			{
				myControlFunction = testWrapperSavedControlFunction;
				testWrapperSavedControlFunction.reset();
			}
		}
		else if (nullptr != myControlFunction)
		{
			testWrapperSavedControlFunction = myControlFunction;
			myControlFunction = std::make_shared<InvalidAddressTestICF>();
		}
	}
	std::shared_ptr<InternalControlFunction> test_wrapper_get_receive_destination() const
	{
		return (nullptr != testWrapperSavedControlFunction) ? testWrapperSavedControlFunction : myControlFunction;
	}
	void test_wrapper_set_preferred_devices(const std::vector<std::tuple<std::uint64_t,
	                                                                     std::uint16_t,
	                                                                     std::vector<AssignedAuxiliaryFunction>>> &devices)
	{
		LOCK_GUARD(Mutex, auxiliaryAssignmentMutex);
		assignedAuxiliaryInputDevices.clear();
		for (const auto &entry : devices)
		{
			AssignedAuxiliaryInputDevice device{};
			device.name = std::get<0>(entry);
			device.modelIdentificationCode = std::get<1>(entry);
			device.preferredFunctions = std::get<2>(entry);
			device.preferredAssignmentsLoaded = true;
			device.ready = true;
			assignedAuxiliaryInputDevices.push_back(std::move(device));
		}
		auxiliaryAssignmentPending = true;
	}
	std::vector<std::uint8_t> test_wrapper_advance_auxiliary_transaction()
	{
		update_auxiliary_assignment_transaction();
		return auxiliaryAssignmentTransaction.message;
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

private:
	std::shared_ptr<InternalControlFunction> testWrapperSavedControlFunction;
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

static std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction> empty_auxiliary_preferences(std::uint64_t, std::uint16_t, void *)
{
	return {};
}

static void ignore_auxiliary_preferences(std::uint64_t, std::uint16_t, const std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction> &, void *)
{
}

struct ModelAuxiliaryPreferences
{
	using Key = std::pair<std::uint64_t, std::uint16_t>;
	std::map<Key, std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction>> byDevice;
};

static std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction> load_model_auxiliary_preferences(std::uint64_t name, std::uint16_t model, void *context)
{
	return static_cast<ModelAuxiliaryPreferences *>(context)->byDevice[{ name, model }];
}

static void store_model_auxiliary_preferences(std::uint64_t name, std::uint16_t model, const std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction> &assignments, void *context)
{
	static_cast<ModelAuxiliaryPreferences *>(context)->byDevice[{ name, model }] = assignments;
}

#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
struct BlockingAuxiliaryPreferences
{
	std::mutex mutex;
	std::condition_variable condition;
	unsigned int calls = 0;
	bool entered = false;
	bool release = false;
};

static std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction> blocking_load_auxiliary_preferences(std::uint64_t, std::uint16_t, void *context)
{
	auto &blocking = *static_cast<BlockingAuxiliaryPreferences *>(context);
	std::unique_lock<std::mutex> lock(blocking.mutex);
	++blocking.calls;
	if (1 == blocking.calls)
	{
		blocking.entered = true;
		blocking.condition.notify_all();
		blocking.condition.wait(lock, [&blocking]() { return blocking.release; });
	}
	return { { 0x0301, 0x2345, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } };
}
#endif

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
		const auto validDestination = client.test_wrapper_get_receive_destination();
		auto message = test_helpers::create_message(6, pgn, validDestination, source, data.data(), static_cast<std::uint32_t>(data.size()));
		if (client.get_internal_control_function() != validDestination)
		{
			message = CANMessage(message.get_type(), message.get_identifier(), message.get_data(), source, client.get_internal_control_function(), message.get_can_port_index(), message.get_timestamp_us());
		}
		client.test_wrapper_process_rx_message(message, &client);
	}
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
		if ((pgn == 0xE700) || (pgn == 0xEC00) || (pgn == 0xEB00))
		{
			frames.push_back(frame);
		}
	}
	return frames;
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

TEST_F(VirtualTerminalTest, InputRegistrationDoesNotEnableFunctionsAndSendsMaintenance)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	EXPECT_FALSE(client.test_wrapper_auxiliary_enabled());
	client.set_auxiliary_assignment_callbacks(empty_auxiliary_preferences, ignore_auxiliary_preferences, nullptr);
	EXPECT_FALSE(client.test_wrapper_auxiliary_enabled());
	client.add_auxiliary_input_object_id(0x1234);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	EXPECT_FALSE(client.test_wrapper_auxiliary_enabled());

	time_source.update_for_ms(100);
	update_auxiliary_client(client);
	const auto frames = auxiliary_frames(can.vt);
	EXPECT_TRUE(std::any_of(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return (CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700) && (frame.data[0] == 0x23);
	}));
}

TEST_F(VirtualTerminalTest, EmptyPreferredAssignmentWaitsForTheCorrectVTResponse)
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
	update_auxiliary_client(client);
	expect_empty_preferred_assignment(receive_preferred_payload(can, time_source));

	auxiliary_response(client, auxiliary_device(99, 0x22));
	update_auxiliary_client(client);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());
	auxiliary_response(client, can.partner);
	time_source.update_for_ms(2000);
	update_auxiliary_client(client);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());

	const auto input = auxiliary_device(601, 0x21);
	auxiliary_maintenance(client, input, 0x3344, 0);
	update_auxiliary_client(client);
	EXPECT_EQ(0U, preferences.loadCalls);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());

	auxiliary_maintenance(client, input, 0x3344, 1);
	update_auxiliary_client(client);
	EXPECT_EQ(1U, preferences.loadCalls);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(10);
	const auto frames = auxiliary_frames(can.vt);
	EXPECT_TRUE(std::any_of(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return (CANIdentifier(frame.identifier).get_parameter_group_number() == 0xEC00) && (frame.data[0] == 0x10) &&
		  (frame.data[6] == 0xE7) && (frame.data[7] == 0x00);
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
	EXPECT_EQ(1U, failures.size());
	EXPECT_EQ(3, failures[0].attempts);
}

TEST_F(VirtualTerminalTest, FailedInitialPreferredAssignmentTransmissionRemainsPending)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);

	client.test_wrapper_set_transmit_available(false);
	update_auxiliary_client(client);
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());

	client.test_wrapper_set_transmit_available(true);
	update_auxiliary_client(client);
	expect_empty_preferred_assignment(receive_preferred_payload(can, time_source));
}

TEST_F(VirtualTerminalTest, RejectedPreferredAssignmentResponseCompletesTheTransaction)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	update_auxiliary_client(client);
	expect_empty_preferred_assignment(receive_preferred_payload(can, time_source));

	std::vector<VirtualTerminalClient::AuxiliaryAssignmentFailureEvent> failures;
	client.get_auxiliary_assignment_failure_event_dispatcher().add_listener([&](const auto &event) { failures.push_back(event); });
	auxiliary_response(client, can.partner, 1);
	time_source.update_for_ms(2000);
	update_auxiliary_client(client);

	const auto frames = auxiliary_frames(can.vt);
	EXPECT_FALSE(std::any_of(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return (CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700) && (frame.data[0] == 0x22);
	}));
	EXPECT_EQ(1U, failures.size());
	EXPECT_EQ(1, failures.front().attempts);
}

TEST_F(VirtualTerminalTest, AcknowledgementBetweenRetryAttemptsCompletesTheTransaction)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	update_auxiliary_client(client);
	expect_empty_preferred_assignment(receive_preferred_payload(can, time_source));

	// Force the due retry to fail queueing. The original successful attempt must
	// remain outstanding so that its untagged acknowledgement still resolves it.
	time_source.update_for_ms(1990);
	client.test_wrapper_set_transmit_available(false);
	update_auxiliary_client(client);
	client.test_wrapper_set_transmit_available(true);
	auxiliary_response(client, can.partner);
	time_source.update_for_ms(2000);
	update_auxiliary_client(client);

	const auto frames = auxiliary_frames(can.vt);
	EXPECT_FALSE(std::any_of(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return (CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700) && (frame.data[0] == 0x22);
	}));
}

TEST_F(VirtualTerminalTest, PendingAssignmentChangeSurvivesRetryExhaustion)
{
	AuxiliaryTestCAN can(time_source);
	AuxiliaryPreferences preferences;
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, store_auxiliary_preferences, &preferences);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	update_auxiliary_client(client);
	expect_empty_preferred_assignment(receive_preferred_payload(can, time_source));

	// This persistent command creates a real pending synchronization while the
	// initial empty transaction is outstanding.
	const auto device = auxiliary_device(650, 0x23);
	auxiliary_maintenance(client, device, 0x3344);
	auxiliary_assignment(client, can.partner, 650, 0x0201, 0x1234, 0x04);
	update_auxiliary_client(client);
	auxiliary_frames(can.vt);

	for (int retry = 1; retry < 3; ++retry)
	{
		time_source.update_for_ms(1990); // Receiving the previous payload advances the clock by 10 ms.
		auxiliary_maintenance(client, device, 0x3344);
		update_auxiliary_client(client);
		expect_empty_preferred_assignment(receive_preferred_payload(can, time_source));
	}

	std::vector<VirtualTerminalClient::AuxiliaryAssignmentFailureEvent> failures;
	client.get_auxiliary_assignment_failure_event_dispatcher().add_listener([&](const auto &event) { failures.push_back(event); });
	time_source.update_for_ms(1990);
	auxiliary_maintenance(client, device, 0x3344);
	update_auxiliary_client(client);
	EXPECT_EQ(1U, failures.size());
	EXPECT_EQ(3, failures.front().attempts);

	// Exhausting the old snapshot must leave the newer demand available to the
	// next update, which starts a complete multi-frame assignment.
	update_auxiliary_client(client);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(10);
	const auto frames = auxiliary_frames(can.vt);
	EXPECT_TRUE(std::any_of(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		return CANIdentifier(frame.identifier).get_parameter_group_number() == 0xEC00;
	}));
}

TEST_F(VirtualTerminalTest, PreferredAssignmentSerializesCompleteMultiDeviceSnapshot)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	using Function = VirtualTerminalClient::AssignedAuxiliaryFunction;
	const std::vector<std::tuple<std::uint64_t, std::uint16_t, std::vector<Function>>> devices = {
		{ 0x0102030405060708ULL, 0x3344, { { 0x0101, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } } },
		{ 0x8877665544332211ULL,
		  0xA1B2,
		  { { 0x0201, 0x4567, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanLatching },
		    { 0x0202, 0x89AB, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::AnalogueLatching } } }
	};
	client.test_wrapper_set_preferred_devices(devices);
	const auto message = client.test_wrapper_advance_auxiliary_transaction();
	EXPECT_EQ((std::vector<std::uint8_t>{
	            0x22,
	            0x02,
	            0x08,
	            0x07,
	            0x06,
	            0x05,
	            0x04,
	            0x03,
	            0x02,
	            0x01,
	            0x44,
	            0x33,
	            0x01,
	            0x01,
	            0x01,
	            0x34,
	            0x12,
	            0x11,
	            0x22,
	            0x33,
	            0x44,
	            0x55,
	            0x66,
	            0x77,
	            0x88,
	            0xB2,
	            0xA1,
	            0x02,
	            0x01,
	            0x02,
	            0x67,
	            0x45,
	            0x02,
	            0x02,
	            0xAB,
	            0x89,
	          }),
	          message);
}

TEST_F(VirtualTerminalTest, PreferredAssignmentAllowsMaximumFunctionCount)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction> functions;
	for (std::uint16_t id = 1; id <= 0xFF; ++id)
	{
		functions.emplace_back(id, id, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary);
	}
	client.test_wrapper_set_preferred_devices({ { 0x123456789ABCDEF0ULL, 0x3344, functions } });
	const auto message = client.test_wrapper_advance_auxiliary_transaction();
	ASSERT_EQ(2U + 11U + 4U * 255U, message.size());
	EXPECT_EQ(0x22, message[0]);
	EXPECT_EQ(1, message[1]);
	EXPECT_EQ(0xFF, message[12]);
	EXPECT_EQ(0xFF, message[1029]);
	EXPECT_EQ(0x00, message[1030]);
	EXPECT_EQ(0xFF, message[1031]);
	EXPECT_EQ(0x00, message[1032]);
}

TEST_F(VirtualTerminalTest, PreferredAssignmentRejectsDuplicateIDsAndCountOverflow)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	using Function = VirtualTerminalClient::AssignedAuxiliaryFunction;
	std::vector<VirtualTerminalClient::AuxiliaryAssignmentFailureEvent> failures;
	client.get_auxiliary_assignment_failure_event_dispatcher().add_listener([&](const auto &event) { failures.push_back(event); });
	client.test_wrapper_set_preferred_devices({ { 0x1001, 0x3344, { { 0x0101, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } } },
	                                            { 0x1002, 0x3344, { { 0x0101, 0x5678, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } } } });
	EXPECT_TRUE(client.test_wrapper_advance_auxiliary_transaction().empty());
	ASSERT_EQ(1U, failures.size());

	std::vector<Function> tooManyFunctions;
	for (std::uint16_t id = 1; id <= 0x100; ++id)
	{
		tooManyFunctions.emplace_back(id, id, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary);
	}
	client.test_wrapper_set_preferred_devices({ { 0x1003, 0x3344, tooManyFunctions } });
	EXPECT_TRUE(client.test_wrapper_advance_auxiliary_transaction().empty());
	EXPECT_EQ(2U, failures.size());
}

TEST_F(VirtualTerminalTest, NonemptyPreferredAssignmentRetriesUseTheOriginalSnapshot)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	client.test_wrapper_set_preferred_devices({ { 0x1004, 0x3344, { { 0x0101, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } } } });
	const auto firstTransmission = client.test_wrapper_advance_auxiliary_transaction();
	ASSERT_FALSE(firstTransmission.empty());

	client.test_wrapper_set_preferred_devices({ { 0x1004, 0x3344, { { 0x0102, 0x5678, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } } } });
	time_source.update_for_ms(2000);
	const auto retry = client.test_wrapper_advance_auxiliary_transaction();
	EXPECT_EQ(firstTransmission, retry);
}

TEST_F(VirtualTerminalTest, LostTransactionMemberDoesNotStartReplacementBeforeTimeout)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	const auto device = auxiliary_device(651, 0x24);
	client.test_wrapper_set_preferred_devices({ { 651, 0x3344, { { 0x0201, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } } } });
	ASSERT_FALSE(client.test_wrapper_advance_auxiliary_transaction().empty());
	auxiliary_frames(can.vt); // Consume the initial multi-frame command.

	auxiliary_maintenance(client, device, 0x3344, 0);
	update_auxiliary_client(client);
	const auto beforeTimeout = auxiliary_frames(can.vt);
	EXPECT_FALSE(std::any_of(beforeTimeout.begin(), beforeTimeout.end(), [](const CANMessageFrame &frame) {
		const auto pgn = CANIdentifier(frame.identifier).get_parameter_group_number();
		return ((pgn == 0xE700) && (frame.data[0] == 0x22)) || (pgn == 0xEC00);
	}));

	time_source.update_for_ms(2000);
	update_auxiliary_client(client); // Obsolete transaction times out and is discarded.
	update_auxiliary_client(client); // Pending synchronization sends an empty complete set.
	expect_empty_preferred_assignment(receive_preferred_payload(can, time_source));
}

TEST_F(VirtualTerminalTest, UnrelatedLossPreservesSnapshotAndModelChangePreservesOldPreferences)
{
	AuxiliaryTestCAN can(time_source);
	ModelAuxiliaryPreferences preferences;
	const auto memberName = std::uint64_t{ 660 };
	const auto otherMemberName = std::uint64_t{ 661 };
	const auto unrelatedName = std::uint64_t{ 662 };
	const auto oldPreferenceKey = std::make_pair(memberName, static_cast<std::uint16_t>(0x3344));
	preferences.byDevice[oldPreferenceKey] = { { 0x0101, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary } };
	preferences.byDevice[{ otherMemberName, 0x4455 }] = { { 0x0102, 0x2345, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanLatching } };
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.set_auxiliary_assignment_callbacks(load_model_auxiliary_preferences, store_model_auxiliary_preferences, &preferences);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	const auto member = auxiliary_device(memberName, 0x26);
	const auto otherMember = auxiliary_device(otherMemberName, 0x27);
	const auto unrelated = auxiliary_device(unrelatedName, 0x28);
	auxiliary_maintenance(client, member, 0x3344);
	auxiliary_maintenance(client, otherMember, 0x4455);
	auxiliary_maintenance(client, unrelated, 0x5566);
	update_auxiliary_client(client);
	const auto originalSnapshot = client.test_wrapper_advance_auxiliary_transaction();
	ASSERT_FALSE(originalSnapshot.empty());
	ASSERT_EQ(2, originalSnapshot[1]);
	auxiliary_frames(can.vt);

	// A Ready change for a device outside the snapshot must not replace or
	// invalidate the outstanding command.
	auxiliary_maintenance(client, unrelated, 0x5566, 0);
	update_auxiliary_client(client);
	EXPECT_EQ(originalSnapshot, client.test_wrapper_advance_auxiliary_transaction());
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());

	// A model change for a participating device obsoletes the snapshot while
	// retaining the old model's stored preference record.
	auxiliary_maintenance(client, member, 0x3345);
	update_auxiliary_client(client);
	EXPECT_EQ(originalSnapshot, client.test_wrapper_advance_auxiliary_transaction());
	EXPECT_TRUE(auxiliary_frames(can.vt).empty());
	ASSERT_EQ(1U, preferences.byDevice[oldPreferenceKey].size());
	EXPECT_EQ(0x0101, preferences.byDevice[oldPreferenceKey][0].functionObjectID);

	time_source.update_for_ms(2000);
	auxiliary_maintenance(client, member, 0x3345);
	auxiliary_maintenance(client, otherMember, 0x4455);
	update_auxiliary_client(client); // Obsolete snapshot times out.
	update_auxiliary_client(client); // The fresh complete set now contains only the still-eligible member.
	const auto freshSnapshot = client.test_wrapper_advance_auxiliary_transaction();
	ASSERT_FALSE(freshSnapshot.empty());
	ASSERT_EQ(1, freshSnapshot[1]);
	EXPECT_EQ(otherMemberName, [&freshSnapshot]() {
		std::uint64_t name = 0;
		for (unsigned int byte = 0; byte < 8; ++byte)
		{
			name |= static_cast<std::uint64_t>(freshSnapshot[2 + byte]) << (8 * byte);
		}
		return name;
	}());
	ASSERT_EQ(1U, preferences.byDevice[oldPreferenceKey].size());
}

#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
TEST_F(VirtualTerminalTest, DisconnectCanProceedDuringPreferenceLoadAndStaleLoadDoesNotSynchronize)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	BlockingAuxiliaryPreferences blocking;
	client.set_auxiliary_functions_enabled(true);
	client.set_auxiliary_assignment_callbacks(blocking_load_auxiliary_preferences, ignore_auxiliary_preferences, &blocking);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	const auto device = auxiliary_device(652, 0x25);
	auxiliary_maintenance(client, device, 0x3344);

	std::thread updater([&client]() { update_auxiliary_client(client); });
	bool loadEntered = false;
	{
		std::unique_lock<std::mutex> lock(blocking.mutex);
		loadEntered = blocking.condition.wait_for(lock, std::chrono::seconds(2), [&blocking]() { return blocking.entered; });
	}
	if (!loadEntered)
	{
		{
			std::lock_guard<std::mutex> lock(blocking.mutex);
			blocking.release = true;
		}
		blocking.condition.notify_all();
		updater.join();
		FAIL() << "Preference load callback was not invoked";
		return;
	}

	std::mutex disconnectMutex;
	std::condition_variable disconnectCondition;
	bool disconnectReturned = false;
	std::thread disconnector([&]() {
		client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Disconnected);
		{
			std::lock_guard<std::mutex> lock(disconnectMutex);
			disconnectReturned = true;
		}
		disconnectCondition.notify_all();
	});
	bool disconnectedBeforeLoadReturned = false;
	{
		std::unique_lock<std::mutex> lock(disconnectMutex);
		disconnectedBeforeLoadReturned = disconnectCondition.wait_for(lock, std::chrono::seconds(2), [&]() { return disconnectReturned; });
	}
	if (!disconnectedBeforeLoadReturned)
	{
		{
			std::lock_guard<std::mutex> lock(blocking.mutex);
			blocking.release = true;
		}
		blocking.condition.notify_all();
		updater.join();
		disconnector.join();
		FAIL() << "Disconnect was blocked by the preference load callback";
		return;
	}
	client.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	auxiliary_maintenance(client, device, 0x3344, 1);
	{
		std::lock_guard<std::mutex> lock(blocking.mutex);
		blocking.release = true;
	}
	blocking.condition.notify_all();
	updater.join();
	disconnector.join();

	const auto frames = auxiliary_frames(can.vt);
	EXPECT_FALSE(std::any_of(frames.begin(), frames.end(), [](const CANMessageFrame &frame) {
		const auto pgn = CANIdentifier(frame.identifier).get_parameter_group_number();
		return ((pgn == 0xE700) && (frame.data[0] == 0x22)) || (pgn == 0xEC00);
	}));
	EXPECT_EQ(1U, blocking.calls);

	update_auxiliary_client(client);
	EXPECT_EQ(2U, blocking.calls);
	const auto message = client.test_wrapper_advance_auxiliary_transaction();
	ASSERT_FALSE(message.empty());
	EXPECT_EQ(0x22, message[0]);
	EXPECT_EQ(1, message[1]);
}
#endif

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

TEST_F(VirtualTerminalTest, DuplicateAssignmentsUseTheVTVersionSpecificResponse)
{
	AuxiliaryTestCAN can(time_source);
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	const auto device = auxiliary_device(401, 0x21);
	auxiliary_maintenance(client, device, 0x3344);

	for (const auto version : { 5, 6 })
	{
		SCOPED_TRACE(version);
		client.test_wrapper_set_vt_version(static_cast<std::uint8_t>(version));
		const auto function = static_cast<std::uint16_t>(0x0100 + version);
		auxiliary_assignment(client, can.partner, 401, function, 0x1234);
		expect_assignment_response(can, function, 0);
		auxiliary_assignment(client, can.partner, 401, function, 0x1234);
		expect_assignment_response(can, function, version == 5 ? 2 : 0);
	}
}

TEST_F(VirtualTerminalTest, FailedAssignmentResponseKeepsAcceptedPreferencesAndPreservesResponseOrder)
{
	AuxiliaryTestCAN can(time_source);
	AuxiliaryPreferences preferences;
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, store_auxiliary_preferences, &preferences);
	client.test_wrapper_set_vt_version(6);
	client.initialize(false);
	const auto device = auxiliary_device(451, 0x21);
	auxiliary_maintenance(client, device, 0x3344);

	client.test_wrapper_set_transmit_available(false);
	auxiliary_assignment(client, can.partner, 451, 0x0101, 0x1234, 0x04);
	update_auxiliary_client(client);
	ASSERT_EQ(1U, preferences.byName[451].size());
	EXPECT_EQ(0x0101, preferences.byName[451][0].functionObjectID);

	// The first response remains pending when the second command arrives.
	auxiliary_assignment(client, can.partner, 451, 0x0102, 0x1235, 0x04);
	update_auxiliary_client(client);
	ASSERT_EQ(2U, preferences.byName[451].size());
	EXPECT_EQ(0x0102, preferences.byName[451][1].functionObjectID);

	client.test_wrapper_set_transmit_available(true);
	update_auxiliary_client(client);
	const auto frames = auxiliary_frames(can.vt);
	std::vector<std::uint16_t> responses;
	for (const auto &frame : frames)
	{
		if ((CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700) && (frame.data[0] == 0x24))
		{
			responses.push_back(static_cast<std::uint16_t>(frame.data[1] | (frame.data[2] << 8)));
		}
	}
	EXPECT_EQ((std::vector<std::uint16_t>{ 0x0101, 0x0102 }), responses);

	// Accepted persistence is flushed by terminate even when its response could
	// not be queued and no update() runs afterward.
	client.test_wrapper_set_transmit_available(false);
	auxiliary_assignment(client, can.partner, 451, 0x0103, 0x1236, 0x04);
	client.test_wrapper_set_transmit_available(true);
	client.terminate();
	ASSERT_EQ(3U, preferences.byName[451].size());
	EXPECT_EQ(0x0103, preferences.byName[451][2].functionObjectID);
}

TEST_F(VirtualTerminalTest, FullAssignmentResponseQueueRefusesUnappliedCommands)
{
	AuxiliaryTestCAN can(time_source);
	AuxiliaryPreferences preferences;
	DerivedTestVTClient client(can.partner, can.ecu);
	client.set_auxiliary_functions_enabled(true);
	client.set_auxiliary_assignment_callbacks(load_auxiliary_preferences, store_auxiliary_preferences, &preferences);
	client.test_wrapper_set_vt_version(6);
	auxiliary_maintenance(client, auxiliary_device(452, 0x22), 0x3344);
	client.test_wrapper_set_transmit_available(false);

	for (std::uint16_t index = 0; index < 9; ++index)
	{
		auxiliary_assignment(client, can.partner, 452, static_cast<std::uint16_t>(0x1100 + index), static_cast<std::uint16_t>(0x2200 + index), 0x04);
		update_auxiliary_client(client);
	}

	ASSERT_EQ(8U, preferences.byName[452].size());
	for (std::uint16_t index = 0; index < 8; ++index)
	{
		EXPECT_EQ(static_cast<std::uint16_t>(0x1100 + index), preferences.byName[452][index].functionObjectID);
	}

	client.test_wrapper_set_transmit_available(true);
	update_auxiliary_client(client);
	const auto frames = auxiliary_frames(can.vt);
	std::vector<std::uint16_t> responses;
	for (const auto &frame : frames)
	{
		if ((CANIdentifier(frame.identifier).get_parameter_group_number() == 0xE700) && (frame.data[0] == 0x24))
		{
			responses.push_back(static_cast<std::uint16_t>(frame.data[1] | (frame.data[2] << 8)));
		}
	}
	std::vector<std::uint16_t> expected;
	for (std::uint16_t index = 0; index < 8; ++index)
	{
		expected.push_back(static_cast<std::uint16_t>(0x1100 + index));
	}
	EXPECT_EQ(expected, responses);
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

	auxiliary_maintenance(client, device, 0x3344);
	auxiliary_assignment(client, can.partner, 501, 0x0101, 0x1234, 0x04);
	update_auxiliary_client(client);
	auxiliary_status(client, device, 0x1234);
	EXPECT_EQ((std::vector<std::uint16_t>{ 0x0101 }), events);
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
