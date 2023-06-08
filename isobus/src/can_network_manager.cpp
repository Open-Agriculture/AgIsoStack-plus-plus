//================================================================================================
/// @file can_network_manager.cpp
///
/// @brief The main class that manages the ISOBUS stack including: callbacks, Name to Address
/// management, making control functions, and driving the various protocols.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"
#include "isobus/isobus/can_protocol.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <numeric>

namespace isobus
{
	CANNetworkManager CANNetworkManager::CANNetwork;

	void CANNetworkManager::initialize()
	{
		receiveMessageList.clear();
		initialized = true;
		transportProtocol.initialize({});
		extendedTransportProtocol.initialize({});
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

	void CANNetworkManager::add_global_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent)
	{
		globalParameterGroupNumberCallbacks.emplace_back(parameterGroupNumber, callback, parent, nullptr);
	}

	void CANNetworkManager::remove_global_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent)
	{
		ParameterGroupNumberCallbackData tempObject(parameterGroupNumber, callback, parent, nullptr);
		auto callbackLocation = std::find(globalParameterGroupNumberCallbacks.begin(), globalParameterGroupNumberCallbacks.end(), tempObject);
		if (globalParameterGroupNumberCallbacks.end() != callbackLocation)
		{
			globalParameterGroupNumberCallbacks.erase(callbackLocation);
		}
	}

	std::size_t CANNetworkManager::get_number_global_parameter_group_number_callbacks() const
	{
		return globalParameterGroupNumberCallbacks.size();
	}

	void CANNetworkManager::add_any_control_function_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent)
	{
		std::lock_guard<std::mutex> lock(anyControlFunctionCallbacksMutex);
		anyControlFunctionParameterGroupNumberCallbacks.emplace_back(parameterGroupNumber, callback, parent, nullptr);
	}

	void CANNetworkManager::remove_any_control_function_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent)
	{
		ParameterGroupNumberCallbackData tempObject(parameterGroupNumber, callback, parent, nullptr);
		std::lock_guard<std::mutex> lock(anyControlFunctionCallbacksMutex);
		auto callbackLocation = std::find(anyControlFunctionParameterGroupNumberCallbacks.begin(), anyControlFunctionParameterGroupNumberCallbacks.end(), tempObject);
		if (anyControlFunctionParameterGroupNumberCallbacks.end() != callbackLocation)
		{
			anyControlFunctionParameterGroupNumberCallbacks.erase(callbackLocation);
		}
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

	float CANNetworkManager::get_estimated_busload(std::uint8_t canChannel)
	{
		const std::lock_guard<std::mutex> lock(busloadUpdateMutex);
		constexpr float ISOBUS_BAUD_RATE_BPS = 250000.0f;
		float retVal = 0.0f;

		if (canChannel < CAN_PORT_MAXIMUM)
		{
			float totalTimeInAccumulatorWindow = (busloadMessageBitsHistory.at(canChannel).size() * BUSLOAD_UPDATE_FREQUENCY_MS) / 1000.0f;
			std::uint32_t totalBitCount = std::accumulate(busloadMessageBitsHistory.at(canChannel).begin(), busloadMessageBitsHistory.at(canChannel).end(), 0);
			retVal = (0 != totalTimeInAccumulatorWindow) ? ((totalBitCount / (totalTimeInAccumulatorWindow * ISOBUS_BAUD_RATE_BPS)) * 100.0f) : 0.0f;
		}
		return retVal;
	}

	bool CANNetworkManager::send_can_message(std::uint32_t parameterGroupNumber,
	                                         const std::uint8_t *dataBuffer,
	                                         std::uint32_t dataLength,
	                                         InternalControlFunction *sourceControlFunction,
	                                         ControlFunction *destinationControlFunction,
	                                         CANIdentifier::CANPriority priority,
	                                         TransmitCompleteCallback transmitCompleteCallback,
	                                         void *parentPointer,
	                                         DataChunkCallback frameChunkCallback)
	{
		bool retVal = false;

		if (((nullptr != dataBuffer) ||
		     (nullptr != frameChunkCallback)) &&
		    (dataLength > 0) &&
		    (dataLength <= CANMessage::ABSOLUTE_MAX_MESSAGE_LENGTH) &&
		    (nullptr != sourceControlFunction) &&
		    ((parameterGroupNumber == static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim)) ||
		     (sourceControlFunction->get_address_valid())))
		{
			CANLibProtocol *currentProtocol;

			// See if any transport layer protocol can handle this message
			for (std::uint32_t i = 0; i < CANLibProtocol::get_number_protocols(); i++)
			{
				if (CANLibProtocol::get_protocol(i, currentProtocol))
				{
					retVal = currentProtocol->protocol_transmit_message(parameterGroupNumber,
					                                                    dataBuffer,
					                                                    dataLength,
					                                                    sourceControlFunction,
					                                                    destinationControlFunction,
					                                                    transmitCompleteCallback,
					                                                    parentPointer,
					                                                    frameChunkCallback);

					if (retVal)
					{
						break;
					}
				}
			}

			//! @todo Allow sending 8 byte message with the frameChunkCallback
			if ((!retVal) &&
			    (nullptr != dataBuffer))
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

				if ((retVal) &&
				    (nullptr != transmitCompleteCallback))
				{
					// Message was not sent via a protocol, so handle the tx callback now
					transmitCompleteCallback(parameterGroupNumber, dataLength, sourceControlFunction, destinationControlFunction, retVal, parentPointer);
				}
			}
		}
		return retVal;
	}

	void CANNetworkManager::receive_can_message(const CANMessage &message)
	{
		if (initialized)
		{
			std::lock_guard<std::mutex> lock(receiveMessageMutex);

			receiveMessageList.push_back(message);
		}
	}

	void CANNetworkManager::update()
	{
		const std::lock_guard<std::mutex> lock(ControlFunction::controlFunctionProcessingMutex);

		if (!initialized)
		{
			initialize();
		}

		update_new_partners();

		process_rx_messages();

		InternalControlFunction::update_address_claiming({});

		if (InternalControlFunction::get_any_internal_control_function_changed_address({}))
		{
			for (std::size_t i = 0; i < InternalControlFunction::get_number_internal_control_functions(); i++)
			{
				InternalControlFunction *currentInternalControlFunction = InternalControlFunction::get_internal_control_function(i);

				if (nullptr != currentInternalControlFunction)
				{
					if (activeControlFunctions.end() == std::find(activeControlFunctions.begin(), activeControlFunctions.end(), currentInternalControlFunction))
					{
						activeControlFunctions.push_back(currentInternalControlFunction);
					}
					if (currentInternalControlFunction->get_changed_address_since_last_update({}))
					{
						update_address_table(currentInternalControlFunction->get_can_port(), currentInternalControlFunction->get_address());
					}
				}
			}
		}

		prune_inactive_control_functions();

		for (std::size_t i = 0; i < CANLibProtocol::get_number_protocols(); i++)
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
		update_busload_history();
		updateTimestamp_ms = SystemTiming::get_timestamp_ms();
	}

	bool CANNetworkManager::send_can_message_raw(std::uint32_t portIndex,
	                                             std::uint8_t sourceAddress,
	                                             std::uint8_t destAddress,
	                                             std::uint32_t parameterGroupNumber,
	                                             std::uint8_t priority,
	                                             const void *data,
	                                             std::uint32_t size,
	                                             CANLibBadge<AddressClaimStateMachine>) const
	{
		return send_can_message_raw(portIndex, sourceAddress, destAddress, parameterGroupNumber, priority, data, size);
	}

	ParameterGroupNumberCallbackData CANNetworkManager::get_global_parameter_group_number_callback(std::uint32_t index) const
	{
		ParameterGroupNumberCallbackData retVal(0, nullptr, nullptr, nullptr);

		if (index < get_number_global_parameter_group_number_callbacks())
		{
			retVal = globalParameterGroupNumberCallbacks[index];
		}
		return retVal;
	}

	void receive_can_message_frame_from_hardware(const CANMessageFrame &rxFrame)
	{
		CANNetworkManager::process_receive_can_message_frame(rxFrame);
	}

	void on_transmit_can_message_frame_from_hardware(const CANMessageFrame &txFrame)
	{
		CANNetworkManager::process_transmitted_can_message_frame(txFrame);
	}

	void periodic_update_from_hardware()
	{
		CANNetworkManager::CANNetwork.update();
	}

	void CANNetworkManager::process_receive_can_message_frame(const CANMessageFrame &rxFrame)
	{
		CANMessage tempCANMessage(rxFrame.channel);

		CANNetworkManager::CANNetwork.update_control_functions(rxFrame);

		tempCANMessage.set_identifier(CANIdentifier(rxFrame.identifier));

		// Note, if this is an address claim message, the address to CF table might be stale.
		// We don't want to update that here though, as we're maybe in some other thread in this callback.
		// So for now, manually search all of them to line up the appropriate CF. A bit unfortunate in that we may have a lot of CFs, but saves pain later so we don't have to
		// do some gross cast to CANMessage to edit the CFs.
		// At least address claiming should be infrequent, so this should not happen a ton.
		if (static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim) == tempCANMessage.get_identifier().get_parameter_group_number())
		{
			for (auto i = CANNetworkManager::CANNetwork.activeControlFunctions.begin(); i != CANNetworkManager::CANNetwork.activeControlFunctions.end(); i++)
			{
				if (((*i)->get_can_port() == tempCANMessage.get_can_port_index()) &&
				    ((*i)->get_address() == tempCANMessage.get_identifier().get_source_address()))
				{
					tempCANMessage.set_source_control_function(*i);
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

		CANNetworkManager::CANNetwork.update_busload(rxFrame.channel, rxFrame.get_number_bits_in_message());

		CANNetworkManager::CANNetwork.receive_can_message(tempCANMessage);
	}

	void CANNetworkManager::process_transmitted_can_message_frame(const CANMessageFrame &txFrame)
	{
		CANNetworkManager::CANNetwork.update_busload(txFrame.channel, txFrame.get_number_bits_in_message());
	}

	void CANNetworkManager::on_partner_deleted(PartneredControlFunction *partner, CANLibBadge<PartneredControlFunction>)
	{
		CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Debug, "[NM]: Partner " + isobus::to_string(static_cast<int>(partner->get_address())) + " was deleted.");

		for (auto activeControlFunction = activeControlFunctions.begin(); activeControlFunction != activeControlFunctions.end(); activeControlFunction++)
		{
			if ((partner->get_can_port() == (*activeControlFunction)->get_can_port()) &&
			    (partner->get_NAME() == (*activeControlFunction)->get_NAME()))
			{
				(*activeControlFunction) = nullptr;
				activeControlFunctions.erase(activeControlFunction);
				if (partner->address < NULL_CAN_ADDRESS)
				{
					controlFunctionTable[partner->get_can_port()][partner->address] = nullptr;
					// If the control function was active, replace it with an external control function
					activeControlFunctions.push_back(new ControlFunction(partner->get_NAME(), partner->get_address(), partner->get_can_port()));
					CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Debug, "[NM]: Since the deleted partner was active, it has been replaced with an external control function.");
				}
				break;
			}
		}
		for (auto inactiveControlFunction = inactiveControlFunctions.begin(); inactiveControlFunction != inactiveControlFunctions.end(); inactiveControlFunction++)
		{
			if ((partner->get_can_port() == (*inactiveControlFunction)->get_can_port()) &&
			    (partner->get_NAME() == (*inactiveControlFunction)->get_NAME()))
			{
				(*inactiveControlFunction) = nullptr;
				inactiveControlFunctions.erase(inactiveControlFunction);
				break;
			}
		}
	}

	FastPacketProtocol &CANNetworkManager::get_fast_packet_protocol()
	{
		return fastPacketProtocol;
	}

	bool CANNetworkManager::add_protocol_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parentPointer)
	{
		bool retVal = false;
		ParameterGroupNumberCallbackData callbackInfo(parameterGroupNumber, callback, parentPointer, nullptr);

		const std::lock_guard<std::mutex> lock(protocolPGNCallbacksMutex);

		if ((nullptr != callback) && (protocolPGNCallbacks.end() == find(protocolPGNCallbacks.begin(), protocolPGNCallbacks.end(), callbackInfo)))
		{
			protocolPGNCallbacks.push_back(callbackInfo);
			retVal = true;
		}
		return retVal;
	}

	bool CANNetworkManager::remove_protocol_parameter_group_number_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parentPointer)
	{
		bool retVal = false;
		ParameterGroupNumberCallbackData callbackInfo(parameterGroupNumber, callback, parentPointer, nullptr);

		const std::lock_guard<std::mutex> lock(protocolPGNCallbacksMutex);

		if (nullptr != callback)
		{
			std::list<ParameterGroupNumberCallbackData>::iterator callbackLocation;
			callbackLocation = find(protocolPGNCallbacks.begin(), protocolPGNCallbacks.end(), callbackInfo);

			if (protocolPGNCallbacks.end() != callbackLocation)
			{
				protocolPGNCallbacks.erase(callbackLocation);
				retVal = true;
			}
		}
		return retVal;
	}

	CANNetworkManager::CANNetworkManager()
	{
		currentBusloadBitAccumulator.fill(0);
		lastAddressClaimRequestTimestamp_ms.fill(0);
		controlFunctionTable.fill({ nullptr });
	}

	void CANNetworkManager::update_address_table(const CANMessage &message)
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
				controlFunctionTable[CANPort][messageSourceAddress]->address = NULL_CAN_ADDRESS;
				controlFunctionTable[CANPort][messageSourceAddress] = nullptr;
			}

			// Now, check for either a free spot in the table or recent eviction and populate if needed
			if (nullptr == controlFunctionTable[CANPort][messageSourceAddress])
			{
				// Look through active CFs, maybe we've heard of this ECU before
				for (auto &currentControlFunction : activeControlFunctions)
				{
					if (currentControlFunction->get_address() == messageSourceAddress)
					{
						// Scan the address table to remove this CF from a previous location if needed
						for (std::uint_fast8_t i = 0; i < NULL_CAN_ADDRESS; i++)
						{
							if ((nullptr != controlFunctionTable[CANPort][i]) &&
							    (controlFunctionTable[CANPort][i]->get_NAME() == currentControlFunction->get_NAME()) &&
							    (i != messageSourceAddress))
							{
								controlFunctionTable[CANPort][i] = nullptr;
								CANStackLogger::debug("[NM]: Detected that CF %016llx on channel %u is moving from address %u to %u.",
								                      currentControlFunction->get_NAME().get_full_name(),
								                      CANPort,
								                      i,
								                      messageSourceAddress);
								break;
							}
						}

						// ECU has claimed since the last update, add it to the table
						controlFunctionTable[CANPort][messageSourceAddress] = currentControlFunction;
						currentControlFunction->address = messageSourceAddress;
						currentControlFunction->claimedAddressSinceLastAddressClaimRequest = true;
						break;
					}
				}
			}
			else
			{
				controlFunctionTable[CANPort][messageSourceAddress]->claimedAddressSinceLastAddressClaimRequest = true;
			}
		}
		else if ((static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest) == message.get_identifier().get_parameter_group_number()) &&
		         (CANPort < CAN_PORT_MAXIMUM))
		{
			auto requestedPGN = message.get_uint24_at(0);

			if (static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim) == requestedPGN)
			{
				lastAddressClaimRequestTimestamp_ms.at(CANPort) = SystemTiming::get_timestamp_ms();

				for (auto &activeControlFunction : activeControlFunctions)
				{
					if (CANPort == activeControlFunction->get_can_port())
					{
						activeControlFunction->claimedAddressSinceLastAddressClaimRequest = false;
					}
				}
			}
		}
	}

	void CANNetworkManager::update_address_table(std::uint8_t CANPort, std::uint8_t claimedAddress)
	{
		if (CANPort < CAN_PORT_MAXIMUM)
		{
			if (nullptr != controlFunctionTable[CANPort][claimedAddress])
			{
				// Someone is at that spot in the table, but their address was stolen by an internal control function
				// Need to evict them from the table
				controlFunctionTable[CANPort][claimedAddress]->address = NULL_CAN_ADDRESS;
				controlFunctionTable[CANPort][claimedAddress] = nullptr;
			}

			// Now, check for either a free spot in the table or recent eviction and populate if needed
			if (nullptr == controlFunctionTable[CANPort][claimedAddress])
			{
				// Look through active CFs, maybe we've heard of this ECU before
				for (auto currentControlFunction : activeControlFunctions)
				{
					if (currentControlFunction->get_address() == claimedAddress)
					{
						// ECU has claimed since the last update, add it to the table
						controlFunctionTable[CANPort][claimedAddress] = currentControlFunction;
						currentControlFunction->address = claimedAddress;
						break;
					}
				}
			}
		}
	}

	void CANNetworkManager::update_busload(std::uint8_t channelIndex, std::uint32_t numberOfBitsProcessed)
	{
		const std::lock_guard<std::mutex> lock(CANNetworkManager::CANNetwork.busloadUpdateMutex);

		currentBusloadBitAccumulator.at(channelIndex) += numberOfBitsProcessed;
	}

	void CANNetworkManager::update_busload_history()
	{
		const std::lock_guard<std::mutex> lock(busloadUpdateMutex);

		if (SystemTiming::time_expired_ms(busloadUpdateTimestamp_ms, BUSLOAD_UPDATE_FREQUENCY_MS))
		{
			for (std::size_t i = 0; i < busloadMessageBitsHistory.size(); i++)
			{
				busloadMessageBitsHistory.at(i).push_back(currentBusloadBitAccumulator.at(i));

				while (busloadMessageBitsHistory.at(i).size() > (BUSLOAD_SAMPLE_WINDOW_MS / BUSLOAD_UPDATE_FREQUENCY_MS))
				{
					busloadMessageBitsHistory.at(i).pop_front();
				}
				currentBusloadBitAccumulator.at(i) = 0;
			}
			busloadUpdateTimestamp_ms = SystemTiming::get_timestamp_ms();
		}
	}

	void CANNetworkManager::update_control_functions(const CANMessageFrame &rxFrame)
	{
		if ((static_cast<std::uint32_t>(CANLibParameterGroupNumber::AddressClaim) == CANIdentifier(rxFrame.identifier).get_parameter_group_number()) &&
		    (CAN_DATA_LENGTH == rxFrame.dataLength) &&
		    (rxFrame.channel < CAN_PORT_MAXIMUM))
		{
			std::uint64_t claimedNAME;
			ControlFunction *foundControlFunction = nullptr;

			claimedNAME = rxFrame.data[0];
			claimedNAME |= (static_cast<std::uint64_t>(rxFrame.data[1]) << 8);
			claimedNAME |= (static_cast<std::uint64_t>(rxFrame.data[2]) << 16);
			claimedNAME |= (static_cast<std::uint64_t>(rxFrame.data[3]) << 24);
			claimedNAME |= (static_cast<std::uint64_t>(rxFrame.data[4]) << 32);
			claimedNAME |= (static_cast<std::uint64_t>(rxFrame.data[5]) << 40);
			claimedNAME |= (static_cast<std::uint64_t>(rxFrame.data[6]) << 48);
			claimedNAME |= (static_cast<std::uint64_t>(rxFrame.data[7]) << 56);

			for (auto i = activeControlFunctions.begin(); i != activeControlFunctions.end(); i++)
			{
				if ((claimedNAME == (*i)->controlFunctionNAME.get_full_name()) &&
				    (rxFrame.channel == (*i)->get_can_port()))
				{
					// Device already in the active list
					foundControlFunction = (*i);
					break;
				}
				else if (((*i)->address == CANIdentifier(rxFrame.identifier).get_source_address()) &&
				         (rxFrame.channel == (*i)->get_can_port()))
				{
					// If this CF has the same address as the one claiming, we need set it to 0xFE (null address)
					(*i)->address = CANIdentifier::NULL_ADDRESS;
				}
			}

			// Maybe it's in the inactive list (device reconnected)
			// Always have to iterate the list to check for duplicate addresses
			for (std::size_t i = 0; i < inactiveControlFunctions.size(); i++)
			{
				if ((claimedNAME == inactiveControlFunctions[i]->controlFunctionNAME.get_full_name()) &&
				    (rxFrame.channel == inactiveControlFunctions[i]->get_can_port()))
				{
					// Device already in the inactive list
					foundControlFunction = inactiveControlFunctions[i];
					break;
				}
				else if (rxFrame.channel == inactiveControlFunctions[i]->get_can_port())
				{
					// If this CF has the same address as the one claiming, we need set it to 0xFE (null address)
					inactiveControlFunctions[i]->address = CANIdentifier::NULL_ADDRESS;
				}
			}

			if (nullptr == foundControlFunction)
			{
				// If we still haven't found it, it might be a partner. Check the list of partners.
				for (std::size_t i = 0; i < PartneredControlFunction::partneredControlFunctionList.size(); i++)
				{
					if ((nullptr != PartneredControlFunction::partneredControlFunctionList[i]) &&
					    (PartneredControlFunction::partneredControlFunctionList[i]->get_can_port() == rxFrame.channel) &&
					    (PartneredControlFunction::partneredControlFunctionList[i]->check_matches_name(NAME(claimedNAME))))
					{
						PartneredControlFunction::partneredControlFunctionList[i]->address = CANIdentifier(rxFrame.identifier).get_source_address();
						PartneredControlFunction::partneredControlFunctionList[i]->controlFunctionNAME = NAME(claimedNAME);
						activeControlFunctions.push_back(PartneredControlFunction::partneredControlFunctionList[i]);
						foundControlFunction = PartneredControlFunction::partneredControlFunctionList[i];
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Debug, "[NM]: A Partner Has Claimed " + isobus::to_string(static_cast<int>(CANIdentifier(rxFrame.identifier).get_source_address())));
						break;
					}
				}

				if (nullptr == foundControlFunction)
				{
					// New device, need to start keeping track of it
					activeControlFunctions.push_back(new ControlFunction(NAME(claimedNAME), CANIdentifier(rxFrame.identifier).get_source_address(), rxFrame.channel));
					CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Debug, "[NM]: New Control function " + isobus::to_string(static_cast<int>(CANIdentifier(rxFrame.identifier).get_source_address())));
				}
			}

			if (nullptr != foundControlFunction)
			{
				foundControlFunction->address = CANIdentifier(rxFrame.identifier).get_source_address();
			}
		}
	}

	void CANNetworkManager::update_new_partners()
	{
		if (PartneredControlFunction::anyPartnerNeedsInitializing)
		{
			for (auto &partner : PartneredControlFunction::partneredControlFunctionList)
			{
				if ((nullptr != partner) && (!partner->initialized))
				{
					bool foundReplaceableControlFunction = false;

					// Check this partner against the existing CFs
					for (auto currentInactiveControlFunction = inactiveControlFunctions.begin(); currentInactiveControlFunction != inactiveControlFunctions.end(); currentInactiveControlFunction++)
					{
						if ((partner->check_matches_name((*currentInactiveControlFunction)->get_NAME())) &&
						    (partner->get_can_port() == (*currentInactiveControlFunction)->get_can_port()) &&
						    (ControlFunction::Type::External == (*currentInactiveControlFunction)->get_type()))
						{
							foundReplaceableControlFunction = true;

							// This CF matches the filter and is not an internal or already partnered CF
							CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Debug, "[NM]: Remapping new partner control function to inactive external control function at address " + isobus::to_string(static_cast<int>((*currentInactiveControlFunction)->get_address())));

							// Populate the partner's data
							partner->address = (*currentInactiveControlFunction)->get_address();
							partner->controlFunctionNAME = (*currentInactiveControlFunction)->get_NAME();
							partner->initialized = true;
							inactiveControlFunctions.erase(currentInactiveControlFunction);
							break;
						}
					}

					if (!foundReplaceableControlFunction)
					{
						for (auto currentActiveControlFunction = activeControlFunctions.begin(); currentActiveControlFunction != activeControlFunctions.end(); currentActiveControlFunction++)
						{
							if ((partner->check_matches_name((*currentActiveControlFunction)->get_NAME())) &&
							    (partner->get_can_port() == (*currentActiveControlFunction)->get_can_port()) &&
							    (ControlFunction::Type::External == (*currentActiveControlFunction)->get_type()))
							{
								// This CF matches the filter and is not an internal or already partnered CF
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Debug, "[NM]: Remapping new partner control function to an active external control function at address " + isobus::to_string(static_cast<int>((*currentActiveControlFunction)->get_address())));

								// Populate the partner's data
								partner->address = (*currentActiveControlFunction)->get_address();
								partner->controlFunctionNAME = (*currentActiveControlFunction)->get_NAME();
								partner->initialized = true;
								controlFunctionTable[partner->get_can_port()][partner->address] = partner;
								activeControlFunctions.erase(currentActiveControlFunction);
								activeControlFunctions.push_back(partner);
								break;
							}
						}
					}
					partner->initialized = true;
				}
			}
			PartneredControlFunction::anyPartnerNeedsInitializing = false;
		}
	}

	CANMessageFrame CANNetworkManager::construct_frame(std::uint32_t portIndex,
	                                                   std::uint8_t sourceAddress,
	                                                   std::uint8_t destAddress,
	                                                   std::uint32_t parameterGroupNumber,
	                                                   std::uint8_t priority,
	                                                   const void *data,
	                                                   std::uint32_t size) const
	{
		CANMessageFrame txFrame;
		txFrame.identifier = DEFAULT_IDENTIFIER;

		if ((NULL_CAN_ADDRESS != destAddress) && (priority <= static_cast<std::uint8_t>(CANIdentifier::CANPriority::PriorityLowest7)) && (size <= CAN_DATA_LENGTH) && (nullptr != data))
		{
			std::uint32_t identifier = 0;

			// Manually encode an identifier
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
				else
				{
					CANStackLogger::warn("[NM]: Cannot send a message with PGN " +
					                     isobus::to_string(static_cast<int>(parameterGroupNumber)) +
					                     " as a destination specific message. " +
					                     "Try resending it using nullptr as your destination control function.");
					identifier = DEFAULT_IDENTIFIER;
				}
			}

			if (DEFAULT_IDENTIFIER != identifier)
			{
				txFrame.channel = portIndex;
				memcpy(reinterpret_cast<void *>(txFrame.data), data, size);
				txFrame.dataLength = size;
				txFrame.isExtendedFrame = true;
				txFrame.identifier = identifier & 0x1FFFFFFF;
			}
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

	CANMessage CANNetworkManager::get_next_can_message_from_rx_queue()
	{
		std::lock_guard<std::mutex> lock(receiveMessageMutex);
		CANMessage retVal = receiveMessageList.front();
		receiveMessageList.pop_front();
		return retVal;
	}

	std::size_t CANNetworkManager::get_number_can_messages_in_rx_queue()
	{
		std::lock_guard<std::mutex> lock(receiveMessageMutex);
		return receiveMessageList.size();
	}

	void CANNetworkManager::process_any_control_function_pgn_callbacks(const CANMessage &currentMessage)
	{
		const std::lock_guard<std::mutex> lock(anyControlFunctionCallbacksMutex);
		for (const auto &currentCallback : anyControlFunctionParameterGroupNumberCallbacks)
		{
			if ((currentCallback.get_parameter_group_number() == currentMessage.get_identifier().get_parameter_group_number()) &&
			    ((nullptr == currentMessage.get_destination_control_function()) ||
			     (ControlFunction::Type::Internal == currentMessage.get_destination_control_function()->get_type())))
			{
				currentCallback.get_callback()(currentMessage, currentCallback.get_parent());
			}
		}
	}

	void CANNetworkManager::process_protocol_pgn_callbacks(const CANMessage &currentMessage)
	{
		const std::lock_guard<std::mutex> lock(protocolPGNCallbacksMutex);
		for (auto &currentCallback : protocolPGNCallbacks)
		{
			if (currentCallback.get_parameter_group_number() == currentMessage.get_identifier().get_parameter_group_number())
			{
				currentCallback.get_callback()(currentMessage, currentCallback.get_parent());
			}
		}
	}

	void CANNetworkManager::process_can_message_for_global_and_partner_callbacks(const CANMessage &message)
	{
		ControlFunction *messageDestination = message.get_destination_control_function();
		if ((nullptr == messageDestination) &&
		    ((nullptr != message.get_source_control_function()) ||
		     ((static_cast<std::uint32_t>(CANLibParameterGroupNumber::ParameterGroupNumberRequest) == message.get_identifier().get_parameter_group_number()) &&
		      (NULL_CAN_ADDRESS == message.get_identifier().get_source_address()))))
		{
			// Message destined to global
			for (std::size_t i = 0; i < get_number_global_parameter_group_number_callbacks(); i++)
			{
				if ((message.get_identifier().get_parameter_group_number() == get_global_parameter_group_number_callback(i).get_parameter_group_number()) &&
				    (nullptr != get_global_parameter_group_number_callback(i).get_callback()))
				{
					// We have a callback that matches this PGN
					get_global_parameter_group_number_callback(i).get_callback()(message, get_global_parameter_group_number_callback(i).get_parent());
				}
			}
		}
		else
		{
			// Message is destination specific
			for (std::size_t i = 0; i < InternalControlFunction::get_number_internal_control_functions(); i++)
			{
				if (messageDestination == InternalControlFunction::get_internal_control_function(i))
				{
					// Message is destined to us
					for (std::size_t j = 0; j < PartneredControlFunction::get_number_partnered_control_functions(); j++)
					{
						PartneredControlFunction *currentControlFunction = PartneredControlFunction::get_partnered_control_function(j);

						if ((nullptr != currentControlFunction) &&
						    (currentControlFunction->get_can_port() == message.get_can_port_index()))
						{
							// Message matches CAN port for a partnered control function
							for (std::size_t k = 0; k < currentControlFunction->get_number_parameter_group_number_callbacks(); k++)
							{
								if ((message.get_identifier().get_parameter_group_number() == currentControlFunction->get_parameter_group_number_callback(k).get_parameter_group_number()) &&
								    (nullptr != currentControlFunction->get_parameter_group_number_callback(k).get_callback()) &&
								    ((nullptr == currentControlFunction->get_parameter_group_number_callback(k).get_internal_control_function()) ||
								     (currentControlFunction->get_parameter_group_number_callback(k).get_internal_control_function()->get_address() == message.get_identifier().get_destination_address())))
								{
									// We have a callback matching this message
									currentControlFunction->get_parameter_group_number_callback(k).get_callback()(message, currentControlFunction->get_parameter_group_number_callback(k).get_parent());
								}
							}
						}
					}
				}
			}
		}
	}

	void CANNetworkManager::process_can_message_for_commanded_address(const CANMessage &message)
	{
		constexpr std::uint8_t COMMANDED_ADDRESS_LENGTH = 9;

		if ((nullptr == message.get_destination_control_function()) &&
		    (static_cast<std::uint32_t>(CANLibParameterGroupNumber::CommandedAddress) == message.get_identifier().get_parameter_group_number()) &&
		    (COMMANDED_ADDRESS_LENGTH == message.get_data_length()))
		{
			std::uint64_t targetNAME = message.get_uint64_at(0);

			for (std::size_t i = 0; i < InternalControlFunction::get_number_internal_control_functions(); i++)
			{
				// This is not a particularly efficient search, but this should be pretty rare
				auto currentICF = InternalControlFunction::get_internal_control_function(i);

				if ((nullptr != currentICF) &&
				    (message.get_can_port_index() == currentICF->get_can_port()) &&
				    (currentICF->get_NAME().get_full_name() == targetNAME))
				{
					currentICF->process_commanded_address(message.get_uint8_at(8), {});
				}
			}
		}
	}

	void CANNetworkManager::process_rx_messages()
	{
		while (0 != get_number_can_messages_in_rx_queue())
		{
			CANMessage currentMessage = get_next_can_message_from_rx_queue();

			update_address_table(currentMessage);

			// Update Special Callbacks, like protocols and non-cf specific ones
			process_protocol_pgn_callbacks(currentMessage);
			process_any_control_function_pgn_callbacks(currentMessage);

			// Update Others
			process_can_message_for_global_and_partner_callbacks(currentMessage);
		}
	}

	void CANNetworkManager::prune_inactive_control_functions()
	{
		for (std::uint_fast8_t i = 0; i < CAN_PORT_MAXIMUM; i++)
		{
			constexpr std::uint32_t MAX_ADDRESS_CLAIM_RESOLUTION_TIME = 755; // This is 250ms + RTxD + 250ms
			if ((0 != lastAddressClaimRequestTimestamp_ms.at(i)) &&
			    (SystemTiming::time_expired_ms(lastAddressClaimRequestTimestamp_ms.at(i), MAX_ADDRESS_CLAIM_RESOLUTION_TIME)))
			{
				for (auto activeCF = activeControlFunctions.begin(); activeCF != activeControlFunctions.end(); activeCF++)
				{
					if ((nullptr != (*activeCF)) &&
					    ((*activeCF)->canPortIndex == i) &&
					    (!(*activeCF)->claimedAddressSinceLastAddressClaimRequest) &&
					    (ControlFunction::Type::Internal != (*activeCF)->get_type()))
					{
						ControlFunction *cfToMove = (*activeCF);
						activeCF = activeControlFunctions.erase(activeCF);

						if (activeControlFunctions.begin() != activeCF)
						{
							activeCF--;
						}

						inactiveControlFunctions.push_back(cfToMove);
						assert(nullptr != cfToMove);

						if ((NULL_CAN_ADDRESS != cfToMove->get_address()) &&
						    (nullptr != controlFunctionTable.at(i).at(cfToMove->get_address())))
						{
							CANStackLogger::debug("[NM]: Control function on channel %u with address %u and NAME %016llx is now offline.", i, cfToMove->get_address(), cfToMove->get_NAME());
							controlFunctionTable.at(i).at(cfToMove->get_address()) = nullptr;
							cfToMove->address = NULL_CAN_ADDRESS;
						}
						if (activeControlFunctions.end() == activeCF)
						{
							break;
						}
					}
				}
				lastAddressClaimRequestTimestamp_ms.at(i) = 0;
			}
		}
	}

	bool CANNetworkManager::send_can_message_raw(std::uint32_t portIndex, std::uint8_t sourceAddress, std::uint8_t destAddress, std::uint32_t parameterGroupNumber, std::uint8_t priority, const void *data, std::uint32_t size) const
	{
		CANMessageFrame tempFrame = construct_frame(portIndex, sourceAddress, destAddress, parameterGroupNumber, priority, data, size);
		bool retVal = false;

		if ((DEFAULT_IDENTIFIER != tempFrame.identifier) &&
		    (portIndex < CAN_PORT_MAXIMUM))
		{
			retVal = send_can_message_frame_to_hardware(tempFrame);
		}
		return retVal;
	}

	void CANNetworkManager::protocol_message_callback(const CANMessage &message)
	{
		process_can_message_for_global_and_partner_callbacks(message);
		process_can_message_for_commanded_address(message);
	}

} // namespace isobus
