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

}; // namespace isobus
