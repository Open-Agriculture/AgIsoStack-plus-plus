//================================================================================================
/// @file nmea2000_fast_packet_protocol.cpp
///
/// @brief A protocol that handles the NMEA 2000 fast packet protocol.
///
/// @note This library and its authors are not affiliated with the National Marine
/// Electronics Association in any way.
///
/// @author Adrian Del Grosso
/// @author Daan Steenbergen
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================

#include "isobus/isobus/nmea2000_fast_packet_protocol.hpp"

#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"

#include <algorithm>
#include <cstring>

namespace isobus
{
	FastPacketProtocol::FastPacketProtocolSession::FastPacketProtocolSession(TransportProtocolSessionBase::Direction direction,
	                                                                         std::unique_ptr<CANMessageData> data,
	                                                                         std::uint32_t parameterGroupNumber,
	                                                                         std::uint16_t totalMessageSize,
	                                                                         std::uint8_t sequenceNumber,
	                                                                         CANIdentifier::CANPriority priority,
	                                                                         std::shared_ptr<ControlFunction> source,
	                                                                         std::shared_ptr<ControlFunction> destination,
	                                                                         TransmitCompleteCallback sessionCompleteCallback,
	                                                                         void *parentPointer) :
	  TransportProtocolSessionBase(direction, std::move(data), parameterGroupNumber, totalMessageSize, source, destination, sessionCompleteCallback, parentPointer),
	  sequenceNumber(sequenceNumber),
	  priority(priority)
	{
	}

	std::uint8_t FastPacketProtocol::FastPacketProtocolSession::get_message_length() const
	{
		// We know that this session can only be used to transfer 223 bytes of data, so we can safely cast to a uint8_t
		return static_cast<std::uint8_t>(TransportProtocolSessionBase::get_message_length());
	}

	bool FastPacketProtocol::FastPacketProtocolSession::is_broadcast() const
	{
		return (nullptr == get_destination());
	}

	std::uint32_t FastPacketProtocol::FastPacketProtocolSession::get_total_bytes_transferred() const
	{
		return numberOfBytesTransferred;
	}

	std::uint8_t FastPacketProtocol::FastPacketProtocolSession::get_last_packet_number() const
	{
		// We know that this session can only be used to transfer 223 bytes of data, so we can safely cast to a uint8_t
		std::uint8_t numberOfFrames = calculate_number_of_frames(static_cast<std::uint8_t>(get_total_bytes_transferred()));
		if (numberOfFrames > 0)
		{
			return numberOfFrames;
		}
		else
		{
			return 0;
		}
	}

	std::uint8_t FastPacketProtocol::FastPacketProtocolSession::get_number_of_remaining_packets() const
	{
		return get_total_number_of_packets() - get_last_packet_number();
	}

	std::uint8_t FastPacketProtocol::FastPacketProtocolSession::get_total_number_of_packets() const
	{
		return calculate_number_of_frames(get_message_length());
	}

	void FastPacketProtocol::FastPacketProtocolSession::add_number_of_bytes_transferred(std::uint8_t bytes)
	{
		numberOfBytesTransferred += bytes;
		update_timestamp();
	}

	std::uint8_t FastPacketProtocol::calculate_number_of_frames(std::uint8_t messageLength)
	{
		std::uint8_t numberOfFrames = 0;
		// Account for the 6 bytes of data in the first frame
		if (messageLength > 6)
		{
			messageLength -= 6;
			numberOfFrames++;
		}
		numberOfFrames += (messageLength / PROTOCOL_BYTES_PER_FRAME);
		if (0 != (messageLength % PROTOCOL_BYTES_PER_FRAME))
		{
			numberOfFrames++;
		}
		return numberOfFrames;
	}

	FastPacketProtocol::FastPacketProtocol(const CANMessageFrameCallback &sendCANFrameCallback) :
	  sendCANFrameCallback(sendCANFrameCallback)
	{
	}

	void FastPacketProtocol::register_multipacket_message_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent, std::shared_ptr<InternalControlFunction> internalControlFunction)
	{
		parameterGroupNumberCallbacks.emplace_back(parameterGroupNumber, callback, parent, internalControlFunction);
	}

	void FastPacketProtocol::remove_multipacket_message_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent, std::shared_ptr<InternalControlFunction> internalControlFunction)
	{
		ParameterGroupNumberCallbackData tempObject(parameterGroupNumber, callback, parent, internalControlFunction);
		auto callbackLocation = std::find(parameterGroupNumberCallbacks.begin(), parameterGroupNumberCallbacks.end(), tempObject);
		if (parameterGroupNumberCallbacks.end() != callbackLocation)
		{
			parameterGroupNumberCallbacks.erase(callbackLocation);
		}
	}

	void FastPacketProtocol::allow_any_control_function(bool allow)
	{
		allowAnyControlFunction = allow;
	}

	bool FastPacketProtocol::send_multipacket_message(std::uint32_t parameterGroupNumber,
	                                                  const std::uint8_t *messageData,
	                                                  std::uint8_t messageLength,
	                                                  std::shared_ptr<InternalControlFunction> source,
	                                                  std::shared_ptr<ControlFunction> destination,
	                                                  CANIdentifier::CANPriority priority,
	                                                  TransmitCompleteCallback txCompleteCallback,
	                                                  void *parentPointer,
	                                                  DataChunkCallback frameChunkCallback)
	{
		std::unique_ptr<CANMessageData> data;
		if (nullptr != frameChunkCallback)
		{
			data.reset(new CANMessageDataCallback(messageLength, frameChunkCallback, parentPointer));
		}
		else if (nullptr != messageData)
		{
			// Make a copy of the data as it could go out of scope
			data.reset(new CANMessageDataVector(messageData, messageLength));
		}

		// Return false early if we can't send the message
		if ((nullptr == data) || (data->size() <= CAN_DATA_LENGTH) || (data->size() > MAX_PROTOCOL_MESSAGE_LENGTH))
		{
			LOG_ERROR("[FP]: Unable to send multipacket message, data is invalid or has invalid length.");
			return false;
		}
		else if ((nullptr == source) || (!source->get_address_valid()) || has_session(parameterGroupNumber, source, destination))
		{
			LOG_ERROR("[FP]: Unable to send multipacket message, source is invalid or already in a session for the PGN.");
			return false;
		}
		else if ((parameterGroupNumber < FP_MIN_PARAMETER_GROUP_NUMBER) || (parameterGroupNumber > FP_MAX_PARAMETER_GROUP_NUMBER))
		{
			LOG_ERROR("[FP]: Unable to send multipacket message, PGN is unsupported by this protocol.");
			return false;
		}
		else if ((nullptr != destination) && (!destination->get_address_valid()))
		{
			LOG_ERROR("[FP]: Unable to send multipacket message, destination is invalid.");
			return false;
		}

		std::uint8_t sequenceNumber = get_new_sequence_number(source->get_NAME(), parameterGroupNumber);
		auto session = std::make_shared<FastPacketProtocolSession>(FastPacketProtocolSession::Direction::Transmit,
		                                                           std::move(data),
		                                                           parameterGroupNumber,
		                                                           messageLength,
		                                                           sequenceNumber,
		                                                           priority,
		                                                           source,
		                                                           destination,
		                                                           txCompleteCallback,
		                                                           parentPointer);

		LOCK_GUARD(Mutex, sessionMutex);
		activeSessions.push_back(session);
		return true;
	}

	void FastPacketProtocol::update()
	{
		LOCK_GUARD(Mutex, sessionMutex);
		// We use a fancy for loop here to allow us to remove sessions from the list while iterating
		for (std::size_t i = activeSessions.size(); i > 0; i--)
		{
			auto session = activeSessions.at(i - 1);
			if (!session->get_source()->get_address_valid())
			{
				LOG_WARNING("[FP]: Closing active session as the source control function is no longer valid");
				close_session(session, false);
			}
			else if (!session->is_broadcast() && !session->get_destination()->get_address_valid())
			{
				LOG_WARNING("[FP]: Closing active session as the destination control function is no longer valid");
				close_session(session, false);
			}
			update_session(session);
		}
	}

	void FastPacketProtocol::add_session_history(const std::shared_ptr<FastPacketProtocolSession> &session)
	{
		if (nullptr != session)
		{
			bool formerSessionMatched = false;

			for (auto &formerSessions : sessionHistory)
			{
				if ((formerSessions.isoName == session->get_source()->get_NAME()) &&
				    (formerSessions.parameterGroupNumber == session->get_parameter_group_number()))
				{
					formerSessions.sequenceNumber = session->sequenceNumber;
					formerSessionMatched = true;
					break;
				}
			}

			if (!formerSessionMatched)
			{
				FastPacketHistory history{
					session->get_source()->get_NAME(),
					session->get_parameter_group_number(),
					session->sequenceNumber
				};
				sessionHistory.push_back(history);
			}
		}
	}

	void FastPacketProtocol::close_session(std::shared_ptr<FastPacketProtocolSession> session, bool successful)
	{
		if (nullptr != session)
		{
			session->complete(successful);
			add_session_history(session);

			auto sessionLocation = std::find(activeSessions.begin(), activeSessions.end(), session);
			if (activeSessions.end() != sessionLocation)
			{
				activeSessions.erase(sessionLocation);
			}
		}
	}

	std::uint8_t FastPacketProtocol::get_new_sequence_number(NAME name, std::uint32_t parameterGroupNumber) const
	{
		std::uint8_t sequenceNumber = 0;
		for (const auto &formerSessions : sessionHistory)
		{
			if ((formerSessions.isoName == name) && (formerSessions.parameterGroupNumber == parameterGroupNumber))
			{
				sequenceNumber = formerSessions.sequenceNumber + 1;
				break;
			}
		}
		return sequenceNumber;
	}

	void FastPacketProtocol::process_message(const CANMessage &message)
	{
		if ((CAN_DATA_LENGTH != message.get_data_length()) ||
		    (message.get_source_control_function() == nullptr) ||
		    (message.get_identifier().get_parameter_group_number() < FP_MIN_PARAMETER_GROUP_NUMBER) ||
		    (message.get_identifier().get_parameter_group_number() > FP_MAX_PARAMETER_GROUP_NUMBER))
		{
			// Not a valid message for this protocol
			return;
		}

		if (parameterGroupNumberCallbacks.empty())
		{
			// No listeners, no need to process the message
			return;
		}

		if ((!message.is_destination_our_device()) && (!allowAnyControlFunction) && (!message.is_broadcast()))
		{
			// Destined for someone else, no need to process the message
			return;
		}

		// Check if we have any callbacks for this PGN
		bool pgnNeedsParsing = false;
		for (const auto &callback : parameterGroupNumberCallbacks)
		{
			if ((callback.get_parameter_group_number() == message.get_identifier().get_parameter_group_number()) &&
			    ((nullptr == callback.get_internal_control_function()) ||
			     (callback.get_internal_control_function() == message.get_destination_control_function())))
			{
				pgnNeedsParsing = true;
				break;
			}
		}

		if (!pgnNeedsParsing)
		{
			// No callbacks for this PGN, no need to process the message
			return;
		}

		std::shared_ptr<FastPacketProtocolSession> session = get_session(message.get_identifier().get_parameter_group_number(),
		                                                                 message.get_source_control_function(),
		                                                                 message.get_destination_control_function());
		std::uint8_t actualFrameCount = (message.get_uint8_at(0) & FRAME_COUNTER_BIT_MASK);

		if (nullptr != session)
		{
			// We have a matching active session
			if (0 == actualFrameCount)
			{
				// This is the beginning of a new message, but we already have a session
				LOG_ERROR("[FP]: Existing session matched new frame counter, aborting the matching session.");
				close_session(session, false);
			}
			else
			{
				// Correct sequence number, copy the data (hybrid optimization)
				// Convert data type to a vector to allow for manipulation
				auto &data = static_cast<CANMessageDataVector &>(session->get_data());

				// Defensive bounds check to prevent potential buffer overflow
				if (session->numberOfBytesTransferred >= session->get_message_length())
				{
					LOG_ERROR("[FP]: Protocol violation - bytes transferred %u exceeds message length %u",
					          session->numberOfBytesTransferred,
					          session->get_message_length());
					close_session(session, false);
					return;
				}

				std::size_t bytes_to_copy = std::min(
				  static_cast<std::size_t>(PROTOCOL_BYTES_PER_FRAME),
				  static_cast<std::size_t>(session->get_message_length() - session->numberOfBytesTransferred));

				if (bytes_to_copy > 0)
				{
					if (bytes_to_copy <= 4)
					{
						// Use byte-by-byte for small copies (better for tiny data)
						for (std::size_t i = 0; i < bytes_to_copy; i++)
						{
							data[session->numberOfBytesTransferred + i] = message.get_data().data()[1 + i];
						}
					}
					else
					{
						// Use memcpy for larger copies (better for bulk data)
						memcpy(&data[session->numberOfBytesTransferred], message.get_data().data() + 1, bytes_to_copy);
					}
					session->add_number_of_bytes_transferred(static_cast<std::uint8_t>(bytes_to_copy));
				}

				if (session->numberOfBytesTransferred >= session->get_message_length())
				{
					// Complete
					CANMessage completedMessage(CANMessage::Type::Receive,
					                            message.get_identifier(),
					                            std::move(data),
					                            message.get_source_control_function(),
					                            message.get_destination_control_function(),
					                            message.get_can_port_index());

					// Find the appropriate callback and let them know
					for (const auto &callback : parameterGroupNumberCallbacks)
					{
						if ((callback.get_parameter_group_number() == message.get_identifier().get_parameter_group_number()) &&
						    ((nullptr == callback.get_internal_control_function()) ||
						     (callback.get_internal_control_function() == message.get_destination_control_function())))
						{
							callback.get_callback()(completedMessage, callback.get_parent());
						}
					}
					close_session(session, true);
				}
			}
		}
		else
		{
			// No matching session. See if we need to start a new session
			if (0 != actualFrameCount)
			{
				// This is the middle of some message that we have no context for.
				// Ignore the message for now until we receive it with a fresh packet counter.
				LOG_WARNING("[FP]: Ignoring FP message with PGN %u, no context available. The message may be processed when packet count returns to zero.",
				            message.get_identifier().get_parameter_group_number());
			}
			else
			{
				// This is the beginning of a new message
				std::uint8_t messageLength = message.get_uint8_at(1);
				if (messageLength > MAX_PROTOCOL_MESSAGE_LENGTH)
				{
					LOG_WARNING("[FP]: Ignoring possible new FP session with advertised length > 233.");
					return;
				}
				else if (messageLength <= CAN_DATA_LENGTH)
				{
					LOG_WARNING("[FP]: Ignoring possible new FP session with advertised length <= 8.");
					return;
				}

				// Create a new session
				session = std::make_shared<FastPacketProtocolSession>(FastPacketProtocolSession::Direction::Receive,
				                                                      std::unique_ptr<CANMessageData>(new CANMessageDataVector(messageLength)),
				                                                      message.get_identifier().get_parameter_group_number(),
				                                                      messageLength,
				                                                      (message.get_uint8_at(0) & SEQUENCE_NUMBER_BIT_MASK),
				                                                      message.get_identifier().get_priority(),
				                                                      message.get_source_control_function(),
				                                                      message.get_destination_control_function(),
				                                                      nullptr, // No callback
				                                                      nullptr);

				// Save the 6 bytes of payload in this first message (hybrid optimization)
				// Convert data type to a vector to allow for manipulation
				auto &data = static_cast<CANMessageDataVector &>(session->get_data());

				// Defensive bounds check to prevent potential buffer overflow
				if (session->numberOfBytesTransferred >= session->get_message_length())
				{
					LOG_ERROR("[FP]: Protocol violation - bytes transferred %u exceeds message length %u",
					          session->numberOfBytesTransferred,
					          session->get_message_length());
					close_session(session, false);
					return;
				}

				std::size_t bytes_to_copy = std::min(
				  static_cast<std::size_t>(PROTOCOL_BYTES_PER_FRAME - 1),
				  static_cast<std::size_t>(session->get_message_length() - session->numberOfBytesTransferred));

				if (bytes_to_copy > 0)
				{
					if (bytes_to_copy <= 4)
					{
						// Use byte-by-byte for small copies (better for tiny data)
						for (std::size_t i = 0; i < bytes_to_copy; i++)
						{
							data[session->numberOfBytesTransferred + i] = message.get_data().data()[2 + i];
						}
					}
					else
					{
						// Use memcpy for larger copies (better for bulk data)
						memcpy(&data[session->numberOfBytesTransferred], message.get_data().data() + 2, bytes_to_copy);
					}
					session->add_number_of_bytes_transferred(static_cast<std::uint8_t>(bytes_to_copy));
				}

				LOCK_GUARD(Mutex, sessionMutex);
				activeSessions.push_back(session);
			}
		}
	}

	void FastPacketProtocol::update_session(const std::shared_ptr<FastPacketProtocolSession> &session)
	{
		if (nullptr == session)
		{
			return;
		}

		if (session->get_direction() == FastPacketProtocolSession::Direction::Receive)
		{
			// We are receiving a message, only need to check for timeouts
			if (session->get_time_since_last_update() > FP_TIMEOUT_MS)
			{
				LOG_ERROR("[FP]: Rx session timed out.");
				close_session(session, false);
			}
		}
		else
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;
			// We are transmitting a message, let's try and send remaining packets
			for (std::uint8_t i = 0; i < session->get_number_of_remaining_packets(); i++)
			{
				buffer[0] = session->get_last_packet_number();
				buffer[0] |= (session->sequenceNumber << SEQUENCE_NUMBER_BIT_OFFSET);

				std::uint8_t startIndex = 1;
				std::uint8_t bytesThisFrame = PROTOCOL_BYTES_PER_FRAME;
				if (0 == session->get_total_bytes_transferred())
				{
					// This is the first frame, so we need to send the message length
					buffer[1] = session->get_message_length();
					startIndex++;
					bytesThisFrame--;
				}

				for (std::uint8_t j = 0; j < bytesThisFrame; j++)
				{
					std::uint8_t index = static_cast<std::uint8_t>(session->get_total_bytes_transferred()) + j;
					if (index < session->get_message_length())
					{
						buffer[startIndex + j] = session->get_data().get_byte(index);
					}
					else
					{
						buffer[startIndex + j] = 0xFF;
					}
				}

				if (sendCANFrameCallback(session->get_parameter_group_number(),
				                         CANDataSpan(buffer.data(), buffer.size()),
				                         std::static_pointer_cast<InternalControlFunction>(session->get_source()),
				                         session->get_destination(),
				                         session->priority))
				{
					session->add_number_of_bytes_transferred(bytesThisFrame);
				}
				else
				{
					if (session->get_time_since_last_update() > FP_TIMEOUT_MS)
					{
						LOG_ERROR("[FP]: Tx session timed out.");
						close_session(session, false);
					}
					break;
				}
			}

			if (session->get_number_of_remaining_packets() == 0)
			{
				close_session(session, true);
			}
		}
	}

	bool FastPacketProtocol::has_session(std::uint32_t parameterGroupNumber, std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination)
	{
		LOCK_GUARD(Mutex, sessionMutex);
		return std::any_of(activeSessions.begin(), activeSessions.end(), [&](const std::shared_ptr<FastPacketProtocolSession> &session) {
			return session->matches(source, destination) && (session->get_parameter_group_number() == parameterGroupNumber);
		});
	}

	std::shared_ptr<FastPacketProtocol::FastPacketProtocolSession> FastPacketProtocol::get_session(std::uint32_t parameterGroupNumber,
	                                                                                               std::shared_ptr<ControlFunction> source,
	                                                                                               std::shared_ptr<ControlFunction> destination)
	{
		LOCK_GUARD(Mutex, sessionMutex);
		auto result = std::find_if(activeSessions.begin(), activeSessions.end(), [&](const std::shared_ptr<FastPacketProtocolSession> &session) {
			return session->matches(source, destination) && (session->get_parameter_group_number() == parameterGroupNumber);
		});
		return (activeSessions.end() != result) ? (*result) : nullptr;
	}

} // namespace isobus
