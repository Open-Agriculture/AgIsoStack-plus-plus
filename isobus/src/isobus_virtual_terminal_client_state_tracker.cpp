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
		CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), process_rx_or_tx_message, this);
		CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal), process_rx_or_tx_message, this);
		CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), process_rx_or_tx_message, this);
	}

	void VirtualTerminalClientStateTracker::terminate()
	{
		CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), process_rx_or_tx_message, this);
		CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal), process_rx_or_tx_message, this);
		CANNetworkManager::CANNetwork.remove_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), process_rx_or_tx_message, this);
	}

	void VirtualTerminalClientStateTracker::add_tracked_numeric_value(std::uint16_t objectId, std::uint32_t initialValue)
	{
		if (numericValueStates.find(objectId) != numericValueStates.end())
		{
			CANStackLogger::warn("[VTStateHelper] add_tracked_numeric_value: objectId '%lu' already tracked", objectId);
			return;
		}

		numericValueStates[objectId] = initialValue;
	}

	void VirtualTerminalClientStateTracker::remove_tracked_numeric_value(std::uint16_t objectId)
	{
		if (numericValueStates.find(objectId) == numericValueStates.end())
		{
			CANStackLogger::warn("[VTStateHelper] remove_tracked_numeric_value: objectId '%lu' was not tracked", objectId);
			return;
		}

		numericValueStates.erase(objectId);
	}

	std::uint32_t VirtualTerminalClientStateTracker::get_numeric_value(std::uint16_t objectId) const
	{
		if (numericValueStates.find(objectId) == numericValueStates.end())
		{
			CANStackLogger::warn("[VTStateHelper] get_numeric_value: objectId '%lu' not tracked", objectId);
			return 0;
		}

		return numericValueStates.at(objectId);
	}

	std::uint16_t VirtualTerminalClientStateTracker::get_active_mask() const
	{
		return activeDataOrAlarmMask;
	}

	const std::deque<std::uint16_t> &VirtualTerminalClientStateTracker::get_mask_history() const
	{
		return dataAndAlarmMaskHistory;
	}

	std::size_t VirtualTerminalClientStateTracker::get_max_mask_history_size() const
	{
		return maxDataAndAlarmMaskHistorySize;
	}

	void VirtualTerminalClientStateTracker::set_max_mask_history_size(std::size_t size)
	{
		maxDataAndAlarmMaskHistorySize = size;
	}

	void VirtualTerminalClientStateTracker::add_tracked_soft_key_mask(std::uint16_t dataOrAlarmMaskId, std::uint16_t initialSoftKeyMaskId)
	{
		if (softKeyMasks.find(dataOrAlarmMaskId) != softKeyMasks.end())
		{
			CANStackLogger::warn("[VTStateHelper] add_tracked_soft_key_mask: data/alarm mask '%lu' already tracked", dataOrAlarmMaskId);
			return;
		}

		softKeyMasks[dataOrAlarmMaskId] = initialSoftKeyMaskId;
	}

	void VirtualTerminalClientStateTracker::remove_tracked_soft_key_mask(std::uint16_t dataOrAlarmMaskId)
	{
		if (softKeyMasks.find(dataOrAlarmMaskId) == softKeyMasks.end())
		{
			CANStackLogger::warn("[VTStateHelper] remove_tracked_soft_key_mask: data/alarm mask '%lu' was not tracked", dataOrAlarmMaskId);
			return;
		}

		softKeyMasks.erase(dataOrAlarmMaskId);
	}

	std::uint16_t VirtualTerminalClientStateTracker::get_active_soft_key_mask() const
	{
		if (softKeyMasks.find(activeDataOrAlarmMask) == softKeyMasks.end())
		{
			CANStackLogger::warn("[VTStateHelper] get_active_soft_key_mask: the currently active data/alarm mask '%lu' is not tracked", activeDataOrAlarmMask);
			return NULL_OBJECT_ID;
		}

		return softKeyMasks.at(activeDataOrAlarmMask);
	}

	std::uint16_t VirtualTerminalClientStateTracker::get_soft_key_mask(std::uint16_t dataOrAlarmMaskId) const
	{
		if (softKeyMasks.find(dataOrAlarmMaskId) == softKeyMasks.end())
		{
			CANStackLogger::warn("[VTStateHelper] get_soft_key_mask: data/alarm mask '%lu' is not tracked", activeDataOrAlarmMask);
			return NULL_OBJECT_ID;
		}

		return softKeyMasks.at(dataOrAlarmMaskId);
	}

	bool VirtualTerminalClientStateTracker::is_working_set_active() const
	{
		return (client != nullptr) && client->get_address_valid() && (client->get_address() == activeWorkingSetAddress);
	}

	void VirtualTerminalClientStateTracker::cache_active_mask(std::uint16_t maskId)
	{
		if (activeDataOrAlarmMask != maskId)
		{
			// Add the current active mask to the history if it is valid
			if (activeDataOrAlarmMask != NULL_OBJECT_ID)
			{
				dataAndAlarmMaskHistory.push_front(activeDataOrAlarmMask);
				if (dataAndAlarmMaskHistory.size() > maxDataAndAlarmMaskHistorySize)
				{
					dataAndAlarmMaskHistory.pop_back();
				}
			}
			// Update the active mask
			activeDataOrAlarmMask = maskId;
		}
	}

	void VirtualTerminalClientStateTracker::process_rx_or_tx_message(const CANMessage &message, void *parentPointer)
	{
		if ((!message.has_valid_source_control_function()) || (message.get_data_length() == 0))
		{
			// We are not interested in messages without a valid source control function or without data
			return;
		}

		auto *parent = static_cast<VirtualTerminalClientStateTracker *>(parentPointer);
		if (message.is_broadcast() &&
		    message.is_parameter_group_number(CANLibParameterGroupNumber::VirtualTerminalToECU) &&
		    (message.get_uint8_at(0) == static_cast<std::uint8_t>(VirtualTerminalClient::Function::VTStatusMessage)))
		{
			parent->process_status_message(message);
		}
		if (message.is_source(parent->server) && (!message.is_broadcast()) && message.is_parameter_group_number(CANLibParameterGroupNumber::VirtualTerminalToECU))
		{
			parent->process_message_from_connected_server(message);
		}
		else if (message.is_destination(parent->server) && (!message.is_broadcast()) && message.is_parameter_group_number(CANLibParameterGroupNumber::ECUtoVirtualTerminal))
		{
			parent->process_message_to_connected_server(message);
		}
	}

	void VirtualTerminalClientStateTracker::process_status_message(const CANMessage &message)
	{
		if (CAN_DATA_LENGTH == message.get_data_length())
		{
			activeWorkingSetAddress = message.get_uint8_at(1);
			if (is_working_set_active())
			{
				server = message.get_source_control_function();
				cache_active_mask(message.get_uint16_at(2));
				if (softKeyMasks.find(activeDataOrAlarmMask) != softKeyMasks.end())
				{
					std::uint16_t softKeyMask = message.get_uint16_at(4);
					softKeyMasks[activeDataOrAlarmMask] = softKeyMask;
				}
			}
		}
	}

	void VirtualTerminalClientStateTracker::process_message_from_connected_server(const CANMessage &message)
	{
		std::uint8_t function = message.get_uint8_at(0);
		switch (function)
		{
			case static_cast<std::uint8_t>(VirtualTerminalClient::Function::VTStatusMessage):
			{
				server = message.get_source_control_function();
				activeWorkingSetAddress = message.get_uint8_at(1);
				if (is_working_set_active())
				{
					cache_active_mask(message.get_uint16_at(2));
					if (softKeyMasks.find(activeDataOrAlarmMask) != softKeyMasks.end())
					{
						std::uint16_t softKeyMask = message.get_uint16_at(4);
						softKeyMasks[activeDataOrAlarmMask] = softKeyMask;
					}
				}
			}
			break;

			case static_cast<std::uint8_t>(VirtualTerminalClient::Function::ChangeActiveMaskCommand):
			{
				auto errorCode = message.get_uint8_at(3);
				if (errorCode == 0)
				{
					cache_active_mask(message.get_uint16_at(1));
				}
			}
			break;

			case static_cast<std::uint8_t>(VirtualTerminalClient::Function::ChangeSoftKeyMaskCommand):
			{
				auto errorCode = message.get_uint8_at(3);
				if (errorCode == 0)
				{
					std::uint16_t associatedMask = message.get_uint16_at(1);
					std::uint16_t softKeyMask = message.get_uint16_at(4);
					if (softKeyMasks.find(associatedMask) != softKeyMasks.end())
					{
						softKeyMasks[associatedMask] = softKeyMask;
					}
				}
			}
			break;

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

	void VirtualTerminalClientStateTracker::process_message_to_connected_server(const CANMessage &message)
	{
		//! TODO: will be used for change attribute command
	}
} // namespace isobus
