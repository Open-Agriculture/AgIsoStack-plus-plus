//================================================================================================
/// @file isobus_virtual_terminal_client_update_helper.cpp
///
/// @brief A helper class to update and track the state of an active working set.
/// @author Daan Steenbergen
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================

#include "isobus/isobus/isobus_virtual_terminal_client_update_helper.hpp"

#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/platform_endianness.hpp"

namespace isobus
{
	VirtualTerminalClientUpdateHelper::VirtualTerminalClientUpdateHelper(std::shared_ptr<VirtualTerminalClient> client) :
	  VirtualTerminalClientStateTracker(nullptr == client ? nullptr : client->get_internal_control_function()),
	  vtClient(client)
	{
		if (nullptr == client)
		{
			LOG_ERROR("[VTStateHelper] constructor: client is nullptr");
			return;
		}
		numericValueChangeEventHandle = client->get_vt_change_numeric_value_event_dispatcher().add_listener(
		  std::bind(&VirtualTerminalClientUpdateHelper::process_numeric_value_change_event, this, std::placeholders::_1));
	}

	VirtualTerminalClientUpdateHelper::~VirtualTerminalClientUpdateHelper()
	{
		if (nullptr != vtClient)
		{
			vtClient->get_vt_change_numeric_value_event_dispatcher().remove_listener(numericValueChangeEventHandle);
		}
	}

	bool VirtualTerminalClientUpdateHelper::set_numeric_value(std::uint16_t object_id, std::uint32_t value)
	{
		if (nullptr == client)
		{
			LOG_ERROR("[VTStateHelper] set_numeric_value: client is nullptr");
			return false;
		}
		if (numericValueStates.find(object_id) == numericValueStates.end())
		{
			LOG_WARNING("[VTStateHelper] set_numeric_value: objectId %hu not tracked", object_id);
			return false;
		}
		if (numericValueStates.at(object_id) == value)
		{
			return true;
		}

		bool success = vtClient->send_change_numeric_value(object_id, value);
		if (success)
		{
			numericValueStates[object_id] = value;
		}
		return success;
	}

	bool VirtualTerminalClientUpdateHelper::increase_numeric_value(std::uint16_t object_id, std::uint32_t step)
	{
		return set_numeric_value(object_id, get_numeric_value(object_id) + step);
	}

	bool VirtualTerminalClientUpdateHelper::decrease_numeric_value(std::uint16_t object_id, std::uint32_t step)
	{
		return set_numeric_value(object_id, get_numeric_value(object_id) - step);
	}

	void VirtualTerminalClientUpdateHelper::set_callback_validate_numeric_value(const std::function<bool(std::uint16_t, std::uint32_t)> &callback)
	{
		callbackValidateNumericValue = callback;
	}

	void VirtualTerminalClientUpdateHelper::process_numeric_value_change_event(const VirtualTerminalClient::VTChangeNumericValueEvent &event)
	{
		if (numericValueStates.find(event.objectID) == numericValueStates.end())
		{
			// Only proccess numeric value changes for tracked objects.
			return;
		}

		if (numericValueStates.at(event.objectID) == event.value)
		{
			// Do not process the event if the value has not changed.
			return;
		}

		std::uint32_t targetValue = event.value; // Default to the value received in the event.
		if ((callbackValidateNumericValue != nullptr) && callbackValidateNumericValue(event.objectID, event.value))
		{
			// If the callback function returns false, reject the change by sending the previous value.
			targetValue = numericValueStates.at(event.objectID);
		}
		vtClient->send_change_numeric_value(event.objectID, targetValue);
	}

	bool VirtualTerminalClientUpdateHelper::set_active_data_or_alarm_mask(std::uint16_t workingSetId, std::uint16_t dataOrAlarmMaskId)
	{
		if (nullptr == client)
		{
			LOG_ERROR("[VTStateHelper] set_active_data_or_alarm_mask: client is nullptr");
			return false;
		}
		if (activeDataOrAlarmMask == dataOrAlarmMaskId)
		{
			return true;
		}

		bool success = vtClient->send_change_active_mask(workingSetId, dataOrAlarmMaskId);
		if (success)
		{
			activeDataOrAlarmMask = dataOrAlarmMaskId;
		}
		return success;
	}

	bool VirtualTerminalClientUpdateHelper::set_active_soft_key_mask(VirtualTerminalClient::MaskType maskType, std::uint16_t maskId, std::uint16_t softKeyMaskId)
	{
		if (nullptr == client)
		{
			LOG_ERROR("[VTStateHelper] set_active_soft_key_mask: client is nullptr");
			return false;
		}
		if (softKeyMasks.find(maskId) == softKeyMasks.end())
		{
			LOG_WARNING("[VTStateHelper] set_active_soft_key_mask: data/alarm mask '%hu' not tracked", maskId);
			return false;
		}
		if (softKeyMasks.at(maskId) == softKeyMaskId)
		{
			return true;
		}

		bool success = vtClient->send_change_softkey_mask(maskType, maskId, softKeyMaskId);
		if (success)
		{
			softKeyMasks[maskId] = softKeyMaskId;
		}
		return success;
	}

	bool VirtualTerminalClientUpdateHelper::set_attribute(std::uint16_t objectId, std::uint8_t attribute, std::uint32_t value)
	{
		if (nullptr == client)
		{
			LOG_ERROR("[VTStateHelper] set_attribute: client is nullptr");
			return false;
		}
		if (attributeStates.find(objectId) == attributeStates.end())
		{
			LOG_ERROR("[VTStateHelper] set_attribute: objectId %hu not tracked", objectId);
			return false;
		}
		if (attributeStates.at(objectId).find(attribute) == attributeStates.at(objectId).end())
		{
			LOG_WARNING("[VTStateHelper] set_attribute: attribute %hhu of objectId %hu not tracked", attribute, objectId);
			return false;
		}
		if (attributeStates.at(objectId).at(attribute) == value)
		{
			return true;
		}

		bool success = vtClient->send_change_attribute(objectId, attribute, value);
		if (success)
		{
			attributeStates[objectId][attribute] = value;
		}
		return success;
	}

	bool VirtualTerminalClientUpdateHelper::set_attribute(std::uint16_t objectId, std::uint8_t attribute, float value)
	{
		return set_attribute(objectId, attribute, float_to_little_endian(value));
	}

} // namespace isobus
