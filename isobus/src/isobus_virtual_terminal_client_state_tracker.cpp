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
#include "isobus/utility/platform_endianness.hpp"

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
	}

	void VirtualTerminalClientStateTracker::terminate()
	{
		CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), process_rx_or_tx_message, this);
		CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ECUtoVirtualTerminal), process_rx_or_tx_message, this);
	}

	void VirtualTerminalClientStateTracker::add_tracked_container_shown(std::uint16_t objectId, bool initialValue)
	{
		if (shownStates.find(objectId) != shownStates.end())
		{
			LOG_WARNING("[VTStateHelper] add_tracked_container_shown: objectId '%hu' already tracked", objectId);
			return;
		}

		shownStates[objectId] = initialValue;
	}

	void VirtualTerminalClientStateTracker::remove_tracked_container_shown(std::uint16_t objectId)
	{
		if (shownStates.find(objectId) == shownStates.end())
		{
			LOG_WARNING("[VTStateHelper] remove_tracked_container_shown: objectId '%hu' was not tracked", objectId);
			return;
		}

		shownStates.erase(objectId);
	}

	bool VirtualTerminalClientStateTracker::get_container_shown(std::uint16_t objectId) const
	{
		if (shownStates.find(objectId) == shownStates.end())
		{
			LOG_WARNING("[VTStateHelper] get_container_shown: objectId '%hu' not tracked", objectId);
			return 0;
		}

		return shownStates.at(objectId);
	}

	void VirtualTerminalClientStateTracker::add_tracked_numeric_value(std::uint16_t objectId, std::uint32_t initialValue)
	{
		if (numericValueStates.find(objectId) != numericValueStates.end())
		{
			LOG_WARNING("[VTStateHelper] add_tracked_numeric_value: objectId '%hu' already tracked", objectId);
			return;
		}

		numericValueStates[objectId] = initialValue;
	}

	void VirtualTerminalClientStateTracker::remove_tracked_numeric_value(std::uint16_t objectId)
	{
		if (numericValueStates.find(objectId) == numericValueStates.end())
		{
			LOG_WARNING("[VTStateHelper] remove_tracked_numeric_value: objectId '%hu' was not tracked", objectId);
			return;
		}

		numericValueStates.erase(objectId);
	}

	std::uint32_t VirtualTerminalClientStateTracker::get_numeric_value(std::uint16_t objectId) const
	{
		if (numericValueStates.find(objectId) == numericValueStates.end())
		{
			LOG_WARNING("[VTStateHelper] get_numeric_value: objectId '%hu' not tracked", objectId);
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
		dataAndAlarmMaskHistory.resize(std::min(size, dataAndAlarmMaskHistory.size()));
	}

	void VirtualTerminalClientStateTracker::add_tracked_soft_key_mask(std::uint16_t dataOrAlarmMaskId, std::uint16_t initialSoftKeyMaskId)
	{
		if (softKeyMasks.find(dataOrAlarmMaskId) != softKeyMasks.end())
		{
			LOG_WARNING("[VTStateHelper] add_tracked_soft_key_mask: data/alarm mask '%hu' already tracked", dataOrAlarmMaskId);
			return;
		}

		softKeyMasks[dataOrAlarmMaskId] = initialSoftKeyMaskId;
	}

	void VirtualTerminalClientStateTracker::remove_tracked_soft_key_mask(std::uint16_t dataOrAlarmMaskId)
	{
		if (softKeyMasks.find(dataOrAlarmMaskId) == softKeyMasks.end())
		{
			LOG_WARNING("[VTStateHelper] remove_tracked_soft_key_mask: data/alarm mask '%hu' was not tracked", dataOrAlarmMaskId);
			return;
		}

		softKeyMasks.erase(dataOrAlarmMaskId);
	}

	std::uint16_t VirtualTerminalClientStateTracker::get_active_soft_key_mask() const
	{
		if (softKeyMasks.find(activeDataOrAlarmMask) == softKeyMasks.end())
		{
			LOG_WARNING("[VTStateHelper] get_active_soft_key_mask: the currently active data/alarm mask '%hu' is not tracked", activeDataOrAlarmMask);
			return NULL_OBJECT_ID;
		}

		return softKeyMasks.at(activeDataOrAlarmMask);
	}

	std::uint16_t VirtualTerminalClientStateTracker::get_soft_key_mask(std::uint16_t dataOrAlarmMaskId) const
	{
		if (softKeyMasks.find(dataOrAlarmMaskId) == softKeyMasks.end())
		{
			LOG_WARNING("[VTStateHelper] get_soft_key_mask: data/alarm mask '%hu' is not tracked", dataOrAlarmMaskId);
			return NULL_OBJECT_ID;
		}

		return softKeyMasks.at(dataOrAlarmMaskId);
	}

	bool VirtualTerminalClientStateTracker::is_working_set_active() const
	{
		return (client != nullptr) && client->get_address_valid() && (client->get_address() == activeWorkingSetAddress);
	}

	void VirtualTerminalClientStateTracker::add_tracked_attribute(std::uint16_t objectId, std::uint8_t attribute, std::uint32_t initialValue)
	{
		if (attributeStates.find(objectId) == attributeStates.end())
		{
			attributeStates[objectId] = {};
		}

		auto &attributeMap = attributeStates.at(objectId);
		if (attributeMap.find(attribute) != attributeMap.end())
		{
			LOG_WARNING("[VTStateHelper] add_tracked_attribute: attribute '%hhu' of objectId '%hu' already tracked", attribute, objectId);
			return;
		}

		attributeMap[attribute] = initialValue;
	}

	void VirtualTerminalClientStateTracker::add_tracked_attribute(std::uint16_t objectId, std::uint8_t attribute, float initialValue)
	{
		return add_tracked_attribute(objectId, attribute, float_to_little_endian(initialValue));
	}

	void VirtualTerminalClientStateTracker::remove_tracked_attribute(std::uint16_t objectId, std::uint8_t attribute)
	{
		if (attributeStates.find(objectId) == attributeStates.end())
		{
			LOG_WARNING("[VTStateHelper] remove_tracked_attribute: objectId '%hu' was not tracked", objectId);
			return;
		}

		auto &attributeMap = attributeStates.at(objectId);
		if (attributeMap.find(attribute) == attributeMap.end())
		{
			LOG_WARNING("[VTStateHelper] remove_tracked_attribute: attribute '%hhu' of objectId '%hu' was not tracked", attribute, objectId);
			return;
		}

		attributeMap.erase(attribute);
	}

	std::uint32_t VirtualTerminalClientStateTracker::get_attribute(std::uint16_t objectId, std::uint8_t attribute) const
	{
		if (attributeStates.find(objectId) == attributeStates.end())
		{
			LOG_WARNING("[VTStateHelper] get_attribute: objectId '%hu' not tracked", objectId);
			return 0;
		}

		const auto &attributeMap = attributeStates.at(objectId);
		if (attributeMap.find(attribute) == attributeMap.end())
		{
			LOG_WARNING("[VTStateHelper] get_attribute: attribute '%hhu' of objectId '%hu' not tracked", attribute, objectId);
			return 0;
		}

		return attributeMap.at(attribute);
	}

	float VirtualTerminalClientStateTracker::get_attribute_as_float(std::uint16_t objectId, std::uint8_t attribute) const
	{
		return little_endian_to_float(get_attribute(objectId, attribute));
	}

	void VirtualTerminalClientStateTracker::cache_active_mask(std::uint16_t maskId)
	{
		if (activeDataOrAlarmMask != maskId)
		{
			if (activeDataOrAlarmMask != NULL_OBJECT_ID)
			{
				dataAndAlarmMaskHistory.push_front(activeDataOrAlarmMask);
				if (dataAndAlarmMaskHistory.size() > maxDataAndAlarmMaskHistorySize)
				{
					dataAndAlarmMaskHistory.pop_back();
				}
			}
			activeDataOrAlarmMask = maskId;
		}
	}

	void VirtualTerminalClientStateTracker::process_rx_or_tx_message(const CANMessage &message, void *parentPointer)
	{
		if ((!message.has_valid_source_control_function()) || (CAN_DATA_LENGTH != message.get_data_length()))
		{
			// ISO 11783-6 pads every message read here to 8 bytes
			return;
		}

		auto *parent = static_cast<VirtualTerminalClientStateTracker *>(parentPointer);
		if (message.is_broadcast() &&
		    message.is_parameter_group_number(CANLibParameterGroupNumber::VirtualTerminalToECU) &&
		    (message.get_uint8_at(0) == static_cast<std::uint8_t>(VirtualTerminalClient::Function::VTStatusMessage)))
		{
			parent->process_status_message(message);
		}
		if (message.is_source(parent->server) && message.is_destination(parent->client) && (!message.is_broadcast()) && message.is_parameter_group_number(CANLibParameterGroupNumber::VirtualTerminalToECU))
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
		const std::uint8_t workingSetAddress = message.get_uint8_at(1);
		const bool isNamingOurClient = (nullptr != client) &&
		  (client->get_can_port() == message.get_can_port_index()) &&
		  (client->get_address() == workingSetAddress);

		// Another VT takes over by naming our client
		if (message.is_source(server) || isNamingOurClient)
		{
			activeWorkingSetAddress = workingSetAddress;
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
				process_status_message(message);
			}
			break;

			case static_cast<std::uint8_t>(VirtualTerminalClient::Function::ChangeActiveMaskCommand):
			{
				auto errorCode = message.get_uint8_at(3);
				if (0 == errorCode)
				{
					cache_active_mask(message.get_uint16_at(1));
				}
			}
			break;

			case static_cast<std::uint8_t>(VirtualTerminalClient::Function::ChangeSoftKeyMaskCommand):
			{
				auto errorCode = message.get_uint8_at(5);
				if (0 == errorCode)
				{
					std::uint16_t associatedMask = message.get_uint16_at(1);
					std::uint16_t softKeyMask = message.get_uint16_at(3);
					if (softKeyMasks.find(associatedMask) != softKeyMasks.end())
					{
						softKeyMasks[associatedMask] = softKeyMask;
					}
				}
			}
			break;

			case static_cast<std::uint8_t>(VirtualTerminalClient::Function::ChangeNumericValueCommand):
			{
				auto errorCode = message.get_uint8_at(3);
				if (0 == errorCode)
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

			case static_cast<std::uint8_t>(VirtualTerminalClient::Function::VTChangeNumericValueMessage):
			{
				std::uint16_t objectId = message.get_uint16_at(1);
				if (numericValueStates.find(objectId) != numericValueStates.end())
				{
					std::uint32_t value = message.get_uint32_at(4);
					numericValueStates[objectId] = value;
				}
			}
			break;

			case static_cast<std::uint8_t>(VirtualTerminalClient::Function::ChangeAttributeCommand):
			{
				auto errorCode = message.get_uint8_at(4);
				if (0 == errorCode)
				{
					std::uint16_t objectId = message.get_uint16_at(1);
					std::uint8_t attribute = message.get_uint8_at(3);
					std::uint8_t error = message.get_uint8_at(4);

					if (pendingChangeAttributeCommands.find(message.get_destination_control_function()) != pendingChangeAttributeCommands.end())
					{
						const auto &pendingCommand = pendingChangeAttributeCommands.at(message.get_source_control_function());
						if ((pendingCommand.objectId == objectId) && (pendingCommand.attribute == attribute) && (0 == error))
						{
							std::uint32_t value = message.get_uint32_at(5);
							attributeStates[objectId][attribute] = value;
						}
						pendingChangeAttributeCommands.erase(message.get_destination_control_function());
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
		std::uint8_t function = message.get_uint8_at(0);
		switch (function)
		{
			case static_cast<std::uint8_t>(VirtualTerminalClient::Function::ChangeAttributeCommand):
			{
				std::uint16_t objectId = message.get_uint16_at(1);
				std::uint8_t attribute = message.get_uint8_at(3);

				if ((attributeStates.find(objectId) != attributeStates.end()) &&
				    (attributeStates.at(objectId).find(attribute) != attributeStates.at(objectId).end()))
				{
					std::uint32_t value = message.get_uint32_at(4);
					pendingChangeAttributeCommands[message.get_source_control_function()] = { value, objectId, attribute };
				}
			}
			break;

			default:
				break;
		}
	}
} // namespace isobus
