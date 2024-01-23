//================================================================================================
/// @file isobus_virtual_terminal_client_state_tracker.cpp
///
/// @brief A helper class to track the state of an active working set.
/// @author Daan Steenbergen
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/isobus_virtual_terminal_client_state_tracker.hpp"

#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client.hpp"

#include <algorithm>

namespace isobus
{
	VirtualTerminalClientStateTracker::VirtualTerminalClientStateTracker(std::shared_ptr<ControlFunction> client) :
	  client(client)
	{
	}

	VirtualTerminalClientStateTracker::~VirtualTerminalClientStateTracker()
	{
		terminate();
	}

	void VirtualTerminalClientStateTracker::initialize()
	{
		CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), process_rx_message, this);
	}

	void VirtualTerminalClientStateTracker::terminate()
	{
		CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), process_rx_message, this);
	}

	void VirtualTerminalClientStateTracker::add_tracked_numeric_value(std::uint16_t objectId, std::uint32_t initialValue)
	{
		if (numericValueStates.find(objectId) != numericValueStates.end())
		{
			CANStackLogger::warn("[VTStateHelper] add_tracked_numeric_value: objectId %lu already tracked", objectId);
			return;
		}

		numericValueStates[objectId] = initialValue;
	}

	void VirtualTerminalClientStateTracker::remove_tracked_numeric_value(std::uint16_t objectId)
	{
		if (numericValueStates.find(objectId) == numericValueStates.end())
		{
			CANStackLogger::warn("[VTStateHelper] remove_tracked_numeric_value: objectId %lu was not tracked", objectId);
			return;
		}

		numericValueStates.erase(objectId);
	}

	std::uint32_t VirtualTerminalClientStateTracker::get_numeric_value(std::uint16_t objectId) const
	{
		if (numericValueStates.find(objectId) == numericValueStates.end())
		{
			CANStackLogger::warn("[VTStateHelper] get_numeric_value: objectId %lu not tracked", objectId);
			return 0;
		}

		return numericValueStates.at(objectId);
	}

	void VirtualTerminalClientStateTracker::process_rx_message(const CANMessage &message, void *parentPointer)
	{
		auto *parent = static_cast<VirtualTerminalClientStateTracker *>(parentPointer);
		parent->process_rx_message(message);
	}

	void VirtualTerminalClientStateTracker::process_rx_message(const CANMessage &message)
	{
		if (message.has_valid_source_control_function() &&
		    message.is_destination(client) &&
		    message.is_parameter_group_number(CANLibParameterGroupNumber::VirtualTerminalToECU) &&
		    (message.get_data_length() >= 1))
		{
			std::uint8_t function = message.get_uint8_at(0);
			switch (function)
			{
				case static_cast<std::uint8_t>(VirtualTerminalClient::Function::ChangeNumericValueCommand):
				{
					if (CAN_DATA_LENGTH == message.get_data_length())
					{
						auto errorCode = message.get_uint8_at(3);
						if (errorCode == 0)
						{
							std::uint16_t objectId = message.get_uint16_at(1);
							if (numericValueStates.find(objectId) != numericValueStates.end())
							{
								std::uint32_t value = message.get_uint32_at(4);
								numericValueStates[objectId] = value;
							}
						}
					}
				}
				break;

				case static_cast<std::uint8_t>(VirtualTerminalClient::Function::VTChangeNumericValueMessage):
				{
					if (CAN_DATA_LENGTH == message.get_data_length())
					{
						std::uint16_t objectId = message.get_uint16_at(1);
						if (numericValueStates.find(objectId) != numericValueStates.end())
						{
							std::uint32_t value = message.get_uint32_at(4);
							numericValueStates[objectId] = value;
						}
					}
				}
				break;

				default:
					break;
			}
		}
	}
}; // namespace isobus
