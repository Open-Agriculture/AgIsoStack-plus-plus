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
#include <cstring>

namespace isobus
{
	VirtualTerminalClient::VirtualTerminalClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource) :
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
	  supportedGraphicsMode(GraphicMode::TwoHundredFiftySixColor),
	  xPixels(0),
	  yPixels(0),
	  hardwareFeaturesBitfield(0),
	  state(StateMachineState::Disconnected),
	  currentObjectPoolState(CurrentObjectPoolUploadState::Uninitialized),
	  stateMachineTimestamp_ms(0),
	  lastWorkingSetMaintenanceTimestamp_ms(0),
	  workerThread(nullptr),
	  initialized(false),
	  sendWorkingSetMaintenenace(false),
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

	void VirtualTerminalClient::register_vt_soft_key_event_callback(VTKeyEventCallback value)
	{
		softKeyEventCallbacks.push_back(value);
	}

	void VirtualTerminalClient::remove_vt_soft_key_event_callback(VTKeyEventCallback value)
	{
		auto callbackLocation = std::find(softKeyEventCallbacks.begin(), softKeyEventCallbacks.end(), value);

		if (softKeyEventCallbacks.end() != callbackLocation)
		{
			softKeyEventCallbacks.erase(callbackLocation);
		}
	}

	void VirtualTerminalClient::register_vt_button_event_callback(VTKeyEventCallback value)
	{
		buttonEventCallbacks.push_back(value);
	}

	void VirtualTerminalClient::remove_vt_button_event_callback(VTKeyEventCallback value)
	{
		auto callbackLocation = std::find(buttonEventCallbacks.begin(), buttonEventCallbacks.end(), value);

		if (buttonEventCallbacks.end() != callbackLocation)
		{
			buttonEventCallbacks.erase(callbackLocation);
		}
	}

	void VirtualTerminalClient::register_vt_pointing_event_callback(VTPointingEventCallback value)
	{
		pointingEventCallbacks.push_back(value);
	}

	void VirtualTerminalClient::remove_vt_pointing_event_callback(VTPointingEventCallback value)
	{
		auto callbackLocation = std::find(pointingEventCallbacks.begin(), pointingEventCallbacks.end(), value);

		if (pointingEventCallbacks.end() != callbackLocation)
		{
			pointingEventCallbacks.erase(callbackLocation);
		}
	}

	void VirtualTerminalClient::register_vt_select_input_object_event_callback(VTSelectInputObjectCallback value)
	{
		selectInputObjectCallbacks.push_back(value);
	}

	void VirtualTerminalClient::remove_vt_selection_input_object_event_callback(VTSelectInputObjectCallback value)
	{
		auto callbackLocation = std::find(selectInputObjectCallbacks.begin(), selectInputObjectCallbacks.end(), value);

		if (selectInputObjectCallbacks.end() != callbackLocation)
		{
			selectInputObjectCallbacks.erase(callbackLocation);
		}
	}

	void VirtualTerminalClient::register_vt_esc_message_event_callback(VTESCMessageCallback value)
	{
		escMessageCallbacks.push_back(value);
	}

	void VirtualTerminalClient::remove_vt_esc_message_event_callback(VTESCMessageCallback value)
	{
		auto callbackLocation = std::find(escMessageCallbacks.begin(), escMessageCallbacks.end(), value);

		if (escMessageCallbacks.end() != callbackLocation)
		{
			escMessageCallbacks.erase(callbackLocation);
		}
	}

	void VirtualTerminalClient::register_vt_change_numeric_value_event_callback(VTChangeNumericValueCallback value)
	{
		changeNumericValueCallbacks.push_back(value);
	}

	void VirtualTerminalClient::remove_vt_change_numeric_value_event_callback(VTChangeNumericValueCallback value)
	{
		auto callbackLocation = std::find(changeNumericValueCallbacks.begin(), changeNumericValueCallbacks.end(), value);

		if (changeNumericValueCallbacks.end() != callbackLocation)
		{
			changeNumericValueCallbacks.erase(callbackLocation);
		}
	}

	void VirtualTerminalClient::register_vt_change_active_mask_event_callback(VTChangeActiveMaskCallback value)
	{
		changeActiveMaskCallbacks.push_back(value);
	}

	void VirtualTerminalClient::remove_vt_change_active_mask_event_callback(VTChangeActiveMaskCallback value)
	{
		auto callbackLocation = std::find(changeActiveMaskCallbacks.begin(), changeActiveMaskCallbacks.end(), value);

		if (changeActiveMaskCallbacks.end() != callbackLocation)
		{
			changeActiveMaskCallbacks.erase(callbackLocation);
		}
	}

	void VirtualTerminalClient::register_vt_change_soft_key_mask_event_callback(VTChangeSoftKeyMaskCallback value)
	{
		changeSoftKeyMaskCallbacks.push_back(value);
	}

	void VirtualTerminalClient::remove_vt_change_soft_key_mask_event_callback(VTChangeSoftKeyMaskCallback value)
	{
		auto callbackLocation = std::find(changeSoftKeyMaskCallbacks.begin(), changeSoftKeyMaskCallbacks.end(), value);

		if (changeSoftKeyMaskCallbacks.end() != callbackLocation)
		{
			changeSoftKeyMaskCallbacks.erase(callbackLocation);
		}
	}

	void VirtualTerminalClient::register_vt_change_string_value_event_callback(VTChangeStringValueCallback value)
	{
		changeStringValueCallbacks.push_back(value);
	}

	void VirtualTerminalClient::remove_vt_change_string_value_event_callback(VTChangeStringValueCallback value)
	{
		auto callbackLocation = std::find(changeStringValueCallbacks.begin(), changeStringValueCallbacks.end(), value);

		if (changeStringValueCallbacks.end() != callbackLocation)
		{
			changeStringValueCallbacks.erase(callbackLocation);
		}
	}

	void VirtualTerminalClient::register_vt_user_layout_hide_show_event_callback(VTUserLayoutHideShowCallback value)
	{
		userLayoutHideShowCallbacks.push_back(value);
	}

	void VirtualTerminalClient::remove_vt_user_layout_hide_show_callback(VTUserLayoutHideShowCallback value)
	{
		auto callbackLocation = std::find(userLayoutHideShowCallbacks.begin(), userLayoutHideShowCallbacks.end(), value);

		if (userLayoutHideShowCallbacks.end() != callbackLocation)
		{
			userLayoutHideShowCallbacks.erase(callbackLocation);
		}
	}

	void VirtualTerminalClient::register_vt_control_audio_signal_termination_event_callback(VTAudioSignalTerminationCallback value)
	{
		audioSignalTerminationCallbacks.push_back(value);
	}

	void VirtualTerminalClient::remove_vt_control_audio_signal_termination_event_callback(VTAudioSignalTerminationCallback value)
	{
		auto callbackLocation = std::find(audioSignalTerminationCallbacks.begin(), audioSignalTerminationCallbacks.end(), value);

		if (audioSignalTerminationCallbacks.end() != callbackLocation)
		{
			audioSignalTerminationCallbacks.erase(callbackLocation);
		}
	}

	void VirtualTerminalClient::register_auxiliary_input_event_callback(AuxiliaryInputCallback value)
	{
		auxiliaryInputCallbacks.push_back(value);
	}

	void VirtualTerminalClient::remove_auxiliary_input_event_callback(AuxiliaryInputCallback value)
	{
		auto callbackLocation = std::find(auxiliaryInputCallbacks.begin(), auxiliaryInputCallbacks.end(), value);

		if (auxiliaryInputCallbacks.end() != callbackLocation)
		{
			auxiliaryInputCallbacks.erase(callbackLocation);
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

	bool VirtualTerminalClient::send_change_background_colour(std::uint16_t objectID, std::uint8_t color)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangeBackgroundColourCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             color,
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

	bool VirtualTerminalClient::send_change_font_attributes(std::uint16_t objectID, std::uint8_t color, FontSize size, std::uint8_t type, std::uint8_t styleBitfield)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangeFontAttributesCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             color,
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

	bool VirtualTerminalClient::send_change_line_attributes(std::uint16_t objectID, std::uint8_t color, std::uint8_t width, std::uint16_t lineArtBitmask)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangeLineAttributesCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             color,
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

	bool VirtualTerminalClient::send_change_fill_attributes(std::uint16_t objectID, FillType fillType, std::uint8_t color, std::uint16_t fillPatternObjectID)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::ChangeFillAttributesCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(fillType),
			                                             color,
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

	bool VirtualTerminalClient::send_select_color_map_or_palette(std::uint16_t objectID)
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

	bool VirtualTerminalClient::send_set_foreground_colour(std::uint16_t objectID, std::uint8_t color)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::SetForegroundColor),
			                                             color,
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

	bool VirtualTerminalClient::send_set_background_colour(std::uint16_t objectID, std::uint8_t color)
	{
		const std::uint8_t buffer[CAN_DATA_LENGTH] = { static_cast<std::uint8_t>(Function::GraphicsContextCommand),
			                                             static_cast<std::uint8_t>(objectID & 0xFF),
			                                             static_cast<std::uint8_t>(objectID >> 8),
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::SetBackgroundColor),
			                                             color,
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
				retVal = (0 != (largeFontSizesBitfield & (1 << (static_cast<std::uint8_t>(value) - static_cast<std::uint8_t>(FontSize::Size32x48) + 1))));
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
		if (nullptr != partnerControlFunction)
		{
			switch (state)
			{
				case StateMachineState::Disconnected:
				{
					sendWorkingSetMaintenenace = false;

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
						sendWorkingSetMaintenenace = true;
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
				}
				break;

				case StateMachineState::Failed:
				{
					constexpr std::uint32_t VT_STATE_MACHINE_RETRY_TIMEOUT_MS = 5000;
					sendWorkingSetMaintenenace = false;

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

		if ((sendWorkingSetMaintenenace) &&
		    (SystemTiming::time_expired_ms(lastWorkingSetMaintenanceTimestamp_ms, WORKING_SET_MAINTENANCE_TIMEOUT_MS)))
		{
			txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendWorkingSetMaintenance));
		}
		txFlags.process_all_flags();
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

	bool VirtualTerminalClient::send_end_of_object_pool()
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

	bool VirtualTerminalClient::send_working_set_master()
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

	bool VirtualTerminalClient::send_aux_n_preferred_assignment()
	{
		//! @todo load preferred assignment from saved configuration
		//! @todo only send command if there is an Auxiliary Function Type 2 object in the object pool
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

	bool VirtualTerminalClient::send_aux_n_assignment_response(std::uint16_t functionObjectID, bool hasError, bool isAlreadyAssigned)
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

	void VirtualTerminalClient::set_state(StateMachineState value)
	{
		stateMachineTimestamp_ms = SystemTiming::get_timestamp_ms();
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

	void VirtualTerminalClient::process_button_event_callback(KeyActivationCode keyEvent, std::uint8_t keyNumber, std::uint16_t objectID, std::uint16_t parentObjectID, VirtualTerminalClient *parentPointer)
	{
		for (std::size_t i = 0; i < parentPointer->buttonEventCallbacks.size(); i++)
		{
			if (nullptr != parentPointer->buttonEventCallbacks[i])
			{
				parentPointer->buttonEventCallbacks[i](keyEvent, keyNumber, objectID, parentObjectID, parentPointer);
			}
		}
	}

	void VirtualTerminalClient::process_softkey_event_callback(KeyActivationCode keyEvent, std::uint8_t keyNumber, std::uint16_t objectID, std::uint16_t parentObjectID, VirtualTerminalClient *parentPointer)
	{
		for (std::size_t i = 0; i < parentPointer->softKeyEventCallbacks.size(); i++)
		{
			if (nullptr != parentPointer->softKeyEventCallbacks[i])
			{
				parentPointer->softKeyEventCallbacks[i](keyEvent, keyNumber, objectID, parentObjectID, parentPointer);
			}
		}
	}

	void VirtualTerminalClient::process_pointing_event_callback(KeyActivationCode keyEvent,
	                                                            std::uint16_t xPosition,
	                                                            std::uint16_t yPosition,
	                                                            std::uint16_t parentMaskObjectID,
	                                                            VirtualTerminalClient *parentPointer)
	{
		for (std::size_t i = 0; i < parentPointer->pointingEventCallbacks.size(); i++)
		{
			if (nullptr != parentPointer->pointingEventCallbacks[i])
			{
				parentPointer->pointingEventCallbacks[i](keyEvent, xPosition, yPosition, parentMaskObjectID, parentPointer);
			}
		}
	}

	void VirtualTerminalClient::process_select_input_object_callback(std::uint16_t objectID, bool objectSelected, bool objectOpenForInput, VirtualTerminalClient *parentPointer)
	{
		for (std::size_t i = 0; i < parentPointer->selectInputObjectCallbacks.size(); i++)
		{
			if (nullptr != parentPointer->selectInputObjectCallbacks[i])
			{
				parentPointer->selectInputObjectCallbacks[i](objectID, objectSelected, objectOpenForInput, parentPointer);
			}
		}
	}

	void VirtualTerminalClient::process_esc_message_callback(std::uint16_t objectID, ESCMessageErrorCode errorCode, VirtualTerminalClient *parentPointer)
	{
		for (std::size_t i = 0; i < parentPointer->escMessageCallbacks.size(); i++)
		{
			if (nullptr != parentPointer->escMessageCallbacks[i])
			{
				parentPointer->escMessageCallbacks[i](objectID, errorCode, parentPointer);
			}
		}
	}

	void VirtualTerminalClient::process_change_numeric_value_callback(std::uint16_t objectID, std::uint32_t value, VirtualTerminalClient *parentPointer)
	{
		for (std::size_t i = 0; i < parentPointer->changeNumericValueCallbacks.size(); i++)
		{
			if (nullptr != parentPointer->changeNumericValueCallbacks[i])
			{
				parentPointer->changeNumericValueCallbacks[i](objectID, value, parentPointer);
			}
		}
	}

	void VirtualTerminalClient::process_change_active_mask_callback(std::uint16_t maskObjectID,
	                                                                std::uint16_t errorObjectID,
	                                                                std::uint16_t parentObjectID,
	                                                                bool missingObjects,
	                                                                bool maskOrChildHasErrors,
	                                                                bool anyOtherEror,
	                                                                bool poolDeleted,
	                                                                VirtualTerminalClient *parentPointer)
	{
		for (std::size_t i = 0; i < parentPointer->changeActiveMaskCallbacks.size(); i++)
		{
			if (nullptr != parentPointer->changeActiveMaskCallbacks[i])
			{
				parentPointer->changeActiveMaskCallbacks[i](maskObjectID,
				                                            errorObjectID,
				                                            parentObjectID,
				                                            missingObjects,
				                                            maskOrChildHasErrors,
				                                            anyOtherEror,
				                                            poolDeleted,
				                                            parentPointer);
			}
		}
	}

	void VirtualTerminalClient::process_change_soft_key_mask_callback(std::uint16_t dataOrAlarmMaskObjectID,
	                                                                  std::uint16_t softKeyMaskObjectID,
	                                                                  bool missingObjects,
	                                                                  bool maskOrChildHasErrors,
	                                                                  bool anyOtherEror,
	                                                                  bool poolDeleted,
	                                                                  VirtualTerminalClient *parentPointer)
	{
		for (std::size_t i = 0; i < parentPointer->changeSoftKeyMaskCallbacks.size(); i++)
		{
			if (nullptr != parentPointer->changeSoftKeyMaskCallbacks[i])
			{
				parentPointer->changeSoftKeyMaskCallbacks[i](dataOrAlarmMaskObjectID,
				                                             softKeyMaskObjectID,
				                                             missingObjects,
				                                             maskOrChildHasErrors,
				                                             anyOtherEror,
				                                             poolDeleted,
				                                             parentPointer);
			}
		}
	}

	void VirtualTerminalClient::process_change_string_value_callback(std::uint16_t objectID, std::string value, VirtualTerminalClient *parentPointer)
	{
		for (std::size_t i = 0; i < parentPointer->changeStringValueCallbacks.size(); i++)
		{
			if (nullptr != parentPointer->changeStringValueCallbacks[i])
			{
				parentPointer->changeStringValueCallbacks[i](objectID, value, parentPointer);
			}
		}
	}

	void VirtualTerminalClient::process_user_layout_hide_show_callback(std::uint16_t objectID, bool isHidden, VirtualTerminalClient *parentPointer)
	{
		for (std::size_t i = 0; i < parentPointer->userLayoutHideShowCallbacks.size(); i++)
		{
			if (nullptr != parentPointer->userLayoutHideShowCallbacks[i])
			{
				parentPointer->userLayoutHideShowCallbacks[i](objectID, isHidden, parentPointer);
			}
		}
	}

	void VirtualTerminalClient::process_audio_signal_termination_callback(bool isTerminated, VirtualTerminalClient *parentPointer)
	{
		for (std::size_t i = 0; i < parentPointer->audioSignalTerminationCallbacks.size(); i++)
		{
			if (nullptr != parentPointer->audioSignalTerminationCallbacks[i])
			{
				parentPointer->audioSignalTerminationCallbacks[i](isTerminated, parentPointer);
			}
		}
	}

	void VirtualTerminalClient::process_auxiliary_input_callback(AssignedAuxiliaryFunction function, std::uint32_t value1, std::uint32_t value2, VirtualTerminalClient *parentPointer)
	{
		for (std::size_t i = 0; i < parentPointer->auxiliaryInputCallbacks.size(); i++)
		{
			if (nullptr != parentPointer->auxiliaryInputCallbacks[i])
			{
				parentPointer->auxiliaryInputCallbacks[i](function, value1, value2, parentPointer);
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

								parentVT->process_softkey_event_callback(static_cast<KeyActivationCode>(keyCode), keyNumber, objectID, parentObjectID, parentVT);
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
								parentVT->process_button_event_callback(static_cast<KeyActivationCode>(keyCode), keyNumber, objectID, parentObjectID, parentVT);
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::PointingEventMessage):
						{
							std::uint16_t xPosition = message->get_uint16_at(1);
							std::uint16_t yPosition = message->get_uint16_at(3);

							std::uint8_t touchState = static_cast<std::uint8_t>(KeyActivationCode::ButtonPressedOrLatched);
							std::uint16_t partenMaskObjectID = NULL_OBJECT_ID;
							if (parentVT->get_vt_version_supported(VTVersion::Version6))
							{
								// VT version is at least 6
								touchState = message->get_uint8_at(5) & 0x0F;
								partenMaskObjectID = message->get_uint16_at(6);
								//! @todo process TAN
							}
							else if (parentVT->get_vt_version_supported(VTVersion::Version4))
							{
								// VT version is either 4 or 5
								touchState = message->get_uint8_at(5);
							}

							if (touchState <= static_cast<std::uint8_t>(KeyActivationCode::ButtonPressAborted))
							{
								parentVT->process_pointing_event_callback(static_cast<KeyActivationCode>(touchState),
								                                          xPosition,
								                                          yPosition,
								                                          partenMaskObjectID,
								                                          parentVT);
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

							parentVT->process_select_input_object_callback(objectID, objectSelected, objectOpenForInput, parentVT);
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

								parentVT->process_esc_message_callback(objectID, static_cast<ESCMessageErrorCode>(errorCode), parentVT);
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
							parentVT->process_change_numeric_value_callback(objectID, value, parentVT);
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

							parentVT->process_change_active_mask_callback(maskObjectID,
							                                              errorObjectID,
							                                              parentObjectID,
							                                              missingObjects,
							                                              maskOrChildHasErrors,
							                                              anyOtherError,
							                                              poolDeleted,
							                                              parentVT);
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

							parentVT->process_change_soft_key_mask_callback(dataOrAlarmMaskID,
							                                                softKeyMaskID,
							                                                missingObjects,
							                                                maskOrChildHasErrors,
							                                                anyOtherError,
							                                                poolDeleted,
							                                                parentVT);
						}
						break;

						case static_cast<std::uint8_t>(Function::VTChangeStringValueMessage):
						{
							std::uint16_t objectID = message->get_uint16_at(1);
							std::uint8_t stringLength = message->get_uint8_at(3);
							std::string value = std::string(message->get_data().begin() + 4, message->get_data().begin() + 4 + stringLength);

							parentVT->process_change_string_value_callback(objectID, value, parentVT);
						}
						break;

						case static_cast<std::uint8_t>(Function::VTOnUserLayoutHideShowMessage):
						{
							std::uint16_t objectID = message->get_uint16_at(1);
							bool hidden = !message->get_bool_at(3, 0);

							parentVT->process_user_layout_hide_show_callback(objectID, hidden, parentVT);

							// There could be two layout messages in one packet
							objectID = message->get_uint16_at(4);
							if (objectID != NULL_OBJECT_ID)
							{
								hidden = !message->get_bool_at(6, 0);
								parentVT->process_user_layout_hide_show_callback(objectID, hidden, parentVT);
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

							parentVT->process_audio_signal_termination_callback(terminated, parentVT);

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
							if (message->get_bool_at(3, 1))
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[AUX-N]: Preferred Assignment Error - Function Object ID(S) not valid");
							}
							if (message->get_bool_at(3, 2))
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[AUX-N]: Preferred Assignment Error - Input Object ID(s) not valid");
							}
							if (message->get_bool_at(3, 3))
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[AUX-N]: Preferred Assignment Error - Duplicate Object ID of Auxiliary Function");
							}
							if (message->get_bool_at(3, 4))
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
								//! @todo load the preferred assignment into parentVT->auxiliaryInputDevices
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
								if (DEFAULT_NAME == isoName)
								{
									for (AuxiliaryInputDevice &aux : parentVT->auxiliaryInputDevices)
									{
										aux.functions.clear();
										if (storeAsPreferred)
										{
											//! @todo save preferred assignment to persistent configuration
										}
									}
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Info, "[AUX-N] Unassigned all functions");
								}
								else if (0x1F == functionType)
								{
									for (AuxiliaryInputDevice &aux : parentVT->auxiliaryInputDevices)
									{
										if (aux.name == isoName)
										{
											aux.functions.clear();
											if (storeAsPreferred)
											{
												//! @todo save preferred assignment to persistent configuration
											}
											CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Info, "[AUX-N] Unassigned function " + isobus::to_string(static_cast<int>(functionObjectID)) + " from input " + isobus::to_string(static_cast<int>(inputObjectID)));
											break;
										}
									}
								}
								else if (NULL_OBJECT_ID == inputObjectID)
								{
									for (AuxiliaryInputDevice &aux : parentVT->auxiliaryInputDevices)
									{
										if (aux.name == isoName)
										{
											for (auto iter = aux.functions.begin(); iter != aux.functions.end(); iter++)
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
											}
										}
									}
								}
								else if (NULL_OBJECT_ID == functionObjectID)
								{
									for (AuxiliaryInputDevice &aux : parentVT->auxiliaryInputDevices)
									{
										if (aux.name == isoName)
										{
											for (auto iter = aux.functions.begin(); iter != aux.functions.end(); iter++)
											{
												if (iter->inputObjectID == inputObjectID)
												{
													aux.functions.erase(iter);
													if (storeAsPreferred)
													{
														//! @todo save preferred assignment to persistent configuration
													}
													CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Info, "[AUX-N] Unassigned function " + isobus::to_string(static_cast<int>(functionObjectID)) + " from input " + isobus::to_string(static_cast<int>(inputObjectID)));
												}
											}
										}
									}
								}
								else
								{
									bool found = false;
									for (AuxiliaryInputDevice &aux : parentVT->auxiliaryInputDevices)
									{
										if (aux.name == isoName)
										{
											found = true;
											if (static_cast<std::uint8_t>(AuxiliaryTypeTwoFunctionType::QuadratureBooleanMomentary) >= functionType)
											{
												AssignedAuxiliaryFunction assignment(functionObjectID, inputObjectID, static_cast<AuxiliaryTypeTwoFunctionType>(functionType));
												auto location = std::find(aux.functions.begin(), aux.functions.end(), assignment);
												if (aux.functions.end() == location)
												{
													aux.functions.push_back(assignment);
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
											break;
										}
									}
									if (false == found)
									{
										hasError = true;
										//! @todo prettier logging of NAME
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[AUX-N]: Unable to store preferred assignment due to missing auxiliary input device with name: " + isobus::to_string(isoName));
									}
								}
								parentVT->send_aux_n_assignment_response(functionObjectID, hasError, isAlreadyAssigned);
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
							bool learnModeActive = message->get_bool_at(7, 0);
							bool inputActive = message->get_bool_at(7, 1); // Only in learn mode?
							bool controlIsLocked = false;
							bool interactionWhileLocked = false;
							if (parentVT->get_vt_version_supported(VTVersion::Version6))
							{
								controlIsLocked = message->get_bool_at(7, 2);
								interactionWhileLocked = message->get_bool_at(7, 3);
							}
							for (AuxiliaryInputDevice &aux : parentVT->auxiliaryInputDevices)
							{
								for (AssignedAuxiliaryFunction &assignment : aux.functions)
								{
									if (assignment.inputObjectID == inputObjectID)
									{
										parentVT->process_auxiliary_input_callback(assignment, value1, value2, parentVT);
									}
								}
							}
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
								if (message->get_uint8_at(2) <= static_cast<std::uint8_t>(GraphicMode::TwoHundredFiftySixColor))
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
									if (parentVT->send_aux_n_preferred_assignment())
									{
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Debug, "[AUX-N]: Sent preferred assignments.");
									}
									else
									{
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[AUX-N]: Failed to send preferred assignments.");
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
									// Check if we need to store this pool
									if (!parentVT->objectPools[0].versionLabel.empty())
									{
										parentVT->set_state(StateMachineState::SendStoreVersion);
									}
									else
									{
										parentVT->set_state(StateMachineState::Connected);
									}

									if (parentVT->send_aux_n_preferred_assignment())
									{
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Debug, "[AUX-N]: Sent preferred assignments.");
									}
									else
									{
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[AUX-N]: Failed to send preferred assignments.");
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
								bool found = false;
								for (AuxiliaryInputDevice &aux : parentVT->auxiliaryInputDevices)
								{
									if (aux.modelIdentificationCode == modelIdentificationCode)
									{
										found = true;
									}
								}
								if (false == found)
								{
									AuxiliaryInputDevice inputDevice{ message->get_source_control_function()->get_NAME().get_full_name(), modelIdentificationCode, {} };
									parentVT->auxiliaryInputDevices.push_back(inputDevice);
									//! @todo prettier logging of NAME
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Info, "[AUX-N]: New auxiliary input device with name: " + isobus::to_string(inputDevice.name) + " and model identification code: " + std::to_string(modelIdentificationCode));
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
