//================================================================================================
/// @file isobus_virtual_terminal_client.cpp
///
/// @brief Implements the client for a virtual terminal
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#include "isobus/isobus/isobus_virtual_terminal_client.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <map>
#include <unordered_map>

namespace isobus
{
	VirtualTerminalClient::VirtualTerminalClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource) :
	  languageCommandInterface(clientSource, partner),
	  partnerControlFunction(partner),
	  myControlFunction(clientSource),
	  txFlags(static_cast<std::uint32_t>(TransmitFlags::NumberFlags), process_flags, this),
	  lastVTStatusTimestamp_ms(0),
	  activeWorkingSetDataMaskObjectID(NULL_OBJECT_ID),
	  activeWorkingSetSoftkeyMaskObjectID(NULL_OBJECT_ID),
	  activeWorkingSetMasterAddress(NULL_CAN_ADDRESS),
	  busyCodesBitfield(0),
	  currentCommandFunctionCode(0),
	  connectedVTVersion(0),
	  softKeyXAxisPixels(0),
	  softKeyYAxisPixels(0),
	  numberVirtualSoftkeysPerSoftkeyMask(0),
	  numberPhysicalSoftkeys(0),
	  smallFontSizesBitfield(0),
	  largeFontSizesBitfield(0),
	  fontStylesBitfield(0),
	  supportedGraphicsMode(GraphicMode::TwoHundredFiftySixColour),
	  xPixels(0),
	  yPixels(0),
	  hardwareFeaturesBitfield(0),
	  state(StateMachineState::Disconnected),
	  currentObjectPoolState(CurrentObjectPoolUploadState::Uninitialized),
	  stateMachineTimestamp_ms(0),
	  lastWorkingSetMaintenanceTimestamp_ms(0),
	  lastAuxiliaryMaintenanceTimestamp_ms(0),
	  ourModelIdentificationCode(1),
	  workerThread(nullptr),
	  firstTimeInState(false),
	  initialized(false),
	  sendWorkingSetMaintenance(false),
	  sendAuxiliaryMaintenance(false),
	  shouldTerminate(false),
	  objectPoolDataCallback(nullptr),
	  objectPoolSize_bytes(0),
	  lastObjectPoolIndex(0)
	{
		if (nullptr != partnerControlFunction)
		{
			partnerControlFunction->add_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), process_rx_message, this);
			partnerControlFunction->add_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::Acknowledge), process_rx_message, this);
			CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), process_rx_message, this);
			CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal), process_rx_message, this);
		}
	}

	VirtualTerminalClient::~VirtualTerminalClient()
	{
		terminate();
	}

	void VirtualTerminalClient::initialize(bool spawnThread)
	{
		if (shouldTerminate)
		{
			shouldTerminate = false;
			initialized = false;
		}

		if (!initialized)
		{
			if (!languageCommandInterface.get_initialized())
			{
				languageCommandInterface.initialize();
			}
			if (spawnThread)
			{
				workerThread = new std::thread([this]() { worker_thread_function(); });
			}
			initialized = true;
		}
	}

	bool VirtualTerminalClient::get_is_initialized() const
	{
		return initialized;
	}

	bool VirtualTerminalClient::get_is_connected() const
	{
		return (StateMachineState::Connected == state);
	}

	void VirtualTerminalClient::terminate()
	{
		if (initialized)
		{
			if (nullptr != partnerControlFunction)
			{
				partnerControlFunction->remove_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), process_rx_message, this);
				partnerControlFunction->remove_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::Acknowledge), process_rx_message, this);
				CANNetworkManager::CANNetwork.remove_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), process_rx_message, this);
				CANNetworkManager::CANNetwork.remove_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal), process_rx_message, this);
			}

			shouldTerminate = true;

			if (nullptr != workerThread)
			{
				workerThread->join();
				delete workerThread;
				workerThread = nullptr;
			}
		}
	}

	std::shared_ptr<void> VirtualTerminalClient::add_vt_soft_key_event_listener(std::function<void(const VTKeyEvent &)> callback)
	{
		return softKeyEventDispatcher.add_listener(callback);
	}

	std::shared_ptr<void> VirtualTerminalClient::add_vt_button_event_listener(std::function<void(const VTKeyEvent &)> callback)
	{
		return buttonEventDispatcher.add_listener(callback);
	}

	std::shared_ptr<void> VirtualTerminalClient::add_vt_pointing_event_listener(std::function<void(const VTPointingEvent &)> callback)
	{
		return pointingEventDispatcher.add_listener(callback);
	}

	std::shared_ptr<void> VirtualTerminalClient::add_vt_select_input_object_event_listener(std::function<void(const VTSelectInputObjectEvent &)> callback)
	{
		return selectInputObjectEventDispatcher.add_listener(callback);
	}

	std::shared_ptr<void> VirtualTerminalClient::add_vt_esc_message_event_listener(std::function<void(const VTESCMessageEvent &)> callback)
	{
		return escMessageEventDispatcher.add_listener(callback);
	}

	std::shared_ptr<void> VirtualTerminalClient::add_vt_change_numeric_value_event_listener(std::function<void(const VTChangeNumericValueEvent &)> callback)
	{
		return changeNumericValueEventDispatcher.add_listener(callback);
	}

	std::shared_ptr<void> VirtualTerminalClient::add_vt_change_active_mask_event_listener(std::function<void(const VTChangeActiveMaskEvent &)> callback)
	{
		return changeActiveMaskEventDispatcher.add_listener(callback);
	}

	std::shared_ptr<void> VirtualTerminalClient::add_vt_change_soft_key_mask_event_listener(std::function<void(const VTChangeSoftKeyMaskEvent &)> callback)
	{
		return changeSoftKeyMaskEventDispatcher.add_listener(callback);
	}

	std::shared_ptr<void> VirtualTerminalClient::add_vt_change_string_value_event_listener(std::function<void(const VTChangeStringValueEvent &)> callback)
	{
		return changeStringValueEventDispatcher.add_listener(callback);
	}

	std::shared_ptr<void> VirtualTerminalClient::add_vt_user_layout_hide_show_event_listener(std::function<void(const VTUserLayoutHideShowEvent &)> callback)
	{
		return userLayoutHideShowEventDispatcher.add_listener(callback);
	}

	std::shared_ptr<void> VirtualTerminalClient::add_vt_control_audio_signal_termination_event_listener(std::function<void(const VTAudioSignalTerminationEvent &)> callback)
	{
		return audioSignalTerminationEventDispatcher.add_listener(callback);
	}

	std::shared_ptr<void> VirtualTerminalClient::add_auxiliary_function_event_listener(std::function<void(const AuxiliaryFunctionEvent &)> callback)
	{
		return auxiliaryFunctionEventDispatcher.add_listener(callback);
	}

	void VirtualTerminalClient::set_auxiliary_input_model_identification_code(std::uint16_t modelIdentificationCode)
	{
		ourModelIdentificationCode = modelIdentificationCode;
	}

	bool VirtualTerminalClient::get_auxiliary_input_learn_mode_enabled() const
	{
		return 0x40 == (busyCodesBitfield & 0x40);
	}

	void VirtualTerminalClient::add_auxiliary_input_object_id(const std::uint16_t auxiliaryInputID)
	{
		ourAuxiliaryInputs[auxiliaryInputID] = AuxiliaryInputState{ 0, false, false, false, 0, 0 };
	}

	void VirtualTerminalClient::remove_auxiliary_input_object_id(const std::uint16_t auxiliaryInputID)
	{
		if (ourAuxiliaryInputs.count(auxiliaryInputID))
		{
			ourAuxiliaryInputs.erase(auxiliaryInputID);
			CANStackLogger::debug("[AUX-N] Removed auxiliary input with ID: " + isobus::to_string(static_cast<int>(auxiliaryInputID)));
		}
	}

	void VirtualTerminalClient::update_auxiliary_input(const std::uint16_t auxiliaryInputID, const std::uint16_t value1, const std::uint16_t value2, const bool controlLocked)
	{
		if (!ourAuxiliaryInputs.count(auxiliaryInputID))
		{
			CANStackLogger::warn("[AUX-N] Auxiliary input with ID '" + isobus::to_string(static_cast<int>(auxiliaryInputID)) + "' has not been registered. Ignoring update");
			return;
		}

		if (state == StateMachineState::Connected)
		{
			if ((value1 != ourAuxiliaryInputs.at(auxiliaryInputID).value1) || (value2 != ourAuxiliaryInputs.at(auxiliaryInputID).value2))
			{
				ourAuxiliaryInputs.at(auxiliaryInputID).value1 = value1;
				ourAuxiliaryInputs.at(auxiliaryInputID).value2 = value2;
				ourAuxiliaryInputs.at(auxiliaryInputID).controlLocked = controlLocked;
				ourAuxiliaryInputs.at(auxiliaryInputID).hasInteraction = true;
				update_auxiliary_input_status(auxiliaryInputID);
			}
		}
	}

	isobus::VirtualTerminalClient::AssignedAuxiliaryFunction::AssignedAuxiliaryFunction(const std::uint16_t functionObjectID, const std::uint16_t inputObjectID, const AuxiliaryTypeTwoFunctionType functionType) :
	  functionObjectID(functionObjectID), inputObjectID(inputObjectID), functionType(functionType)
	{
	}

	bool VirtualTerminalClient::AssignedAuxiliaryFunction::operator==(const AssignedAuxiliaryFunction &other) const
	{
		return (functionObjectID == other.functionObjectID) && (inputObjectID == other.inputObjectID) && (functionType == other.functionType);
	}

	bool VirtualTerminalClient::send_hide_show_object(std::uint16_t objectID, HideShowObjectCommand command)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::HideShowObjectCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(command),
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_enable_disable_object(std::uint16_t objectID, EnableDisableObjectCommand command)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::EnableDisableObjectCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(command),
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_select_input_object(std::uint16_t objectID, SelectInputObjectOptions option)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::SelectInputObjectCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(option),
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_ESC()
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ESCCommand),
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_control_audio_signal(std::uint8_t activations, std::uint16_t frequency_hz, std::uint16_t duration_ms, std::uint16_t offTimeDuration_ms)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ControlAudioSignalCommand),
			                                             activations,
			                                             static_cast<std::uint8_t>(frequency_hz & 0xFF),
			                                             static_cast<std::uint8_t>(frequency_hz >> 8),
			                                             static_cast<std::uint8_t>(duration_ms & 0xFF),
			                                             static_cast<std::uint8_t>(duration_ms >> 8),
			                                             static_cast<std::uint8_t>(offTimeDuration_ms & 0xFF),
			                                             static_cast<std::uint8_t>(offTimeDuration_ms >> 8) };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_set_audio_volume(std::uint8_t volume_percent)
	{
		if (volume_percent > 100)
		{
			volume_percent = 100;
		}

		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::SetAudioVolumeCommand),
			                                             volume_percent,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_child_location(std::uint16_t objectID, std::uint16_t parentObjectID, std::uint8_t relativeXPositionChange, std::uint8_t relativeYPositionChange)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangeChildLocationCommand),
			                                             static_cast<std::uint8_t>(parentObjectID & 0xFF),
			                                             static_cast<std::uint8_t>(parentObjectID >> 8),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             relativeXPositionChange,
			                                             relativeYPositionChange,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_child_position(std::uint16_t objectID, std::uint16_t parentObjectID, std::uint16_t xPosition, std::uint16_t yPosition)
	{
		const std::uint8_t buffer[9] = {
			static_cast<std::uint8_t>(Function::ChangeChildPositionCommand),
			static_cast<std::uint8_t>(parentObjectID & 0xFF),
			static_cast<std::uint8_t>(parentObjectID >> 8),
			static_cast<std::uint8_t>(objectID & 0xFF),
			static_cast<std::uint8_t>(objectID >> 8),
			static_cast<std::uint8_t>(xPosition & 0xFF),
			static_cast<std::uint8_t>(xPosition >> 8),
			static_cast<std::uint8_t>(yPosition & 0xFF),
			static_cast<std::uint8_t>(yPosition >> 8),
		};
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      9,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_size_command(std::uint16_t objectID, std::uint16_t newWidth, std::uint16_t newHeight)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangeSizeCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(newWidth & 0xFF),
			                                             static_cast<std::uint8_t>(newWidth >> 8),
			                                             static_cast<std::uint8_t>(newHeight & 0xFF),
			                                             static_cast<std::uint8_t>(newHeight >> 8),
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_background_colour(std::uint16_t objectID, std::uint8_t colour)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangeBackgroundColourCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             colour,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_numeric_value(std::uint16_t objectID, std::uint32_t value)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = {
			static_cast<std::uint8_t>(Function::ChangeNumericValueCommand),
			static_cast<std::uint8_t>(objectID & 0xFF),
			static_cast<std::uint8_t>(objectID >> 8),
			0xFF,
			static_cast<std::uint8_t>(value & 0xFF),
			static_cast<std::uint8_t>((value >> 8) & 0xFF),
			static_cast<std::uint8_t>((value >> 16) & 0xFF),
			static_cast<std::uint8_t>((value >> 24) & 0xFF),
		};
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_string_value(std::uint16_t objectID, uint16_t stringLength, const char *value)
	{
		bool retVal = false;

		if (nullptr != value)
		{
			std::uint8_t *buffer = new std::uint8_t[5 + stringLength];
			buffer[0] = static_cast<std::uint8_t>(Function::ChangeStringValueCommand);
			buffer[1] = static_cast<std::uint8_t>(objectID & 0xFF);
			buffer[2] = static_cast<std::uint8_t>(objectID >> 8);
			buffer[3] = static_cast<std::uint8_t>(stringLength & 0xFF);
			buffer[4] = static_cast<std::uint8_t>(stringLength >> 8);
			for (uint16_t i = 0; i < stringLength; i++)
			{
				buffer[5 + i] = value[i];
			}
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
			                                                        buffer,
			                                                        5 + stringLength,
			                                                        myControlFunction.get(),
			                                                        partnerControlFunction.get(),
			                                                        CANIdentifier::PriorityLowest7);
			delete[] buffer;
		}
		return retVal;
	}

	bool VirtualTerminalClient::send_change_string_value(std::uint16_t objectID, const std::string &value)
	{
		return send_change_string_value(objectID, value.size(), value.c_str());
	}

	bool VirtualTerminalClient::send_change_endpoint(std::uint16_t objectID, std::uint16_t width_px, std::uint16_t height_px, LineDirection direction)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangeEndPointCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(width_px & 0xFF),
			                                             static_cast<std::uint8_t>(width_px >> 8),
			                                             static_cast<std::uint8_t>(height_px & 0xFF),
			                                             static_cast<std::uint8_t>(height_px >> 8),
			                                             static_cast<std::uint8_t>(direction) };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_font_attributes(std::uint16_t objectID, std::uint8_t colour, FontSize size, std::uint8_t type, std::uint8_t styleBitfield)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangeFontAttributesCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             colour,
			                                             static_cast<std::uint8_t>(size),
			                                             type,
			                                             styleBitfield,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_line_attributes(std::uint16_t objectID, std::uint8_t colour, std::uint8_t width, std::uint16_t lineArtBitmask)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangeLineAttributesCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             colour,
			                                             static_cast<std::uint8_t>(width),
			                                             static_cast<std::uint8_t>(lineArtBitmask & 0xFF),
			                                             static_cast<std::uint8_t>(lineArtBitmask >> 8),
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_fill_attributes(std::uint16_t objectID, FillType fillType, std::uint8_t colour, std::uint16_t fillPatternObjectID)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangeFillAttributesCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(fillType),
			                                             colour,
			                                             static_cast<std::uint8_t>(fillPatternObjectID & 0xFF),
			                                             static_cast<std::uint8_t>(fillPatternObjectID >> 8),
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_active_mask(std::uint16_t workingSetObjectID, std::uint16_t newActiveMaskObjectID)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangeActiveMaskCommand),
			                                             static_cast<std::uint8_t>(workingSetObjectID & 0xFF),
			                                             static_cast<std::uint8_t>(workingSetObjectID >> 8),
			                                             static_cast<std::uint8_t>(newActiveMaskObjectID & 0xFF),
			                                             static_cast<std::uint8_t>(newActiveMaskObjectID >> 8),
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_softkey_mask(MaskType type, std::uint16_t dataOrAlarmMaskObjectID, std::uint16_t newSoftKeyMaskObjectID)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangeSoftKeyMaskCommand),
			                                             static_cast<std::uint8_t>(type),
			                                             static_cast<std::uint8_t>(dataOrAlarmMaskObjectID & 0xFF),
			                                             static_cast<std::uint8_t>(dataOrAlarmMaskObjectID >> 8),
			                                             static_cast<std::uint8_t>(newSoftKeyMaskObjectID & 0xFF),
			                                             static_cast<std::uint8_t>(newSoftKeyMaskObjectID >> 8),
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_attribute(std::uint16_t objectID, std::uint8_t attributeID, std::uint32_t value)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangeAttributeCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             attributeID,
			                                             static_cast<std::uint8_t>(value & 0xFF),
			                                             static_cast<std::uint8_t>((value >> 8) & 0xFF),
			                                             static_cast<std::uint8_t>((value >> 16) & 0xFF),
			                                             static_cast<std::uint8_t>((value >> 24) & 0xFF) };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_priority(std::uint16_t alarmMaskObjectID, AlarmMaskPriority priority)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangePriorityCommand),
			                                             static_cast<std::uint8_t>(alarmMaskObjectID & 0xFF),
			                                             static_cast<std::uint8_t>(alarmMaskObjectID >> 8),
			                                             static_cast<std::uint8_t>(priority),
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_list_item(std::uint16_t objectID, std::uint8_t listIndex, std::uint16_t newObjectID)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangeListItemCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             listIndex,
			                                             static_cast<std::uint8_t>(newObjectID & 0xFF),
			                                             static_cast<std::uint8_t>(newObjectID >> 8),
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_lock_unlock_mask(MaskLockState state, std::uint16_t objectID, std::uint16_t timeout_ms)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::LockUnlockMaskCommand),
			                                             static_cast<std::uint8_t>(state),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(timeout_ms & 0xFF),
			                                             static_cast<std::uint8_t>(timeout_ms >> 8),
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_execute_macro(std::uint16_t objectID)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ExecuteMacroCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_object_label(std::uint16_t objectID, std::uint16_t labelStringObjectID, std::uint8_t fontType, std::uint16_t graphicalDesignatorObjectID)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangeObjectLabelCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(labelStringObjectID & 0xFF),
			                                             static_cast<std::uint8_t>(labelStringObjectID >> 8),
			                                             fontType,
			                                             static_cast<std::uint8_t>(graphicalDesignatorObjectID & 0xFF),
			                                             static_cast<std::uint8_t>(graphicalDesignatorObjectID >> 8) };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_polygon_point(std::uint16_t objectID, std::uint8_t pointIndex, std::uint16_t newXValue, std::uint16_t newYValue)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangePolygonPointCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             pointIndex,
			                                             static_cast<std::uint8_t>(newXValue & 0xFF),
			                                             static_cast<std::uint8_t>(newXValue >> 8),
			                                             static_cast<std::uint8_t>(newYValue & 0xFF),
			                                             static_cast<std::uint8_t>(newYValue >> 8) };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_polygon_scale(std::uint16_t objectID, std::uint16_t widthAttribute, std::uint16_t heightAttribute)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangePolygonScaleCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(widthAttribute & 0xFF),
			                                             static_cast<std::uint8_t>(widthAttribute >> 8),
			                                             static_cast<std::uint8_t>(heightAttribute & 0xFF),
			                                             static_cast<std::uint8_t>(heightAttribute >> 8),
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_select_colour_map_or_palette(std::uint16_t objectID)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::SelectColourMapCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_execute_extended_macro(std::uint16_t objectID)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ExecuteExtendedMacroCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_select_active_working_set(std::uint64_t NAMEofWorkingSetMasterForDesiredWorkingSet)
	{
		const std::uint8_t buffer[9] = { static_cast<std::uint8_t>(Function::SelectActiveWorkingSet),
			                               static_cast<std::uint8_t>(NAMEofWorkingSetMasterForDesiredWorkingSet & 0xFF),
			                               static_cast<std::uint8_t>((NAMEofWorkingSetMasterForDesiredWorkingSet >> 8) & 0xFF),
			                               static_cast<std::uint8_t>((NAMEofWorkingSetMasterForDesiredWorkingSet >> 16) & 0xFF),
			                               static_cast<std::uint8_t>((NAMEofWorkingSetMasterForDesiredWorkingSet >> 24) & 0xFF),
			                               static_cast<std::uint8_t>((NAMEofWorkingSetMasterForDesiredWorkingSet >> 32) & 0xFF),
			                               static_cast<std::uint8_t>((NAMEofWorkingSetMasterForDesiredWorkingSet >> 40) & 0xFF),
			                               static_cast<std::uint8_t>((NAMEofWorkingSetMasterForDesiredWorkingSet >> 48) & 0xFF),
			                               static_cast<std::uint8_t>((NAMEofWorkingSetMasterForDesiredWorkingSet >> 56) & 0xFF) };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      9,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_set_graphics_cursor(std::uint16_t objectID, std::int16_t xPosition, std::int16_t yPosition)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::SetGraphicsCursor),
			                                             static_cast<std::uint8_t>(xPosition & 0xFF),
			                                             static_cast<std::uint8_t>(xPosition >> 8),
			                                             static_cast<std::uint8_t>(yPosition & 0xFF),
			                                             static_cast<std::uint8_t>(yPosition >> 8) };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_move_graphics_cursor(std::uint16_t objectID, std::int16_t xOffset, std::int16_t yOffset)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::MoveGraphicsCursor),
			                                             static_cast<std::uint8_t>(xOffset & 0xFF),
			                                             static_cast<std::uint8_t>(xOffset >> 8),
			                                             static_cast<std::uint8_t>(yOffset & 0xFF),
			                                             static_cast<std::uint8_t>(yOffset >> 8) };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_set_foreground_colour(std::uint16_t objectID, std::uint8_t colour)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::SetForegroundColour),
			                                             colour,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_set_background_colour(std::uint16_t objectID, std::uint8_t colour)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::SetBackgroundColour),
			                                             colour,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_set_line_attributes_object_id(std::uint16_t objectID, std::uint16_t lineAttributesObjectID)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::SetLineAttributesObjectID),
			                                             static_cast<std::uint8_t>(lineAttributesObjectID & 0xFF),
			                                             static_cast<std::uint8_t>(lineAttributesObjectID >> 8),
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_set_fill_attributes_object_id(std::uint16_t objectID, std::uint16_t fillAttributesObjectID)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::SetFillAttributesObjectID),
			                                             static_cast<std::uint8_t>(fillAttributesObjectID & 0xFF),
			                                             static_cast<std::uint8_t>(fillAttributesObjectID >> 8),
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_set_font_attributes_object_id(std::uint16_t objectID, std::uint16_t fontAttributesObjectID)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::SetFontAttributesObjectID),
			                                             static_cast<std::uint8_t>(fontAttributesObjectID & 0xFF),
			                                             static_cast<std::uint8_t>(fontAttributesObjectID >> 8),
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_erase_rectangle(std::uint16_t objectID, std::uint16_t width, std::uint16_t height)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::EraseRectangle),
			                                             static_cast<std::uint8_t>(width & 0xFF),
			                                             static_cast<std::uint8_t>(width >> 8),
			                                             static_cast<std::uint8_t>(height & 0xFF),
			                                             static_cast<std::uint8_t>(height >> 8) };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_draw_point(std::uint16_t objectID, std::int16_t xOffset, std::int16_t yOffset)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::DrawPoint),
			                                             static_cast<std::uint8_t>(xOffset & 0xFF),
			                                             static_cast<std::uint8_t>(xOffset >> 8),
			                                             static_cast<std::uint8_t>(yOffset & 0xFF),
			                                             static_cast<std::uint8_t>(yOffset >> 8) };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_draw_line(std::uint16_t objectID, std::int16_t xOffset, std::int16_t yOffset)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::DrawLine),
			                                             static_cast<std::uint8_t>(xOffset & 0xFF),
			                                             static_cast<std::uint8_t>(xOffset >> 8),
			                                             static_cast<std::uint8_t>(yOffset & 0xFF),
			                                             static_cast<std::uint8_t>(yOffset >> 8) };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_draw_rectangle(std::uint16_t objectID, std::uint16_t width, std::uint16_t height)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::DrawRectangle),
			                                             static_cast<std::uint8_t>(width & 0xFF),
			                                             static_cast<std::uint8_t>(width >> 8),
			                                             static_cast<std::uint8_t>(height & 0xFF),
			                                             static_cast<std::uint8_t>(height >> 8) };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_draw_closed_ellipse(std::uint16_t objectID, std::uint16_t width, std::uint16_t height)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::DrawClosedEllipse),
			                                             static_cast<std::uint8_t>(width & 0xFF),
			                                             static_cast<std::uint8_t>(width >> 8),
			                                             static_cast<std::uint8_t>(height & 0xFF),
			                                             static_cast<std::uint8_t>(height >> 8) };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_draw_polygon(std::uint16_t objectID, std::uint8_t numberOfPoints, std::int16_t *listOfXOffsetsRelativeToCursor, std::int16_t *listOfYOffsetsRelativeToCursor)
	{
		bool retVal = false;

		if ((numberOfPoints > 0) &&
		    (nullptr != listOfXOffsetsRelativeToCursor) &&
		    (nullptr != listOfYOffsetsRelativeToCursor))

		{
			const std::uint16_t messageLength = (9 + (4 * numberOfPoints));
			std::uint8_t *buffer = new std::uint8_t[messageLength];
			buffer[0] = static_cast<std::uint8_t>(Function::GraphicsContextCommand);
			buffer[1] = static_cast<std::uint8_t>(objectID & 0xFF);
			buffer[2] = static_cast<std::uint8_t>(objectID >> 8);
			buffer[3] = static_cast<std::uint8_t>(GraphicsContextSubCommandID::DrawPolygon);
			buffer[4] = numberOfPoints;
			for (uint8_t i = 0; i < numberOfPoints; i++)
			{
				buffer[5 + i] = static_cast<std::uint8_t>(listOfXOffsetsRelativeToCursor[0] & 0xFF);
				buffer[6 + i] = static_cast<std::uint8_t>((listOfXOffsetsRelativeToCursor[0] >> 8) & 0xFF);
				buffer[7 + i] = static_cast<std::uint8_t>(listOfYOffsetsRelativeToCursor[0] & 0xFF);
				buffer[8 + i] = static_cast<std::uint8_t>((listOfYOffsetsRelativeToCursor[0] >> 8) & 0xFF);
			}
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
			                                                        buffer,
			                                                        messageLength,
			                                                        myControlFunction.get(),
			                                                        partnerControlFunction.get(),
			                                                        CANIdentifier::PriorityLowest7);
			delete[] buffer;
		}
		return retVal;
	}

	bool VirtualTerminalClient::send_draw_text(std::uint16_t objectID, bool transparent, std::uint8_t textLength, const char *value)
	{
		bool retVal = false;

		if ((nullptr != value) &&
		    (0 != textLength))
		{
			std::uint16_t messageLength = (6 + textLength);
			std::uint8_t *buffer = new std::uint8_t[messageLength];
			buffer[0] = static_cast<std::uint8_t>(Function::GraphicsContextCommand);
			buffer[1] = static_cast<std::uint8_t>(objectID & 0xFF);
			buffer[2] = static_cast<std::uint8_t>(objectID >> 8);
			buffer[3] = static_cast<std::uint8_t>(GraphicsContextSubCommandID::DrawText);
			buffer[4] = static_cast<std::uint8_t>(transparent);
			buffer[5] = textLength;
			memcpy(buffer, value, textLength);
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
			                                                        buffer,
			                                                        messageLength,
			                                                        myControlFunction.get(),
			                                                        partnerControlFunction.get(),
			                                                        CANIdentifier::PriorityLowest7);
			delete[] buffer;
		}
		return retVal;
	}

	bool VirtualTerminalClient::send_pan_viewport(std::uint16_t objectID, std::int16_t xAttribute, std::int16_t yAttribute)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::PanViewport),
			                                             static_cast<std::uint8_t>(xAttribute & 0xFF),
			                                             static_cast<std::uint8_t>(xAttribute >> 8),
			                                             static_cast<std::uint8_t>(yAttribute & 0xFF),
			                                             static_cast<std::uint8_t>(yAttribute >> 8) };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_zoom_viewport(std::uint16_t objectID, float zoom)
	{
		std::uint8_t floatToBytesBuffer[sizeof(float)] = { 0 };

		for (std::uint8_t i = 0; i < sizeof(float); i++)
		{
			floatToBytesBuffer[i] = reinterpret_cast<std::uint8_t *>(&zoom)[i]; // Endianness?
		}

		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::ZoomViewport),
			                                             floatToBytesBuffer[0],
			                                             floatToBytesBuffer[1],
			                                             floatToBytesBuffer[2],
			                                             floatToBytesBuffer[3] };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_pan_and_zoom_viewport(std::uint16_t objectID, std::int16_t xAttribute, std::int16_t yAttribute, float zoom)
	{
		std::uint8_t floatToBytesBuffer[sizeof(float)] = { 0 };

		for (std::uint8_t i = 0; i < sizeof(float); i++)
		{
			floatToBytesBuffer[i] = reinterpret_cast<std::uint8_t *>(&zoom)[i]; // Endianness?
		}

		const std::uint8_t buffer[12] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                static_cast<std::uint8_t>(objectID & 0xFF),
			                                static_cast<std::uint8_t>(objectID >> 8),
			                                static_cast<std::uint8_t>(GraphicsContextSubCommandID::PanAndZoomViewport),
			                                static_cast<std::uint8_t>(xAttribute & 0xFF),
			                                static_cast<std::uint8_t>(xAttribute >> 8),
			                                static_cast<std::uint8_t>(yAttribute & 0xFF),
			                                static_cast<std::uint8_t>(yAttribute >> 8),
			                                floatToBytesBuffer[0],
			                                floatToBytesBuffer[1],
			                                floatToBytesBuffer[2],
			                                floatToBytesBuffer[3] };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      12,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_change_viewport_size(std::uint16_t objectID, std::uint16_t width, std::uint16_t height)
	{
		constexpr std::uint16_t MAX_WIDTH_HEIGHT = 32767;
		bool retVal = false;

		if ((width <= MAX_WIDTH_HEIGHT) &&
		    (height <= MAX_WIDTH_HEIGHT))
		{
			const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
				                                             static_cast<std::uint8_t>(objectID & 0xFF),
				                                             static_cast<std::uint8_t>(objectID >> 8),
				                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::ChangeViewportSize),
				                                             static_cast<std::uint8_t>(width & 0xFF),
				                                             static_cast<std::uint8_t>(width >> 8),
				                                             static_cast<std::uint8_t>(height & 0xFF),
				                                             static_cast<std::uint8_t>(height >> 8) };
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
			                                                        buffer,
			                                                        CAN_DATA_LENGTH,
			                                                        myControlFunction.get(),
			                                                        partnerControlFunction.get(),
			                                                        CANIdentifier::PriorityLowest7);
		}
		return retVal;
	}

	bool VirtualTerminalClient::send_draw_vt_object(std::uint16_t graphicsContextObjectID, std::uint16_t objectID)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(graphicsContextObjectID & 0xFF),
			                                             static_cast<std::uint8_t>(graphicsContextObjectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::DrawVTObject),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_copy_canvas_to_picture_graphic(std::uint16_t graphicsContextObjectID, std::uint16_t objectID)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(graphicsContextObjectID & 0xFF),
			                                             static_cast<std::uint8_t>(graphicsContextObjectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::CopyCanvasToPictureGraphic),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_copy_viewport_to_picture_graphic(std::uint16_t graphicsContextObjectID, std::uint16_t objectID)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(graphicsContextObjectID & 0xFF),
			                                             static_cast<std::uint8_t>(graphicsContextObjectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::CopyViewportToPictureGraphic),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_get_attribute_value(std::uint16_t objectID, std::uint8_t attributeID)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GetAttributeValueMessage),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             attributeID,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	std::uint8_t VirtualTerminalClient::get_softkey_x_axis_pixels() const
	{
		return softKeyXAxisPixels;
	}

	std::uint8_t VirtualTerminalClient::get_softkey_y_axis_pixels() const
	{
		return softKeyYAxisPixels;
	}

	std::uint8_t VirtualTerminalClient::get_number_virtual_softkeys() const
	{
		return numberVirtualSoftkeysPerSoftkeyMask;
	}

	std::uint8_t VirtualTerminalClient::get_number_physical_softkeys() const
	{
		return numberPhysicalSoftkeys;
	}

	bool VirtualTerminalClient::get_font_size_supported(FontSize value) const
	{
		bool retVal = false;

		switch (value)
		{
			case FontSize::Size6x8:
			case FontSize::Size8x8:
			case FontSize::Size8x12:
			case FontSize::Size12x16:
			case FontSize::Size16x16:
			case FontSize::Size16x24:
			case FontSize::Size24x32:
			case FontSize::Size32x32:
			{
				retVal = (0 != (smallFontSizesBitfield & (1 << static_cast<std::uint8_t>(value))));
			}
			break;

			case FontSize::Size32x48:
			case FontSize::Size48x64:
			case FontSize::Size64x64:
			case FontSize::Size64x96:
			case FontSize::Size96x128:
			case FontSize::Size128x128:
			case FontSize::Size128x192:
			{
				retVal = (0 != (largeFontSizesBitfield & (1 << (static_cast<std::uint8_t>(value) - static_cast<std::uint8_t>(FontSize::Size32x48)))));
			}
			break;
		}
		return retVal;
	}

	bool VirtualTerminalClient::get_font_style_supported(FontStyleBits value) const
	{
		return (0 != (static_cast<std::uint8_t>(value) & fontStylesBitfield));
	}

	VirtualTerminalClient::GraphicMode VirtualTerminalClient::get_graphic_mode() const
	{
		return supportedGraphicsMode;
	}

	bool VirtualTerminalClient::get_support_touchscreen_with_pointing_message() const
	{
		return (0 != (hardwareFeaturesBitfield & 0x01));
	}

	bool VirtualTerminalClient::get_support_pointing_device_with_pointing_message() const
	{
		return (0 != (hardwareFeaturesBitfield & 0x02));
	}

	bool VirtualTerminalClient::get_multiple_frequency_audio_output() const
	{
		return (0 != (hardwareFeaturesBitfield & 0x04));
	}

	bool VirtualTerminalClient::get_has_adjustable_volume_output() const
	{
		return (0 != (hardwareFeaturesBitfield & 0x08));
	}

	bool VirtualTerminalClient::get_support_simultaneous_activation_physical_keys() const
	{
		return (0 != (hardwareFeaturesBitfield & 0x10));
	}

	bool VirtualTerminalClient::get_support_simultaneous_activation_buttons_and_softkeys() const
	{
		return (0 != (hardwareFeaturesBitfield & 0x20));
	}

	bool VirtualTerminalClient::get_support_drag_operation() const
	{
		return (0 != (hardwareFeaturesBitfield & 0x40));
	}

	bool VirtualTerminalClient::get_support_intermediate_coordinates_during_drag_operations() const
	{
		return (0 != (hardwareFeaturesBitfield & 0x80));
	}

	std::uint16_t VirtualTerminalClient::get_number_x_pixels() const
	{
		return xPixels;
	}

	std::uint16_t VirtualTerminalClient::get_number_y_pixels() const
	{
		return yPixels;
	}

	VirtualTerminalClient::VTVersion VirtualTerminalClient::get_connected_vt_version() const
	{
		VTVersion retVal;

		switch (connectedVTVersion)
		{
			case 0x03:
			{
				retVal = VTVersion::Version3;
			}
			break;

			case 0x04:
			{
				retVal = VTVersion::Version4;
			}
			break;

			case 0x05:
			{
				retVal = VTVersion::Version5;
			}
			break;

			case 0x06:
			{
				retVal = VTVersion::Version6;
			}
			break;

			case 0xFF:
			{
				retVal = VTVersion::Version2OrOlder;
			}
			break;

			default:
			{
				retVal = VTVersion::ReservedOrUnknown;
			}
			break;
		}
		return retVal;
	}

	bool VirtualTerminalClient::get_vt_version_supported(VTVersion minimumVersion) const
	{
		bool retVal = false;

		if (get_connected_vt_version() != VTVersion::ReservedOrUnknown)
		{
			retVal = get_connected_vt_version() >= minimumVersion;
		}

		return retVal;
	}

	void VirtualTerminalClient::set_object_pool(std::uint8_t poolIndex, VTVersion poolSupportedVTVersion, const std::uint8_t *pool, std::uint32_t size, std::string version)
	{
		if ((nullptr != pool) &&
		    (0 != size))
		{
			ObjectPoolDataStruct tempData;

			tempData.objectPoolDataPointer = pool;
			tempData.objectPoolVectorPointer = nullptr;
			tempData.dataCallback = nullptr;
			tempData.objectPoolSize = size;
			tempData.autoScaleDataMaskOriginalDimension = 0;
			tempData.autoScaleSoftKeyDesignatorOriginalHeight = 0;
			tempData.version = poolSupportedVTVersion;
			tempData.useDataCallback = false;
			tempData.uploaded = false;
			tempData.versionLabel = version;

			if (poolIndex < objectPools.size())
			{
				objectPools[poolIndex] = tempData;
			}
			else
			{
				objectPools.resize(poolIndex + 1);
				objectPools[poolIndex] = tempData;
			}
		}
	}

	void VirtualTerminalClient::set_object_pool(std::uint8_t poolIndex, VTVersion poolSupportedVTVersion, const std::vector<std::uint8_t> *pool, std::string version)
	{
		if ((nullptr != pool) &&
		    (0 != pool->size()))
		{
			ObjectPoolDataStruct tempData;

			tempData.objectPoolDataPointer = nullptr;
			tempData.objectPoolVectorPointer = pool;
			tempData.dataCallback = nullptr;
			tempData.objectPoolSize = pool->size();
			tempData.autoScaleDataMaskOriginalDimension = 0;
			tempData.autoScaleSoftKeyDesignatorOriginalHeight = 0;
			tempData.version = poolSupportedVTVersion;
			tempData.useDataCallback = false;
			tempData.uploaded = false;
			tempData.versionLabel = version;

			if (poolIndex < objectPools.size())
			{
				objectPools[poolIndex] = tempData;
			}
			else
			{
				objectPools.resize(poolIndex + 1);
				objectPools[poolIndex] = tempData;
			}
		}
	}

	void VirtualTerminalClient::set_object_pool_scaling(std::uint8_t poolIndex,
	                                                    std::uint32_t originalDataMaskDimensions_px,
	                                                    std::uint32_t originalSoftKyeDesignatorHeight_px)
	{
		// You have to call set_object_pool or register_object_pool_data_chunk_callback before calling this function
		assert(poolIndex < objectPools.size());
		objectPools[poolIndex].autoScaleDataMaskOriginalDimension = originalDataMaskDimensions_px;
		objectPools[poolIndex].autoScaleSoftKeyDesignatorOriginalHeight = originalSoftKyeDesignatorHeight_px;
	}

	void VirtualTerminalClient::register_object_pool_data_chunk_callback(std::uint8_t poolIndex, VTVersion poolSupportedVTVersion, std::uint32_t poolTotalSize, DataChunkCallback value)
	{
		if ((nullptr != value) &&
		    (0 != poolTotalSize))
		{
			ObjectPoolDataStruct tempData;

			tempData.objectPoolDataPointer = nullptr;
			tempData.objectPoolVectorPointer = nullptr;
			tempData.dataCallback = value;
			tempData.objectPoolSize = poolTotalSize;
			tempData.version = poolSupportedVTVersion;
			tempData.useDataCallback = true;
			tempData.uploaded = false;

			if (poolIndex < objectPools.size())
			{
				objectPools[poolIndex] = tempData;
			}
			else
			{
				objectPools.resize(poolIndex + 1);
				objectPools[poolIndex] = tempData;
			}
		}
	}

	void VirtualTerminalClient::update()
	{
		StateMachineState previousStateMachineState = state; // Save state to see if it changes this update

		if (nullptr != partnerControlFunction)
		{
			switch (state)
			{
				case StateMachineState::Disconnected:
				{
					sendWorkingSetMaintenance = false;
					sendAuxiliaryMaintenance = false;

					if (partnerControlFunction->get_address_valid())
					{
						set_state(StateMachineState::WaitForPartnerVTStatusMessage);
					}
				}
				break;

				case StateMachineState::WaitForPartnerVTStatusMessage:
				{
					if (0 != lastVTStatusTimestamp_ms)
					{
						set_state(StateMachineState::SendWorkingSetMasterMessage);
					}
				}
				break;

				case StateMachineState::SendWorkingSetMasterMessage:
				{
					if (send_working_set_master())
					{
						set_state(StateMachineState::ReadyForObjectPool);
					}
				}
				break;

				case StateMachineState::ReadyForObjectPool:
				{
					// If we're in this state, we are ready to upload the
					// object pool but no pool has been set to this class
					// so the state machine cannot progress.
					if (SystemTiming::time_expired_ms(lastVTStatusTimestamp_ms, VT_STATUS_TIMEOUT_MS))
					{
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Ready to upload pool, but VT server has timed out. Disconnecting.");
						set_state(StateMachineState::Disconnected);
					}

					if (0 != objectPools.size())
					{
						set_state(StateMachineState::SendGetMemory);
						send_working_set_maintenance(true, objectPools[0].version);
						lastWorkingSetMaintenanceTimestamp_ms = SystemTiming::get_timestamp_ms();
						sendWorkingSetMaintenance = true;
						sendAuxiliaryMaintenance = true;
					}
				}
				break;

				case StateMachineState::SendGetMemory:
				{
					std::uint32_t totalPoolSize = 0;

					for (auto &pool : objectPools)
					{
						totalPoolSize += pool.objectPoolSize;
					}

					if (send_get_memory(totalPoolSize))
					{
						set_state(StateMachineState::WaitForGetMemoryResponse);
					}
				}
				break;

				case StateMachineState::WaitForGetMemoryResponse:
				{
					if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, VT_STATUS_TIMEOUT_MS))
					{
						set_state(StateMachineState::Failed);
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Get Memory Response Timeout");
					}
				}
				break;

				case StateMachineState::SendGetNumberSoftkeys:
				{
					if (send_get_number_of_softkeys())
					{
						set_state(StateMachineState::WaitForGetNumberSoftKeysResponse);
					}
				}
				break;

				case StateMachineState::WaitForGetNumberSoftKeysResponse:
				{
					if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, VT_STATUS_TIMEOUT_MS))
					{
						set_state(StateMachineState::Failed);
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Get Number Softkeys Response Timeout");
					}
				}
				break;

				case StateMachineState::SendGetTextFontData:
				{
					if (send_get_text_font_data())
					{
						set_state(StateMachineState::WaitForGetTextFontDataResponse);
					}
				}
				break;

				case StateMachineState::WaitForGetTextFontDataResponse:
				{
					if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, VT_STATUS_TIMEOUT_MS))
					{
						set_state(StateMachineState::Failed);
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Get Text Font Data Response Timeout");
					}
				}
				break;

				case StateMachineState::SendGetHardware:
				{
					if (send_get_hardware())
					{
						set_state(StateMachineState::WaitForGetHardwareResponse);
					}
				}
				break;

				case StateMachineState::WaitForGetHardwareResponse:
				{
					if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, VT_STATUS_TIMEOUT_MS))
					{
						set_state(StateMachineState::Failed);
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Get Hardware Response Timeout");
					}
				}
				break;

				case StateMachineState::SendGetVersions:
				{
					if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, VT_STATUS_TIMEOUT_MS))
					{
						set_state(StateMachineState::Failed);
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Get Versions Timeout");
					}
					else if ((!objectPools.empty()) &&
					         (!objectPools[0].versionLabel.empty()) &&
					         (send_get_versions()))
					{
						set_state(StateMachineState::WaitForGetVersionsResponse);
					}
				}
				break;

				case StateMachineState::WaitForGetVersionsResponse:
				{
					if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, VT_STATUS_TIMEOUT_MS))
					{
						set_state(StateMachineState::Failed);
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Get Versions Response Timeout");
					}
				}
				break;

				case StateMachineState::SendLoadVersion:
				{
					if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, VT_STATUS_TIMEOUT_MS))
					{
						set_state(StateMachineState::Failed);
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Send Load Version Timeout");
					}
					else
					{
						constexpr std::uint8_t VERSION_LABEL_LENGTH = 7;
						std::array<std::uint8_t, VERSION_LABEL_LENGTH> tempVersionBuffer;

						// Unused bytes filled with spaces
						tempVersionBuffer[0] = ' ';
						tempVersionBuffer[1] = ' ';
						tempVersionBuffer[2] = ' ';
						tempVersionBuffer[3] = ' ';
						tempVersionBuffer[4] = ' ';
						tempVersionBuffer[5] = ' ';
						tempVersionBuffer[6] = ' ';

						for (std::size_t i = 0; ((i < VERSION_LABEL_LENGTH) && (i < objectPools[0].versionLabel.size())); i++)
						{
							tempVersionBuffer[i] = objectPools[0].versionLabel[i];
						}

						if (send_load_version(tempVersionBuffer))
						{
							set_state(StateMachineState::WaitForLoadVersionResponse);
						}
					}
				}
				break;

				case StateMachineState::WaitForLoadVersionResponse:
				{
					if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, VT_STATUS_TIMEOUT_MS))
					{
						set_state(StateMachineState::Failed);
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Load Version Response Timeout");
					}
				}
				break;

				case StateMachineState::SendStoreVersion:
				{
					if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, VT_STATUS_TIMEOUT_MS))
					{
						set_state(StateMachineState::Failed);
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Send Store Version Timeout");
					}
					else
					{
						constexpr std::uint8_t VERSION_LABEL_LENGTH = 7;
						std::array<std::uint8_t, VERSION_LABEL_LENGTH> tempVersionBuffer;

						// Unused bytes filled with spaces
						tempVersionBuffer[0] = ' ';
						tempVersionBuffer[1] = ' ';
						tempVersionBuffer[2] = ' ';
						tempVersionBuffer[3] = ' ';
						tempVersionBuffer[4] = ' ';
						tempVersionBuffer[5] = ' ';
						tempVersionBuffer[6] = ' ';

						for (std::size_t i = 0; ((i < VERSION_LABEL_LENGTH) && (i < objectPools[0].versionLabel.size())); i++)
						{
							tempVersionBuffer[i] = objectPools[0].versionLabel[i];
						}

						if (send_store_version(tempVersionBuffer))
						{
							set_state(StateMachineState::WaitForStoreVersionResponse);
						}
					}
				}
				break;

				case StateMachineState::WaitForStoreVersionResponse:
				{
					if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, VT_STATUS_TIMEOUT_MS))
					{
						set_state(StateMachineState::Failed);
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Store Version Response Timeout");
					}
				}
				break;

				case StateMachineState::UploadObjectPool:
				{
					bool allPoolsProcessed = true;

					if (firstTimeInState)
					{
						if (get_any_pool_needs_scaling())
						{
							// Scale object pools before upload.
							if (!scale_object_pools())
							{
								set_state(StateMachineState::Failed);
							}
						}
					}

					for (std::uint32_t i = 0; i < objectPools.size(); i++)
					{
						if (((nullptr != objectPools[i].objectPoolDataPointer) ||
						     (nullptr != objectPools[i].dataCallback)) &&
						    (objectPools[i].objectPoolSize > 0))
						{
							if (!objectPools[i].uploaded)
							{
								allPoolsProcessed = false;
							}

							if (CurrentObjectPoolUploadState::Uninitialized == currentObjectPoolState)
							{
								if (!objectPools[i].uploaded)
								{
									bool transmitSuccessful = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
									                                                                         nullptr,
									                                                                         objectPools[i].objectPoolSize + 1, // Account for Mux byte
									                                                                         myControlFunction.get(),
									                                                                         partnerControlFunction.get(),
									                                                                         CANIdentifier::CANPriority::PriorityLowest7,
									                                                                         process_callback,
									                                                                         this,
									                                                                         process_internal_object_pool_upload_callback);

									if (transmitSuccessful)
									{
										currentObjectPoolState = CurrentObjectPoolUploadState::InProgress;
									}
								}
								else
								{
									// Pool already uploaded, move on to the next one
								}
							}
							else if (CurrentObjectPoolUploadState::Success == currentObjectPoolState)
							{
								objectPools[i].uploaded = true;
								currentObjectPoolState = CurrentObjectPoolUploadState::Uninitialized;
							}
							else if (CurrentObjectPoolUploadState::Failed == currentObjectPoolState)
							{
								currentObjectPoolState = CurrentObjectPoolUploadState::Uninitialized;
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: An object pool failed to upload. Resetting connection to VT.");
								set_state(StateMachineState::Disconnected);
							}
							else
							{
								// Transfer is in progress. Nothing to do now.
								break;
							}
						}
						else
						{
							CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: An object pool was supplied with an invalid size or pointer. Ignoring it.");
							objectPools[i].uploaded = true;
						}
					}

					if (allPoolsProcessed)
					{
						set_state(StateMachineState::SendEndOfObjectPool);
					}
				}
				break;

				case StateMachineState::SendEndOfObjectPool:
				{
					if (send_end_of_object_pool())
					{
						set_state(StateMachineState::WaitForEndOfObjectPoolResponse);
					}
				}
				break;

				case StateMachineState::WaitForEndOfObjectPoolResponse:
				{
					if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, VT_STATUS_TIMEOUT_MS))
					{
						set_state(StateMachineState::Failed);
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Get End of Object Pool Response Timeout");
					}
				}
				break;

				case StateMachineState::Connected:
				{
					// Check for timeouts
					if (SystemTiming::time_expired_ms(lastVTStatusTimestamp_ms, VT_STATUS_TIMEOUT_MS))
					{
						set_state(StateMachineState::Disconnected);
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Status Timeout");
					}
					update_auxiliary_input_status();
				}
				break;

				case StateMachineState::Failed:
				{
					constexpr std::uint32_t VT_STATE_MACHINE_RETRY_TIMEOUT_MS = 5000;
					sendWorkingSetMaintenance = false;
					sendAuxiliaryMaintenance = false;

					// Retry connecting after a while
					if (SystemTiming::time_expired_ms(stateMachineTimestamp_ms, VT_STATE_MACHINE_RETRY_TIMEOUT_MS))
					{
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Info, "[VT]: Resetting Failed VT Connection");
						set_state(StateMachineState::Disconnected);
					}
				}
				break;

				default:
				{
				}
				break;
			}
		}
		else
		{
			set_state(StateMachineState::Disconnected);
		}

		if ((sendWorkingSetMaintenance) &&
		    (SystemTiming::time_expired_ms(lastWorkingSetMaintenanceTimestamp_ms, WORKING_SET_MAINTENANCE_TIMEOUT_MS)))
		{
			txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendWorkingSetMaintenance));
		}
		if ((sendAuxiliaryMaintenance) &&
		    (!ourAuxiliaryInputs.empty()) &&
		    (SystemTiming::time_expired_ms(lastAuxiliaryMaintenanceTimestamp_ms, AUXILIARY_MAINTENANCE_TIMEOUT_MS)))
		{
			/// @todo We should make sure that when we disconnect/reconnect atleast 500ms has passed since the last auxiliary maintenance message
			txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendAuxiliaryMaintenance));
		}
		txFlags.process_all_flags();

		if (state == previousStateMachineState)
		{
			firstTimeInState = false;
		}
	}

	bool VirtualTerminalClient::send_delete_object_pool()
	{
		constexpr std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::DeleteObjectPoolCommand),
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_working_set_maintenance(bool initializing, VTVersion workingSetVersion)
	{
		std::uint8_t versionByte;
		std::uint8_t bitmask = (initializing ? 0x01 : 0x00);

		switch (workingSetVersion)
		{
			case VTVersion::Version3:
			{
				versionByte = 0x03;
			}
			break;

			case VTVersion::Version4:
			{
				versionByte = 0x04;
			}
			break;

			case VTVersion::Version5:
			{
				versionByte = 0x05;
			}
			break;

			case VTVersion::Version6:
			{
				versionByte = 0x06;
			}
			break;

			default:
			{
				versionByte = 0xFF;
			}
			break;
		}

		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::WorkingSetMaintenanceMessage),
			                                             bitmask,
			                                             versionByte,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_get_memory(std::uint32_t requiredMemory)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GetMemoryMessage),
			                                             0xFF,
			                                             static_cast<std::uint8_t>(requiredMemory & 0xFF),
			                                             static_cast<std::uint8_t>((requiredMemory >> 8) & 0xFF),
			                                             static_cast<std::uint8_t>((requiredMemory >> 16) & 0xFF),
			                                             static_cast<std::uint8_t>((requiredMemory >> 24) & 0xFF),
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_get_number_of_softkeys()
	{
		constexpr std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GetNumberOfSoftKeysMessage),
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_get_text_font_data()
	{
		constexpr std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GetTextFontDataMessage),
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_get_hardware()
	{
		constexpr std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GetHardwareMessage),
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_get_supported_widechars()
	{
		constexpr std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GetSupportedWidecharsMessage),
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_get_window_mask_data()
	{
		constexpr std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GetWindowMaskDataMessage),
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_get_supported_objects()
	{
		constexpr std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GetSupportedObjectsMessage),
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_get_versions()
	{
		constexpr std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GetVersionsMessage),
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_store_version(std::array<std::uint8_t, 7> versionLabel)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::StoreVersionCommand),
			                                             versionLabel[0],
			                                             versionLabel[1],
			                                             versionLabel[2],
			                                             versionLabel[3],
			                                             versionLabel[4],
			                                             versionLabel[5],
			                                             versionLabel[6] };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_load_version(std::array<std::uint8_t, 7> versionLabel)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::LoadVersionCommand),
			                                             versionLabel[0],
			                                             versionLabel[1],
			                                             versionLabel[2],
			                                             versionLabel[3],
			                                             versionLabel[4],
			                                             versionLabel[5],
			                                             versionLabel[6] };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_delete_version(std::array<std::uint8_t, 7> versionLabel)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::DeleteVersionCommand),
			                                             versionLabel[0],
			                                             versionLabel[1],
			                                             versionLabel[2],
			                                             versionLabel[3],
			                                             versionLabel[4],
			                                             versionLabel[5],
			                                             versionLabel[6] };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_extended_get_versions()
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ExtendedDeleteVersionCommand),
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_extended_store_version(std::array<std::uint8_t, 32> versionLabel)
	{
		bool retVal;

		std::uint8_t *buffer = new std::uint8_t[33];
		buffer[0] = static_cast<std::uint8_t>(Function::ExtendedStoreVersionCommand);
		memcpy(&buffer[1], versionLabel.data(), 32);
		retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                        buffer,
		                                                        33,
		                                                        myControlFunction.get(),
		                                                        partnerControlFunction.get(),
		                                                        CANIdentifier::PriorityLowest7);
		delete[] buffer;
		return retVal;
	}

	bool VirtualTerminalClient::send_extended_load_version(std::array<std::uint8_t, 32> versionLabel)
	{
		bool retVal;

		std::uint8_t *buffer = new std::uint8_t[33];
		buffer[0] = static_cast<std::uint8_t>(Function::ExtendedLoadVersionCommand);
		memcpy(&buffer[1], versionLabel.data(), 32);
		retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                        buffer,
		                                                        33,
		                                                        myControlFunction.get(),
		                                                        partnerControlFunction.get(),
		                                                        CANIdentifier::PriorityLowest7);
		delete[] buffer;
		return retVal;
	}

	bool VirtualTerminalClient::send_extended_delete_version(std::array<std::uint8_t, 32> versionLabel)
	{
		bool retVal;

		std::uint8_t *buffer = new std::uint8_t[33];
		buffer[0] = static_cast<std::uint8_t>(Function::ExtendedDeleteVersionCommand);
		memcpy(&buffer[1], versionLabel.data(), 32);
		retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                        buffer,
		                                                        33,
		                                                        myControlFunction.get(),
		                                                        partnerControlFunction.get(),
		                                                        CANIdentifier::PriorityLowest7);
		delete[] buffer;
		return retVal;
	}

	bool VirtualTerminalClient::send_end_of_object_pool() const
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::EndOfObjectPoolMessage),
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_working_set_master() const
	{
		constexpr std::uint8_t buffer[CAN_DATA_LENGTH] = { 0x01,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF,
			                                                 0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::WorkingSetMaster),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      nullptr,
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_auxiliary_functions_preferred_assignment() const
	{
		//! @todo load preferred assignment from saved configuration
		std::vector<std::uint8_t> buffer = { static_cast<std::uint8_t>(Function::PreferredAssignmentCommand), 0 };
		if (buffer.size() < CAN_DATA_LENGTH)
		{
			buffer.resize(CAN_DATA_LENGTH);
		}
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer.data(),
		                                                      buffer.size(),
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_auxiliary_function_assignment_response(std::uint16_t functionObjectID, bool hasError, bool isAlreadyAssigned) const
	{
		std::uint8_t errorCode = 0;
		if (hasError)
		{
			errorCode |= 0x01;
		}
		if ((isAlreadyAssigned) && (false == get_vt_version_supported(VTVersion::Version6)))
		{
			errorCode |= 0x02;
		}
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::AuxiliaryAssignmentTypeTwoCommand),
			                                             static_cast<std::uint8_t>(functionObjectID),
			                                             static_cast<std::uint8_t>(functionObjectID >> 8),
			                                             errorCode,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	bool VirtualTerminalClient::send_auxiliary_input_maintenance() const
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::AuxiliaryInputTypeTwoMaintenanceMessage),
			                                             static_cast<std::uint8_t>(ourModelIdentificationCode),
			                                             static_cast<std::uint8_t>(ourModelIdentificationCode >> 8),
			                                             static_cast<std::uint8_t>(StateMachineState::Connected == state ? 0x01 : 0x00),
			                                             0xFF,
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      nullptr,
		                                                      CANIdentifier::Priority3);
	}

	bool VirtualTerminalClient::send_auxiliary_input_status_enable_response(std::uint16_t objectID, bool isEnabled, bool invalidObjectID) const
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::AuxiliaryInputStatusTypeTwoEnableCommand),
			                                             static_cast<std::uint8_t>(objectID),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(isEnabled ? 0x01 : 0x00),
			                                             static_cast<std::uint8_t>(invalidObjectID ? 0x01 : 0x00),
			                                             0xFF,
			                                             0xFF,
			                                             0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
		                                                      buffer,
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get(),
		                                                      CANIdentifier::PriorityLowest7);
	}

	void VirtualTerminalClient::update_auxiliary_input_status()
	{
		for (auto &auxiliaryInput : ourAuxiliaryInputs)
		{
			update_auxiliary_input_status(auxiliaryInput.first);
		}
	}

	bool VirtualTerminalClient::update_auxiliary_input_status(std::uint16_t objectID)
	{
		bool retVal = false;
		AuxiliaryInputState &state = ourAuxiliaryInputs.at(objectID);
		/// @todo Change status message every 50ms to every 200ms for non-latched boolean inputs on interaction
		if (SystemTiming::time_expired_ms(state.lastStatusUpdate, AUXILIARY_INPUT_STATUS_DELAY) ||
		    (state.hasInteraction &&
		     !get_auxiliary_input_learn_mode_enabled() &&
		     SystemTiming::time_expired_ms(state.lastStatusUpdate, AUXILIARY_INPUT_STATUS_DELAY_INTERACTION)))
		{
			state.lastStatusUpdate = SystemTiming::get_timestamp_ms();

			std::uint8_t operatingState = 0;
			if (get_auxiliary_input_learn_mode_enabled())
			{
				operatingState |= 0x01;
				if (state.hasInteraction)
				{
					operatingState |= 0x02;
				}
			}
			if (state.controlLocked)
			{
				operatingState |= 0x04;
				if (state.hasInteraction)
				{
					operatingState |= 0x08;
				}
			}
			state.hasInteraction = false; // reset interaction flag

			/// @todo Change values based on state of auxiliary input, e.g. for non-latched boolean inputs we have to change from value=1 (momentary) to value=2 (held)
			const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::AuxiliaryInputTypeTwoStatusMessage),
				                                             static_cast<std::uint8_t>(objectID),
				                                             static_cast<std::uint8_t>(objectID >> 8),
				                                             static_cast<std::uint8_t>(state.value1),
				                                             static_cast<std::uint8_t>(state.value1 >> 8),
				                                             static_cast<std::uint8_t>(state.value2),
				                                             static_cast<std::uint8_t>(state.value2 >> 8),
				                                             operatingState };
			if (get_auxiliary_input_learn_mode_enabled())
			{
				retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal),
				                                                        buffer,
				                                                        CAN_DATA_LENGTH,
				                                                        myControlFunction.get(),
				                                                        partnerControlFunction.get(),
				                                                        CANIdentifier::Priority3);
			}
			else
			{
				retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU),
				                                                        buffer,
				                                                        CAN_DATA_LENGTH,
				                                                        myControlFunction.get(),
				                                                        nullptr,
				                                                        CANIdentifier::Priority3);
			}
		}
		return retVal;
	}

	void VirtualTerminalClient::set_state(StateMachineState value)
	{
		stateMachineTimestamp_ms = SystemTiming::get_timestamp_ms();

		if (value != state)
		{
			firstTimeInState = true;
		}

		state = value;

		if (StateMachineState::Disconnected == value)
		{
			lastVTStatusTimestamp_ms = 0;
			for (std::size_t i = 0; i < objectPools.size(); i++)
			{
				objectPools[i].uploaded = false;
			}
		}
	}

	void VirtualTerminalClient::process_flags(std::uint32_t flag, void *parent)
	{
		if ((flag <= static_cast<std::uint32_t>(TransmitFlags::NumberFlags)) &&
		    (nullptr != parent))
		{
			TransmitFlags flagToProcess = static_cast<TransmitFlags>(flag);
			VirtualTerminalClient *vtClient = reinterpret_cast<VirtualTerminalClient *>(parent);
			bool transmitSuccessful = false;

			switch (flagToProcess)
			{
				case TransmitFlags::SendWorkingSetMaintenance:
				{
					if (vtClient->objectPools.size() > 0)
					{
						transmitSuccessful = vtClient->send_working_set_maintenance(false, vtClient->objectPools[0].version);

						if (transmitSuccessful)
						{
							vtClient->lastWorkingSetMaintenanceTimestamp_ms = SystemTiming::get_timestamp_ms();
						}
					}
				}
				break;

				case TransmitFlags::SendAuxiliaryMaintenance:
				{
					transmitSuccessful = vtClient->send_auxiliary_input_maintenance();

					if (transmitSuccessful)
					{
						vtClient->lastAuxiliaryMaintenanceTimestamp_ms = SystemTiming::get_timestamp_ms();
					}
				}
				break;

				case TransmitFlags::NumberFlags:
				default:
				{
				}
				break;
			}

			if (false == transmitSuccessful)
			{
				vtClient->txFlags.set_flag(flag);
			}
		}
	}

	void VirtualTerminalClient::process_rx_message(CANMessage *message, void *parentPointer)
	{
		if ((nullptr != message) &&
		    (nullptr != parentPointer) &&
		    (CAN_DATA_LENGTH <= message->get_data_length()))
		{
			VirtualTerminalClient *parentVT = reinterpret_cast<VirtualTerminalClient *>(parentPointer);
			switch (message->get_identifier().get_parameter_group_number())
			{
				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::Acknowledge):
				{
					if (AcknowledgementType::Negative == static_cast<AcknowledgementType>(message->get_uint8_at(0)))
					{
						std::uint32_t targetParameterGroupNumber = message->get_uint24_at(5);
						if (static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal) == targetParameterGroupNumber)
						{
							CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: The VT Server is NACK-ing our VT messages. Disconnecting.");
							parentVT->set_state(StateMachineState::Disconnected);
						}
					}
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU):
				{
					switch (message->get_uint8_at(0))
					{
						case static_cast<std::uint8_t>(Function::SoftKeyActivationMessage):
						{
							std::uint8_t keyCode = message->get_uint8_at(1);
							if (keyCode <= static_cast<std::uint8_t>(KeyActivationCode::ButtonPressAborted))
							{
								std::uint16_t objectID = message->get_uint16_at(2);
								std::uint16_t parentObjectID = message->get_uint16_at(4);
								std::uint8_t keyNumber = message->get_uint8_at(6);
								if (parentVT->get_vt_version_supported(VTVersion::Version6))
								{
									//! @todo process TAN
								}

								parentVT->softKeyEventDispatcher.invoke({ parentVT, objectID, parentObjectID, keyNumber, static_cast<KeyActivationCode>(keyCode) });
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::ButtonActivationMessage):
						{
							std::uint8_t keyCode = message->get_uint8_at(1);
							if (keyCode <= static_cast<std::uint8_t>(KeyActivationCode::ButtonPressAborted))
							{
								std::uint16_t objectID = message->get_uint16_at(2);
								std::uint16_t parentObjectID = message->get_uint16_at(4);
								std::uint8_t keyNumber = message->get_uint8_at(6);
								if (parentVT->get_vt_version_supported(VTVersion::Version6))
								{
									//! @todo process TAN
								}
								parentVT->buttonEventDispatcher.invoke({ parentVT, objectID, parentObjectID, keyNumber, static_cast<KeyActivationCode>(keyCode) });
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::PointingEventMessage):
						{
							std::uint16_t xPosition = message->get_uint16_at(1);
							std::uint16_t yPosition = message->get_uint16_at(3);

							std::uint8_t touchState = static_cast<std::uint8_t>(KeyActivationCode::ButtonPressedOrLatched);
							std::uint16_t parentMaskObjectID = NULL_OBJECT_ID;
							if (parentVT->get_vt_version_supported(VTVersion::Version6))
							{
								// VT version is at least 6
								touchState = message->get_uint8_at(5) & 0x0F;
								parentMaskObjectID = message->get_uint16_at(6);
								//! @todo process TAN
							}
							else if (parentVT->get_vt_version_supported(VTVersion::Version4))
							{
								// VT version is either 4 or 5
								touchState = message->get_uint8_at(5);
							}

							if (touchState <= static_cast<std::uint8_t>(KeyActivationCode::ButtonPressAborted))
							{
								parentVT->pointingEventDispatcher.invoke({ parentVT, xPosition, yPosition, parentMaskObjectID, static_cast<KeyActivationCode>(touchState) });
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::VTSelectInputObjectMessage):
						{
							std::uint16_t objectID = message->get_uint16_at(1);
							bool objectSelected = (0x01 == message->get_uint8_at(3));
							bool objectOpenForInput = true;

							if (parentVT->get_vt_version_supported(VTVersion::Version4))
							{
								objectOpenForInput = message->get_bool_at(4, 0);
							}

							if (parentVT->get_vt_version_supported(VTVersion::Version6))
							{
								//! @todo process TAN
							}

							parentVT->selectInputObjectEventDispatcher.invoke({ parentVT, objectID, objectSelected, objectOpenForInput });
						}
						break;

						case static_cast<std::uint8_t>(Function::VTESCMessage):
						{
							std::uint16_t objectID = message->get_uint16_at(1);
							std::uint8_t errorCode = message->get_uint8_at(3) & 0x1F;
							if ((errorCode == static_cast<std::uint8_t>(ESCMessageErrorCode::OtherError)) ||
							    (errorCode <= static_cast<std::uint8_t>(ESCMessageErrorCode::NoInputFieldOpen)))
							{
								if (parentVT->get_vt_version_supported(VTVersion::Version6))
								{
									//! @todo process TAN
								}

								parentVT->escMessageEventDispatcher.invoke({ parentVT, objectID, static_cast<ESCMessageErrorCode>(errorCode) });
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::VTChangeNumericValueMessage):
						{
							std::uint16_t objectID = message->get_uint16_at(1);
							std::uint32_t value = message->get_uint32_at(4);

							if (parentVT->get_vt_version_supported(VTVersion::Version6))
							{
								//! @todo process TAN
							}
							parentVT->changeNumericValueEventDispatcher.invoke({ parentVT, value, objectID });
						}
						break;

						case static_cast<std::uint8_t>(Function::VTChangeActiveMaskMessage):
						{
							std::uint16_t maskObjectID = message->get_uint16_at(1);

							bool missingObjects = message->get_bool_at(3, 2);
							bool maskOrChildHasErrors = message->get_bool_at(3, 3);
							bool anyOtherError = message->get_bool_at(3, 4);
							bool poolDeleted = message->get_bool_at(3, 5);

							std::uint16_t errorObjectID = message->get_uint16_at(4);
							std::uint16_t parentObjectID = message->get_uint16_at(6);

							parentVT->changeActiveMaskEventDispatcher.invoke({ parentVT,
							                                                   maskObjectID,
							                                                   errorObjectID,
							                                                   parentObjectID,
							                                                   missingObjects,
							                                                   maskOrChildHasErrors,
							                                                   anyOtherError,
							                                                   poolDeleted });
						}
						break;

						case static_cast<std::uint8_t>(Function::VTChangeSoftKeyMaskMessage):
						{
							std::uint16_t dataOrAlarmMaskID = message->get_uint16_at(1);
							std::uint16_t softKeyMaskID = message->get_uint16_at(3);

							bool missingObjects = message->get_bool_at(5, 2);
							bool maskOrChildHasErrors = message->get_bool_at(5, 3);
							bool anyOtherError = message->get_bool_at(5, 4);
							bool poolDeleted = message->get_bool_at(5, 5);

							parentVT->changeSoftKeyMaskEventDispatcher.invoke({ parentVT,
							                                                    dataOrAlarmMaskID,
							                                                    softKeyMaskID,
							                                                    missingObjects,
							                                                    maskOrChildHasErrors,
							                                                    anyOtherError,
							                                                    poolDeleted });
						}
						break;

						case static_cast<std::uint8_t>(Function::VTChangeStringValueMessage):
						{
							std::uint16_t objectID = message->get_uint16_at(1);
							std::uint8_t stringLength = message->get_uint8_at(3);
							std::string value = std::string(message->get_data().begin() + 4, message->get_data().begin() + 4 + stringLength);

							parentVT->changeStringValueEventDispatcher.invoke({ value, parentVT, objectID });
						}
						break;

						case static_cast<std::uint8_t>(Function::VTOnUserLayoutHideShowMessage):
						{
							std::uint16_t objectID = message->get_uint16_at(1);
							bool hidden = !message->get_bool_at(3, 0);

							parentVT->userLayoutHideShowEventDispatcher.invoke({ parentVT, objectID, hidden });

							// There could be two layout messages in one packet
							objectID = message->get_uint16_at(4);
							if (objectID != NULL_OBJECT_ID)
							{
								hidden = !message->get_bool_at(6, 0);
								parentVT->userLayoutHideShowEventDispatcher.invoke({ parentVT, objectID, hidden });
							}

							if (parentVT->get_vt_version_supported(VTVersion::Version6))
							{
								//! @todo process TAN
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::VTControlAudioSignalTerminationMessage):
						{
							bool terminated = message->get_bool_at(1, 0);

							parentVT->audioSignalTerminationEventDispatcher.invoke({ parentVT, terminated });

							if (parentVT->get_vt_version_supported(VTVersion::Version6))
							{
								//! @todo process TAN
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::PreferredAssignmentCommand):
						{
							if (message->get_bool_at(1, 0))
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[AUX-N]: Preferred Assignment Error - Auxiliary Input Unit(s) (NAME or Model Identification Code) not valid");
							}
							if (message->get_bool_at(1, 1))
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[AUX-N]: Preferred Assignment Error - Function Object ID(S) not valid");
							}
							if (message->get_bool_at(1, 2))
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[AUX-N]: Preferred Assignment Error - Input Object ID(s) not valid");
							}
							if (message->get_bool_at(1, 3))
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[AUX-N]: Preferred Assignment Error - Duplicate Object ID of Auxiliary Function");
							}
							if (message->get_bool_at(1, 4))
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[AUX-N]: Preferred Assignment Error - Other");
							}

							if (0 != message->get_uint8_at(1))
							{
								std::uint16_t faultyObjectID = message->get_uint16_at(2);
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[AUX-N]: Auxiliary Function Object ID of faulty assignment: " + isobus::to_string(faultyObjectID));
							}
							else
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Debug, "[AUX-N]: Preferred Assignment OK");
								//! @todo load the preferred assignment into parentVT->assignedAuxiliaryInputDevices
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::AuxiliaryAssignmentTypeTwoCommand):
						{
							if (14 == message->get_data_length())
							{
								std::uint64_t isoName = message->get_uint64_at(1);
								bool storeAsPreferred = message->get_bool_at(9, 7);
								std::uint8_t functionType = (message->get_uint8_at(9) & 0x1F);
								std::uint16_t inputObjectID = message->get_uint16_at(10);
								std::uint16_t functionObjectID = message->get_uint16_at(12);

								bool hasError = false;
								bool isAlreadyAssigned = false;
								if (DEFAULT_NAME == isoName && 0x1F == functionType)
								{
									if (NULL_OBJECT_ID == functionObjectID)
									{
										for (AssignedAuxiliaryInputDevice &aux : parentVT->assignedAuxiliaryInputDevices)
										{
											aux.functions.clear();
										}
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Info, "[AUX-N] Unassigned all functions");
									}
									else if (NULL_OBJECT_ID == inputObjectID)
									{
										for (AssignedAuxiliaryInputDevice &aux : parentVT->assignedAuxiliaryInputDevices)
										{
											for (auto iter = aux.functions.begin(); iter != aux.functions.end();)
											{
												if (iter->functionObjectID == functionObjectID)
												{
													aux.functions.erase(iter);
													if (storeAsPreferred)
													{
														//! @todo save preferred assignment to persistent configuration
													}
													CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Info, "[AUX-N] Unassigned function " + isobus::to_string(static_cast<int>(functionObjectID)) + " from input " + isobus::to_string(static_cast<int>(inputObjectID)));
												}
												else
												{
													++iter;
												}
											}
										}
									}
								}
								else
								{
									auto result = std::find_if(parentVT->assignedAuxiliaryInputDevices.begin(), parentVT->assignedAuxiliaryInputDevices.end(), [&isoName](const AssignedAuxiliaryInputDevice &aux) {
										return aux.name == isoName;
									});
									if (result != std::end(parentVT->assignedAuxiliaryInputDevices))
									{
										if (static_cast<std::uint8_t>(AuxiliaryTypeTwoFunctionType::QuadratureBooleanMomentary) >= functionType)
										{
											AssignedAuxiliaryFunction assignment(functionObjectID, inputObjectID, static_cast<AuxiliaryTypeTwoFunctionType>(functionType));
											auto location = std::find(result->functions.begin(), result->functions.end(), assignment);
											if (location == std::end(result->functions))
											{
												result->functions.push_back(assignment);
												if (storeAsPreferred)
												{
													//! @todo save preferred assignment to persistent configuration
												}
												CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Info, "[AUX-N]: Assigned function " + isobus::to_string(static_cast<int>(functionObjectID)) + " to input " + isobus::to_string(static_cast<int>(inputObjectID)));
											}
											else
											{
												hasError = true;
												isAlreadyAssigned = true;
												CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[AUX-N]: Unable to store preferred assignment due to missing auxiliary input device with name: " + isobus::to_string(isoName));
											}
										}
										else
										{
											hasError = true;
											CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[AUX-N]: Unable to store preferred assignment due to unsupported function type: " + isobus::to_string(functionType));
										}
									}
									else
									{
										hasError = true;
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[AUX-N]: Unable to store preferred assignment due to missing auxiliary input device with name: " + isobus::to_string(isoName));
									}
								}
								parentVT->send_auxiliary_function_assignment_response(functionObjectID, hasError, isAlreadyAssigned);
							}
							else
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[AUX-N]: Received AuxiliaryAssignmentTypeTwoCommand with wrong data length: " + isobus::to_string(message->get_data_length()) + " but expected 14.");
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::AuxiliaryInputTypeTwoStatusMessage):
						{
							std::uint16_t inputObjectID = message->get_uint16_at(1);
							std::uint16_t value1 = message->get_uint16_at(3);
							std::uint16_t value2 = message->get_uint16_at(5);
							/// @todo figure out how to best pass other status properties below to application
							/// @todo The standard requires us to not perform any auxiliary function when learn mode is active, so we probably want to let the application know about that somehow
							// bool learnModeActive = message->get_bool_at(7, 0);
							// bool inputActive = message->get_bool_at(7, 1); // Only in learn mode?
							// bool controlIsLocked = false;
							// bool interactionWhileLocked = false;
							if (parentVT->get_vt_version_supported(VTVersion::Version6))
							{
								// controlIsLocked = message->get_bool_at(7, 2);
								// interactionWhileLocked = message->get_bool_at(7, 3);
							}
							for (AssignedAuxiliaryInputDevice &aux : parentVT->assignedAuxiliaryInputDevices)
							{
								auto result = std::find_if(aux.functions.begin(), aux.functions.end(), [&inputObjectID](const AssignedAuxiliaryFunction &assignment) {
									return assignment.inputObjectID == inputObjectID;
								});
								if (aux.functions.end() != result)
								{
									parentVT->auxiliaryFunctionEventDispatcher.invoke({ *result, parentVT, value1, value2 });
								}
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::AuxiliaryInputStatusTypeTwoEnableCommand):
						{
							std::uint16_t inputObjectID = message->get_uint16_at(1);
							bool shouldEnable = message->get_bool_at(3, 0);
							auto result = std::find_if(parentVT->ourAuxiliaryInputs.begin(), parentVT->ourAuxiliaryInputs.end(), [&inputObjectID](const std::pair<std::uint16_t, AuxiliaryInputState> &input) {
								return input.first == inputObjectID;
							});
							bool isInvalidObjectID = (result == std::end(parentVT->ourAuxiliaryInputs));
							if (!isInvalidObjectID)
							{
								result->second.enabled = shouldEnable;
							}
							parentVT->send_auxiliary_input_status_enable_response(inputObjectID, isInvalidObjectID ? false : shouldEnable, isInvalidObjectID);
						}
						break;

						case static_cast<std::uint8_t>(Function::VTStatusMessage):
						{
							parentVT->lastVTStatusTimestamp_ms = SystemTiming::get_timestamp_ms();
							parentVT->activeWorkingSetMasterAddress = message->get_uint8_at(1);
							parentVT->activeWorkingSetDataMaskObjectID = message->get_uint16_at(2);
							parentVT->activeWorkingSetSoftkeyMaskObjectID = message->get_uint16_at(4);
							parentVT->busyCodesBitfield = message->get_uint8_at(6);
							parentVT->currentCommandFunctionCode = message->get_uint8_at(7);
						}
						break;

						case static_cast<std::uint8_t>(Function::GetMemoryMessage):
						{
							if (StateMachineState::WaitForGetMemoryResponse == parentVT->state)
							{
								parentVT->connectedVTVersion = message->get_uint8_at(1);

								if (0 == message->get_uint8_at(2))
								{
									// There IS enough memory
									parentVT->set_state(StateMachineState::SendGetNumberSoftkeys);
								}
								else
								{
									parentVT->set_state(StateMachineState::Failed);
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Connection Failed Not Enough Memory");
								}
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::GetNumberOfSoftKeysMessage):
						{
							if (StateMachineState::WaitForGetNumberSoftKeysResponse == parentVT->state)
							{
								parentVT->softKeyXAxisPixels = message->get_uint8_at(4);
								parentVT->softKeyYAxisPixels = message->get_uint8_at(5);
								parentVT->numberVirtualSoftkeysPerSoftkeyMask = message->get_uint8_at(6);
								parentVT->numberPhysicalSoftkeys = message->get_uint8_at(7);
								parentVT->set_state(StateMachineState::SendGetTextFontData);
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::GetTextFontDataMessage):
						{
							if (StateMachineState::WaitForGetTextFontDataResponse == parentVT->state)
							{
								parentVT->smallFontSizesBitfield = message->get_uint8_at(5);
								parentVT->largeFontSizesBitfield = message->get_uint8_at(6);
								parentVT->fontStylesBitfield = message->get_uint8_at(7);
								parentVT->set_state(StateMachineState::SendGetHardware);
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::GetHardwareMessage):
						{
							if (StateMachineState::WaitForGetHardwareResponse == parentVT->state)
							{
								if (message->get_uint8_at(2) <= static_cast<std::uint8_t>(GraphicMode::TwoHundredFiftySixColour))
								{
									parentVT->supportedGraphicsMode = static_cast<GraphicMode>(message->get_uint8_at(2));
								}
								parentVT->hardwareFeaturesBitfield = message->get_uint8_at(3);
								parentVT->xPixels = message->get_uint16_at(4);
								parentVT->yPixels = message->get_uint16_at(6);
								parentVT->lastObjectPoolIndex = 0;

								// Check if we need to ask for pool versions
								// Ony check the first pool, all pools are labeled the same per working set.
								if ((!parentVT->objectPools.empty()) &&
								    (!parentVT->objectPools[0].versionLabel.empty()))
								{
									parentVT->set_state(StateMachineState::SendGetVersions);
								}
								else
								{
									parentVT->set_state(StateMachineState::UploadObjectPool);
								}
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::GetVersionsResponse):
						{
							if (StateMachineState::WaitForGetVersionsResponse == parentVT->state)
							{
								// See if the server returned any labels
								const std::uint8_t numberOfLabels = message->get_uint8_at(1);
								constexpr std::size_t LABEL_LENGTH = 7;

								if (numberOfLabels > 0)
								{
									// Check for label match
									bool labelMatched = false;
									const std::size_t remainingLength = (2 + (LABEL_LENGTH * numberOfLabels));

									if (message->get_data_length() >= remainingLength)
									{
										for (std::uint_fast8_t i = 0; i < numberOfLabels; i++)
										{
											char tempStringLabel[8] = { 0 };
											tempStringLabel[0] = message->get_uint8_at(2 + (LABEL_LENGTH * i));
											tempStringLabel[1] = message->get_uint8_at(3 + (LABEL_LENGTH * i));
											tempStringLabel[2] = message->get_uint8_at(4 + (LABEL_LENGTH * i));
											tempStringLabel[3] = message->get_uint8_at(5 + (LABEL_LENGTH * i));
											tempStringLabel[4] = message->get_uint8_at(6 + (LABEL_LENGTH * i));
											tempStringLabel[5] = message->get_uint8_at(7 + (LABEL_LENGTH * i));
											tempStringLabel[6] = message->get_uint8_at(8 + (LABEL_LENGTH * i));
											tempStringLabel[7] = '\0';
											std::string labelDecoded(tempStringLabel);
											std::string tempActualLabel(parentVT->objectPools[0].versionLabel);

											// Check if we need to manipulate the passed in label by padding with spaces
											while (tempActualLabel.size() < LABEL_LENGTH)
											{
												tempActualLabel.push_back(' ');
											}

											if (tempActualLabel.size() > LABEL_LENGTH)
											{
												tempActualLabel.resize(LABEL_LENGTH);
											}

											if (tempActualLabel == labelDecoded)
											{
												labelMatched = true;
												parentVT->set_state(StateMachineState::SendLoadVersion);
												CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Info, "[VT]: VT Server has a matching label for " + isobus::to_string(labelDecoded) + ". It will be loaded and upload will be skipped.");
												break;
											}
											else
											{
												CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Info, "[VT]: VT Server has a label for " + isobus::to_string(labelDecoded) + ". This version will be deleted.");
												const std::array<std::uint8_t, 7> deleteBuffer = {
													static_cast<std::uint8_t>(labelDecoded[0]),
													static_cast<std::uint8_t>(labelDecoded[1]),
													static_cast<std::uint8_t>(labelDecoded[2]),
													static_cast<std::uint8_t>(labelDecoded[3]),
													static_cast<std::uint8_t>(labelDecoded[4]),
													static_cast<std::uint8_t>(labelDecoded[5]),
													static_cast<std::uint8_t>(labelDecoded[6])
												};
												if (!parentVT->send_delete_version(deleteBuffer))
												{
													CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: Failed to send the delete version message for label " + isobus::to_string(labelDecoded));
												}
											}
										}
										if (!labelMatched)
										{
											CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Info, "[VT]: No version label from the VT matched. Client will upload the pool and store it instead.");
											parentVT->set_state(StateMachineState::UploadObjectPool);
										}
									}
									else
									{
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: Get Versions Response length is not long enough. Message ignored.");
									}
								}
								else
								{
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Info, "[VT]: No version label from the VT matched. Client will upload the pool and store it instead.");
									parentVT->set_state(StateMachineState::UploadObjectPool);
								}
							}
							else
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: Get Versions Response ignored!");
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::LoadVersionCommand):
						{
							if (StateMachineState::WaitForLoadVersionResponse == parentVT->state)
							{
								if (0 == message->get_uint8_at(5))
								{
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Info, "[VT]: Loaded object pool version from VT non-volatile memory with no errors.");
									parentVT->set_state(StateMachineState::Connected);

									//! @todo maybe a better way available than relying on aux function callbacks registered?
									if (parentVT->auxiliaryFunctionEventDispatcher.get_listener_count() > 0)
									{
										if (parentVT->send_auxiliary_functions_preferred_assignment())
										{
											CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Debug, "[AUX-N]: Sent preferred assignments after LoadVersionCommand.");
										}
										else
										{
											CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[AUX-N]: Failed to send preferred assignments after LoadVersionCommand.");
										}
									}
								}
								else
								{
									// At least one error is set
									if (message->get_bool_at(5, 0))
									{
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: Load Versions Response error: File system error or corruption.");
									}
									if (message->get_bool_at(5, 1))
									{
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: Load Versions Response error: Insufficient memory.");
									}
									if (message->get_bool_at(5, 2))
									{
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: Load Versions Response error: Any other error.");
									}

									// Not sure what happened here... should be mostly impossible. Try to upload instead.
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: Switching to pool upload instead.");
									parentVT->set_state(StateMachineState::UploadObjectPool);
								}
							}
							else
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: Load Versions Response ignored!");
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::StoreVersionCommand):
						{
							if (StateMachineState::WaitForStoreVersionResponse == parentVT->state)
							{
								if (0 == message->get_uint8_at(5))
								{
									// Stored with no error
									parentVT->set_state(StateMachineState::Connected);
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Info, "[VT]: Stored object pool with no error.");
								}
								else
								{
									// At least one error is set
									if (message->get_bool_at(5, 0))
									{
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: Store Versions Response error: Version label is not correct.");
									}
									if (message->get_bool_at(5, 1))
									{
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: Store Versions Response error: Insufficient memory.");
									}
									if (message->get_bool_at(5, 2))
									{
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: Store Versions Response error: Any other error.");
									}
								}
							}
							else
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: Store Versions Response ignored!");
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::DeleteVersionCommand):
						{
							if (0 == message->get_uint8_at(5))
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Info, "[VT]: Delete Version Response OK!");
							}
							else
							{
								if (message->get_bool_at(5, 1))
								{
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: Delete Version Response error: Version label is not correct, or unknown.");
								}
								if (message->get_bool_at(5, 3))
								{
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: Delete Version Response error: Any other error.");
								}
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::EndOfObjectPoolMessage):
						{
							if (StateMachineState::WaitForEndOfObjectPoolResponse == parentVT->state)
							{
								bool anyErrorInPool = message->get_bool_at(1, 0);
								bool vtRanOutOfMemory = message->get_bool_at(1, 1);
								bool otherErrors = message->get_bool_at(1, 3);
								std::uint16_t parentObjectIDOfFaultyObject = message->get_uint16_at(2);
								std::uint16_t objectIDOfFaultyObject = message->get_uint16_at(4);
								std::uint8_t objectPoolErrorBitmask = message->get_uint8_at(6);

								if ((!anyErrorInPool) &&
								    (0 == objectPoolErrorBitmask))
								{
									// Clear scaling buffers
									for (auto &objectPool : parentVT->objectPools)
									{
										objectPool.scaledObjectPool.clear();
									}

									// Check if we need to store this pool
									if (!parentVT->objectPools[0].versionLabel.empty())
									{
										parentVT->set_state(StateMachineState::SendStoreVersion);
									}
									else
									{
										parentVT->set_state(StateMachineState::Connected);
									}
									//! @todo maybe a better way available than relying on aux function callbacks registered?
									if (parentVT->auxiliaryFunctionEventDispatcher.get_listener_count() > 0)
									{
										if (parentVT->send_auxiliary_functions_preferred_assignment())
										{
											CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Debug, "[AUX-N]: Sent preferred assignments after EndOfObjectPoolMessage.");
										}
										else
										{
											CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[AUX-N]: Failed to send preferred assignments after EndOfObjectPoolMessage.");
										}
									}
								}
								else
								{
									parentVT->set_state(StateMachineState::Failed);
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Error in end of object pool message." + std::string("Faulty Object ") + isobus::to_string(static_cast<int>(objectIDOfFaultyObject)) + std::string(" Faulty Object Parent ") + isobus::to_string(static_cast<int>(parentObjectIDOfFaultyObject)) + std::string(" Pool error bitmask value ") + isobus::to_string(static_cast<int>(objectPoolErrorBitmask)));
									if (vtRanOutOfMemory)
									{
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Ran out of memory");
									}
									if (otherErrors)
									{
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[VT]: Reported other errors in EOM response");
									}
								}
							}
						}
						break;
					}
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal):
				{
					switch (message->get_uint8_at(0))
					{
						case static_cast<std::uint8_t>(Function::AuxiliaryInputTypeTwoMaintenanceMessage):
						{
							std::uint16_t modelIdentificationCode = message->get_uint16_at(1);
							bool ready = message->get_uint8_at(3);

							if (ready)
							{
								auto result = std::find_if(parentVT->assignedAuxiliaryInputDevices.begin(), parentVT->assignedAuxiliaryInputDevices.end(), [&modelIdentificationCode](const AssignedAuxiliaryInputDevice &aux) {
									return aux.modelIdentificationCode == modelIdentificationCode;
								});
								if (result == std::end(parentVT->assignedAuxiliaryInputDevices))
								{
									AssignedAuxiliaryInputDevice inputDevice{ message->get_source_control_function()->get_NAME().get_full_name(), modelIdentificationCode, {} };
									parentVT->assignedAuxiliaryInputDevices.push_back(inputDevice);
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Info, "[AUX-N]: New auxiliary input device with name: " + isobus::to_string(inputDevice.name) + " and model identification code: " + isobus::to_string(modelIdentificationCode));
								}
							}
						}
						break;
					}
				}
				break;

				default:
				{
					CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: Client unknown message: " + isobus::to_string(static_cast<int>(message->get_identifier().get_parameter_group_number())));
				}
				break;
			}
		}
		else
		{
			CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[VT]: VT-ECU Client message invalid");
		}
	}

	void VirtualTerminalClient::process_callback(std::uint32_t parameterGroupNumber,
	                                             std::uint32_t,
	                                             InternalControlFunction *,
	                                             ControlFunction *destinationControlFunction,
	                                             bool successful,
	                                             void *parentPointer)
	{
		if ((nullptr != parentPointer) &&
		    (static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal) == parameterGroupNumber) &&
		    (nullptr != destinationControlFunction))
		{
			VirtualTerminalClient *parent = reinterpret_cast<VirtualTerminalClient *>(parentPointer);

			if (StateMachineState::UploadObjectPool == parent->state)
			{
				if (successful)
				{
					parent->currentObjectPoolState = CurrentObjectPoolUploadState::Success;
				}
				else
				{
					parent->currentObjectPoolState = CurrentObjectPoolUploadState::Failed;
				}
			}
		}
	}

	bool VirtualTerminalClient::process_internal_object_pool_upload_callback(std::uint32_t callbackIndex,
	                                                                         std::uint32_t bytesOffset,
	                                                                         std::uint32_t numberOfBytesNeeded,
	                                                                         std::uint8_t *chunkBuffer,
	                                                                         void *parentPointer)
	{
		bool retVal = false;

		if ((nullptr != parentPointer) &&
		    (nullptr != chunkBuffer) &&
		    (0 != numberOfBytesNeeded))
		{
			VirtualTerminalClient *parentVTClient = reinterpret_cast<VirtualTerminalClient *>(parentPointer);
			std::uint32_t poolIndex = std::numeric_limits<std::uint32_t>::max();
			bool usingExternalCallback = false;

			// Need to figure out which pool we're currently uploading
			for (std::uint32_t i = 0; i < parentVTClient->objectPools.size(); i++)
			{
				if (!parentVTClient->objectPools[i].uploaded)
				{
					poolIndex = i;
					usingExternalCallback = parentVTClient->objectPools[i].useDataCallback;
					break;
				}
			}

			// If pool index is FFs, something is wrong with the state machine state, return false.
			if ((std::numeric_limits<std::uint32_t>::max() != poolIndex) &&
			    (bytesOffset + numberOfBytesNeeded) <= parentVTClient->objectPools[poolIndex].objectPoolSize + 1)
			{
				// We've got more data to transfer
				if ((0 != parentVTClient->objectPools[poolIndex].autoScaleDataMaskOriginalDimension) && (0 != parentVTClient->objectPools[poolIndex].autoScaleSoftKeyDesignatorOriginalHeight))
				{
					// Object pool has been pre-scaled. Use the scaling buffer instead
					retVal = true;
					if (0 == bytesOffset)
					{
						chunkBuffer[0] = static_cast<std::uint8_t>(Function::ObjectPoolTransferMessage);
						memcpy(&chunkBuffer[1], &parentVTClient->objectPools[poolIndex].scaledObjectPool[bytesOffset], numberOfBytesNeeded - 1);
					}
					else
					{
						// Subtract off 1 to account for the mux in the first byte of the message
						memcpy(chunkBuffer, &parentVTClient->objectPools[poolIndex].scaledObjectPool[bytesOffset - 1], numberOfBytesNeeded);
					}
				}
				else
				{
					if (usingExternalCallback)
					{
						// We're using the user's supplied callback to get a chunk of info
						if (0 == bytesOffset)
						{
							chunkBuffer[0] = static_cast<std::uint8_t>(Function::ObjectPoolTransferMessage);
							retVal = parentVTClient->objectPools[poolIndex].dataCallback(callbackIndex, bytesOffset, numberOfBytesNeeded - 1, &chunkBuffer[1], parentVTClient);
						}
						else
						{
							// Subtract off 1 to account for the mux in the first byte of the message
							retVal = parentVTClient->objectPools[poolIndex].dataCallback(callbackIndex, bytesOffset - 1, numberOfBytesNeeded, chunkBuffer, parentVTClient);
						}
					}
					else
					{
						// We already have the whole pool in RAM
						retVal = true;
						if (0 == bytesOffset)
						{
							chunkBuffer[0] = static_cast<std::uint8_t>(Function::ObjectPoolTransferMessage);
							memcpy(&chunkBuffer[1], &parentVTClient->objectPools[poolIndex].objectPoolDataPointer[bytesOffset], numberOfBytesNeeded - 1);
						}
						else
						{
							// Subtract off 1 to account for the mux in the first byte of the message
							memcpy(chunkBuffer, &parentVTClient->objectPools[poolIndex].objectPoolDataPointer[bytesOffset - 1], numberOfBytesNeeded);
						}
					}
				}
			}
		}
		return retVal;
	}

	bool VirtualTerminalClient::get_any_pool_needs_scaling() const
	{
		bool retVal = false;

		for (auto &objectPool : objectPools)
		{
			if ((0 != objectPool.autoScaleDataMaskOriginalDimension) &&
			    (0 != objectPool.autoScaleSoftKeyDesignatorOriginalHeight))
			{
				retVal = true;
				break;
			}
		}
		return retVal;
	}

	bool VirtualTerminalClient::scale_object_pools()
	{
		bool retVal = true;

		for (auto &objectPool : objectPools)
		{
			// Step 1: Make a read/write copy of the pool
			if (nullptr != objectPool.objectPoolDataPointer)
			{
				objectPool.scaledObjectPool.resize(objectPool.objectPoolSize);
				memcpy(&objectPool.scaledObjectPool[0], objectPool.objectPoolDataPointer, objectPool.objectPoolSize);
			}
			else if (nullptr != objectPool.objectPoolVectorPointer)
			{
				objectPool.scaledObjectPool.resize(objectPool.objectPoolVectorPointer->size());
				std::copy(objectPool.objectPoolVectorPointer->begin(), objectPool.objectPoolVectorPointer->end(), objectPool.scaledObjectPool.begin());
			}
			else if (objectPool.useDataCallback)
			{
				objectPool.scaledObjectPool.resize(objectPool.objectPoolSize);

				for (std::uint32_t i = 0; i < objectPool.objectPoolSize; i++)
				{
					retVal &= objectPool.dataCallback(i, i, 1, &objectPool.scaledObjectPool[i], this);
				}

				if (!retVal)
				{
					break;
				}
			}

			// Step 2, Parse the pool and resize each object as we iterate through it
			auto poolIterator = objectPool.scaledObjectPool.begin();

			while ((poolIterator != objectPool.scaledObjectPool.end()) &&
			       retVal)
			{
				if (VirtualTerminalObjectType::Key == static_cast<VirtualTerminalObjectType>(poolIterator[2]))
				{
					retVal &= resize_object(&poolIterator[0],
					                        static_cast<float>(get_softkey_x_axis_pixels()) / static_cast<float>(objectPool.autoScaleSoftKeyDesignatorOriginalHeight),
					                        static_cast<VirtualTerminalObjectType>(poolIterator[2]));
				}
				else
				{
					retVal &= resize_object(&poolIterator[0],
					                        static_cast<float>(get_number_x_pixels()) / static_cast<float>(objectPool.autoScaleDataMaskOriginalDimension),
					                        static_cast<VirtualTerminalObjectType>(poolIterator[2]));
				}

				std::uint32_t objectSize = get_number_bytes_in_object(&poolIterator[0]);
				if (retVal)
				{
					if (get_is_object_scalable(static_cast<VirtualTerminalObjectType>(*(poolIterator + 2))))
					{
						CANStackLogger::debug("[VT]: Resized an object: " +
						                      isobus::to_string(static_cast<int>((*poolIterator)) | (static_cast<int>((*poolIterator + 1))) << 8) +
						                      " with type " +
						                      isobus::to_string(static_cast<int>((*(poolIterator + 2)))) +
						                      " with size " +
						                      isobus::to_string(static_cast<int>(objectSize)));
					}
				}
				else
				{
					CANStackLogger::error("[VT]: Failed to resize an object: " +
					                      isobus::to_string(static_cast<int>((*poolIterator)) | (static_cast<int>((*poolIterator + 1))) << 8) +
					                      " with type " +
					                      isobus::to_string(static_cast<int>((*poolIterator + 2))) +
					                      " with size " +
					                      isobus::to_string(static_cast<int>(objectSize)));
				}
				poolIterator += objectSize;
			}
		}
		return retVal;
	}

	bool VirtualTerminalClient::get_is_object_scalable(VirtualTerminalObjectType type)
	{
		bool retVal = false;

		switch (type)
		{
			case VirtualTerminalObjectType::WorkingSet:
			{
				retVal = false;
			}
			break;

			case VirtualTerminalObjectType::DataMask:
			case VirtualTerminalObjectType::AlarmMask:
			case VirtualTerminalObjectType::Container:
			case VirtualTerminalObjectType::Key:
			case VirtualTerminalObjectType::Button:
			case VirtualTerminalObjectType::InputBoolean:
			case VirtualTerminalObjectType::InputString:
			case VirtualTerminalObjectType::InputNumber:
			case VirtualTerminalObjectType::InputList:
			case VirtualTerminalObjectType::OutputString:
			case VirtualTerminalObjectType::OutputNumber:
			case VirtualTerminalObjectType::OutputList:
			case VirtualTerminalObjectType::OutputLine:
			case VirtualTerminalObjectType::OutputRectangle:
			case VirtualTerminalObjectType::OutputEllipse:
			case VirtualTerminalObjectType::OutputPolygon:
			case VirtualTerminalObjectType::OutputMeter:
			case VirtualTerminalObjectType::OutputLinearBarGraph:
			case VirtualTerminalObjectType::OutputArchedBarGraph:
			case VirtualTerminalObjectType::PictureGraphic:
			case VirtualTerminalObjectType::AuxiliaryFunctionType1:
			case VirtualTerminalObjectType::AuxiliaryInputType1:
			case VirtualTerminalObjectType::AuxiliaryFunctionType2:
			case VirtualTerminalObjectType::AuxiliaryInputType2:
			case VirtualTerminalObjectType::FontAttributes:
			{
				retVal = true;
			}
			break;

			default:
			{
				retVal = false;
			}
			break;
		}
		return retVal;
	}

	VirtualTerminalClient::FontSize VirtualTerminalClient::get_font_or_next_smallest_font(FontSize originalFont) const
	{
		FontSize retVal = originalFont;

		while ((!get_font_size_supported(retVal)) && (FontSize::Size6x8 != retVal))
		{
			retVal = static_cast<FontSize>(static_cast<std::uint8_t>(originalFont) - 1);
		}
		return retVal;
	}

	VirtualTerminalClient::FontSize VirtualTerminalClient::remap_font_to_scale(FontSize originalFont, float scaleFactor)
	{
		static constexpr float SCALE_FACTOR_POSITIVE_FUDGE = 1.05f;
		static constexpr float SCALE_FACTOR_NEGATIVE_FUDGE = 0.95f;
		FontSize retVal = originalFont;

		if (scaleFactor > SCALE_FACTOR_POSITIVE_FUDGE || scaleFactor < SCALE_FACTOR_NEGATIVE_FUDGE)
		{
			const std::unordered_map<FontSize, std::map<float, FontSize, std::greater<float>>> FONT_SCALING_MAPPER{
				{ FontSize::Size6x8,
				  { { 23.95f, FontSize::Size128x192 },
				    { 21.30f, FontSize::Size128x128 },
				    { 15.95f, FontSize::Size96x128 },
				    { 11.95f, FontSize::Size64x96 },
				    { 10.60f, FontSize::Size64x64 },
				    { 7.95f, FontSize::Size48x64 },
				    { 5.95f, FontSize::Size32x48 },
				    { 5.30f, FontSize::Size32x32 },
				    { 3.95f, FontSize::Size24x32 },
				    { 2.95f, FontSize::Size16x24 },
				    { 2.60f, FontSize::Size16x16 },
				    { 1.95f, FontSize::Size12x16 },
				    { 1.45f, FontSize::Size8x12 },
				    { 1.30f, FontSize::Size8x8 } } },
				{ FontSize::Size8x8,
				  { { 23.95f, FontSize::Size128x192 },
				    { 15.95f, FontSize::Size128x128 },
				    { 11.95f, FontSize::Size64x96 },
				    { 7.95f, FontSize::Size64x64 },
				    { 5.95f, FontSize::Size32x48 },
				    { 3.95f, FontSize::Size32x32 },
				    { 2.95f, FontSize::Size16x24 },
				    { 1.95f, FontSize::Size16x16 },
				    { 1.45f, FontSize::Size8x12 },
				    { 0.0f, FontSize::Size6x8 } } },
				{ FontSize::Size8x12,
				  { { 15.95f, FontSize::Size128x192 },
				    { 11.95f, FontSize::Size96x128 },
				    { 7.95f, FontSize::Size64x96 },
				    { 5.95f, FontSize::Size48x64 },
				    { 3.95f, FontSize::Size32x48 },
				    { 2.95f, FontSize::Size24x32 },
				    { 1.95f, FontSize::Size16x24 },
				    { 1.45f, FontSize::Size12x16 },
				    { 0.0f, FontSize::Size6x8 } } },
				{ FontSize::Size12x16,
				  { { 11.95f, FontSize::Size128x192 },
				    { 10.60f, FontSize::Size128x128 },
				    { 7.95f, FontSize::Size96x128 },
				    { 5.95f, FontSize::Size64x96 },
				    { 5.30f, FontSize::Size64x64 },
				    { 3.95f, FontSize::Size48x64 },
				    { 2.95f, FontSize::Size32x48 },
				    { 2.60f, FontSize::Size32x32 },
				    { 1.95f, FontSize::Size24x32 },
				    { 1.45f, FontSize::Size16x24 },
				    { 1.30f, FontSize::Size16x16 },
				    { 0.75f, FontSize::Size8x12 },
				    { 0.67f, FontSize::Size8x8 },
				    { 0.0f, FontSize::Size6x8 } } },
				{ FontSize::Size16x16,
				  { { 11.95f, FontSize::Size128x192 },
				    { 7.95f, FontSize::Size128x128 },
				    { 5.95f, FontSize::Size64x96 },
				    { 3.95f, FontSize::Size64x64 },
				    { 2.95f, FontSize::Size32x48 },
				    { 1.95f, FontSize::Size32x32 },
				    { 1.45f, FontSize::Size16x24 },
				    { 0.75f, FontSize::Size8x12 },
				    { 0.50f, FontSize::Size8x8 },
				    { 0.0f, FontSize::Size6x8 } } },
				{ FontSize::Size16x24,
				  { { 7.95f, FontSize::Size128x128 },
				    { 5.95f, FontSize::Size96x128 },
				    { 3.95f, FontSize::Size64x96 },
				    { 2.95f, FontSize::Size48x64 },
				    { 1.95f, FontSize::Size32x48 },
				    { 1.45f, FontSize::Size24x32 },
				    { 0.75f, FontSize::Size12x16 },
				    { 0.50f, FontSize::Size8x12 },
				    { 0.0f, FontSize::Size6x8 } } },
				{ FontSize::Size24x32,
				  { { 5.95f, FontSize::Size128x192 },
				    { 5.30f, FontSize::Size128x128 },
				    { 3.95f, FontSize::Size96x128 },
				    { 2.95f, FontSize::Size64x96 },
				    { 2.60f, FontSize::Size64x64 },
				    { 1.95f, FontSize::Size48x64 },
				    { 1.45f, FontSize::Size32x48 },
				    { 1.30f, FontSize::Size32x32 },
				    { 0.75f, FontSize::Size16x24 },
				    { 0.60f, FontSize::Size16x16 },
				    { 0.50f, FontSize::Size12x16 },
				    { 0.37f, FontSize::Size8x12 },
				    { 0.30f, FontSize::Size8x8 },
				    { 0.0f, FontSize::Size6x8 } } },
				{ FontSize::Size32x32,
				  { { 5.95f, FontSize::Size128x192 },
				    { 3.95f, FontSize::Size128x128 },
				    { 2.95f, FontSize::Size64x96 },
				    { 1.95f, FontSize::Size64x64 },
				    { 1.45f, FontSize::Size32x48 },
				    { 0.75f, FontSize::Size16x24 },
				    { 0.50f, FontSize::Size16x16 },
				    { 0.37f, FontSize::Size8x12 },
				    { 0.20f, FontSize::Size8x8 },
				    { 0.0f, FontSize::Size6x8 } } },
				{ FontSize::Size32x48,
				  { { 3.95f, FontSize::Size128x192 },
				    { 2.95f, FontSize::Size96x128 },
				    { 1.95f, FontSize::Size64x96 },
				    { 1.45f, FontSize::Size48x64 },
				    { 0.75f, FontSize::Size24x32 },
				    { 0.50f, FontSize::Size16x24 },
				    { 0.37f, FontSize::Size12x16 },
				    { 0.20f, FontSize::Size8x12 },
				    { 0.0f, FontSize::Size6x8 } } },
				{ FontSize::Size48x64,
				  { { 2.95f, FontSize::Size128x192 },
				    { 2.60f, FontSize::Size128x128 },
				    { 1.95f, FontSize::Size96x128 },
				    { 1.45f, FontSize::Size64x96 },
				    { 1.30f, FontSize::Size64x64 },
				    { 0.75f, FontSize::Size32x48 },
				    { 0.60f, FontSize::Size32x32 },
				    { 0.50f, FontSize::Size24x32 },
				    { 0.37f, FontSize::Size16x24 },
				    { 0.33f, FontSize::Size16x16 },
				    { 0.25f, FontSize::Size12x16 },
				    { 0.18f, FontSize::Size8x12 },
				    { 0.16f, FontSize::Size8x8 },
				    { 0.0f, FontSize::Size6x8 } } },
				{ FontSize::Size64x64,
				  { { 2.95f, FontSize::Size128x192 },
				    { 1.95f, FontSize::Size128x128 },
				    { 1.45f, FontSize::Size64x96 },
				    { 0.75f, FontSize::Size32x48 },
				    { 0.50f, FontSize::Size32x32 },
				    { 0.37f, FontSize::Size16x24 },
				    { 0.25f, FontSize::Size16x16 },
				    { 0.18f, FontSize::Size8x12 },
				    { 0.12f, FontSize::Size8x8 },
				    { 0.0f, FontSize::Size6x8 } } },
				{ FontSize::Size64x96,
				  { { 1.95f, FontSize::Size128x192 },
				    { 1.45f, FontSize::Size96x128 },
				    { 0.75f, FontSize::Size48x64 },
				    { 0.50f, FontSize::Size32x48 },
				    { 0.37f, FontSize::Size24x32 },
				    { 0.25f, FontSize::Size16x24 },
				    { 0.18f, FontSize::Size12x16 },
				    { 0.12f, FontSize::Size8x12 },
				    { 0.0f, FontSize::Size6x8 } } },
				{ FontSize::Size96x128,
				  { { 1.45f, FontSize::Size128x192 },
				    { 1.30f, FontSize::Size128x128 },
				    { 0.75f, FontSize::Size64x96 },
				    { 0.60f, FontSize::Size64x64 },
				    { 0.50f, FontSize::Size48x64 },
				    { 0.37f, FontSize::Size32x48 },
				    { 0.33f, FontSize::Size32x32 },
				    { 0.25f, FontSize::Size24x32 },
				    { 0.18f, FontSize::Size16x24 },
				    { 0.16f, FontSize::Size16x16 },
				    { 0.125f, FontSize::Size12x16 },
				    { 0.09f, FontSize::Size8x12 },
				    { 0.08f, FontSize::Size8x8 },
				    { 0.0f, FontSize::Size6x8 } } },
				{ FontSize::Size128x128,
				  { { 1.45f, FontSize::Size128x192 },
				    { 0.75f, FontSize::Size64x96 },
				    { 0.50f, FontSize::Size64x64 },
				    { 0.37f, FontSize::Size32x48 },
				    { 0.25f, FontSize::Size32x32 },
				    { 0.18f, FontSize::Size16x24 },
				    { 0.125f, FontSize::Size16x16 },
				    { 0.09f, FontSize::Size8x12 },
				    { 0.06f, FontSize::Size8x8 },
				    { 0.0f, FontSize::Size6x8 } } },
				{ FontSize::Size128x192,
				  { { 0.75f, FontSize::Size96x128 },
				    { 0.50f, FontSize::Size64x96 },
				    { 0.37f, FontSize::Size48x64 },
				    { 0.25f, FontSize::Size32x48 },
				    { 0.18f, FontSize::Size24x32 },
				    { 0.125f, FontSize::Size16x24 },
				    { 0.09f, FontSize::Size12x16 },
				    { 0.06f, FontSize::Size8x12 },
				    { 0.0f, FontSize::Size6x8 } } }
			};

			auto iterator = FONT_SCALING_MAPPER.find(originalFont);
			if (iterator != FONT_SCALING_MAPPER.end())
			{
				for (auto &pair : iterator->second)
				{
					if (scaleFactor >= pair.first)
					{
						retVal = pair.second;
						break;
					}
				}
			}

			if (retVal == originalFont)
			{
				// Unknown font? Newer version than we support of the ISO standard? Or scaling factor out of range?
				CANStackLogger::error("[VT]: Unable to scale font type " + isobus::to_string(static_cast<int>(originalFont)) +
				                      " with scale factor " + isobus::to_string(scaleFactor) + ". Returning original font.");
			}
		}
		return retVal;
	}

	std::uint32_t VirtualTerminalClient::get_minimum_object_length(VirtualTerminalObjectType type)
	{
		std::uint32_t retVal = 0;

		switch (type)
		{
			case VirtualTerminalObjectType::WorkingSet:
			{
				retVal = 10;
			}
			break;

			case VirtualTerminalObjectType::OutputList:
			case VirtualTerminalObjectType::ExternalReferenceNAME:
			case VirtualTerminalObjectType::ObjectLabelRefrenceList:
			{
				retVal = 12;
			}
			break;

			case VirtualTerminalObjectType::AlarmMask:
			case VirtualTerminalObjectType::Container:
			case VirtualTerminalObjectType::KeyGroup:
			{
				retVal = 10;
			}
			break;

			case VirtualTerminalObjectType::ExternalObjectPointer:
			{
				retVal = 9;
			}
			break;

			case VirtualTerminalObjectType::SoftKeyMask:
			case VirtualTerminalObjectType::ColourMap:
			{
				retVal = 6;
			}
			break;

			case VirtualTerminalObjectType::Key:
			case VirtualTerminalObjectType::NumberVariable:
			case VirtualTerminalObjectType::InputAttributes:
			{
				retVal = 7;
			}
			break;

			case VirtualTerminalObjectType::Button:
			case VirtualTerminalObjectType::InputBoolean:
			case VirtualTerminalObjectType::OutputRectangle:
			case VirtualTerminalObjectType::InputList:
			case VirtualTerminalObjectType::ExternalObjectDefinition:
			{
				retVal = 13;
			}
			break;

			case VirtualTerminalObjectType::InputString:
			{
				retVal = 19;
			}
			break;

			case VirtualTerminalObjectType::InputNumber:
			{
				retVal = 38;
			}
			break;

			case VirtualTerminalObjectType::OutputString:
			{
				retVal = 17;
			}
			break;

			case VirtualTerminalObjectType::OutputNumber:
			{
				retVal = 29;
			}
			break;

			case VirtualTerminalObjectType::OutputLine:
			{
				retVal = 11;
			}
			break;

			case VirtualTerminalObjectType::OutputEllipse:
			{
				retVal = 15;
			}
			break;

			case VirtualTerminalObjectType::OutputPolygon:
			{
				retVal = 14;
			}
			break;

			case VirtualTerminalObjectType::OutputMeter:
			{
				retVal = 21;
			}
			break;

			case VirtualTerminalObjectType::OutputLinearBarGraph:
			{
				retVal = 24;
			}
			break;

			case VirtualTerminalObjectType::OutputArchedBarGraph:
			{
				retVal = 27;
			}
			break;

			case VirtualTerminalObjectType::PictureGraphic:
			case VirtualTerminalObjectType::Animation:
			case VirtualTerminalObjectType::WindowMask:
			{
				retVal = 17;
			}
			break;

			case VirtualTerminalObjectType::StringVariable:
			case VirtualTerminalObjectType::ExtendedInputAttributes:
			case VirtualTerminalObjectType::ObjectPointer:
			case VirtualTerminalObjectType::Macro:
			{
				retVal = 5;
			}
			break;

			case VirtualTerminalObjectType::FontAttributes:
			case VirtualTerminalObjectType::LineAttributes:
			case VirtualTerminalObjectType::FillAttributes:
			case VirtualTerminalObjectType::DataMask:
			{
				retVal = 8;
			}
			break;

			case VirtualTerminalObjectType::GraphicsContext:
			{
				retVal = 34;
			}
			break;

			default:
			{
				CANStackLogger::error("[VT]: Cannot autoscale object pool due to unknown object minimum length - type " + isobus::to_string(static_cast<int>(type)));
			}
			break;
		}
		return retVal;
	}

	std::uint32_t VirtualTerminalClient::get_number_bytes_in_object(std::uint8_t *buffer)
	{
		auto currentObjectType = static_cast<VirtualTerminalObjectType>(buffer[2]);
		std::uint32_t retVal = get_minimum_object_length(currentObjectType);

		switch (currentObjectType)
		{
			case VirtualTerminalObjectType::WorkingSet:
			{
				const std::uint32_t sizeOfChildObjects = (buffer[7] * 6);
				const std::uint32_t sizeOfMacros = (buffer[8] * 2);
				const std::uint32_t sizeOfLanguageCodes = (buffer[9] * 2);
				retVal += (sizeOfLanguageCodes + sizeOfChildObjects + sizeOfMacros);
			}
			break;

			case VirtualTerminalObjectType::DataMask:
			{
				const std::uint32_t sizeOfChildObjects = (buffer[6] * 6);
				const std::uint32_t sizeOfMacros = (buffer[7] * 2);
				retVal += (sizeOfChildObjects + sizeOfMacros);
			}
			break;

			case VirtualTerminalObjectType::AlarmMask:
			case VirtualTerminalObjectType::Container:
			{
				const std::uint32_t sizeOfChildObjects = (buffer[8] * 6);
				const std::uint32_t sizeOfMacros = (buffer[9] * 2);
				retVal += (sizeOfChildObjects + sizeOfMacros);
			}
			break;

			case VirtualTerminalObjectType::SoftKeyMask:
			{
				const std::uint32_t sizeOfChildObjects = (buffer[4] * 2);
				const std::uint32_t sizeOfMacros = (buffer[5] * 2);
				retVal += (sizeOfChildObjects + sizeOfMacros);
			}
			break;

			case VirtualTerminalObjectType::Key:
			{
				const std::uint32_t sizeOfChildObjects = (buffer[5] * 6);
				const std::uint32_t sizeOfMacros = (buffer[6] * 2);
				retVal += (sizeOfChildObjects + sizeOfMacros);
			}
			break;

			case VirtualTerminalObjectType::Button:
			{
				const std::uint32_t sizeOfChildObjects = (buffer[11] * 6);
				const std::uint32_t sizeOfMacros = (buffer[12] * 2);
				retVal += (sizeOfChildObjects + sizeOfMacros);
			}
			break;

			case VirtualTerminalObjectType::InputBoolean:
			{
				const std::uint32_t sizeOfMacros = (buffer[12] * 2);
				retVal += sizeOfMacros;
			}
			break;

			case VirtualTerminalObjectType::InputString:
			{
				const std::uint32_t sizeOfValue = buffer[16];
				const std::uint32_t sizeOfMacros = (buffer[18 + sizeOfValue] * 2);
				retVal += (sizeOfValue + sizeOfMacros);
			}
			break;

			case VirtualTerminalObjectType::InputNumber:
			{
				const std::uint32_t sizeOfMacros = (buffer[37] * 2);
				retVal += sizeOfMacros;
			}
			break;

			case VirtualTerminalObjectType::InputList:
			{
				const std::uint32_t sizeOfMacros = (buffer[12] * 2);
				const std::uint32_t sizeOfListObjectIDs = (buffer[10] * 2);
				retVal += (sizeOfMacros + sizeOfListObjectIDs);
			}
			break;

			case VirtualTerminalObjectType::OutputString:
			{
				const std::uint32_t sizeOfValue = (static_cast<uint16_t>(buffer[14]) | static_cast<uint16_t>(buffer[15] << 8));
				const std::uint32_t sizeOfMacros = (buffer[16 + sizeOfValue] * 2);
				retVal += (sizeOfMacros + sizeOfValue);
			}
			break;

			case VirtualTerminalObjectType::OutputNumber:
			{
				const std::uint32_t sizeOfMacros = (buffer[28] * 2);
				retVal += sizeOfMacros;
			}
			break;

			case VirtualTerminalObjectType::OutputList:
			{
				const std::uint32_t sizeOfMacros = (buffer[11] * 2);
				const std::uint32_t sizeOfListObjectIDs = (buffer[10] * 2);
				retVal += (sizeOfMacros + sizeOfListObjectIDs);
			}
			break;

			case VirtualTerminalObjectType::OutputLine:
			{
				const std::uint32_t sizeOfMacros = (buffer[10] * 2);
				retVal += sizeOfMacros;
			}
			break;

			case VirtualTerminalObjectType::OutputRectangle:
			{
				const std::uint32_t sizeOfMacros = (buffer[12] * 2);
				retVal += sizeOfMacros;
			}
			break;

			case VirtualTerminalObjectType::OutputEllipse:
			{
				const std::uint32_t sizeOfMacros = (buffer[14] * 2);
				retVal += sizeOfMacros;
			}
			break;

			case VirtualTerminalObjectType::OutputPolygon:
			{
				const std::uint32_t sizeOfPoints = (buffer[12] * 4);
				const std::uint32_t sizeOfMacros = (buffer[13] * 2);
				retVal += (sizeOfMacros + sizeOfPoints);
			}
			break;

			case VirtualTerminalObjectType::OutputMeter:
			{
				const std::uint32_t sizeOfMacros = (buffer[20] * 2);
				retVal += sizeOfMacros;
			}
			break;

			case VirtualTerminalObjectType::OutputLinearBarGraph:
			{
				const std::uint32_t sizeOfMacros = (buffer[23] * 2);
				retVal += sizeOfMacros;
			}
			break;

			case VirtualTerminalObjectType::OutputArchedBarGraph:
			{
				const std::uint32_t sizeOfMacros = (buffer[26] * 2);
				retVal += sizeOfMacros;
			}
			break;

			case VirtualTerminalObjectType::PictureGraphic:
			{
				const std::uint32_t sizeOfMacros = (buffer[16] * 2);
				const std::uint32_t sizeOfRawData = (static_cast<std::uint32_t>(buffer[12]) |
				                                     (static_cast<std::uint32_t>(buffer[13]) << 8) |
				                                     (static_cast<std::uint32_t>(buffer[14]) << 16) |
				                                     (static_cast<std::uint32_t>(buffer[15]) << 24));
				retVal += (sizeOfRawData + sizeOfMacros);
			}
			break;

			case VirtualTerminalObjectType::ObjectPointer:
			case VirtualTerminalObjectType::NumberVariable:
			case VirtualTerminalObjectType::GraphicsContext:
			case VirtualTerminalObjectType::ExternalReferenceNAME:
			case VirtualTerminalObjectType::ExternalObjectPointer:
			case VirtualTerminalObjectType::AuxiliaryControlDesignatorType2:
			{
				// No additional length
			}
			break;

			case VirtualTerminalObjectType::StringVariable:
			{
				const std::uint32_t sizeOfValue = (static_cast<uint16_t>(buffer[3]) | static_cast<uint16_t>(buffer[4]) << 8);
				retVal += sizeOfValue;
			}
			break;

			case VirtualTerminalObjectType::FontAttributes:
			case VirtualTerminalObjectType::LineAttributes:
			case VirtualTerminalObjectType::FillAttributes:
			{
				const std::uint32_t sizeOfMacros = (buffer[7] * 2);
				retVal += sizeOfMacros;
			}
			break;

			case VirtualTerminalObjectType::InputAttributes:
			{
				const std::uint32_t sizeOfValidationString = buffer[4];
				const std::uint32_t sizeOfMacros = (buffer[5 + sizeOfValidationString] * 2);
				retVal += (sizeOfMacros + sizeOfValidationString);
			}
			break;

			case VirtualTerminalObjectType::ExtendedInputAttributes:
			{
				const std::uint32_t numberOfCodePlanes = buffer[5];
				retVal += (numberOfCodePlanes * 2); // Doesn't include the character ranges, need to handle those externally
			}
			break;

			case VirtualTerminalObjectType::Macro:
			{
				const std::uint32_t numberOfMacroBytes = (static_cast<std::uint16_t>(buffer[3]) | (static_cast<std::uint16_t>(buffer[4]) << 8));
				retVal += numberOfMacroBytes;
			}
			break;

			case VirtualTerminalObjectType::ColourMap:
			{
				const std::uint32_t numberIndexes = (static_cast<std::uint16_t>(buffer[3]) | (static_cast<std::uint16_t>(buffer[4]) << 8));
				retVal += numberIndexes;
			}
			break;

			case VirtualTerminalObjectType::WindowMask:
			{
				const std::uint32_t sizeOfReferences = (buffer[14] * 2);
				const std::uint32_t numberObjects = (buffer[15] * 6);
				const std::uint32_t sizeOfMacros = (buffer[16] * 2);
				retVal += (sizeOfMacros + numberObjects + sizeOfReferences);
			}
			break;

			case VirtualTerminalObjectType::KeyGroup:
			{
				const std::uint32_t numberObjects = (buffer[8] * 2);
				const std::uint32_t sizeOfMacros = (buffer[9] * 2);
				retVal += (sizeOfMacros + numberObjects);
			}
			break;

			case VirtualTerminalObjectType::ObjectLabelRefrenceList:
			{
				const std::uint32_t sizeOfLabeledObjects = ((static_cast<uint16_t>(buffer[4]) | static_cast<uint16_t>(buffer[5]) << 8) * 7);
				retVal += sizeOfLabeledObjects;
			}
			break;

			case VirtualTerminalObjectType::ExternalObjectDefinition:
			{
				const std::uint32_t sizeOfObjects = (buffer[12] * 2);
				retVal += sizeOfObjects;
			}
			break;

			case VirtualTerminalObjectType::Animation:
			{
				const std::uint32_t sizeOfObjects = (buffer[15] * 6);
				const std::uint32_t sizeOfMacros = (buffer[16] * 2);
				retVal += (sizeOfMacros + sizeOfObjects);
			}
			break;

			case VirtualTerminalObjectType::AuxiliaryFunctionType1:
			case VirtualTerminalObjectType::AuxiliaryFunctionType2:
			{
				const std::uint32_t sizeOfObjects = (buffer[5] * 6);
				retVal += sizeOfObjects;
			}
			break;

			case VirtualTerminalObjectType::AuxiliaryInputType1:
			case VirtualTerminalObjectType::AuxiliaryInputType2:
			{
				const std::uint32_t sizeOfObjects = (buffer[6] * 6);
				retVal += sizeOfObjects;
			}
			break;

			default:
			{
				CANStackLogger::error("[VT]: Cannot autoscale object pool due to unknown object total length - type " + isobus::to_string(static_cast<int>(buffer[2])));
			}
			break;
		}
		return retVal;
	}

	void VirtualTerminalClient::process_standard_object_height_and_width(std::uint8_t *buffer, float scaleFactor)
	{
		auto width = static_cast<std::uint16_t>(((static_cast<std::uint16_t>(buffer[3]) | (static_cast<std::uint16_t>(buffer[4]) << 8))) * scaleFactor);
		auto height = static_cast<std::uint16_t>(((static_cast<std::uint16_t>(buffer[5]) | (static_cast<std::uint16_t>(buffer[6]) << 8))) * scaleFactor);
		buffer[3] = (width & 0xFF);
		buffer[4] = (width >> 8);
		buffer[5] = (height & 0xFF);
		buffer[6] = (height >> 8);
	}

	bool VirtualTerminalClient::resize_object(std::uint8_t *buffer, float scaleFactor, VirtualTerminalObjectType type)
	{
		bool retVal = false;

		if (get_is_object_scalable(type))
		{
			switch (type)
			{
				case VirtualTerminalObjectType::DataMask:
				{
					const std::uint8_t childrenToFollow = buffer[6];

					for (std::uint_fast8_t i = 0; i < childrenToFollow; i++)
					{
						auto childX = static_cast<std::int16_t>(((static_cast<std::int16_t>(buffer[10 + (6 * i)]) | (static_cast<std::int16_t>(buffer[11 + (6 * i)]) << 8))) * scaleFactor);
						auto childY = static_cast<std::int16_t>(((static_cast<std::int16_t>(buffer[12 + (6 * i)]) | (static_cast<std::int16_t>(buffer[13 + (6 * i)]) << 8))) * scaleFactor);
						buffer[10 + (6 * i)] = (childX & 0xFF);
						buffer[11 + (6 * i)] = (childX >> 8);
						buffer[12 + (6 * i)] = (childY & 0xFF);
						buffer[13 + (6 * i)] = (childY >> 8);
					}
					retVal = true;
				}
				break;

				case VirtualTerminalObjectType::AlarmMask:
				{
					const std::uint8_t childrenToFollow = buffer[8];

					for (std::uint_fast8_t i = 0; i < childrenToFollow; i++)
					{
						auto childX = static_cast<std::int16_t>(((static_cast<std::int16_t>(buffer[12 + (6 * i)]) | (static_cast<std::int16_t>(buffer[13 + (6 * i)]) << 8))) * scaleFactor);
						auto childY = static_cast<std::int16_t>(((static_cast<std::int16_t>(buffer[14 + (6 * i)]) | (static_cast<std::int16_t>(buffer[15 + (6 * i)]) << 8))) * scaleFactor);
						buffer[12 + (6 * i)] = (childX & 0xFF);
						buffer[13 + (6 * i)] = (childX >> 8);
						buffer[14 + (6 * i)] = (childY & 0xFF);
						buffer[15 + (6 * i)] = (childY >> 8);
					}
					retVal = true;
				}
				break;

				case VirtualTerminalObjectType::Container:
				{
					std::uint8_t childrenToFollow = buffer[8];

					// Modify the object in memory
					process_standard_object_height_and_width(buffer, scaleFactor);

					// Iterate over the list of children and move them proportionally to the new size
					// The container is 10 bytes, followed by children with 2 bytes of ID, 2 of X, and 2 of Y
					for (std::uint_fast8_t i = 0; i < childrenToFollow; i++)
					{
						auto childX = static_cast<std::int16_t>(((static_cast<std::int16_t>(buffer[12 + (6 * i)]) | (static_cast<std::int16_t>(buffer[13 + (6 * i)]) << 8))) * scaleFactor);
						auto childY = static_cast<std::int16_t>(((static_cast<std::int16_t>(buffer[14 + (6 * i)]) | (static_cast<std::int16_t>(buffer[15 + (6 * i)]) << 8))) * scaleFactor);
						buffer[12 + (6 * i)] = (childX & 0xFF);
						buffer[13 + (6 * i)] = (childX >> 8);
						buffer[14 + (6 * i)] = (childY & 0xFF);
						buffer[15 + (6 * i)] = (childY >> 8);
					}
					retVal = true;
				}
				break;

				case VirtualTerminalObjectType::Button:
				{
					std::uint8_t childrenToFollow = buffer[11];

					// Modify the object in memory
					process_standard_object_height_and_width(buffer, scaleFactor);

					// Iterate over the list of children and move them proportionally to the new size
					for (std::uint_fast8_t i = 0; i < childrenToFollow; i++)
					{
						auto childWidth = static_cast<std::uint16_t>(((static_cast<std::uint16_t>(buffer[15 + (6 * i)]) | (static_cast<std::uint16_t>(buffer[16 + (6 * i)]) << 8))) * scaleFactor);
						auto childHeight = static_cast<std::uint16_t>(((static_cast<std::uint16_t>(buffer[17 + (6 * i)]) | (static_cast<std::uint16_t>(buffer[18 + (6 * i)]) << 8))) * scaleFactor);
						buffer[15 + (6 * i)] = (childWidth & 0xFF);
						buffer[16 + (6 * i)] = (childWidth >> 8);
						buffer[17 + (6 * i)] = (childHeight & 0xFF);
						buffer[18 + (6 * i)] = (childHeight >> 8);
					}
					retVal = true;
				}
				break;

				case VirtualTerminalObjectType::InputBoolean:
				{
					auto width = static_cast<std::uint16_t>((static_cast<std::uint16_t>(buffer[4]) | (static_cast<std::uint16_t>(buffer[5]) << 8)));

					// Modify the object in memory
					buffer[4] = (width & 0xFF);
					buffer[5] = (width >> 8);
					retVal = true;
				}
				break;

				case VirtualTerminalObjectType::InputString:
				case VirtualTerminalObjectType::InputNumber:
				case VirtualTerminalObjectType::InputList:
				case VirtualTerminalObjectType::OutputString:
				case VirtualTerminalObjectType::OutputNumber:
				case VirtualTerminalObjectType::OutputList:
				case VirtualTerminalObjectType::OutputLinearBarGraph:
				{
					// Modify the object in memory
					process_standard_object_height_and_width(buffer, scaleFactor);
					retVal = true;
				}
				break;

				case VirtualTerminalObjectType::OutputLine:
				case VirtualTerminalObjectType::OutputRectangle:
				case VirtualTerminalObjectType::OutputEllipse:
				{
					// Modify the object in memory
					auto width = static_cast<std::uint16_t>(((static_cast<std::uint16_t>(buffer[5]) | (static_cast<std::uint16_t>(buffer[6]) << 8))) * scaleFactor);
					auto height = static_cast<std::uint16_t>(((static_cast<std::uint16_t>(buffer[7]) | (static_cast<std::uint16_t>(buffer[8]) << 8))) * scaleFactor);
					buffer[5] = (width & 0xFF);
					buffer[6] = (width >> 8);
					buffer[7] = (height & 0xFF);
					buffer[8] = (height >> 8);
					retVal = true;
				}
				break;

				case VirtualTerminalObjectType::OutputPolygon:
				{
					const std::uint8_t numberOfPoints = buffer[12];

					// Modify the object in memory
					process_standard_object_height_and_width(buffer, scaleFactor);

					// Reposition the child points
					for (std::uint_fast8_t i = 0; i < numberOfPoints; i++)
					{
						auto xPosition = static_cast<std::uint16_t>(((static_cast<std::uint16_t>(buffer[14 + (4 * i)]) | (static_cast<std::uint16_t>(buffer[15 + (4 * i)]) << 8))) * scaleFactor);
						auto yPosition = static_cast<std::uint16_t>(((static_cast<std::uint16_t>(buffer[16 + (4 * i)]) | (static_cast<std::uint16_t>(buffer[17 + (4 * i)]) << 8))) * scaleFactor);
						buffer[14 + (4 * i)] = (xPosition & 0xFF);
						buffer[15 + (4 * i)] = (xPosition >> 8);
						buffer[16 + (4 * i)] = (yPosition & 0xFF);
						buffer[17 + (4 * i)] = (yPosition >> 8);
					}
					retVal = true;
				}
				break;

				case VirtualTerminalObjectType::OutputMeter:
				case VirtualTerminalObjectType::PictureGraphic:
				{
					// Modify the object in memory
					auto width = static_cast<std::uint16_t>(((static_cast<std::uint16_t>(buffer[3]) | (static_cast<std::uint16_t>(buffer[4]) << 8))) * scaleFactor);
					buffer[3] = (width & 0xFF);
					buffer[4] = (width >> 8);
					retVal = true;
				}
				break;

				case VirtualTerminalObjectType::OutputArchedBarGraph:
				{
					// Modify the object in memory
					process_standard_object_height_and_width(buffer, scaleFactor);

					auto width = static_cast<std::uint16_t>(((static_cast<std::uint16_t>(buffer[12]) | (static_cast<std::uint16_t>(buffer[13]) << 8))) * scaleFactor);
					buffer[12] = (width & 0xFF);
					buffer[13] = (width >> 8);
					retVal = true;
				}
				break;

				case VirtualTerminalObjectType::Animation:
				{
					std::uint8_t childrenToFollow = buffer[15];

					// Modify the object in memory
					process_standard_object_height_and_width(buffer, scaleFactor);

					// Iterate over the list of children and move them proportionally to the new size
					for (std::uint_fast8_t i = 0; i < childrenToFollow; i++)
					{
						auto childX = static_cast<std::int16_t>(((static_cast<std::int16_t>(buffer[20 + (6 * i)]) | (static_cast<std::int16_t>(buffer[21 + (6 * i)]) << 8))) * scaleFactor);
						auto childY = static_cast<std::int16_t>(((static_cast<std::int16_t>(buffer[22 + (6 * i)]) | (static_cast<std::int16_t>(buffer[23 + (6 * i)]) << 8))) * scaleFactor);
						buffer[20 + (6 * i)] = (childX & 0xFF);
						buffer[21 + (6 * i)] = (childX >> 8);
						buffer[22 + (6 * i)] = (childY & 0xFF);
						buffer[23 + (6 * i)] = (childY >> 8);
					}
					retVal = true;
				}
				break;

				case VirtualTerminalObjectType::Key:
				{
					const std::uint8_t childrenToFollow = buffer[5];

					for (std::uint_fast8_t i = 0; i < childrenToFollow; i++)
					{
						auto childX = static_cast<std::int16_t>(((static_cast<std::int16_t>(buffer[9 + (6 * i)]) | (static_cast<std::int16_t>(buffer[10 + (6 * i)]) << 8))) * scaleFactor);
						auto childY = static_cast<std::int16_t>(((static_cast<std::int16_t>(buffer[11 + (6 * i)]) | (static_cast<std::int16_t>(buffer[12 + (6 * i)]) << 8))) * scaleFactor);
						buffer[9 + (6 * i)] = (childX & 0xFF);
						buffer[10 + (6 * i)] = (childX >> 8);
						buffer[11 + (6 * i)] = (childY & 0xFF);
						buffer[12 + (6 * i)] = (childY >> 8);
					}
					retVal = true;
				}
				break;

				case VirtualTerminalObjectType::FontAttributes:
				{
					buffer[4] = static_cast<std::uint8_t>(get_font_or_next_smallest_font(remap_font_to_scale(static_cast<FontSize>(buffer[4]), scaleFactor)));
					retVal = true;
				}
				break;

				case VirtualTerminalObjectType::AuxiliaryFunctionType1:
				case VirtualTerminalObjectType::AuxiliaryFunctionType2:
				case VirtualTerminalObjectType::AuxiliaryInputType2:
				{
					std::uint8_t childrenToFollow = buffer[5];

					for (std::uint_fast8_t i = 0; i < childrenToFollow; i++)
					{
						auto childX = static_cast<std::int16_t>(((static_cast<std::int16_t>(buffer[8 + (6 * i)]) | (static_cast<std::int16_t>(buffer[9 + (6 * i)]) << 8))) * scaleFactor);
						auto childY = static_cast<std::int16_t>(((static_cast<std::int16_t>(buffer[10 + (6 * i)]) | (static_cast<std::int16_t>(buffer[11 + (6 * i)]) << 8))) * scaleFactor);
						buffer[8 + (6 * i)] = (childX & 0xFF);
						buffer[9 + (6 * i)] = (childX >> 8);
						buffer[10 + (6 * i)] = (childY & 0xFF);
						buffer[11 + (6 * i)] = (childY >> 8);
					}
					retVal = true;
				}
				break;

				case VirtualTerminalObjectType::AuxiliaryInputType1:
				{
					std::uint8_t childrenToFollow = buffer[6];

					for (std::uint_fast8_t i = 0; i < childrenToFollow; i++)
					{
						auto childX = static_cast<std::int16_t>(((static_cast<std::int16_t>(buffer[9 + (6 * i)]) | (static_cast<std::int16_t>(buffer[10 + (6 * i)]) << 8))) * scaleFactor);
						auto childY = static_cast<std::int16_t>(((static_cast<std::int16_t>(buffer[11 + (6 * i)]) | (static_cast<std::int16_t>(buffer[12 + (6 * i)]) << 8))) * scaleFactor);
						buffer[9 + (6 * i)] = (childX & 0xFF);
						buffer[10 + (6 * i)] = (childX >> 8);
						buffer[11 + (6 * i)] = (childY & 0xFF);
						buffer[12 + (6 * i)] = (childY >> 8);
					}
					retVal = true;
				}
				break;

				default:
				{
					CANStackLogger::debug("[VT]: Skipping resize of non-resizable object type " +
					                      isobus::to_string(static_cast<int>(type)));
					retVal = false;
				}
				break;
			}
		}
		else
		{
			CANStackLogger::debug("[VT]: Skipping resize of non-resizable object type " +
			                      isobus::to_string(static_cast<int>(type)));
			retVal = true;
		}
		return retVal;
	}

	void VirtualTerminalClient::worker_thread_function()
	{
		for (;;)
		{
			if (shouldTerminate)
			{
				break;
			}
			update();
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}

} // namespace isobus
