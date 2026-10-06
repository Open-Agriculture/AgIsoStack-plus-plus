#include <gtest/gtest.h>

#include <limits>
#include <map>
#include <utility>

#include "isobus//utility/iop_file_interface.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"
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
		VirtualTerminalClient::set_state(value);
	}

	void test_wrapper_set_connected_vt_version(std::uint8_t version)
	{
		connectedVTVersion = version;
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

	std::size_t test_wrapper_get_auxiliary_device_count() const
	{
		return assignedAuxiliaryInputDevices.size();
	}

	std::uint64_t test_wrapper_get_auxiliary_device_name(std::size_t index) const
	{
		return assignedAuxiliaryInputDevices.at(index).name;
	}

	bool test_wrapper_get_auxiliary_device_ready(std::size_t index) const
	{
		return assignedAuxiliaryInputDevices.at(index).ready;
	}

	bool test_wrapper_get_our_auxiliary_input_enabled(std::uint16_t objectID) const
	{
		return ourAuxiliaryInputs.at(objectID).enabled;
	}

	std::size_t test_wrapper_get_assignment_transaction_device_count() const
	{
		return auxiliaryAssignmentTransactionDevices.size();
	}

	std::uint64_t test_wrapper_get_assignment_transaction_device_name(std::size_t index) const
	{
		return auxiliaryAssignmentTransactionDevices.at(index).name;
	}

	std::uint8_t test_wrapper_get_assignment_attempt_count() const
	{
		return auxiliaryAssignmentAttemptCount;
	}

	bool test_wrapper_get_assignment_in_flight() const
	{
		return auxiliaryAssignmentTransactionInFlight;
	}

	bool test_wrapper_get_auxiliary_assignment_dirty() const
	{
		return auxiliaryAssignmentDirty;
	}

	void test_wrapper_refresh_vt_status_timestamp()
	{
		lastVTStatusTimestamp_ms = SystemTiming::get_timestamp_ms();
	}

	bool test_wrapper_send_preferred_assignments(std::uint64_t deviceName,
	                                             std::uint16_t modelIdentificationCode,
	                                             const std::vector<AssignedAuxiliaryFunction> &functions)
	{
		return send_auxiliary_functions_preferred_assignment({ { deviceName, modelIdentificationCode, functions, 0, {}, false, true } });
	}

	void test_wrapper_update_auxiliary_assignment_transaction(bool startup)
	{
		update_auxiliary_assignment_transaction(startup);
	}

	bool test_wrapper_send_auxiliary_input_maintenance() const
	{
		return send_auxiliary_input_maintenance();
	}

	bool test_wrapper_send_delete_object_pool() const
	{
		return send_delete_object_pool();
	}

	bool test_wrapper_update_auxiliary_input_status(std::uint16_t objectID)
	{
		return update_auxiliary_input_status(objectID);
	}

	bool test_wrapper_ensure_auxiliary_preferences_loaded()
	{
		LOCK_GUARD(Mutex, auxiliaryPreferenceOperationMutex);
		return ensure_auxiliary_preferences_loaded();
	}
};

std::vector<std::uint8_t> DerivedTestVTClient::staticTestPool;

struct AuxiliaryAssignmentStoreCapture
{
	std::size_t callCount = 0;
	std::uint64_t deviceName = 0;
	std::uint16_t modelIdentificationCode = 0;
	std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction> assignments;
};

struct AuxiliaryAssignmentPreferenceMap
{
	std::size_t loadCount = 0;
	std::size_t storeCount = 0;
	std::map<std::uint64_t, std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction>> assignmentsByDevice;
};

static void capture_auxiliary_assignment_store(std::uint64_t deviceName,
                                               std::uint16_t modelIdentificationCode,
                                               const std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction> &assignments,
                                               void *context)
{
	auto *capture = static_cast<AuxiliaryAssignmentStoreCapture *>(context);
	++capture->callCount;
	capture->deviceName = deviceName;
	capture->modelIdentificationCode = modelIdentificationCode;
	capture->assignments = assignments;
}

static std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction> load_auxiliary_assignment_preferences(std::uint64_t deviceName,
                                                                                                           std::uint16_t,
                                                                                                           void *context)
{
	auto *preferences = static_cast<AuxiliaryAssignmentPreferenceMap *>(context);
	++preferences->loadCount;
	return preferences->assignmentsByDevice[deviceName];
}

static void store_auxiliary_assignment_preferences(std::uint64_t deviceName,
                                                   std::uint16_t,
                                                   const std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction> &assignments,
                                                   void *context)
{
	auto *preferences = static_cast<AuxiliaryAssignmentPreferenceMap *>(context);
	++preferences->storeCount;
	preferences->assignmentsByDevice[deviceName] = assignments;
}

static CANMessage make_auxiliary_message(std::uint8_t function,
                                         std::vector<std::uint8_t> data,
                                         const std::shared_ptr<ControlFunction> &source)
{
	CANIdentifier identifier(CANIdentifier::Type::Extended,
	                         static_cast<std::uint32_t>(function == static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage) ? CANLibParameterGroupNumber::ECUtoVirtualTerminal : CANLibParameterGroupNumber::VirtualTerminalToECU),
	                         CANIdentifier::CANPriority::PriorityDefault6,
	                         0,
	                         source->get_address());
	data[0] = function;
	return CANMessage(CANMessage::Type::Receive, identifier, std::move(data), source, nullptr, source->get_can_port(), 0);
}

static std::size_t count_transmitted_function_frames(VirtualCANPlugin &virtualVT, std::uint8_t function)
{
	CANHardwareInterface::update();
	std::size_t count = 0;
	CANMessageFrame frame = {};
	while (virtualVT.read_frame(frame, 0))
	{
		if (function == frame.data[0])
		{
			++count;
		}
	}
	return count;
}

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

TEST_F(VirtualTerminalTest, AuxiliaryAssignmentPreferenceBitAndFunctionRemoval)
{
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, nullptr);
	AuxiliaryAssignmentStoreCapture storedAssignments;
	clientUnderTest.set_auxiliary_assignment_callbacks(nullptr, capture_auxiliary_assignment_store, &storedAssignments);

	NAME inputDeviceName(0);
	inputDeviceName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	inputDeviceName.set_identity_number(42);
	const auto inputDevice = std::make_shared<ControlFunction>(inputDeviceName, 0x21, 0);
	const auto deviceName = inputDeviceName.get_full_name();

	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(
	                                                  static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage),
	                                                  { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
	                                                  inputDevice),
	                                                &clientUnderTest);
	ASSERT_TRUE(clientUnderTest.test_wrapper_ensure_auxiliary_preferences_loaded());

	// The bit 7 clear command stores preferred assignments; bit 7 set leaves
	// the assignment temporary.
	const auto makeAssignment = [deviceName, vtPartner](std::uint16_t functionObjectID, std::uint8_t flags) {
		std::vector<std::uint8_t> bytes(14, 0xFF);
		bytes[0] = static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand);
		for (std::size_t index = 0; index < 8; ++index)
		{
			bytes[index + 1] = static_cast<std::uint8_t>(deviceName >> (index * 8));
		}
		bytes[9] = flags;
		bytes[10] = 0x01;
		bytes[11] = 0x02;
		bytes[12] = static_cast<std::uint8_t>(functionObjectID);
		bytes[13] = static_cast<std::uint8_t>(functionObjectID >> 8);
		return make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand), std::move(bytes), vtPartner);
	};
	clientUnderTest.test_wrapper_process_rx_message(makeAssignment(0x0102, 0x80), &clientUnderTest);
	EXPECT_EQ(0U, storedAssignments.callCount);
	clientUnderTest.test_wrapper_process_rx_message(makeAssignment(0x0101, 0x00), &clientUnderTest);
	EXPECT_EQ(0U, storedAssignments.callCount);
	clientUnderTest.update();
	ASSERT_EQ(1U, storedAssignments.callCount);
	EXPECT_EQ(deviceName, storedAssignments.deviceName);
	EXPECT_EQ(0x1234, storedAssignments.modelIdentificationCode);
	ASSERT_EQ(1U, storedAssignments.assignments.size());
	EXPECT_EQ(0x0101, storedAssignments.assignments[0].functionObjectID);

	// Removing the temporary entry must preserve the preferred entry, while
	// removing the preferred entry must clear storage.
	const auto unassign = [vtPartner](std::uint16_t functionObjectID) {
		std::vector<std::uint8_t> bytes(14, 0xFF);
		bytes[0] = static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand);
		for (std::size_t index = 1; index <= 8; ++index)
		{
			bytes[index] = 0xFF; // DEFAULT_NAME means a global unassignment command
		}
		bytes[9] = 0x1F; // ReservedRemoveAssignment, preferred bit clear
		bytes[10] = 0xFF;
		bytes[11] = 0xFF; // NULL_OBJECT_ID selects function unassignment
		bytes[12] = static_cast<std::uint8_t>(functionObjectID);
		bytes[13] = static_cast<std::uint8_t>(functionObjectID >> 8);
		return make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand), std::move(bytes), vtPartner);
	};
	clientUnderTest.test_wrapper_process_rx_message(unassign(0x0102), &clientUnderTest);
	clientUnderTest.update();
	EXPECT_EQ(2U, storedAssignments.callCount);
	ASSERT_EQ(1U, storedAssignments.assignments.size());
	EXPECT_EQ(0x0101, storedAssignments.assignments[0].functionObjectID);
	clientUnderTest.test_wrapper_process_rx_message(unassign(0x0101), &clientUnderTest);
	clientUnderTest.update();
	EXPECT_EQ(3U, storedAssignments.callCount);
	EXPECT_TRUE(storedAssignments.assignments.empty());
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
}

TEST_F(VirtualTerminalTest, AuxiliaryMaintenanceUsesNameAndRequiresExactReadyValue)
{
	DerivedTestVTClient clientUnderTest(nullptr, nullptr);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	NAME firstName(0);
	firstName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	firstName.set_identity_number(101);
	NAME secondName(0);
	secondName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	secondName.set_identity_number(102);
	const auto firstDevice = std::make_shared<ControlFunction>(firstName, 0x21, 0);
	const auto secondDevice = std::make_shared<ControlFunction>(secondName, 0x22, 0);

	auto maintenance = [](std::uint8_t ready, const std::shared_ptr<ControlFunction> &source) {
		return make_auxiliary_message(
		  static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage),
		  { 0, 0x34, 0x12, ready, 0xFF, 0xFF, 0xFF, 0xFF },
		  source);
	};

	clientUnderTest.test_wrapper_process_rx_message(maintenance(0xFF, firstDevice), &clientUnderTest);
	ASSERT_EQ(1U, clientUnderTest.test_wrapper_get_auxiliary_device_count());
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_auxiliary_device_ready(0));
	clientUnderTest.test_wrapper_process_rx_message(maintenance(1, firstDevice), &clientUnderTest);
	clientUnderTest.test_wrapper_process_rx_message(maintenance(1, secondDevice), &clientUnderTest);
	ASSERT_EQ(2U, clientUnderTest.test_wrapper_get_auxiliary_device_count());
	EXPECT_EQ(firstName.get_full_name(), clientUnderTest.test_wrapper_get_auxiliary_device_name(0));
	EXPECT_EQ(secondName.get_full_name(), clientUnderTest.test_wrapper_get_auxiliary_device_name(1));
	EXPECT_TRUE(clientUnderTest.test_wrapper_get_auxiliary_device_ready(0));
	EXPECT_TRUE(clientUnderTest.test_wrapper_get_auxiliary_device_ready(1));

	// Both devices share a model code; each must independently expire when its
	// maintenance messages stop.
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	clientUnderTest.update();
	time_source.update_for_ms(301);
	clientUnderTest.update();
	EXPECT_EQ(2U, clientUnderTest.test_wrapper_get_auxiliary_device_count());
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_auxiliary_device_ready(0));
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_auxiliary_device_ready(1));
}

TEST_F(VirtualTerminalTest, ReturningToReadyMarksCompleteAssignmentSetDirtyAgain)
{
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, nullptr);
	AuxiliaryAssignmentPreferenceMap preferences;
	NAME inputName(0);
	inputName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	inputName.set_identity_number(103);
	preferences.assignmentsByDevice[inputName.get_full_name()] = { VirtualTerminalClient::AssignedAuxiliaryFunction(0x0101, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary) };
	clientUnderTest.set_auxiliary_assignment_callbacks(load_auxiliary_assignment_preferences, nullptr, &preferences);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	clientUnderTest.test_wrapper_set_connected_vt_version(0x03);
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	clientUnderTest.test_wrapper_refresh_vt_status_timestamp();

	const auto inputDevice = std::make_shared<ControlFunction>(inputName, 0x21, 0);
	const auto maintenance = [&inputDevice](std::uint8_t state) {
		return make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage),
		                              { 0, 0x34, 0x12, state, 0xFF, 0xFF, 0xFF, 0xFF },
		                              inputDevice);
	};
	clientUnderTest.test_wrapper_process_rx_message(maintenance(0), &clientUnderTest);
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_auxiliary_assignment_dirty());
	clientUnderTest.test_wrapper_process_rx_message(maintenance(1), &clientUnderTest);
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_auxiliary_assignment_dirty()); // Stored preferences are loaded outside the receive callback.
	ASSERT_TRUE(clientUnderTest.test_wrapper_ensure_auxiliary_preferences_loaded());
	EXPECT_TRUE(clientUnderTest.test_wrapper_get_auxiliary_assignment_dirty());
	clientUnderTest.set_auxiliary_functions_enabled(false);
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
}

TEST_F(VirtualTerminalTest, RemovingOneGlobalPreferencePreservesUnloadedPreferencesForOtherDevices)
{
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, nullptr);
	AuxiliaryAssignmentPreferenceMap preferences;
	NAME firstName(0);
	firstName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	firstName.set_identity_number(121);
	NAME secondName(0);
	secondName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	secondName.set_identity_number(122);
	const auto firstNameValue = firstName.get_full_name();
	const auto secondNameValue = secondName.get_full_name();
	preferences.assignmentsByDevice[firstNameValue] = { VirtualTerminalClient::AssignedAuxiliaryFunction(0x0101, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary) };
	preferences.assignmentsByDevice[secondNameValue] = { VirtualTerminalClient::AssignedAuxiliaryFunction(0x0202, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary) };
	clientUnderTest.set_auxiliary_assignment_callbacks(load_auxiliary_assignment_preferences, store_auxiliary_assignment_preferences, &preferences);
	clientUnderTest.set_auxiliary_functions_enabled(true);

	const auto firstDevice = std::make_shared<ControlFunction>(firstName, 0x21, 0);
	const auto secondDevice = std::make_shared<ControlFunction>(secondName, 0x22, 0);
	const auto maintenance = [](const std::shared_ptr<ControlFunction> &device) {
		return make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage),
		                              { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
		                              device);
	};
	clientUnderTest.test_wrapper_process_rx_message(maintenance(firstDevice), &clientUnderTest);
	clientUnderTest.test_wrapper_process_rx_message(maintenance(secondDevice), &clientUnderTest);
	ASSERT_EQ(0U, preferences.loadCount); // Preferences remain lazy until assignment handling needs them.

	std::vector<std::uint8_t> removeOneFunction(14, 0xFF);
	removeOneFunction[9] = 0x1F; // Remove one function, with preferred storage enabled.
	removeOneFunction[12] = 0x01;
	removeOneFunction[13] = 0x01;
	const auto removeGlobalFunction = [&]() {
		return make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand), removeOneFunction, vtPartner);
	};
	clientUnderTest.test_wrapper_process_rx_message(removeGlobalFunction(), &clientUnderTest);
	clientUnderTest.update();
	EXPECT_EQ(0U, preferences.loadCount);
	EXPECT_EQ(1U, preferences.assignmentsByDevice[firstNameValue].size());
	EXPECT_EQ(1U, preferences.assignmentsByDevice[secondNameValue].size());
	ASSERT_TRUE(clientUnderTest.test_wrapper_ensure_auxiliary_preferences_loaded());
	ASSERT_EQ(2U, preferences.loadCount);
	clientUnderTest.test_wrapper_process_rx_message(removeGlobalFunction(), &clientUnderTest);
	EXPECT_EQ(0U, preferences.storeCount);
	clientUnderTest.update();
	EXPECT_EQ(2U, preferences.storeCount);
	ASSERT_TRUE(preferences.assignmentsByDevice[firstNameValue].empty());
	ASSERT_EQ(1U, preferences.assignmentsByDevice[secondNameValue].size());
	EXPECT_EQ(0x0202, preferences.assignmentsByDevice[secondNameValue][0].functionObjectID);
	clientUnderTest.set_auxiliary_functions_enabled(false);
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
}

TEST_F(VirtualTerminalTest, ReassigningFunctionMovesActiveAndPreferredOwnershipBetweenDevices)
{
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, nullptr);
	AuxiliaryAssignmentPreferenceMap preferences;
	NAME firstName(0);
	firstName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	firstName.set_identity_number(131);
	NAME secondName(0);
	secondName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	secondName.set_identity_number(132);
	const auto firstNameValue = firstName.get_full_name();
	const auto secondNameValue = secondName.get_full_name();
	preferences.assignmentsByDevice[firstNameValue] = { VirtualTerminalClient::AssignedAuxiliaryFunction(0x0101, 0x1111, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary) };
	clientUnderTest.set_auxiliary_assignment_callbacks(load_auxiliary_assignment_preferences, store_auxiliary_assignment_preferences, &preferences);

	const auto firstDevice = std::make_shared<ControlFunction>(firstName, 0x21, 0);
	const auto secondDevice = std::make_shared<ControlFunction>(secondName, 0x22, 0);
	const auto maintenance = [](const std::shared_ptr<ControlFunction> &device) {
		return make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage),
		                              { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
		                              device);
	};
	clientUnderTest.test_wrapper_process_rx_message(maintenance(firstDevice), &clientUnderTest);
	clientUnderTest.test_wrapper_process_rx_message(maintenance(secondDevice), &clientUnderTest);
	ASSERT_TRUE(clientUnderTest.test_wrapper_ensure_auxiliary_preferences_loaded());

	std::vector<std::uint8_t> assignment(14, 0xFF);
	for (std::size_t index = 0; index < 8; ++index)
	{
		assignment[index + 1] = static_cast<std::uint8_t>(secondNameValue >> (index * 8));
	}
	assignment[9] = static_cast<std::uint8_t>(0x80 | static_cast<std::uint8_t>(VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary));
	assignment[10] = 0x22;
	assignment[11] = 0x22;
	assignment[12] = 0x01;
	assignment[13] = 0x01;
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand),
	                                                                       std::move(assignment),
	                                                                       vtPartner),
	                                                &clientUnderTest);
	ASSERT_EQ(2U, preferences.loadCount);
	ASSERT_EQ(0U, preferences.storeCount); // A temporary assignment must not rewrite stored preferences.
	ASSERT_EQ(1U, preferences.assignmentsByDevice[firstNameValue].size());
	EXPECT_TRUE(preferences.assignmentsByDevice[secondNameValue].empty());
	EXPECT_EQ(0x1111, preferences.assignmentsByDevice[firstNameValue][0].inputObjectID);

	std::vector<std::uint8_t> preferredAssignment(14, 0xFF);
	for (std::size_t index = 0; index < 8; ++index)
	{
		preferredAssignment[index + 1] = static_cast<std::uint8_t>(secondNameValue >> (index * 8));
	}
	preferredAssignment[9] = static_cast<std::uint8_t>(VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary);
	preferredAssignment[10] = 0x22;
	preferredAssignment[11] = 0x22;
	preferredAssignment[12] = 0x01;
	preferredAssignment[13] = 0x01;
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand),
	                                                                       std::move(preferredAssignment),
	                                                                       vtPartner),
	                                                &clientUnderTest);
	EXPECT_EQ(0U, preferences.storeCount);
	clientUnderTest.update();
	EXPECT_EQ(2U, preferences.storeCount);
	EXPECT_TRUE(preferences.assignmentsByDevice[firstNameValue].empty());
	ASSERT_EQ(1U, preferences.assignmentsByDevice[secondNameValue].size());
	EXPECT_EQ(0x2222, preferences.assignmentsByDevice[secondNameValue][0].inputObjectID);

	std::vector<std::uint16_t> dispatchedFunctions;
	clientUnderTest.get_auxiliary_function_event_dispatcher().add_listener([&dispatchedFunctions](const VirtualTerminalClient::AuxiliaryFunctionEvent &event) {
		dispatchedFunctions.push_back(event.function.functionObjectID);
	});
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoStatusMessage),
	                                                                       { 0, 0x11, 0x11, 1, 0, 0, 0, 0 },
	                                                                       firstDevice),
	                                                &clientUnderTest);
	EXPECT_TRUE(dispatchedFunctions.empty());
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoStatusMessage),
	                                                                       { 0, 0x22, 0x22, 1, 0, 0, 0, 0 },
	                                                                       secondDevice),
	                                                &clientUnderTest);
	ASSERT_EQ(1U, dispatchedFunctions.size());
	EXPECT_EQ(0x0101, dispatchedFunctions[0]);
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
}

TEST_F(VirtualTerminalTest, AuxiliaryStatusIsScopedToDeviceDispatchesAllMatchesAndHonorsLearnMode)
{
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, nullptr);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	std::vector<std::uint16_t> dispatchedFunctions;
	clientUnderTest.get_auxiliary_function_event_dispatcher().add_listener([&dispatchedFunctions](const VirtualTerminalClient::AuxiliaryFunctionEvent &event) {
		dispatchedFunctions.push_back(event.function.functionObjectID);
	});

	NAME firstName(0);
	firstName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	firstName.set_identity_number(111);
	NAME secondName(0);
	secondName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	secondName.set_identity_number(112);
	const auto firstDevice = std::make_shared<ControlFunction>(firstName, 0x21, 0);
	const auto secondDevice = std::make_shared<ControlFunction>(secondName, 0x22, 0);
	const auto modelCode = std::uint16_t{ 0x1234 };
	const auto maintenance = [modelCode](const std::shared_ptr<ControlFunction> &device) {
		return make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage),
		                              { 0, static_cast<std::uint8_t>(modelCode), static_cast<std::uint8_t>(modelCode >> 8), 1, 0xFF, 0xFF, 0xFF, 0xFF },
		                              device);
	};
	clientUnderTest.test_wrapper_process_rx_message(maintenance(firstDevice), &clientUnderTest);
	clientUnderTest.test_wrapper_process_rx_message(maintenance(secondDevice), &clientUnderTest);
	ASSERT_TRUE(clientUnderTest.test_wrapper_ensure_auxiliary_preferences_loaded());

	const auto assign = [&clientUnderTest, &vtPartner](std::uint64_t deviceName, std::uint16_t functionObjectID, std::uint8_t type) {
		std::vector<std::uint8_t> data(14, 0xFF);
		for (std::size_t index = 0; index < 8; ++index)
		{
			data[index + 1] = static_cast<std::uint8_t>(deviceName >> (index * 8));
		}
		data[9] = type;
		data[10] = 0x34;
		data[11] = 0x12;
		data[12] = static_cast<std::uint8_t>(functionObjectID);
		data[13] = static_cast<std::uint8_t>(functionObjectID >> 8);
		clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand),
		                                                                       std::move(data),
		                                                                       vtPartner),
		                                                &clientUnderTest);
	};
	assign(firstName.get_full_name(), 0x0101, static_cast<std::uint8_t>(VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary));
	assign(firstName.get_full_name(), 0x0102, static_cast<std::uint8_t>(VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary));
	assign(firstName.get_full_name(), 0x0103, static_cast<std::uint8_t>(VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::QuadratureAnalogueLatching));
	assign(firstName.get_full_name(), 0x0104, static_cast<std::uint8_t>(VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::QuadratureAnalogueMomentary));
	assign(firstName.get_full_name(), 0x0105, static_cast<std::uint8_t>(VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BidirectionalEncoder));

	const auto makeStatus = [](std::uint16_t inputObjectID, const std::shared_ptr<ControlFunction> &device) {
		return make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoStatusMessage),
		                              { 0, static_cast<std::uint8_t>(inputObjectID), static_cast<std::uint8_t>(inputObjectID >> 8), 1, 0, 2, 0, 0 },
		                              device);
	};
	clientUnderTest.test_wrapper_process_rx_message(makeStatus(0x1234, secondDevice), &clientUnderTest);
	EXPECT_TRUE(dispatchedFunctions.empty()); // A second unit cannot generate this unit's function events.

	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::VTStatusMessage),
	                                                                       { 0, 0x26, 0xFF, 0xFF, 0xFF, 0xFF, 0x40, 0xFF },
	                                                                       vtPartner),
	                                                &clientUnderTest);
	EXPECT_TRUE(clientUnderTest.get_auxiliary_input_learn_mode_enabled());
	clientUnderTest.test_wrapper_process_rx_message(makeStatus(0x1234, firstDevice), &clientUnderTest);
	EXPECT_TRUE(dispatchedFunctions.empty());

	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::VTStatusMessage),
	                                                                       { 0, 0x26, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF },
	                                                                       vtPartner),
	                                                &clientUnderTest);
	clientUnderTest.test_wrapper_process_rx_message(makeStatus(0x1234, firstDevice), &clientUnderTest);
	ASSERT_EQ(5U, dispatchedFunctions.size());
	EXPECT_EQ(0x0101, dispatchedFunctions[0]);
	EXPECT_EQ(0x0102, dispatchedFunctions[1]);
	EXPECT_EQ(0x0103, dispatchedFunctions[2]);
	EXPECT_EQ(0x0104, dispatchedFunctions[3]);
	EXPECT_EQ(0x0105, dispatchedFunctions[4]);
	clientUnderTest.set_auxiliary_functions_enabled(false);
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
}

TEST_F(VirtualTerminalTest, AuxiliaryInputEnableWorksWithoutOutputFunctionsEnabled)
{
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, nullptr);
	clientUnderTest.add_auxiliary_input_object_id(0x1234);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputStatusTypeTwoEnableCommand),
	                                                                       { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
	                                                                       vtPartner),
	                                                &clientUnderTest);
	EXPECT_TRUE(clientUnderTest.test_wrapper_get_our_auxiliary_input_enabled(0x1234));
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
}

TEST_F(VirtualTerminalTest, AuxiliaryAssignmentIsAcknowledgedBeforePreferencesAreStored)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	clientUnderTest.test_wrapper_set_connected_vt_version(0x03);
	AuxiliaryAssignmentStoreCapture storedAssignments;
	clientUnderTest.set_auxiliary_assignment_callbacks(nullptr, capture_auxiliary_assignment_store, &storedAssignments);

	NAME inputName(0);
	inputName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	inputName.set_identity_number(281);
	const auto inputDevice = std::make_shared<ControlFunction>(inputName, 0x21, 0);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage),
	                                                                       { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
	                                                                       inputDevice),
	                                                &clientUnderTest);
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(true);
	(void)count_transmitted_function_frames(serverVT, static_cast<std::uint8_t>(VirtualTerminalClient::Function::PreferredAssignmentCommand));

	std::vector<std::uint8_t> assignment(14, 0xFF);
	for (std::size_t index = 0; index < 8; ++index)
	{
		assignment[index + 1] = static_cast<std::uint8_t>(inputName.get_full_name() >> (index * 8));
	}
	assignment[9] = static_cast<std::uint8_t>(VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary);
	assignment[10] = 0x34;
	assignment[11] = 0x12;
	assignment[12] = 0x01;
	assignment[13] = 0x01;
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand),
	                                                                       std::move(assignment),
	                                                                       vtPartner),
	                                                &clientUnderTest);
	EXPECT_EQ(0U, storedAssignments.callCount);

	CANHardwareInterface::update();
	CANMessageFrame frame = {};
	bool assignmentResponseQueued = false;
	while (serverVT.read_frame(frame, 0))
	{
		if ((static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand) == frame.data[0]) &&
		    (0x01 == frame.data[1]) && (0x01 == frame.data[2]))
		{
			assignmentResponseQueued = true;
		}
	}
	EXPECT_TRUE(assignmentResponseQueued);
	EXPECT_EQ(0U, storedAssignments.callCount);

	clientUnderTest.update();
	EXPECT_EQ(1U, storedAssignments.callCount);
	EXPECT_EQ(inputName.get_full_name(), storedAssignments.deviceName);
	ASSERT_EQ(1U, storedAssignments.assignments.size());
	EXPECT_EQ(0x0101, storedAssignments.assignments[0].functionObjectID);

	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, TerminatingClientFlushesAcknowledgedAuxiliaryPreferences)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	AuxiliaryAssignmentStoreCapture storedAssignments; // Must outlive the client callback context.
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	clientUnderTest.test_wrapper_set_connected_vt_version(0x03);
	clientUnderTest.set_auxiliary_assignment_callbacks(nullptr, capture_auxiliary_assignment_store, &storedAssignments);

	NAME inputName(0);
	inputName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	inputName.set_identity_number(282);
	const auto inputDevice = std::make_shared<ControlFunction>(inputName, 0x21, 0);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage),
	                                                                       { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
	                                                                       inputDevice),
	                                                &clientUnderTest);
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(true);
	(void)count_transmitted_function_frames(serverVT, static_cast<std::uint8_t>(VirtualTerminalClient::Function::PreferredAssignmentCommand));

	std::vector<std::uint8_t> assignment(14, 0xFF);
	for (std::size_t index = 0; index < 8; ++index)
	{
		assignment[index + 1] = static_cast<std::uint8_t>(inputName.get_full_name() >> (index * 8));
	}
	assignment[9] = static_cast<std::uint8_t>(VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary);
	assignment[10] = 0x34;
	assignment[11] = 0x12;
	assignment[12] = 0x01;
	assignment[13] = 0x01;
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand),
	                                                                       std::move(assignment),
	                                                                       vtPartner),
	                                                &clientUnderTest);
	EXPECT_EQ(0U, storedAssignments.callCount);
	CANHardwareInterface::update();
	CANMessageFrame responseFrame = {};
	bool assignmentResponseSent = false;
	while (serverVT.read_frame(responseFrame, 0))
	{
		if ((static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand) == responseFrame.data[0]) &&
		    (0x01 == responseFrame.data[1]) && (0x01 == responseFrame.data[2]))
		{
			assignmentResponseSent = true;
		}
	}
	EXPECT_TRUE(assignmentResponseSent);
	clientUnderTest.terminate();
	EXPECT_EQ(1U, storedAssignments.callCount);
	ASSERT_EQ(1U, storedAssignments.assignments.size());
	EXPECT_EQ(0x0101, storedAssignments.assignments[0].functionObjectID);

	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, MalformedAuxiliaryRemoveCommandsLeaveAssignmentAndPreferencesUntouched)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	clientUnderTest.test_wrapper_set_connected_vt_version(0x03);
	AuxiliaryAssignmentStoreCapture storedAssignments;
	clientUnderTest.set_auxiliary_assignment_callbacks(nullptr, capture_auxiliary_assignment_store, &storedAssignments);
	NAME inputName(0);
	inputName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	inputName.set_identity_number(291);
	const auto inputDevice = std::make_shared<ControlFunction>(inputName, 0x21, 0);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage),
	                                                                       { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
	                                                                       inputDevice),
	                                                &clientUnderTest);
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(true);

	auto assignment = [inputName]() {
		std::vector<std::uint8_t> bytes(14, 0xFF);
		for (std::size_t index = 0; index < 8; ++index)
		{
			bytes[index + 1] = static_cast<std::uint8_t>(inputName.get_full_name() >> (index * 8));
		}
		bytes[9] = static_cast<std::uint8_t>(VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary);
		bytes[10] = 0x34;
		bytes[11] = 0x12;
		bytes[12] = 0x01;
		bytes[13] = 0x01;
		return bytes;
	};
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand), assignment(), vtPartner), &clientUnderTest);
	clientUnderTest.update();
	ASSERT_EQ(1U, storedAssignments.callCount);
	ASSERT_EQ(1U, storedAssignments.assignments.size());
	storedAssignments.callCount = 0;
	CANHardwareInterface::update();
	CANMessageFrame initialFrame = {};
	while (serverVT.read_frame(initialFrame, 0))
	{
	}

	auto removeCommand = [vtPartner](std::uint64_t deviceName, std::uint8_t functionType, std::uint16_t inputObjectID, std::uint16_t functionObjectID) {
		std::vector<std::uint8_t> bytes(14, 0xFF);
		for (std::size_t index = 0; index < 8; ++index)
		{
			bytes[index + 1] = static_cast<std::uint8_t>(deviceName >> (index * 8));
		}
		bytes[9] = functionType;
		bytes[10] = static_cast<std::uint8_t>(inputObjectID);
		bytes[11] = static_cast<std::uint8_t>(inputObjectID >> 8);
		bytes[12] = static_cast<std::uint8_t>(functionObjectID);
		bytes[13] = static_cast<std::uint8_t>(functionObjectID >> 8);
		return make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand), std::move(bytes), vtPartner);
	};
	const auto nullDeviceName = std::numeric_limits<std::uint64_t>::max();
	const std::vector<CANMessage> malformedCommands = {
		removeCommand(nullDeviceName & ~std::uint64_t{ 1 }, 0x1F, 0xFFFF, 0x0101),
		removeCommand(nullDeviceName, 0x1E, 0xFFFF, 0x0101),
		removeCommand(nullDeviceName, 0x1F, 0x1234, 0x0101),
		removeCommand(nullDeviceName, 0x1F, 0x1234, 0xFFFF)
	};
	for (const auto &command : malformedCommands)
	{
		clientUnderTest.test_wrapper_process_rx_message(command, &clientUnderTest);
		clientUnderTest.update();
		CANHardwareInterface::update();
		CANMessageFrame responseFrame = {};
		bool errorResponseSent = false;
		while (serverVT.read_frame(responseFrame, 0))
		{
			if (static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand) == responseFrame.data[0])
			{
				errorResponseSent = (responseFrame.data[3] & 0x01) != 0;
			}
		}
		EXPECT_TRUE(errorResponseSent);
		EXPECT_EQ(0U, storedAssignments.callCount);
	}
	ASSERT_EQ(1U, storedAssignments.assignments.size());
	EXPECT_EQ(0x0101, storedAssignments.assignments[0].functionObjectID);
	clientUnderTest.terminate();
	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, DisabledAuxiliaryInputSendsStatusOnlyInLearnMode)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	clientUnderTest.add_auxiliary_input_object_id(0x1234);
	const std::vector<std::uint8_t> testPool{ 0 };
	clientUnderTest.set_object_pool(0, &testPool);
	clientUnderTest.update_auxiliary_input(0x1234, 1, 7);
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForEndOfObjectPoolResponse);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::EndOfObjectPoolMessage),
	                                                                       { 0, 0, 0, 0, 0, 0, 0, 0 },
	                                                                       vtPartner),
	                                                &clientUnderTest);

	EXPECT_FALSE(clientUnderTest.test_wrapper_update_auxiliary_input_status(0x1234));
	EXPECT_EQ(0U, count_transmitted_function_frames(serverVT, static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoStatusMessage)));

	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::VTStatusMessage),
	                                                                       { 0, 0x26, 0, 0, 0, 0, 0x40, 0 },
	                                                                       vtPartner),
	                                                &clientUnderTest);
	EXPECT_TRUE(clientUnderTest.get_auxiliary_input_learn_mode_enabled());
	EXPECT_TRUE(clientUnderTest.test_wrapper_update_auxiliary_input_status(0x1234));
	EXPECT_EQ(1U, count_transmitted_function_frames(serverVT, static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoStatusMessage)));
	time_source.update_for_ms(25);
	clientUnderTest.update_auxiliary_input(0x1234, 2, 7);
	EXPECT_EQ(0U, count_transmitted_function_frames(serverVT, static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoStatusMessage)));
	time_source.update_for_ms(25);
	clientUnderTest.update_auxiliary_input(0x1234, 3, 7);
	EXPECT_EQ(1U, count_transmitted_function_frames(serverVT, static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoStatusMessage)));

	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::VTStatusMessage),
	                                                                       { 0, 0x26, 0, 0, 0, 0, 0, 0 },
	                                                                       vtPartner),
	                                                &clientUnderTest);
	EXPECT_FALSE(clientUnderTest.test_wrapper_update_auxiliary_input_status(0x1234));
	EXPECT_EQ(0U, count_transmitted_function_frames(serverVT, static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoStatusMessage)));
	time_source.update_for_ms(50);
	clientUnderTest.update_auxiliary_input(0x1234, 2, 7);

	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputStatusTypeTwoEnableCommand),
	                                                                       { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
	                                                                       vtPartner),
	                                                &clientUnderTest);
	EXPECT_TRUE(clientUnderTest.test_wrapper_get_our_auxiliary_input_enabled(0x1234));
	EXPECT_TRUE(clientUnderTest.test_wrapper_update_auxiliary_input_status(0x1234));
	EXPECT_EQ(1U, count_transmitted_function_frames(serverVT, static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoStatusMessage)));
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::SendLoadVersion);
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_our_auxiliary_input_enabled(0x1234));
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForLoadVersionResponse);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::LoadVersionCommand),
	                                                                       { 0, 0, 0, 0, 0, 0, 0, 0 },
	                                                                       vtPartner),
	                                                &clientUnderTest);
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_our_auxiliary_input_enabled(0x1234));
	EXPECT_FALSE(clientUnderTest.test_wrapper_update_auxiliary_input_status(0x1234));
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputStatusTypeTwoEnableCommand),
	                                                                       { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
	                                                                       vtPartner),
	                                                &clientUnderTest);
	EXPECT_TRUE(clientUnderTest.test_wrapper_get_our_auxiliary_input_enabled(0x1234));

	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, AuxiliaryInputDisableAllRejectsEnableAllAndHonorsLearnMode)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.add_auxiliary_input_object_id(0x1234);
	clientUnderTest.add_auxiliary_input_object_id(0x1235);
	const std::vector<std::uint8_t> testPool{ 0 };
	clientUnderTest.set_object_pool(0, &testPool);
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForEndOfObjectPoolResponse);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::EndOfObjectPoolMessage), { 0, 0, 0, 0, 0, 0, 0, 0 }, vtPartner), &clientUnderTest);
	const auto enableCommand = static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputStatusTypeTwoEnableCommand);
	const auto sendEnable = [&](std::uint16_t objectID, std::uint8_t enabled) {
		clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(enableCommand, { 0, static_cast<std::uint8_t>(objectID), static_cast<std::uint8_t>(objectID >> 8), enabled, 0xFF, 0xFF, 0xFF, 0xFF }, vtPartner), &clientUnderTest);
		CANHardwareInterface::update();
		CANMessageFrame frame = {};
		while (serverVT.read_frame(frame, 0))
		{
			if (enableCommand == frame.data[0])
			{
				EXPECT_EQ(static_cast<std::uint8_t>(objectID), frame.data[1]);
				EXPECT_EQ(static_cast<std::uint8_t>(objectID >> 8), frame.data[2]);
				return std::pair<std::uint8_t, std::uint8_t>(frame.data[3], frame.data[4]);
			}
		}
		return std::pair<std::uint8_t, std::uint8_t>(0xFF, 0xFF);
	};
	EXPECT_EQ(std::make_pair(std::uint8_t{ 1 }, std::uint8_t{ 0 }), sendEnable(0x1234, 1));
	EXPECT_EQ(std::make_pair(std::uint8_t{ 1 }, std::uint8_t{ 0 }), sendEnable(0x1235, 1));
	EXPECT_TRUE(clientUnderTest.test_wrapper_get_our_auxiliary_input_enabled(0x1234));
	EXPECT_TRUE(clientUnderTest.test_wrapper_get_our_auxiliary_input_enabled(0x1235));
	EXPECT_EQ(std::make_pair(std::uint8_t{ 0 }, std::uint8_t{ 0 }), sendEnable(NULL_OBJECT_ID, 0));
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_our_auxiliary_input_enabled(0x1234));
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_our_auxiliary_input_enabled(0x1235));
	EXPECT_NE(0U, sendEnable(NULL_OBJECT_ID, 1).second);
	EXPECT_NE(0U, sendEnable(0x1236, 1).second);
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_our_auxiliary_input_enabled(0x1234));
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_our_auxiliary_input_enabled(0x1235));
	NAME otherName(0);
	otherName.set_identity_number(991);
	const auto otherVT = std::make_shared<ControlFunction>(otherName, 0x27, 0);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(enableCommand, { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF }, otherVT), &clientUnderTest);
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_our_auxiliary_input_enabled(0x1234));
	EXPECT_EQ(0U, count_transmitted_function_frames(serverVT, enableCommand));
	time_source.update_for_ms(1000);
	clientUnderTest.update_auxiliary_input(0x1234, 1, 0);
	clientUnderTest.update_auxiliary_input(0x1235, 1, 0);
	EXPECT_FALSE(clientUnderTest.test_wrapper_update_auxiliary_input_status(0x1234));
	EXPECT_FALSE(clientUnderTest.test_wrapper_update_auxiliary_input_status(0x1235));
	const auto statusCommand = static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoStatusMessage);
	EXPECT_EQ(0U, count_transmitted_function_frames(serverVT, statusCommand));
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::VTStatusMessage), { 0, 0x26, 0, 0, 0, 0, 0x40, 0 }, vtPartner), &clientUnderTest);
	EXPECT_TRUE(clientUnderTest.test_wrapper_update_auxiliary_input_status(0x1234));
	EXPECT_TRUE(clientUnderTest.test_wrapper_update_auxiliary_input_status(0x1235));
	EXPECT_EQ(2U, count_transmitted_function_frames(serverVT, statusCommand));
	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, AuxiliaryInputReconfigurationInvalidatesPoolAndEnableState)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.add_auxiliary_input_object_id(0x1234);
	const std::vector<std::uint8_t> testPool{ 0 };
	clientUnderTest.set_object_pool(0, &testPool);
	const auto loadAndEnable = [&]() {
		clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForEndOfObjectPoolResponse);
		clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::EndOfObjectPoolMessage), { 0, 0, 0, 0, 0, 0, 0, 0 }, vtPartner), &clientUnderTest);
		clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputStatusTypeTwoEnableCommand), { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF }, vtPartner), &clientUnderTest);
		EXPECT_TRUE(clientUnderTest.test_wrapper_get_our_auxiliary_input_enabled(0x1234));
	};
	const auto expectUnavailable = [&]() {
		EXPECT_FALSE(clientUnderTest.test_wrapper_get_our_auxiliary_input_enabled(0x1234));
		EXPECT_FALSE(clientUnderTest.test_wrapper_update_auxiliary_input_status(0x1234));
		EXPECT_TRUE(clientUnderTest.test_wrapper_send_auxiliary_input_maintenance());
		CANHardwareInterface::update();
		CANMessageFrame frame = {};
		bool maintenanceFound = false;
		while (serverVT.read_frame(frame, 0))
		{
			if (static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage) == frame.data[0])
			{
				maintenanceFound = true;
				EXPECT_EQ(0U, frame.data[3]);
			}
		}
		EXPECT_TRUE(maintenanceFound);
	};
	loadAndEnable();
	clientUnderTest.add_auxiliary_input_object_id(0x1235, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary);
	expectUnavailable();
	loadAndEnable();
	clientUnderTest.remove_auxiliary_input_object_id(0x1235);
	expectUnavailable();
	loadAndEnable();
	clientUnderTest.set_object_pool(0, &testPool);
	expectUnavailable();
	loadAndEnable();
	EXPECT_TRUE(clientUnderTest.test_wrapper_send_delete_object_pool());
	EXPECT_FALSE(clientUnderTest.test_wrapper_update_auxiliary_input_status(0x1234));
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Disconnected);
	expectUnavailable();
	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, AuxiliaryInputLockBitsAndValuesFollowConnectedVTVersion)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	const std::vector<std::uint8_t> testPool{ 0 };
	clientUnderTest.set_object_pool(0, &testPool);
	for (std::uint8_t version = 3; version <= 6; ++version)
	{
		clientUnderTest.add_auxiliary_input_object_id(static_cast<std::uint16_t>(0x1200 + version));
	}
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForEndOfObjectPoolResponse);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::EndOfObjectPoolMessage),
	                                                                       { 0, 0, 0, 0, 0, 0, 0, 0 },
	                                                                       vtPartner),
	                                                &clientUnderTest);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::VTStatusMessage),
	                                                                       { 0, 0x26, 0, 0, 0, 0, 0x40, 0 },
	                                                                       vtPartner),
	                                                &clientUnderTest);

	auto readStatusFrame = [&]() {
		CANHardwareInterface::update();
		CANMessageFrame frame = {};
		while (serverVT.read_frame(frame, 0))
		{
			if (static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoStatusMessage) == frame.data[0])
			{
				return std::vector<std::uint8_t>(frame.data, frame.data + CAN_DATA_LENGTH);
			}
		}
		return std::vector<std::uint8_t>{};
	};

	for (std::uint8_t version = 3; version <= 6; ++version)
	{
		const auto inputID = static_cast<std::uint16_t>(0x1200 + version);
		clientUnderTest.test_wrapper_set_connected_vt_version(version);
		clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputStatusTypeTwoEnableCommand),
		                                                                       { 0, static_cast<std::uint8_t>(inputID), static_cast<std::uint8_t>(inputID >> 8), 1, 0xFF, 0xFF, 0xFF, 0xFF },
		                                                                       vtPartner),
		                                                &clientUnderTest);

		time_source.update_for_ms(51);
		clientUnderTest.update_auxiliary_input(inputID, 10, 20, false);
		auto frame = readStatusFrame();
		ASSERT_EQ(CAN_DATA_LENGTH, frame.size());
		EXPECT_EQ(0U, static_cast<std::uint8_t>(frame[7] & 0x0C));

		time_source.update_for_ms(51);
		clientUnderTest.update_auxiliary_input(inputID, 10, 20, true);
		if (version < 6)
		{
			EXPECT_TRUE(readStatusFrame().empty());
			time_source.update_for_ms(51);
			clientUnderTest.update_auxiliary_input(inputID, 30, 40, true);
			frame = readStatusFrame();
			ASSERT_EQ(CAN_DATA_LENGTH, frame.size());
			EXPECT_EQ(30U, static_cast<std::uint16_t>(frame[3] | (frame[4] << 8)));
			EXPECT_EQ(40U, static_cast<std::uint16_t>(frame[5] | (frame[6] << 8)));
			EXPECT_EQ(0U, static_cast<std::uint8_t>(frame[7] & 0x0C));
		}
		else
		{
			frame = readStatusFrame();
			ASSERT_EQ(CAN_DATA_LENGTH, frame.size());
			EXPECT_EQ(10U, static_cast<std::uint16_t>(frame[3] | (frame[4] << 8)));
			EXPECT_EQ(20U, static_cast<std::uint16_t>(frame[5] | (frame[6] << 8)));
			EXPECT_EQ(0x04U, static_cast<std::uint8_t>(frame[7] & 0x0C));

			// A physical change while locked is reported as interaction, while
			// the communicated values remain those captured when lock began.
			time_source.update_for_ms(51);
			clientUnderTest.update_auxiliary_input(inputID, 30, 40, true);
			frame = readStatusFrame();
			ASSERT_EQ(CAN_DATA_LENGTH, frame.size());
			EXPECT_EQ(10U, static_cast<std::uint16_t>(frame[3] | (frame[4] << 8)));
			EXPECT_EQ(20U, static_cast<std::uint16_t>(frame[5] | (frame[6] << 8)));
			EXPECT_EQ(0x0CU, static_cast<std::uint8_t>(frame[7] & 0x0C));

			// Interaction remains set after the regular status-on-change flag is
			// cleared, until the input is unlocked.
			time_source.update_for_ms(1000);
			EXPECT_TRUE(clientUnderTest.test_wrapper_update_auxiliary_input_status(inputID));
			frame = readStatusFrame();
			ASSERT_EQ(CAN_DATA_LENGTH, frame.size());
			EXPECT_EQ(10U, static_cast<std::uint16_t>(frame[3] | (frame[4] << 8)));
			EXPECT_EQ(20U, static_cast<std::uint16_t>(frame[5] | (frame[6] << 8)));
			EXPECT_EQ(0x0CU, static_cast<std::uint8_t>(frame[7] & 0x0C));

			// Unlocking is itself a status change and exposes the latest value.
			time_source.update_for_ms(51);
			clientUnderTest.update_auxiliary_input(inputID, 30, 40, false);
			frame = readStatusFrame();
			ASSERT_EQ(CAN_DATA_LENGTH, frame.size());
			EXPECT_EQ(30U, static_cast<std::uint16_t>(frame[3] | (frame[4] << 8)));
			EXPECT_EQ(40U, static_cast<std::uint16_t>(frame[5] | (frame[6] << 8)));
			EXPECT_EQ(0U, static_cast<std::uint8_t>(frame[7] & 0x0C));
		}
	}

	clientUnderTest.terminate();
	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, AlreadyAssignedResponseBitDependsOnVTVersionAndExactAssignment)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.set_auxiliary_functions_enabled(true);

	NAME firstName(0);
	firstName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	firstName.set_identity_number(181);
	NAME secondName(0);
	secondName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	secondName.set_identity_number(182);
	const auto firstDevice = std::make_shared<ControlFunction>(firstName, 0x21, 0);
	const auto secondDevice = std::make_shared<ControlFunction>(secondName, 0x22, 0);
	auto maintenance = [](const std::shared_ptr<ControlFunction> &device) {
		return make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage),
		                              { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
		                              device);
	};
	clientUnderTest.test_wrapper_process_rx_message(maintenance(firstDevice), &clientUnderTest);
	clientUnderTest.test_wrapper_process_rx_message(maintenance(secondDevice), &clientUnderTest);
	ASSERT_TRUE(clientUnderTest.test_wrapper_ensure_auxiliary_preferences_loaded());

	auto assign = [&clientUnderTest, &vtPartner](std::uint64_t deviceName, std::uint16_t inputID) {
		std::vector<std::uint8_t> bytes(14, 0xFF);
		for (std::size_t index = 0; index < 8; ++index)
		{
			bytes[index + 1] = static_cast<std::uint8_t>(deviceName >> (index * 8));
		}
		bytes[9] = static_cast<std::uint8_t>(VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary);
		bytes[10] = static_cast<std::uint8_t>(inputID);
		bytes[11] = static_cast<std::uint8_t>(inputID >> 8);
		bytes[12] = 0x01;
		bytes[13] = 0x01;
		clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand),
		                                                                       std::move(bytes),
		                                                                       vtPartner),
		                                                &clientUnderTest);
	};
	auto readAssignmentErrorCode = [&serverVT]() {
		CANHardwareInterface::update();
		CANMessageFrame frame = {};
		while (serverVT.read_frame(frame, 0))
		{
			if (static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand) == frame.data[0])
			{
				return frame.data[3];
			}
		}
		return static_cast<std::uint8_t>(0xFF);
	};

	for (std::uint8_t version = 3; version <= 6; ++version)
	{
		clientUnderTest.test_wrapper_set_connected_vt_version(version);
		assign(firstName.get_full_name(), 0x1234);
		ASSERT_EQ(0U, readAssignmentErrorCode());
		assign(firstName.get_full_name(), 0x1234);
		const auto expectedAlreadyAssignedBit = static_cast<std::uint8_t>(version < 6 ? 0x02 : 0x00);
		EXPECT_EQ(expectedAlreadyAssignedBit, readAssignmentErrorCode()) << "VT version " << static_cast<unsigned int>(version);
		assign(firstName.get_full_name(), 0x1235);
		EXPECT_EQ(0U, readAssignmentErrorCode()); // Same function, different input object.
		assign(secondName.get_full_name(), 0x1235);
		EXPECT_EQ(0U, readAssignmentErrorCode()); // Same function and input ID, different device.
	}

	clientUnderTest.set_auxiliary_functions_enabled(false);
	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, HeldMomentaryAuxiliaryInputRepeatsEveryTwoHundredMilliseconds)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	clientUnderTest.add_auxiliary_input_object_id(0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary);
	const std::vector<std::uint8_t> testPool{ 0 };
	clientUnderTest.set_object_pool(0, &testPool);
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForEndOfObjectPoolResponse);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::EndOfObjectPoolMessage),
	                                                                       { 0, 0, 0, 0, 0, 0, 0, 0 },
	                                                                       vtPartner),
	                                                &clientUnderTest);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputStatusTypeTwoEnableCommand),
	                                                                       { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
	                                                                       vtPartner),
	                                                &clientUnderTest);

	auto readStatusFrames = [&]() {
		std::vector<CANMessageFrame> statusFrames;
		CANHardwareInterface::update();
		CANMessageFrame frame = {};
		while (serverVT.read_frame(frame, 0))
		{
			if (static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoStatusMessage) == frame.data[0])
			{
				statusFrames.push_back(frame);
			}
		}
		return statusFrames;
	};

	time_source.update_for_ms(51);
	clientUnderTest.update_auxiliary_input(0x1234, 1, 7);
	ASSERT_EQ(1U, readStatusFrames().size());

	time_source.update_for_ms(100);
	clientUnderTest.update_auxiliary_input(0x1234, 2, 7); // Momentary becomes held without changing its transition counter.
	auto statusFrames = readStatusFrames();
	ASSERT_EQ(1U, statusFrames.size());
	EXPECT_EQ(2U, statusFrames[0].data[3]);
	EXPECT_EQ(7U, statusFrames[0].data[5]);

	time_source.update_for_ms(199);
	EXPECT_FALSE(clientUnderTest.test_wrapper_update_auxiliary_input_status(0x1234));
	EXPECT_TRUE(readStatusFrames().empty());
	time_source.update_for_ms(1);
	EXPECT_TRUE(clientUnderTest.test_wrapper_update_auxiliary_input_status(0x1234));
	statusFrames = readStatusFrames();
	ASSERT_EQ(1U, statusFrames.size());
	EXPECT_EQ(2U, statusFrames[0].data[3]);
	EXPECT_EQ(7U, statusFrames[0].data[5]);

	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, StaleMomentaryStatusForcesOneReleaseAndKeepsTransitionCount)
{
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, nullptr);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	clientUnderTest.test_wrapper_refresh_vt_status_timestamp();

	NAME inputName(0);
	inputName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	inputName.set_identity_number(141);
	const auto device = std::make_shared<ControlFunction>(inputName, 0x21, 0);
	const auto maintenance = make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage),
	                                                { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
	                                                device);
	clientUnderTest.test_wrapper_process_rx_message(maintenance, &clientUnderTest);
	ASSERT_TRUE(clientUnderTest.test_wrapper_ensure_auxiliary_preferences_loaded());

	auto assign = [&](std::uint16_t functionID, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType functionType) {
		std::vector<std::uint8_t> data(14, 0xFF);
		for (std::size_t index = 0; index < 8; ++index)
		{
			data[index + 1] = static_cast<std::uint8_t>(inputName.get_full_name() >> (index * 8));
		}
		data[9] = static_cast<std::uint8_t>(functionType);
		data[10] = 0x34;
		data[11] = 0x12;
		data[12] = static_cast<std::uint8_t>(functionID);
		data[13] = static_cast<std::uint8_t>(functionID >> 8);
		clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryAssignmentTypeTwoCommand),
		                                                                       std::move(data),
		                                                                       vtPartner),
		                                                &clientUnderTest);
	};
	assign(0x0101, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary);
	assign(0x0102, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanLatching);

	std::vector<std::pair<std::uint16_t, std::uint16_t>> events;
	clientUnderTest.get_auxiliary_function_event_dispatcher().add_listener([&events](const VirtualTerminalClient::AuxiliaryFunctionEvent &event) {
		events.emplace_back(event.value1, event.value2);
	});
	time_source.update_for_ms(1);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoStatusMessage),
	                                                                       { 0, 0x34, 0x12, 2, 0, 7, 0, 0 },
	                                                                       device),
	                                                &clientUnderTest);
	ASSERT_EQ(2U, events.size());

	time_source.update_for_ms(301);
	clientUnderTest.test_wrapper_refresh_vt_status_timestamp();
	clientUnderTest.test_wrapper_process_rx_message(maintenance, &clientUnderTest);
	clientUnderTest.update();
	ASSERT_EQ(3U, events.size());
	EXPECT_EQ(0U, events[2].first);
	EXPECT_EQ(7U, events[2].second);

	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoStatusMessage),
	                                                                       { 0, 0x34, 0x12, 0xFF, 0xFF, 7, 0, 0 },
	                                                                       device),
	                                                &clientUnderTest);
	ASSERT_EQ(5U, events.size());
	time_source.update_for_ms(301);
	clientUnderTest.test_wrapper_refresh_vt_status_timestamp();
	clientUnderTest.test_wrapper_process_rx_message(maintenance, &clientUnderTest);
	clientUnderTest.update();
	EXPECT_EQ(5U, events.size()); // Error values are not converted to a synthetic zero release.
	clientUnderTest.set_auxiliary_functions_enabled(false);
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
}

TEST_F(VirtualTerminalTest, PreferredAssignmentRejectsUnserializableCount)
{
	DerivedTestVTClient clientUnderTest(nullptr, nullptr);
	const auto deviceName = 0x0123456789ABCDEFULL;
	std::vector<VirtualTerminalClient::AssignedAuxiliaryFunction> assignments(256, VirtualTerminalClient::AssignedAuxiliaryFunction(0x1234, 0x5678, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanLatching));
	EXPECT_FALSE(clientUnderTest.test_wrapper_send_preferred_assignments(deviceName, 0xABCD, assignments));
}

TEST_F(VirtualTerminalTest, AuxiliaryPreferredAssignmentWirePayloadContainsEveryReadyDevice)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	AuxiliaryAssignmentPreferenceMap preferences;
	NAME firstName(0);
	firstName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	firstName.set_identity_number(211);
	NAME secondName(0);
	secondName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	secondName.set_identity_number(212);
	const auto firstNameValue = firstName.get_full_name();
	const auto secondNameValue = secondName.get_full_name();
	preferences.assignmentsByDevice[firstNameValue] = { VirtualTerminalClient::AssignedAuxiliaryFunction(0x0101, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary) };
	preferences.assignmentsByDevice[secondNameValue] = { VirtualTerminalClient::AssignedAuxiliaryFunction(0x0102, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary) };
	clientUnderTest.set_auxiliary_assignment_callbacks(load_auxiliary_assignment_preferences, nullptr, &preferences);
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	clientUnderTest.test_wrapper_set_connected_vt_version(0x03);
	time_source.update_for_ms(1);
	clientUnderTest.test_wrapper_refresh_vt_status_timestamp();

	while (!serverVT.get_queue_empty())
	{
		CANMessageFrame ignoredFrame = {};
		serverVT.read_frame(ignoredFrame);
	}

	const auto maintenance = [](std::uint64_t identityNumber, std::uint8_t address) {
		NAME name(0);
		name.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
		name.set_identity_number(identityNumber);
		const auto device = std::make_shared<ControlFunction>(name, address, 0);
		return make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage),
		                              { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
		                              device);
	};
	clientUnderTest.test_wrapper_process_rx_message(maintenance(211, 0x21), &clientUnderTest);
	clientUnderTest.test_wrapper_process_rx_message(maintenance(212, 0x22), &clientUnderTest);
	clientUnderTest.update();
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(10);
	EXPECT_TRUE(clientUnderTest.test_wrapper_get_assignment_in_flight());
	EXPECT_EQ(2U, preferences.loadCount);

	CANMessageFrame frame = {};
	bool foundPreferredAssignmentRts = false;
	while (serverVT.read_frame(frame, 10))
	{
		if ((frame.data[0] == 0x10) && (frame.data[5] == 0x00) && (frame.data[6] == 0xE7) && (frame.data[7] == 0x00))
		{
			foundPreferredAssignmentRts = true;
			break;
		}
	}
	ASSERT_TRUE(foundPreferredAssignmentRts);
	const auto payloadLength = static_cast<std::uint16_t>(frame.data[1] | (static_cast<std::uint16_t>(frame.data[2]) << 8));
	EXPECT_EQ(32U, payloadLength); // Two device records, each carrying one preferred function.
	EXPECT_EQ(5U, frame.data[3]);
	EXPECT_EQ(2U, preferences.loadCount);
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Disconnected);
	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, DeviceDiscoveredDuringStartupTransactionIsIncludedAfterResponse)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	const auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	clientUnderTest.test_wrapper_set_connected_vt_version(0x03);

	NAME preferredName(0);
	preferredName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	preferredName.set_identity_number(213);
	AuxiliaryAssignmentPreferenceMap preferences;
	preferences.assignmentsByDevice[preferredName.get_full_name()] = { VirtualTerminalClient::AssignedAuxiliaryFunction(0x0101, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary) };
	clientUnderTest.set_auxiliary_assignment_callbacks(load_auxiliary_assignment_preferences, nullptr, &preferences);

	const auto maintenance = [](const NAME &name, std::uint8_t address) {
		return make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage),
		                              { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
		                              std::make_shared<ControlFunction>(name, address, 0));
	};
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(true);
	ASSERT_TRUE(clientUnderTest.test_wrapper_get_assignment_in_flight());

	// The preferred device becomes ready while the empty startup snapshot awaits its response.
	clientUnderTest.test_wrapper_process_rx_message(maintenance(preferredName, 0x21), &clientUnderTest);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::PreferredAssignmentCommand),
	                                                                       { 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	                                                                       vtPartner),
	                                                &clientUnderTest);
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_assignment_in_flight());
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(false);
	ASSERT_TRUE(clientUnderTest.test_wrapper_get_assignment_in_flight());
	ASSERT_EQ(1U, clientUnderTest.test_wrapper_get_assignment_transaction_device_count());
	EXPECT_EQ(preferredName.get_full_name(), clientUnderTest.test_wrapper_get_assignment_transaction_device_name(0));
	clientUnderTest.set_auxiliary_functions_enabled(false);
	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, InitialAuxiliaryAssignmentSendsAnEmptyPreferredSet)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	clientUnderTest.test_wrapper_set_connected_vt_version(0x03);
	while (!serverVT.get_queue_empty())
	{
		CANMessageFrame ignoredFrame = {};
		serverVT.read_frame(ignoredFrame);
	}
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(true);
	EXPECT_TRUE(clientUnderTest.test_wrapper_get_assignment_in_flight());
	EXPECT_EQ(1U, clientUnderTest.test_wrapper_get_assignment_attempt_count());
	time_source.update_for_ms(1);

	CANMessageFrame frame = {};
	const bool frameReceived = serverVT.read_frame(frame, 10);
	if (frameReceived)
	{
		EXPECT_EQ(0x14E72637, frame.identifier);
		EXPECT_EQ(static_cast<std::uint8_t>(VirtualTerminalClient::Function::PreferredAssignmentCommand), frame.data[0]);
		EXPECT_EQ(0U, frame.data[1]);
		for (std::size_t index = 2; index < CAN_DATA_LENGTH; ++index)
		{
			EXPECT_EQ(0xFF, frame.data[index]);
		}
	}
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Disconnected);
	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	EXPECT_TRUE(frameReceived);
}

TEST_F(VirtualTerminalTest, DuplicatePreferredFunctionAcrossDevicesIsReportedWithoutSending)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	AuxiliaryAssignmentPreferenceMap preferences;
	NAME firstName(0);
	firstName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	firstName.set_identity_number(221);
	NAME secondName(0);
	secondName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	secondName.set_identity_number(222);
	const auto firstNameValue = firstName.get_full_name();
	const auto secondNameValue = secondName.get_full_name();
	const auto duplicate = VirtualTerminalClient::AssignedAuxiliaryFunction(0x0101, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary);
	preferences.assignmentsByDevice[firstNameValue] = { duplicate };
	preferences.assignmentsByDevice[secondNameValue] = { duplicate };
	clientUnderTest.set_auxiliary_assignment_callbacks(load_auxiliary_assignment_preferences, nullptr, &preferences);
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	clientUnderTest.test_wrapper_set_connected_vt_version(0x03);
	clientUnderTest.test_wrapper_refresh_vt_status_timestamp();

	while (!serverVT.get_queue_empty())
	{
		CANMessageFrame ignoredFrame = {};
		serverVT.read_frame(ignoredFrame);
	}
	const auto maintenance = [](std::uint64_t identityNumber, std::uint8_t address) {
		NAME name(0);
		name.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
		name.set_identity_number(identityNumber);
		return make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage),
		                              { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
		                              std::make_shared<ControlFunction>(name, address, 0));
	};
	clientUnderTest.test_wrapper_process_rx_message(maintenance(221, 0x21), &clientUnderTest);
	clientUnderTest.test_wrapper_process_rx_message(maintenance(222, 0x22), &clientUnderTest);
	std::size_t failureCount = 0;
	std::uint8_t reportedAttempts = 0;
	clientUnderTest.get_auxiliary_assignment_failure_event_dispatcher().add_listener([&](const VirtualTerminalClient::AuxiliaryAssignmentFailureEvent &event) {
		++failureCount;
		reportedAttempts = event.attempts;
	});
	clientUnderTest.update();

	CANMessageFrame frame = {};
	bool foundPreferredAssignmentRts = false;
	while (serverVT.read_frame(frame, 10))
	{
		if ((frame.data[0] == 0x10) && (frame.data[5] == 0x00) && (frame.data[6] == 0xE7) && (frame.data[7] == 0x00))
		{
			foundPreferredAssignmentRts = true;
		}
	}
	EXPECT_FALSE(foundPreferredAssignmentRts);
	EXPECT_EQ(1U, failureCount);
	EXPECT_EQ(1U, reportedAttempts);
	clientUnderTest.update();
	EXPECT_EQ(1U, failureCount); // Invalid stored preferences are reported once for this readiness transition.
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Disconnected);
	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, PreferredAssignmentResponseMustComeFromPartnerVT)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	clientUnderTest.test_wrapper_set_connected_vt_version(0x03);
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(true);
	ASSERT_TRUE(clientUnderTest.test_wrapper_get_assignment_in_flight());

	NAME otherDeviceName(0);
	otherDeviceName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	otherDeviceName.set_identity_number(231);
	const auto otherDevice = std::make_shared<ControlFunction>(otherDeviceName, 0x22, 0);
	const auto responseFunction = static_cast<std::uint8_t>(VirtualTerminalClient::Function::PreferredAssignmentCommand);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(responseFunction, { 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, otherDevice), &clientUnderTest);
	EXPECT_TRUE(clientUnderTest.test_wrapper_get_assignment_in_flight());

	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(responseFunction, { 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }, vtPartner), &clientUnderTest);
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_assignment_in_flight());
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Disconnected);
	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, FailedLocalPreferredAssignmentSendDoesNotConsumeAnAttempt)
{
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, nullptr);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	clientUnderTest.test_wrapper_set_connected_vt_version(0x03);
	std::size_t failureCount = 0;
	clientUnderTest.get_auxiliary_assignment_failure_event_dispatcher().add_listener([&failureCount](const VirtualTerminalClient::AuxiliaryAssignmentFailureEvent &) {
		++failureCount;
	});

	// There is no local control function, so the CAN send cannot be queued.
	for (std::uint8_t retry = 0; retry < 4; ++retry)
	{
		clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(true);
		EXPECT_FALSE(clientUnderTest.test_wrapper_get_assignment_in_flight());
		EXPECT_EQ(0U, clientUnderTest.test_wrapper_get_assignment_attempt_count());
		time_source.update_for_ms(2000);
	}
	EXPECT_EQ(0U, failureCount);
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
}

TEST_F(VirtualTerminalTest, AuxiliaryInputReadyTracksSuccessfulPoolLoadIndependentlyOfAssignmentResponse)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	clientUnderTest.test_wrapper_set_connected_vt_version(0x03);
	const std::vector<std::uint8_t> testPool{ 0 };
	clientUnderTest.set_object_pool(0, &testPool);
	const auto checkMaintenanceReady = [&](std::uint8_t expectedReady) {
		ASSERT_TRUE(clientUnderTest.test_wrapper_send_auxiliary_input_maintenance());
		CANHardwareInterface::update();
		CANMessageFrame frame = {};
		bool foundMaintenance = false;
		while (serverVT.read_frame(frame, 0))
		{
			if (static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage) == frame.data[0])
			{
				foundMaintenance = true;
				EXPECT_EQ(expectedReady, frame.data[3]);
			}
		}
		EXPECT_TRUE(foundMaintenance);
	};

	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForEndOfObjectPoolResponse);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::EndOfObjectPoolMessage),
	                                                                       { 0, 0, 0, 0, 0, 0, 0, 0 },
	                                                                       vtPartner),
	                                                &clientUnderTest);
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(true);
	ASSERT_TRUE(clientUnderTest.test_wrapper_get_assignment_in_flight());
	(void)count_transmitted_function_frames(serverVT, static_cast<std::uint8_t>(VirtualTerminalClient::Function::PreferredAssignmentCommand));
	checkMaintenanceReady(1); // Assignment response is still pending.

	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::SendLoadVersion);
	checkMaintenanceReady(0); // Reloading makes the pool unavailable.
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::WaitForLoadVersionResponse);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::LoadVersionCommand),
	                                                                       { 0, 0, 0, 0, 0, 0, 0, 0 },
	                                                                       vtPartner),
	                                                &clientUnderTest);
	checkMaintenanceReady(1);
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Disconnected);
	checkMaintenanceReady(0);

	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, UnrelatedAuxiliaryInputDiscoveryDoesNotDirtyPreferredAssignments)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	clientUnderTest.test_wrapper_set_connected_vt_version(0x03);
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(true);
	ASSERT_TRUE(clientUnderTest.test_wrapper_get_assignment_in_flight());
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::PreferredAssignmentCommand),
	                                                                       { 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	                                                                       vtPartner),
	                                                &clientUnderTest);
	ASSERT_FALSE(clientUnderTest.test_wrapper_get_auxiliary_assignment_dirty());

	NAME inputName(0);
	inputName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	inputName.set_identity_number(311);
	const auto inputDevice = std::make_shared<ControlFunction>(inputName, 0x21, 0);
	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage),
	                                                                       { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
	                                                                       inputDevice),
	                                                &clientUnderTest);
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_auxiliary_assignment_dirty());
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(false);
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_assignment_in_flight());
	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, UnrelatedDeviceReadinessDoesNotInvalidatePendingPreferredAssignment)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	clientUnderTest.test_wrapper_set_connected_vt_version(0x03);
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Connected);
	clientUnderTest.test_wrapper_refresh_vt_status_timestamp();

	NAME preferredName(0);
	preferredName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	preferredName.set_identity_number(321);
	NAME unrelatedName(0);
	unrelatedName.set_function_code(static_cast<std::uint8_t>(NAME::Function::IOController));
	unrelatedName.set_identity_number(322);
	AuxiliaryAssignmentPreferenceMap preferences;
	preferences.assignmentsByDevice[preferredName.get_full_name()] = { VirtualTerminalClient::AssignedAuxiliaryFunction(0x0101, 0x1234, VirtualTerminalClient::AuxiliaryTypeTwoFunctionType::BooleanMomentary) };
	clientUnderTest.set_auxiliary_assignment_callbacks(load_auxiliary_assignment_preferences, nullptr, &preferences);
	auto maintenance = [](const NAME &name, std::uint8_t address) {
		return make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::AuxiliaryInputTypeTwoMaintenanceMessage),
		                              { 0, 0x34, 0x12, 1, 0xFF, 0xFF, 0xFF, 0xFF },
		                              std::make_shared<ControlFunction>(name, address, 0));
	};
	clientUnderTest.test_wrapper_process_rx_message(maintenance(preferredName, 0x21), &clientUnderTest);
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(true);
	ASSERT_TRUE(clientUnderTest.test_wrapper_get_assignment_in_flight());
	ASSERT_EQ(1U, clientUnderTest.test_wrapper_get_assignment_attempt_count());
	ASSERT_EQ(1U, clientUnderTest.test_wrapper_get_assignment_transaction_device_count());

	clientUnderTest.test_wrapper_process_rx_message(maintenance(unrelatedName, 0x22), &clientUnderTest);
	time_source.update_for_ms(301);
	clientUnderTest.test_wrapper_refresh_vt_status_timestamp();
	clientUnderTest.test_wrapper_process_rx_message(maintenance(preferredName, 0x21), &clientUnderTest);
	clientUnderTest.update(); // The unrelated input times out while the preferred device stays ready.
	EXPECT_EQ(1U, clientUnderTest.test_wrapper_get_assignment_attempt_count());
	EXPECT_TRUE(clientUnderTest.test_wrapper_get_assignment_in_flight());

	clientUnderTest.test_wrapper_process_rx_message(make_auxiliary_message(static_cast<std::uint8_t>(VirtualTerminalClient::Function::PreferredAssignmentCommand),
	                                                                       { 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
	                                                                       vtPartner),
	                                                &clientUnderTest);
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_auxiliary_assignment_dirty());
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(false);
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_assignment_in_flight());
	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(VirtualTerminalTest, AuxiliaryPreferredAssignmentFailureIsReportedAfterThreeAttempts)
{
	VirtualCANPlugin serverVT;
	serverVT.open();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	auto internalECU = test_helpers::claim_internal_control_function(0x37, 0, time_source);
	auto vtPartner = test_helpers::force_claim_partnered_control_function(0x26, 0);
	DerivedTestVTClient clientUnderTest(vtPartner, internalECU);
	clientUnderTest.set_auxiliary_functions_enabled(true);
	clientUnderTest.test_wrapper_set_connected_vt_version(0x03);
	std::size_t failureCount = 0;
	std::uint8_t reportedAttempts = 0;
	clientUnderTest.get_auxiliary_assignment_failure_event_dispatcher().add_listener([&](const VirtualTerminalClient::AuxiliaryAssignmentFailureEvent &event) {
		++failureCount;
		reportedAttempts = event.attempts;
		EXPECT_EQ(&clientUnderTest, event.parentPointer);
	});

	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(true);
	ASSERT_TRUE(clientUnderTest.test_wrapper_get_assignment_in_flight());
	ASSERT_EQ(1U, clientUnderTest.test_wrapper_get_assignment_attempt_count());
	ASSERT_EQ(0U, clientUnderTest.test_wrapper_get_assignment_transaction_device_count());

	// No response is injected.  Retries happen only after two full seconds, and
	// the third transmitted attempt is the last one.
	std::size_t preferredAssignmentFrames = 0;
	auto countPreferredAssignmentFrames = [&]() {
		preferredAssignmentFrames += count_transmitted_function_frames(serverVT, static_cast<std::uint8_t>(VirtualTerminalClient::Function::PreferredAssignmentCommand));
	};
	countPreferredAssignmentFrames();
	EXPECT_EQ(1U, preferredAssignmentFrames);
	time_source.update_for_ms(1999);
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(true);
	countPreferredAssignmentFrames();
	EXPECT_EQ(1U, preferredAssignmentFrames);
	EXPECT_EQ(1U, clientUnderTest.test_wrapper_get_assignment_attempt_count());

	time_source.update_for_ms(1);
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(true);
	EXPECT_EQ(2U, clientUnderTest.test_wrapper_get_assignment_attempt_count());
	EXPECT_TRUE(clientUnderTest.test_wrapper_get_assignment_in_flight());
	countPreferredAssignmentFrames();
	EXPECT_EQ(2U, preferredAssignmentFrames);
	time_source.update_for_ms(1999);
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(true);
	EXPECT_EQ(2U, clientUnderTest.test_wrapper_get_assignment_attempt_count());
	countPreferredAssignmentFrames();
	EXPECT_EQ(2U, preferredAssignmentFrames);
	time_source.update_for_ms(1);
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(true);
	EXPECT_EQ(3U, clientUnderTest.test_wrapper_get_assignment_attempt_count());
	EXPECT_TRUE(clientUnderTest.test_wrapper_get_assignment_in_flight());
	countPreferredAssignmentFrames();
	EXPECT_EQ(3U, preferredAssignmentFrames);

	time_source.update_for_ms(2000);
	clientUnderTest.test_wrapper_update_auxiliary_assignment_transaction(true);
	EXPECT_EQ(0U, clientUnderTest.test_wrapper_get_assignment_attempt_count());
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_assignment_in_flight());
	EXPECT_EQ(0U, clientUnderTest.test_wrapper_get_assignment_transaction_device_count());
	EXPECT_EQ(1U, failureCount);
	EXPECT_EQ(3U, reportedAttempts);
	EXPECT_TRUE(clientUnderTest.get_is_connected());
	countPreferredAssignmentFrames();
	EXPECT_EQ(3U, preferredAssignmentFrames);
	clientUnderTest.test_wrapper_set_state(VirtualTerminalClient::StateMachineState::Disconnected);
	EXPECT_EQ(0U, clientUnderTest.test_wrapper_get_assignment_transaction_device_count());
	EXPECT_FALSE(clientUnderTest.test_wrapper_get_assignment_in_flight());

	serverVT.close();
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(vtPartner);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
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
