//================================================================================================
/// @file can_network_manager.cpp
///
/// @brief The main class that manages the ISOBUS stack including: callbacks, Name to Address
/// management, making control functions, and driving the various protocols.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#include "can_network_manager.hpp"
#include "can_hardware_abstraction.hpp"
#include "can_managed_message.hpp"
#include "can_general_parameter_group_numbers.hpp"
#include "can_protocol.hpp"
#include "can_message.hpp"
#include "can_constants.hpp"
#include "can_partnered_control_function.hpp"
#include "can_warning_logger.hpp"

#include <cstring>
#include <algorithm>
namespace isobus
{
	CANNetworkManager CANNetworkManager::CANNetwork;

	void CANNetworkManager::initialize()
	{
		receiveMessageList.clear();
		initialized = true;
	}

	ControlFunction *CANNetworkManager::get_control_function(std::uint8_t CANPort, std::uint8_t CFAddress, CANLibBadge<AddressClaimStateMachine>) const
	{
		return get_control_function(CANPort, CFAddress);
	}

	void CANNetworkManager::add_control_function(std::uint8_t CANPort, ControlFunction *newControlFunction, std::uint8_t CFAddress, CANLibBadge<AddressClaimStateMachine>)
	{
		if ((nullptr != newControlFunction) && (CFAddress < NULL_CAN_ADDRESS) && (CANPort < CAN_PORT_MAXIMUM))
		{
			controlFunctionTable[CANPort][CFAddress] = newControlFunction;
		}
	}

	void CANNetworkManager::add_global_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback)
	{
		globalParameterGroupNumberCallbacks.push_back(ParameterGroupNumberCallbackData(parameterGroupNumber, callback));
	}

    void CANNetworkManager::remove_global_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback)
	{
		ParameterGroupNumberCallbackData tempObject(parameterGroupNumber, callback);
		auto callbackLocation = std::find(globalParameterGroupNumberCallbacks.begin(), globalParameterGroupNumberCallbacks.end(), tempObject);
		if (globalParameterGroupNumberCallbacks.end() != callbackLocation)
		{
			globalParameterGroupNumberCallbacks.erase(callbackLocation);
		}
	}

	std::uint32_t CANNetworkManager::get_number_global_parameter_group_number_callbacks() const
	{
		return globalParameterGroupNumberCallbacks.size();
	}

	InternalControlFunction *CANNetworkManager::get_internal_control_function(ControlFunction *controlFunction)
	{
		InternalControlFunction *retVal = nullptr;

		if ((nullptr != controlFunction) && 
			(ControlFunction::Type::Internal == controlFunction->get_type()))
		{
			retVal = static_cast<InternalControlFunction *>(controlFunction);
		}
		return retVal;
	}

	bool CANNetworkManager::send_can_message(std::uint32_t parameterGroupNumber,
	                                         const std::uint8_t *dataBuffer,
	                                         std::uint32_t dataLength,
	                                         InternalControlFunction *sourceControlFunction,
	                                         ControlFunction *destinationControlFunction,
	                                         CANIdentifier::CANPriority priority)
	{
		bool retVal = false;

		if ((nullptr != dataBuffer) && (dataLength > 0) && (dataLength <= CANMessage::ABSOLUTE_MAX_MESSAGE_LENGTH) && (nullptr != sourceControlFunction) && ((parameterGroupNumber == static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim)) || (sourceControlFunction->get_address_valid())))
		{
			if (nullptr == destinationControlFunction)
			{
				// Todo move binding of dest address to hardware layer
				retVal = send_can_message_raw(sourceControlFunction->get_can_port(), sourceControlFunction->get_address(), 0xFF, parameterGroupNumber, priority, dataBuffer, dataLength);
			}
			else if (destinationControlFunction->get_address_valid())
			{
				retVal = send_can_message_raw(sourceControlFunction->get_can_port(), sourceControlFunction->get_address(), destinationControlFunction->get_address(), parameterGroupNumber, priority, dataBuffer, dataLength);
			}
		}
		return retVal;
	}

	void CANNetworkManager::receive_can_message(CANMessage message)
	{
		if (initialized)
		{
			receiveMessageMutex.lock();

			receiveMessageList.push_back(message);

			receiveMessageMutex.unlock();
		}
	}

	void CANNetworkManager::update()
	{
		if (!initialized)
		{
			initialize();
		}

		process_rx_messages();

		InternalControlFunction::update_address_claiming();

		for (uint32_t i = 0; i <  CANLibProtocol::get_number_protocols(); i++)
		{
			CANLibProtocol *currentProtocol = nullptr;

			if (CANLibProtocol::get_protocol(i, currentProtocol))
			{
				if (!currentProtocol->get_is_initialized())
				{
					currentProtocol->initialize({});
				}
				currentProtocol->update({});
			}
		}
	}

	bool CANNetworkManager::send_can_message_raw(std::uint32_t portIndex, std::uint8_t sourceAddress, std::uint8_t destAddress, std::uint32_t parameterGroupNumber, std::uint8_t priority, const void *data, std::uint32_t size, CANLibBadge<AddressClaimStateMachine>)
	{
		return send_can_message_raw(portIndex, sourceAddress, destAddress, parameterGroupNumber, priority, data, size);
	}

	ParameterGroupNumberCallbackData CANNetworkManager::get_global_parameter_group_number_callback(std::uint32_t index) const
	{
		ParameterGroupNumberCallbackData retVal(0, nullptr);

		if (index < get_number_global_parameter_group_number_callbacks())
		{
			retVal = globalParameterGroupNumberCallbacks[index];
		}
		return retVal;
	}

	void CANNetworkManager::can_lib_process_rx_message(HardwareInterfaceCANFrame &rxFrame, void *)
	{
		CANLibManagedMessage tempCANMessage(rxFrame.channel);

		CANNetworkManager::CANNetwork.update_control_functions(rxFrame);

		tempCANMessage.set_identifier(CANIdentifier(rxFrame.identifier));
		
		// Note, if this is an address claim message, the address to CF table might be stale.
		// We don't want to update that here though, as we're maybe in some other thread in this callback.
		// So for now, manually search all of them to line up the appropriate CF. A bit unfortunate in that we may have a lot of CFs, but saves pain later so we don't have to
		// do some gross cast to CANLibManagedMessage to edit the CFs.
		// At least address claiming should be infrequent, so this should not happen a ton.
		if (static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim) == tempCANMessage.get_identifier().get_parameter_group_number())
		{
			for (std::uint32_t i = 0; i < CANNetworkManager::CANNetwork.activeControlFunctions.size(); i++)
			{
				if ((CANNetworkManager::CANNetwork.activeControlFunctions[i]->get_can_port() == tempCANMessage.get_can_port_index()) &&
					(CANNetworkManager::CANNetwork.activeControlFunctions[i]->get_address() == tempCANMessage.get_identifier().get_source_address()))
				{
					tempCANMessage.set_source_control_function(CANNetworkManager::CANNetwork.activeControlFunctions[i]);
					break;
				}
			}
		}
		else
		{
			tempCANMessage.set_source_control_function(CANNetworkManager::CANNetwork.get_control_function(rxFrame.channel, tempCANMessage.get_identifier().get_source_address()));
			tempCANMessage.set_destination_control_function(CANNetworkManager::CANNetwork.get_control_function(rxFrame.channel, tempCANMessage.get_identifier().get_destination_address()));
		}
		tempCANMessage.set_data(rxFrame.data, rxFrame.dataLength);
		
		CANNetworkManager::CANNetwork.receive_can_message(tempCANMessage);
	}

	bool CANNetworkManager::add_protocol_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parentPointer)
	{
		bool retVal = false;
		CANLibProtocolPGNCallbackInfo callbackInfo;

		callbackInfo.callback = callback;
		callbackInfo.parent = parentPointer;
		callbackInfo.parameterGroupNumber = parameterGroupNumber;

		protocolPGNCallbacksMutex.lock();

		if ((nullptr != callback) && (protocolPGNCallbacks.end() == find(protocolPGNCallbacks.begin(), protocolPGNCallbacks.end(), callbackInfo)))
		{
			protocolPGNCallbacks.push_back(callbackInfo);
			retVal = true;
		}

		protocolPGNCallbacksMutex.unlock();

		return retVal;
	}

	bool CANNetworkManager::remove_protocol_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parentPointer)
	{
		bool retVal = false;
		CANLibProtocolPGNCallbackInfo callbackInfo;

		callbackInfo.callback = callback;
		callbackInfo.parent = parentPointer;
		callbackInfo.parameterGroupNumber = parameterGroupNumber;

		protocolPGNCallbacksMutex.lock();

		if (nullptr != callback)
		{
			std::list<CANLibProtocolPGNCallbackInfo>::iterator callbackLocation;
			callbackLocation = find(protocolPGNCallbacks.begin(), protocolPGNCallbacks.end(), callbackInfo);

			if (protocolPGNCallbacks.end() != callbackLocation)
			{
				protocolPGNCallbacks.erase(callbackLocation);
				retVal = true;
			}
		}

		protocolPGNCallbacksMutex.unlock();

		return retVal;
	}

	void CANNetworkManager::update_address_table(CANMessage &message)
	{
		std::uint8_t CANPort = message.get_can_port_index();

		if ((static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim) == message.get_identifier().get_parameter_group_number()) &&
			(CANPort < CAN_PORT_MAXIMUM))
		{
			std::uint8_t messageSourceAddress = message.get_identifier().get_source_address();

			if ((nullptr != controlFunctionTable[CANPort][messageSourceAddress]) &&
				(CANIdentifier::NULL_ADDRESS == controlFunctionTable[CANPort][messageSourceAddress]->get_address()))
			{
				// Someone is at that spot in the table, but their address was stolen
				// Need to evict them from the table
				controlFunctionTable[CANPort][messageSourceAddress] = nullptr;
			}

			// Now, check for either a free spot in the table or recent eviction and populate if needed
			if (nullptr == controlFunctionTable[CANPort][messageSourceAddress])
			{
				// Look through active CFs, maybe we've heard of this ECU before
				for (auto currentControlFunction : activeControlFunctions)
				{
					if (currentControlFunction->get_address() == messageSourceAddress)
					{
						// ECU has claimed since the last update, add it to the table
						controlFunctionTable[CANPort][messageSourceAddress] = currentControlFunction;
						break;
					}
				}
			}
		}
	}

	void CANNetworkManager::update_control_functions(HardwareInterfaceCANFrame &rxFrame)
	{
		if ((static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim) == CANIdentifier(rxFrame.identifier).get_parameter_group_number()) &&
			(CAN_DATA_LENGTH == rxFrame.dataLength))
		{
			std::uint64_t claimedNAME;
			ControlFunction *foundControlFunction = nullptr;

			// TODO: Endianness?
			claimedNAME = rxFrame.data[0];
			claimedNAME |= (8 << rxFrame.data[1]);
			claimedNAME |= (16 << rxFrame.data[2]);
			claimedNAME |= (24 << rxFrame.data[3]);
			claimedNAME |= (32 << rxFrame.data[4]);
			claimedNAME |= (40 << rxFrame.data[5]);
			claimedNAME |= (48 << rxFrame.data[6]);
			claimedNAME |= (56 << rxFrame.data[7]);

			for (std::uint32_t i = 0; i < activeControlFunctions.size(); i++)
			{
				if (claimedNAME ==  activeControlFunctions[i]->controlFunctionNAME.get_full_name())
				{
					// Device already in the active list
					foundControlFunction = activeControlFunctions[i];
				}
				else
				{
					// If this CF has the same address as the one claiming, we need set it to 0xFE (null address)
					activeControlFunctions[i]->address = CANIdentifier::NULL_ADDRESS;
				}
			}

			// Maybe it's in the inactive list (device reconnected)
			// Alwas have to iterate the list to check for duplicate addresses
			for (std::uint32_t i = 0; i < inactiveControlFunctions.size(); i++)
			{
				if (claimedNAME ==  inactiveControlFunctions[i]->controlFunctionNAME.get_full_name())
				{
					// Device already in the inactive list
					foundControlFunction = inactiveControlFunctions[i];
				}
				else
				{
					// If this CF has the same address as the one claiming, we need set it to 0xFE (null address)
					inactiveControlFunctions[i]->address = CANIdentifier::NULL_ADDRESS;
				}
			}

			if (nullptr == foundControlFunction)
			{
				// New device, need to start keeping track of it
				activeControlFunctions.push_back(new ControlFunction(NAME(claimedNAME), CANIdentifier(rxFrame.identifier).get_source_address(), rxFrame.channel));
				CANStackLogger::CAN_stack_log("NM: New Control function " + std::to_string(static_cast<int>(CANIdentifier(rxFrame.identifier).get_source_address())));
			}
			else
			{
				foundControlFunction->address = CANIdentifier(rxFrame.identifier).get_source_address();
			}
		}
	}

	HardwareInterfaceCANFrame CANNetworkManager::construct_frame(std::uint32_t portIndex,
	                                                             std::uint8_t sourceAddress,
	                                                             std::uint8_t destAddress,
	                                                             std::uint32_t parameterGroupNumber,
	                                                             std::uint8_t priority,
	                                                             const void *data,
	                                                             std::uint32_t size)
	{
		HardwareInterfaceCANFrame txFrame;
		txFrame.identifier = DEFAULT_IDENTIFIER;

		if ((NULL_CAN_ADDRESS != destAddress) && (priority <= static_cast<std::uint8_t>(CANIdentifier::CANPriority::PriorityLowest7)) && (size <= CAN_DATA_LENGTH) && (nullptr != data))
		{
			std::uint32_t identifier = 0;

			// Manually encode an identifier
			// Todo: If we bring in a CAN library we sould replace this with some class
			identifier |= ((priority & 0x07) << 26);
			identifier |= (sourceAddress & 0xFF);

			if (BROADCAST_CAN_ADDRESS == destAddress)
			{
				if ((parameterGroupNumber & 0xF000) >= 0xF000)
				{
					identifier |= ((parameterGroupNumber & 0x3FFFF) << 8);
				}
				else
				{
					identifier |= (destAddress << 8);
					identifier |= ((parameterGroupNumber & 0x3FF00) << 8);
				}
			}
			else
			{
				if ((parameterGroupNumber & 0xF000) < 0xF000)
				{
					identifier |= (destAddress << 8);
					identifier |= ((parameterGroupNumber & 0x3FF00) << 8);
				}
			}

			txFrame.channel = portIndex;
			memcpy((void *)txFrame.data, data, size);
			txFrame.dataLength = size;
			txFrame.isExtendedFrame = true;
			txFrame.identifier = identifier & 0x1FFFFFFF;
		}
		return txFrame;
	}

	bool CANNetworkManager::CANLibProtocolPGNCallbackInfo::operator==(const CANLibProtocolPGNCallbackInfo &obj)
	{
		return ((obj.callback == this->callback) &&
		        (obj.parent == this->parent) &&
		        (obj.parameterGroupNumber == this->parameterGroupNumber));
	}

	ControlFunction *CANNetworkManager::get_control_function(std::uint8_t CANPort, std::uint8_t CFAddress) const
	{
		ControlFunction *retVal = nullptr;

		if ((CFAddress < NULL_CAN_ADDRESS) && (CANPort < CAN_PORT_MAXIMUM))
		{
			retVal = controlFunctionTable[CANPort][CFAddress];
		}
		return retVal;
	}

	void CANNetworkManager::process_can_message_for_callbacks(CANMessage *message)
	{
		if (nullptr != message)
		{
			ControlFunction *messageDestination = message->get_destination_control_function();
			if ((nullptr == messageDestination) &&
				(nullptr != message->get_source_control_function()))
			{
				// Message destined to global
				for (std::uint32_t i = 0; i < get_number_global_parameter_group_number_callbacks(); i++)
				{
					if ((message->get_identifier().get_parameter_group_number() == get_global_parameter_group_number_callback(i).get_parameter_group_number()) &&
						(nullptr != get_global_parameter_group_number_callback(i).get_callback()))
					{
						// We have a callback that matches this PGN
						get_global_parameter_group_number_callback(i).get_callback()(message, nullptr);
					}
				}
			}
			else
			{
				// Message is destination specific
				for (std::uint32_t i = 0; i < InternalControlFunction::get_number_internal_control_functions(); i++)
				{
					if (messageDestination == InternalControlFunction::get_internal_control_function(i))
					{
						// Message is destined to us
						for (std::uint32_t j = 0; j < PartneredControlFunction::get_number_partnered_control_functions(); j++)
						{
							PartneredControlFunction *currentControlFunction = PartneredControlFunction::get_partnered_control_function(j);

							if ((nullptr != currentControlFunction) &&
								(currentControlFunction->get_can_port() == message->get_can_port_index()))
							{
								// Message matches CAN port for a partnered control function
								for (std::uint32_t k = 0; k < currentControlFunction->get_number_parameter_group_number_callbacks(); k++)
								{
									if ((message->get_identifier().get_parameter_group_number() == currentControlFunction->get_parameter_group_number_callback(k).get_parameter_group_number()) &&
										(nullptr != currentControlFunction->get_parameter_group_number_callback(k).get_callback()))
									{
										// We have a callback matching this message
										currentControlFunction->get_parameter_group_number_callback(k).get_callback()(message, nullptr);
									}
								}
							}
						}
					}
				}
			}
		}
	}

	void CANNetworkManager::process_rx_messages()
	{
		// Move this to a function
		receiveMessageMutex.lock();
		bool processNextMessage = (!receiveMessageList.empty());
		receiveMessageMutex.unlock();

		while (processNextMessage)
		{
			receiveMessageMutex.lock();
			CANMessage currentMessage = receiveMessageList.front();
			receiveMessageList.pop_front();
			processNextMessage = (!receiveMessageList.empty());
			receiveMessageMutex.unlock();

			update_address_table(currentMessage);

			// Update Protocols
			protocolPGNCallbacksMutex.lock();
			for (auto currentCallback : protocolPGNCallbacks)
			{
				if (currentCallback.parameterGroupNumber == currentMessage.get_identifier().get_parameter_group_number())
				{
					currentCallback.callback(&currentMessage, currentCallback.parent);
				}
			}
			protocolPGNCallbacksMutex.unlock();

			// Update Others
			process_can_message_for_callbacks(&currentMessage);
		}
	}

	bool CANNetworkManager::send_can_message_raw(std::uint32_t portIndex, std::uint8_t sourceAddress, std::uint8_t destAddress, std::uint32_t parameterGroupNumber, std::uint8_t priority, const void *data, std::uint32_t size)
	{
		HardwareInterfaceCANFrame tempFrame = construct_frame(portIndex, sourceAddress, destAddress, parameterGroupNumber, priority, data, size);
		bool retVal = false;

		if ((DEFAULT_IDENTIFIER != tempFrame.identifier) &&
		    (portIndex < CAN_PORT_MAXIMUM))
		{
			retVal = send_can_message_to_hardware(tempFrame);
		}
		return retVal;
	}

	void CANNetworkManager::protocol_message_callback(CANMessage *protocolMessage)
	{
		process_can_message_for_callbacks(protocolMessage);
	}

} // namespace isobus
