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

namespace isobus
{
	VirtualTerminalClientUpdateHelper::VirtualTerminalClientUpdateHelper(std::shared_ptr<VirtualTerminalClient> client) :
	  VirtualTerminalClientStateTracker(client->get_internal_control_function()),
	  client(client)
	{
		numericValueChangeEventHandle = client->add_vt_change_numeric_value_event_listener(
		  std::bind(&VirtualTerminalClientUpdateHelper::process_numeric_value_change_event, this, std::placeholders::_1));
	}

	bool VirtualTerminalClientUpdateHelper::set_numeric_value(std::uint16_t object_id, std::uint32_t value)
	{
		if (numericValueStates.find(object_id) == numericValueStates.end())
		{
			CANStackLogger::warn("[VTStateHelper] set_numeric_value: objectId %lu not tracked", object_id);
			return false;
		}
		if (numericValueStates.at(object_id) == value)
		{
			return false;
		}

		bool success = client->send_change_numeric_value(object_id, value);
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
		client->send_change_numeric_value(event.objectID, targetValue);
	}

} // namespace isobus
