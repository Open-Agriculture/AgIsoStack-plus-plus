//================================================================================================
/// @file can_extended_transport_protocol.cpp
///
/// @brief A protocol class that handles the ISO11783 extended transport protocol.
/// @author Adrian Del Grosso
/// @author Daan Steenbergen
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================

#include "isobus/isobus/can_extended_transport_protocol.hpp"

#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

#include <algorithm>
#include <cstring>
#include <memory>

namespace isobus
{
	ExtendedTransportProtocolManager::StateMachineState ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::get_state() const
	{
		return state;
	}

	void ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::set_state(StateMachineState value)
	{
		state = value;
		update_timestamp();
	}

	std::uint32_t ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::get_total_bytes_transferred() const
	{
		uint32_t transferred = get_last_packet_number() * PROTOCOL_BYTES_PER_FRAME;
		if (transferred > get_message_length())
		{
			transferred = get_message_length();
		}
		return transferred;
	}

	std::uint8_t ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::get_dpo_number_of_packets_remaining() const
	{
		auto packetsSinceDPO = static_cast<std::uint8_t>(get_last_packet_number() - lastAcknowledgedPacketNumber);
		return dataPacketOffsetPacketCount - packetsSinceDPO;
	}

	void ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::set_dpo_number_of_packets(std::uint8_t value)
	{
		dataPacketOffsetPacketCount = value;
		update_timestamp();
	}

	std::uint8_t ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::get_dpo_number_of_packets() const
	{
		return dataPacketOffsetPacketCount;
	}

	std::uint8_t ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::get_cts_number_of_packet_limit() const
	{
		return clearToSendPacketCountLimit;
	}

	void ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::set_cts_number_of_packet_limit(std::uint8_t value)
	{
		clearToSendPacketCountLimit = value;
		update_timestamp();
	}

	std::uint8_t ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::get_last_sequence_number() const
	{
		return lastSequenceNumber;
	}

	std::uint32_t ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::get_last_packet_number() const
	{
		return lastSequenceNumber + sequenceNumberOffset;
	}

	void ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::set_sequence_number_offset(std::uint32_t value)
	{
		sequenceNumberOffset = value;
		update_timestamp();
	}

	void ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::set_last_sequency_number(std::uint8_t value)
	{
		lastSequenceNumber = value;
		update_timestamp();
	}

	void ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::set_acknowledged_packet_number(std::uint32_t value)
	{
		lastAcknowledgedPacketNumber = value;
		update_timestamp();
	}

	std::uint32_t ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::get_last_acknowledged_packet_number() const
	{
		return lastAcknowledgedPacketNumber;
	}

	std::uint32_t ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::get_number_of_remaining_packets() const
	{
		return get_total_number_of_packets() - get_last_packet_number();
	}

	std::uint32_t ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::get_total_number_of_packets() const
	{
		auto totalNumberOfPackets = get_message_length() / PROTOCOL_BYTES_PER_FRAME;
		if ((get_message_length() % PROTOCOL_BYTES_PER_FRAME) > 0)
		{
			totalNumberOfPackets++;
		}
		return totalNumberOfPackets;
	}

	ExtendedTransportProtocolManager::ExtendedTransportProtocolManager(const CANMessageFrameCallback &sendCANFrameCallback,
	                                                                   const CANMessageCallback &canMessageReceivedCallback,
	                                                                   const CANNetworkConfiguration *configuration) :
	  sendCANFrameCallback(sendCANFrameCallback),
	  canMessageReceivedCallback(canMessageReceivedCallback),
	  configuration(configuration)
	{
	}

	void ExtendedTransportProtocolManager::process_request_to_send(const std::shared_ptr<ControlFunction> source,
	                                                               const std::shared_ptr<ControlFunction> destination,
	                                                               std::uint32_t parameterGroupNumber,
	                                                               std::uint32_t totalMessageSize)
	{
		if (activeSessions.size() >= configuration->get_max_number_transport_protocol_sessions())
		{
			// TODO: consider using maximum memory instead of maximum number of sessions
			LOG_WARNING("[ETP]: Replying with abort to Request To Send (RTS) for 0x%05X, configured maximum number of sessions reached.",
			            parameterGroupNumber);
			send_abort(std::static_pointer_cast<InternalControlFunction>(destination), source, parameterGroupNumber, ConnectionAbortReason::AlreadyInCMSession);
		}
		else
		{
			auto oldSession = get_session(source, destination);
			if (nullptr != oldSession)
			{
				if (oldSession->get_parameter_group_number() != parameterGroupNumber)
				{
					LOG_ERROR("[ETP]: Received Request To Send (RTS) while a session already existed for this source and destination, aborting for 0x%05X...",
					          parameterGroupNumber);
					abort_session(oldSession, ConnectionAbortReason::AlreadyInCMSession);
				}
				else
				{
					LOG_WARNING("[ETP]: Received Request To Send (RTS) while a session already existed for this source and destination and parameterGroupNumber, overwriting for 0x%05X...",
					            parameterGroupNumber);
					close_session(oldSession, false);
				}
			}

			auto newSession = std::make_shared<ExtendedTransportProtocolSession>(ExtendedTransportProtocolSession::Direction::Receive,
			                                                                     std::unique_ptr<CANMessageData>(new CANMessageDataVector(totalMessageSize)),
			                                                                     parameterGroupNumber,
			                                                                     totalMessageSize,
			                                                                     source,
			                                                                     destination,
			                                                                     nullptr, // No callback
			                                                                     nullptr);

			// Request the maximum number of packets per DPO via the CTS message
			newSession->set_cts_number_of_packet_limit(configuration->get_number_of_packets_per_dpo_message());

			newSession->set_state(StateMachineState::SendClearToSend);
			activeSessions.push_back(newSession);
			LOG_DEBUG("[ETP]: New rx session for 0x%05X. Source: %hu, destination: %hu", parameterGroupNumber, source->get_address(), destination->get_address());
			update_state_machine(newSession);
		}
	}

	void ExtendedTransportProtocolManager::process_clear_to_send(const std::shared_ptr<ControlFunction> source,
	                                                             const std::shared_ptr<ControlFunction> destination,
	                                                             std::uint32_t parameterGroupNumber,
	                                                             std::uint8_t packetsToBeSent,
	                                                             std::uint32_t nextPacketNumber)
	{
		auto session = get_session(destination, source);
		if (nullptr != session)
		{
			if (session->get_parameter_group_number() != parameterGroupNumber)
			{
				LOG_ERROR("[ETP]: Received a Clear To Send (CTS) message for 0x%05X while a session already existed for this source and destination, sending abort for both...", parameterGroupNumber);
				abort_session(session, ConnectionAbortReason::ClearToSendReceivedWhileTransferInProgress);
				send_abort(std::static_pointer_cast<InternalControlFunction>(destination), source, parameterGroupNumber, ConnectionAbortReason::ClearToSendReceivedWhileTransferInProgress);
			}
			else if (nextPacketNumber > session->get_total_number_of_packets())
			{
				LOG_ERROR("[ETP]: Received a Clear To Send (CTS) message for 0x%05X with a bad sequence number, aborting...", parameterGroupNumber);
				abort_session(session, ConnectionAbortReason::NumberOfClearToSendPacketsExceedsMessage);
			}
			else if (StateMachineState::WaitForClearToSend != session->state)
			{
				// The session exists, but we're not in the right state to receive a CTS, so we must abort
				LOG_WARNING("[ETP]: Received a Clear To Send (CTS) message for 0x%05X, but not expecting one, aborting session->", parameterGroupNumber);
				abort_session(session, ConnectionAbortReason::ClearToSendReceivedWhileTransferInProgress);
			}
			else
			{
				session->set_acknowledged_packet_number(nextPacketNumber - 1);
				session->set_cts_number_of_packet_limit(packetsToBeSent);

				// If 0 was sent as the packet number, they want us to wait.
				// Just sit here in this state until we get a non-zero packet count
				if (0 != packetsToBeSent)
				{
					session->set_state(StateMachineState::SendDataPacketOffset);
				}
			}
		}
		else
		{
			// We got a CTS but no session exists, by the standard we must ignore it
			LOG_WARNING("[ETP]: Received Clear To Send (CTS) for 0x%05X while no session existed for this source and destination, ignoring...", parameterGroupNumber);
		}
	}

	void ExtendedTransportProtocolManager::process_data_packet_offset(const std::shared_ptr<ControlFunction> source,
	                                                                  const std::shared_ptr<ControlFunction> destination,
	                                                                  std::uint32_t parameterGroupNumber,
	                                                                  std::uint8_t numberOfPackets,
	                                                                  std::uint32_t packetOffset)
	{
		auto session = get_session(source, destination);
		if (nullptr != session)
		{
			if (session->get_parameter_group_number() != parameterGroupNumber)
			{
				LOG_ERROR("[ETP]: Received a Data Packet Offset message for 0x%05X while a session already existed for this source and destination with a different PGN, sending abort for both...", parameterGroupNumber);
				abort_session(session, ConnectionAbortReason::UnexpectedDataPacketOffsetPGN);
				send_abort(std::static_pointer_cast<InternalControlFunction>(destination), source, parameterGroupNumber, ConnectionAbortReason::UnexpectedDataPacketOffsetPGN);
			}
			else if (StateMachineState::WaitForDataPacketOffset != session->state)
			{
				// The session exists, but we're not in the right state to receive a DPO, so we must abort
				LOG_WARNING("[ETP]: Received a Data Packet Offset message for 0x%05X, but not expecting one, aborting session->", parameterGroupNumber);
				abort_session(session, ConnectionAbortReason::UnexpectedDataPacketOffsetReceived);
			}
			else if (numberOfPackets > session->get_cts_number_of_packet_limit())
			{
				LOG_ERROR("[ETP]: Received a Data Packet Offset message for 0x%05X with an higher number of packets than our CTS, aborting...", parameterGroupNumber);
				abort_session(session, ConnectionAbortReason::DataPacketOffsetExceedsClearToSend);
			}
			else if (packetOffset != session->get_last_acknowledged_packet_number())
			{
				LOG_ERROR("[ETP]: Received a Data Packet Offset message for 0x%05X with a bad sequence number, aborting...", parameterGroupNumber);
				abort_session(session, ConnectionAbortReason::BadDataPacketOffset);
			}
			else
			{
				session->set_dpo_number_of_packets(numberOfPackets);
				session->set_sequence_number_offset(packetOffset);
				session->set_last_sequency_number(0);

				// If 0 was sent as the packet number, they want us to wait.
				// Just sit here in this state until we get a non-zero packet count
				if (0 != numberOfPackets)
				{
					session->set_state(StateMachineState::WaitForDataTransferPacket);
				}
			}
		}
		else
		{
			// We got a CTS but no session exists, by the standard we must ignore it
			LOG_WARNING("[ETP]: Received Data Packet Offset for 0x%05X while no session existed for this source and destination, ignoring...", parameterGroupNumber);
		}
	}

	void ExtendedTransportProtocolManager::process_end_of_session_acknowledgement(const std::shared_ptr<ControlFunction> source,
	                                                                              const std::shared_ptr<ControlFunction> destination,
	                                                                              std::uint32_t parameterGroupNumber,
	                                                                              std::uint32_t numberOfBytesTransferred)
	{
		auto session = get_session(destination, source);
		if (nullptr != session)
		{
			if (StateMachineState::WaitForEndOfMessageAcknowledge == session->state)
			{
				session->state = StateMachineState::None;
				bool successful = (numberOfBytesTransferred == session->get_message_length());
				close_session(session, successful);
				LOG_DEBUG("[ETP]: Completed tx session for 0x%05X from %hu (successful=%s)", parameterGroupNumber, source->get_address(), successful ? "true" : "false");
			}
			else
			{
				// The session exists, but we're not in the right state to receive an EOM, by the standard we must ignore it
				LOG_WARNING("[ETP]: Received an End Of Message Acknowledgement message for 0x%05X, but not expecting one, ignoring.", parameterGroupNumber);
			}
		}
		else
		{
			LOG_WARNING("[ETP]: Received End Of Message Acknowledgement for 0x%05X while no session existed for this source and destination, ignoring.", parameterGroupNumber);
		}
	}

	void ExtendedTransportProtocolManager::process_abort(const std::shared_ptr<ControlFunction> source,
	                                                     const std::shared_ptr<ControlFunction> destination,
	                                                     std::uint32_t parameterGroupNumber,
	                                                     ExtendedTransportProtocolManager::ConnectionAbortReason reason)
	{
		bool foundSession = false;

		auto session = get_session(source, destination);
		if ((nullptr != session) && (session->get_parameter_group_number() == parameterGroupNumber))
		{
			foundSession = true;
			LOG_ERROR("[ETP]: Received an abort (reason=%hu) for an rx session for parameterGroupNumber 0x%05X", static_cast<std::uint8_t>(reason), parameterGroupNumber);
			close_session(session, false);
		}
		session = get_session(destination, source);
		if ((nullptr != session) && (session->get_parameter_group_number() == parameterGroupNumber))
		{
			foundSession = true;
			LOG_ERROR("[ETP]: Received an abort (reason=%hu) for a tx session for parameterGroupNumber 0x%05X", static_cast<std::uint8_t>(reason), parameterGroupNumber);
			close_session(session, false);
		}

		if (!foundSession)
		{
			LOG_WARNING("[ETP]: Received an abort (reason=%hu) with no matching session for parameterGroupNumber 0x%05X", static_cast<std::uint8_t>(reason), parameterGroupNumber);
		}
	}

	void ExtendedTransportProtocolManager::process_connection_management_message(const CANMessage &message)
	{
		if (CAN_DATA_LENGTH != message.get_data_length())
		{
			LOG_WARNING("[ETP]: Received a Connection Management message of invalid length %hu", message.get_data_length());
			return;
		}

		const auto parameterGroupNumber = message.get_uint24_at(5);

		switch (message.get_uint8_at(0))
		{
			case REQUEST_TO_SEND_MULTIPLEXOR:
			{
				const auto totalMessageSize = message.get_uint32_at(1);
				process_request_to_send(message.get_source_control_function(),
				                        message.get_destination_control_function(),
				                        parameterGroupNumber,
				                        totalMessageSize);
			}
			break;

			case CLEAR_TO_SEND_MULTIPLEXOR:
			{
				const auto packetsToBeSent = message.get_uint8_at(1);
				const auto nextPacketNumber = message.get_uint24_at(2);
				process_clear_to_send(message.get_source_control_function(),
				                      message.get_destination_control_function(),
				                      parameterGroupNumber,
				                      packetsToBeSent,
				                      nextPacketNumber);
			}
			break;

			case DATA_PACKET_OFFSET_MULTIPLXOR:
			{
				const auto numberOfPackets = message.get_uint8_at(1);
				const auto packetOffset = message.get_uint24_at(2);
				process_data_packet_offset(message.get_source_control_function(),
				                           message.get_destination_control_function(),
				                           parameterGroupNumber,
				                           numberOfPackets,
				                           packetOffset);
			}
			break;

			case END_OF_MESSAGE_ACKNOWLEDGE_MULTIPLEXOR:
			{
				const auto numberOfBytesTransferred = message.get_uint32_at(1);
				process_end_of_session_acknowledgement(message.get_source_control_function(),
				                                       message.get_destination_control_function(),
				                                       parameterGroupNumber,
				                                       numberOfBytesTransferred);
			}
			break;

			case CONNECTION_ABORT_MULTIPLEXOR:
			{
				const auto reason = static_cast<ConnectionAbortReason>(message.get_uint8_at(1));
				process_abort(message.get_source_control_function(),
				              message.get_destination_control_function(),
				              parameterGroupNumber,
				              reason);
			}
			break;

			default:
			{
				LOG_WARNING("[ETP]: Bad Mux in Transport Protocol Connection Management message");
			}
			break;
		}
	}

	void ExtendedTransportProtocolManager::process_data_transfer_message(const CANMessage &message)
	{
		if (CAN_DATA_LENGTH != message.get_data_length())
		{
			LOG_WARNING("[ETP]: Received a Data Transfer message of invalid length %hu", message.get_data_length());
			return;
		}

		auto source = message.get_source_control_function();
		auto destination = message.get_destination_control_function();

		auto sequenceNumber = message.get_uint8_at(SEQUENCE_NUMBER_DATA_INDEX);

		auto session = get_session(source, destination);
		if (nullptr != session)
		{
			if (StateMachineState::WaitForDataTransferPacket != session->state)
			{
				LOG_WARNING("[ETP]: Received a Data Transfer message from %hu while not expecting one, sending abort", source->get_address());
				abort_session(session, ConnectionAbortReason::UnexpectedDataTransferPacketReceived);
			}
			else if (sequenceNumber == session->get_last_sequence_number())
			{
				LOG_ERROR("[ETP]: Aborting rx session for 0x%05X due to duplicate sequence number", session->get_parameter_group_number());
				abort_session(session, ConnectionAbortReason::DuplicateSequenceNumber);
			}
			else if (sequenceNumber == (session->get_last_sequence_number() + 1))
			{
				// Convert data type to a vector to allow for manipulation
				auto &data = static_cast<CANMessageDataVector &>(session->get_data());

				// Correct sequence number, copy the data (hybrid optimization)
				std::uint32_t currentDataIndex = PROTOCOL_BYTES_PER_FRAME * session->get_last_packet_number();

				// Defensive bounds check to prevent potential buffer overflow
				if (currentDataIndex >= session->get_message_length())
				{
					LOG_ERROR("[ETP]: Protocol violation - packet index %u exceeds message length %u", currentDataIndex, session->get_message_length());
					abort_session(session, ConnectionAbortReason::AnyOtherError);
					return;
				}

				std::size_t bytes_to_copy = std::min(
				  static_cast<std::size_t>(PROTOCOL_BYTES_PER_FRAME),
				  static_cast<std::size_t>(session->get_message_length() - currentDataIndex));

				if (bytes_to_copy > 0)
				{
					if (bytes_to_copy <= 4)
					{
						// Use byte-by-byte for small copies (better for tiny data)
						for (std::size_t i = 0; i < bytes_to_copy; i++)
						{
							data[currentDataIndex + i] = message.get_data().data()[1 + i];
						}
					}
					else
					{
						// Use memcpy for larger copies (better for bulk data)
						memcpy(&data[currentDataIndex], message.get_data().data() + 1, bytes_to_copy);
					}
				}

				session->set_last_sequency_number(sequenceNumber);
				if (session->get_number_of_remaining_packets() == 0)
				{
					// Send End of Message Acknowledgement for sessions with specific destination only
					send_end_of_session_acknowledgement(session);

					// Construct the completed message
					CANIdentifier identifier(CANIdentifier::Type::Extended,
					                         session->get_parameter_group_number(),
					                         CANIdentifier::CANPriority::PriorityDefault6,
					                         destination->get_address(),
					                         source->get_address());
					CANMessage completedMessage(CANMessage::Type::Receive,
					                            identifier,
					                            std::move(data),
					                            source,
					                            destination,
					                            0);

					canMessageReceivedCallback(completedMessage);
					close_session(session, true);
					LOG_DEBUG("[ETP]: Completed rx session for 0x%05X from %hu", session->get_parameter_group_number(), source->get_address());
				}
				else if (session->get_dpo_number_of_packets_remaining() == 0)
				{
					session->set_state(StateMachineState::SendClearToSend);
					update_state_machine(session);
				}
			}
			else
			{
				LOG_ERROR("[ETP]: Aborting rx session for 0x%05X due to bad sequence number", session->get_parameter_group_number());
				abort_session(session, ConnectionAbortReason::BadSequenceNumber);
			}
		}
	}

	void ExtendedTransportProtocolManager::process_message(const CANMessage &message)
	{
		// TODO: Allow sniffing of messages to all addresses, not just the ones we normally listen to (#297)
		if (message.has_valid_source_control_function() && message.is_destination_our_device())
		{
			switch (message.get_identifier().get_parameter_group_number())
			{
				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement):
				{
					process_connection_management_message(message);
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolDataTransfer):
				{
					process_data_transfer_message(message);
				}
				break;

				default:
					break;
			}
		}
	}

	bool ExtendedTransportProtocolManager::protocol_transmit_message(std::uint32_t parameterGroupNumber,
	                                                                 std::unique_ptr<CANMessageData> &data,
	                                                                 std::shared_ptr<ControlFunction> source,
	                                                                 std::shared_ptr<ControlFunction> destination,
	                                                                 TransmitCompleteCallback sessionCompleteCallback,
	                                                                 void *parentPointer)
	{
		// Return false early if we can't send the message
		if ((nullptr == data) || (data->size() <= 1785) || (data->size() > MAX_PROTOCOL_DATA_LENGTH))
		{
			// Invalid message length
			return false;
		}
		else if ((nullptr == source) || (!source->get_address_valid()) ||
		         (nullptr == destination) || (!destination->get_address_valid()) ||
		         has_session(source, destination))
		{
			// Invalid source/destination or already have a session for this source and destination
			return false;
		}

		// We can handle this message! If we only have a view of the data, let's clone the data,
		// so we don't have to worry about it being deleted.
		data = data->copy_if_not_owned(std::move(data));
		auto dataLength = static_cast<std::uint32_t>(data->size());

		auto session = std::make_shared<ExtendedTransportProtocolSession>(ExtendedTransportProtocolSession::Direction::Transmit,
		                                                                  std::move(data),
		                                                                  parameterGroupNumber,
		                                                                  dataLength,
		                                                                  source,
		                                                                  destination,
		                                                                  sessionCompleteCallback,
		                                                                  parentPointer);
		session->set_state(StateMachineState::SendRequestToSend);
		LOG_DEBUG("[ETP]: New tx session for 0x%05X. Source: %hu, destination: %hu",
		          parameterGroupNumber,
		          source->get_address(),
		          destination->get_address());

		activeSessions.push_back(session);
		update_state_machine(session);
		return true;
	}

	void ExtendedTransportProtocolManager::update()
	{
		// We use a fancy for loop here to allow us to remove sessions from the list while iterating
		for (std::size_t i = activeSessions.size(); i > 0; i--)
		{
			auto session = activeSessions.at(i - 1);
			if (!session->get_source()->get_address_valid())
			{
				LOG_WARNING("[ETP]: Closing active session as the source control function is no longer valid");
				close_session(session, false);
			}
			else if (!session->get_destination()->get_address_valid())
			{
				LOG_WARNING("[ETP]: Closing active session as the destination control function is no longer valid");
				close_session(session, false);
			}
			else if (StateMachineState::None != session->state)
			{
				update_state_machine(session);
			}
		}
	}

	void ExtendedTransportProtocolManager::send_data_transfer_packets(const std::shared_ptr<ExtendedTransportProtocolSession> &session) const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;
		std::uint8_t framesToSend = session->get_dpo_number_of_packets_remaining();
		if (framesToSend > configuration->get_max_number_of_network_manager_protocol_frames_per_update())
		{
			framesToSend = configuration->get_max_number_of_network_manager_protocol_frames_per_update();
		}

		// Try and send packets
		for (std::uint8_t i = 0; i < framesToSend; i++)
		{
			buffer[0] = session->get_last_sequence_number() + 1;

			std::uint32_t dataOffset = session->get_last_packet_number() * PROTOCOL_BYTES_PER_FRAME;
			for (std::uint8_t j = 0; j < PROTOCOL_BYTES_PER_FRAME; j++)
			{
				std::uint32_t index = dataOffset + j;
				if (index < session->get_message_length())
				{
					buffer[1 + j] = session->get_data().get_byte(index);
				}
				else
				{
					buffer[1 + j] = 0xFF;
				}
			}

			if (sendCANFrameCallback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolDataTransfer),
			                         CANDataSpan(buffer.data(), buffer.size()),
			                         std::static_pointer_cast<InternalControlFunction>(session->get_source()),
			                         session->get_destination(),
			                         CANIdentifier::CANPriority::PriorityLowest7))
			{
				session->set_last_sequency_number(session->get_last_sequence_number() + 1);
			}
			else
			{
				// Process more next time protocol is updated
				break;
			}
		}

		if (session->get_number_of_remaining_packets() == 0)
		{
			session->set_state(StateMachineState::WaitForEndOfMessageAcknowledge);
		}
		else if (session->get_dpo_number_of_packets_remaining() == 0)
		{
			session->set_state(StateMachineState::WaitForClearToSend);
		}
	}

	void ExtendedTransportProtocolManager::update_state_machine(std::shared_ptr<ExtendedTransportProtocolSession> &session)
	{
		switch (session->state)
		{
			case StateMachineState::None:
				break;

			case StateMachineState::SendRequestToSend:
			{
				if (send_request_to_send(session))
				{
					session->set_state(StateMachineState::WaitForClearToSend);
				}
			}
			break;

			case StateMachineState::WaitForClearToSend:
			{
				if (session->get_time_since_last_update() > T2_T3_TIMEOUT_MS)
				{
					LOG_ERROR("[ETP]: Timeout tx session for 0x%05X (expected CTS)", session->get_parameter_group_number());
					if (session->get_cts_number_of_packet_limit() > 0)
					{
						// A connection is only considered established if we've received at least one CTS before
						// And we can only abort a connection if it's considered established
						abort_session(session, ConnectionAbortReason::Timeout);
					}
					else
					{
						close_session(session, false);
					}
				}
			}
			break;

			case StateMachineState::SendClearToSend:
			{
				if (send_clear_to_send(session))
				{
					session->set_state(StateMachineState::WaitForDataPacketOffset);
				}
			}
			break;

			case StateMachineState::WaitForDataPacketOffset:
			{
				if (session->get_time_since_last_update() > T2_T3_TIMEOUT_MS)
				{
					LOG_ERROR("[ETP]: Timeout rx session for 0x%05X (expected DPO)", session->get_parameter_group_number());
					abort_session(session, ConnectionAbortReason::Timeout);
				}
			}
			break;

			case StateMachineState::SendDataPacketOffset:
			{
				if (send_data_packet_offset(session))
				{
					session->set_state(StateMachineState::SendDataTransferPackets);
				}
			}
			break;

			case StateMachineState::SendDataTransferPackets:
			{
				send_data_transfer_packets(session);
			}
			break;

			case StateMachineState::WaitForDataTransferPacket:
			{
				if (session->get_time_since_last_update() > T1_TIMEOUT_MS)
				{
					LOG_ERROR("[ETP]: Timeout for destination-specific rx session (expected sequential data frame)");
					abort_session(session, ConnectionAbortReason::Timeout);
				}
			}
			break;

			case StateMachineState::WaitForEndOfMessageAcknowledge:
			{
				if (session->get_time_since_last_update() > T2_T3_TIMEOUT_MS)
				{
					LOG_ERROR("[ETP]: Timeout tx session for 0x%05X (expected EOMA)", session->get_parameter_group_number());
					abort_session(session, ConnectionAbortReason::Timeout);
				}
			}
			break;
		}
	}

	bool ExtendedTransportProtocolManager::abort_session(const std::shared_ptr<ExtendedTransportProtocolSession> &session, ConnectionAbortReason reason)
	{
		bool retVal = false;
		std::shared_ptr<InternalControlFunction> myControlFunction;
		std::shared_ptr<ControlFunction> partnerControlFunction;
		if (ExtendedTransportProtocolSession::Direction::Transmit == session->get_direction())
		{
			myControlFunction = std::static_pointer_cast<InternalControlFunction>(session->get_source());
			partnerControlFunction = session->get_destination();
		}
		else
		{
			myControlFunction = std::static_pointer_cast<InternalControlFunction>(session->get_destination());
			partnerControlFunction = session->get_source();
		}

		if ((nullptr != myControlFunction) && (nullptr != partnerControlFunction))
		{
			retVal = send_abort(myControlFunction, partnerControlFunction, session->get_parameter_group_number(), reason);
		}
		close_session(session, false);
		return retVal;
	}

	bool ExtendedTransportProtocolManager::send_abort(std::shared_ptr<InternalControlFunction> sender,
	                                                  std::shared_ptr<ControlFunction> receiver,
	                                                  std::uint32_t parameterGroupNumber,
	                                                  ConnectionAbortReason reason) const
	{
		const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer{
			CONNECTION_ABORT_MULTIPLEXOR,
			static_cast<std::uint8_t>(reason),
			0xFF,
			0xFF,
			0xFF,
			static_cast<std::uint8_t>(parameterGroupNumber & 0xFF),
			static_cast<std::uint8_t>((parameterGroupNumber >> 8) & 0xFF),
			static_cast<std::uint8_t>((parameterGroupNumber >> 16) & 0xFF)
		};
		return sendCANFrameCallback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement),
		                            CANDataSpan(buffer.data(), buffer.size()),
		                            sender,
		                            receiver,
		                            CANIdentifier::CANPriority::PriorityLowest7);
	}

	void ExtendedTransportProtocolManager::close_session(const std::shared_ptr<ExtendedTransportProtocolSession> &session, bool successful)
	{
		session->complete(successful);
		auto sessionLocation = std::find(activeSessions.begin(), activeSessions.end(), session);
		if (activeSessions.end() != sessionLocation)
		{
			activeSessions.erase(sessionLocation);
			LOG_DEBUG("[ETP]: Session Closed");
		}
	}

	bool ExtendedTransportProtocolManager::send_request_to_send(const std::shared_ptr<ExtendedTransportProtocolSession> &session) const
	{
		const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer{
			REQUEST_TO_SEND_MULTIPLEXOR,
			static_cast<std::uint8_t>(session->get_message_length() & 0xFF),
			static_cast<std::uint8_t>((session->get_message_length() >> 8) & 0xFF),
			static_cast<std::uint8_t>((session->get_message_length() >> 16) & 0xFF),
			static_cast<std::uint8_t>((session->get_message_length() >> 24) & 0xFF),
			static_cast<std::uint8_t>(session->get_parameter_group_number() & 0xFF),
			static_cast<std::uint8_t>((session->get_parameter_group_number() >> 8) & 0xFF),
			static_cast<std::uint8_t>((session->get_parameter_group_number() >> 16) & 0xFF)
		};
		return sendCANFrameCallback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement),
		                            CANDataSpan(buffer.data(), buffer.size()),
		                            std::static_pointer_cast<InternalControlFunction>(session->get_source()),
		                            session->get_destination(),
		                            CANIdentifier::CANPriority::PriorityLowest7);
	}

	bool ExtendedTransportProtocolManager::send_clear_to_send(const std::shared_ptr<ExtendedTransportProtocolSession> &session) const
	{
		std::uint32_t nextPacketNumber = session->get_last_packet_number() + 1;
		std::uint8_t packetLimit = session->get_cts_number_of_packet_limit();

		if (packetLimit > session->get_number_of_remaining_packets())
		{
			packetLimit = static_cast<std::uint8_t>(session->get_number_of_remaining_packets());
		}

		const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer{
			CLEAR_TO_SEND_MULTIPLEXOR,
			packetLimit,
			static_cast<std::uint8_t>(nextPacketNumber & 0xFF),
			static_cast<std::uint8_t>((nextPacketNumber >> 8) & 0xFF),
			static_cast<std::uint8_t>((nextPacketNumber >> 16) & 0xFF),
			static_cast<std::uint8_t>(session->get_parameter_group_number() & 0xFF),
			static_cast<std::uint8_t>((session->get_parameter_group_number() >> 8) & 0xFF),
			static_cast<std::uint8_t>((session->get_parameter_group_number() >> 16) & 0xFF)
		};
		bool retVal = sendCANFrameCallback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement),
		                                   CANDataSpan(buffer.data(), buffer.size()),
		                                   std::static_pointer_cast<InternalControlFunction>(session->get_destination()), // Since we're the receiving side, we are the destination of the session
		                                   session->get_source(),
		                                   CANIdentifier::CANPriority::PriorityLowest7);
		if (retVal)
		{
			session->set_acknowledged_packet_number(session->get_last_packet_number());
		}
		return retVal;
	}

	bool isobus::ExtendedTransportProtocolManager::send_data_packet_offset(const std::shared_ptr<ExtendedTransportProtocolSession> &session) const
	{
		std::uint8_t packetsThisSegment = 0xFF;
		if (packetsThisSegment > session->get_number_of_remaining_packets())
		{
			packetsThisSegment = static_cast<std::uint8_t>(session->get_number_of_remaining_packets());
		}

		if (packetsThisSegment > session->get_cts_number_of_packet_limit())
		{
			packetsThisSegment = session->get_cts_number_of_packet_limit();
		}
		if (packetsThisSegment > configuration->get_number_of_packets_per_dpo_message())
		{
			LOG_DEBUG("[TP]: Received Request To Send (RTS) with a CTS packet count of %hu, which is greater than the configured maximum of %hu, using the configured maximum instead.",
			          packetsThisSegment,
			          configuration->get_number_of_packets_per_dpo_message());
			packetsThisSegment = configuration->get_number_of_packets_per_dpo_message();
		}

		const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer{
			DATA_PACKET_OFFSET_MULTIPLXOR,
			packetsThisSegment,
			static_cast<std::uint8_t>(session->get_last_packet_number()),
			static_cast<std::uint8_t>((session->get_last_packet_number() >> 8) & 0xFF),
			static_cast<std::uint8_t>((session->get_last_packet_number() >> 16) & 0xFF),
			static_cast<std::uint8_t>(session->get_parameter_group_number() & 0xFF),
			static_cast<std::uint8_t>((session->get_parameter_group_number() >> 8) & 0xFF),
			static_cast<std::uint8_t>((session->get_parameter_group_number() >> 16) & 0xFF)
		};
		bool retVal = sendCANFrameCallback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement),
		                                   CANDataSpan(buffer.data(), buffer.size()),
		                                   std::static_pointer_cast<InternalControlFunction>(session->get_source()),
		                                   session->get_destination(),
		                                   CANIdentifier::CANPriority::PriorityLowest7);
		if (retVal)
		{
			session->set_dpo_number_of_packets(packetsThisSegment);
			session->set_sequence_number_offset(session->get_last_packet_number());
			session->set_last_sequency_number(0);
		}
		return retVal;
	}

	bool ExtendedTransportProtocolManager::send_end_of_session_acknowledgement(const std::shared_ptr<ExtendedTransportProtocolSession> &session) const
	{
		std::uint32_t messageLength = session->get_message_length();
		std::uint32_t parameterGroupNumber = session->get_parameter_group_number();

		const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer{
			END_OF_MESSAGE_ACKNOWLEDGE_MULTIPLEXOR,
			static_cast<std::uint8_t>(messageLength & 0xFF),
			static_cast<std::uint8_t>((messageLength >> 8) & 0xFF),
			static_cast<std::uint8_t>((messageLength >> 16) & 0xFF),
			static_cast<std::uint8_t>((messageLength >> 24) & 0xFF),
			static_cast<std::uint8_t>(parameterGroupNumber & 0xFF),
			static_cast<std::uint8_t>((parameterGroupNumber >> 8) & 0xFF),
			static_cast<std::uint8_t>((parameterGroupNumber >> 16) & 0xFF),
		};

		return sendCANFrameCallback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement),
		                            CANDataSpan(buffer.data(), buffer.size()),
		                            std::static_pointer_cast<InternalControlFunction>(session->get_destination()), // Since we're the receiving side, we are the destination of the session
		                            session->get_source(),
		                            CANIdentifier::CANPriority::PriorityLowest7);
	}

	bool ExtendedTransportProtocolManager::has_session(std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination)
	{
		return std::any_of(activeSessions.begin(), activeSessions.end(), [&](const std::shared_ptr<ExtendedTransportProtocolSession> &session) {
			return session->matches(source, destination);
		});
	}

	std::shared_ptr<ExtendedTransportProtocolManager::ExtendedTransportProtocolSession> ExtendedTransportProtocolManager::get_session(std::shared_ptr<ControlFunction> source,
	                                                                                                                                  std::shared_ptr<ControlFunction> destination)
	{
		auto result = std::find_if(activeSessions.begin(), activeSessions.end(), [&](const std::shared_ptr<ExtendedTransportProtocolSession> &session) {
			return session->matches(source, destination);
		});
		return (activeSessions.end() != result) ? (*result) : nullptr;
	}

	const std::vector<std::shared_ptr<ExtendedTransportProtocolManager::ExtendedTransportProtocolSession>> &ExtendedTransportProtocolManager::get_sessions() const
	{
		return activeSessions;
	}
}
