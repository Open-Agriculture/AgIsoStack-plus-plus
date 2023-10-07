//================================================================================================
/// @file can_extended_transport_protocol.cpp
///
/// @brief A protocol class that handles the ISO11783 extended transport protocol.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#include "isobus/isobus/can_extended_transport_protocol.hpp"

#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_configuration.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

#include <algorithm>

namespace isobus
{
	ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::ExtendedTransportProtocolSession(Direction sessionDirection, std::uint8_t canPortIndex) :
	  sessionMessage(canPortIndex),
	  sessionDirection(sessionDirection)
	{
	}

	bool ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::operator==(const ExtendedTransportProtocolSession &obj)
	{
		return ((sessionMessage.get_source_control_function() == obj.sessionMessage.get_source_control_function()) &&
		        (sessionMessage.get_destination_control_function() == obj.sessionMessage.get_destination_control_function()) &&
		        (sessionMessage.get_identifier().get_parameter_group_number() == obj.sessionMessage.get_identifier().get_parameter_group_number()));
	}

	std::uint32_t ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::get_message_data_length() const
	{
		if (nullptr != frameChunkCallback)
		{
			return frameChunkCallbackMessageLength;
		}
		return sessionMessage.get_data_length();
	}

	ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::~ExtendedTransportProtocolSession()
	{
	}

	ExtendedTransportProtocolManager::ExtendedTransportProtocolManager()
	{
	}

	ExtendedTransportProtocolManager ::~ExtendedTransportProtocolManager()
	{
		// No need to clean up, as this object is a member of the network manager
		// so its callbacks will be cleared at destruction time
	}

	void ExtendedTransportProtocolManager::initialize(CANLibBadge<CANNetworkManager>)
	{
		if (!initialized)
		{
			initialized = true;
			CANNetworkManager::CANNetwork.add_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolDataTransfer), process_message, this);
			CANNetworkManager::CANNetwork.add_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement), process_message, this);
		}
	}

	void ExtendedTransportProtocolManager::process_message(const CANMessage &message)
	{
		if ((nullptr != CANNetworkManager::CANNetwork.get_internal_control_function(message.get_destination_control_function())))
		{
			switch (message.get_identifier().get_parameter_group_number())
			{
				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement):
				{
					if (CAN_DATA_LENGTH == message.get_data_length())
					{
						ExtendedTransportProtocolSession *session;
						const auto &data = message.get_data();
						const std::uint32_t pgn = (static_cast<std::uint32_t>(data[5]) | (static_cast<std::uint32_t>(data[6]) << 8) | (static_cast<std::uint32_t>(data[7]) << 16));

						switch (message.get_data()[0])
						{
							case EXTENDED_REQUEST_TO_SEND_MULTIPLEXOR:
							{
								if ((nullptr != message.get_destination_control_function()) &&
								    (activeSessions.size() < CANNetworkManager::CANNetwork.get_configuration().get_max_number_transport_protocol_sessions()) &&
								    (!get_session(session, message.get_source_control_function(), message.get_destination_control_function(), pgn)))
								{
									ExtendedTransportProtocolSession *newSession = new ExtendedTransportProtocolSession(ExtendedTransportProtocolSession::Direction::Receive, message.get_can_port_index());
									CANIdentifier tempIdentifierData(CANIdentifier::Type::Extended, pgn, CANIdentifier::CANPriority::PriorityLowest7, message.get_destination_control_function()->get_address(), message.get_source_control_function()->get_address());
									newSession->sessionMessage.set_data_size(static_cast<std::uint32_t>(data[1]) | static_cast<std::uint32_t>(data[2] << 8) | static_cast<std::uint32_t>(data[3] << 16) | static_cast<std::uint32_t>(data[4] << 24));
									newSession->sessionMessage.set_source_control_function(message.get_source_control_function());
									newSession->sessionMessage.set_destination_control_function(message.get_destination_control_function());
									newSession->packetCount = 0xFF;
									newSession->sessionMessage.set_identifier(tempIdentifierData);
									newSession->state = StateMachineState::ClearToSend;
									newSession->timestamp_ms = SystemTiming::get_timestamp_ms();
									activeSessions.push_back(newSession);
								}
								else if ((get_session(session, message.get_source_control_function(), message.get_destination_control_function(), pgn)) &&
								         (nullptr != message.get_destination_control_function()) &&
								         (ControlFunction::Type::Internal == message.get_destination_control_function()->get_type()))
								{
									abort_session(pgn, ConnectionAbortReason::AlreadyInConnectionManagedSessionAndCannotSupportAnother, std::static_pointer_cast<InternalControlFunction>(message.get_destination_control_function()), message.get_source_control_function());
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[ETP]: Sent abort to address " + isobus::to_string(static_cast<int>(message.get_source_control_function()->get_address())) + " RTS when already in session");
									close_session(session, false);
								}
								else if ((activeSessions.size() >= CANNetworkManager::CANNetwork.get_configuration().get_max_number_transport_protocol_sessions()) &&
								         (nullptr != message.get_destination_control_function()) &&
								         (ControlFunction::Type::Internal == message.get_destination_control_function()->get_type()))
								{
									abort_session(pgn, ConnectionAbortReason::SystemResourcesNeededForAnotherTask, std::static_pointer_cast<InternalControlFunction>(message.get_destination_control_function()), message.get_source_control_function());
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[ETP]: Sent abort to address " + isobus::to_string(static_cast<int>(message.get_source_control_function()->get_address())) + " No Sessions Available");
									close_session(session, false);
								}
							}
							break;

							case EXTENDED_CLEAR_TO_SEND_MULTIPLEXOR:
							{
								const std::uint8_t packetsToBeSent = data[1];

								if (get_session(session, message.get_destination_control_function(), message.get_source_control_function(), pgn))
								{
									if (StateMachineState::WaitForClearToSend == session->state)
									{
										session->packetCount = packetsToBeSent;

										if (session->packetCount > CANNetworkManager::CANNetwork.get_configuration().get_max_number_of_etp_frames_per_edpo())
										{
											session->packetCount = CANNetworkManager::CANNetwork.get_configuration().get_max_number_of_etp_frames_per_edpo();
										}
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
										abort_session(pgn, ConnectionAbortReason::ClearToSendReceivedWhenDataTransferInProgress, std::static_pointer_cast<InternalControlFunction>(message.get_destination_control_function()), message.get_source_control_function());
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[ETP]: Sent abort to address " + isobus::to_string(static_cast<int>(message.get_source_control_function()->get_address())) + " CTS while in data session");
										close_session(session, false);
									}
								}
								else
								{
									// We got a CTS but no session exists. Aborting clears up the situation faster than waiting for them to timeout
									// In the case of Rx'ing a CTS, we're the source in the session
									abort_session(pgn, ConnectionAbortReason::AnyOtherReason, std::static_pointer_cast<InternalControlFunction>(message.get_destination_control_function()), message.get_source_control_function());
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[ETP]: Sent abort to address " + isobus::to_string(static_cast<int>(message.get_source_control_function()->get_address())) + " CTS With no matching session");
								}
							}
							break;

							case EXTENDED_DATA_PACKET_OFFSET_MULTIPLEXOR:
							{
								const std::uint32_t dataPacketOffset = (static_cast<std::uint32_t>(data[2]) | (static_cast<std::uint32_t>(data[3]) << 8) | (static_cast<std::uint32_t>(data[4]) << 16));

								if (get_session(session, message.get_source_control_function(), message.get_destination_control_function(), pgn))
								{
									const std::uint8_t packetsToBeSent = data[1];

									if (packetsToBeSent != session->packetCount)
									{
										if (packetsToBeSent > session->packetCount)
										{
											CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[ETP]: Sent abort to address " + isobus::to_string(static_cast<int>(message.get_source_control_function()->get_address())) + " DPO packet count is greater than CTS");
											abort_session(session, ConnectionAbortReason::EDPONumberOfPacketsGreaterThanClearToSend);
											close_session(session, false);
										}
										else
										{
											/// @note If byte 2 is less than byte 2 of the ETP.CM_CTS message, then the receiver shall make
											/// necessary adjustments to its session to accept the data block defined by the
											/// ETP.CM_DPO message and the subsequent ETP.DT packets.
											CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[ETP]: DPO packet count disagrees with CTS. Using DPO value.");
											session->packetCount = packetsToBeSent;
										}
									}

									if (dataPacketOffset == session->processedPacketsThisSession)
									{
										// All is good. Proceed with message.
										session->lastPacketNumber = 0;
										set_state(session, StateMachineState::RxDataSession);
									}
									else
									{
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[ETP]: Sent abort to address " + isobus::to_string(static_cast<int>(message.get_source_control_function()->get_address())) + " DPO packet offset is not valid");
										abort_session(session, ConnectionAbortReason::BadEDPOOffset);
										close_session(session, false);
									}
								}
								else
								{
									bool anySessionMatched = false;
									// Do we have any session that matches except for PGN?
									for (auto currentSession : activeSessions)
									{
										if ((currentSession->sessionMessage.get_source_control_function() == message.get_source_control_function()) &&
										    (currentSession->sessionMessage.get_destination_control_function() == message.get_destination_control_function()))
										{
											// Sending EDPO for this session with mismatched PGN is not allowed
											CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[ETP]: Sent abort to address " + isobus::to_string(static_cast<int>(message.get_source_control_function()->get_address())) + " EDPO for this session with mismatched PGN is not allowed");
											abort_session(currentSession, ConnectionAbortReason::UnexpectedEDPOPgn);
											close_session(session, false);
											anySessionMatched = true;
											break;
										}
									}

									if (!anySessionMatched)
									{
										abort_session(pgn, ConnectionAbortReason::UnexpectedEDPOPacket, std::static_pointer_cast<InternalControlFunction>(message.get_destination_control_function()), message.get_source_control_function());
									}
								}
							}
							break;

							case EXTENDED_END_OF_MESSAGE_ACKNOWLEDGEMENT:
							{
								if ((nullptr != message.get_destination_control_function()) &&
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
											abort_session(pgn, ConnectionAbortReason::AnyOtherReason, std::static_pointer_cast<InternalControlFunction>(message.get_destination_control_function()), message.get_source_control_function());
											close_session(session, false);
											CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[ETP]: Sent abort to address " + isobus::to_string(static_cast<int>(message.get_source_control_function()->get_address())) + " received EOM in wrong session state");
										}
									}
									else
									{
										abort_session(pgn, ConnectionAbortReason::AnyOtherReason, std::static_pointer_cast<InternalControlFunction>(message.get_destination_control_function()), message.get_source_control_function());
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[ETP]: Sent abort to address " + isobus::to_string(static_cast<int>(message.get_source_control_function()->get_address())) + " EOM without matching session");
									}
								}
								else
								{
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[ETP]: Bad EOM received, sent to or from an invalid control function");
								}
							}
							break;

							case EXTENDED_CONNECTION_ABORT_MULTIPLEXOR:
							{
								if (get_session(session, message.get_destination_control_function(), message.get_source_control_function(), pgn))
								{
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[ETP]: Received an abort for an session with PGN: " + isobus::to_string(pgn));
									close_session(session, false);
								}
								else
								{
									CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[ETP]: Received an abort with no matching session with PGN: " + isobus::to_string(pgn));
								}
							}
							break;

							default:
							{
							}
							break;
						}
					}
					else
					{
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[ETP]: Received an invalid ETP CM frame");
					}
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolDataTransfer):
				{
					ExtendedTransportProtocolSession *tempSession = nullptr;
					auto &messageData = message.get_data();

					if ((CAN_DATA_LENGTH == message.get_data_length()) &&
					    (get_session(tempSession, message.get_source_control_function(), message.get_destination_control_function())) &&
					    (StateMachineState::RxDataSession == tempSession->state) &&
					    (messageData[SEQUENCE_NUMBER_DATA_INDEX] == (tempSession->lastPacketNumber + 1)))
					{
						for (std::uint8_t i = SEQUENCE_NUMBER_DATA_INDEX; (i < PROTOCOL_BYTES_PER_FRAME) && (((PROTOCOL_BYTES_PER_FRAME * tempSession->processedPacketsThisSession) + i) < tempSession->get_message_data_length()); i++)
						{
							std::uint32_t currentDataIndex = (PROTOCOL_BYTES_PER_FRAME * tempSession->processedPacketsThisSession) + i;
							tempSession->sessionMessage.set_data(messageData[1 + SEQUENCE_NUMBER_DATA_INDEX + i], currentDataIndex);
						}
						tempSession->lastPacketNumber++;
						tempSession->processedPacketsThisSession++;
						if ((tempSession->processedPacketsThisSession * PROTOCOL_BYTES_PER_FRAME) >= tempSession->get_message_data_length())
						{
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
					else
					{
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[ETP]: Received an unexpected or invalid data transfer frame");
					}
				}
				break;

				default:
					break;
			}
		}
	}

	void ExtendedTransportProtocolManager::process_message(const CANMessage &message, void *parent)
	{
		if (nullptr != parent)
		{
			reinterpret_cast<ExtendedTransportProtocolManager *>(parent)->process_message(message);
		}
	}

	bool ExtendedTransportProtocolManager::protocol_transmit_message(std::uint32_t parameterGroupNumber,
	                                                                 const std::uint8_t *dataBuffer,
	                                                                 std::uint32_t messageLength,
	                                                                 std::shared_ptr<ControlFunction> source,
	                                                                 std::shared_ptr<ControlFunction> destination,
	                                                                 TransmitCompleteCallback sessionCompleteCallback,
	                                                                 void *parentPointer,
	                                                                 DataChunkCallback frameChunkCallback)
	{
		ExtendedTransportProtocolSession *session;
		bool retVal = false;

		if ((messageLength < MAX_PROTOCOL_DATA_LENGTH) &&
		    (messageLength >= MIN_PROTOCOL_DATA_LENGTH) &&
		    (nullptr != destination) &&
		    ((nullptr != dataBuffer) ||
		     (nullptr != frameChunkCallback)) &&
		    (nullptr != source) &&
		    (true == source->get_address_valid()) &&
		    (destination->get_address_valid()) &&
		    (!get_session(session, source, destination, parameterGroupNumber)))
		{
			ExtendedTransportProtocolSession *newSession = new ExtendedTransportProtocolSession(ExtendedTransportProtocolSession::Direction::Transmit,
			                                                                                    source->get_can_port());

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
			CANIdentifier messageVirtualID(CANIdentifier::Type::Extended,
			                               parameterGroupNumber,
			                               CANIdentifier::CANPriority::PriorityDefault6,
			                               destination->get_address(),
			                               source->get_address());

			newSession->sessionMessage.set_identifier(messageVirtualID);
			set_state(newSession, StateMachineState::RequestToSend);
			activeSessions.push_back(newSession);
			CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Debug, "[ETP]: New ETP Session. Dest: " + isobus::to_string(static_cast<int>(destination->get_address())));
			retVal = true;
		}
		return retVal;
	}

	void ExtendedTransportProtocolManager::update(CANLibBadge<CANNetworkManager>)
	{
		for (auto i : activeSessions)
		{
			update_state_machine(i);
		}
	}

	bool ExtendedTransportProtocolManager::abort_session(ExtendedTransportProtocolSession *session, ConnectionAbortReason reason)
	{
		bool retVal = false;

		if (nullptr != session)
		{
			std::shared_ptr<InternalControlFunction> myControlFunction;
			std::shared_ptr<ControlFunction> partnerControlFunction;
			std::array<std::uint8_t, CAN_DATA_LENGTH> data;
			std::uint32_t pgn = session->sessionMessage.get_identifier().get_parameter_group_number();

			if (ExtendedTransportProtocolSession::Direction::Transmit == session->sessionDirection)
			{
				myControlFunction = CANNetworkManager::CANNetwork.get_internal_control_function(session->sessionMessage.get_source_control_function());
				partnerControlFunction = session->sessionMessage.get_destination_control_function();
			}
			else
			{
				myControlFunction = CANNetworkManager::CANNetwork.get_internal_control_function(session->sessionMessage.get_destination_control_function());
				partnerControlFunction = session->sessionMessage.get_source_control_function();
			}

			data[0] = EXTENDED_CONNECTION_ABORT_MULTIPLEXOR;
			data[1] = static_cast<std::uint8_t>(reason);
			data[2] = 0xFF;
			data[3] = 0xFF;
			data[4] = 0xFF;
			data[5] = static_cast<std::uint8_t>(pgn & 0xFF);
			data[6] = static_cast<std::uint8_t>((pgn >> 8) & 0xFF);
			data[7] = static_cast<std::uint8_t>((pgn >> 16) & 0xFF);
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement),
			                                                        data.data(),
			                                                        CAN_DATA_LENGTH,
			                                                        myControlFunction,
			                                                        partnerControlFunction,
			                                                        CANIdentifier::CANPriority::PriorityLowest7);
		}
		return retVal;
	}

	bool ExtendedTransportProtocolManager::abort_session(std::uint32_t parameterGroupNumber, ConnectionAbortReason reason, std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination)
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> data;

		data[0] = EXTENDED_CONNECTION_ABORT_MULTIPLEXOR;
		data[1] = static_cast<std::uint8_t>(reason);
		data[2] = 0xFF;
		data[3] = 0xFF;
		data[4] = 0xFF;
		data[5] = static_cast<std::uint8_t>(parameterGroupNumber & 0xFF);
		data[6] = static_cast<std::uint8_t>((parameterGroupNumber >> 8) & 0xFF);
		data[7] = static_cast<std::uint8_t>((parameterGroupNumber >> 16) & 0xFF);
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement),
		                                                      data.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      source,
		                                                      destination,
		                                                      CANIdentifier::CANPriority::PriorityLowest7);
	}

	void ExtendedTransportProtocolManager::close_session(ExtendedTransportProtocolSession *session, bool successfull)
	{
		if (nullptr != session)
		{
			process_session_complete_callback(session, successfull);
			auto sessionLocation = std::find(activeSessions.begin(), activeSessions.end(), session);
			if (activeSessions.end() != sessionLocation)
			{
				activeSessions.erase(sessionLocation);
				delete session;
				CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Debug, "[ETP]: Session Closed");
			}
		}
	}

	bool ExtendedTransportProtocolManager::get_session(ExtendedTransportProtocolSession *&session, std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination) const
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

	bool ExtendedTransportProtocolManager::get_session(ExtendedTransportProtocolSession *&session, std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination, std::uint32_t parameterGroupNumber) const
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

	void ExtendedTransportProtocolManager::process_session_complete_callback(ExtendedTransportProtocolSession *session, bool success)
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

	bool ExtendedTransportProtocolManager::send_end_of_session_acknowledgement(ExtendedTransportProtocolSession *session) const
	{
		bool retVal = false;

		if (nullptr != session)
		{
			std::uint32_t totalBytesTransferred = (session->get_message_data_length());
			const std::uint8_t dataBuffer[CAN_DATA_LENGTH] = { EXTENDED_END_OF_MESSAGE_ACKNOWLEDGEMENT,
				                                                 static_cast<std::uint8_t>(totalBytesTransferred & 0xFF),
				                                                 static_cast<std::uint8_t>((totalBytesTransferred >> 8) & 0xFF),
				                                                 static_cast<std::uint8_t>((totalBytesTransferred >> 16) & 0xFF),
				                                                 static_cast<std::uint8_t>((totalBytesTransferred >> 24) & 0xFF),
				                                                 static_cast<std::uint8_t>(session->sessionMessage.get_identifier().get_parameter_group_number() & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 8) & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 16) & 0xFF) };
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement),
			                                                        dataBuffer,
			                                                        CAN_DATA_LENGTH,
			                                                        std::static_pointer_cast<InternalControlFunction>(session->sessionMessage.get_destination_control_function()),
			                                                        session->sessionMessage.get_source_control_function(),
			                                                        CANIdentifier::CANPriority::PriorityDefault6);
		}
		return retVal;
	}

	bool ExtendedTransportProtocolManager::send_extended_connection_mode_clear_to_send(ExtendedTransportProtocolSession *session) const
	{
		bool retVal = false;

		if (nullptr != session)
		{
			std::uint32_t packetMax = ((((session->get_message_data_length() - 1) / 7) + 1) - session->processedPacketsThisSession);

			if (packetMax > 0xFF)
			{
				packetMax = 0xFF;
			}

			if (packetMax < session->packetCount)
			{
				session->packetCount = packetMax; // If we're sending a CTS with less than 0xFF, set the expected packet count to the CTS packet count
			}

			const std::uint8_t dataBuffer[CAN_DATA_LENGTH] = { EXTENDED_CLEAR_TO_SEND_MULTIPLEXOR,
				                                                 static_cast<std::uint8_t>(packetMax), /// @todo Make CTS Max user configurable
				                                                 static_cast<std::uint8_t>((session->processedPacketsThisSession + 1) & 0xFF),
				                                                 static_cast<std::uint8_t>(((session->processedPacketsThisSession + 1) >> 8) & 0xFF),
				                                                 static_cast<std::uint8_t>(((session->processedPacketsThisSession + 1) >> 16) & 0xFF),
				                                                 static_cast<std::uint8_t>(session->sessionMessage.get_identifier().get_parameter_group_number() & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 8) & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 16) & 0xFF) };
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement),
			                                                        dataBuffer,
			                                                        CAN_DATA_LENGTH,
			                                                        std::static_pointer_cast<InternalControlFunction>(session->sessionMessage.get_destination_control_function()),
			                                                        session->sessionMessage.get_source_control_function(),
			                                                        CANIdentifier::CANPriority::PriorityDefault6);
		}
		return retVal;
	}

	bool ExtendedTransportProtocolManager::send_extended_connection_mode_request_to_send(const ExtendedTransportProtocolSession *session) const
	{
		bool retVal = false;

		if (nullptr != session)
		{
			const std::uint8_t dataBuffer[CAN_DATA_LENGTH] = { EXTENDED_REQUEST_TO_SEND_MULTIPLEXOR,
				                                                 static_cast<std::uint8_t>(session->get_message_data_length() & 0xFF),
				                                                 static_cast<std::uint8_t>((session->get_message_data_length() >> 8) & 0xFF),
				                                                 static_cast<std::uint8_t>((session->get_message_data_length() >> 16) & 0xFF),
				                                                 static_cast<std::uint8_t>((session->get_message_data_length() >> 24) & 0xFF),
				                                                 static_cast<std::uint8_t>(session->sessionMessage.get_identifier().get_parameter_group_number() & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 8) & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 16) & 0xFF) };
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement),
			                                                        dataBuffer,
			                                                        CAN_DATA_LENGTH,
			                                                        std::static_pointer_cast<InternalControlFunction>(session->sessionMessage.get_source_control_function()),
			                                                        session->sessionMessage.get_destination_control_function(),
			                                                        CANIdentifier::CANPriority::PriorityDefault6);
		}
		return retVal;
	}

	bool ExtendedTransportProtocolManager::send_extended_connection_mode_data_packet_offset(const ExtendedTransportProtocolSession *session) const
	{
		bool retVal = false;

		if (nullptr != session)
		{
			const std::uint8_t dataBuffer[CAN_DATA_LENGTH] = { EXTENDED_DATA_PACKET_OFFSET_MULTIPLEXOR,
				                                                 static_cast<std::uint8_t>(session->packetCount & 0xFF),
				                                                 static_cast<std::uint8_t>((session->processedPacketsThisSession) & 0xFF),
				                                                 static_cast<std::uint8_t>((session->processedPacketsThisSession >> 8) & 0xFF),
				                                                 static_cast<std::uint8_t>((session->processedPacketsThisSession >> 16) & 0xFF),
				                                                 static_cast<std::uint8_t>(session->sessionMessage.get_identifier().get_parameter_group_number() & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 8) & 0xFF),
				                                                 static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 16) & 0xFF) };
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement),
			                                                        dataBuffer,
			                                                        CAN_DATA_LENGTH,
			                                                        std::static_pointer_cast<InternalControlFunction>(session->sessionMessage.get_source_control_function()),
			                                                        session->sessionMessage.get_destination_control_function(),
			                                                        CANIdentifier::CANPriority::PriorityDefault6);
		}
		return retVal;
	}

	void ExtendedTransportProtocolManager::set_state(ExtendedTransportProtocolSession *session, StateMachineState value)
	{
		if (nullptr != session)
		{
			session->timestamp_ms = SystemTiming::get_timestamp_ms();
			session->state = value;
		}
	}

	void ExtendedTransportProtocolManager::update_state_machine(ExtendedTransportProtocolSession *session)
	{
		if (nullptr != session)
		{
			switch (session->state)
			{
				case StateMachineState::RequestToSend:
				{
					if (send_extended_connection_mode_request_to_send(session))
					{
						set_state(session, StateMachineState::WaitForClearToSend);
					}
					else
					{
						if (SystemTiming::time_expired_ms(session->timestamp_ms, T2_3_TIMEOUT_MS))
						{
							CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[ETP]: Aborting session, T2-3 timeout reached while in RTS state");
							abort_session(session, ConnectionAbortReason::Timeout);
							close_session(session, false);
						}
					}
				}
				break;

				case StateMachineState::WaitForEndOfMessageAcknowledge:
				case StateMachineState::WaitForExtendedDataPacketOffset:
				case StateMachineState::WaitForClearToSend:
				{
					if (SystemTiming::time_expired_ms(session->timestamp_ms, T2_3_TIMEOUT_MS))
					{
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[ETP]: Aborting session, T2-3 timeout reached while waiting for CTS");
						abort_session(session, ConnectionAbortReason::Timeout);
						close_session(session, false);
					}
				}
				break;

				case StateMachineState::TxDataSession:
				{
					if (nullptr != session->sessionMessage.get_destination_control_function())
					{
						std::uint8_t dataBuffer[CAN_DATA_LENGTH];
						bool proceedToSendDataPackets = true;
						bool sessionStillValid = true;

						if (0 == session->lastPacketNumber)
						{
							proceedToSendDataPackets = send_extended_connection_mode_data_packet_offset(session);
						}

						if (proceedToSendDataPackets)
						{
							std::uint32_t framesSentThisUpdate = 0;
							// Try and send packets
							for (std::uint32_t i = session->lastPacketNumber; i < session->packetCount; i++)
							{
								dataBuffer[0] = (session->lastPacketNumber + 1);

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
									std::uint32_t numberBytesLeft = (session->get_message_data_length() - (PROTOCOL_BYTES_PER_FRAME * session->processedPacketsThisSession));

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
										CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[ETP]: Aborting session, unable to transfer chunk of data (numberBytesLeft=" + to_string(numberBytesLeft) + ")");
										abort_session(session, ConnectionAbortReason::AnyOtherReason);
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

								if (CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolDataTransfer),
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

									if (framesSentThisUpdate >= CANNetworkManager::CANNetwork.get_configuration().get_max_number_of_network_manager_protocol_frames_per_update())
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
				}
				break;

				case StateMachineState::RxDataSession:
				{
					if (session->packetCount == session->lastPacketNumber)
					{
						set_state(session, StateMachineState::ClearToSend);
					}
					else if (SystemTiming::time_expired_ms(session->timestamp_ms, T1_TIMEOUT_MS))
					{
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[ETP]: Aborting session, RX T1 timeout reached");
						abort_session(session, ConnectionAbortReason::Timeout);
						close_session(session, false);
					}
				}
				break;

				case StateMachineState::ClearToSend:
				{
					if (send_extended_connection_mode_clear_to_send(session))
					{
						set_state(session, StateMachineState::WaitForExtendedDataPacketOffset);
					}
					else if (SystemTiming::time_expired_ms(session->timestamp_ms, T2_3_TIMEOUT_MS))
					{
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[ETP]: Aborting session, T2-3 timeout reached while in CTS state");
						abort_session(session, ConnectionAbortReason::Timeout);
						close_session(session, false);
					}
				}
				break;

				default:
				{
					// Nothing to do.
				}
				break;
			}
		}
	}

} // namespace isobus
