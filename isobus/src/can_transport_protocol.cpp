//================================================================================================
/// @file can_transport_protocol.cpp
///
/// @brief A protocol that handles the ISO11783/J1939 transport protocol.
/// It handles both the broadcast version (BAM) and and the connection mode version.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#include "can_transport_protocol.hpp"

#include "can_general_parameter_group_numbers.hpp"
#include "can_network_configuration.hpp"
#include "system_timing.hpp"
#include "can_warning_logger.hpp"

#include <algorithm>

namespace isobus
{

	TransportProtocolManager TransportProtocolManager::Protocol;

	TransportProtocolManager::TransportProtocolSession::TransportProtocolSession(Direction sessionDirection, std::uint8_t canPortIndex) :
	  state(StateMachineState::None),
	  sessionMessage(canPortIndex),
	  timestamp_ms(0),
	  lastPacketNumber(0),
	  packetCount(0),
	  processedPacketsThisSession(0),
	  sessionDirection(sessionDirection)
	{
	}

	bool TransportProtocolManager::TransportProtocolSession::operator==(const TransportProtocolSession& obj)
	{
		return ((sessionMessage.get_source_control_function() == obj.sessionMessage.get_source_control_function()) &&
			    (sessionMessage.get_destination_control_function() == obj.sessionMessage.get_destination_control_function()) &&
			    (sessionMessage.get_identifier().get_parameter_group_number() == obj.sessionMessage.get_identifier().get_parameter_group_number()));
	}

	TransportProtocolManager::TransportProtocolSession::~TransportProtocolSession()
	{

	}

	TransportProtocolManager::TransportProtocolManager()
	{
	}

	TransportProtocolManager::~TransportProtocolManager()
	{
        if (initialized)
        {
            CANNetworkManager::CANNetwork.remove_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolCommand), process_message, this);
            CANNetworkManager::CANNetwork.remove_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolData), process_message, this);
        }
	}

    void TransportProtocolManager::initialize(CANLibBadge<CANNetworkManager>)
    {
        if (!initialized)
        {
            initialized = true;
            CANNetworkManager::CANNetwork.add_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolCommand), process_message, this);
            CANNetworkManager::CANNetwork.add_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolData), process_message, this);
        }
    }

	void TransportProtocolManager::process_message(CANMessage *const message)
	{
		if (nullptr != message)
		{
			switch (message->get_identifier().get_parameter_group_number())
			{
				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolCommand):
				{
					switch(message->get_data()[0])
					{
							case BROADCAST_ANNOUNCE_MESSAGE_MULTIPLEXOR:
							{
								if (CAN_DATA_LENGTH == message->get_data_length())
								{
									auto data = message->get_data();
									TransportProtocolSession *session;
									const std::uint32_t pgn = (static_cast<std::uint32_t>(data[5]) | (static_cast<std::uint32_t>(data[6]) << 8) | (static_cast<std::uint32_t>(data[7]) << 16));

									if ((nullptr == message->get_destination_control_function()) &&
										(activeSessions.size() < CANNetworkConfiguration::get_max_number_transport_protcol_sessions()) &&
										(!get_session(session, message->get_source_control_function(), message->get_destination_control_function(), pgn)))
									{
										TransportProtocolSession *newSession = new TransportProtocolSession(TransportProtocolSession::Direction::Receive, message->get_can_port_index());
										CANIdentifier tempIdentifierData(CANIdentifier::Type::Extended, pgn, CANIdentifier::CANPriority::PriorityLowest7, BROADCAST_CAN_ADDRESS, message->get_source_control_function()->get_address());
										newSession->sessionMessage.set_data_size(static_cast<std::uint16_t>(data[1]) | static_cast<std::uint16_t>(data[2] << 8));
										newSession->sessionMessage.set_source_control_function(message->get_source_control_function());
										newSession->sessionMessage.set_destination_control_function(nullptr);
										newSession->packetCount = data[3];
										newSession->sessionMessage.set_identifier(tempIdentifierData);
										newSession->state = StateMachineState::RxDataSession;
										newSession->timestamp_ms = SystemTiming::get_timestamp_ms();
										activeSessions.push_back(newSession);
										CANStackLogger::CAN_stack_log("TP: New BAM Session. Source: " + std::to_string(static_cast<int>(newSession->sessionMessage.get_source_control_function()->get_address())));
									}
									else
									{
										// Don't send an abort, they're probably expecting a CTS so it'll timeout
										// Or maybe if we already had a session they sent a second BAM? Also bad
										CANStackLogger::CAN_stack_log("TP: Can't Create BAM session");
									}
								}
								else
								{
									CANStackLogger::CAN_stack_log("TP: Bad BAM Message Length");
								}
							}
							break;
							
							case REQUEST_TO_SEND_MULTIPLEXOR:
							{
								if (CAN_DATA_LENGTH == message->get_data_length())
								{
									TransportProtocolSession *session;
									auto data = message->get_data();
									const std::uint32_t pgn = (static_cast<std::uint32_t>(data[5]) | (static_cast<std::uint32_t>(data[6]) << 8) | (static_cast<std::uint32_t>(data[7]) << 16));

									if ((nullptr != message->get_destination_control_function()) &&
										(activeSessions.size() < CANNetworkConfiguration::get_max_number_transport_protcol_sessions()) &&
										(!get_session(session, message->get_source_control_function(), message->get_destination_control_function(), pgn)))
									{
										TransportProtocolSession *newSession = new TransportProtocolSession(TransportProtocolSession::Direction::Receive, message->get_can_port_index());
										CANIdentifier tempIdentifierData(CANIdentifier::Type::Extended, pgn, CANIdentifier::CANPriority::PriorityLowest7, message->get_destination_control_function()->get_address(), message->get_source_control_function()->get_address());
										newSession->sessionMessage.set_data_size(static_cast<std::uint16_t>(data[1]) | static_cast<std::uint16_t>(data[2] << 8));
										newSession->sessionMessage.set_source_control_function(message->get_source_control_function());
										newSession->sessionMessage.set_destination_control_function(message->get_destination_control_function());
										newSession->packetCount = data[3];
										newSession->sessionMessage.set_identifier(tempIdentifierData);
										newSession->state = StateMachineState::RxDataSession;
										newSession->timestamp_ms = SystemTiming::get_timestamp_ms();
										activeSessions.push_back(newSession);
									}
									else if ((get_session(session, message->get_source_control_function(), message->get_destination_control_function(), pgn)) &&
											(nullptr != message->get_destination_control_function()) &&
											(ControlFunction::Type::Internal == message->get_destination_control_function()->get_type()))
									{
										abort_session(pgn, ConnectionAbortReason::AlreadyInCMSession, reinterpret_cast<InternalControlFunction *>(message->get_destination_control_function()), message->get_source_control_function());
										CANStackLogger::CAN_stack_log("TP: Abort RTS when already in CM session");
									}
									else if ((activeSessions.size() >= CANNetworkConfiguration::get_max_number_transport_protcol_sessions()) &&
											(nullptr != message->get_destination_control_function()) &&
											(ControlFunction::Type::Internal == message->get_destination_control_function()->get_type()))
									{
										abort_session(pgn, ConnectionAbortReason::SystemResourcesNeeded, reinterpret_cast<InternalControlFunction *>(message->get_destination_control_function()), message->get_source_control_function());
										CANStackLogger::CAN_stack_log("TP: Abort No Sessions Available");
									}
								}
								else
								{
									// Bad RTS message length. Can't really abort? Not sure what the PGN is if length < 8
									CANStackLogger::CAN_stack_log("TP: Bad Message Length");
								}
							}
							break;

							case CLEAR_TO_SEND_MULTIPLEXOR:
							{
								// Can't happen when doing a BAM session, make sure the session type is correct
								if ((CAN_DATA_LENGTH == message->get_data_length()) &&
									(nullptr != message->get_destination_control_function()) &&
									(nullptr != message->get_source_control_function()))
								{
									TransportProtocolSession *session;
									auto data = message->get_data();
									const std::uint32_t pgn = (static_cast<std::uint32_t>(data[5]) | (static_cast<std::uint32_t>(data[6]) << 8) | (static_cast<std::uint32_t>(data[7]) << 16));
									const std::uint8_t packetsToBeSent = data[1];

									if (get_session(session, message->get_destination_control_function(), message->get_source_control_function(), pgn))
									{
										if (StateMachineState::WaitForClearToSend == session->state)
										{
											session->packetCount = packetsToBeSent;
											session->timestamp_ms = SystemTiming::get_timestamp_ms();
											// If 0 was sent as the packet number, they want us to wait.
											// Just sit here in this state until we get a non-zero packet count
											if (0 != packetsToBeSent)
											{
												session->lastPacketNumber = 0;
												session->state = StateMachineState::TxDataSession;
											}
										}
										else
										{
											// The session exists, but we're probably already in the TxDataSession state. Need to abort
											// In the case of Rx'ing a CTS, we're the source in the session
											abort_session(pgn, ConnectionAbortReason::ClearToSendReceivedWhileTransferInProgress, reinterpret_cast<InternalControlFunction *>(message->get_destination_control_function()), message->get_source_control_function());
											CANStackLogger::CAN_stack_log("TP: Abort CTS while in data session");
										}
									}
									else
									{
										// We got a CTS but no session exists. Aborting clears up the situation faster than waiting for them to timeout
										// In the case of Rx'ing a CTS, we're the source in the session
										abort_session(pgn, ConnectionAbortReason::AnyOtherError, reinterpret_cast<InternalControlFunction *>(message->get_destination_control_function()), message->get_source_control_function());
										CANStackLogger::CAN_stack_log("TP: Abort CTS With no matching session");
									}
								}
								else
								{
									CANStackLogger::CAN_stack_log("TP: Invalid CTS");
								}
							}
							break;

							case END_OF_MESSAGE_ACKNOWLEDGE_MULTIPLEXOR:
							{
								// Can't happen when doing a BAM session, make sure the session type is correct
								if ((CAN_DATA_LENGTH == message->get_data_length()) &&
									(nullptr != message->get_destination_control_function()) &&
									(nullptr != message->get_source_control_function()))
								{
									TransportProtocolSession *session;
									auto data = message->get_data();
									const std::uint32_t pgn = (static_cast<std::uint32_t>(data[5]) | (static_cast<std::uint32_t>(data[6]) << 8) | (static_cast<std::uint32_t>(data[7]) << 16));
									
									if (get_session(session, message->get_destination_control_function(), message->get_source_control_function(), pgn))
									{
										if (StateMachineState::WaitForEndOfMessageAcknowledge == session->state)
										{
											// We completed our Tx session!
											session->state = StateMachineState::None;
											close_session(session);
										}
										else
										{
											abort_session(pgn, ConnectionAbortReason::AnyOtherError, reinterpret_cast<InternalControlFunction *>(message->get_destination_control_function()), message->get_source_control_function());
											CANStackLogger::CAN_stack_log("TP: Abort EOM in wrong session state");
										}
									}
									else
									{
										abort_session(pgn, ConnectionAbortReason::AnyOtherError, reinterpret_cast<InternalControlFunction *>(message->get_destination_control_function()), message->get_source_control_function());
										CANStackLogger::CAN_stack_log("TP: Abort EOM without matching session");
									}
								}
								else
								{
									CANStackLogger::CAN_stack_log("TP: Bad EOM received");
								}
							}
							break;

							case CONNECTION_ABORT_MULTIPLEXOR:
							{
								CANStackLogger::CAN_stack_log("TP: Received an abort");
							}
							break;

							default:
							{
								CANStackLogger::CAN_stack_log("TP: Bad Mux in Transport Protocol Command");
							}
							break;
           }
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolData):
				{
					TransportProtocolSession *tempSession = nullptr;

					if (get_session(tempSession, message->get_source_control_function(), message->get_destination_control_function()))
					{
						CANStackLogger::CAN_stack_log("TP: Matched Session");
					}

					if ((CAN_DATA_LENGTH == message->get_data_length()) &&
						(get_session(tempSession, message->get_source_control_function(), message->get_destination_control_function())) &&
						(StateMachineState::RxDataSession == tempSession->state) &&
						(message->get_data()[SEQUENCE_NUMBER_DATA_INDEX] == (tempSession->lastPacketNumber + 1)))
					{
						for (std::uint8_t i = SEQUENCE_NUMBER_DATA_INDEX; i < CAN_DATA_LENGTH; i++)
						{
							std::uint16_t currentDataIndex = (8 * tempSession->lastPacketNumber) + i;
							tempSession->sessionMessage.set_data(message->get_data()[SEQUENCE_NUMBER_DATA_INDEX + i], currentDataIndex);
						}
						tempSession->lastPacketNumber++;
						tempSession->processedPacketsThisSession++;
						if ((tempSession->lastPacketNumber * 8) >= tempSession->sessionMessage.get_data_length())
						{
							// Send EOM Ack for CM sessions only
							if (nullptr != tempSession->sessionMessage.get_destination_control_function())
							{
								send_end_of_session_acknowledgement(tempSession);
							}
							CANNetworkManager::CANNetwork.protocol_message_callback(&tempSession->sessionMessage);
							close_session(tempSession);
						}
						tempSession->timestamp_ms = SystemTiming::get_timestamp_ms();
					}
					else
					{
						CANStackLogger::CAN_stack_log("TP: Invalid BAM TP Data Received");
						if (get_session(tempSession, message->get_source_control_function(), message->get_destination_control_function()))
						{
							// If a session matches and ther was an error, get rid of the session
							close_session(tempSession);
						}
					}
				}
				break;

				default:
				{
                    // This is not a runtime error, should never happen.
                    // Bad PGN passed to protocol. Check PGN registrations.
					CANStackLogger::CAN_stack_log("TP: Received an unexpected PGN");
				}
				break;
			}
		}
	}

    void TransportProtocolManager::process_message(CANMessage *const message, void *parent)
    {
        if (nullptr != parent)
        {
            reinterpret_cast<TransportProtocolManager *>(parent)->process_message(message);
        }
    }

	bool TransportProtocolManager::protocol_transmit_message(std::uint32_t parameterGroupNumber,
		                               const std::uint8_t *dataBuffer,
		                               std::uint32_t messageLength,
		                               ControlFunction *source,
		                               ControlFunction *destination)
	{
		TransportProtocolSession *session;
		bool retVal = false;

		if ((messageLength < MAX_PROTOCOL_DATA_LENGTH) &&
			(messageLength > 8) &&
			(nullptr != dataBuffer) &&
			(nullptr != source) &&
			(true == source->get_address_valid()) &&
			((nullptr == destination) || 
			 (destination->get_address_valid())) &&
			 (!get_session(session, source, destination, parameterGroupNumber)))
		{
			TransportProtocolSession *newSession = new TransportProtocolSession(TransportProtocolSession::Direction::Transmit,
			                                                                    source->get_can_port());
			newSession->sessionMessage.set_data(dataBuffer, messageLength);
			newSession->sessionMessage.set_source_control_function(source);
			newSession->sessionMessage.set_destination_control_function(destination);
			newSession->packetCount = (messageLength / PROTOCOL_BYTES_PER_FRAME);
			newSession->lastPacketNumber = 0;
			newSession->processedPacketsThisSession = 0;
			if (0 != (messageLength % PROTOCOL_BYTES_PER_FRAME))
			{
				newSession->packetCount++;
			}
			CANIdentifier messageVirtualID(CANIdentifier::Type::Extended,
			              parameterGroupNumber,
			              CANIdentifier::CANPriority::PriorityDefault6,
			              destination->get_address(),
			              source->get_address());

			newSession->sessionMessage.set_identifier(messageVirtualID);
			set_state(newSession, StateMachineState::RequestToSend);
			activeSessions.push_back(newSession);
			CANStackLogger::CAN_stack_log("TP: New CM Tx Session. Dest: " + std::to_string(static_cast<int>(destination->get_address())));
		}
		return retVal;
	}

	void TransportProtocolManager::update(CANLibBadge<CANNetworkManager>)
	{
		for (auto i : activeSessions)
		{
			update_state_machine(i);
		}
	}

	bool TransportProtocolManager::abort_session(TransportProtocolSession *session, ConnectionAbortReason reason)
	{
		bool retVal = false;

		if (nullptr != session)
		{
			InternalControlFunction *myControlFunction;
			ControlFunction *partnerControlFunction;
			std::array<std::uint8_t, 8> data;
			std::uint32_t pgn = session->sessionMessage.get_identifier().get_parameter_group_number();

			if (TransportProtocolSession::Direction::Transmit == session->sessionDirection)
			{
				myControlFunction = CANNetworkManager::CANNetwork.get_internal_control_function(session->sessionMessage.get_source_control_function());
				partnerControlFunction = session->sessionMessage.get_destination_control_function();
			}
			else
			{
				myControlFunction = CANNetworkManager::CANNetwork.get_internal_control_function(session->sessionMessage.get_destination_control_function());
				partnerControlFunction = session->sessionMessage.get_source_control_function();
			}

			data[0] = CONNECTION_ABORT_MULTIPLEXOR;
			data[1] = static_cast<std::uint8_t>(reason);
			data[2] = 0xFF;
			data[3] = 0xFF;
			data[4] = 0xFF;
			data[5] = static_cast<std::uint8_t>(pgn & 0xFF);
			data[6] = static_cast<std::uint8_t>((pgn>> 8) & 0xFF);
			data[7] = static_cast<std::uint8_t>((pgn >> 16) & 0xFF);
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolCommand),
																data.data(),
																8,
																myControlFunction,
																partnerControlFunction,
																CANIdentifier::CANPriority::PriorityDefault6);
		}
		return retVal;
	}

	bool TransportProtocolManager::abort_session(std::uint32_t parameterGroupNumber, ConnectionAbortReason reason, InternalControlFunction *source, ControlFunction *destination)
	{
		std::array<std::uint8_t, 8> data;

		data[0] = CONNECTION_ABORT_MULTIPLEXOR;
		data[1] = static_cast<std::uint8_t>(reason);
		data[2] = 0xFF;
		data[3] = 0xFF;
		data[4] = 0xFF;
		data[5] = static_cast<std::uint8_t>(parameterGroupNumber & 0xFF);
		data[6] = static_cast<std::uint8_t>((parameterGroupNumber >> 8) & 0xFF);
		data[7] = static_cast<std::uint8_t>((parameterGroupNumber >> 16) & 0xFF);
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolCommand),
		                                                      data.data(),
		                                                      8,
		                                                      source,
		                                                      destination,
		                                                      CANIdentifier::CANPriority::PriorityDefault6);
	}

	void TransportProtocolManager::close_session(TransportProtocolSession *session)
	{
		if (nullptr != session)
		{
			auto sessionLocation = std::find(activeSessions.begin(), activeSessions.end(), session);
			if (activeSessions.end() != sessionLocation)
			{
				activeSessions.erase(sessionLocation);
				delete session;
				CANStackLogger::CAN_stack_log("TP: Session Closed");
			}
		}
	}

	bool TransportProtocolManager::send_request_to_send(TransportProtocolSession *session)
	{
		bool retVal = false;

		if (nullptr != session)
		{
			const std::uint8_t dataBuffer[CAN_DATA_LENGTH] = { REQUEST_TO_SEND_MULTIPLEXOR,
				                                                 static_cast<std::uint8_t>(session->sessionMessage.get_data_length() & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_data_length() >> 8) & 0xFF),
				                                                 session->packetCount,
				                                                 0xFF,
				                                                 static_cast<std::uint8_t>(session->sessionMessage.get_identifier().get_parameter_group_number() & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 8) & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 16) & 0xFF) };
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolCommand),
			                                                        dataBuffer,
			                                                        CAN_DATA_LENGTH,
			                                                        reinterpret_cast<InternalControlFunction *>(session->sessionMessage.get_source_control_function()),
			                                                        session->sessionMessage.get_destination_control_function(),
			                                                        CANIdentifier::CANPriority::PriorityLowest7);
		}
		return retVal;
	}

	bool TransportProtocolManager::send_end_of_session_acknowledgement(TransportProtocolSession *session)
	{
		bool retVal = false;

		if (nullptr != session)
		{
			std::uint8_t dataBuffer[CAN_DATA_LENGTH] = {0};
			std::uint32_t messageLength = session->sessionMessage.get_data_length();
			std::uint32_t pgn = session->sessionMessage.get_identifier().get_parameter_group_number();
			
			dataBuffer[0] = END_OF_MESSAGE_ACKNOWLEDGE_MULTIPLEXOR;
			dataBuffer[1] = static_cast<std::uint8_t>(messageLength);
			dataBuffer[2] = static_cast<std::uint8_t>(messageLength >> 8);
			dataBuffer[3] = (static_cast<std::uint8_t>((messageLength- 1) / 7) + 1);
			dataBuffer[4] = 0xFF;
			dataBuffer[5] = static_cast<std::uint8_t>(pgn);
			dataBuffer[6] = static_cast<std::uint8_t>(pgn >> 8);
			dataBuffer[7] = static_cast<std::uint8_t>(pgn >> 16);

			// This message only needs to be sent if we're the recipient. Sanity check the destination is us
			if (ControlFunction::Type::Internal == session->sessionMessage.get_destination_control_function()->get_type())
			{
				retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolCommand), 
				dataBuffer, 
				CAN_DATA_LENGTH, 
				reinterpret_cast<InternalControlFunction *>(session->sessionMessage.get_destination_control_function()),
				session->sessionMessage.get_source_control_function(),
				CANIdentifier::CANPriority::PriorityDefault6);
			}
		}
		else
		{
			CANStackLogger::CAN_stack_log("TP: Attempted to send EOM to null session");
		}
		return retVal;
	}

	void TransportProtocolManager::set_state(TransportProtocolSession *session, StateMachineState value)
	{
		if (nullptr != session)
		{
			session->timestamp_ms = SystemTiming::get_timestamp_ms();
			session->state = value;
		}
	}

	bool TransportProtocolManager::get_session(TransportProtocolSession *&session, ControlFunction *source, ControlFunction *destination)
	{
		session = nullptr;

		for (auto i : activeSessions)
		{
			if ((i->sessionMessage.get_source_control_function() == source) &&
				(i->sessionMessage.get_destination_control_function() == destination))
			{
				session = i;
				break;
			}
		}
		return (nullptr != session);
	}

	bool TransportProtocolManager::get_session(TransportProtocolSession *&session, ControlFunction *source, ControlFunction *destination, std::uint32_t parameterGroupNumber)
	{
		bool retVal = false;
		session = nullptr;

		if ((get_session(session, source, destination)) &&
			(session->sessionMessage.get_identifier().get_parameter_group_number() == parameterGroupNumber))
		{
			retVal = true;
		}
		return retVal;
	}

	void TransportProtocolManager::update_state_machine(TransportProtocolSession *session)
	{
		if (nullptr != session)
		{
			switch (session->state)
			{
				case StateMachineState::None:
				case StateMachineState::ClearToSend:
				{

				}
				break;

				case StateMachineState::WaitForClearToSend:
				case StateMachineState::WaitForEndOfMessageAcknowledge:
				{
					if (SystemTiming::time_expired_ms(session->timestamp_ms, T2_T3_TIMEOUT_MS))
					{
						CANStackLogger::CAN_stack_log("TP: Timeout");
						abort_session(session, ConnectionAbortReason::Timeout);
						close_session(session);
					}
				}
				break;

				case StateMachineState::RequestToSend:
				{
					if (send_request_to_send(session))
					{
						set_state(session, StateMachineState::WaitForClearToSend);
					}
				}
				break;

				case StateMachineState::TxDataSession:
				{
					if (nullptr != session->sessionMessage.get_destination_control_function())
					{
						std::uint8_t dataBuffer[CAN_DATA_LENGTH];
						// Try and send packets
						for (std::uint8_t i = session->lastPacketNumber; i < session->packetCount; i++)
						{
							dataBuffer[0] = session->processedPacketsThisSession + 1;
							
							for (std::uint8_t j = 0; j < PROTOCOL_BYTES_PER_FRAME; j++)
							{
								std::uint32_t index = (j + (PROTOCOL_BYTES_PER_FRAME * session->processedPacketsThisSession));
								if (index < session->sessionMessage.get_data_length())
								{
									dataBuffer[1 + j] = session->sessionMessage.get_data()[j + (PROTOCOL_BYTES_PER_FRAME * session->processedPacketsThisSession)];
								}
								else
								{
									dataBuffer[1 + j] = 0xFF;
								}
							}
							if (CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolData),
							                                                   dataBuffer,
							                                                   CAN_DATA_LENGTH,
							                                                   reinterpret_cast<InternalControlFunction*>(session->sessionMessage.get_source_control_function()),
							                                                   session->sessionMessage.get_destination_control_function(),
							                                                   CANIdentifier::CANPriority::PriorityLowest7))
							{
								session->lastPacketNumber++;
								session->processedPacketsThisSession++;
								session->timestamp_ms = SystemTiming::get_timestamp_ms();
							}
							else
							{
								// Process more next time protocol is updated
								break;
							}
						}

						if ((session->lastPacketNumber == (session->packetCount)) && 
							(session->sessionMessage.get_data_length() <= (PROTOCOL_BYTES_PER_FRAME * session->processedPacketsThisSession)))
						{
							set_state(session, StateMachineState::WaitForEndOfMessageAcknowledge);
							session->timestamp_ms = SystemTiming::get_timestamp_ms();
						}
						else if (session->lastPacketNumber == session->packetCount)
						{
							set_state(session, StateMachineState::WaitForClearToSend);
							session->timestamp_ms = SystemTiming::get_timestamp_ms();
						}
					}
				}
				break;

				case StateMachineState::RxDataSession:
				{
					if (nullptr == session->sessionMessage.get_destination_control_function())
					{
						// BAM Timeout check
						if (SystemTiming::time_expired_ms(session->timestamp_ms, T1_TIMEOUT_MS))
						{
							CANStackLogger::CAN_stack_log("TP: BAM Rx Timeout");
							close_session(session);
						}
					}
					else
					{
						// CM TP Timeout check
						if (SystemTiming::time_expired_ms(session->timestamp_ms, MESSAGE_TR_TIMEOUT_MS))
						{
							CANStackLogger::CAN_stack_log("TP: CM Rx Timeout");
							abort_session(session, ConnectionAbortReason::Timeout);
							close_session(session);
						}
					}
				}
				break;
			}
		}
	}

}
