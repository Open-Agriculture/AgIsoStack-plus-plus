//================================================================================================
/// @file nmea2000_fast_packet_protocol.cpp
///
/// @brief A protocol that handles the NMEA 2000 fast packet protocol.
///
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#include "isobus/isobus/nmea2000_fast_packet_protocol.hpp"

#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"

#include <algorithm>

namespace isobus
{
	FastPacketProtocol FastPacketProtocol::Protocol;

	FastPacketProtocol::FastPacketProtocolSession::FastPacketProtocolSession(Direction sessionDirection, std::uint8_t canPortIndex) :
	  sessionMessage(canPortIndex),
	  sessionCompleteCallback(nullptr),
	  frameChunkCallback(nullptr),
	  timestamp_ms(0),
	  lastPacketNumber(0),
	  packetCount(0),
	  processedPacketsThisSession(0),
	  sequenceNumber(0),
	  sessionDirection(sessionDirection)
	{
	}

	bool FastPacketProtocol::FastPacketProtocolSession::operator==(const FastPacketProtocolSession &obj)
	{
		return ((sessionMessage.get_source_control_function() == obj.sessionMessage.get_source_control_function()) &&
		        (sessionMessage.get_destination_control_function() == obj.sessionMessage.get_destination_control_function()) &&
		        (sessionMessage.get_identifier().get_parameter_group_number() == obj.sessionMessage.get_identifier().get_parameter_group_number()));
	}

	FastPacketProtocol::FastPacketProtocolSession::~FastPacketProtocolSession()
	{
	}

	void FastPacketProtocol::initialize(CANLibBadge<CANNetworkManager>)
	{
		if (!initialized)
		{
			initialized = true;
		}
	}

	void FastPacketProtocol::register_multipacket_message_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent)
	{
		parameterGroupNumberCallbacks.push_back(ParameterGroupNumberCallbackData(parameterGroupNumber, callback, parent));
		CANNetworkManager::CANNetwork.add_protocol_parameter_group_number_callback(parameterGroupNumber, process_message, this);
	}

	void FastPacketProtocol::remove_multipacket_message_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent)
	{
		ParameterGroupNumberCallbackData tempObject(parameterGroupNumber, callback, parent);
		auto callbackLocation = std::find(parameterGroupNumberCallbacks.begin(), parameterGroupNumberCallbacks.end(), tempObject);
		if (parameterGroupNumberCallbacks.end() != callbackLocation)
		{
			parameterGroupNumberCallbacks.erase(callbackLocation);
		}
		CANNetworkManager::CANNetwork.remove_protocol_parameter_group_number_callback(parameterGroupNumber, process_message, this);
	}

	bool FastPacketProtocol::send_multipacket_message(std::uint32_t parameterGroupNumber,
	                                                  const std::uint8_t *data,
	                                                  std::uint8_t messageLength,
	                                                  InternalControlFunction *source,
	                                                  ControlFunction *destination,
	                                                  CANIdentifier::CANPriority priority,
	                                                  TransmitCompleteCallback txCompleteCallback,
	                                                  void *parentPointer,
	                                                  DataChunkCallback frameChunkCallback)
	{
		bool retVal = false;

		if ((nullptr != source) &&
		    (source->get_address_valid()) &&
		    (parameterGroupNumber >= FP_MIN_PARAMETER_GROUP_NUMBER) &&
		    (parameterGroupNumber <= FP_MAX_PARAMETER_GROUP_NUMBER) &&
		    (messageLength <= MAX_PROTOCOL_MESSAGE_LENGTH) &&
		    ((nullptr != data) ||
		     (nullptr != frameChunkCallback)))
		{
			FastPacketProtocolSession *tempSession = nullptr;

			if (!get_session(tempSession, parameterGroupNumber, source, destination))
			{
				tempSession = new FastPacketProtocolSession(FastPacketProtocolSession::Direction::Transmit, source->get_can_port());
				tempSession->sessionMessage.set_source_control_function(source);
				tempSession->sessionMessage.set_destination_control_function(destination);
				tempSession->sessionMessage.set_identifier(CANIdentifier(CANIdentifier::Type::Extended, parameterGroupNumber, priority, (destination == nullptr ? 0xFF : destination->get_address()), source->get_address()));
				tempSession->sessionMessage.set_data(data, messageLength);
				tempSession->frameChunkCallback = frameChunkCallback;
				tempSession->parent = parentPointer;
				tempSession->packetCount = ((messageLength - 6) / PROTOCOL_BYTES_PER_FRAME);
				tempSession->timestamp_ms = SystemTiming::get_timestamp_ms();
				tempSession->processedPacketsThisSession = 0;
				tempSession->sessionCompleteCallback = txCompleteCallback;
				tempSession->sequenceNumber = get_new_sequence_number(tempSession);

				if (0 != (messageLength % PROTOCOL_BYTES_PER_FRAME))
				{
					tempSession->packetCount++;
				}

				std::unique_lock<std::mutex> lock(sessionMutex);

				activeSessions.push_back(tempSession);
				retVal = true;
			}
			else
			{
				// Already in a matching session, can't start another.
				CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[FP]: Can't send fast packet message, already in matching session.");
			}
		}
		else
		{
			CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[FP]: Can't send fast packet message, bad parameters or ICF is invalid");
		}
		return retVal;
	}

	void FastPacketProtocol::update(CANLibBadge<CANNetworkManager>)
	{
		std::unique_lock<std::mutex> lock(sessionMutex);

		for (auto i : activeSessions)
		{
			update_state_machine(i);
		}
	}

	void FastPacketProtocol::add_session_history(FastPacketProtocolSession *session)
	{
		if (nullptr != session)
		{
			bool formerSessionMatched = false;

			for (std::size_t i = 0; i < sessionHistory.size(); i++)
			{
				if ((sessionHistory[i].isoName == session->sessionMessage.get_source_control_function()->get_NAME()) &&
				    (sessionHistory[i].parameterGroupNumber == session->sessionMessage.get_identifier().get_parameter_group_number()))
				{
					sessionHistory[i].sequenceNumber++;
					formerSessionMatched = true;
					break;
				}
			}

			if (!formerSessionMatched)
			{
				FastPacketHistory history{
					session->sessionMessage.get_source_control_function()->get_NAME(),
					session->sessionMessage.get_identifier().get_parameter_group_number(),
					session->sequenceNumber
				};
				history.sequenceNumber++;
				sessionHistory.push_back(history);
			}
		}
	}

	void FastPacketProtocol::close_session(FastPacketProtocolSession *session, bool successfull)
	{
		if (nullptr != session)
		{
			process_session_complete_callback(session, successfull);
			for (auto currentSession = activeSessions.begin(); currentSession != activeSessions.end(); currentSession++)
			{
				if (session == *currentSession)
				{
					activeSessions.erase(currentSession);
					delete session;
					break;
				}
			}
		}
	}

	std::uint8_t FastPacketProtocol::get_new_sequence_number(FastPacketProtocolSession *session)
	{
		std::uint8_t retVal = 0;

		if (nullptr != session)
		{
			for (auto formerSessions : sessionHistory)
			{
				if ((formerSessions.isoName == session->sessionMessage.get_source_control_function()->get_NAME()) &&
				    (formerSessions.parameterGroupNumber == session->sessionMessage.get_identifier().get_parameter_group_number()))
				{
					retVal = formerSessions.sequenceNumber;
					break;
				}
			}
		}
		return retVal;
	}

	bool FastPacketProtocol::get_session(FastPacketProtocolSession *&returnedSession, std::uint32_t parameterGroupNumber, ControlFunction *source, ControlFunction *destination)
	{
		returnedSession = nullptr;
		std::unique_lock<std::mutex> lock(sessionMutex);

		for (auto session : activeSessions)
		{
			if ((session->sessionMessage.get_identifier().get_parameter_group_number() == parameterGroupNumber) &&
			    (session->sessionMessage.get_source_control_function() == source) &&
			    (session->sessionMessage.get_destination_control_function() == destination))
			{
				returnedSession = session;
				break;
			}
		}
		return (nullptr != returnedSession);
	}

	void FastPacketProtocol::process_message(CANMessage *const message, void *parent)
	{
		if (nullptr != parent)
		{
			reinterpret_cast<FastPacketProtocol *>(parent)->process_message(message);
		}
	}

	void FastPacketProtocol::process_message(CANMessage *const message)
	{
		if ((nullptr != message) &&
		    (CAN_DATA_LENGTH == message->get_data_length()) &&
		    (message->get_identifier().get_parameter_group_number() >= FP_MIN_PARAMETER_GROUP_NUMBER) &&
		    (message->get_identifier().get_parameter_group_number() <= FP_MAX_PARAMETER_GROUP_NUMBER))
		{
			// See if we care about parsing this message
			if (parameterGroupNumberCallbacks.size() > 0)
			{
				bool pgnNeedsParsing = false;

				for (auto callback : parameterGroupNumberCallbacks)
				{
					if (callback.get_parameter_group_number() == message->get_identifier().get_parameter_group_number())
					{
						pgnNeedsParsing = true;
						break;
					}
				}

				if (pgnNeedsParsing)
				{
					FastPacketProtocolSession *currentSession = nullptr;
					std::vector<std::uint8_t> messageData = message->get_data();
					std::uint8_t frameCount = (messageData[0] & FRAME_COUNTER_BIT_MASK);

					// Check for a valid session
					if (get_session(currentSession, message->get_identifier().get_parameter_group_number(), message->get_source_control_function(), message->get_destination_control_function()))
					{
						// Matched a session
						if (0 != frameCount)
						{
							// Continue processing the message
							for (std::uint8_t i = 0; i < PROTOCOL_BYTES_PER_FRAME; i++)
							{
								currentSession->sessionMessage.set_data(messageData[1 + i], i + (currentSession->processedPacketsThisSession * PROTOCOL_BYTES_PER_FRAME) - 1);
							}
							currentSession->processedPacketsThisSession++;

							// Currently counting one by index and one by value, so add 1 to expected packet count
							if (currentSession->processedPacketsThisSession >= currentSession->packetCount + 1)
							{
								// Complete
								// Find the appropriate callback and let them know
								for (auto callback : parameterGroupNumberCallbacks)
								{
									if (callback.get_parameter_group_number() == currentSession->sessionMessage.get_identifier().get_parameter_group_number())
									{
										callback.get_callback()(&currentSession->sessionMessage, callback.get_parent());
									}
								}
								close_session(currentSession, true); // All done
							}
						}
						else
						{
							CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[FP]: Existing session matched new frame counter, aborting the matching session.");
							close_session(currentSession, false);
						}
					}
					else
					{
						// No matching session. See if we need to start a new session
						if (0 == frameCount)
						{
							if (messageData[1] <= MAX_PROTOCOL_MESSAGE_LENGTH)
							{
								// This is the beginning of a new message
								currentSession = new FastPacketProtocolSession(FastPacketProtocolSession::Direction::Receive, message->get_can_port_index());
								currentSession->frameChunkCallback = nullptr;
								if (messageData[1] >= PROTOCOL_BYTES_PER_FRAME - 1)
								{
									currentSession->packetCount = ((messageData[1] - 6) / PROTOCOL_BYTES_PER_FRAME);
								}
								else
								{
									currentSession->packetCount = 1;
								}
								currentSession->lastPacketNumber = ((messageData[0] >> SEQUENCE_NUMBER_BIT_OFFSET) & SEQUENCE_NUMBER_BIT_MASK);
								currentSession->processedPacketsThisSession = 1;
								currentSession->sessionMessage.set_data_size(messageData[1]);
								currentSession->sessionMessage.set_identifier(message->get_identifier());
								currentSession->sessionMessage.set_source_control_function(message->get_source_control_function());
								currentSession->sessionMessage.set_destination_control_function(message->get_destination_control_function());
								currentSession->timestamp_ms = SystemTiming::get_timestamp_ms();

								if (0 != (messageData[1] % PROTOCOL_BYTES_PER_FRAME))
								{
									currentSession->packetCount++;
								}

								// Save the 6 bytes of payload in this first message
								for (std::uint8_t i = 0; i < (PROTOCOL_BYTES_PER_FRAME - 1); i++)
								{
									currentSession->sessionMessage.set_data(messageData[2 + i], i);
								}

								std::unique_lock<std::mutex> lock(sessionMutex);

								activeSessions.push_back(currentSession);
							}
							else
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[FP]: Ignoring possible new FP session with advertised length > 233.");
							}
						}
						else
						{
							// This is the middle of some message that we have no context for.
							// Ignore the message.
							CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Warning, "[FP]: Ignoring FP message, no context available.");
						}
					}
				}
			}
		}
	}

	void FastPacketProtocol::process_session_complete_callback(FastPacketProtocolSession *session, bool success)
	{
		if ((nullptr != session) &&
		    (nullptr != session->sessionCompleteCallback) &&
		    (nullptr != session->sessionMessage.get_source_control_function()) &&
		    (ControlFunction::Type::Internal == session->sessionMessage.get_source_control_function()->get_type()))
		{
			session->sessionCompleteCallback(session->sessionMessage.get_identifier().get_parameter_group_number(),
			                                 session->sessionMessage.get_data_length(),
			                                 reinterpret_cast<InternalControlFunction *>(session->sessionMessage.get_source_control_function()),
			                                 session->sessionMessage.get_destination_control_function(),
			                                 success,
			                                 session->parent);
		}
	}

	bool FastPacketProtocol::protocol_transmit_message(std::uint32_t,
	                                                   const std::uint8_t *,
	                                                   std::uint32_t,
	                                                   ControlFunction *,
	                                                   ControlFunction *,
	                                                   TransmitCompleteCallback,
	                                                   void *,
	                                                   DataChunkCallback)
	{
		return false;
	}

	void FastPacketProtocol::update_state_machine(FastPacketProtocolSession *session)
	{
		if (nullptr != session)
		{
			switch (session->sessionDirection)
			{
				case FastPacketProtocolSession::Direction::Receive:
				{
					if (SystemTiming::time_expired_ms(session->timestamp_ms, FP_TIMEOUT_MS))
					{
						CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[FP]: Rx session timed out.");
						close_session(session, false);
					}
				}
				break;

				case FastPacketProtocolSession::Direction::Transmit:
				{
					std::array<std::uint8_t, CAN_DATA_LENGTH> dataBuffer;
					std::vector<std::uint8_t> messageData;
					bool txSessionCancelled = false;

					for (std::uint8_t i = session->processedPacketsThisSession; i <= session->packetCount; i++)
					{
						std::uint8_t bytesProcessedSoFar = (session->processedPacketsThisSession > 0 ? 6 : 0);

						if (0 != bytesProcessedSoFar)
						{
							bytesProcessedSoFar += (PROTOCOL_BYTES_PER_FRAME * (session->processedPacketsThisSession - 1));
						}

						std::uint16_t numberBytesLeft = (session->sessionMessage.get_data_length() - bytesProcessedSoFar);

						if (numberBytesLeft > PROTOCOL_BYTES_PER_FRAME)
						{
							numberBytesLeft = PROTOCOL_BYTES_PER_FRAME;
						}

						if (nullptr != session->frameChunkCallback)
						{
							std::uint8_t callbackBuffer[CAN_DATA_LENGTH] = { 0 }; // Only need 7 but give them 8 in case they make a mistake
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
								close_session(session, false);
								break;
							}
						}
						else
						{
							messageData = session->sessionMessage.get_data();
							if (0 == session->processedPacketsThisSession)
							{
								dataBuffer[0] = session->processedPacketsThisSession;
								dataBuffer[0] |= (session->sequenceNumber << SEQUENCE_NUMBER_BIT_OFFSET);
								dataBuffer[1] = session->sessionMessage.get_data_length();
								dataBuffer[2] = messageData[0];
								dataBuffer[3] = messageData[1];
								dataBuffer[4] = messageData[2];
								dataBuffer[5] = messageData[3];
								dataBuffer[6] = messageData[4];
								dataBuffer[7] = messageData[5];
							}
							else
							{
								dataBuffer[0] = session->processedPacketsThisSession;
								dataBuffer[0] |= (session->sequenceNumber << SEQUENCE_NUMBER_BIT_OFFSET);

								if (numberBytesLeft < PROTOCOL_BYTES_PER_FRAME)
								{
									dataBuffer[1] = 0xFF;
									dataBuffer[2] = 0xFF;
									dataBuffer[3] = 0xFF;
									dataBuffer[4] = 0xFF;
									dataBuffer[5] = 0xFF;
									dataBuffer[6] = 0xFF;
									dataBuffer[7] = 0xFF;
								}

								for (std::uint8_t j = 0; j < numberBytesLeft; j++)
								{
									dataBuffer[1 + j] = messageData[6 + ((i - 1) * PROTOCOL_BYTES_PER_FRAME) + j];
								}
							}
						}
						if (CANNetworkManager::CANNetwork.send_can_message(session->sessionMessage.get_identifier().get_parameter_group_number(),
						                                                   dataBuffer.data(),
						                                                   CAN_DATA_LENGTH,
						                                                   reinterpret_cast<InternalControlFunction *>(session->sessionMessage.get_source_control_function()),
						                                                   session->sessionMessage.get_destination_control_function(),
						                                                   session->sessionMessage.get_identifier().get_priority(),
						                                                   nullptr,
						                                                   nullptr))
						{
							session->processedPacketsThisSession++;
							session->timestamp_ms = SystemTiming::get_timestamp_ms();
						}
						else
						{
							if (SystemTiming::time_expired_ms(session->timestamp_ms, FP_TIMEOUT_MS))
							{
								CANStackLogger::CAN_stack_log(CANStackLogger::LoggingLevel::Error, "[FP]: Tx session timed out.");
								close_session(session, false);
								txSessionCancelled = true;
							}
							break;
						}
					}

					if ((!txSessionCancelled) &&
					    (session->processedPacketsThisSession >= session->packetCount))
					{
						add_session_history(session);
						close_session(session, true); // Session is done!
					}
				}
				break;
			}
		}
	}

} // namespace isobus
