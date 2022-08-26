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

#include <cstring>

namespace isobus
{

	VirtualTerminalClient::VirtualTerminalClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource) :
	  partnerControlFunction(partner),
	  myControlFunction(clientSource)
	{
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

} // namespace isobus
