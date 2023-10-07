//================================================================================================
/// @file can_transport_protocol.cpp
///
/// @brief A protocol that handles the ISO11783/J1939 transport protocol.
/// It handles both the broadcast version (BAM) and and the connection mode version.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#include "isobus/isobus/can_transport_protocol.hpp"

#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_configuration.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

#include <algorithm>

namespace isobus
{
	TransportProtocolManager::TransportProtocolSession::TransportProtocolSession(Direction sessionDirection, std::uint8_t canPortIndex) :
	  sessionMessage(canPortIndex),
	  sessionDirection(sessionDirection)
	{
	}

	bool TransportProtocolManager::TransportProtocolSession::operator==(const TransportProtocolSession &obj)
	{
		return ((sessionMessage.get_source_control_function() == obj.sessionMessage.get_source_control_function()) &&
		        (sessionMessage.get_destination_control_function() == obj.sessionMessage.get_destination_control_function()) &&
		        (sessionMessage.get_identifier().get_parameter_group_number() == obj.sessionMessage.get_identifier().get_parameter_group_number()));
	}

	std::uint32_t TransportProtocolManager::TransportProtocolSession::get_message_data_length() const
	{
		if (nullptr != frameChunkCallback)
		{
			return frameChunkCallbackMessageLength;
		}
		return sessionMessage.get_data_length();
	}

	TransportProtocolManager::~TransportProtocolManager()
	{
		// No need to clean up, as this object is a member of the network manager
		// so its callbacks will be cleared at destruction time
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

	void TransportProtocolManager::process_message(const CANMessage &message)
	{
		if ((nullptr != message.get_source_control_function()) &&
		    ((nullptr == message.get_destination_control_function()) ||
		     (nullptr != CANNetworkManager::CANNetwork.get_internal_control_function(message.get_destination_control_function()))))
		{
			switch (message.get_identifier().get_parameter_group_number())
			{
				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolCommand):
				{
					TransportProtocolSession *session;
					const auto &data = message.get_data();
					const std::uint32_t pgn = (static_cast<std::uint32_t>(data[5]) | (static_cast<std::uint32_t>(data[6]) << 8) | (static_cast<std::uint32_t>(data[7]) << 16));
					switch (data[0])
					{
						case BROADCAST_ANNOUNCE_MESSAGE_MULTIPLEXOR:
						{
							if (CAN_DATA_LENGTH == message.get_data_length())
							{
								if ((nullptr == message.get_destination_control_function()) &&
								    (activeSessions.size() < CANNetworkManager::CANNetwork.get_configuration().get_max_number_transport_protocol_sessions()) &&
								    (!get_session(session, message.get_source_control_function(), message.get_destination_control_function(), pgn)))
								{
									TransportProtocolSession *newSession = new TransportProtocolSession(TransportProtocolSession::Direction::Receive, message.get_can_port_index());
									CANIdentifier tempIdentifierData(CANIdentifier::Type::Extended, pgn, CANIdentifier::CANPriority::PriorityLowest7, BROADCAST_CAN_ADDRESS, message.get_source_control_function()->get_address());
									newSession->sessionMessage.set_data_size(static_cast<std::uint16_t>(data[1]) | static_cast<std::uint16_t>(data[2] << 8));
									newSession->sessionMessage.set_source_control_function(message.get_source_control_function());
									newSession->sessionMessage.set_destination_control_function(nullptr);
									newSession->packetCount = data[3];
									newSession->sessionMessage.set_identifier(tempIdentifierData);
									newSession->state = StateMachineState::RxDataSession;
									newSession->timestamp_ms = SystemTiming::get_timestamp_ms();
									activeSessions.push_back(newSession);
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Debug,
									                              "[TP]: New Rx BAM Session. Source: " +
									                                isobus::to_string(static_cast<int>(newSession->sessionMessage.get_source_control_function()->get_address())));
								}
								else
								{
									// Don't send an abort, they're probably expecting a CTS so it'll timeout
									// Or maybe if we already had a session they sent a second BAM? Also bad
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[TP]: Can't Create an Rx BAM session");
								}
							}
							else
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[TP]: Bad BAM Message Length");
							}
						}
						break;

						case REQUEST_TO_SEND_MULTIPLEXOR:
						{
							if (CAN_DATA_LENGTH == message.get_data_length())
							{
								if ((nullptr != message.get_destination_control_function()) &&
								    (activeSessions.size() < CANNetworkManager::CANNetwork.get_configuration().get_max_number_transport_protocol_sessions()) &&
								    (!get_session(session, message.get_source_control_function(), message.get_destination_control_function(), pgn)))
								{
									TransportProtocolSession *newSession = new TransportProtocolSession(TransportProtocolSession::Direction::Receive, message.get_can_port_index());
									CANIdentifier tempIdentifierData(CANIdentifier::Type::Extended, pgn, CANIdentifier::CANPriority::PriorityLowest7, message.get_destination_control_function()->get_address(), message.get_source_control_function()->get_address());
									newSession->sessionMessage.set_data_size(static_cast<std::uint16_t>(data[1]) | static_cast<std::uint16_t>(data[2] << 8));
									newSession->sessionMessage.set_source_control_function(message.get_source_control_function());
									newSession->sessionMessage.set_destination_control_function(message.get_destination_control_function());
									newSession->packetCount = data[3];
									newSession->clearToSendPacketMax = data[4];
									newSession->sessionMessage.set_identifier(tempIdentifierData);
									newSession->state = StateMachineState::ClearToSend;
									newSession->timestamp_ms = SystemTiming::get_timestamp_ms();
									activeSessions.push_back(newSession);
								}
								else if ((get_session(session, message.get_source_control_function(), message.get_destination_control_function(), pgn)) &&
								         (nullptr != message.get_destination_control_function()) &&
								         (ControlFunction::Type::Internal == message.get_destination_control_function()->get_type()))
								{
									abort_session(pgn, ConnectionAbortReason::AlreadyInCMSession, std::static_pointer_cast<InternalControlFunction>(message.get_destination_control_function()), message.get_source_control_function());
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[TP]: Sent abort, RTS when already in CM session");
								}
								else if ((activeSessions.size() >= CANNetworkManager::CANNetwork.get_configuration().get_max_number_transport_protocol_sessions()) &&
								         (nullptr != message.get_destination_control_function()) &&
								         (ControlFunction::Type::Internal == message.get_destination_control_function()->get_type()))
								{
									abort_session(pgn, ConnectionAbortReason::SystemResourcesNeeded, std::static_pointer_cast<InternalControlFunction>(message.get_destination_control_function()), message.get_source_control_function());
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[TP]: Sent abort, No Sessions Available");
								}
							}
							else
							{
								// Bad RTS message length. Can't really abort? Not sure what the PGN is if length < 8
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[TP]: Received Bad Message Length for an RTS");
							}
						}
						break;

						case CLEAR_TO_SEND_MULTIPLEXOR:
						{
							// Can't happen when doing a BAM session, make sure the session type is correct
							if ((CAN_DATA_LENGTH == message.get_data_length()) &&
							    (nullptr != message.get_destination_control_function()) &&
							    (nullptr != message.get_source_control_function()))
							{
								const std::uint8_t packetsToBeSent = data[1];

								if (get_session(session, message.get_destination_control_function(), message.get_source_control_function(), pgn))
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
										abort_session(pgn, ConnectionAbortReason::ClearToSendReceivedWhileTransferInProgress, std::static_pointer_cast<InternalControlFunction>(message.get_destination_control_function()), message.get_source_control_function());
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[TP]: Sent abort, CTS while in data session, PGN: " + isobus::to_string(pgn));
									}
								}
								else
								{
									// We got a CTS but no session exists. Aborting clears up the situation faster than waiting for them to timeout
									// In the case of Rx'ing a CTS, we're the source in the session
									abort_session(pgn, ConnectionAbortReason::AnyOtherError, std::static_pointer_cast<InternalControlFunction>(message.get_destination_control_function()), message.get_source_control_function());
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[TP]: Sent abort, CTS With no matching session, PGN: " + isobus::to_string(pgn));
								}
							}
							else
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[TP]: Received an Invalid CTS");
							}
						}
						break;

						case END_OF_MESSAGE_ACKNOWLEDGE_MULTIPLEXOR:
						{
							// Can't happen when doing a BAM session, make sure the session type is correct
							if ((CAN_DATA_LENGTH == message.get_data_length()) &&
							    (nullptr != message.get_destination_control_function()) &&
							    (nullptr != message.get_source_control_function()))
							{
								if (get_session(session, message.get_destination_control_function(), message.get_source_control_function(), pgn))
								{
									if (StateMachineState::WaitForEndOfMessageAcknowledge == session->state)
									{
										// We completed our Tx session!
										session->state = StateMachineState::None;
										close_session(session, true);
									}
									else
									{
										abort_session(pgn, ConnectionAbortReason::AnyOtherError, std::static_pointer_cast<InternalControlFunction>(message.get_destination_control_function()), message.get_source_control_function());
										close_session(session, false);
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[TP]: Sent abort, received EOM in wrong session state, PGN: " + isobus::to_string(pgn));
									}
								}
								else
								{
									abort_session(pgn, ConnectionAbortReason::AnyOtherError, std::static_pointer_cast<InternalControlFunction>(message.get_destination_control_function()), message.get_source_control_function());
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[TP]: Sent abort, received EOM without matching session, PGN: " + isobus::to_string(pgn));
								}
							}
							else
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[TP]: Bad EOM received");
							}
						}
						break;

						case CONNECTION_ABORT_MULTIPLEXOR:
						{
							if (get_session(session, message.get_destination_control_function(), message.get_source_control_function(), pgn))
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[TP]: Received an abort for an session with PGN: " + isobus::to_string(pgn));
								close_session(session, false);
							}
							else
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[TP]: Received an abort with no matching session with PGN: " + isobus::to_string(pgn));
							}
						}
						break;

						default:
						{
							CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[TP]: Bad Mux in Transport Protocol Command");
						}
						break;
					}
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolData):
				{
					TransportProtocolSession *tempSession = nullptr;

					if ((CAN_DATA_LENGTH == message.get_data_length()) &&
					    (get_session(tempSession, message.get_source_control_function(), message.get_destination_control_function())) &&
					    (StateMachineState::RxDataSession == tempSession->state))
					{
						// Check for valid sequence number
						if (message.get_data()[SEQUENCE_NUMBER_DATA_INDEX] == (tempSession->lastPacketNumber + 1))
						{
							for (std::uint8_t i = SEQUENCE_NUMBER_DATA_INDEX; (i < PROTOCOL_BYTES_PER_FRAME) && (static_cast<std::uint32_t>((PROTOCOL_BYTES_PER_FRAME * tempSession->lastPacketNumber) + i) < tempSession->get_message_data_length()); i++)
							{
								std::uint16_t currentDataIndex = (PROTOCOL_BYTES_PER_FRAME * tempSession->lastPacketNumber) + i;
								tempSession->sessionMessage.set_data(message.get_data()[1 + SEQUENCE_NUMBER_DATA_INDEX + i], currentDataIndex);
							}
							tempSession->lastPacketNumber++;
							tempSession->processedPacketsThisSession++;
							if ((tempSession->lastPacketNumber * PROTOCOL_BYTES_PER_FRAME) >= tempSession->get_message_data_length())
							{
								// Send EOM Ack for CM sessions only
								if (nullptr != tempSession->sessionMessage.get_destination_control_function())
								{
									send_end_of_session_acknowledgement(tempSession);
								}
								CANNetworkManager::CANNetwork.process_any_control_function_pgn_callbacks(tempSession->sessionMessage);
								CANNetworkManager::CANNetwork.protocol_message_callback(tempSession->sessionMessage);
								close_session(tempSession, true);
							}
							tempSession->timestamp_ms = SystemTiming::get_timestamp_ms();
						}
						else if (message.get_data()[SEQUENCE_NUMBER_DATA_INDEX] == (tempSession->lastPacketNumber))
						{
							// Sequence number is duplicate of the last one
							CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[TP]: Aborting session due to duplciate sequence number");
							abort_session(tempSession, ConnectionAbortReason::DuplicateSequenceNumber);
							close_session(tempSession, false);
						}
						else
						{
							CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[TP]: Aborting session due to bad sequence number");
							abort_session(tempSession, ConnectionAbortReason::BadSequenceNumber);
							close_session(tempSession, false);
						}
					}
					else
					{
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[TP]: Invalid BAM TP Data Received");
						if (get_session(tempSession, message.get_source_control_function(), message.get_destination_control_function()))
						{
							// If a session matches and ther was an error, get rid of the session
							close_session(tempSession, false);
						}
					}
				}
				break;

				default:
				{
					// This is not a runtime error, should never happen.
					// Bad PGN passed to protocol. Check PGN registrations.
					CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[TP]: Received an unexpected PGN");
				}
				break;
			}
		}
	}

	void TransportProtocolManager::process_message(const CANMessage &message, void *parent)
	{
		if (nullptr != parent)
		{
			reinterpret_cast<TransportProtocolManager *>(parent)->process_message(message);
		}
	}

	bool TransportProtocolManager::protocol_transmit_message(std::uint32_t parameterGroupNumber,
	                                                         const std::uint8_t *dataBuffer,
	                                                         std::uint32_t messageLength,
	                                                         std::shared_ptr<ControlFunction> source,
	                                                         std::shared_ptr<ControlFunction> destination,
	                                                         TransmitCompleteCallback sessionCompleteCallback,
	                                                         void *parentPointer,
	                                                         DataChunkCallback frameChunkCallback)
	{
		TransportProtocolSession *session;
		bool retVal = false;

		if ((messageLength <= MAX_PROTOCOL_DATA_LENGTH) &&
		    (messageLength > CAN_DATA_LENGTH) &&
		    ((nullptr != dataBuffer) ||
		     (nullptr != frameChunkCallback)) &&
		    (nullptr != source) &&
		    (true == source->get_address_valid()) &&
		    ((nullptr == destination) ||
		     (destination->get_address_valid())) &&
		    (!get_session(session, source, destination, parameterGroupNumber)) &&
		    ((nullptr != destination) ||
		     ((nullptr == destination) &&
		      (!get_session(session, source, destination)))))
		{
			TransportProtocolSession *newSession = new TransportProtocolSession(TransportProtocolSession::Direction::Transmit,
			                                                                    source->get_can_port());
			std::uint8_t destinationAddress;

			if (dataBuffer != nullptr)
			{
				newSession->sessionMessage.set_data(dataBuffer, messageLength);
			}
			else
			{
				newSession->frameChunkCallback = frameChunkCallback;
				newSession->frameChunkCallbackMessageLength = messageLength;
			}
			newSession->sessionMessage.set_source_control_function(source);
			newSession->sessionMessage.set_destination_control_function(destination);
			newSession->packetCount = (messageLength / PROTOCOL_BYTES_PER_FRAME);
			newSession->lastPacketNumber = 0;
			newSession->processedPacketsThisSession = 0;
			newSession->sessionCompleteCallback = sessionCompleteCallback;
			newSession->parent = parentPointer;
			if (0 != (messageLength % PROTOCOL_BYTES_PER_FRAME))
			{
				newSession->packetCount++;
			}

			if (nullptr != destination)
			{
				// CM Message
				destinationAddress = destination->get_address();
				set_state(newSession, StateMachineState::RequestToSend);
			}
			else
			{
				// BAM message
				destinationAddress = BROADCAST_CAN_ADDRESS;
				set_state(newSession, StateMachineState::BroadcastAnnounce);
			}

			CANIdentifier messageVirtualID(CANIdentifier::Type::Extended,
			                               parameterGroupNumber,
			                               CANIdentifier::CANPriority::PriorityDefault6,
			                               destinationAddress,
			                               source->get_address());

			newSession->sessionMessage.set_identifier(messageVirtualID);

			activeSessions.push_back(newSession);
			retVal = true;
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
			std::shared_ptr<InternalControlFunction> myControlFunction;
			std::shared_ptr<ControlFunction> partnerControlFunction;
			std::array<std::uint8_t, CAN_DATA_LENGTH> data;
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
			data[6] = static_cast<std::uint8_t>((pgn >> 8) & 0xFF);
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

	bool TransportProtocolManager::abort_session(std::uint32_t parameterGroupNumber, ConnectionAbortReason reason, std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination)
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

	void TransportProtocolManager::close_session(TransportProtocolSession *session, bool successfull)
	{
		if (nullptr != session)
		{
			process_session_complete_callback(session, successfull);
			auto sessionLocation = std::find(activeSessions.begin(), activeSessions.end(), session);
			if (activeSessions.end() != sessionLocation)
			{
				activeSessions.erase(sessionLocation);
				delete session;
				CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Debug, "[TP]: Session Closed");
			}
		}
	}

	void TransportProtocolManager::process_session_complete_callback(TransportProtocolSession *session, bool success)
	{
		if ((nullptr != session) &&
		    (nullptr != session->sessionCompleteCallback) &&
		    (nullptr != session->sessionMessage.get_source_control_function()) &&
		    (ControlFunction::Type::Internal == session->sessionMessage.get_source_control_function()->get_type()))
		{
			session->sessionCompleteCallback(session->sessionMessage.get_identifier().get_parameter_group_number(),
			                                 session->get_message_data_length(),
			                                 std::static_pointer_cast<InternalControlFunction>(session->sessionMessage.get_source_control_function()),
			                                 session->sessionMessage.get_destination_control_function(),
			                                 success,
			                                 session->parent);
		}
	}

	bool TransportProtocolManager::send_broadcast_announce_message(TransportProtocolSession *session) const
	{
		bool retVal = false;

		if (nullptr != session)
		{
			const std::uint8_t dataBuffer[CAN_DATA_LENGTH] = { BROADCAST_ANNOUNCE_MESSAGE_MULTIPLEXOR,
				                                                 static_cast<std::uint8_t>(session->get_message_data_length() & 0xFF),
				                                                 static_cast<std::uint8_t>((session->get_message_data_length() >> 8) & 0xFF),
				                                                 session->packetCount,
				                                                 0xFF,
				                                                 static_cast<std::uint8_t>(session->sessionMessage.get_identifier().get_parameter_group_number() & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 8) & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 16) & 0xFF) };
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolCommand),
			                                                        dataBuffer,
			                                                        CAN_DATA_LENGTH,
			                                                        std::static_pointer_cast<InternalControlFunction>(session->sessionMessage.get_source_control_function()),
			                                                        nullptr,
			                                                        CANIdentifier::CANPriority::PriorityDefault6);
		}
		return retVal;
	}

	bool TransportProtocolManager::send_clear_to_send(TransportProtocolSession *session) const
	{
		bool retVal = false;

		if (nullptr != session)
		{
			std::uint8_t packetsRemaining = (session->packetCount - session->processedPacketsThisSession);
			std::uint8_t packetsThisSegment;

			if (session->clearToSendPacketMax < packetsRemaining)
			{
				packetsThisSegment = session->clearToSendPacketMax;
			}
			else
			{
				packetsThisSegment = packetsRemaining;
			}

			const std::uint8_t dataBuffer[CAN_DATA_LENGTH] = { CLEAR_TO_SEND_MULTIPLEXOR,
				                                                 packetsThisSegment,
				                                                 static_cast<std::uint8_t>(session->processedPacketsThisSession + 1),
				                                                 0xFF,
				                                                 0xFF,
				                                                 static_cast<std::uint8_t>(session->sessionMessage.get_identifier().get_parameter_group_number() & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 8) & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 16) & 0xFF) };
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolCommand),
			                                                        dataBuffer,
			                                                        CAN_DATA_LENGTH,
			                                                        std::static_pointer_cast<InternalControlFunction>(session->sessionMessage.get_destination_control_function()),
			                                                        session->sessionMessage.get_source_control_function(),
			                                                        CANIdentifier::CANPriority::PriorityDefault6);
		}
		return retVal;
	}

	bool TransportProtocolManager::send_request_to_send(TransportProtocolSession *session) const
	{
		bool retVal = false;

		if (nullptr != session)
		{
			const std::uint8_t dataBuffer[CAN_DATA_LENGTH] = { REQUEST_TO_SEND_MULTIPLEXOR,
				                                                 static_cast<std::uint8_t>(session->get_message_data_length() & 0xFF),
				                                                 static_cast<std::uint8_t>((session->get_message_data_length() >> 8) & 0xFF),
				                                                 session->packetCount,
				                                                 0xFF,
				                                                 static_cast<std::uint8_t>(session->sessionMessage.get_identifier().get_parameter_group_number() & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 8) & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 16) & 0xFF) };
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolCommand),
			                                                        dataBuffer,
			                                                        CAN_DATA_LENGTH,
			                                                        std::static_pointer_cast<InternalControlFunction>(session->sessionMessage.get_source_control_function()),
			                                                        session->sessionMessage.get_destination_control_function(),
			                                                        CANIdentifier::CANPriority::PriorityDefault6);
		}
		return retVal;
	}

	bool TransportProtocolManager::send_end_of_session_acknowledgement(TransportProtocolSession *session) const
	{
		bool retVal = false;

		if (nullptr != session)
		{
			std::uint8_t dataBuffer[CAN_DATA_LENGTH] = { 0 };
			std::uint32_t messageLength = session->get_message_data_length();
			std::uint32_t pgn = session->sessionMessage.get_identifier().get_parameter_group_number();

			dataBuffer[0] = END_OF_MESSAGE_ACKNOWLEDGE_MULTIPLEXOR;
			dataBuffer[1] = static_cast<std::uint8_t>(messageLength);
			dataBuffer[2] = static_cast<std::uint8_t>(messageLength >> 8);
			dataBuffer[3] = (static_cast<std::uint8_t>((messageLength - 1) / 7) + 1);
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
				                                                        std::static_pointer_cast<InternalControlFunction>(session->sessionMessage.get_destination_control_function()),
				                                                        session->sessionMessage.get_source_control_function(),
				                                                        CANIdentifier::CANPriority::PriorityDefault6);
			}
		}
		else
		{
			CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[TP]: Attempted to send EOM to null session");
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

	bool TransportProtocolManager::get_session(TransportProtocolSession *&session, std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination)
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

	bool TransportProtocolManager::get_session(TransportProtocolSession *&session, std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination, std::uint32_t parameterGroupNumber)
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
				{
				}
				break;

				case StateMachineState::ClearToSend:
				{
					if (send_clear_to_send(session))
					{
						set_state(session, StateMachineState::RxDataSession);
					}
				}
				break;

				case StateMachineState::WaitForClearToSend:
				case StateMachineState::WaitForEndOfMessageAcknowledge:
				{
					if (SystemTiming::time_expired_ms(session->timestamp_ms, T2_T3_TIMEOUT_MS))
					{
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[TP]: Timeout");
						abort_session(session, ConnectionAbortReason::Timeout);
						close_session(session, false);
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

				case StateMachineState::BroadcastAnnounce:
				{
					if (send_broadcast_announce_message(session))
					{
						set_state(session, StateMachineState::TxDataSession);
					}
				}
				break;

				case StateMachineState::TxDataSession:
				{
					bool sessionStillValid = true;

					if ((nullptr != session->sessionMessage.get_destination_control_function()) || (SystemTiming::time_expired_ms(session->timestamp_ms, CANNetworkManager::CANNetwork.get_configuration().get_minimum_time_between_transport_protocol_bam_frames())))
					{
						std::uint8_t dataBuffer[CAN_DATA_LENGTH];
						std::uint32_t framesSentThisUpdate = 0;

						// Try and send packets
						for (std::uint16_t i = session->lastPacketNumber; i < session->packetCount; i++)
						{
							dataBuffer[0] = (session->processedPacketsThisSession + 1);

							if (nullptr != session->frameChunkCallback)
							{
								// Use the callback to get this frame's data
								std::uint8_t callbackBuffer[7] = {
									0xFF,
									0xFF,
									0xFF,
									0xFF,
									0xFF,
									0xFF,
									0xFF
								};
								std::uint16_t numberBytesLeft = (session->get_message_data_length() - (PROTOCOL_BYTES_PER_FRAME * session->processedPacketsThisSession));

								if (numberBytesLeft > PROTOCOL_BYTES_PER_FRAME)
								{
									numberBytesLeft = PROTOCOL_BYTES_PER_FRAME;
								}

								bool callbackSuccessful = session->frameChunkCallback(dataBuffer[0], (PROTOCOL_BYTES_PER_FRAME * session->processedPacketsThisSession), numberBytesLeft, callbackBuffer, session->parent);

								if (callbackSuccessful)
								{
									for (std::uint8_t j = 0; j < PROTOCOL_BYTES_PER_FRAME; j++)
									{
										dataBuffer[1 + j] = callbackBuffer[j];
									}
								}
								else
								{
									abort_session(session, ConnectionAbortReason::AnyOtherError);
									close_session(session, false);
									sessionStillValid = false;
									break;
								}
							}
							else
							{
								// Use the data buffer to get the data for this frame
								for (std::uint8_t j = 0; j < PROTOCOL_BYTES_PER_FRAME; j++)
								{
									std::uint32_t index = (j + (PROTOCOL_BYTES_PER_FRAME * session->processedPacketsThisSession));
									if (index < session->get_message_data_length())
									{
										dataBuffer[1 + j] = session->sessionMessage.get_data()[j + (PROTOCOL_BYTES_PER_FRAME * session->processedPacketsThisSession)];
									}
									else
									{
										dataBuffer[1 + j] = 0xFF;
									}
								}
							}

							if (CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolData),
							                                                   dataBuffer,
							                                                   CAN_DATA_LENGTH,
							                                                   std::static_pointer_cast<InternalControlFunction>(session->sessionMessage.get_source_control_function()),
							                                                   session->sessionMessage.get_destination_control_function(),
							                                                   CANIdentifier::CANPriority::PriorityLowest7))
							{
								framesSentThisUpdate++;
								session->lastPacketNumber++;
								session->processedPacketsThisSession++;
								session->timestamp_ms = SystemTiming::get_timestamp_ms();

								if (nullptr == session->sessionMessage.get_destination_control_function())
								{
									// Need to wait for the frame delay time before continuing BAM session
									break;
								}
								else if (framesSentThisUpdate >= CANNetworkManager::CANNetwork.get_configuration().get_max_number_of_network_manager_protocol_frames_per_update())
								{
									break; // Throttle the session
								}
							}
							else
							{
								// Process more next time protocol is updated
								break;
							}
						}
					}

					if (sessionStillValid)
					{
						if ((session->lastPacketNumber == (session->packetCount)) &&
						    (session->get_message_data_length() <= (PROTOCOL_BYTES_PER_FRAME * session->processedPacketsThisSession)))
						{
							if (nullptr == session->sessionMessage.get_destination_control_function())
							{
								// BAM is complete
								close_session(session, true);
							}
							else
							{
								set_state(session, StateMachineState::WaitForEndOfMessageAcknowledge);
								session->timestamp_ms = SystemTiming::get_timestamp_ms();
							}
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
							CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[TP]: BAM Rx Timeout");
							close_session(session, false);
						}
					}
					else
					{
						// CM TP Timeout check
						if (SystemTiming::time_expired_ms(session->timestamp_ms, MESSAGE_TR_TIMEOUT_MS))
						{
							CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[TP]: CM Rx Timeout");
							abort_session(session, ConnectionAbortReason::Timeout);
							close_session(session, false);
						}
					}
				}
				break;
			}
		}
	}

}
