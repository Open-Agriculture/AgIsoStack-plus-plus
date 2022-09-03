//================================================================================================
/// @file isobus_virtual_terminal.cpp
///
/// @brief Implements the client for a virtual terminal
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#include "isobus_virtual_terminal_client.hpp"
#include "can_general_parameter_group_numbers.hpp"
#include "can_network_manager.hpp"
#include "can_warning_logger.hpp"
#include "system_timing.hpp"

#include <cstring>
#include <algorithm>

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
	  xPixels(0),
	  yPixels(0),
	  hardwareFeaturesBitfield(0),
	  state(StateMachineState::Disconnected),
	  stateMachineTimestamp_ms(0),
	  lastWorkingSetMaintenanceTimestamp_ms(0),
	  workerThread(nullptr),
	  initialized(false),
	  sendWorkingSetMaintenenace(false),
	  shouldTerminate(false),
	  objectPoolDataCallback(nullptr),
	  objectPoolSize_bytes(0)
	{
		if (nullptr != partnerControlFunction)
		{
			partnerControlFunction->add_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), process_rx_message);
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

	bool VirtualTerminalClient::get_is_initialized()
	{
		return initialized;
	}

	void VirtualTerminalClient::terminate()
	{
		if (initialized)
		{
			shouldTerminate = true;

			if (nullptr != workerThread)
			{
				workerThread->join();
				delete workerThread;
				workerThread = nullptr;
			}
		}
	}

	void VirtualTerminalClient::RegisterVTSoftKeyEventCallback(VTKeyEventCallback value)
	{
		softKeyEventCallbacks.push_back(value);
	}

	void VirtualTerminalClient::RemoveVTSoftKeyEventCallback(VTKeyEventCallback value)
	{
		auto callbackLocation = find(softKeyEventCallbacks.begin(), softKeyEventCallbacks.end(), value);

		if (softKeyEventCallbacks.end() != callbackLocation)
		{
			softKeyEventCallbacks.erase(callbackLocation);
		}
	}

	void VirtualTerminalClient::RegisterVTButtonEventCallback(VTKeyEventCallback value)
	{
		buttonEventCallbacks.push_back(value);
	}

	void VirtualTerminalClient::RemoveVTButtonEventCallback(VTKeyEventCallback value)
	{
		auto callbackLocation = find(buttonEventCallbacks.begin(), buttonEventCallbacks.end(), value);

		if (buttonEventCallbacks.end() != callbackLocation)
		{
			buttonEventCallbacks.erase(callbackLocation);
		}
	}

	void VirtualTerminalClient::RegisterVTPointingEventCallback(VTPointingEventCallback value)
	{
		pointingEventCallbacks.push_back(value);
	}

	void VirtualTerminalClient::RemoveVTPointingEventCallback(VTPointingEventCallback value)
	{
		auto callbackLocation = find(pointingEventCallbacks.begin(), pointingEventCallbacks.end(), value);

		if (pointingEventCallbacks.end() != callbackLocation)
		{
			pointingEventCallbacks.erase(callbackLocation);
		}
	}

	void VirtualTerminalClient::RegisterVTSelectInputObjectEventCallback(VTSelectInputObjectCallback value)
	{
		selectInputObjectCallbacks.push_back(value);
	}

	void VirtualTerminalClient::RemoveVTSelectInputObjectEventCallback(VTSelectInputObjectCallback value)
	{
		auto callbackLocation = find(selectInputObjectCallbacks.begin(), selectInputObjectCallbacks.end(), value);

		if (selectInputObjectCallbacks.end() != callbackLocation)
		{
			selectInputObjectCallbacks.erase(callbackLocation);
		}
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
			                                             static_cast<std::uint8_t>(GraphicsContextSubCommandID::SetFontAttributesObjectOD),
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

	void VirtualTerminalClient::set_object_pool(std::uint8_t poolIndex, VTVersion poolSupportedVTVersion, const std::uint8_t *pool, std::uint32_t size)
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

	void VirtualTerminalClient::set_object_pool(std::uint8_t poolIndex, VTVersion poolSupportedVTVersion, const std::vector<std::uint8_t> *pool)
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
					if (send_get_memory(objectPoolSize_bytes))
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
						CANStackLogger::CAN_stack_log("VT Get Memory Response Timout");
					}
				}
				break;

				case StateMachineState::SendGetNumberSoftkeys:
				{
					if(send_get_number_of_softkeys())
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
						CANStackLogger::CAN_stack_log("VT Get Number Softkeys Response Timout");
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
						CANStackLogger::CAN_stack_log("VT Get Text Font Data Response Timout");
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
						CANStackLogger::CAN_stack_log("VT Get Hardware Response Timout");
					}
				}
				break;

				case StateMachineState::UploadObjectPool:
				{

				}
				break;

				case StateMachineState::SendEndOfObjectPool:
				{
					//if (send_end_of_object_pool())
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
						CANStackLogger::CAN_stack_log("VT Get End of Object Pool Response Timout");
					}
				}
				break;

				case StateMachineState::Connected:
				{
					// Check for timeouts
					if (SystemTiming::time_expired_ms(lastVTStatusTimestamp_ms, VT_STATUS_TIMEOUT_MS))
					{
						set_state(StateMachineState::Disconnected);
						CANStackLogger::CAN_stack_log("VT Status Timout");
					}
				}
				break;

				case StateMachineState::Failed:
				{
					sendWorkingSetMaintenenace = false;
					CANStackLogger::CAN_stack_log("VT Connection Failed");
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
		delete [] buffer;
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

	void VirtualTerminalClient::set_state(StateMachineState value)
	{
		stateMachineTimestamp_ms = SystemTiming::get_timestamp_ms();
		state = value;
	}

	void VirtualTerminalClient::process_button_event_callback(KeyActivationCode keyEvent, std::uint8_t keyNumber, std::uint16_t objectID, std::uint16_t parentObjectID, VirtualTerminalClient *parentPointer)
	{
		for (uint32_t i = 0; i < parentPointer->buttonEventCallbacks.size(); i++)
		{
			if (nullptr != parentPointer->buttonEventCallbacks[i])
			{
				parentPointer->buttonEventCallbacks[i](keyEvent, keyNumber, objectID, parentObjectID, parentPointer);
			}
		}
	}

	void VirtualTerminalClient::process_softkey_event_callback(KeyActivationCode keyEvent, std::uint8_t keyNumber, std::uint16_t objectID, std::uint16_t parentObjectID, VirtualTerminalClient *parentPointer)
	{
		for (uint32_t i = 0; i < parentPointer->softKeyEventCallbacks.size(); i++)
		{
			if (nullptr != parentPointer->softKeyEventCallbacks[i])
			{
				parentPointer->softKeyEventCallbacks[i](keyEvent, keyNumber, objectID, parentObjectID, parentPointer);
			}
		}
	}

	void VirtualTerminalClient::process_pointing_event_callback(KeyActivationCode signal, std::uint16_t xPosition, std::uint16_t yPosition, VirtualTerminalClient *parentPointer)
	{
		for (uint32_t i = 0; i < parentPointer->pointingEventCallbacks.size(); i++)
		{
			if (nullptr != parentPointer->pointingEventCallbacks[i])
			{
				parentPointer->pointingEventCallbacks[i](signal, xPosition, yPosition, parentPointer);
			}
		}
	}

	void VirtualTerminalClient::process_select_input_object_callback(std::uint16_t objectID, bool objectSelected, bool objectOpenForInput, VirtualTerminalClient *parentPointer)
	{
		for (uint32_t i = 0; i < parentPointer->selectInputObjectCallbacks.size(); i++)
		{
			if (nullptr != parentPointer->selectInputObjectCallbacks[i])
			{
				parentPointer->selectInputObjectCallbacks[i](objectID, objectSelected, objectOpenForInput, parentPointer);
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
		    (CAN_DATA_LENGTH == message->get_data_length()))
		{
			VirtualTerminalClient *parentVT = reinterpret_cast<VirtualTerminalClient *>(parentPointer);

			switch (message->get_identifier().get_parameter_group_number())
			{
				//! @todo Handle NACK, any other PGNs needed as well
				
				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU):
				{
					switch (message->get_data().at(0))
					{
						case static_cast <std::uint8_t>(Function::SoftKeyActivationMessage):
						{
							std::uint8_t keyCode = message->get_data().at(1);
							if (keyCode <= static_cast<std::uint8_t>(KeyActivationCode::ButtonPressAborted))
							{
								parentVT->process_softkey_event_callback(static_cast<KeyActivationCode>(keyCode), 
									static_cast<std::uint16_t>(message->get_data().at(6)), 
									(static_cast<std::uint16_t>(message->get_data().at(2)) | static_cast<std::uint16_t>(message->get_data().at(3) << 8)), 
									(static_cast<std::uint16_t>(message->get_data().at(4)) | static_cast<std::uint16_t>(message->get_data().at(5) << 8)), 
									parentVT);
							}
						}
						break;

						case static_cast <std::uint8_t>(Function::ButtonActivationMessage):
						{
							std::uint8_t keyCode = message->get_data().at(1);
							if (keyCode <= static_cast<std::uint8_t>(KeyActivationCode::ButtonPressAborted))
							{
								parentVT->process_button_event_callback(static_cast<KeyActivationCode>(keyCode),
								                                        static_cast<std::uint16_t>(message->get_data().at(6)),
								                                        (static_cast<std::uint16_t>(message->get_data().at(2)) |
								                                         static_cast<std::uint16_t>(message->get_data().at(3) << 8)),
								                                        (static_cast<std::uint16_t>(message->get_data().at(4)) |
								                                         static_cast<std::uint16_t>(message->get_data().at(5) << 8)),
								                                        parentVT);
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::PointingEventMessage): 
						{
							std::uint16_t xPosition = (static_cast<std::uint16_t>(message->get_data().at(1)) &
							                           ((static_cast<std::uint16_t>(message->get_data().at(2))) << 8));
							std::uint16_t yPosition = (static_cast<std::uint16_t>(message->get_data().at(3)) &
							                           ((static_cast<std::uint16_t>(message->get_data().at(4))) << 8));
							std::uint8_t keyCode = message->get_data().at(5) & 0x0F;

							if (VTVersion::Version6 == parentVT->get_connected_vt_version())
							{
								//! @todo process TAN
							}							

							if (keyCode <= static_cast<std::uint8_t>(KeyActivationCode::ButtonPressAborted))
							{
								parentVT->process_pointing_event_callback(static_cast<KeyActivationCode>(keyCode), xPosition, yPosition, parentVT);
							}
						}
						break;	

						case static_cast<std::uint8_t>(Function::SelectInputObjectCommand):
						{
							std::uint16_t objectID = (static_cast<std::uint16_t>(message->get_data()[1]) & 
								((static_cast<std::uint16_t>(message->get_data()[2])) << 8));
							bool objectSelected = (0x01 == message->get_data()[3]);
							bool objectOpenForInput = false;

							if (parentVT->get_connected_vt_version() >= VTVersion::Version4)
							{
								objectOpenForInput = (0x01 == (message->get_data()[4] & 0x01));
							}

							if (VTVersion::Version6 == parentVT->get_connected_vt_version())
							{
								//! @todo process TAN
							}	
							parentVT->process_select_input_object_callback(objectID, objectSelected, objectOpenForInput, parentVT);
						}
						break;

						case static_cast<std::uint8_t>(Function::VTStatusMessage):
						{
							parentVT->lastVTStatusTimestamp_ms = SystemTiming::get_timestamp_ms();
							parentVT->activeWorkingSetMasterAddress = message->get_data()[1];
							parentVT->activeWorkingSetDataMaskObjectID = (static_cast<std::uint16_t>(message->get_data()[2]) &
							                                              ((static_cast<std::uint16_t>(message->get_data()[3])) << 8));
							parentVT->activeWorkingSetSoftkeyMaskObjectID = (static_cast<std::uint16_t>(message->get_data()[4]) &
							                                                 ((static_cast<std::uint16_t>(message->get_data()[5])) << 8));
							parentVT->busyCodesBitfield = message->get_data()[6];
							parentVT->currentCommandFunctionCode = message->get_data()[7];

							if (StateMachineState::WaitForPartnerVTStatusMessage == parentVT->state)
							{
								parentVT->set_state(StateMachineState::ReadyForObjectPool);
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::GetMemoryMessage):
						{
							if (StateMachineState::WaitForGetMemoryResponse == parentVT->state)
							{
								parentVT->connectedVTVersion = message->get_data()[1];

								if (0 == message->get_data()[2])
								{
									// There IS enough memory
									parentVT->set_state(StateMachineState::SendGetNumberSoftkeys);
								}
								else
								{
									parentVT->set_state(StateMachineState::Failed);
								}
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::GetNumberOfSoftKeysMessage):
						{
							if (StateMachineState::WaitForGetNumberSoftKeysResponse == parentVT->state)
							{
								parentVT->softKeyXAxisPixels = message->get_data()[4];
								parentVT->softKeyYAxisPixels = message->get_data()[5];
								parentVT->numberVirtualSoftkeysPerSoftkeyMask = message->get_data()[6];
								parentVT->numberPhysicalSoftkeys = message->get_data()[7];
								parentVT->set_state(StateMachineState::SendGetTextFontData);
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::GetTextFontDataMessage):
						{
							if (StateMachineState::WaitForGetTextFontDataResponse == parentVT->state)
							{
								parentVT->smallFontSizesBitfield = message->get_data()[5];
								parentVT->largeFontSizesBitfield = message->get_data()[6];
								parentVT->fontStylesBitfield = message->get_data()[7];
								parentVT->set_state(StateMachineState::SendGetHardware);
							}
						}
						break;

						case static_cast<std::uint8_t>(Function::GetHardwareMessage):
						{
							if (StateMachineState::WaitForGetHardwareResponse == parentVT->state)
							{
								if (message->get_data()[2] <= static_cast<std::uint8_t>(GraphicMode::TwoHundredFiftySixColor))
								{
									parentVT->supportedGraphicsMode = static_cast<GraphicMode>(message->get_data()[2]);
								}
								parentVT->hardwareFeaturesBitfield = message->get_data()[3];
								parentVT->xPixels = (static_cast<std::uint16_t>(message->get_data()[4]) &
								                     ((static_cast<std::uint16_t>(message->get_data()[5])) << 8));
								parentVT->yPixels = (static_cast<std::uint16_t>(message->get_data()[6]) &
								                     ((static_cast<std::uint16_t>(message->get_data()[7])) << 8));
								parentVT->set_state(StateMachineState::UploadObjectPool);
							}
						}
						break;

						case static_cast <std::uint8_t>(Function::EndOfObjectPoolMessage):
						{
							if (StateMachineState::WaitForEndOfObjectPoolResponse == parentVT->state)
							{
								bool anyErrorInPool =  (0 != (message->get_data()[1] & 0x01));
								bool vtRanOutOfMemory = (0 != (message->get_data()[1] & 0x02));
								bool otherErrors = (0 != (message->get_data()[1] & 0x08));
								std::uint16_t parentObjectIDOfFaultyObject = (static_cast<std::uint16_t>(message->get_data()[2]) &
								                                              ((static_cast<std::uint16_t>(message->get_data()[3])) << 8));
								std::uint16_t objectIDOfFaultyObject = (static_cast<std::uint16_t>(message->get_data()[4]) &
								                                        ((static_cast<std::uint16_t>(message->get_data()[5])) << 8));
								std::uint8_t objectPoolErrorBitmask = message->get_data()[6];

								if ((!anyErrorInPool) &&
									(0 == objectPoolErrorBitmask))
								{
									parentVT->set_state(StateMachineState::Connected);
								}
								else
								{
									parentVT->set_state(StateMachineState::Failed);
									CANStackLogger::CAN_stack_log("Error in end of object pool message." +
									                              std::string("Faulty Object ") +
									                              std::to_string(static_cast<int>(objectIDOfFaultyObject)) +
									                              std::string(" Faulty Object Parent ") +
									                              std::to_string(static_cast<int>(parentObjectIDOfFaultyObject)) +
									                              std::string(" Pool error bitmask value ") +
																  std::to_string(static_cast<int>(objectPoolErrorBitmask)));
									if (vtRanOutOfMemory)
									{
										CANStackLogger::CAN_stack_log("VT Ran out of memory");
									}
									if (otherErrors)
									{
										CANStackLogger::CAN_stack_log("VT Reported other errors in EOM response");
									}
								}
							}
						}
						break;
					}
				}
				break;

				default:
				{
					CANStackLogger::CAN_stack_log("VT Client unknown message");
				}
				break;
			}
		}
		else
		{
			CANStackLogger::CAN_stack_log("VT-ECU Client message invalid");
		}
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
