#include "can_network_manager.hpp"
#include "can_hardware_abstraction.hpp"
#include "can_lib_managed_message.hpp"
#include "can_lib_parameter_group_numbers.hpp"
#include "can_lib_protocol.hpp"
#include "can_message.hpp"
#include "can_types.hpp"
#include "can_partnered_control_function.hpp"

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

		InternalControlFunction::update_address_claiming();
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
		tempCANMessage.set_source_control_function(CANNetworkManager::CANNetwork.get_control_function(rxFrame.channel, tempCANMessage.get_identifier().get_source_address()));
		tempCANMessage.set_destination_control_function(CANNetworkManager::CANNetwork.get_control_function(rxFrame.channel, tempCANMessage.get_identifier().get_destination_address()));
		tempCANMessage.set_data(rxFrame.data);

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
		if ((static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim) == message.get_identifier().get_parameter_group_number()) &&
			(message.get_can_port_index() < CAN_PORT_MAXIMUM))
		{
			if (nullptr == controlFunctionTable[message.get_can_port_index()][message.get_identifier().get_source_address()])
			{
				// Look through active CFs? 
				for (auto currentControlFunction : activeControlFunctions)
				{
					
				}
				controlFunctionTable[message.get_can_port_index()][message.get_source_control_function()->address] = message.get_source_control_function();
			}
		}
	}

	void CANNetworkManager::update_control_functions(HardwareInterfaceCANFrame &rxFrame)
	{
		if ((static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim) == CANIdentifier(rxFrame.identifier).get_parameter_group_number()) &&
			(CAN_DATA_LENGTH == rxFrame.dataLength))
		{
			std::uint64_t claimedNAME;
			bool found = false;

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
					found = true;
					break;
				}
			}

			if (!found)
			{
				// Not active, maybe it's in the inactive list (device reconnected)
				for (std::uint32_t i = 0; i < inactiveControlFunctions.size(); i++)
				{
					if (claimedNAME ==  inactiveControlFunctions[i]->controlFunctionNAME.get_full_name())
					{
						// Device already in the inactive list
						found = true;
						break;
					}
				}
			}

			if (!found)
			{
				// New device, need to start keeping track of it
				activeControlFunctions.push_back(new ControlFunction(NAME(claimedNAME), CANIdentifier(rxFrame.identifier).get_source_address(), rxFrame.channel));
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
			ControlFunction *messageDestination = currentMessage.get_destination_control_function();
			if (nullptr == messageDestination)
			{
				// Message destined to global
				for (std::uint32_t i = 0; i < get_number_global_parameter_group_number_callbacks(); i++)
				{
					if ((currentMessage.get_identifier().get_parameter_group_number() == get_global_parameter_group_number_callback(i).get_parameter_group_number()) &&
						(nullptr != get_global_parameter_group_number_callback(i).get_callback()))
					{
						// We have a callback that matches this PGN
						get_global_parameter_group_number_callback(i).get_callback()(&currentMessage, nullptr);
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
								(currentControlFunction->get_can_port() == currentMessage.get_can_port_index()))
							{
								// Message matches CAN port for a partnered control function
								for (std::uint32_t k = 0; k < currentControlFunction->get_number_parameter_group_number_callbacks(); k++)
								{
									if ((currentMessage.get_identifier().get_parameter_group_number() == currentControlFunction->get_parameter_group_number_callback(k).get_parameter_group_number()) &&
										(nullptr != currentControlFunction->get_parameter_group_number_callback(k).get_callback()))
									{
										// We have a callback matching this message

									}
								}
							}
						}
					}
				}
			}
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

} // namespace isobus
