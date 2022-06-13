#include "can_network_manager.hpp"
#include "can_hardware_abstraction.hpp"
#include "can_lib_managed_message.hpp"
#include "can_lib_parameter_group_numbers.hpp"
#include "can_lib_protocol.hpp"
#include "can_message.hpp"
#include "can_types.hpp"

#include <cstring>

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

		InternalControlFunction::update_address_claiming();
	}

	bool CANNetworkManager::send_can_message_raw(std::uint32_t portIndex, std::uint8_t sourceAddress, std::uint8_t destAddress, std::uint32_t parameterGroupNumber, std::uint8_t priority, const void *data, std::uint32_t size, CANLibBadge<AddressClaimStateMachine>)
	{
		return send_can_message_raw(portIndex, sourceAddress, destAddress, parameterGroupNumber, priority, data, size);
	}

	void CANNetworkManager::can_lib_process_rx_message(HardwareInterfaceCANFrame &rxFrame, void *)
	{
		CANLibManagedMessage tempCANMessage(rxFrame.channel);

		tempCANMessage.set_identifier(CANIdentifier(rxFrame.identifier));
		tempCANMessage.set_source_control_function(CANNetworkManager::CANNetwork.get_control_function(rxFrame.channel, tempCANMessage.get_identifier().get_source_address()));
		tempCANMessage.set_destination_control_function(CANNetworkManager::CANNetwork.get_control_function(rxFrame.channel, tempCANMessage.get_identifier().get_destination_address()));

		CANNetworkManager::CANNetwork.receive_can_message(tempCANMessage);
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
		receiveMessageMutex.lock();

		while (receiveMessageList.size() > 0)
		{
			ControlFunction *messageDestination = receiveMessageList.front().get_destination_control_function();

			if (nullptr == messageDestination)
			{
				// Message destined to global
			}
			else
			{
				// Message is destination specific
				for (std::uint32_t i = 0; i < InternalControlFunction::get_number_internal_control_functions(); i++)
				{
					if (messageDestination == InternalControlFunction::get_internal_control_function(i))
					{
					}
				}
			}
			receiveMessageList.pop_front();
		}

		receiveMessageMutex.unlock();
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
