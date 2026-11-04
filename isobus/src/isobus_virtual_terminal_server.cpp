//================================================================================================
/// @file isobus_virtual_terminal_server.cpp
///
/// @brief Implements portions of an abstract VT server.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#if !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
#include "isobus/isobus/isobus_virtual_terminal_server.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>

namespace isobus
{
	/// @brief Extracts the low byte from a 16-bit value.
	/// @param value The value to extract the low byte from.
	/// @returns The low byte of the value.
	static constexpr std::uint8_t get_low_byte(std::uint16_t value) noexcept
	{
		return static_cast<std::uint8_t>(value & 0xFFU);
	}

	/// @brief Extracts the high byte from a 16-bit value.
	/// @param value The value to extract the high byte from.
	/// @returns The high byte of the value.
	static constexpr std::uint8_t get_high_byte(std::uint16_t value) noexcept
	{
		return static_cast<std::uint8_t>((value >> 8) & 0xFFU);
	}

	/// @brief Creates a mask for a bit in an 8-bit value.
	/// @param bitIndex The zero-based bit index.
	/// @returns A mask with the requested bit set, or zero if the index is invalid.
	static constexpr std::uint8_t get_bit(std::uint8_t bitIndex) noexcept
	{
		return (bitIndex < 8U) ? static_cast<std::uint8_t>(1U << bitIndex) : static_cast<std::uint8_t>(0U);
	}

	/// @brief The VT status message busy code bit index for "VT is busy parsing an object pool" (VT version 3 and later)
	constexpr std::uint8_t BUSY_PARSING_OBJECT_POOL_BIT = 4;

	/// @brief The VT status message busy code bit index for "Auxiliary controls learn mode active" (VT version 3 and later)
	constexpr std::uint8_t AUXILIARY_LEARN_MODE_BIT = 6;

	/// @brief Checks if a working set's object pool parse has started but its response has not been sent yet.
	/// @param workingSet The working set to check.
	/// @returns true if update() has not yet joined the parsing thread, otherwise false.
	static bool is_object_pool_parse_unanswered(VirtualTerminalServerManagedWorkingSet &workingSet)
	{
		const auto state = workingSet.get_object_pool_processing_state();

		return ((VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Running == state) ||
		        (VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Success == state) ||
		        (VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Fail == state));
	}

	/// @brief Builds a get versions or extended get versions response.
	/// @param function The response function code.
	/// @param versions The stored version labels, of which only the first 255 fit the one byte count.
	/// @returns The response payload, padded to at least 8 bytes.
	template<std::size_t LABEL_LENGTH>
	static std::vector<std::uint8_t> build_versions_response(std::uint8_t function, const std::vector<std::array<std::uint8_t, LABEL_LENGTH>> &versions)
	{
		const std::size_t numberOfVersions = std::min<std::size_t>(versions.size(), std::numeric_limits<std::uint8_t>::max());
		std::vector<std::uint8_t> buffer = { function, static_cast<std::uint8_t>(numberOfVersions) };

		if (numberOfVersions < versions.size())
		{
			LOG_WARNING("[VT Server]: get_versions returned too many versions! This client should really delete some.");
		}

		for (std::size_t i = 0; i < numberOfVersions; i++)
		{
			buffer.insert(buffer.end(), versions[i].begin(), versions[i].end());
		}
		buffer.resize(std::max<std::size_t>(buffer.size(), CAN_DATA_LENGTH), 0xFF);
		return buffer;
	}

	/// @brief Reads the version label that follows the function code of a version command.
	/// @param data The message data.
	/// @param labelLength The label length the command uses.
	/// @returns The version label, or an empty vector if the message is too short to hold it.
	static std::vector<std::uint8_t> get_version_label(const std::vector<std::uint8_t> &data, std::uint8_t labelLength)
	{
		std::vector<std::uint8_t> versionLabel;

		if (data.size() > labelLength)
		{
			versionLabel.assign(data.begin() + 1, data.begin() + 1 + labelLength);
		}
		else
		{
			LOG_WARNING("[VT Server]: Ignoring version command %u, it is too short to hold a %u byte version label.", data.at(0), labelLength);
		}
		return versionLabel;
	}

	/// @brief Extracts a byte from a 32-bit value.
	/// @param value The value to extract the byte from.
	/// @param byteIndex The zero-based byte index.
	/// @returns The requested byte, or zero if the index is invalid.
	static constexpr std::uint8_t get_byte(std::uint32_t value, std::uint8_t byteIndex) noexcept
	{
		return (byteIndex < 4U) ? static_cast<std::uint8_t>((value >> (byteIndex * 8U)) & 0xFFU) : static_cast<std::uint8_t>(0U);
	}

	/// @brief Extracts the low 16 bits from a 32-bit value.
	/// @param value The value to extract the low bits from.
	/// @returns The low 16 bits of the value.
	static constexpr std::uint16_t get_low_16_bits(std::uint32_t value) noexcept
	{
		return static_cast<std::uint16_t>(value & 0xFFFFU);
	}

	/// @brief Reads a little-endian 16-bit value from a byte vector.
	/// @param data The vector containing the encoded value.
	/// @param index The index of the first byte. The vector must contain at least two bytes beginning at this index.
	/// @returns The decoded 16-bit value.
	static std::uint16_t get_little_endian_uint16(const std::vector<std::uint8_t> &data, std::size_t index)
	{
		const auto lowByte = static_cast<std::uint16_t>(data[index]);
		const auto highByte = static_cast<std::uint16_t>(data[index + 1]);

		return static_cast<std::uint16_t>(lowByte | static_cast<std::uint16_t>(highByte << 8));
	}

	/// @brief Reads a little-endian 32-bit value from a byte vector.
	/// @param data The vector containing the encoded value.
	/// @param index The index of the first byte. The vector must contain at least four bytes beginning at this index.
	/// @returns The decoded 32-bit value.
	static std::uint32_t get_little_endian_uint32(const std::vector<std::uint8_t> &data, std::size_t index)
	{
		return static_cast<std::uint32_t>(data[index]) |
		  (static_cast<std::uint32_t>(data[index + 1]) << 8) |
		  (static_cast<std::uint32_t>(data[index + 2]) << 16) |
		  (static_cast<std::uint32_t>(data[index + 3]) << 24);
	}

	/// @brief Reads a little-endian 64-bit value from a byte vector.
	/// @param data The vector containing the encoded value.
	/// @param index The index of the first byte. The vector must contain at least eight bytes beginning at this index.
	/// @returns The decoded 64-bit value.
	static std::uint64_t get_little_endian_uint64(const std::vector<std::uint8_t> &data, std::size_t index)
	{
		return static_cast<std::uint64_t>(get_little_endian_uint32(data, index)) |
		  (static_cast<std::uint64_t>(get_little_endian_uint32(data, index + 4)) << 32);
	}

	/// @brief Checks if a working set's parsed object pool has an object of the given type.
	/// @param workingSet The working set to look in.
	/// @param objectID The object ID to look for.
	/// @param type The type the object must have.
	/// @returns true if it does, otherwise false.
	static bool has_object_of_type(VirtualTerminalServerManagedWorkingSet &workingSet, std::uint16_t objectID, VirtualTerminalObjectType type)
	{
		// The object tree is only safe to read once the parsing thread is joined
		if (VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined != workingSet.get_object_pool_processing_state())
		{
			return false;
		}

		const auto object = workingSet.get_object_by_id(objectID);
		return (nullptr != object) && (type == object->get_object_type());
	}

	VirtualTerminalServer::VirtualTerminalServer(std::shared_ptr<InternalControlFunction> controlFunctionToUse) :
	  languageCommandInterface(controlFunctionToUse, true),
	  serverInternalControlFunction(controlFunctionToUse)
	{
	}

	VirtualTerminalServer ::~VirtualTerminalServer()
	{
		if (initialized)
		{
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
			                                                                                          process_rx_message,
			                                                                                          this);
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::WorkingSetMaster),
			                                                                                          process_rx_message,
			                                                                                          this);
		}

		// A shut down working set can still be parsing its object pool, and its thread must not outlive it
		LOCK_GUARD(Mutex, pendingShutdownsMutex);
		for (const auto &shutDown : pendingShutdowns)
		{
			shutDown.first->join_parsing_thread();
		}
	}

	void VirtualTerminalServer::initialize()
	{
		if (!initialized)
		{
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
			                                                                                       process_rx_message,
			                                                                                       this);
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::WorkingSetMaster),
			                                                                                       process_rx_message,
			                                                                                       this);
			initialized = true;
		}
	}

	bool VirtualTerminalServer::get_initialized() const
	{
		return initialized;
	}

	std::shared_ptr<InternalControlFunction> VirtualTerminalServer::get_internal_control_function() const
	{
		return serverInternalControlFunction;
	}

	std::shared_ptr<VirtualTerminalServerManagedWorkingSet> VirtualTerminalServer::get_active_working_set() const
	{
		return activeWorkingSet;
	}

	VirtualTerminalBase::GraphicMode VirtualTerminalServer::get_graphic_mode() const
	{
		return VirtualTerminalBase::GraphicMode::TwoHundredFiftySixColour;
	}

	std::uint8_t VirtualTerminalServer::get_powerup_time() const
	{
		return 0xFF;
	}

	std::uint8_t VirtualTerminalServer::get_supported_small_fonts_bitfield() const
	{
		return 0x7F;
	}

	std::uint8_t VirtualTerminalServer::get_supported_large_fonts_bitfield() const
	{
		return 0x7F;
	}

	void VirtualTerminalServer::identify_vt()
	{
		LOG_ERROR("[VT Server]: The Identify VT command is not implemented");
	}

	void VirtualTerminalServer::screen_capture(std::uint8_t item, std::uint8_t path, std::shared_ptr<ControlFunction> requestor)
	{
		(void)item;
		(void)path;
		(void)requestor;
		LOG_ERROR("[VT Server]: The Screen Capture command is not implemented");
	}

	std::uint8_t VirtualTerminalServer::get_user_layout_softkeymask_bg_color() const
	{
		LOG_ERROR("[VT Server]: The Get User Layout Softkeymask background color is not implemented, returning with black");
		return 0;
	}

	void VirtualTerminalServer::transferred_object_pool_parse_start(std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> &ws) const
	{
		(void)ws;
	}

	void VirtualTerminalServer::object_pool_parse_finished(std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, bool)
	{
	}

	void VirtualTerminalServer::object_pool_deleted(std::shared_ptr<VirtualTerminalServerManagedWorkingSet>)
	{
	}

	void VirtualTerminalServer::control_audio_signal(std::uint8_t, std::uint16_t, std::uint16_t, std::uint16_t, std::shared_ptr<ControlFunction>)
	{
	}

	void VirtualTerminalServer::set_audio_volume(std::uint8_t, std::shared_ptr<ControlFunction>)
	{
	}

	void VirtualTerminalServer::select_working_set(std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet)
	{
		activeWorkingSet = workingSet;
		activeWorkingSetMasterAddress = workingSet->get_control_function()->get_address();
		activeWorkingSetDataMaskObjectID = std::static_pointer_cast<WorkingSet>(workingSet->get_working_set_object())->get_active_mask();
	}

	std::uint8_t VirtualTerminalServer::get_user_layout_datamask_bg_color() const
	{
		LOG_ERROR("[VT Server]: The Get User Layout Datamask background color is not implemented, returning with black");
		return 0;
	}

	EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>> &VirtualTerminalServer::get_on_repaint_event_dispatcher()
	{
		return onRepaintEventDispatcher;
	}

	EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, std::uint16_t> &VirtualTerminalServer::get_on_change_active_mask_event_dispatcher()
	{
		return onChangeActiveMaskEventDispatcher;
	}

	EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, std::uint16_t> &VirtualTerminalServer::get_on_change_active_softkey_mask_event_dispatcher()
	{
		return onChangeActiveSoftKeyMaskEventDispatcher;
	}

	EventDispatcher<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, std::uint16_t, bool> &VirtualTerminalServer::get_on_focus_object_event_dispatcher()
	{
		return onFocusObjectEventDispatcher;
	}

	LanguageCommandInterface &VirtualTerminalServer::get_language_command_interface()
	{
		return languageCommandInterface;
	}

	bool VirtualTerminalServer::check_if_source_is_managed(const CANMessage &message) const
	{
		return (nullptr != get_managed_working_set(message.get_source_control_function()));
	}

	std::shared_ptr<VirtualTerminalServerManagedWorkingSet> VirtualTerminalServer::get_managed_working_set(std::shared_ptr<ControlFunction> controlFunction) const
	{
		for (const auto &ws : managedWorkingSetList)
		{
			if (ws->get_control_function() == controlFunction)
			{
				return ws;
			}
		}
		return nullptr;
	}

	std::shared_ptr<VirtualTerminalServerManagedWorkingSet> VirtualTerminalServer::get_managed_working_set(std::uint64_t name) const
	{
		for (const auto &ws : managedWorkingSetList)
		{
			if ((nullptr != ws->get_control_function()) && (name == ws->get_control_function()->get_NAME().get_full_name()))
			{
				return ws;
			}
		}
		return nullptr;
	}

	void VirtualTerminalServer::process_preferred_assignment_command(const CANMessage &message, std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet)
	{
		std::vector<VirtualTerminalServerManagedWorkingSet::PreferredAuxiliaryAssignment> assignments;
		std::uint16_t faultyFunctionObjectID = NULL_OBJECT_ID;
		const std::uint8_t errorBitfield = validate_preferred_assignment_command(message.get_data(), *workingSet, assignments, faultyFunctionObjectID);

		if (0 == errorBitfield)
		{
			workingSet->set_preferred_auxiliary_assignments(assignments, {});
			LOG_INFO("[VT Server]: Client %u preferred assignment command: accepted %u assignments.", workingSet->get_control_function()->get_address(), static_cast<std::uint32_t>(assignments.size()));
			apply_preferred_auxiliary_assignments(workingSet);
		}
		else
		{
			// A command with any error is ignored as a whole (ISO 11783-6 J.5)
			LOG_WARNING("[VT Server]: Client %u preferred assignment command: rejected with error bits 0x%02X, the first faulty function is %u.", workingSet->get_control_function()->get_address(), errorBitfield, faultyFunctionObjectID);
		}
		send_preferred_assignment_response(errorBitfield, faultyFunctionObjectID, message.get_source_control_function());
	}

	std::uint8_t VirtualTerminalServer::validate_preferred_assignment_command(const std::vector<std::uint8_t> &data,
	                                                                          VirtualTerminalServerManagedWorkingSet &workingSet,
	                                                                          std::vector<VirtualTerminalServerManagedWorkingSet::PreferredAuxiliaryAssignment> &assignments,
	                                                                          std::uint16_t &faultyFunctionObjectID) const
	{
		constexpr std::size_t INPUT_UNIT_LENGTH = 11; // NAME, Model Identification Code and number of functions
		constexpr std::size_t ASSIGNMENT_LENGTH = 4; // Auxiliary Function and Auxiliary Input object IDs
		const std::uint8_t numberOfInputUnits = data.at(1);
		std::vector<std::uint16_t> functionObjectIDs;
		std::uint8_t errorBitfield = 0;
		std::size_t offset = 2;
		bool isTruncated = false;

		faultyFunctionObjectID = NULL_OBJECT_ID;
		for (std::uint8_t unit = 0; (unit < numberOfInputUnits) && (!isTruncated); unit++)
		{
			isTruncated = (data.size() < (offset + INPUT_UNIT_LENGTH));
			if (isTruncated)
			{
				break;
			}

			const std::uint64_t inputUnitName = get_little_endian_uint64(data, offset);
			const std::uint16_t modelIdentificationCode = get_little_endian_uint16(data, offset + 8);
			const std::uint8_t numberOfAssignments = data[offset + 10];
			const auto inputUnit = get_managed_working_set(inputUnitName);
			const bool isInputUnitValid = (nullptr != inputUnit) && (modelIdentificationCode == inputUnit->get_auxiliary_input_model_identification_code());
			// A unit that is not Ready gets its inputs reported, not itself (ISO 11783-6 J.7.7)
			const bool isInputUnitReady = isInputUnitValid && inputUnit->get_is_auxiliary_input_ready();
			offset += INPUT_UNIT_LENGTH;

			if (!isInputUnitValid)
			{
				errorBitfield |= get_bit(static_cast<std::uint8_t>(PreferredAssignmentErrorBit::AuxiliaryInputUnitNotValid));
			}

			for (std::uint8_t i = 0; (i < numberOfAssignments) && (!isTruncated); i++)
			{
				isTruncated = (data.size() < (offset + ASSIGNMENT_LENGTH));
				if (isTruncated)
				{
					break;
				}

				VirtualTerminalServerManagedWorkingSet::PreferredAuxiliaryAssignment assignment;
				std::uint8_t assignmentErrorBitfield = 0;
				assignment.inputUnitName = inputUnitName;
				assignment.inputUnitModelIdentificationCode = modelIdentificationCode;
				assignment.functionObjectID = get_little_endian_uint16(data, offset);
				assignment.inputObjectID = get_little_endian_uint16(data, offset + 2);
				offset += ASSIGNMENT_LENGTH;

				if (!isInputUnitValid)
				{
					assignmentErrorBitfield |= get_bit(static_cast<std::uint8_t>(PreferredAssignmentErrorBit::AuxiliaryInputUnitNotValid));
				}
				else if ((!isInputUnitReady) || (!has_object_of_type(*inputUnit, assignment.inputObjectID, VirtualTerminalObjectType::AuxiliaryInputType2)))
				{
					assignmentErrorBitfield |= get_bit(static_cast<std::uint8_t>(PreferredAssignmentErrorBit::InputObjectIDNotValid));
				}

				if (!has_object_of_type(workingSet, assignment.functionObjectID, VirtualTerminalObjectType::AuxiliaryFunctionType2))
				{
					assignmentErrorBitfield |= get_bit(static_cast<std::uint8_t>(PreferredAssignmentErrorBit::FunctionObjectIDNotValid));
				}

				if (functionObjectIDs.end() != std::find(functionObjectIDs.begin(), functionObjectIDs.end(), assignment.functionObjectID))
				{
					assignmentErrorBitfield |= get_bit(static_cast<std::uint8_t>(PreferredAssignmentErrorBit::DuplicateFunctionObjectID));
				}

				if ((0 != assignmentErrorBitfield) && (NULL_OBJECT_ID == faultyFunctionObjectID))
				{
					faultyFunctionObjectID = assignment.functionObjectID;
				}
				errorBitfield |= assignmentErrorBitfield;
				functionObjectIDs.push_back(assignment.functionObjectID);
				assignments.push_back(assignment);
			}
		}

		if (isTruncated)
		{
			errorBitfield |= get_bit(static_cast<std::uint8_t>(PreferredAssignmentErrorBit::AnyOtherError));
		}
		return errorBitfield;
	}

	VirtualTerminalServer::AuxiliaryAssignmentResult VirtualTerminalServer::check_auxiliary_assignment(std::shared_ptr<VirtualTerminalServerManagedWorkingSet> functionWorkingSet,
	                                                                                                   std::uint16_t functionObjectID,
	                                                                                                   std::shared_ptr<VirtualTerminalServerManagedWorkingSet> inputWorkingSet,
	                                                                                                   std::uint16_t inputObjectID) const
	{
		LOCK_GUARD(Mutex, auxiliaryAssignmentMutex);
		return check_auxiliary_assignment_locked(functionWorkingSet, functionObjectID, inputWorkingSet, inputObjectID, true);
	}

	VirtualTerminalServer::AuxiliaryAssignmentResult VirtualTerminalServer::assign_auxiliary_function(std::shared_ptr<VirtualTerminalServerManagedWorkingSet> functionWorkingSet,
	                                                                                                  std::uint16_t functionObjectID,
	                                                                                                  std::shared_ptr<VirtualTerminalServerManagedWorkingSet> inputWorkingSet,
	                                                                                                  std::uint16_t inputObjectID)
	{
		LOCK_GUARD(Mutex, auxiliaryAssignmentMutex);
		const auto result = check_auxiliary_assignment_locked(functionWorkingSet, functionObjectID, inputWorkingSet, inputObjectID, true);

		if (AuxiliaryAssignmentResult::Allowed == result)
		{
			set_auxiliary_assignment_locked(get_auxiliary_function_state_locked(functionWorkingSet, functionObjectID), inputWorkingSet, inputObjectID, true);
			update_auxiliary_input_states_locked();
		}
		return result;
	}

	VirtualTerminalServer::AuxiliaryAssignmentResult VirtualTerminalServer::unassign_auxiliary_function(std::shared_ptr<VirtualTerminalServerManagedWorkingSet> functionWorkingSet, std::uint16_t functionObjectID)
	{
		LOCK_GUARD(Mutex, auxiliaryAssignmentMutex);
		auto result = AuxiliaryAssignmentResult::Allowed;

		if (!get_is_primary_vt())
		{
			result = AuxiliaryAssignmentResult::NotPrimaryVT;
		}
		else if ((nullptr == functionWorkingSet) || (!has_object_of_type(*functionWorkingSet, functionObjectID, VirtualTerminalObjectType::AuxiliaryFunctionType2)))
		{
			result = AuxiliaryAssignmentResult::InvalidFunction;
		}
		else if (!functionWorkingSet->get_was_preferred_assignment_received())
		{
			result = AuxiliaryAssignmentResult::NoPreferredAssignment;
		}
		else
		{
			set_auxiliary_assignment_locked(get_auxiliary_function_state_locked(functionWorkingSet, functionObjectID), nullptr, NULL_OBJECT_ID, true);
			update_auxiliary_input_states_locked();
		}
		return result;
	}

	std::vector<VirtualTerminalServer::AuxiliaryAssignment> VirtualTerminalServer::get_auxiliary_assignments() const
	{
		LOCK_GUARD(Mutex, auxiliaryAssignmentMutex);
		std::vector<AuxiliaryAssignment> assignments;

		for (const auto &state : auxiliaryFunctionStates)
		{
			assignments.push_back(state.assignment);
		}
		return assignments;
	}

	EventDispatcher<VirtualTerminalServer::AuxiliaryAssignment, VirtualTerminalServer::AuxiliaryAssignmentEvent> &VirtualTerminalServer::get_on_auxiliary_assignment_event_dispatcher()
	{
		return onAuxiliaryAssignmentEventDispatcher;
	}

	void VirtualTerminalServer::set_auxiliary_learn_mode(bool active)
	{
		if (active != isAuxiliaryLearnModeActive.exchange(active))
		{
			LOG_INFO("[VT Server]: Auxiliary controls learn mode is now %s.", active ? "on" : "off");
			if (send_status_message())
			{
				statusMessageTimestamp_ms = SystemTiming::get_timestamp_ms();
			}
		}
	}

	bool VirtualTerminalServer::get_auxiliary_learn_mode() const
	{
		return isAuxiliaryLearnModeActive;
	}

	EventDispatcher<VirtualTerminalServer::AuxiliaryInputStatus> &VirtualTerminalServer::get_on_auxiliary_input_status_event_dispatcher()
	{
		return onAuxiliaryInputStatusEventDispatcher;
	}

	void VirtualTerminalServer::process_auxiliary_input_status_message(const CANMessage &message, std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet)
	{
		const auto &data = message.get_data();
		AuxiliaryInputStatus status;
		status.inputWorkingSet = workingSet;
		status.inputObjectID = get_little_endian_uint16(data, 1);
		status.value1 = get_little_endian_uint16(data, 3);
		status.value2 = get_little_endian_uint16(data, 5);
		status.isActivated = (0 != (data[7] & 0x02));
		status.isLocked = (0 != (data[7] & 0x04));
		status.isInteractionDetectedWhileLocked = (0 != (data[7] & 0x08));

		// Outside of learn mode the status is meant for the implements, which receive it globally (ISO 11783-6 J.7.9)
		if (isAuxiliaryLearnModeActive && has_object_of_type(*workingSet, status.inputObjectID, VirtualTerminalObjectType::AuxiliaryInputType2))
		{
			LOCK_GUARD(Mutex, auxiliaryAssignmentMutex);
			pendingAuxiliaryInputStatuses.push_back(status);
		}
	}

	/// @brief Groups the auxiliary objects of one type in a working set's object pool into the sets of an Auxiliary Capabilities response.
	/// @param workingSet The working set to list.
	/// @param type AuxiliaryInputType2 or AuxiliaryFunctionType2.
	/// @param assignments The assignments the server made, to fill in the assigned attribute.
	/// @returns Three bytes per set: the number of instances, the function attribute and the assigned attribute (ISO 11783-6 Table J.8).
	static std::vector<std::uint8_t> get_auxiliary_capability_sets(VirtualTerminalServerManagedWorkingSet &workingSet, VirtualTerminalObjectType type, const std::vector<VirtualTerminalServer::AuxiliaryAssignment> &assignments)
	{
		const bool isFunction = (VirtualTerminalObjectType::AuxiliaryFunctionType2 == type);
		std::vector<std::uint8_t> sets;

		// Only object pools the VT has answered count as sent (ISO 11783-6 J.7.14)
		if (VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Joined == workingSet.get_object_pool_processing_state())
		{
			for (const auto &object : workingSet.get_object_tree())
			{
				std::uint32_t functionAttribute = 0;

				// Both object types have their function attributes as attribute 2
				if ((nullptr != object.second) &&
				    (type == object.second->get_object_type()) &&
				    object.second->get_attribute(static_cast<std::uint8_t>(AuxiliaryInputType2::AttributeName::FunctionAttributes), functionAttribute))
				{
					bool isAssigned = false;

					for (const auto &assignment : assignments)
					{
						const auto &assignedWorkingSet = isFunction ? assignment.functionWorkingSet : assignment.inputWorkingSet;
						const auto assignedObjectID = isFunction ? assignment.functionObjectID : assignment.inputObjectID;
						isAssigned = isAssigned || ((&workingSet == assignedWorkingSet.get()) && (object.first == assignedObjectID) && assignment.isAcknowledged && (!assignment.hasFailed) && (nullptr != assignment.inputWorkingSet));
					}

					const auto assignedAttribute = static_cast<std::uint8_t>((isFunction ? 0x01 : 0x00) | (isAssigned ? 0x02 : 0x00));
					bool isCounted = false;

					for (std::size_t i = 0; (i < sets.size()) && (!isCounted); i += 3)
					{
						isCounted = (static_cast<std::uint8_t>(functionAttribute) == sets[i + 1]) && (assignedAttribute == sets[i + 2]) && (sets[i] < 0xFF);
						if (isCounted)
						{
							sets[i]++;
						}
					}

					if ((!isCounted) && (sets.size() < (3 * 0xFF)))
					{
						sets.insert(sets.end(), { 1, static_cast<std::uint8_t>(functionAttribute), assignedAttribute });
					}
				}
			}
		}
		return sets;
	}

	bool VirtualTerminalServer::get_is_primary_vt() const
	{
		return (nullptr != serverInternalControlFunction) && (0 == serverInternalControlFunction->get_NAME().get_function_instance());
	}

	/// @brief Checks if an assignment drives a function with an input.
	/// @param assignment The assignment to check.
	/// @param inputWorkingSet The working set that provides the input.
	/// @param inputObjectID The Auxiliary Input Type 2 object.
	/// @returns true if the assignment uses the input and has not failed, otherwise false.
	static bool get_does_assignment_use_input(const VirtualTerminalServer::AuxiliaryAssignment &assignment, const std::shared_ptr<VirtualTerminalServerManagedWorkingSet> &inputWorkingSet, std::uint16_t inputObjectID)
	{
		return (!assignment.hasFailed) && (nullptr != inputWorkingSet) && (inputWorkingSet == assignment.inputWorkingSet) && (inputObjectID == assignment.inputObjectID);
	}

	/// @brief Checks if the function of an assignment is missing from the new object pool of its working set.
	/// @param assignment The assignment to check.
	/// @param workingSet The working set whose object pool was parsed.
	/// @returns true if the assignment is for a function of the working set that is no longer an Auxiliary Function Type 2, otherwise false.
	static bool get_is_function_gone_from_object_pool(const VirtualTerminalServer::AuxiliaryAssignment &assignment, const std::shared_ptr<VirtualTerminalServerManagedWorkingSet> &workingSet)
	{
		return (workingSet == assignment.functionWorkingSet) && (!has_object_of_type(*workingSet, assignment.functionObjectID, VirtualTerminalObjectType::AuxiliaryFunctionType2));
	}

	/// @brief Checks if an Auxiliary Function has the Single Assignment attribute.
	/// @param workingSet The working set that provides the function.
	/// @param functionObjectID The Auxiliary Function Type 2 object.
	/// @returns true if the function exists and has the attribute, otherwise false.
	static bool get_is_single_assignment_function(VirtualTerminalServerManagedWorkingSet &workingSet, std::uint16_t functionObjectID)
	{
		return has_object_of_type(workingSet, functionObjectID, VirtualTerminalObjectType::AuxiliaryFunctionType2) &&
		  std::static_pointer_cast<AuxiliaryFunctionType2>(workingSet.get_object_by_id(functionObjectID))->get_function_attribute(AuxiliaryFunctionType2::SingleAssignment);
	}

	/// @brief Checks if a Preferred Assignment command assigned a function to an input.
	/// @param functionWorkingSet The working set that provides the function.
	/// @param functionObjectID The Auxiliary Function Type 2 object.
	/// @param inputWorkingSet The working set that provides the input.
	/// @param inputObjectID The Auxiliary Input Type 2 object.
	/// @returns true if the last valid Preferred Assignment command of the function's working set holds the assignment, otherwise false.
	static bool get_is_preferred_assignment(VirtualTerminalServerManagedWorkingSet &functionWorkingSet, std::uint16_t functionObjectID, VirtualTerminalServerManagedWorkingSet &inputWorkingSet, std::uint16_t inputObjectID)
	{
		const std::uint64_t inputUnitName = inputWorkingSet.get_control_function()->get_NAME().get_full_name();
		bool retVal = false;

		for (const auto &assignment : functionWorkingSet.get_preferred_auxiliary_assignments())
		{
			retVal = retVal || ((functionObjectID == assignment.functionObjectID) && (inputObjectID == assignment.inputObjectID) && (inputUnitName == assignment.inputUnitName));
		}
		return retVal;
	}

	VirtualTerminalServer::AuxiliaryAssignmentResult VirtualTerminalServer::check_auxiliary_assignment_locked(const std::shared_ptr<VirtualTerminalServerManagedWorkingSet> &functionWorkingSet,
	                                                                                                          std::uint16_t functionObjectID,
	                                                                                                          const std::shared_ptr<VirtualTerminalServerManagedWorkingSet> &inputWorkingSet,
	                                                                                                          std::uint16_t inputObjectID,
	                                                                                                          bool isMadeByOperator) const
	{
		auto result = AuxiliaryAssignmentResult::Allowed;

		if (!get_is_primary_vt())
		{
			result = AuxiliaryAssignmentResult::NotPrimaryVT;
		}
		else if ((nullptr == functionWorkingSet) || (!has_object_of_type(*functionWorkingSet, functionObjectID, VirtualTerminalObjectType::AuxiliaryFunctionType2)))
		{
			result = AuxiliaryAssignmentResult::InvalidFunction;
		}
		else if (isMadeByOperator && (!functionWorkingSet->get_was_preferred_assignment_received()))
		{
			result = AuxiliaryAssignmentResult::NoPreferredAssignment;
		}
		else if ((nullptr == inputWorkingSet) || (!inputWorkingSet->get_is_auxiliary_input_ready()) || (!has_object_of_type(*inputWorkingSet, inputObjectID, VirtualTerminalObjectType::AuxiliaryInputType2)))
		{
			result = AuxiliaryAssignmentResult::InvalidInput;
		}
		else
		{
			const auto function = std::static_pointer_cast<AuxiliaryFunctionType2>(functionWorkingSet->get_object_by_id(functionObjectID));
			const auto input = std::static_pointer_cast<AuxiliaryInputType2>(inputWorkingSet->get_object_by_id(inputObjectID));
			const bool isSingleAssignment = function->get_function_attribute(AuxiliaryFunctionType2::SingleAssignment) || input->get_function_attribute(AuxiliaryInputType2::SingleAssignment);

			if (function->get_function_type() != input->get_function_type())
			{
				result = AuxiliaryAssignmentResult::TypeMismatch;
			}
			else if (function->get_function_attribute(AuxiliaryFunctionType2::CriticalControl) && (!input->get_function_attribute(AuxiliaryInputType2::CriticalControl)))
			{
				result = AuxiliaryAssignmentResult::CriticalControl;
			}
			else if (isMadeByOperator &&
			         function->get_function_attribute(AuxiliaryFunctionType2::AssignmentRestriction) &&
			         (!get_is_preferred_assignment(*functionWorkingSet, functionObjectID, *inputWorkingSet, inputObjectID)))
			{
				result = AuxiliaryAssignmentResult::AssignmentRestriction;
			}

			for (const auto &state : auxiliaryFunctionStates)
			{
				const auto &other = state.assignment;
				const bool isOtherFunction = (functionWorkingSet != other.functionWorkingSet) || (functionObjectID != other.functionObjectID);

				if ((AuxiliaryAssignmentResult::Allowed == result) &&
				    isOtherFunction &&
				    get_does_assignment_use_input(other, inputWorkingSet, inputObjectID) &&
				    (isSingleAssignment || get_is_single_assignment_function(*other.functionWorkingSet, other.functionObjectID)))
				{
					result = AuxiliaryAssignmentResult::SingleAssignment;
				}
			}
		}
		return result;
	}

	VirtualTerminalServer::AuxiliaryFunctionState &VirtualTerminalServer::get_auxiliary_function_state_locked(const std::shared_ptr<VirtualTerminalServerManagedWorkingSet> &functionWorkingSet, std::uint16_t functionObjectID)
	{
		for (auto &state : auxiliaryFunctionStates)
		{
			if ((functionWorkingSet == state.assignment.functionWorkingSet) && (functionObjectID == state.assignment.functionObjectID))
			{
				return state;
			}
		}

		// A new function starts unassigned but unacknowledged, so the removal is sent once (ISO 11783-6 J.5 step 9)
		AuxiliaryFunctionState newState;
		newState.assignment.functionWorkingSet = functionWorkingSet;
		newState.assignment.functionObjectID = functionObjectID;
		auxiliaryFunctionStates.push_back(newState);
		return auxiliaryFunctionStates.back();
	}

	void VirtualTerminalServer::set_auxiliary_assignment_locked(AuxiliaryFunctionState &state, const std::shared_ptr<VirtualTerminalServerManagedWorkingSet> &inputWorkingSet, std::uint16_t inputObjectID, bool isMadeByOperator)
	{
		auto &assignment = state.assignment;
		const bool isUnchanged = (inputWorkingSet == assignment.inputWorkingSet) && (inputObjectID == assignment.inputObjectID) && (!assignment.hasFailed);

		// An unchanged assignment from a Preferred Assignment command is not sent again, one from the operator is (ISO 11783-6 J.5 step 8)
		if ((!isUnchanged) || isMadeByOperator)
		{
			assignment.inputWorkingSet = inputWorkingSet;
			assignment.inputObjectID = inputObjectID;
			assignment.isMadeByOperator = isMadeByOperator;
			assignment.isAcknowledged = false;
			assignment.hasFailed = false;
			state.functionType = REMOVE_ASSIGNMENT_FUNCTION_TYPE;
			state.attempts = 0;
			state.isChangedWhileAwaitingResponse = state.isAwaitingResponse;

			if ((nullptr != inputWorkingSet) && has_object_of_type(*assignment.functionWorkingSet, assignment.functionObjectID, VirtualTerminalObjectType::AuxiliaryFunctionType2))
			{
				state.functionType = static_cast<std::uint8_t>(std::static_pointer_cast<AuxiliaryFunctionType2>(assignment.functionWorkingSet->get_object_by_id(assignment.functionObjectID))->get_function_type());
			}

			// An input that was refused gets another chance when a function is assigned to it again
			for (auto &input : auxiliaryInputStates)
			{
				if ((nullptr != inputWorkingSet) && (inputWorkingSet == input.inputWorkingSet) && (inputObjectID == input.inputObjectID) && input.hasFailed)
				{
					input.hasFailed = false;
					input.attempts = 0;
				}
			}
		}
	}

	void VirtualTerminalServer::remove_auxiliary_assignment_locked(AuxiliaryFunctionState &state, const std::shared_ptr<VirtualTerminalServerManagedWorkingSet> &inputWorkingSet, std::uint16_t inputObjectID)
	{
		auto removedAssignment = state.assignment;
		removedAssignment.inputWorkingSet = inputWorkingSet;
		removedAssignment.inputObjectID = inputObjectID;

		set_auxiliary_assignment_locked(state, nullptr, NULL_OBJECT_ID, false);
		pendingAuxiliaryAssignmentEvents.emplace_back(removedAssignment, AuxiliaryAssignmentEvent::Removed);
	}

	void VirtualTerminalServer::remove_auxiliary_assignments_of_input_unit_locked(const std::shared_ptr<VirtualTerminalServerManagedWorkingSet> &inputWorkingSet)
	{
		for (auto &state : auxiliaryFunctionStates)
		{
			if (inputWorkingSet == state.assignment.inputWorkingSet)
			{
				const auto inputObjectID = state.assignment.inputObjectID;
				remove_auxiliary_assignment_locked(state, inputWorkingSet, inputObjectID);
			}
		}

		auxiliaryInputStates.erase(std::remove_if(auxiliaryInputStates.begin(), auxiliaryInputStates.end(), [&inputWorkingSet](const AuxiliaryInputState &input) { return inputWorkingSet == input.inputWorkingSet; }),
		                           auxiliaryInputStates.end());
	}

	void VirtualTerminalServer::fail_auxiliary_assignments_of_input_locked(const AuxiliaryInputState &input)
	{
		for (auto &state : auxiliaryFunctionStates)
		{
			if (get_does_assignment_use_input(state.assignment, input.inputWorkingSet, input.inputObjectID))
			{
				state.assignment.hasFailed = true;
				pendingAuxiliaryAssignmentEvents.emplace_back(state.assignment, AuxiliaryAssignmentEvent::NotAvailable);
			}
		}
	}

	void VirtualTerminalServer::apply_preferred_auxiliary_assignments(std::shared_ptr<VirtualTerminalServerManagedWorkingSet> functionWorkingSet)
	{
		// Other VTs parse the auxiliary objects, but do not use them for assignments (ISO 11783-6 J.3.1)
		if (get_is_primary_vt())
		{
			const auto preferredAssignments = functionWorkingSet->get_preferred_auxiliary_assignments();
			LOCK_GUARD(Mutex, auxiliaryAssignmentMutex);

			for (const auto &object : functionWorkingSet->get_object_tree())
			{
				if ((nullptr != object.second) && (VirtualTerminalObjectType::AuxiliaryFunctionType2 == object.second->get_object_type()))
				{
					auto &state = get_auxiliary_function_state_locked(functionWorkingSet, object.first);
					std::shared_ptr<VirtualTerminalServerManagedWorkingSet> inputWorkingSet;
					std::uint16_t inputObjectID = NULL_OBJECT_ID;

					for (const auto &preferred : preferredAssignments)
					{
						if (object.first == preferred.functionObjectID)
						{
							inputWorkingSet = get_managed_working_set(preferred.inputUnitName);
							inputObjectID = preferred.inputObjectID;
						}
					}

					if (nullptr == inputWorkingSet)
					{
						// A function that is not in the command is unassigned (ISO 11783-6 J.7.7)
						set_auxiliary_assignment_locked(state, nullptr, NULL_OBJECT_ID, false);
					}
					else
					{
						const auto result = check_auxiliary_assignment_locked(functionWorkingSet, object.first, inputWorkingSet, inputObjectID, false);

						if (AuxiliaryAssignmentResult::Allowed == result)
						{
							set_auxiliary_assignment_locked(state, inputWorkingSet, inputObjectID, false);
						}
						else
						{
							// A conflicting assignment is removed, and the operator has to make it again (ISO 11783-6 J.5 step 7)
							LOG_WARNING("[VT Server]: Client %u preferred assignment of function %u to input %u conflicts (reason %u), removing it.", functionWorkingSet->get_control_function()->get_address(), object.first, inputObjectID, static_cast<std::uint8_t>(result));
							remove_auxiliary_assignment_locked(state, inputWorkingSet, inputObjectID);
						}
					}
				}
			}
			update_auxiliary_input_states_locked();
		}
	}

	void VirtualTerminalServer::remove_auxiliary_assignments(std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet)
	{
		LOCK_GUARD(Mutex, auxiliaryAssignmentMutex);

		auxiliaryFunctionStates.erase(std::remove_if(auxiliaryFunctionStates.begin(), auxiliaryFunctionStates.end(), [&workingSet](const AuxiliaryFunctionState &state) { return workingSet == state.assignment.functionWorkingSet; }),
		                              auxiliaryFunctionStates.end());
		remove_auxiliary_assignments_of_input_unit_locked(workingSet);
		update_auxiliary_input_states_locked();
	}

	void VirtualTerminalServer::revalidate_auxiliary_assignments(std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet)
	{
		LOCK_GUARD(Mutex, auxiliaryAssignmentMutex);

		// Functions that the new object pool no longer has are forgotten
		auxiliaryFunctionStates.erase(std::remove_if(auxiliaryFunctionStates.begin(), auxiliaryFunctionStates.end(), [&workingSet](const AuxiliaryFunctionState &state) { return get_is_function_gone_from_object_pool(state.assignment, workingSet); }),
		                              auxiliaryFunctionStates.end());

		for (auto &state : auxiliaryFunctionStates)
		{
			const auto inputWorkingSet = state.assignment.inputWorkingSet;
			const auto inputObjectID = state.assignment.inputObjectID;

			if ((nullptr != inputWorkingSet) &&
			    ((workingSet == state.assignment.functionWorkingSet) || (workingSet == inputWorkingSet)) &&
			    (AuxiliaryAssignmentResult::Allowed != check_auxiliary_assignment_locked(state.assignment.functionWorkingSet, state.assignment.functionObjectID, inputWorkingSet, inputObjectID, false)))
			{
				LOG_WARNING("[VT Server]: The object pool of client %u invalidates the assignment of function %u to input %u, removing it.", workingSet->get_control_function()->get_address(), state.assignment.functionObjectID, inputObjectID);
				remove_auxiliary_assignment_locked(state, inputWorkingSet, inputObjectID);
			}
		}
		update_auxiliary_input_states_locked();
	}

	void VirtualTerminalServer::update_auxiliary_input_states_locked()
	{
		for (const auto &state : auxiliaryFunctionStates)
		{
			const auto &assignment = state.assignment;

			if ((!assignment.hasFailed) &&
			    (nullptr != assignment.inputWorkingSet) &&
			    (auxiliaryInputStates.end() == std::find_if(auxiliaryInputStates.begin(), auxiliaryInputStates.end(), [&assignment](const AuxiliaryInputState &input) { return get_does_assignment_use_input(assignment, input.inputWorkingSet, input.inputObjectID); })))
			{
				AuxiliaryInputState newInput;
				newInput.inputWorkingSet = assignment.inputWorkingSet;
				newInput.inputObjectID = assignment.inputObjectID;
				auxiliaryInputStates.push_back(newInput);
			}
		}

		for (auto &input : auxiliaryInputStates)
		{
			bool isUsed = false;

			for (const auto &state : auxiliaryFunctionStates)
			{
				isUsed = isUsed || get_does_assignment_use_input(state.assignment, input.inputWorkingSet, input.inputObjectID);
			}

			if (isUsed != input.shouldBeEnabled)
			{
				input.shouldBeEnabled = isUsed;
				input.attempts = 0;
				input.hasFailed = false;
			}
		}
	}

	bool VirtualTerminalServer::get_is_auxiliary_input_enabled_locked(const std::shared_ptr<VirtualTerminalServerManagedWorkingSet> &inputWorkingSet, std::uint16_t inputObjectID) const
	{
		bool retVal = false;

		for (const auto &input : auxiliaryInputStates)
		{
			retVal = retVal || ((inputWorkingSet == input.inputWorkingSet) && (inputObjectID == input.inputObjectID) && input.isEnabled && (!input.hasFailed));
		}
		return retVal;
	}

	void VirtualTerminalServer::collect_auxiliary_input_commands_locked(std::vector<AuxiliaryCommand> &commands)
	{
		for (auto &input : auxiliaryInputStates)
		{
			if (input.isAwaitingResponse && SystemTiming::time_expired_ms(input.commandTimestamp_ms, AUXILIARY_COMMAND_TIMEOUT_MS))
			{
				input.isAwaitingResponse = false;
				if (input.attempts >= AUXILIARY_COMMAND_ATTEMPTS)
				{
					// The operator is alerted that the input is not available (ISO 11783-6 J.7.11)
					LOG_WARNING("[VT Server]: Client %u did not answer the status enable command for input %u, it is not available.", input.inputWorkingSet->get_control_function()->get_address(), input.inputObjectID);
					input.hasFailed = true;
					fail_auxiliary_assignments_of_input_locked(input);
				}
			}

			if ((!input.hasFailed) && (!input.isAwaitingResponse) && (input.shouldBeEnabled != input.isEnabled))
			{
				AuxiliaryCommand command;
				command.destination = input.inputWorkingSet->get_control_function();
				command.inputObjectID = input.inputObjectID;
				command.flag = input.shouldBeEnabled;
				commands.push_back(command);
				input.isAwaitingResponse = true;
				input.commandTimestamp_ms = SystemTiming::get_timestamp_ms();
				input.attempts++;
			}
		}

		// An input that no assignment uses is forgotten once it is disabled, or could not be enabled
		auxiliaryInputStates.erase(std::remove_if(auxiliaryInputStates.begin(), auxiliaryInputStates.end(), [](const AuxiliaryInputState &input) { return (!input.shouldBeEnabled) && (!input.isAwaitingResponse) && ((!input.isEnabled) || input.hasFailed); }),
		                           auxiliaryInputStates.end());
	}

	void VirtualTerminalServer::collect_auxiliary_assignment_commands_locked(std::vector<AuxiliaryCommand> &commands)
	{
		std::vector<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>> busyWorkingSets;

		for (auto &state : auxiliaryFunctionStates)
		{
			if (state.isAwaitingResponse && SystemTiming::time_expired_ms(state.commandTimestamp_ms, AUXILIARY_COMMAND_TIMEOUT_MS))
			{
				state.isAwaitingResponse = false;
				if (state.isChangedWhileAwaitingResponse)
				{
					state.isChangedWhileAwaitingResponse = false;
				}
				else if (state.attempts >= AUXILIARY_COMMAND_ATTEMPTS)
				{
					// The operator is alerted that the function is not available (ISO 11783-6 J.7.5)
					LOG_WARNING("[VT Server]: Client %u did not answer the assignment command for function %u, it is not available.", state.assignment.functionWorkingSet->get_control_function()->get_address(), state.assignment.functionObjectID);
					state.assignment.hasFailed = true;
					pendingAuxiliaryAssignmentEvents.emplace_back(state.assignment, AuxiliaryAssignmentEvent::NotAvailable);
				}
			}

			if (state.isAwaitingResponse)
			{
				busyWorkingSets.push_back(state.assignment.functionWorkingSet);
			}
		}

		for (auto &state : auxiliaryFunctionStates)
		{
			const auto &assignment = state.assignment;
			// An input is only assigned once its status message is enabled (ISO 11783-6 J.7.11)
			const bool isDue = (!assignment.isAcknowledged) &&
			  (!assignment.hasFailed) &&
			  (!state.isAwaitingResponse) &&
			  ((nullptr == assignment.inputWorkingSet) || get_is_auxiliary_input_enabled_locked(assignment.inputWorkingSet, assignment.inputObjectID));

			// A working set only has one assignment command to answer at a time (ISO 11783-6 J.7.5)
			if (isDue && (busyWorkingSets.end() == std::find(busyWorkingSets.begin(), busyWorkingSets.end(), assignment.functionWorkingSet)))
			{
				AuxiliaryCommand command;
				command.isAssignment = true;
				command.destination = assignment.functionWorkingSet->get_control_function();
				command.functionObjectID = assignment.functionObjectID;
				command.flag = assignment.isMadeByOperator;
				command.inputUnitName = std::numeric_limits<std::uint64_t>::max();

				if (nullptr != assignment.inputWorkingSet)
				{
					command.inputUnitName = assignment.inputWorkingSet->get_control_function()->get_NAME().get_full_name();
					command.inputObjectID = assignment.inputObjectID;
					command.functionType = state.functionType;
				}
				commands.push_back(command);
				state.isAwaitingResponse = true;
				state.commandTimestamp_ms = SystemTiming::get_timestamp_ms();
				state.attempts++;
				busyWorkingSets.push_back(assignment.functionWorkingSet);
			}
		}
	}

	void VirtualTerminalServer::update_auxiliary_assignments()
	{
		std::vector<AuxiliaryCommand> commands;
		std::vector<std::pair<AuxiliaryAssignment, AuxiliaryAssignmentEvent>> events;
		std::vector<AuxiliaryInputStatus> inputStatuses;
		{
			LOCK_GUARD(Mutex, auxiliaryAssignmentMutex);
			std::vector<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>> silentInputUnits;

			for (const auto &state : auxiliaryFunctionStates)
			{
				const auto &inputWorkingSet = state.assignment.inputWorkingSet;

				if ((nullptr != inputWorkingSet) &&
				    SystemTiming::time_expired_ms(inputWorkingSet->get_auxiliary_input_maintenance_timestamp_ms(), AUXILIARY_INPUT_MAINTENANCE_TIMEOUT_MS) &&
				    (silentInputUnits.end() == std::find(silentInputUnits.begin(), silentInputUnits.end(), inputWorkingSet)))
				{
					silentInputUnits.push_back(inputWorkingSet);
				}
			}

			for (const auto &inputWorkingSet : silentInputUnits)
			{
				// It may have shut down or be reconfiguring, so it has to report Ready again before its inputs are assigned (ISO 11783-6 J.7.10)
				LOG_WARNING("[VT Server]: Client %u has not sent an auxiliary input maintenance message for %u ms, removing the assignments to its inputs.", inputWorkingSet->get_control_function()->get_address(), AUXILIARY_INPUT_MAINTENANCE_TIMEOUT_MS);
				inputWorkingSet->set_auxiliary_input_maintenance_status(inputWorkingSet->get_auxiliary_input_model_identification_code(), false);
				remove_auxiliary_assignments_of_input_unit_locked(inputWorkingSet);
			}
			if (!silentInputUnits.empty())
			{
				update_auxiliary_input_states_locked();
			}

			// An assignment is removed before its input is disabled (ISO 11783-6 J.6 rule 9), and only made once its input is enabled
			collect_auxiliary_assignment_commands_locked(commands);
			collect_auxiliary_input_commands_locked(commands);
			events.swap(pendingAuxiliaryAssignmentEvents);
			inputStatuses.swap(pendingAuxiliaryInputStatuses);
		}

		// Sent outside of the lock, as the application can be waiting for it to make an assignment
		for (const auto &command : commands)
		{
			if (command.isAssignment)
			{
				send_auxiliary_assignment_command(command.inputUnitName, command.flag, command.functionType, command.inputObjectID, command.functionObjectID, command.destination);
			}
			else
			{
				send_auxiliary_input_status_enable_command(command.inputObjectID, command.flag, command.destination);
			}
		}

		for (const auto &event : events)
		{
			onAuxiliaryAssignmentEventDispatcher.call(event.first, event.second);
		}

		for (const auto &status : inputStatuses)
		{
			onAuxiliaryInputStatusEventDispatcher.call(status);
		}
	}

	void VirtualTerminalServer::process_auxiliary_assignment_response(const CANMessage &message, std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet)
	{
		const auto &data = message.get_data();
		const std::uint16_t functionObjectID = get_little_endian_uint16(data, 1);
		// Bit 1 of VT version 5 and prior only tells that the function was already assigned to that input, which is not an error (J.7.6)
		const bool isRejected = (0 != (data[3] & 0x01));
		LOCK_GUARD(Mutex, auxiliaryAssignmentMutex);

		for (auto &state : auxiliaryFunctionStates)
		{
			if ((workingSet == state.assignment.functionWorkingSet) && (functionObjectID == state.assignment.functionObjectID) && state.isAwaitingResponse)
			{
				state.isAwaitingResponse = false;
				if (state.isChangedWhileAwaitingResponse)
				{
					// This answers a previous assignment, the current one is sent next
					state.isChangedWhileAwaitingResponse = false;
				}
				else if (isRejected)
				{
					LOG_WARNING("[VT Server]: Client %u rejected the assignment of function %u.", workingSet->get_control_function()->get_address(), functionObjectID);
					state.assignment.hasFailed = true;
					pendingAuxiliaryAssignmentEvents.emplace_back(state.assignment, AuxiliaryAssignmentEvent::Rejected);
					update_auxiliary_input_states_locked();
				}
				else
				{
					state.assignment.isAcknowledged = true;
					if (nullptr != state.assignment.inputWorkingSet)
					{
						pendingAuxiliaryAssignmentEvents.emplace_back(state.assignment, AuxiliaryAssignmentEvent::Assigned);
					}
					else if (state.assignment.isMadeByOperator)
					{
						pendingAuxiliaryAssignmentEvents.emplace_back(state.assignment, AuxiliaryAssignmentEvent::Unassigned);
					}
				}
				break;
			}
		}
	}

	void VirtualTerminalServer::process_auxiliary_input_status_enable_response(const CANMessage &message, std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet)
	{
		const auto &data = message.get_data();
		const std::uint16_t inputObjectID = get_little_endian_uint16(data, 1);
		const bool isEnabled = (1 == data[3]);
		const bool hasError = (0 != data[4]);
		LOCK_GUARD(Mutex, auxiliaryAssignmentMutex);

		for (auto &input : auxiliaryInputStates)
		{
			if ((workingSet == input.inputWorkingSet) && (inputObjectID == input.inputObjectID) && input.isAwaitingResponse)
			{
				input.isAwaitingResponse = false;
				if (hasError)
				{
					LOG_WARNING("[VT Server]: Client %u refused the status enable command for input %u with error bits 0x%02X, it is not available.", workingSet->get_control_function()->get_address(), inputObjectID, data[4]);
					input.hasFailed = true;
					fail_auxiliary_assignments_of_input_locked(input);
				}
				else
				{
					input.isEnabled = isEnabled;
				}
				break;
			}
		}
	}

	void VirtualTerminalServer::process_working_set_maintenance_message(const CANMessage &message)
	{
		const auto &data = message.get_data();
		const bool initiating = (0 != (data[1] & 0x01));
		const std::uint8_t clientVersion = data[2];
		// VT version 2 and prior clients have no Initiating bit, they send 0xFF there,
		// so every maintenance message they send looks like an initiating one.
		const bool isVersion2OrPriorClient = ((clientVersion < 3) || (0xFF == clientVersion));
		auto workingSet = get_managed_working_set(message.get_source_control_function());

		if (nullptr != workingSet)
		{
			// Only a version 3 or later client can tell us it has restarted
			if (initiating && !isVersion2OrPriorClient)
			{
				LOG_WARNING("[VT Server]: Client %u sent another initiating working set maintenance message, treating it as an unexpected shutdown", workingSet->get_control_function()->get_address());
				shut_down_working_set(workingSet, UnexpectedShutdownReason::RepeatedInitiatingMessage);
				workingSet = nullptr;
			}
			else
			{
				workingSet->set_working_set_maintenance_message_timestamp_ms(SystemTiming::get_timestamp_ms());
			}
		}

		if ((nullptr == workingSet) && initiating)
		{
			auto announcement = std::find(announcedWorkingSetMasters.begin(), announcedWorkingSetMasters.end(), message.get_source_control_function());

			// A version 2 or prior client sends a Working Set Master message when it really starts up.
			// Without one, this is a working set that timed out and still thinks its object pool is loaded.
			if (isVersion2OrPriorClient && (announcedWorkingSetMasters.end() == announcement))
			{
				LOG_DEBUG("[VT Server]: Ignoring a version 2 or prior working set maintenance message from client %u, it has not sent a Working Set Master message", message.get_source_control_function()->get_address());
			}
			else
			{
				if (announcedWorkingSetMasters.end() != announcement)
				{
					announcedWorkingSetMasters.erase(announcement);
				}

				// This CF is trying to initiate communication with us.
				auto newWorkingSet = std::make_shared<VirtualTerminalServerManagedWorkingSet>(message.get_source_control_function());
				newWorkingSet->set_working_set_maintenance_message_timestamp_ms(SystemTiming::get_timestamp_ms());
				managedWorkingSetList.emplace_back(newWorkingSet);

				LOG_INFO("[VT Server]: Client %u initiated working set maintenance messages with version %u", newWorkingSet->get_control_function()->get_address(), clientVersion);
				if (clientVersion > get_vt_version_byte(get_version()))
				{
					LOG_WARNING("[VT Server]: Client %u version %u is higher than our reported version, which is %u", newWorkingSet->get_control_function()->get_address(), clientVersion, get_vt_version_byte(get_version()));
				}
			}
		}
	}

	void VirtualTerminalServer::process_working_set_master_message(const CANMessage &message)
	{
		const auto source = message.get_source_control_function();

		// A managed working set master also sends this when another node requests it, which is not a restart
		if ((nullptr == get_managed_working_set(source)) &&
		    (announcedWorkingSetMasters.end() == std::find(announcedWorkingSetMasters.begin(), announcedWorkingSetMasters.end(), source)))
		{
			announcedWorkingSetMasters.push_back(source);
		}
	}

	void VirtualTerminalServer::shut_down_working_set(std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet, UnexpectedShutdownReason reason)
	{
		deactivate_working_set(workingSet);
		remove_auxiliary_assignments(workingSet);
		managedWorkingSetIopLoadStateMap.erase(workingSet);
		managedWorkingSetList.erase(std::remove(managedWorkingSetList.begin(), managedWorkingSetList.end(), workingSet), managedWorkingSetList.end());

		LOCK_GUARD(Mutex, pendingShutdownsMutex);
		pendingShutdowns.emplace_back(workingSet, reason);
	}

	void VirtualTerminalServer::deactivate_working_set(const std::shared_ptr<VirtualTerminalServerManagedWorkingSet> &workingSet)
	{
		if ((activeWorkingSet == workingSet) ||
		    (workingSet->get_control_function()->get_address() == activeWorkingSetMasterAddress))
		{
			activeWorkingSet = nullptr;
			activeWorkingSetMasterAddress = NULL_CAN_ADDRESS;
			activeWorkingSetDataMaskObjectID = NULL_OBJECT_ID;
			activeWorkingSetSoftkeyMaskObjectID = NULL_OBJECT_ID;

			if (send_status_message())
			{
				statusMessageTimestamp_ms = SystemTiming::get_timestamp_ms();
			}
		}
	}

	void VirtualTerminalServer::report_shut_down_working_sets()
	{
		std::vector<std::pair<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, UnexpectedShutdownReason>> parsedWorkingSets;
		{
			LOCK_GUARD(Mutex, pendingShutdownsMutex);
			auto stillParsing = std::stable_partition(pendingShutdowns.begin(), pendingShutdowns.end(), [](const std::pair<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, UnexpectedShutdownReason> &shutDown) {
				return VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Running == shutDown.first->get_object_pool_processing_state();
			});
			parsedWorkingSets.assign(stillParsing, pendingShutdowns.end());
			pendingShutdowns.erase(stillParsing, pendingShutdowns.end());
		}

		for (const auto &shutDown : parsedWorkingSets)
		{
			// The parsing thread uses the working set, so it has to be joined before the working set can be released
			shutDown.first->join_parsing_thread();
			handle_unexpected_working_set_shutdown(shutDown.first, shutDown.second);
		}
	}

	void VirtualTerminalServer::execute_macro_as_rx_message(const CANMessage &message)
	{
		if ((message.get_destination_control_function() == serverInternalControlFunction) &&
		    (message.get_source_control_function() != nullptr))
		{
			process_rx_message(message, this);
		}
	}

	bool VirtualTerminalServer::execute_macro(std::uint16_t objectIDOfMacro, std::shared_ptr<VirtualTerminalServerManagedWorkingSet> workingSet)
	{
		auto object = workingSet->get_object_by_id(objectIDOfMacro);
		bool retVal = false;

		if ((nullptr != object) && (VirtualTerminalObjectType::Macro == object->get_object_type()))
		{
			auto macro = std::static_pointer_cast<Macro>(object);

			if (macro->get_are_command_packets_valid())
			{
				LOG_DEBUG("[VT Server]: Executing macro %u", macro->get_id());
				retVal = true;

				for (std::uint8_t j = 0; j < macro->get_number_of_commands(); j++)
				{
					std::vector<std::uint8_t> commandPacket;

					if (macro->get_command_packet(j, commandPacket))
					{
						isobus::CANMessage message(isobus::CANMessage::Type::Receive,
						                           isobus::CANIdentifier(0x14E70000),
						                           commandPacket,
						                           workingSet->get_control_function(),
						                           get_internal_control_function(),
						                           workingSet->get_control_function()->get_can_port());
						LOG_DEBUG("[VT Server]: Executing macro #%u command code %u, len %u", j, commandPacket.size() > 0 ? commandPacket.at(0) : 'X', message.get_data_length());
						execute_macro_as_rx_message(message);
					}
				}
			}
		}
		return retVal;
	}

	CANIdentifier::CANPriority VirtualTerminalServer::get_priority() const
	{
		return (VTVersion::Version6 == get_version()) ? CANIdentifier::CANPriority::Priority5 : CANIdentifier::CANPriority::PriorityLowest7;
	}

	std::uint8_t VirtualTerminalServer::get_vt_version_byte(VTVersion version)
	{
		std::uint8_t retVal = 2;

		switch (version)
		{
			case VTVersion::Version3:
			{
				retVal = 3;
			}
			break;

			case VTVersion::Version4:
			{
				retVal = 4;
			}
			break;

			case VTVersion::Version5:
			{
				retVal = 5;
			}
			break;

			case VTVersion::Version6:
			{
				retVal = 6;
			}
			break;

			default:
			{
				// Report version 2
			}
			break;
		}
		return retVal;
	}

	bool VirtualTerminalServer::process_stateless_messages(const CANMessage &message)
	{
		bool retVal = false;
		const auto &data = message.get_data();
		switch (static_cast<Function>(data.at(0)))
		{
			case Function::GetMemoryMessage:
			{
				const auto requiredMemory = get_little_endian_uint32(data, 2);
				bool isEnoughMemory = get_is_enough_memory(requiredMemory);
				LOG_INFO("[VT Server]: An ecu requested %u bytes of memory.", requiredMemory);

				if (!isEnoughMemory)
				{
					LOG_WARNING("[VT Server]: Callback indicated there is NOT enough memory.", requiredMemory);
				}
				else
				{
					LOG_DEBUG("[VT Server]: Callback indicated there may be enough memory, but since there is overhead associated to object storage it is impossible to be sure.", requiredMemory);
				}

				std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };
				buffer[0] = static_cast<std::uint8_t>(Function::GetMemoryMessage);
				buffer[1] = get_vt_version_byte(get_version());
				buffer[2] = static_cast<std::uint8_t>(!isEnoughMemory);
				buffer[3] = 0xFF; // Reserved
				buffer[4] = 0xFF; // Reserved
				buffer[5] = 0xFF; // Reserved
				buffer[6] = 0xFF; // Reserved
				buffer[7] = 0xFF; // Reserved
				CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
				                                               buffer.data(),
				                                               CAN_DATA_LENGTH,
				                                               serverInternalControlFunction,
				                                               message.get_source_control_function(),
				                                               get_priority());
				retVal = true;
			}
			break;

			case Function::GetNumberOfSoftKeysMessage:
			{
				std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };
				buffer[0] = static_cast<std::uint8_t>(Function::GetNumberOfSoftKeysMessage);
				buffer[1] = get_number_of_navigation_soft_keys(); // No navigation softkeys
				buffer[2] = 0xFF; // Reserved
				buffer[3] = 0xFF; // Reserved
				buffer[4] = get_soft_key_descriptor_x_pixel_width(); // Width of the softkey descriptor in pixels
				buffer[5] = get_soft_key_descriptor_y_pixel_height(); // Height of the softkey descriptor in pixels
				buffer[6] = get_number_of_possible_virtual_soft_keys_in_soft_key_mask(); // Number of possible virtual Soft Keys in a Soft Key Mask
				buffer[7] = get_number_of_physical_soft_keys(); // No physical softkeys

				CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
				                                               buffer.data(),
				                                               CAN_DATA_LENGTH,
				                                               serverInternalControlFunction,
				                                               message.get_source_control_function(),
				                                               get_priority());
				retVal = true;
			}
			break;

			case Function::GetTextFontDataMessage:
			{
				std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };
				buffer[0] = static_cast<std::uint8_t>(Function::GetTextFontDataMessage);
				buffer[1] = 0xFF; // Reserved
				buffer[2] = 0xFF; // Reserved
				buffer[3] = 0xFF; // Reserved
				buffer[4] = 0xFF; // Reserved
				buffer[5] = get_supported_small_fonts_bitfield(); // Say we support all small fonts
				buffer[6] = get_supported_large_fonts_bitfield(); // Say we support all large fonts
				buffer[7] = 0x8F; // Support normal, bold, italic, proportional
				CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
				                                               buffer.data(),
				                                               CAN_DATA_LENGTH,
				                                               serverInternalControlFunction,
				                                               message.get_source_control_function(),
				                                               get_priority());
				retVal = true;
			}
			break;

			case Function::GetHardwareMessage:
			{
				std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };
				buffer[0] = static_cast<std::uint8_t>(Function::GetHardwareMessage);
				buffer[1] = get_powerup_time();
				buffer[2] = static_cast<std::uint8_t>(get_graphic_mode()); // 256 Colour Mode by default
				buffer[3] = 0x0F; // Support pointing event message
				buffer[4] = get_low_byte(get_data_mask_area_size_x_pixels()); // X Pixels LSB
				buffer[5] = get_high_byte(get_data_mask_area_size_x_pixels()); // X Pixels MSB
				buffer[6] = get_low_byte(get_data_mask_area_size_y_pixels()); // Y Pixels LSB
				buffer[7] = get_high_byte(get_data_mask_area_size_y_pixels()); // Y Pixels MSB
				CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
				                                               buffer.data(),
				                                               CAN_DATA_LENGTH,
				                                               serverInternalControlFunction,
				                                               message.get_source_control_function(),
				                                               get_priority());
				retVal = true;
			}
			break;

			case Function::GetSupportedWidecharsMessage:
			{
				std::vector<std::uint8_t> wideCharRangeArray;
				std::uint8_t numberOfRanges = 0;
				std::uint8_t codePlane = data.at(1);
				std::uint16_t firstWideCharInInquiryRange = get_little_endian_uint16(data, 2);
				std::uint16_t lastWideCharInInquiryRange = get_little_endian_uint16(data, 4);
				auto errorCode = get_supported_wide_chars(codePlane, firstWideCharInInquiryRange, lastWideCharInInquiryRange, numberOfRanges, wideCharRangeArray);

				std::vector<std::uint8_t> buffer;
				buffer.push_back(static_cast<std::uint8_t>(Function::GetSupportedWidecharsMessage));
				buffer.push_back(codePlane);
				buffer.push_back(get_low_byte(firstWideCharInInquiryRange));
				buffer.push_back(get_high_byte(firstWideCharInInquiryRange));
				buffer.push_back(get_low_byte(lastWideCharInInquiryRange));
				buffer.push_back(get_high_byte(lastWideCharInInquiryRange));
				buffer.push_back(static_cast<std::uint8_t>(errorCode));
				buffer.push_back(numberOfRanges);

				for (const auto &range : wideCharRangeArray)
				{
					buffer.push_back(range);
				}
				CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
				                                               buffer.data(),
				                                               static_cast<std::uint32_t>(buffer.size()),
				                                               serverInternalControlFunction,
				                                               message.get_source_control_function(),
				                                               get_priority());
				retVal = true;
			}
			break;

			case Function::GetWindowMaskDataMessage:
			{
				send_get_window_mask_data_response(message.get_source_control_function());
				retVal = true;
			}
			break;

			case Function::AuxiliaryCapabilitiesRequest:
			{
				// Any control function can ask, in VT version 5 and later (ISO 11783-6 J.7.13)
				if (get_vt_version_byte(get_version()) >= 5)
				{
					send_auxiliary_capabilities_response(data.at(1), message.get_source_control_function());
					retVal = true;
				}
			}
			break;

			default:
				break;
		}
		return retVal;
	}

	void VirtualTerminalServer::process_connection_dependent_messages(const CANMessage &message, std::shared_ptr<VirtualTerminalServerManagedWorkingSet> managedWorkingSet)
	{
		const auto &data = message.get_data();
		const auto function = static_cast<Function>(data.at(0));

		// The client must wait for the response first, and the parsing thread is still reading the pool data
		if (((Function::ObjectPoolTransferMessage == function) ||
		     (Function::EndOfObjectPoolMessage == function) ||
		     (Function::LoadVersionCommand == function) ||
		     (Function::ExtendedLoadVersionCommand == function)) &&
		    is_object_pool_parse_unanswered(*managedWorkingSet))
		{
			LOG_WARNING("[VT Server]: Ignoring object pool command %u from client %u, its previous object pool has not been answered yet.", data.at(0), message.get_identifier().get_source_address());
			return;
		}

		switch (function)
		{
			case Function::GetMemoryMessage:
			{
				// The response is handled in process_stateless_messages
				// but save the size requested for later use if we have a connected working set
				const auto requiredMemory = get_little_endian_uint32(data, 2);
				managedWorkingSet->set_iop_size(requiredMemory);
			}
			break;

			case Function::ObjectPoolTransferMessage:
			{
				std::vector<std::uint8_t> tempPool = data; // Make a copy of the data (ouch)
				tempPool.erase(tempPool.begin()); // Strip off the mux byte (double ouch, good thing this is rare)
				LOG_INFO("[VT Server]: An ecu at address %u transferred %u bytes of object pool data to us.", message.get_identifier().get_source_address(), static_cast<std::uint32_t>(tempPool.size()));
				managedWorkingSet->add_iop_raw_data(tempPool);
			}
			break;

			case Function::GetVersionsMessage:
			case Function::ExtendedGetVersionsMessage:
			{
				const NAME clientNAME = message.get_source_control_function()->get_NAME();

				LOG_DEBUG("[VT Server]: Client %u requests stored versions", message.get_source_control_function()->get_address());

				// The extended response reuses the request's function code, the standard one does not
				std::vector<std::uint8_t> buffer;
				if (Function::GetVersionsMessage == function)
				{
					buffer = build_versions_response(static_cast<std::uint8_t>(Function::GetVersionsResponse), get_versions(clientNAME));
				}
				else
				{
					buffer = build_versions_response(static_cast<std::uint8_t>(Function::ExtendedGetVersionsMessage), get_extended_versions(clientNAME));
				}

				CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
				                                               buffer.data(),
				                                               static_cast<std::uint32_t>(buffer.size()),
				                                               serverInternalControlFunction,
				                                               message.get_source_control_function(),
				                                               get_priority());
			}
			break;

			case Function::LoadVersionCommand:
			case Function::ExtendedLoadVersionCommand:
			{
				const bool isExtended = (Function::ExtendedLoadVersionCommand == function);
				const auto versionLabel = get_version_label(data, isExtended ? EXTENDED_VERSION_LABEL_LENGTH : VERSION_LABEL_LENGTH);

				if (!versionLabel.empty())
				{
					auto loadedVersion = load_version(versionLabel, message.get_source_control_function()->get_NAME());
					if (!loadedVersion.empty())
					{
						remove_auxiliary_assignments(managedWorkingSet);
						managedWorkingSet->clear_object_pool_data({});
						managedWorkingSet->set_iop_size(static_cast<std::uint32_t>(loadedVersion.size()));
						managedWorkingSet->add_iop_raw_data(loadedVersion);
						managedWorkingSet->set_was_object_pool_loaded_from_non_volatile_memory(true, isExtended, {});
						managedWorkingSet->start_parsing_thread();
						LOG_DEBUG("[VT Server]: Starting parsing thread for loaded pool data.");
					}
					else
					{
						send_load_version_response(function, get_bit(static_cast<std::uint8_t>(LoadVersionErrorBit::VersionLabelNotCorrectOrUnknown)), managedWorkingSet->get_control_function());
						LOG_ERROR("[VT Server]: Failed to load requested object pool version");
					}
				}
			}
			break;

			case Function::StoreVersionCommand:
			case Function::ExtendedStoreVersionCommand:
			{
				const auto versionLabel = get_version_label(data, (Function::ExtendedStoreVersionCommand == function) ? EXTENDED_VERSION_LABEL_LENGTH : VERSION_LABEL_LENGTH);

				if ((!versionLabel.empty()) && managedWorkingSet->get_any_object_pools())
				{
					std::ostringstream nameString;
					nameString << std::hex << std::setfill('0') << std::setw(16) << managedWorkingSet->get_control_function()->get_NAME().get_full_name();
					bool allPoolsSaved = true;

					// A Store replaces the version with the same label, and save_version() appends, so the label is emptied first.
					// The result is not checked, as nothing may be stored under the label yet
					delete_version(versionLabel, message.get_source_control_function()->get_NAME());

					for (std::size_t i = 0; i < managedWorkingSet->get_number_iop_files(); i++)
					{
						bool didSave = save_version(managedWorkingSet->get_iop_raw_data(i), versionLabel, message.get_source_control_function()->get_NAME());

						if (didSave)
						{
							LOG_INFO("[VT Server]: Object pool " + isobus::to_string(i) + " for NAME " + nameString.str() + " was stored.");
						}
						else
						{
							LOG_ERROR("[VT Server]: Object pool " + isobus::to_string(i) + " for NAME " + nameString.str() + " could not be stored.");
							allPoolsSaved = false;
							break;
						}
					}

					std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };
					buffer[0] = static_cast<std::uint8_t>(function);
					buffer[1] = 0xFF; // Reserved
					buffer[2] = 0xFF; // Reserved
					buffer[3] = 0xFF; // Reserved
					buffer[4] = 0xFF; // Reserved
					if (allPoolsSaved)
					{
						buffer[5] = 0; // No error
					}
					else
					{
						buffer[5] = get_bit(static_cast<std::uint8_t>(StoreVersionErrorBit::AnyOtherError));
					}
					buffer[6] = 0xFF; // Reserved
					buffer[7] = 0xFF; // Reserved
					CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
					                                               buffer.data(),
					                                               CAN_DATA_LENGTH,
					                                               serverInternalControlFunction,
					                                               message.get_source_control_function(),
					                                               get_priority());
				}
				else if (!versionLabel.empty())
				{
					// Whomever this is appears to be behaving badly, send them a NACK
					send_acknowledgement(AcknowledgementType::Negative, static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal), serverInternalControlFunction, managedWorkingSet->get_control_function());
				}
			}
			break;

			case Function::DeleteVersionCommand:
			case Function::ExtendedDeleteVersionCommand:
			{
				const std::uint8_t versionLabelLength = (Function::ExtendedDeleteVersionCommand == function) ? EXTENDED_VERSION_LABEL_LENGTH : VERSION_LABEL_LENGTH;
				const auto versionLabel = get_version_label(data, versionLabelLength);

				if (!versionLabel.empty())
				{
					std::ostringstream nameString;
					nameString << std::hex << std::setfill('0') << std::setw(16) << managedWorkingSet->get_control_function()->get_NAME().get_full_name();

					// The wildcard came with VT version 6, so a lower version VT treats it as an ordinary label
					const bool isDeleteAllWildcard = (VTVersion::Version6 == get_version()) &&
					  ('*' == versionLabel.front()) &&
					  std::all_of(versionLabel.begin() + 1, versionLabel.end(), [](std::uint8_t character) { return ' ' == character; });

					bool deleted = false;
					if (isDeleteAllWildcard)
					{
						deleted = delete_all_versions(versionLabelLength, managedWorkingSet->get_control_function()->get_NAME());
					}
					else
					{
						deleted = delete_version(versionLabel, managedWorkingSet->get_control_function()->get_NAME());
					}

					if (deleted)
					{
						LOG_INFO("[VT Server]: Deleted an object pool version for client NAME %s", nameString.str().c_str());
						send_delete_version_response(function, 0, managedWorkingSet->get_control_function());
					}
					else
					{
						LOG_WARNING("[VT Server]: Delete version failed for client NAME %s", nameString.str().c_str());
						const DeleteVersionErrorBit error = isDeleteAllWildcard ? DeleteVersionErrorBit::AnyOtherError : DeleteVersionErrorBit::VersionLabelNotCorrectOrUnknown;
						send_delete_version_response(function, get_bit(static_cast<std::uint8_t>(error)), managedWorkingSet->get_control_function());
					}
				}
			}
			break;

			case Function::EndOfObjectPoolMessage:
			{
				if (managedWorkingSet->get_any_object_pools())
				{
					managedWorkingSet->start_parsing_thread();
				}
				else
				{
					LOG_WARNING("[VT Server]: End of object pool message ignored - no object pools are loaded for the source control function");
				}
			}
			break;

			case Function::WorkingSetMaintenanceMessage:
			{
				// Already handled by process_working_set_maintenance_message()
			}
			break;

			case Function::ChangeNumericValueCommand:
			{
				const auto value = get_little_endian_uint32(data, 4);
				auto objectId = get_little_endian_uint16(data, 1);
				auto lTargetObject = managedWorkingSet->get_object_by_id(objectId);
				bool logSuccess = true;

				if (nullptr != lTargetObject)
				{
					switch (lTargetObject->get_object_type())
					{
						case VirtualTerminalObjectType::InputBoolean:
						{
							std::static_pointer_cast<InputBoolean>(lTargetObject)->set_value(get_byte(value, 0));
							onRepaintEventDispatcher.call(managedWorkingSet);
							send_change_numeric_value_response(objectId, 0, value, managedWorkingSet->get_control_function());
						}
						break;

						case VirtualTerminalObjectType::InputNumber:
						{
							std::static_pointer_cast<InputNumber>(lTargetObject)->set_value(value);
							onRepaintEventDispatcher.call(managedWorkingSet);
							send_change_numeric_value_response(objectId, 0, value, managedWorkingSet->get_control_function());
						}
						break;

						case VirtualTerminalObjectType::InputList:
						{
							std::static_pointer_cast<InputList>(lTargetObject)->set_value(get_byte(value, 0));
							onRepaintEventDispatcher.call(managedWorkingSet);
							send_change_numeric_value_response(objectId, 0, value, managedWorkingSet->get_control_function());
						}
						break;

						case VirtualTerminalObjectType::OutputNumber:
						{
							std::static_pointer_cast<OutputNumber>(lTargetObject)->set_value(value);
							onRepaintEventDispatcher.call(managedWorkingSet);
							send_change_numeric_value_response(objectId, 0, value, managedWorkingSet->get_control_function());
						}
						break;

						case VirtualTerminalObjectType::OutputList:
						{
							std::static_pointer_cast<OutputList>(lTargetObject)->set_value(get_byte(value, 0));
							onRepaintEventDispatcher.call(managedWorkingSet);
							send_change_numeric_value_response(objectId, 0, value, managedWorkingSet->get_control_function());
						}
						break;

						case VirtualTerminalObjectType::OutputMeter:
						{
							std::static_pointer_cast<OutputMeter>(lTargetObject)->set_value(get_low_16_bits(value));
							onRepaintEventDispatcher.call(managedWorkingSet);
							send_change_numeric_value_response(objectId, 0, value, managedWorkingSet->get_control_function());
						}
						break;

						case VirtualTerminalObjectType::OutputLinearBarGraph:
						{
							std::static_pointer_cast<OutputLinearBarGraph>(lTargetObject)->set_value(get_low_16_bits(value));
							onRepaintEventDispatcher.call(managedWorkingSet);
							send_change_numeric_value_response(objectId, 0, value, managedWorkingSet->get_control_function());
						}
						break;

						case VirtualTerminalObjectType::OutputArchedBarGraph:
						{
							std::static_pointer_cast<OutputArchedBarGraph>(lTargetObject)->set_value(get_low_16_bits(value));
							onRepaintEventDispatcher.call(managedWorkingSet);
							send_change_numeric_value_response(objectId, 0, value, managedWorkingSet->get_control_function());
						}
						break;

						case VirtualTerminalObjectType::NumberVariable:
						{
							std::static_pointer_cast<NumberVariable>(lTargetObject)->set_value(value);
							onRepaintEventDispatcher.call(managedWorkingSet);
							send_change_numeric_value_response(objectId, 0, value, managedWorkingSet->get_control_function());
						}
						break;

						case VirtualTerminalObjectType::ObjectPointer:
						{
							std::static_pointer_cast<ObjectPointer>(lTargetObject)->set_value(get_low_16_bits(value));
							onRepaintEventDispatcher.call(managedWorkingSet);
							send_change_numeric_value_response(objectId, 0, value, managedWorkingSet->get_control_function());
						}
						break;

						case VirtualTerminalObjectType::ExternalObjectPointer:
						{
							std::uint16_t externalReferenceNAMEObjectIdD = get_little_endian_uint16(data, 4);
							std::uint16_t referencedObjectID = get_little_endian_uint16(data, 6);
							std::static_pointer_cast<ExternalObjectPointer>(lTargetObject)->set_external_reference_name_id(externalReferenceNAMEObjectIdD);
							std::static_pointer_cast<ExternalObjectPointer>(lTargetObject)->set_external_object_id(referencedObjectID);
							send_change_numeric_value_response(objectId, 0, value, managedWorkingSet->get_control_function());
							// Todo: event dispatcher
						}
						break;

						case VirtualTerminalObjectType::Animation:
						{
							// Todo std::static_pointer_cast<Animation>(lTargetObject)->set_value(value);
							// onChangeNumericValueEventDispatcher.call(objectId, value);
							send_change_numeric_value_response(objectId, get_bit(static_cast<std::uint8_t>(ChangeNumericValueErrorBit::AnyOtherError)), value, managedWorkingSet->get_control_function());
							LOG_WARNING("[VT Server]: Client %u change numeric value for animation not implemented yet", managedWorkingSet->get_control_function()->get_address());
							logSuccess = false;
						}
						break;

						default:
						{
							send_change_numeric_value_response(objectId, get_bit(static_cast<std::uint8_t>(ChangeNumericValueErrorBit::InvalidObjectID)), value, managedWorkingSet->get_control_function());
							LOG_WARNING("[VT Server]: Client %u change numeric value invalid object type. ID: %u", managedWorkingSet->get_control_function()->get_address(), objectId);
							logSuccess = false;
						}
						break;
					}

					if (logSuccess)
					{
						LOG_DEBUG("[VT Server]: Client %u change numeric value command: change object ID %u to be %u", managedWorkingSet->get_control_function()->get_address(), objectId, value);
						process_macro(lTargetObject, isobus::EventID::OnChangeValue, lTargetObject->get_object_type(), managedWorkingSet);
					}
				}
				else
				{
					send_change_numeric_value_response(objectId, get_bit(static_cast<std::uint8_t>(ChangeNumericValueErrorBit::InvalidObjectID)), value, managedWorkingSet->get_control_function());
					LOG_WARNING("[VT Server]: Client %u change numeric value invalid object ID of %u", managedWorkingSet->get_control_function()->get_address(), objectId);
				}
			}
			break;

			case Function::HideShowObjectCommand:
			{
				auto objectId = get_little_endian_uint16(data, 1);
				auto targetObject = managedWorkingSet->get_object_by_id(objectId);

				if ((nullptr != targetObject) && (VirtualTerminalObjectType::Container == targetObject->get_object_type()))
				{
					std::static_pointer_cast<Container>(targetObject)->set_hidden(0 == data[3]);
					send_hide_show_object_response(objectId, 0, (0 != data[3]), managedWorkingSet->get_control_function());
					onRepaintEventDispatcher.call(managedWorkingSet);

					if (0 == data[3])
					{
						LOG_DEBUG("[VT Server]: Client %u hide object command %u", managedWorkingSet->get_control_function()->get_address(), objectId);
						process_macro(targetObject, EventID::OnHide, targetObject->get_object_type(), managedWorkingSet);
					}
					else
					{
						LOG_DEBUG("[VT Server]: Client %u show object command %u", managedWorkingSet->get_control_function()->get_address(), objectId);
						process_macro(targetObject, EventID::OnShow, targetObject->get_object_type(), managedWorkingSet);
					}
				}
				else
				{
					send_hide_show_object_response(objectId, get_bit(static_cast<std::uint8_t>(HideShowObjectErrorBit::InvalidObjectID)), (0 != data[3]), managedWorkingSet->get_control_function());
					LOG_WARNING("[VT Server]: Client %u hide/show object command failed. It can only affect containers! ID: %u", managedWorkingSet->get_control_function()->get_address(), objectId);
				}
			}
			break;

			case Function::EnableDisableObjectCommand:
			{
				auto objectId = get_little_endian_uint16(data, 1);
				auto lTargetObject = managedWorkingSet->get_object_by_id(objectId);

				if (nullptr != lTargetObject)
				{
					if (data[3] <= 1)
					{
						switch (lTargetObject->get_object_type())
						{
							case VirtualTerminalObjectType::InputBoolean:
							{
								std::static_pointer_cast<InputBoolean>(lTargetObject)->set_enabled(0 != data[3]);
								send_enable_disable_object_response(objectId, 0, (0 != data[3]), managedWorkingSet->get_control_function());
								onRepaintEventDispatcher.call(managedWorkingSet);
							}
							break;

							case VirtualTerminalObjectType::InputList:
							{
								std::static_pointer_cast<InputList>(lTargetObject)->set_option(InputList::Options::Enabled, (0 != data[3]));
								send_enable_disable_object_response(objectId, 0, (0 != data[3]), managedWorkingSet->get_control_function());
								onRepaintEventDispatcher.call(managedWorkingSet);
							}
							break;

							case VirtualTerminalObjectType::InputString:
							{
								std::static_pointer_cast<InputString>(lTargetObject)->set_enabled((0 != data[3]));
								send_enable_disable_object_response(objectId, 0, (0 != data[3]), managedWorkingSet->get_control_function());
								onRepaintEventDispatcher.call(managedWorkingSet);
							}
							break;

							case VirtualTerminalObjectType::InputNumber:
							{
								std::static_pointer_cast<InputNumber>(lTargetObject)->set_option2(InputNumber::Options2::Enabled, (0 != data[3]));
								send_enable_disable_object_response(objectId, 0, (0 != data[3]), managedWorkingSet->get_control_function());
								onRepaintEventDispatcher.call(managedWorkingSet);
							}
							break;

							case VirtualTerminalObjectType::Button:
							{
								std::static_pointer_cast<Button>(lTargetObject)->set_option(Button::Options::Disabled, (0 == data[3]));
								send_enable_disable_object_response(objectId, 0, (0 != data[3]), managedWorkingSet->get_control_function());
								onRepaintEventDispatcher.call(managedWorkingSet);
							}
							break;

							default:
							{
								send_enable_disable_object_response(objectId, get_bit(static_cast<std::uint8_t>(EnableDisableObjectErrorBit::InvalidObjectID)), (0 != data[3]), managedWorkingSet->get_control_function());
							}
							break;
						}
					}
					else
					{
						send_enable_disable_object_response(objectId, get_bit(static_cast<std::uint8_t>(EnableDisableObjectErrorBit::InvalidEnableDisableCommandValue)), (0 != data[3]), managedWorkingSet->get_control_function());
					}
				}
				else
				{
					send_enable_disable_object_response(objectId, get_bit(static_cast<std::uint8_t>(EnableDisableObjectErrorBit::InvalidObjectID)), (0 != data[3]), managedWorkingSet->get_control_function());
				}
			}
			break;

			case Function::ChangeChildLocationCommand:
			{
				auto parentObjectId = get_little_endian_uint16(data, 1);
				auto objectID = get_little_endian_uint16(data, 3);
				auto parentObject = managedWorkingSet->get_object_by_id(parentObjectId);

				if (nullptr != parentObject)
				{
					auto lTargetObject = managedWorkingSet->get_object_by_id(objectID);

					if (nullptr != lTargetObject)
					{
						auto xRelativeChange = static_cast<std::int8_t>(static_cast<std::int16_t>(data[5]) - 127);
						auto yRelativeChange = static_cast<std::int8_t>(static_cast<std::int16_t>(data[6]) - 127);
						bool anyObjectMatched = parentObject->offset_all_children_with_id(objectID, xRelativeChange, yRelativeChange);

						onRepaintEventDispatcher.call(managedWorkingSet);

						if (anyObjectMatched)
						{
							send_change_child_location_response(parentObjectId, objectID, 0, managedWorkingSet->get_control_function());
							LOG_DEBUG("[VT Server]: Client %u change child location command. Parent: %u, Target: %u, X-Offset: %d, Y-Offset: %d", managedWorkingSet->get_control_function()->get_address(), parentObjectId, objectID, xRelativeChange, yRelativeChange);
							process_macro(parentObject, EventID::ChangeChildLocation, parentObject->get_object_type(), managedWorkingSet);
						}
						else
						{
							send_change_child_location_response(parentObjectId, objectID, get_bit(static_cast<std::uint8_t>(ChangeChildLocationorPositionErrorBit::TargetObjectDoesNotExistOrIsNotApplicable)), managedWorkingSet->get_control_function());
							LOG_WARNING("[VT Server]: Client %u change child location failed because the target object with ID %u isn't applicable", managedWorkingSet->get_control_function()->get_address(), objectID);
						}
					}
					else
					{
						send_change_child_location_response(parentObjectId, objectID, get_bit(static_cast<std::uint8_t>(ChangeChildLocationorPositionErrorBit::TargetObjectDoesNotExistOrIsNotApplicable)), managedWorkingSet->get_control_function());
						LOG_WARNING("[VT Server]: Client %u change child location failed because the target object with ID %u doesn't exist", managedWorkingSet->get_control_function()->get_address(), objectID);
					}
				}
				else
				{
					send_change_child_location_response(parentObjectId, objectID, get_bit(static_cast<std::uint8_t>(ChangeChildLocationorPositionErrorBit::ParentObjectDoesntExistOrIsNotAParentOfSpecifiedObject)), managedWorkingSet->get_control_function());
					LOG_WARNING("[VT Server]: Client %u change child location failed because the parent object with ID %u doesn't exist", managedWorkingSet->get_control_function()->get_address(), parentObjectId);
				}
			}
			break;

			case Function::ChangeActiveMaskCommand:
			{
				auto workingSetObjectId = get_little_endian_uint16(data, 1);
				auto newActiveMaskObjectId = get_little_endian_uint16(data, 3);
				auto workingSetObject = managedWorkingSet->get_object_by_id(workingSetObjectId);

				if ((nullptr != workingSetObject) && (VirtualTerminalObjectType::WorkingSet == workingSetObject->get_object_type()))
				{
					auto newActiveMaskObject = managedWorkingSet->get_object_by_id(newActiveMaskObjectId);

					if ((nullptr != newActiveMaskObject) &&
					    ((VirtualTerminalObjectType::DataMask == newActiveMaskObject->get_object_type()) ||
					     (VirtualTerminalObjectType::AlarmMask == newActiveMaskObject->get_object_type())))
					{
						std::static_pointer_cast<WorkingSet>(workingSetObject)->set_active_mask(newActiveMaskObjectId);
						send_change_active_mask_response(newActiveMaskObjectId, 0, managedWorkingSet->get_control_function());
						onChangeActiveMaskEventDispatcher.call(managedWorkingSet, workingSetObjectId, newActiveMaskObjectId);
						LOG_DEBUG("[VT Server]: Client %u changed active mask to object %u for working set object %u", managedWorkingSet->get_control_function()->get_address(), newActiveMaskObjectId, workingSetObjectId);
					}
					else
					{
						send_change_active_mask_response(newActiveMaskObjectId, get_bit(static_cast<std::uint8_t>(ChangeActiveMaskErrorBit::InvalidMaskObjectID)), managedWorkingSet->get_control_function());
						LOG_WARNING("[VT Server]: Client %u change active mask failed because the new mask object ID %u was not valid.", managedWorkingSet->get_control_function()->get_address(), newActiveMaskObjectId);
					}
				}
				else
				{
					send_change_active_mask_response(newActiveMaskObjectId, get_bit(static_cast<std::uint8_t>(ChangeActiveMaskErrorBit::InvalidWorkingSetObjectID)), managedWorkingSet->get_control_function());
					LOG_WARNING("[VT Server]: Client %u change active mask failed because the working set object ID %u was not valid.", managedWorkingSet->get_control_function()->get_address(), workingSetObjectId);
				}
			}
			break;

			case Function::GetSupportedObjectsMessage:
			{
				send_supported_objects(message.get_source_control_function());
				LOG_DEBUG("[VT Server]: Sent supported object list to client %u", managedWorkingSet->get_control_function()->get_address());
			}
			break;

			case Function::ChangeStringValueCommand:
			{
				auto objectIdToChange = get_little_endian_uint16(data, 1);
				auto numberOfBytesInString = get_little_endian_uint16(data, 3);
				auto stringObject = managedWorkingSet->get_object_by_id(objectIdToChange);

				if (message.get_data_length() >= static_cast<std::uint32_t>(numberOfBytesInString + 5))
				{
					if (nullptr != stringObject)
					{
						std::string newStringValue;

						for (std::uint32_t i = 0; i < numberOfBytesInString; i++)
						{
							newStringValue.push_back(static_cast<char>(data.at(5 + i)));
						}

						switch (stringObject->get_object_type())
						{
							case VirtualTerminalObjectType::StringVariable:
							{
								auto stringVariable = std::static_pointer_cast<StringVariable>(stringObject);

								// The transferred string is allowed to be smaller than the length of the value attribute of the target object
								// and in this case the VT shall pad the value attribute with space characters.
								while (newStringValue.length() < stringVariable->get_value().length())
								{
									newStringValue.push_back(' ');
								}
								stringVariable->set_value(newStringValue);
								send_change_string_value_response(objectIdToChange, 0, message.get_source_control_function());
								onRepaintEventDispatcher.call(managedWorkingSet);
								LOG_DEBUG("[VT Server]: Client %u change string value command for string variable object %u. Value: " + newStringValue, managedWorkingSet->get_control_function()->get_address(), objectIdToChange);
							}
							break;

							case VirtualTerminalObjectType::OutputString:
							{
								auto outputString = std::static_pointer_cast<OutputString>(stringObject);

								// The transferred string is allowed to be smaller than the length of the value attribute of the target object
								// and in this case the VT shall pad the value attribute with space characters.
								while (newStringValue.length() < outputString->get_value().length())
								{
									newStringValue.push_back(' ');
								}
								outputString->set_value(newStringValue);
								send_change_string_value_response(objectIdToChange, 0, message.get_source_control_function());
								onRepaintEventDispatcher.call(managedWorkingSet);
								LOG_DEBUG("[VT Server]: Client %u change string value command for output string object %u. Value: " + newStringValue, managedWorkingSet->get_control_function()->get_address(), objectIdToChange);
							}
							break;

							case VirtualTerminalObjectType::InputString:
							{
								auto inputString = std::static_pointer_cast<InputString>(stringObject);

								// The transferred string is allowed to be smaller than the length of the value attribute of the target object
								// and in this case the VT shall pad the value attribute with space characters.
								while (newStringValue.length() < inputString->get_value().length())
								{
									newStringValue.push_back(' ');
								}
								inputString->set_value(newStringValue);
								send_change_string_value_response(objectIdToChange, 0, message.get_source_control_function());
								onRepaintEventDispatcher.call(managedWorkingSet);
								LOG_DEBUG("[VT Server]: Client %u change string value command for input string object %u. Value: " + newStringValue, managedWorkingSet->get_control_function()->get_address(), objectIdToChange);
							}
							break;

							default:
							{
								send_change_string_value_response(objectIdToChange, get_bit(static_cast<std::uint8_t>(ChangeStringValueErrorBit::InvalidObjectID)), message.get_source_control_function());
								LOG_WARNING("[VT Server]: Client %u change string value command for object %u failed because the object ID was for an object that isn't a string.", managedWorkingSet->get_control_function()->get_address(), objectIdToChange);
							}
							break;
						}
					}
					else
					{
						send_change_string_value_response(objectIdToChange, get_bit(static_cast<std::uint8_t>(ChangeStringValueErrorBit::InvalidObjectID)), message.get_source_control_function());
						LOG_WARNING("[VT Server]: Client %u change string value command for object %u failed because the object ID was invalid.", managedWorkingSet->get_control_function()->get_address(), objectIdToChange);
					}
				}
				else
				{
					send_change_string_value_response(objectIdToChange, get_bit(static_cast<std::uint8_t>(ChangeStringValueErrorBit::AnyOtherError)), message.get_source_control_function());
					LOG_WARNING("[VT Server]: Client %u change string value command for object %u failed because data length is not valid when compared to the amount sent.", managedWorkingSet->get_control_function()->get_address(), objectIdToChange);
				}
			}
			break;

			case Function::ChangeFillAttributesCommand:
			{
				auto objectIdToChange = get_little_endian_uint16(data, 1);
				auto fillPatternID = get_little_endian_uint16(data, 5);
				auto object = managedWorkingSet->get_object_by_id(objectIdToChange);
				auto fillPatternObject = managedWorkingSet->get_object_by_id(fillPatternID);

				if ((nullptr != object) && (VirtualTerminalObjectType::FillAttributes == object->get_object_type()))
				{
					auto fillObject = std::static_pointer_cast<FillAttributes>(object);

					if (((nullptr != fillPatternObject) && (VirtualTerminalObjectType::PictureGraphic == fillPatternObject->get_object_type())) || (NULL_OBJECT_ID == fillPatternID))
					{
						if (data[3] <= static_cast<std::uint8_t>(FillAttributes::FillType::FillWithPatternGivenByFillPatternAttribute))
						{
							fillObject->set_fill_pattern(fillPatternID);
							fillObject->set_type(static_cast<FillAttributes::FillType>(data[3]));
							fillObject->set_background_color(data[4]);
							send_change_fill_attributes_response(objectIdToChange, 0, message.get_source_control_function());
							onRepaintEventDispatcher.call(managedWorkingSet);
							LOG_DEBUG("[VT Server]: Client %u change fill attributes command for object %u", managedWorkingSet->get_control_function()->get_address(), objectIdToChange);
						}
						else
						{
							send_change_fill_attributes_response(objectIdToChange, get_bit(static_cast<std::uint8_t>(ChangeFillAttributesErrorBit::InvalidType)), message.get_source_control_function());
							LOG_WARNING("[VT Server]: Client %u change fill attributes of object %u invalid fill object type. Must be a picture graphic.", managedWorkingSet->get_control_function()->get_address(), objectIdToChange);
						}
					}
					else
					{
						send_change_fill_attributes_response(objectIdToChange, get_bit(static_cast<std::uint8_t>(ChangeFillAttributesErrorBit::InvalidPatternObjectID)), message.get_source_control_function());
						LOG_WARNING("[VT Server]: Client %u change fill attributes invalid pattern object ID of %u for object %u", managedWorkingSet->get_control_function()->get_address(), fillPatternID, objectIdToChange);
					}
				}
				else
				{
					send_change_fill_attributes_response(objectIdToChange, get_bit(static_cast<std::uint8_t>(ChangeFillAttributesErrorBit::InvalidObjectID)), message.get_source_control_function());
					LOG_WARNING("[VT Server]: Client %u change fill attributes invalid object ID of %u", managedWorkingSet->get_control_function()->get_address(), objectIdToChange);
				}
			}
			break;

			case Function::ChangeChildPositionCommand:
			{
				auto parentObjectId = get_little_endian_uint16(data, 1);
				auto objectID = get_little_endian_uint16(data, 3);
				if (message.get_data_length() > CAN_DATA_LENGTH) // Must be at least 9 bytes
				{
					std::uint16_t newXPosition = get_little_endian_uint16(data, 5);
					std::uint16_t newYPosition = get_little_endian_uint16(data, 7);
					auto parentObject = managedWorkingSet->get_object_by_id(parentObjectId);
					auto targetObject = managedWorkingSet->get_object_by_id(objectID);

					if (nullptr != parentObject)
					{
						if (nullptr != targetObject)
						{
							switch (parentObject->get_object_type())
							{
								case VirtualTerminalObjectType::Button:
								case VirtualTerminalObjectType::Container:
								case VirtualTerminalObjectType::AlarmMask:
								case VirtualTerminalObjectType::DataMask:
								case VirtualTerminalObjectType::Key:
								case VirtualTerminalObjectType::WorkingSet:
								case VirtualTerminalObjectType::AuxiliaryInputType2:
								case VirtualTerminalObjectType::WindowMask:
								{
									bool wasFound = false;

									// If a parent object includes the child object multiple times, then each instance will be moved
									for (std::uint16_t i = 0; i < parentObject->get_number_children(); i++)
									{
										if (objectID == parentObject->get_child_id(i))
										{
											wasFound = true;
											parentObject->set_child_x(i, newXPosition);
											parentObject->set_child_y(i, newYPosition);
											onRepaintEventDispatcher.call(managedWorkingSet);
										}
									}

									if (wasFound)
									{
										LOG_DEBUG("[VT Server]: Client %u changed child position: object %u of parent object %u, x: %u, y: %u", managedWorkingSet->get_control_function()->get_address(), objectID, parentObjectId, newXPosition, newYPosition);
										send_change_child_position_response(parentObjectId, objectID, 0, message.get_source_control_function());
										process_macro(parentObject, EventID::OnChangeChildPosition, parentObject->get_object_type(), managedWorkingSet);
									}
									else
									{
										LOG_WARNING("[VT Server]: Client %u change child position error. Target object does not exist or is not applicable: object %u of parent object %u, x: %u, y: %u", managedWorkingSet->get_control_function()->get_address(), objectID, parentObjectId, newXPosition, newYPosition);
										send_change_child_position_response(parentObjectId, objectID, get_bit(static_cast<std::uint8_t>(ChangeChildLocationorPositionErrorBit::TargetObjectDoesNotExistOrIsNotApplicable)), message.get_source_control_function());
									}
								}
								break;

								default:
								{
									LOG_WARNING("[VT Server]: Client %u change child position error. Parent object type cannot be targeted by this command: object %u of parent object %u, x: %u, y: %u", managedWorkingSet->get_control_function()->get_address(), objectID, parentObjectId, newXPosition, newYPosition);
									send_change_child_position_response(parentObjectId, objectID, get_bit(static_cast<std::uint8_t>(ChangeChildLocationorPositionErrorBit::AnyOtherError)), message.get_source_control_function());
								}
								break;
							}
						}
						else
						{
							LOG_WARNING("[VT Server]: Client %u change child position error. Target object does not exist or is not applicable: object %u of parent object %u, x: %u, y: %u", managedWorkingSet->get_control_function()->get_address(), objectID, parentObjectId, newXPosition, newYPosition);
							send_change_child_position_response(parentObjectId, objectID, get_bit(static_cast<std::uint8_t>(ChangeChildLocationorPositionErrorBit::TargetObjectDoesNotExistOrIsNotApplicable)), message.get_source_control_function());
						}
					}
					else
					{
						LOG_WARNING("[VT Server]: Client %u change child position error. Parent object does not exist or is not applicable: object %u of parent object %u, x: %u, y: %u", managedWorkingSet->get_control_function()->get_address(), objectID, parentObjectId, newXPosition, newYPosition);
						send_change_child_position_response(parentObjectId, objectID, get_bit(static_cast<std::uint8_t>(ChangeChildLocationorPositionErrorBit::ParentObjectDoesntExistOrIsNotAParentOfSpecifiedObject)), message.get_source_control_function());
					}
				}
				else
				{
					LOG_WARNING("[VT Server]: Client %u change child position error. DLC must be 9 bytes for the message to be valid.");
					send_change_child_position_response(parentObjectId, objectID, get_bit(static_cast<std::uint8_t>(ChangeChildLocationorPositionErrorBit::AnyOtherError)), message.get_source_control_function());
				}
			}
			break;

			case Function::ChangeAttributeCommand:
			{
				auto objectID = get_little_endian_uint16(data, 1);
				auto targetObject = managedWorkingSet->get_object_by_id(objectID);
				std::uint8_t attributeID = data[3];
				const auto attributeData = get_little_endian_uint32(data, 4);
				VTObject::AttributeError errorCode = VTObject::AttributeError::AnyOtherError;

				if ((NULL_OBJECT_ID != objectID) && (nullptr != targetObject))
				{
					if (targetObject->set_attribute(attributeID, attributeData, managedWorkingSet->get_object_tree(), errorCode)) // 0 Is always the read-only "type" attribute
					{
						send_change_attribute_response(objectID, 0, data.at(3), message.get_source_control_function());
						LOG_DEBUG("[VT Server]: Client %u changed object %u attribute %u to %u", managedWorkingSet->get_control_function()->get_address(), objectID, attributeID, attributeData);
						onRepaintEventDispatcher.call(managedWorkingSet);
						process_macro(targetObject, EventID::OnChangeAttribute, targetObject->get_object_type(), managedWorkingSet);
					}
					else
					{
						send_change_attribute_response(objectID, get_bit(static_cast<std::uint8_t>(errorCode)), data.at(3), message.get_source_control_function());
						LOG_WARNING("[VT Server]: Client %u change object %u attribute %u to %ul error %u", managedWorkingSet->get_control_function()->get_address(), objectID, attributeID, attributeData, static_cast<std::uint8_t>(errorCode));
					}
				}
				else
				{
					send_change_attribute_response(objectID, get_bit(static_cast<std::uint8_t>(VTObject::AttributeError::InvalidObjectID)), data.at(3), message.get_source_control_function());
					LOG_WARNING("[VT Server]: Client %u change attribute %u invalid object ID of %u", managedWorkingSet->get_control_function()->get_address(), attributeID, objectID);
				}
			}
			break;

			case Function::ChangeSizeCommand:
			{
				auto objectID = get_little_endian_uint16(data, 1);
				auto newWidth = get_little_endian_uint16(data, 3);
				auto newHeight = get_little_endian_uint16(data, 5);
				auto targetObject = managedWorkingSet->get_object_by_id(objectID);

				if (nullptr != targetObject)
				{
					bool success = false;

					switch (targetObject->get_object_type())
					{
						case VirtualTerminalObjectType::OutputMeter:
						{
							if (newWidth == newHeight) // Output meter must be square!
							{
								targetObject->set_width(newWidth);
								targetObject->set_height(newHeight);
								success = true;
								LOG_DEBUG("[VT Server]: Client %u change size command: Object: %u, Width: %u, Height: %u", managedWorkingSet->get_control_function()->get_address(), objectID, newWidth, newHeight);
								onRepaintEventDispatcher.call(managedWorkingSet);
							}
							else
							{
								LOG_WARNING("[VT Server]: Client %u change size command: invalid new size. Meter must be square! Object: %u", managedWorkingSet->get_control_function()->get_address(), objectID);
								send_change_size_response(objectID, get_bit(static_cast<std::uint8_t>(ChangeSizeErrorBit::AnyOtherError)), message.get_source_control_function());
							}
						}
						break;

						case VirtualTerminalObjectType::Animation:
						case VirtualTerminalObjectType::Button:
						case VirtualTerminalObjectType::Container:
						case VirtualTerminalObjectType::InputBoolean:
						case VirtualTerminalObjectType::InputList:
						case VirtualTerminalObjectType::InputString:
						case VirtualTerminalObjectType::InputNumber:
						case VirtualTerminalObjectType::OutputArchedBarGraph:
						case VirtualTerminalObjectType::OutputEllipse:
						case VirtualTerminalObjectType::OutputLine:
						case VirtualTerminalObjectType::OutputLinearBarGraph:
						case VirtualTerminalObjectType::OutputList:
						case VirtualTerminalObjectType::OutputNumber:
						case VirtualTerminalObjectType::OutputPolygon:
						case VirtualTerminalObjectType::OutputRectangle:
						case VirtualTerminalObjectType::OutputString:
						{
							targetObject->set_width(newWidth);
							targetObject->set_height(newHeight);
							success = true;
							LOG_DEBUG("[VT Server]: Client %u change size command: Object: %u, Width: %u, Height: %u", managedWorkingSet->get_control_function()->get_address(), objectID, newWidth, newHeight);
							onRepaintEventDispatcher.call(managedWorkingSet);
						}
						break;

						default:
						{
							LOG_WARNING("[VT Server]: Client %u change size command: invalid object type for object %u", managedWorkingSet->get_control_function()->get_address(), objectID);
							send_change_size_response(objectID, get_bit(static_cast<std::uint8_t>(ChangeSizeErrorBit::AnyOtherError)), message.get_source_control_function());
						}
						break;
					}

					if (success)
					{
						send_change_size_response(objectID, 0, message.get_source_control_function());
						process_macro(targetObject, EventID::OnChangeSize, targetObject->get_object_type(), managedWorkingSet);
					}
				}
				else
				{
					LOG_WARNING("[VT Server]: Client %u change size command: invalid object ID of %u", managedWorkingSet->get_control_function()->get_address(), objectID);
					send_change_size_response(objectID, get_bit(static_cast<std::uint8_t>(ChangeSizeErrorBit::InvalidObjectID)), message.get_source_control_function());
				}
			}
			break;

			case Function::ChangeListItemCommand:
			{
				auto objectID = get_little_endian_uint16(data, 1);
				auto newObjectID = get_little_endian_uint16(data, 4);
				auto listIndex = data[3];
				auto targetObject = managedWorkingSet->get_object_by_id(objectID);
				auto newObject = managedWorkingSet->get_object_by_id(newObjectID);

				if (nullptr != targetObject)
				{
					if ((NULL_OBJECT_ID == newObjectID) || (nullptr != newObject))
					{
						switch (targetObject->get_object_type())
						{
							case VirtualTerminalObjectType::InputList:
							{
								if (std::static_pointer_cast<InputList>(targetObject)->change_list_item(listIndex, newObjectID, managedWorkingSet->get_object_tree()))
								{
									send_change_list_item_response(objectID, newObjectID, 0, listIndex, message.get_source_control_function());
									LOG_DEBUG("[VT Server]: Client %u change list item command: Object ID: %u, New Object ID: %u, Index: %u", managedWorkingSet->get_control_function()->get_address(), objectID, newObjectID, listIndex);
									onRepaintEventDispatcher.call(managedWorkingSet);
								}
								else
								{
									send_change_list_item_response(objectID, newObjectID, get_bit(static_cast<std::uint8_t>(ChangeListItemErrorBit::AnyOtherError)), listIndex, message.get_source_control_function());
									LOG_WARNING("[VT Server]: Client %u change list item command failed. Object ID: %u, New Object ID: %u, Index: %u", managedWorkingSet->get_control_function()->get_address(), objectID, newObjectID, listIndex);
								}
							}
							break;

							case VirtualTerminalObjectType::Animation:
							case VirtualTerminalObjectType::ExternalObjectDefinition:
							{
								// @todo
								send_change_list_item_response(objectID, newObjectID, get_bit(static_cast<std::uint8_t>(ChangeListItemErrorBit::AnyOtherError)), listIndex, message.get_source_control_function());
								LOG_WARNING("[VT Server]: Client %u change list item command: TODO object type", managedWorkingSet->get_control_function()->get_address());
							}
							break;

							case VirtualTerminalObjectType::OutputList:
							{
								if (std::static_pointer_cast<OutputList>(targetObject)->change_list_item(listIndex, newObjectID, managedWorkingSet->get_object_tree()))
								{
									send_change_list_item_response(objectID, newObjectID, 0, listIndex, message.get_source_control_function());
									LOG_DEBUG("[VT Server]: Client %u change list item command: Object ID: %u, New Object ID: %u, Index: %u", managedWorkingSet->get_control_function()->get_address(), objectID, newObjectID, listIndex);
									onRepaintEventDispatcher.call(managedWorkingSet);
								}
								else
								{
									send_change_list_item_response(objectID, newObjectID, get_bit(static_cast<std::uint8_t>(ChangeListItemErrorBit::AnyOtherError)), listIndex, message.get_source_control_function());
									LOG_WARNING("[VT Server]: Client %u change list item command failed. Object ID: %u, New Object ID: %u, Index: %u", managedWorkingSet->get_control_function()->get_address(), objectID, newObjectID, listIndex);
								}
							}
							break;

							default:
							{
								LOG_WARNING("[VT Server]: Client %u change list item command: invalid object type. Object: %u", managedWorkingSet->get_control_function()->get_address(), objectID);
								send_change_list_item_response(objectID, newObjectID, get_bit(static_cast<std::uint8_t>(ChangeListItemErrorBit::AnyOtherError)), listIndex, message.get_source_control_function());
							}
							break;
						}
					}
					else
					{
						LOG_WARNING("[VT Server]: Client %u change list item command: invalid new object ID of %u", managedWorkingSet->get_control_function()->get_address(), newObjectID);
						send_change_list_item_response(objectID, newObjectID, get_bit(static_cast<std::uint8_t>(ChangeListItemErrorBit::InvalidNewListItemObjectID)), listIndex, message.get_source_control_function());
					}
				}
				else
				{
					LOG_WARNING("[VT Server]: Client %u change list item command: invalid object ID of %u", managedWorkingSet->get_control_function()->get_address(), objectID);
					send_change_list_item_response(objectID, newObjectID, get_bit(static_cast<std::uint8_t>(ChangeListItemErrorBit::InvalidObjectID)), listIndex, message.get_source_control_function());
				}
			}
			break;

			case Function::ChangeFontAttributesCommand:
			{
				auto objectID = get_little_endian_uint16(data, 1);
				auto targetObject = managedWorkingSet->get_object_by_id(objectID);
				std::uint8_t fontColour = data[3];
				std::uint8_t fontSize = data[4];
				std::uint8_t fontType = data[5];
				std::uint8_t fontStyle = data[6];

				if ((nullptr != targetObject) &&
				    (VirtualTerminalObjectType::FontAttributes == targetObject->get_object_type()))
				{
					if (fontSize <= static_cast<std::uint8_t>(FontAttributes::FontSize::Size128x192))
					{
						auto font = std::static_pointer_cast<FontAttributes>(targetObject);
						font->set_colour(fontColour);
						font->set_size(static_cast<FontAttributes::FontSize>(fontSize));
						font->set_type(static_cast<FontAttributes::FontType>(fontType));
						font->set_style(fontStyle);
						LOG_DEBUG("[VT Server]: Client %u change font attributes command: ObjectID: %u", managedWorkingSet->get_control_function()->get_address(), objectID);
						send_change_font_attributes_response(objectID, 0, message.get_source_control_function());
						onRepaintEventDispatcher.call(managedWorkingSet);
					}
					else
					{
						LOG_WARNING("[VT Server]: Client %u change font attributes command: invalid font size %u. ObjectID: %u", managedWorkingSet->get_control_function()->get_address(), fontSize, objectID);
						send_change_font_attributes_response(objectID, get_bit(static_cast<std::uint8_t>(ChangeFontAttributesErrorBit::InvalidSize)), message.get_source_control_function());
					}
				}
				else
				{
					LOG_WARNING("[VT Server]: Client %u change font attributes command: invalid object ID of %u", managedWorkingSet->get_control_function()->get_address(), objectID);
					send_change_font_attributes_response(objectID, get_bit(static_cast<std::uint8_t>(ChangeFontAttributesErrorBit::InvalidObjectID)), message.get_source_control_function());
				}
			}
			break;

			case Function::ChangeLineAttributesCommand:
			{
				auto objectID = get_little_endian_uint16(data, 1);
				auto targetObject = managedWorkingSet->get_object_by_id(objectID);
				std::uint8_t lineColour = data[3];
				std::uint8_t lineWidth = data[4];
				std::uint16_t lineArt = get_little_endian_uint16(data, 5);

				if ((nullptr != targetObject) &&
				    (VirtualTerminalObjectType::LineAttributes == targetObject->get_object_type()))
				{
					auto line = std::static_pointer_cast<LineAttributes>(targetObject);
					line->set_background_color(lineColour);
					line->set_width(lineWidth);
					line->set_line_art_bit_pattern(lineArt);
					LOG_DEBUG("[VT Server]: Client %u change line attributes command: ObjectID: %u", managedWorkingSet->get_control_function()->get_address(), objectID);
					send_change_line_attributes_response(objectID, 0, message.get_source_control_function());
					onRepaintEventDispatcher.call(managedWorkingSet);
				}
				else
				{
					LOG_WARNING("[VT Server]: Client %u change line attributes command: invalid object ID of %u", managedWorkingSet->get_control_function()->get_address(), objectID);
					send_change_line_attributes_response(objectID, get_bit(static_cast<std::uint8_t>(ChangeFontAttributesErrorBit::InvalidObjectID)), message.get_source_control_function());
				}
			}
			break;

			case Function::ChangeSoftKeyMaskCommand:
			{
				auto dataOrAlarmMaskId = get_little_endian_uint16(data, 2);
				auto newSoftKeyMaskId = get_little_endian_uint16(data, 4);
				auto targetMask = managedWorkingSet->get_object_by_id(dataOrAlarmMaskId);
				auto newSoftKeyMask = managedWorkingSet->get_object_by_id(newSoftKeyMaskId);

				if (nullptr != targetMask)
				{
					if ((NULL_OBJECT_ID == newSoftKeyMaskId) || (nullptr != newSoftKeyMask))
					{
						switch (targetMask->get_object_type())
						{
							case VirtualTerminalObjectType::AlarmMask:
							{
								if (std::static_pointer_cast<AlarmMask>(targetMask)->change_soft_key_mask(newSoftKeyMaskId, managedWorkingSet->get_object_tree()))
								{
									LOG_DEBUG("[VT Server]: Client %u change soft key mask command: alarm mask object %u to %u", managedWorkingSet->get_control_function()->get_address(), dataOrAlarmMaskId, newSoftKeyMaskId);
									send_change_soft_key_mask_response(dataOrAlarmMaskId, newSoftKeyMaskId, 0, message.get_source_control_function());
									onChangeActiveSoftKeyMaskEventDispatcher.call(managedWorkingSet, dataOrAlarmMaskId, newSoftKeyMaskId);
									process_macro(targetMask, EventID::OnChangeSoftKeyMask, VirtualTerminalObjectType::AlarmMask, managedWorkingSet);
								}
								else
								{
									LOG_WARNING("[VT Server]: Client %u change soft key mask command: failed to set mask for alarm mask object %u to %u", managedWorkingSet->get_control_function()->get_address(), dataOrAlarmMaskId, newSoftKeyMaskId);
									send_change_soft_key_mask_response(dataOrAlarmMaskId, newSoftKeyMaskId, get_bit(static_cast<std::uint8_t>(ChangeSoftKeyMaskErrorBit::AnyOtherError)), message.get_source_control_function());
								}
							}
							break;

							case VirtualTerminalObjectType::DataMask:
							{
								if (std::static_pointer_cast<DataMask>(targetMask)->change_soft_key_mask(newSoftKeyMaskId, managedWorkingSet->get_object_tree()))
								{
									LOG_DEBUG("[VT Server]: Client %u change soft key mask command: data mask object %u to %u", managedWorkingSet->get_control_function()->get_address(), dataOrAlarmMaskId, newSoftKeyMaskId);
									send_change_soft_key_mask_response(dataOrAlarmMaskId, newSoftKeyMaskId, 0, message.get_source_control_function());
									onChangeActiveSoftKeyMaskEventDispatcher.call(managedWorkingSet, dataOrAlarmMaskId, newSoftKeyMaskId);
									process_macro(targetMask, EventID::OnChangeSoftKeyMask, VirtualTerminalObjectType::DataMask, managedWorkingSet);
								}
								else
								{
									LOG_WARNING("[VT Server]: Client %u change soft key mask command: failed to set mask for data mask object %u to %u", managedWorkingSet->get_control_function()->get_address(), dataOrAlarmMaskId, newSoftKeyMaskId);
									send_change_soft_key_mask_response(dataOrAlarmMaskId, newSoftKeyMaskId, get_bit(static_cast<std::uint8_t>(ChangeSoftKeyMaskErrorBit::AnyOtherError)), message.get_source_control_function());
								}
							}
							break;

							default:
							{
								LOG_WARNING("[VT Server]: Client %u change soft key mask command: invalid object type for object %u", managedWorkingSet->get_control_function()->get_address(), dataOrAlarmMaskId);
								send_change_soft_key_mask_response(dataOrAlarmMaskId, newSoftKeyMaskId, get_bit(static_cast<std::uint8_t>(ChangeSoftKeyMaskErrorBit::AnyOtherError)), message.get_source_control_function());
							}
							break;
						}
					}
					else
					{
						LOG_WARNING("[VT Server]: Client %u change soft key mask command: invalid soft key object ID of %u", managedWorkingSet->get_control_function()->get_address(), newSoftKeyMaskId);
						send_change_soft_key_mask_response(dataOrAlarmMaskId, newSoftKeyMaskId, get_bit(static_cast<std::uint8_t>(ChangeSoftKeyMaskErrorBit::InvalidSoftKeyMaskObjectID)), message.get_source_control_function());
					}
				}
				else
				{
					LOG_WARNING("[VT Server]: Client %u change soft key mask command: invalid data mask or alarm mask object ID of %u", managedWorkingSet->get_control_function()->get_address(), dataOrAlarmMaskId);
					send_change_soft_key_mask_response(dataOrAlarmMaskId, newSoftKeyMaskId, get_bit(static_cast<std::uint8_t>(ChangeSoftKeyMaskErrorBit::InvalidDataOrAlarmMaskObjectID)), message.get_source_control_function());
				}
			}
			break;

			case Function::ChangeBackgroundColourCommand:
			{
				auto objectID = get_little_endian_uint16(data, 1);
				auto targetObject = managedWorkingSet->get_object_by_id(objectID);
				std::uint8_t backgroundColour = data[3];

				if (nullptr != targetObject)
				{
					switch (targetObject->get_object_type())
					{
						case VirtualTerminalObjectType::AuxiliaryInputType2:
						case VirtualTerminalObjectType::WorkingSet:
						case VirtualTerminalObjectType::DataMask:
						case VirtualTerminalObjectType::AlarmMask:
						case VirtualTerminalObjectType::SoftKeyMask:
						case VirtualTerminalObjectType::Key:
						case VirtualTerminalObjectType::Button:
						case VirtualTerminalObjectType::InputNumber:
						case VirtualTerminalObjectType::InputBoolean:
						case VirtualTerminalObjectType::InputString:
						case VirtualTerminalObjectType::OutputString:
						case VirtualTerminalObjectType::OutputNumber:
						case VirtualTerminalObjectType::GraphicsContext:
						case VirtualTerminalObjectType::WindowMask:
						{
							targetObject->set_background_color(backgroundColour);
							LOG_DEBUG("[VT Server]: Client %u change background colour command: colour = %u", managedWorkingSet->get_control_function()->get_address(), objectID, backgroundColour);
							send_change_background_colour_response(objectID, 0, backgroundColour, message.get_source_control_function());
							process_macro(targetObject, EventID::OnChangeBackgroundColour, targetObject->get_object_type(), managedWorkingSet);
							onRepaintEventDispatcher.call(managedWorkingSet);
						}
						break;

						default:
						{
							LOG_WARNING("[VT Server]: Client %u change background colour command: invalid object type for object %u", managedWorkingSet->get_control_function()->get_address(), objectID);
							send_change_background_colour_response(objectID, get_bit(static_cast<std::uint8_t>(ChangeBackgroundColourErrorBit::AnyOtherError)), backgroundColour, message.get_source_control_function());
						}
						break;
					}
				}
				else
				{
					LOG_WARNING("[VT Server]: Client %u change background colour command: invalid object ID of %u", managedWorkingSet->get_control_function()->get_address(), objectID);
					send_change_background_colour_response(objectID, get_bit(static_cast<std::uint8_t>(ChangeBackgroundColourErrorBit::InvalidObjectID)), backgroundColour, message.get_source_control_function());
				}
			}
			break;

			case Function::ChangePriorityCommand:
			{
				auto objectID = get_little_endian_uint16(data, 1);
				auto targetObject = managedWorkingSet->get_object_by_id(objectID);
				std::uint8_t newPriority = data[3];

				if (nullptr != targetObject)
				{
					if (VirtualTerminalObjectType::AlarmMask == targetObject->get_object_type())
					{
						if (newPriority <= static_cast<std::uint8_t>(AlarmMaskPriority::Low))
						{
							send_change_priority_response(objectID, 0, newPriority, message.get_source_control_function());
							LOG_DEBUG("[VT Server]: Client %u change priority command: New Priority %u", managedWorkingSet->get_control_function()->get_address(), newPriority);
							process_macro(targetObject, EventID::OnChangePriority, VirtualTerminalObjectType::AlarmMask, managedWorkingSet);
						}
						else
						{
							send_change_priority_response(objectID, get_bit(static_cast<std::uint8_t>(ChangePriorityErrorBit::InvalidPriority)), newPriority, message.get_source_control_function());
							LOG_WARNING("[VT Server]: Client %u change priority command: Invalid Priority %u. Must be 2 or less.", managedWorkingSet->get_control_function()->get_address(), newPriority);
						}
					}
					else
					{
						send_change_priority_response(objectID, get_bit(static_cast<std::uint8_t>(ChangePriorityErrorBit::AnyOtherError)), newPriority, message.get_source_control_function());
						LOG_WARNING("[VT Server]: Client %u change priority command: invalid object ID of %u - the object must be an alarm mask.", managedWorkingSet->get_control_function()->get_address(), objectID);
					}
				}
				else
				{
					send_change_priority_response(objectID, get_bit(static_cast<std::uint8_t>(ChangePriorityErrorBit::InvalidObjectID)), newPriority, message.get_source_control_function());
					LOG_WARNING("[VT Server]: Client %u change priority command: invalid object ID of %u", managedWorkingSet->get_control_function()->get_address(), objectID);
				}
			}
			break;

			case Function::SelectInputObjectCommand:
			{
				auto objectID = get_little_endian_uint16(data, 1);
				auto targetObject = managedWorkingSet->get_object_by_id(objectID);

				if (nullptr != targetObject)
				{
					switch (targetObject->get_object_type())
					{
						case VirtualTerminalObjectType::Button:
						case VirtualTerminalObjectType::Key:
						{
							if (get_vt_version_byte(get_version()) > 3)
							{
								if (0 == data[3])
								{
									// 0 in Version 4+ means to activate the object for input
									managedWorkingSet->set_object_focus(objectID);
									LOG_DEBUG("[VT Server]: Client %u select input object %u and open for input", managedWorkingSet->get_control_function()->get_address(), objectID);
									onFocusObjectEventDispatcher.call(managedWorkingSet, objectID, true);
									send_select_input_object_response(objectID, 0, NULL_OBJECT_ID == objectID ? SelectInputObjectResponse::ObjectIsNotSelectedOrIsNullOrError : SelectInputObjectResponse::ObjectIsOpenedForEdit, message.get_source_control_function());
									process_macro(targetObject, NULL_OBJECT_ID == objectID ? EventID::OnInputFieldDeselection : EventID::OnInputFieldSelection, targetObject->get_object_type(), managedWorkingSet);
								}
								else if (0xFF == data[3])
								{
									// This removes focus if the ID is NULL_OBJECT_ID, or sets focus if not
									managedWorkingSet->set_object_focus(objectID);
									LOG_DEBUG("[VT Server]: Client %u select input object %u", managedWorkingSet->get_control_function()->get_address(), objectID);
									onFocusObjectEventDispatcher.call(managedWorkingSet, objectID, false);
									send_select_input_object_response(objectID, 0, NULL_OBJECT_ID == objectID ? SelectInputObjectResponse::ObjectIsNotSelectedOrIsNullOrError : SelectInputObjectResponse::ObjectIsSelected, message.get_source_control_function());
									process_macro(targetObject, NULL_OBJECT_ID == objectID ? EventID::OnInputFieldDeselection : EventID::OnInputFieldSelection, targetObject->get_object_type(), managedWorkingSet);
								}
								else
								{
									LOG_WARNING("[VT Server]: Client %u select input object command: Illegal option byte", managedWorkingSet->get_control_function()->get_address(), objectID);
									send_select_input_object_response(objectID, get_bit(static_cast<std::uint8_t>(SelectInputObjectErrorBit::InvalidOptionValue)), SelectInputObjectResponse::ObjectIsNotSelectedOrIsNullOrError, message.get_source_control_function());
								}
							}
							else
							{
								send_select_input_object_response(objectID, get_bit(static_cast<std::uint8_t>(SelectInputObjectErrorBit::AnyOtherError)), SelectInputObjectResponse::ObjectIsNotSelectedOrIsNullOrError, message.get_source_control_function());
								LOG_WARNING("[VT Server]: Client %u select input object command: buttons and keys can only be selected when the server is version 4 or higher.", managedWorkingSet->get_control_function()->get_address(), objectID);
							}
						}
						break;

						case VirtualTerminalObjectType::InputNumber:
						case VirtualTerminalObjectType::InputString:
						case VirtualTerminalObjectType::InputList:
						{
							if (0 == data[3])
							{
								// 0 in Version 4+ means to activate the object for input
								managedWorkingSet->set_object_focus(objectID);
								LOG_DEBUG("[VT Server]: Client %u select input object %u and open for input", managedWorkingSet->get_control_function()->get_address(), objectID);
								onFocusObjectEventDispatcher.call(managedWorkingSet, objectID, true);
								send_select_input_object_response(objectID, 0, NULL_OBJECT_ID == objectID ? SelectInputObjectResponse::ObjectIsNotSelectedOrIsNullOrError : SelectInputObjectResponse::ObjectIsOpenedForEdit, message.get_source_control_function());
								process_macro(targetObject, NULL_OBJECT_ID == objectID ? EventID::OnInputFieldDeselection : EventID::OnInputFieldSelection, targetObject->get_object_type(), managedWorkingSet);
							}
							else if (0xFF == data[3])
							{
								// This removes focus if the ID is NULL_OBJECT_ID, or sets focus if not
								managedWorkingSet->set_object_focus(objectID);
								LOG_DEBUG("[VT Server]: Client %u select input object %u", managedWorkingSet->get_control_function()->get_address(), objectID);
								onFocusObjectEventDispatcher.call(managedWorkingSet, objectID, false);
								send_select_input_object_response(objectID, 0, NULL_OBJECT_ID == objectID ? SelectInputObjectResponse::ObjectIsNotSelectedOrIsNullOrError : SelectInputObjectResponse::ObjectIsSelected, message.get_source_control_function());
								process_macro(targetObject, NULL_OBJECT_ID == objectID ? EventID::OnInputFieldDeselection : EventID::OnInputFieldSelection, targetObject->get_object_type(), managedWorkingSet);
							}
							else
							{
								LOG_WARNING("[VT Server]: Client %u select input object command: Illegal option byte", managedWorkingSet->get_control_function()->get_address(), objectID);
								send_select_input_object_response(objectID, get_bit(static_cast<std::uint8_t>(SelectInputObjectErrorBit::InvalidOptionValue)), SelectInputObjectResponse::ObjectIsNotSelectedOrIsNullOrError, message.get_source_control_function());
							}
						}
						break;

						default:
						{
							LOG_WARNING("[VT Server]: Client %u select input object command: invalid object type", managedWorkingSet->get_control_function()->get_address(), objectID);
							send_select_input_object_response(objectID, get_bit(static_cast<std::uint8_t>(SelectInputObjectErrorBit::AnyOtherError)), SelectInputObjectResponse::ObjectIsNotSelectedOrIsNullOrError, message.get_source_control_function());
						}
						break;
					}
				}
				else
				{
					send_select_input_object_response(objectID, get_bit(static_cast<std::uint8_t>(SelectInputObjectErrorBit::InvalidObjectID)), SelectInputObjectResponse::ObjectIsNotSelectedOrIsNullOrError, message.get_source_control_function());
					LOG_WARNING("[VT Server]: Client %u select input object command: invalid object ID of %u", managedWorkingSet->get_control_function()->get_address(), objectID);
				}
			}
			break;

			case Function::ESCCommand:
			{
				auto objectID = abort_input(managedWorkingSet);
				send_esc_response(objectID, NULL_OBJECT_ID == objectID ? get_bit(static_cast<std::uint8_t>(ESCErrorBit::NoInputFieldOpen)) : 0, message.get_source_control_function());
			}
			break;

			case Function::AuxiliaryInputTypeTwoMaintenanceMessage:
			{
				managedWorkingSet->set_auxiliary_input_maintenance_status(get_little_endian_uint16(data, 1), 1 == data[3]);
				managedWorkingSet->set_auxiliary_input_maintenance_timestamp_ms(SystemTiming::get_timestamp_ms());
			}
			break;

			case Function::PreferredAssignmentCommand:
			{
				process_preferred_assignment_command(message, managedWorkingSet);
			}
			break;

			case Function::AuxiliaryAssignmentTypeTwoCommand:
			{
				process_auxiliary_assignment_response(message, managedWorkingSet);
			}
			break;

			case Function::AuxiliaryInputStatusTypeTwoEnableCommand:
			{
				process_auxiliary_input_status_enable_response(message, managedWorkingSet);
			}
			break;

			case Function::AuxiliaryInputTypeTwoStatusMessage:
			{
				process_auxiliary_input_status_message(message, managedWorkingSet);
			}
			break;

			case Function::ExecuteMacroCommand:
			{
				auto objectID = static_cast<std::uint16_t>(data[1]);
				auto targetObject = managedWorkingSet->get_object_by_id(objectID);

				if (nullptr != targetObject)
				{
					if (VirtualTerminalObjectType::Macro == targetObject->get_object_type())
					{
						if (execute_macro(objectID, managedWorkingSet))
						{
							LOG_DEBUG("[VT Server]: Client %u execute macro command %u: completed.", managedWorkingSet->get_control_function()->get_address(), objectID);
							send_execute_macro_or_extended_macro_response(objectID, 0, message.get_source_control_function(), false);
						}
						else
						{
							LOG_ERROR("[VT Server]: Client %u execute macro command: failed. Macro probably contains invalid commands. Object pool state may now be undefined!", managedWorkingSet->get_control_function()->get_address(), objectID);
							send_execute_macro_or_extended_macro_response(objectID, get_bit(static_cast<std::uint8_t>(ExecuteMacroResponseErrorBit::AnyOtherError)), message.get_source_control_function(), false);
						}
					}
					else
					{
						LOG_WARNING("[VT Server]: Client %u execute macro command: object ID %u is not a macro!", managedWorkingSet->get_control_function()->get_address(), objectID);
						send_execute_macro_or_extended_macro_response(objectID, get_bit(static_cast<std::uint8_t>(ExecuteMacroResponseErrorBit::ObjectIsNotAMacro)), message.get_source_control_function(), false);
					}
				}
				else
				{
					LOG_WARNING("[VT Server]: Client %u execute macro command: invalid object ID of %u", managedWorkingSet->get_control_function()->get_address(), objectID);
					send_execute_macro_or_extended_macro_response(objectID, get_bit(static_cast<std::uint8_t>(ExecuteMacroResponseErrorBit::ObjectDoesntExist)), message.get_source_control_function(), false);
				}
			}
			break;

			case Function::ExecuteExtendedMacroCommand:
			{
				auto objectID = get_little_endian_uint16(data, 1);
				auto targetObject = managedWorkingSet->get_object_by_id(objectID);

				if (nullptr != targetObject)
				{
					if (VirtualTerminalObjectType::Macro == targetObject->get_object_type())
					{
						if (execute_macro(objectID, managedWorkingSet))
						{
							LOG_DEBUG("[VT Server]: Client %u execute extended macro command %u: completed.", managedWorkingSet->get_control_function()->get_address(), objectID);
							send_execute_macro_or_extended_macro_response(objectID, 0, message.get_source_control_function(), true);
						}
						else
						{
							LOG_ERROR("[VT Server]: Client %u execute extended macro command: failed. Macro probably contains invalid commands. Object pool state may now be undefined!", managedWorkingSet->get_control_function()->get_address(), objectID);
							send_execute_macro_or_extended_macro_response(objectID, get_bit(static_cast<std::uint8_t>(ExecuteMacroResponseErrorBit::AnyOtherError)), message.get_source_control_function(), true);
						}
					}
					else
					{
						LOG_WARNING("[VT Server]: Client %u execute extended macro command: object ID %u is not a macro!", managedWorkingSet->get_control_function()->get_address(), objectID);
						send_execute_macro_or_extended_macro_response(objectID, get_bit(static_cast<std::uint8_t>(ExecuteMacroResponseErrorBit::ObjectIsNotAMacro)), message.get_source_control_function(), true);
					}
				}
				else
				{
					LOG_WARNING("[VT Server]: Client %u execute extended macro command: invalid object ID of %u", managedWorkingSet->get_control_function()->get_address(), objectID);
					send_execute_macro_or_extended_macro_response(objectID, get_bit(static_cast<std::uint8_t>(ExecuteMacroResponseErrorBit::ObjectDoesntExist)), message.get_source_control_function(), true);
				}
			}
			break;

			case Function::DeleteObjectPoolCommand:
			{
				LOG_INFO("[VT Server]: Client %u requests deletion of object pool from volatile memory.", managedWorkingSet->get_control_function()->get_address());
				if (is_object_pool_parse_unanswered(*managedWorkingSet))
				{
					LOG_WARNING("[VT Server]: Client %u object pool can not be deleted while it is being parsed.", managedWorkingSet->get_control_function()->get_address());
					send_delete_object_pool_response(get_bit(static_cast<std::uint8_t>(DeleteObjectPoolErrorBit::AnyOtherError)), message.get_source_control_function());
				}
				else if (delete_object_pool(managedWorkingSet->get_control_function()->get_NAME()))
				{
					deactivate_working_set(managedWorkingSet);
					managedWorkingSetIopLoadStateMap.erase(managedWorkingSet);
					remove_auxiliary_assignments(managedWorkingSet);
					managedWorkingSet->clear_object_pool_data({});
					{
						LOCK_GUARD(Mutex, pendingShutdownsMutex);
						pendingObjectPoolDeletions.push_back(managedWorkingSet);
					}
					LOG_INFO("[VT Server]: Client %u object pool has been deleted.", managedWorkingSet->get_control_function()->get_address());
					send_delete_object_pool_response(0, message.get_source_control_function());
				}
				else
				{
					LOG_ERROR("[VT Server]: Client %u object pool failed to be deactivated.", managedWorkingSet->get_control_function()->get_address());
					send_delete_object_pool_response(get_bit(static_cast<std::uint8_t>(DeleteObjectPoolErrorBit::DeletionError)), message.get_source_control_function());
				}
			}
			break;

			case Function::ChangePolygonPointCommand:
			{
				auto objectID = get_little_endian_uint16(data, 1);
				const std::uint8_t polygonPointIndex = data[3];
				const std::uint16_t newXValue = get_little_endian_uint16(data, 4);
				const std::uint16_t newYValue = get_little_endian_uint16(data, 6);
				auto targetObject = managedWorkingSet->get_object_by_id(objectID);

				if (nullptr != targetObject)
				{
					if (VirtualTerminalObjectType::OutputPolygon == targetObject->get_object_type())
					{
						auto polygon = std::static_pointer_cast<OutputPolygon>(targetObject);

						if (polygon->change_point(polygonPointIndex, newXValue, newYValue))
						{
							LOG_DEBUG("[VT Server]: Client %u change polygon id %u point index %u. X = %u, Y = %u", managedWorkingSet->get_control_function()->get_address(), objectID, polygonPointIndex, newXValue, newYValue);
							send_change_polygon_point_response(objectID, 0, message.get_source_control_function());
						}
						else
						{
							LOG_WARNING("[VT Server]: Client %u change polygon point: the point index of %u is not valid for object %u", managedWorkingSet->get_control_function()->get_address(), polygonPointIndex, objectID);
							send_change_polygon_point_response(objectID, get_bit(static_cast<std::uint8_t>(ChangePolygonPointErrorBit::InvalidPointIndex)), message.get_source_control_function());
						}
					}
					else
					{
						LOG_WARNING("[VT Server]: Client %u change polygon point: object id %u is not an output polygon", managedWorkingSet->get_control_function()->get_address(), objectID);
						send_change_polygon_point_response(objectID, get_bit(static_cast<std::uint8_t>(ChangePolygonPointErrorBit::AnyOtherError)), message.get_source_control_function());
					}
				}
				else
				{
					LOG_WARNING("[VT Server]: Client %u change polygon point: invalid object ID of %u", managedWorkingSet->get_control_function()->get_address(), objectID);
					send_change_polygon_point_response(objectID, get_bit(static_cast<std::uint8_t>(ChangePolygonPointErrorBit::InvalidObjectID)), message.get_source_control_function());
				}
			}
			break;

			case Function::ControlAudioSignalCommand:
			{
				control_audio_signal(data[1],
				                     get_little_endian_uint16(data, 2),
				                     get_little_endian_uint16(data, 4),
				                     get_little_endian_uint16(data, 6),
				                     message.get_source_control_function());
				send_audio_signal_successful(message.get_source_control_function());
			}
			break;

			case Function::SetAudioVolumeCommand:
			{
				// The volume is a percentage, so a larger value is passed on as 100 %
				constexpr std::uint8_t MAX_AUDIO_VOLUME_PERCENT = 100;
				set_audio_volume(std::min(data[1], MAX_AUDIO_VOLUME_PERCENT), message.get_source_control_function());
				send_audio_volume_response(message.get_source_control_function());
			}
			break;

			case Function::IdentifyVTMessage:
			{
				identify_vt();
			}
			break;

			case Function::ScreenCapture:
			{
				screen_capture(data[1], data[2], message.get_source_control_function());
			}
			break;

			case Function::ButtonActivationMessage:
			case Function::SoftKeyActivationMessage:
			case Function::PointingEventMessage:
			case Function::VTSelectInputObjectMessage:
			case Function::VTESCMessage:
			case Function::VTChangeNumericValueMessage:
			case Function::VTChangeActiveMaskMessage:
			case Function::VTChangeStringValueMessage:
			case Function::VTControlAudioSignalTerminationMessage:
			case Function::VTChangeSoftKeyMaskMessage:
			case Function::VTOnUserLayoutHideShowMessage:
			case Function::AuxiliaryAssignmentTypeOneCommand:
			{
				// Todo, do something with the responses
			}
			break;

			// Already answered by process_stateless_messages, and a received 0xFD must never be answered
			case Function::GetSupportedWidecharsMessage:
			case Function::GetNumberOfSoftKeysMessage:
			case Function::GetTextFontDataMessage:
			case Function::GetWindowMaskDataMessage:
			case Function::GetHardwareMessage:
			case Function::AuxiliaryCapabilitiesRequest:
			case Function::UnsupportedVTFunctionMessage:
				break;

			// TODO: implement these, until then they are answered as unsupported
			case Function::ChangeEndPointCommand:
			case Function::LockUnlockMaskCommand:
			case Function::ChangeObjectLabelCommand:
			case Function::ChangePolygonScaleCommand:
			case Function::GetAttributeValueMessage:
			case Function::SelectColourMapCommand:
			case Function::GraphicsContextCommand:
			case Function::SelectActiveWorkingSet:
			default:
			{
				// A VT before version 5 had no defined response to an unknown function
				if ((5 <= get_vt_version_byte(get_version())) && (!message.is_broadcast()))
				{
					const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
						static_cast<std::uint8_t>(Function::UnsupportedVTFunctionMessage),
						data.at(0),
						0xFF,
						0xFF,
						0xFF,
						0xFF,
						0xFF,
						0xFF
					};
					CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
					                                               buffer.data(),
					                                               CAN_DATA_LENGTH,
					                                               serverInternalControlFunction,
					                                               message.get_source_control_function(),
					                                               get_priority());
				}
			}
			break;
		}
	}

	void VirtualTerminalServer::process_rx_message(const CANMessage &message, void *parent)
	{
		auto parentServer = static_cast<VirtualTerminalServer *>(parent);
		if ((nullptr != message.get_source_control_function()) &&
		    (nullptr != parentServer) &&
		    ((CAN_DATA_LENGTH <= message.get_data_length()) ||
		     ((message.get_data_length() > 5) && (static_cast<std::uint8_t>(Function::ChangeStringValueCommand) == message.get_uint8_at(0))))) // Technically this message can be 6 bytes
		{
			if (static_cast<std::uint32_t>(CANLibParameterGroupNumber::WorkingSetMaster) == message.get_identifier().get_parameter_group_number())
			{
				parentServer->process_working_set_master_message(message);
			}
			else if (static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal) == message.get_identifier().get_parameter_group_number())
			{
				if (static_cast<std::uint8_t>(Function::WorkingSetMaintenanceMessage) == message.get_uint8_at(0))
				{
					parentServer->process_working_set_maintenance_message(message);
				}

				bool responseSent = parentServer->process_stateless_messages(message);
				bool isManaged = parentServer->check_if_source_is_managed(message);
				if (!isManaged && !responseSent)
				{
					// Whomever this is has probably timed out. Send them a NACK
					LOG_WARNING("[VT Server]: Received a non-status message from a client at address %u, but they are not connected to this VT.", message.get_identifier().get_source_address());
					parentServer->send_acknowledgement(AcknowledgementType::Negative, static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal), parentServer->get_internal_control_function(), message.get_source_control_function());
				}
				if (isManaged)
				{
					for (const auto &cf : parentServer->managedWorkingSetList)
					{
						if (cf->get_control_function() == message.get_source_control_function())
						{
							parentServer->process_connection_dependent_messages(message, cf);
						}
					}
				}
			}
		}
	}

	bool VirtualTerminalServer::send_acknowledgement(AcknowledgementType type, std::uint32_t parameterGroupNumber, std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if ((nullptr != source) && (nullptr != destination))
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;

			buffer[0] = static_cast<std::uint8_t>(type);
			buffer[1] = 0xFF;
			buffer[2] = 0xFF;
			buffer[3] = 0xFF;
			buffer[4] = destination->get_address();
			buffer[5] = get_byte(parameterGroupNumber, 0);
			buffer[6] = get_byte(parameterGroupNumber, 1);
			buffer[7] = get_byte(parameterGroupNumber, 2);

			// ISO 11783-6:2014 4.6.9: the NACK is sent to the working set master, not broadcast globally.
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::Acknowledge),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        source,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_active_mask_response(std::uint16_t newMaskObjectID, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(Function::ChangeActiveMaskCommand),
				get_low_byte(newMaskObjectID),
				get_high_byte(newMaskObjectID),
				errorBitfield,
				0xFF,
				0xFF,
				0xFF,
				0xFF
			};

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_attribute_response(std::uint16_t objectID, std::uint8_t errorBitfield, std::uint8_t attributeID, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(Function::ChangeAttributeCommand),
				get_low_byte(objectID),
				get_high_byte(objectID),
				attributeID,
				errorBitfield,
				0xFF,
				0xFF,
				0xFF
			};

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_background_colour_response(std::uint16_t objectID, std::uint8_t errorBitfield, std::uint8_t colour, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(Function::ChangeBackgroundColourCommand),
				get_low_byte(objectID),
				get_high_byte(objectID),
				colour,
				errorBitfield,
				0xFF,
				0xFF,
				0xFF
			};

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_child_location_response(std::uint16_t parentObjectID, std::uint16_t objectID, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;

			buffer[0] = static_cast<std::uint8_t>(Function::ChangeChildLocationCommand);
			buffer[1] = get_low_byte(parentObjectID);
			buffer[2] = get_high_byte(parentObjectID);
			buffer[3] = get_low_byte(objectID);
			buffer[4] = get_high_byte(objectID);
			buffer[5] = errorBitfield;
			buffer[6] = 0xFF;
			buffer[7] = 0xFF;

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_child_position_response(std::uint16_t parentObjectID, std::uint16_t objectID, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer{
				static_cast<std::uint8_t>(Function::ChangeChildPositionCommand),
				get_low_byte(parentObjectID),
				get_high_byte(parentObjectID),
				get_low_byte(objectID),
				get_high_byte(objectID),
				errorBitfield,
				0xFF,
				0xFF
			};

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_fill_attributes_response(std::uint16_t objectID, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(Function::ChangeFillAttributesCommand),
				get_low_byte(objectID),
				get_high_byte(objectID),
				errorBitfield,
				0xFF,
				0xFF,
				0xFF,
				0xFF
			};
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_font_attributes_response(std::uint16_t objectID, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(Function::ChangeFontAttributesCommand),
				get_low_byte(objectID),
				get_high_byte(objectID),
				errorBitfield,
				0xFF,
				0xFF,
				0xFF,
				0xFF
			};
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_line_attributes_response(std::uint16_t objectID, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(Function::ChangeLineAttributesCommand),
				get_low_byte(objectID),
				get_high_byte(objectID),
				errorBitfield,
				0xFF,
				0xFF,
				0xFF,
				0xFF
			};
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_list_item_response(std::uint16_t objectID, std::uint16_t newObjectID, std::uint8_t errorBitfield, std::uint8_t listIndex, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(Function::ChangeListItemCommand),
				get_low_byte(objectID),
				get_high_byte(objectID),
				listIndex,
				get_low_byte(newObjectID),
				get_high_byte(newObjectID),
				errorBitfield,
				0xFF
			};
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_button_activation_message(KeyActivationCode activationCode, std::uint16_t objectId, std::uint16_t parentObjectId, std::uint8_t keyNumber, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;

			buffer[0] = static_cast<std::uint8_t>(Function::ButtonActivationMessage);
			buffer[1] = static_cast<std::uint8_t>(activationCode);
			buffer[2] = get_low_byte(objectId);
			buffer[3] = get_high_byte(objectId);
			buffer[4] = get_low_byte(parentObjectId);
			buffer[5] = get_high_byte(parentObjectId);
			buffer[6] = keyNumber;
			buffer[7] = 0xFF; // Reserved TODO: TAN

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_numeric_value_message(std::uint16_t objectId, std::uint32_t value, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(Function::VTChangeNumericValueMessage),
				get_low_byte(objectId),
				get_high_byte(objectId),
				0xFF, // TODO: TAN, version 6
				get_byte(value, 0),
				get_byte(value, 1),
				get_byte(value, 2),
				get_byte(value, 3)
			};

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_select_input_object_message(std::uint16_t objectId, bool isObjectSelected, bool isObjectOpenForInput, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(Function::VTSelectInputObjectMessage),
				get_low_byte(objectId),
				get_high_byte(objectId),
				static_cast<std::uint8_t>(isObjectSelected),
				static_cast<std::uint8_t>(isObjectOpenForInput),
				0xFF,
				0xFF,
				0xFF // Reserved TODO: TAN
			};

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_esc_message(std::uint16_t objectId, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(Function::VTESCMessage),
				get_low_byte(objectId),
				get_high_byte(objectId),
				errorBitfield,
				0xFF,
				0xFF,
				0xFF,
				0xFF // Reserved TODO: TAN
			};

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_soft_key_activation_message(KeyActivationCode activationCode, std::uint16_t objectId, std::uint16_t parentObjectId, std::uint8_t keyNumber, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(Function::SoftKeyActivationMessage),
				static_cast<std::uint8_t>(activationCode),
				get_low_byte(objectId),
				get_high_byte(objectId),
				get_low_byte(parentObjectId),
				get_high_byte(parentObjectId),
				keyNumber,
				0xFF // Reserved TODO: TAN
			};

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_string_value_message(std::uint16_t objectId, const std::string &value, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			if (value.length() > 255)
			{
				LOG_WARNING("[VT Server] Truncated user input string value to the maximum of 255. The string was: " + value);
			}

			std::vector<std::uint8_t> buffer = {
				static_cast<std::uint8_t>(Function::VTChangeStringValueMessage),
				get_low_byte(objectId),
				get_high_byte(objectId),
				static_cast<std::uint8_t>(value.length() > 255 ? 255 : value.length())
			};

			for (std::uint16_t i = 0; i < value.length() && i < 255; i++)
			{
				buffer.push_back(static_cast<std::uint8_t>(value.at(i)));
			}

			while (buffer.size() < CAN_DATA_LENGTH)
			{
				buffer.push_back(0xFF); // The standard specifies the message must be padded to 8 bytes
			}

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        static_cast<std::uint32_t>(buffer.size()),
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_load_version_response(Function function, std::uint8_t errorCodes, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(function),
				0xFF, // Reserved
				0xFF, // Reserved
				0xFF, // Reserved
				0xFF, // Reserved
				errorCodes,
				0xFF, // Reserved
				0xFF // Reserved
			};
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	void VirtualTerminalServer::process_macro(std::shared_ptr<isobus::VTObject> object, isobus::EventID macroEvent, isobus::VirtualTerminalObjectType targetObjectType, std::shared_ptr<isobus::VirtualTerminalServerManagedWorkingSet> workingset)
	{
		if (nullptr != object && targetObjectType == object->get_object_type())
		{
			for (std::uint8_t i = 0; i < object->get_number_macros(); i++)
			{
				auto macroMetadata = object->get_macro(i);
				if (macroMetadata.event == macroEvent)
				{
					execute_macro(macroMetadata.macroID, workingset);
				}
			}
		}
	}

	bool VirtualTerminalServer::send_change_numeric_value_response(std::uint16_t objectID, std::uint8_t errorBitfield, std::uint32_t value, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;

			buffer[0] = static_cast<std::uint8_t>(Function::ChangeNumericValueCommand);
			buffer[1] = get_low_byte(objectID);
			buffer[2] = get_high_byte(objectID);
			buffer[3] = errorBitfield;
			buffer[4] = get_byte(value, 0);
			buffer[5] = get_byte(value, 1);
			buffer[6] = get_byte(value, 2);
			buffer[7] = get_byte(value, 3);

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_polygon_point_response(std::uint16_t objectID, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;

			buffer[0] = static_cast<std::uint8_t>(Function::ChangePolygonPointCommand);
			buffer[1] = get_low_byte(objectID);
			buffer[2] = get_high_byte(objectID);
			buffer[3] = errorBitfield;
			buffer[4] = 0xFF;
			buffer[5] = 0xFF;
			buffer[6] = 0xFF;
			buffer[7] = 0xFF;

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_size_response(std::uint16_t objectID, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer{
				static_cast<std::uint8_t>(Function::ChangeSizeCommand),
				get_low_byte(objectID),
				get_high_byte(objectID),
				errorBitfield,
				0xFF,
				0xFF,
				0xFF,
				0xFF
			};

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_soft_key_mask_response(std::uint16_t objectID, std::uint16_t newObjectID, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer{
				static_cast<std::uint8_t>(Function::ChangeSoftKeyMaskCommand),
				get_low_byte(objectID),
				get_high_byte(objectID),
				get_low_byte(newObjectID),
				get_high_byte(newObjectID),
				errorBitfield,
				0xFF,
				0xFF
			};

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_change_string_value_response(std::uint16_t objectID, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(Function::ChangeStringValueCommand),
				0xFF,
				0xFF,
				get_low_byte(objectID),
				get_high_byte(objectID),
				errorBitfield,
				0xFF,
				0xFF
			};
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	void VirtualTerminalServer::send_restored_object_pool_response(VirtualTerminalServerManagedWorkingSet &workingSet, std::uint8_t errorCodes)
	{
		const Function function = workingSet.get_was_object_pool_loaded_with_extended_version_label() ? Function::ExtendedLoadVersionCommand : Function::LoadVersionCommand;

		send_load_version_response(function, errorCodes, workingSet.get_control_function());
		workingSet.set_was_object_pool_loaded_from_non_volatile_memory(false, false, {});
	}

	std::uint8_t VirtualTerminalServer::get_load_version_parse_error_bitfield() const
	{
		LoadVersionErrorBit errorBit = LoadVersionErrorBit::AnyOtherError;

		if (get_vt_version_byte(get_version()) > 3)
		{
			errorBit = LoadVersionErrorBit::FileSystemErrorOrPoolDataCorruption;
		}
		return get_bit(static_cast<std::uint8_t>(errorBit));
	}

	bool VirtualTerminalServer::send_delete_version_response(Function function, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(function),
				0xFF,
				0xFF,
				0xFF,
				0xFF,
				errorBitfield,
				0xFF,
				0xFF
			};
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_delete_object_pool_response(std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(Function::DeleteObjectPoolCommand),
				errorBitfield,
				0xFF,
				0xFF,
				0xFF,
				0xFF,
				0xFF,
				0xFF
			};
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_enable_disable_object_response(std::uint16_t objectID, std::uint8_t errorBitfield, bool value, std::shared_ptr<ControlFunction> destination) const
	{
		bool retVal = false;

		if (nullptr != destination)
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;

			buffer[0] = static_cast<std::uint8_t>(Function::EnableDisableObjectCommand);
			buffer[1] = get_low_byte(objectID);
			buffer[2] = get_high_byte(objectID);
			buffer[3] = value;
			buffer[4] = errorBitfield;
			buffer[5] = 0xFF;
			buffer[6] = 0xFF;
			buffer[7] = 0xFF;

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
			                                                        buffer.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        serverInternalControlFunction,
			                                                        destination,
			                                                        get_priority());
		}
		return retVal;
	}

	bool VirtualTerminalServer::send_end_of_object_pool_response(bool success,
	                                                             std::uint16_t parentIDOfFaultingObject,
	                                                             std::uint16_t faultingObjectID,
	                                                             std::uint8_t objectPoolErrorCodes,
	                                                             std::shared_ptr<ControlFunction> destination) const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };

		buffer[0] = static_cast<std::uint8_t>(Function::EndOfObjectPoolMessage);
		buffer[1] = (success ? 0x00 : 0x01); // Error in object pool is 0x01, no error is 0x00
		buffer[2] = get_low_byte(parentIDOfFaultingObject);
		buffer[3] = get_high_byte(parentIDOfFaultingObject);
		buffer[4] = get_low_byte(faultingObjectID);
		buffer[5] = get_high_byte(faultingObjectID);
		buffer[6] = objectPoolErrorCodes;
		buffer[7] = 0xFF; // Reserved

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      serverInternalControlFunction,
		                                                      destination,
		                                                      get_priority());
	}

	bool VirtualTerminalServer::send_execute_macro_or_extended_macro_response(std::uint16_t objectID, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination, bool extendedMacro) const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };

		if (extendedMacro)
		{
			buffer[0] = static_cast<std::uint8_t>(Function::ExecuteExtendedMacroCommand);
		}
		else
		{
			buffer[0] = static_cast<std::uint8_t>(Function::ExecuteMacroCommand);
		}

		buffer[1] = get_low_byte(objectID);

		if (extendedMacro)
		{
			buffer[2] = get_high_byte(objectID);
		}
		else
		{
			buffer[2] = 0xFF;
		}

		buffer[3] = errorBitfield;
		buffer[4] = 0xFF;
		buffer[5] = 0xFF;
		buffer[6] = 0xFF;
		buffer[7] = 0xFF;

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      serverInternalControlFunction,
		                                                      destination,
		                                                      get_priority());
	}

	bool VirtualTerminalServer::send_hide_show_object_response(std::uint16_t objectID, std::uint8_t errorBitfield, bool value, std::shared_ptr<ControlFunction> destination) const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };

		buffer[0] = static_cast<std::uint8_t>(Function::HideShowObjectCommand);
		buffer[1] = get_low_byte(objectID);
		buffer[2] = get_high_byte(objectID);
		buffer[3] = static_cast<std::uint8_t>(value);
		buffer[4] = errorBitfield;
		buffer[5] = 0xFF; // Reserved
		buffer[6] = 0xFF; // Reserved
		buffer[7] = 0xFF; // Reserved

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      serverInternalControlFunction,
		                                                      destination,
		                                                      get_priority());
	}

	bool VirtualTerminalServer::send_change_priority_response(std::uint16_t objectID, std::uint8_t errorBitfield, std::uint8_t priority, std::shared_ptr<ControlFunction> destination) const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };

		buffer[0] = static_cast<std::uint8_t>(Function::ChangePriorityCommand);
		buffer[1] = get_low_byte(objectID);
		buffer[2] = get_high_byte(objectID);
		buffer[3] = priority;
		buffer[4] = errorBitfield;
		buffer[5] = 0xFF; // Reserved
		buffer[6] = 0xFF; // Reserved
		buffer[7] = 0xFF; // Reserved

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      serverInternalControlFunction,
		                                                      destination,
		                                                      get_priority());
	}

	bool VirtualTerminalServer::send_select_input_object_response(std::uint16_t objectID, std::uint8_t errorBitfield, SelectInputObjectResponse response, std::shared_ptr<ControlFunction> destination) const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };

		buffer[0] = static_cast<std::uint8_t>(Function::SelectInputObjectCommand);
		buffer[1] = get_low_byte(objectID);
		buffer[2] = get_high_byte(objectID);
		buffer[3] = static_cast<std::uint8_t>(response);
		buffer[4] = errorBitfield;
		buffer[5] = 0xFF; // Reserved
		buffer[6] = 0xFF; // Reserved
		buffer[7] = 0xFF; // Reserved

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      serverInternalControlFunction,
		                                                      destination,
		                                                      get_priority());
	}

	bool VirtualTerminalServer::send_preferred_assignment_response(std::uint8_t errorBitfield, std::uint16_t faultyFunctionObjectID, std::shared_ptr<ControlFunction> destination) const
	{
		const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
			static_cast<std::uint8_t>(Function::PreferredAssignmentCommand),
			errorBitfield,
			get_low_byte(faultyFunctionObjectID),
			get_high_byte(faultyFunctionObjectID),
			0xFF,
			0xFF,
			0xFF,
			0xFF
		};

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      serverInternalControlFunction,
		                                                      destination,
		                                                      get_priority());
	}

	bool VirtualTerminalServer::send_auxiliary_assignment_command(std::uint64_t inputUnitName,
	                                                              bool storeAsPreferredAssignment,
	                                                              std::uint8_t functionType,
	                                                              std::uint16_t inputObjectID,
	                                                              std::uint16_t functionObjectID,
	                                                              std::shared_ptr<ControlFunction> destination) const
	{
		// 14 bytes, padded to the 8 byte boundary (ISO 11783-6 4.6.26)
		const std::array<std::uint8_t, 2 * CAN_DATA_LENGTH> buffer = {
			static_cast<std::uint8_t>(Function::AuxiliaryAssignmentTypeTwoCommand),
			static_cast<std::uint8_t>(inputUnitName),
			static_cast<std::uint8_t>(inputUnitName >> 8),
			static_cast<std::uint8_t>(inputUnitName >> 16),
			static_cast<std::uint8_t>(inputUnitName >> 24),
			static_cast<std::uint8_t>(inputUnitName >> 32),
			static_cast<std::uint8_t>(inputUnitName >> 40),
			static_cast<std::uint8_t>(inputUnitName >> 48),
			static_cast<std::uint8_t>(inputUnitName >> 56),
			static_cast<std::uint8_t>((storeAsPreferredAssignment ? 0x00 : 0x80) | (functionType & REMOVE_ASSIGNMENT_FUNCTION_TYPE)),
			get_low_byte(inputObjectID),
			get_high_byte(inputObjectID),
			get_low_byte(functionObjectID),
			get_high_byte(functionObjectID),
			0xFF,
			0xFF
		};

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      static_cast<std::uint32_t>(buffer.size()),
		                                                      serverInternalControlFunction,
		                                                      destination,
		                                                      get_priority());
	}

	bool VirtualTerminalServer::send_auxiliary_capabilities_response(std::uint8_t requestType, std::shared_ptr<ControlFunction> destination) const
	{
		constexpr std::uint8_t FUNCTION_UNITS_REQUEST = 1;
		const auto type = (FUNCTION_UNITS_REQUEST == requestType) ? VirtualTerminalObjectType::AuxiliaryFunctionType2 : VirtualTerminalObjectType::AuxiliaryInputType2;
		std::vector<std::uint8_t> buffer = { static_cast<std::uint8_t>(Function::AuxiliaryCapabilitiesRequest), 0 };

		// An invalid request type lists no units (ISO 11783-6 J.7.14)
		if (requestType <= FUNCTION_UNITS_REQUEST)
		{
			const auto assignments = get_auxiliary_assignments();

			for (const auto &workingSet : managedWorkingSetList)
			{
				const auto sets = get_auxiliary_capability_sets(*workingSet, type, assignments);

				if ((!sets.empty()) && (buffer[1] < 0xFF))
				{
					const std::uint64_t name = workingSet->get_control_function()->get_NAME().get_full_name();

					for (std::uint8_t i = 0; i < 8; i++)
					{
						buffer.push_back(static_cast<std::uint8_t>(name >> (8 * i)));
					}
					buffer.push_back(static_cast<std::uint8_t>(sets.size() / 3));
					buffer.insert(buffer.end(), sets.begin(), sets.end());
					buffer[1]++;
				}
			}
		}

		// Padded to the 8 byte boundary (ISO 11783-6 4.6.26)
		buffer.resize(((buffer.size() + CAN_DATA_LENGTH - 1) / CAN_DATA_LENGTH) * CAN_DATA_LENGTH, 0xFF);
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      static_cast<std::uint32_t>(buffer.size()),
		                                                      serverInternalControlFunction,
		                                                      destination,
		                                                      get_priority());
	}

	bool VirtualTerminalServer::send_auxiliary_input_status_enable_command(std::uint16_t inputObjectID, bool enable, std::shared_ptr<ControlFunction> destination) const
	{
		const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
			static_cast<std::uint8_t>(Function::AuxiliaryInputStatusTypeTwoEnableCommand),
			get_low_byte(inputObjectID),
			get_high_byte(inputObjectID),
			static_cast<std::uint8_t>(enable ? 0x01 : 0x00),
			0xFF,
			0xFF,
			0xFF,
			0xFF
		};

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      serverInternalControlFunction,
		                                                      destination,
		                                                      get_priority());
	}

	bool VirtualTerminalServer::send_esc_response(std::uint16_t objectID, std::uint8_t errorBitfield, std::shared_ptr<ControlFunction> destination) const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
			static_cast<std::uint8_t>(Function::ESCCommand),
			get_low_byte(objectID),
			get_high_byte(objectID),
			errorBitfield,
			0xFF,
			0xFF,
			0xFF,
			0xFF
		};

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      serverInternalControlFunction,
		                                                      destination,
		                                                      get_priority());
	}

	bool VirtualTerminalServer::send_status_message() const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };
		std::uint8_t busyCodes = busyCodesBitfield;

		if (get_vt_version_byte(get_version()) >= 3)
		{
			for (const auto &ws : managedWorkingSetList)
			{
				if (is_object_pool_parse_unanswered(*ws))
				{
					busyCodes |= get_bit(BUSY_PARSING_OBJECT_POOL_BIT);
				}
			}

			if (isAuxiliaryLearnModeActive)
			{
				busyCodes |= get_bit(AUXILIARY_LEARN_MODE_BIT);
			}
		}

		buffer[0] = static_cast<std::uint8_t>(Function::VTStatusMessage);
		buffer[1] = activeWorkingSetMasterAddress;
		buffer[2] = get_low_byte(activeWorkingSetDataMaskObjectID);
		buffer[3] = get_high_byte(activeWorkingSetDataMaskObjectID);
		buffer[4] = get_low_byte(activeWorkingSetSoftkeyMaskObjectID);
		buffer[5] = get_high_byte(activeWorkingSetSoftkeyMaskObjectID);
		buffer[6] = busyCodes;
		buffer[7] = currentCommandFunctionCode;
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      serverInternalControlFunction,
		                                                      nullptr,
		                                                      get_priority());
	}

	bool VirtualTerminalServer::send_supported_objects(std::shared_ptr<ControlFunction> destination) const
	{
		auto supportedObjects = get_supported_objects();
		std::sort(supportedObjects.begin(), supportedObjects.end());
		std::vector<std::uint8_t> buffer = { static_cast<std::uint8_t>(Function::GetSupportedObjectsMessage),
			                                   static_cast<std::uint8_t>(supportedObjects.size()) };

		for (const auto &supportedObject : supportedObjects)
		{
			buffer.push_back(supportedObject);
		}
		buffer.resize(std::max<std::size_t>(buffer.size(), CAN_DATA_LENGTH), 0xFF);
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      static_cast<std::uint32_t>(buffer.size()),
		                                                      serverInternalControlFunction,
		                                                      destination,
		                                                      get_priority());
	}

	bool VirtualTerminalServer::send_audio_signal_successful(std::shared_ptr<ControlFunction> destination) const
	{
		std::vector<std::uint8_t> buffer = { static_cast<std::uint8_t>(Function::ControlAudioSignalCommand), 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      serverInternalControlFunction,
		                                                      destination,
		                                                      get_priority());
	}

	bool VirtualTerminalServer::send_get_window_mask_data_response(std::shared_ptr<ControlFunction> destination) const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };

		buffer[0] = static_cast<std::uint8_t>(Function::GetWindowMaskDataMessage);
		buffer[1] = get_user_layout_datamask_bg_color();
		buffer[2] = get_user_layout_softkeymask_bg_color();
		buffer[3] = 0xFF; // Reserved
		buffer[4] = 0xFF; // Reserved
		buffer[5] = 0xFF; // Reserved
		buffer[6] = 0xFF; // Reserved
		buffer[7] = 0xFF; // Reserved

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      serverInternalControlFunction,
		                                                      destination,
		                                                      get_priority());
	}

	bool VirtualTerminalServer::send_audio_volume_response(std::shared_ptr<ControlFunction> destination) const
	{
		std::vector<std::uint8_t> buffer = { static_cast<std::uint8_t>(Function::SetAudioVolumeCommand), 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      serverInternalControlFunction,
		                                                      destination,
		                                                      get_priority());
	}

	bool VirtualTerminalServer::send_capture_screen_response(std::uint8_t item, std::uint8_t path, std::uint8_t errorCode, std::uint16_t imageId, std::shared_ptr<ControlFunction> requestor) const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { 0 };

		buffer[0] = static_cast<std::uint8_t>(Function::ScreenCapture);
		buffer[1] = item;
		buffer[2] = path;
		buffer[3] = errorCode;
		buffer[4] = get_low_byte(imageId);
		buffer[5] = get_high_byte(imageId);
		buffer[6] = 0xFF;
		buffer[7] = 0xFF;
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      serverInternalControlFunction,
		                                                      requestor,
		                                                      get_priority());
	}

	void VirtualTerminalServer::update()
	{
		std::vector<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>> deletedObjectPools;
		{
			LOCK_GUARD(Mutex, pendingShutdownsMutex);
			deletedObjectPools.swap(pendingObjectPoolDeletions);
		}
		for (const auto &ws : deletedObjectPools)
		{
			object_pool_deleted(ws);
		}

		// Timeouts are handled first, so the status message never reports a working set that has already shut down
		std::vector<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>> timedOutWorkingSets;
		for (const auto &ws : managedWorkingSetList)
		{
			if (SystemTiming::time_expired_ms(ws->get_working_set_maintenance_message_timestamp_ms(), WORKING_SET_MAINTENANCE_TIMEOUT_MS))
			{
				timedOutWorkingSets.push_back(ws);
			}
		}
		for (const auto &ws : timedOutWorkingSets)
		{
			LOG_WARNING("[VT Server]: Client %u has not sent a working set maintenance message for %u ms, treating it as an unexpected shutdown", ws->get_control_function()->get_address(), WORKING_SET_MAINTENANCE_TIMEOUT_MS);
			shut_down_working_set(ws, UnexpectedShutdownReason::MaintenanceTimeout);
		}
		report_shut_down_working_sets();

		if ((isobus::SystemTiming::time_expired_ms(statusMessageTimestamp_ms, 1000)) &&
		    (send_status_message()))
		{
			statusMessageTimestamp_ms = isobus::SystemTiming::get_timestamp_ms();
		}

		std::vector<std::pair<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>, bool>> parsedWorkingSets;
		std::vector<std::shared_ptr<VirtualTerminalServerManagedWorkingSet>> selectableWorkingSets;
		for (const auto &ws : managedWorkingSetList)
		{
			const auto processingState = ws->get_object_pool_processing_state();
			bool deletionPending = false;
			{
				// Read after the state, so a deletion that came before this parse is found and reported before it
				LOCK_GUARD(Mutex, pendingShutdownsMutex);
				deletionPending = (pendingObjectPoolDeletions.end() != std::find(pendingObjectPoolDeletions.begin(), pendingObjectPoolDeletions.end(), ws));
			}

			if ((!deletionPending) && (VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Success == processingState))
			{
				ws->join_parsing_thread();
				bool success = false;

				auto workingSetObject = ws->get_working_set_object();

				if (nullptr == workingSetObject)
				{
					if (ws->get_was_object_pool_loaded_from_non_volatile_memory())
					{
						send_restored_object_pool_response(*ws, get_load_version_parse_error_bitfield());
						LOG_ERROR("[VT Server]: The object pool loaded from non-volatile memory contains no working set object. Rejecting the pool.");
					}
					else
					{
						send_end_of_object_pool_response(false, NULL_OBJECT_ID, NULL_OBJECT_ID, get_bit(static_cast<std::uint8_t>(ObjectPoolErrorBit::AnyOtherError)), ws->get_control_function());
						LOG_ERROR("[VT Server]: The object pool was parsed but contains no working set object. Rejecting the pool.");
					}
				}
				else
				{
					success = true;
					if (ws->get_was_object_pool_loaded_from_non_volatile_memory())
					{
						send_restored_object_pool_response(*ws, 0);
					}
					else
					{
						send_end_of_object_pool_response(true, NULL_OBJECT_ID, NULL_OBJECT_ID, 0, ws->get_control_function());
					}

					// A Load Version can replace the pool of the active working set, so its mask has to be refreshed
					if (ws->get_control_function()->get_address() == activeWorkingSetMasterAddress)
					{
						activeWorkingSet = ws;
						activeWorkingSetDataMaskObjectID = std::static_pointer_cast<WorkingSet>(workingSetObject)->get_active_mask();
					}
					else if (std::static_pointer_cast<WorkingSet>(workingSetObject)->get_selectable())
					{
						selectableWorkingSets.push_back(ws);
					}
				}
				parsedWorkingSets.emplace_back(ws, success);
			}
			else if ((!deletionPending) && (VirtualTerminalServerManagedWorkingSet::ObjectPoolProcessingThreadState::Fail == processingState))
			{
				ws->join_parsing_thread();
				if (ws->get_was_object_pool_loaded_from_non_volatile_memory())
				{
					send_restored_object_pool_response(*ws, get_load_version_parse_error_bitfield());
				}
				else
				{
					send_end_of_object_pool_response(false, ws->get_object_pool_faulting_parent_object_id(), ws->get_object_pool_faulting_object_id(), get_bit(static_cast<std::uint8_t>(ObjectPoolErrorBit::AnyOtherError)), ws->get_control_function());
				}
				parsedWorkingSets.emplace_back(ws, false);
			}
		}

		// Called after the loop, so the application can change the working set list. Offers first, as a parse callback can delete a pool
		for (const auto &ws : selectableWorkingSets)
		{
			if (NULL_CAN_ADDRESS == activeWorkingSetMasterAddress)
			{
				select_working_set(ws);
			}
		}
		for (const auto &parsed : parsedWorkingSets)
		{
			if (parsed.second)
			{
				revalidate_auxiliary_assignments(parsed.first);
			}
			object_pool_parse_finished(parsed.first, parsed.second);
		}
		update_auxiliary_assignments();
	}
}
#endif // !defined CAN_STACK_DISABLE_THREADS && !defined ARDUINO
