//================================================================================================
/// @file can_transport_protocol.cpp
///
/// @brief A protocol that handles the ISO11783/J1939 transport protocol.
/// It handles both the broadcast version (BAM) and and the connection mode version.
/// @author Adrian Del Grosso
/// @author Daan Steenbergen
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#include "isobus/isobus/can_transport_protocol.hpp"

#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include "isobus/utility/to_string.hpp"

#include <algorithm>
#include <memory>

namespace isobus
{
	TransportProtocolManager::TransportProtocolSession::TransportProtocolSession(Direction direction,
	                                                                             std::unique_ptr<CANMessageData> data,
	                                                                             std::uint32_t parameterGroupNumber,
	                                                                             std::uint16_t totalMessageSize,
	                                                                             std::uint8_t totalNumberOfPackets,
	                                                                             std::uint8_t clearToSendPacketMax,
	                                                                             std::shared_ptr<ControlFunction> source,
	                                                                             std::shared_ptr<ControlFunction> destination,
	                                                                             TransmitCompleteCallback sessionCompleteCallback,
	                                                                             void *parentPointer) :
	  direction(direction),
	  parameterGroupNumber(parameterGroupNumber),
	  data(std::move(data)),
	  totalMessageSize(totalMessageSize),
	  source(source),
	  destination(destination),
	  totalNumberOfPackets(totalNumberOfPackets),
	  clearToSendPacketCountMax(clearToSendPacketMax),
	  sessionCompleteCallback(sessionCompleteCallback),
	  parent(parentPointer)
	{
	}

	bool TransportProtocolManager::TransportProtocolSession::operator==(const TransportProtocolSession &obj) const
	{
		return ((source == obj.source) && (destination == obj.destination) && (parameterGroupNumber == obj.parameterGroupNumber));
	}

	bool TransportProtocolManager::TransportProtocolSession::matches(std::shared_ptr<ControlFunction> other_source, std::shared_ptr<ControlFunction> other_destination) const
	{
		return ((source == other_source) && (destination == other_destination));
	}

	TransportProtocolManager::TransportProtocolSession::Direction TransportProtocolManager::TransportProtocolSession::get_direction() const
	{
		return direction;
	}

	TransportProtocolManager::StateMachineState TransportProtocolManager::TransportProtocolSession::get_state() const
	{
		return state;
	}

	std::uint32_t TransportProtocolManager::TransportProtocolSession::get_message_length() const
	{
		return totalMessageSize;
	}

	CANMessageData &TransportProtocolManager::TransportProtocolSession::get_data() const
	{
		return *data;
	}

	std::shared_ptr<ControlFunction> TransportProtocolManager::TransportProtocolSession::get_source() const
	{
		return source;
	}

	std::shared_ptr<ControlFunction> TransportProtocolManager::TransportProtocolSession::get_destination() const
	{
		return destination;
	}

	std::uint32_t TransportProtocolManager::TransportProtocolSession::get_parameter_group_number() const
	{
		return parameterGroupNumber;
	}

	bool TransportProtocolManager::TransportProtocolSession::is_broadcast() const
	{
		return (nullptr == destination);
	}

	TransportProtocolManager::TransportProtocolSession TransportProtocolManager::TransportProtocolSession::create_receive_session(std::uint32_t parameterGroupNumber,
	                                                                                                                              std::uint16_t totalMessageSize,
	                                                                                                                              std::uint8_t totalNumberOfPackets,
	                                                                                                                              std::uint8_t clearToSendPacketMax,
	                                                                                                                              std::shared_ptr<ControlFunction> source,
	                                                                                                                              std::shared_ptr<ControlFunction> destination)
	{
		return TransportProtocolSession(TransportProtocolSession::Direction::Receive,
		                                std::unique_ptr<CANMessageData>(new CANMessageDataVector(totalMessageSize)),
		                                parameterGroupNumber,
		                                totalMessageSize,
		                                totalNumberOfPackets,
		                                clearToSendPacketMax,
		                                source,
		                                destination,
		                                nullptr,
		                                nullptr);
	}

	TransportProtocolManager::TransportProtocolSession TransportProtocolManager::TransportProtocolSession::create_transmit_session(std::uint32_t parameterGroupNumber,
	                                                                                                                               std::unique_ptr<CANMessageData> data,
	                                                                                                                               std::shared_ptr<ControlFunction> source,
	                                                                                                                               std::shared_ptr<ControlFunction> destination,
	                                                                                                                               std::uint8_t clearToSendPacketMax,
	                                                                                                                               TransmitCompleteCallback sessionCompleteCallback,
	                                                                                                                               void *parentPointer)
	{
		auto totalMessageSize = static_cast<std::uint16_t>(data->size());
		auto totalPacketCount = static_cast<std::uint8_t>(totalMessageSize / PROTOCOL_BYTES_PER_FRAME);
		if (0 != (totalMessageSize % PROTOCOL_BYTES_PER_FRAME))
		{
			totalPacketCount++;
		}
		return TransportProtocolSession(TransportProtocolSession::Direction::Transmit,
		                                std::move(data),
		                                parameterGroupNumber,
		                                totalMessageSize,
		                                totalPacketCount,
		                                clearToSendPacketMax,
		                                source,
		                                destination,
		                                sessionCompleteCallback,
		                                parentPointer);
	}

	void TransportProtocolManager::TransportProtocolSession::set_state(StateMachineState value)
	{
		state = value;
		timestamp_ms = SystemTiming::get_timestamp_ms();
	}

	std::uint8_t TransportProtocolManager::TransportProtocolSession::get_cts_number_of_packets_remaining() const
	{
		std::uint8_t packetsSinceCTS = get_last_packet_number() - lastAcknowledgedPacketNumber;
		return clearToSendPacketCount - packetsSinceCTS;
	}

	void TransportProtocolManager::TransportProtocolSession::set_cts_number_of_packets(std::uint8_t value)
	{
		clearToSendPacketCount = value;
		timestamp_ms = SystemTiming::get_timestamp_ms();
	}

	std::uint8_t TransportProtocolManager::TransportProtocolSession::get_cts_number_of_packets() const
	{
		return clearToSendPacketCount;
	}

	std::uint8_t TransportProtocolManager::TransportProtocolSession::get_rts_number_of_packet_limit() const
	{
		return clearToSendPacketCountMax;
	}

	std::uint8_t TransportProtocolManager::TransportProtocolSession::get_last_sequence_number() const
	{
		return lastSequenceNumber;
	}

	std::uint8_t TransportProtocolManager::TransportProtocolSession::get_last_packet_number() const
	{
		return lastSequenceNumber;
	}

	void TransportProtocolManager::TransportProtocolSession::set_last_sequency_number(std::uint8_t value)
	{
		lastSequenceNumber = value;
		timestamp_ms = SystemTiming::get_timestamp_ms();
	}

	void TransportProtocolManager::TransportProtocolSession::set_acknowledged_packet_number(std::uint8_t value)
	{
		lastAcknowledgedPacketNumber = value;
		timestamp_ms = SystemTiming::get_timestamp_ms();
	}

	std::uint8_t TransportProtocolManager::TransportProtocolSession::get_number_of_remaining_packets() const
	{
		return totalNumberOfPackets - get_last_packet_number();
	}

	std::uint8_t TransportProtocolManager::TransportProtocolSession::get_total_number_of_packets() const
	{
		return totalNumberOfPackets;
	}

	TransportProtocolManager::TransportProtocolManager(const SendCANFrameCallback &sendCANFrameCallback,
	                                                   const CANMessageReceivedCallback &canMessageReceivedCallback,
	                                                   const CANNetworkConfiguration *configuration) :
	  sendCANFrameCallback(sendCANFrameCallback),
	  canMessageReceivedCallback(canMessageReceivedCallback),
	  configuration(configuration)
	{
	}

	void TransportProtocolManager::process_broadcast_announce_message(const std::shared_ptr<ControlFunction> source,
	                                                                  std::uint32_t parameterGroupNumber,
	                                                                  std::uint16_t totalMessageSize,
	                                                                  std::uint8_t totalNumberOfPackets)
	{
		// The standard defines that we may not send aborts for messages with a global destination, we can only ignore them if we need to
		if (activeSessions.size() >= configuration->get_max_number_transport_protocol_sessions())
		{
			// TODO: consider using maximum memory instead of maximum number of sessions
			CANStackLogger::warn("[TP]: Ignoring Broadcast Announcement Message (BAM) for 0x%05X, configured maximum number of sessions reached.", parameterGroupNumber);
		}
		else
		{
			auto oldSession = get_session(source, nullptr);
			if (nullptr != oldSession)
			{
				CANStackLogger::warn("[TP]: Received Broadcast Announcement Message (BAM) while a session already existed for this source (%hu), overwriting for 0x%05X...", source->get_address(), parameterGroupNumber);
				close_session(*oldSession, false);
			}

			auto data = std::unique_ptr<CANMessageData>(new CANMessageDataVector(totalMessageSize));

			TransportProtocolSession newSession = TransportProtocolSession::create_receive_session(parameterGroupNumber,
			                                                                                       totalMessageSize,
			                                                                                       totalNumberOfPackets,
			                                                                                       0xFF, // Arbitrary - unused for broadcast
			                                                                                       source,
			                                                                                       nullptr); // Global destination
			newSession.set_state(StateMachineState::RxDataSession);
			activeSessions.push_back(std::move(newSession));

			CANStackLogger::debug("[TP]: New rx broadcast message session for 0x%05X. Source: %hu", parameterGroupNumber, source->get_address());
		}
	}

	void TransportProtocolManager::process_request_to_send(const std::shared_ptr<ControlFunction> source,
	                                                       const std::shared_ptr<ControlFunction> destination,
	                                                       std::uint32_t parameterGroupNumber,
	                                                       std::uint16_t totalMessageSize,
	                                                       std::uint8_t totalNumberOfPackets,
	                                                       std::uint8_t clearToSendPacketMax)
	{
		if (activeSessions.size() >= configuration->get_max_number_transport_protocol_sessions())
		{
			// TODO: consider using maximum memory instead of maximum number of sessions
			CANStackLogger::warn("[TP]: Replying with abort to Request To Send (RTS) for 0x%05X, configured maximum number of sessions reached.", parameterGroupNumber);
			send_abort(std::static_pointer_cast<InternalControlFunction>(destination), source, parameterGroupNumber, ConnectionAbortReason::AlreadyInCMSession);
		}
		else
		{
			auto oldSession = get_session(source, destination);
			if (nullptr != oldSession)
			{
				if (oldSession->get_parameter_group_number() != parameterGroupNumber)
				{
					CANStackLogger::error("[TP]: Received Request To Send (RTS) while a session already existed for this source and destination, aborting for 0x%05X...", parameterGroupNumber);
					abort_session(*oldSession, ConnectionAbortReason::AlreadyInCMSession);
				}
				else
				{
					CANStackLogger::warn("[TP]: Received Request To Send (RTS) while a session already existed for this source and destination and parameterGroupNumber, overwriting for 0x%05X...", parameterGroupNumber);
					close_session(*oldSession, false);
				}
			}

			auto data = std::unique_ptr<CANMessageData>(new CANMessageDataVector(totalMessageSize));

			if (clearToSendPacketMax > configuration->get_number_of_packets_per_cts_message())
			{
				CANStackLogger::debug("[TP]: Received Request To Send (RTS) with a CTS packet count of %hu, which is greater than the configured maximum of %hu, using the configured maximum instead.", clearToSendPacketMax, configuration->get_number_of_packets_per_cts_message());
				clearToSendPacketMax = configuration->get_number_of_packets_per_cts_message();
			}

			TransportProtocolSession newSession = TransportProtocolSession::create_receive_session(parameterGroupNumber,
			                                                                                       totalMessageSize,
			                                                                                       totalNumberOfPackets,
			                                                                                       clearToSendPacketMax,
			                                                                                       source,
			                                                                                       destination);
			newSession.set_state(StateMachineState::ClearToSend);
			activeSessions.push_back(std::move(newSession));
		}
	}

	void TransportProtocolManager::process_clear_to_send(const std::shared_ptr<ControlFunction> source,
	                                                     const std::shared_ptr<ControlFunction> destination,
	                                                     std::uint32_t parameterGroupNumber,
	                                                     std::uint8_t packetsToBeSent,
	                                                     std::uint8_t nextPacketNumber)
	{
		auto session = get_session(destination, source);
		if (nullptr != session)
		{
			if (session->get_parameter_group_number() != parameterGroupNumber)
			{
				CANStackLogger::error("[TP]: Received a Clear To Send (CTS) message for 0x%05X while a session already existed for this source and destination, sending abort for both...", parameterGroupNumber);
				abort_session(*session, ConnectionAbortReason::ClearToSendReceivedWhileTransferInProgress);
				send_abort(std::static_pointer_cast<InternalControlFunction>(destination), source, parameterGroupNumber, ConnectionAbortReason::ClearToSendReceivedWhileTransferInProgress);
			}
			else if (nextPacketNumber > session->get_total_number_of_packets())
			{
				CANStackLogger::error("[TP]: Received a Clear To Send (CTS) message for 0x%05X with a bad sequence number, aborting...", parameterGroupNumber);
				abort_session(*session, ConnectionAbortReason::BadSequenceNumber);
			}
			else if (StateMachineState::WaitForClearToSend != session->state)
			{
				// The session exists, but we're not in the right state to receive a CTS, so we must abort
				CANStackLogger::warn("[TP]: Received a Clear To Send (CTS) message for 0x%05X, but not expecting one, aborting session.", parameterGroupNumber);
				abort_session(*session, ConnectionAbortReason::ClearToSendReceivedWhileTransferInProgress);
			}
			else
			{
				session->set_acknowledged_packet_number(nextPacketNumber - 1);
				session->set_cts_number_of_packets(packetsToBeSent);

				// If 0 was sent as the packet number, they want us to wait.
				// Just sit here in this state until we get a non-zero packet count
				if (0 != packetsToBeSent)
				{
					session->set_state(StateMachineState::TxDataSession);
				}
			}
		}
		else
		{
			// We got a CTS but no session exists, by the standard we must ignore it
			CANStackLogger::warn("[TP]: Received Clear To Send (CTS) for 0x%05X while no session existed for this source and destination, ignoring...", parameterGroupNumber);
		}
	}

	void TransportProtocolManager::process_end_of_session_acknowledgement(const std::shared_ptr<ControlFunction> source,
	                                                                      const std::shared_ptr<ControlFunction> destination,
	                                                                      std::uint32_t parameterGroupNumber)
	{
		auto session = get_session(destination, source);
		if (nullptr != session)
		{
			if (StateMachineState::WaitForEndOfMessageAcknowledge == session->state)
			{
				CANStackLogger::debug("[TP]: Completed rx session for 0x%05X from %hu", parameterGroupNumber, source->get_address());
				session->state = StateMachineState::None;
				close_session(*session, true);
			}
			else
			{
				// The session exists, but we're not in the right state to receive an EOM, by the standard we must ignore it
				CANStackLogger::warn("[TP]: Received an End Of Message Acknowledgement message for 0x%05X, but not expecting one, ignoring.", parameterGroupNumber);
			}
		}
		else
		{
			CANStackLogger::warn("[TP]: Received End Of Message Acknowledgement for 0x%05X while no session existed for this source and destination, ignoring.", parameterGroupNumber);
		}
	}

	void TransportProtocolManager::process_abort(const std::shared_ptr<ControlFunction> source,
	                                             const std::shared_ptr<ControlFunction> destination,
	                                             std::uint32_t parameterGroupNumber,
	                                             TransportProtocolManager::ConnectionAbortReason reason)
	{
		bool foundSession = false;

		auto session = get_session(source, destination);
		if ((nullptr != session) && (session->get_parameter_group_number() == parameterGroupNumber))
		{
			foundSession = true;
			CANStackLogger::error("[TP]: Received an abort (reason=%hu) for an rx session for parameterGroupNumber 0x%05X", static_cast<std::uint8_t>(reason), parameterGroupNumber);
			close_session(*session, false);
		}
		session = get_session(destination, source);
		if ((nullptr != session) && (session->get_parameter_group_number() == parameterGroupNumber))
		{
			foundSession = true;
			CANStackLogger::error("[TP]: Received an abort (reason=%hu) for a tx session for parameterGroupNumber 0x%05X", static_cast<std::uint8_t>(reason), parameterGroupNumber);
			close_session(*session, false);
		}

		if (!foundSession)
		{
			CANStackLogger::warn("[TP]: Received an abort (reason=%hu) with no matching session for parameterGroupNumber 0x%05X", static_cast<std::uint8_t>(reason), parameterGroupNumber);
		}
	}

	void TransportProtocolManager::process_connection_management_message(const CANMessage &message)
	{
		if (CAN_DATA_LENGTH != message.get_data_length())
		{
			CANStackLogger::warn("[TP]: Received a Connection Management message of invalid length %hu", message.get_data_length());
			return;
		}

		const auto parameterGroupNumber = message.get_uint24_at(5);

		switch (message.get_uint8_at(0))
		{
			case BROADCAST_ANNOUNCE_MESSAGE_MULTIPLEXOR:
			{
				if (message.is_broadcast())
				{
					const auto totalMessageSize = message.get_uint16_at(1);
					const auto totalNumberOfPackets = message.get_uint8_at(3);
					process_broadcast_announce_message(message.get_source_control_function(),
					                                   parameterGroupNumber,
					                                   totalMessageSize,
					                                   totalNumberOfPackets);
				}
				else
				{
					CANStackLogger::warn("[TP]: Received a Broadcast Announcement Message (BAM) with a non-global destination, ignoring");
				}
			}
			break;

			case REQUEST_TO_SEND_MULTIPLEXOR:
			{
				if (message.is_broadcast())
				{
					CANStackLogger::warn("[TP]: Received a Request to Send (RTS) message with a global destination, ignoring");
				}
				else
				{
					const auto totalMessageSize = message.get_uint16_at(1);
					const auto totalNumberOfPackets = message.get_uint8_at(3);
					const auto clearToSendPacketMax = message.get_uint8_at(4);
					process_request_to_send(message.get_source_control_function(),
					                        message.get_destination_control_function(),
					                        parameterGroupNumber,
					                        totalMessageSize,
					                        totalNumberOfPackets,
					                        clearToSendPacketMax);
				}
			}
			break;

			case CLEAR_TO_SEND_MULTIPLEXOR:
			{
				if (message.is_broadcast())
				{
					CANStackLogger::warn("[TP]: Received a Clear to Send (CTS) message with a global destination, ignoring");
				}
				else
				{
					const auto packetsToBeSent = message.get_uint8_at(1);
					const auto nextPacketNumber = message.get_uint8_at(2);
					process_clear_to_send(message.get_source_control_function(),
					                      message.get_destination_control_function(),
					                      parameterGroupNumber,
					                      packetsToBeSent,
					                      nextPacketNumber);
				}
			}
			break;

			case END_OF_MESSAGE_ACKNOWLEDGE_MULTIPLEXOR:
			{
				if (message.is_broadcast())
				{
					CANStackLogger::warn("[TP]: Received an End of Message Acknowledge message with a global destination, ignoring");
				}
				else
				{
					process_end_of_session_acknowledgement(message.get_source_control_function(),
					                                       message.get_destination_control_function(),
					                                       parameterGroupNumber);
				}
			}
			break;

			case CONNECTION_ABORT_MULTIPLEXOR:
			{
				if (message.is_broadcast())
				{
					CANStackLogger::warn("[TP]: Received an Abort message with a global destination, ignoring");
				}
				else
				{
					const auto reason = static_cast<ConnectionAbortReason>(message.get_uint8_at(1));
					process_abort(message.get_source_control_function(),
					              message.get_destination_control_function(),
					              parameterGroupNumber,
					              reason);
				}
			}
			break;

			default:
			{
				CANStackLogger::warn("[TP]: Bad Mux in Transport Protocol Connection Management message");
			}
			break;
		}
	}

	void TransportProtocolManager::process_data_transfer_message(const CANMessage &message)
	{
		if (CAN_DATA_LENGTH != message.get_data_length())
		{
			CANStackLogger::warn("[TP]: Received a Data Transfer message of invalid length %hu", message.get_data_length());
			return;
		}

		auto source = message.get_source_control_function();
		auto destination = message.is_broadcast() ? nullptr : message.get_destination_control_function();

		auto sequenceNumber = message.get_uint8_at(SEQUENCE_NUMBER_DATA_INDEX);

		auto session = get_session(source, destination);
		if (nullptr != session)
		{
			if (StateMachineState::RxDataSession != session->state)
			{
				CANStackLogger::warn("[TP]: Received a Data Transfer message from %hu while not expecting one, sending abort", source->get_address());
				abort_session(*session, ConnectionAbortReason::UnexpectedDataTransferPacketReceived);
			}
			else if (sequenceNumber == session->get_last_sequence_number())
			{
				CANStackLogger::error("[TP]: Aborting rx session for 0x%05X due to duplicate sequence number", session->get_parameter_group_number());
				abort_session(*session, ConnectionAbortReason::DuplicateSequenceNumber);
			}
			else if (sequenceNumber == (session->get_last_sequence_number() + 1))
			{
				// Convert data type to a vector to allow for manipulation
				auto &data = static_cast<CANMessageDataVector &>(session->get_data());

				// Correct sequence number, copy the data
				for (std::uint8_t i = 0; i < PROTOCOL_BYTES_PER_FRAME; i++)
				{
					std::uint32_t currentDataIndex = (PROTOCOL_BYTES_PER_FRAME * session->get_last_packet_number()) + i;
					if (currentDataIndex < session->get_message_length())
					{
						data.set_byte(currentDataIndex, message.get_uint8_at(1 + i));
					}
					else
					{
						// Reached the end of the message, no need to copy any more data
						break;
					}
				}

				session->set_last_sequency_number(sequenceNumber);
				if (session->get_number_of_remaining_packets() == 0)
				{
					// Send End of Message Acknowledgement for sessions with specific destination only
					if (!message.is_broadcast())
					{
						send_end_of_session_acknowledgement(*session);
					}
					else
					{
						CANStackLogger::debug("[TP]: Completed broadcast rx session for 0x%05X", session->get_parameter_group_number());
					}

					// Construct the completed message
					CANMessage completedMessage(0);
					completedMessage.set_identifier(CANIdentifier(CANIdentifier::Type::Extended,
					                                              session->get_parameter_group_number(),
					                                              CANIdentifier::CANPriority::PriorityDefault6,
					                                              session->is_broadcast() ? CANIdentifier::GLOBAL_ADDRESS : destination->get_address(),
					                                              source->get_address()));
					completedMessage.set_source_control_function(source);
					completedMessage.set_destination_control_function(destination);
					completedMessage.set_data(data.data().begin(), static_cast<std::uint32_t>(data.size()));

					canMessageReceivedCallback(completedMessage);
					close_session(*session, true);
				}
				else if (session->get_cts_number_of_packets_remaining() == 0)
				{
					send_clear_to_send(*session);
				}
			}
			else
			{
				CANStackLogger::error("[TP]: Aborting rx session for 0x%05X due to bad sequence number", session->get_parameter_group_number());
				abort_session(*session, ConnectionAbortReason::BadSequenceNumber);
			}
		}
		else if (!message.is_broadcast())
		{
			CANStackLogger::warn("[TP]: Received a Data Transfer message from %hu with no matching session, ignoring...", source->get_address());
		}
	}

	void TransportProtocolManager::process_message(const CANMessage &message)
	{
		// TODO: Allow sniffing of messages to all addresses, not just the ones we normally listen to (#297)
		if (message.has_valid_source_control_function() && (message.has_valid_destination_control_function() || message.is_broadcast()))
		{
			switch (message.get_identifier().get_parameter_group_number())
			{
				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolConnectionManagement):
				{
					process_connection_management_message(message);
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolDataTransfer):
				{
					process_data_transfer_message(message);
				}
				break;

				default:
					break;
			}
		}
	}

	bool TransportProtocolManager::protocol_transmit_message(std::uint32_t parameterGroupNumber,
	                                                         std::unique_ptr<CANMessageData> &data,
	                                                         std::shared_ptr<ControlFunction> source,
	                                                         std::shared_ptr<ControlFunction> destination,
	                                                         TransmitCompleteCallback sessionCompleteCallback,
	                                                         void *parentPointer)
	{
		// Return false early if we can't send the message
		if ((nullptr == data) || (data->size() <= CAN_DATA_LENGTH) || (data->size() > MAX_PROTOCOL_DATA_LENGTH))
		{
			// Invalid message length
			return false;
		}
		else if ((nullptr == source) || (!source->get_address_valid()) || has_session(source, destination))
		{
			return false;
		}

		// We can handle this message! If we only have a view of the data, let's clone the data,
		// so we don't have to worry about it being deleted.
		data = data->copy_if_not_owned(std::move(data));

		TransportProtocolSession session = TransportProtocolSession::create_transmit_session(parameterGroupNumber,
		                                                                                     std::move(data),
		                                                                                     source,
		                                                                                     destination,
		                                                                                     configuration->get_number_of_packets_per_cts_message(),
		                                                                                     sessionCompleteCallback,
		                                                                                     parentPointer);

		if (session.is_broadcast())
		{
			// Broadcast message
			session.set_state(StateMachineState::BroadcastAnnounce);
			CANStackLogger::debug("[TP]: New broadcast tx session for 0x%05X. Source: %hu",
			                      parameterGroupNumber,
			                      source->get_address());
		}
		else
		{
			// Destination specific message
			session.set_state(StateMachineState::RequestToSend);
			CANStackLogger::debug("[TP]: New tx session for 0x%05X. Source: %hu, destination: %hu",
			                      parameterGroupNumber,
			                      source->get_address(),
			                      destination->get_address());
		}
		activeSessions.push_back(std::move(session));
		return true;
	}

	void TransportProtocolManager::update()
	{
		// We use a fancy for loop here to allow us to remove sessions from the list while iterating
		for (std::size_t i = activeSessions.size(); i > 0; i--)
		{
			auto &session = activeSessions.at(i - 1);
			if (!session.get_source()->get_address_valid())
			{
				CANStackLogger::warn("[TP]: Closing active session as the source control function is no longer valid");
				close_session(session, false);
			}
			else if (!session.is_broadcast() && !session.get_destination()->get_address_valid())
			{
				CANStackLogger::warn("[TP]: Closing active session as the destination control function is no longer valid");
				close_session(session, false);
			}
			else if (StateMachineState::None != session.state)
			{
				update_state_machine(session);
			}
		}
	}

	void TransportProtocolManager::send_data_transfer_packets(TransportProtocolSession &session)
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer;
		std::uint32_t framesToSend = session.get_cts_number_of_packets_remaining();
		if (session.is_broadcast())
		{
			framesToSend = 1;
		}
		else if (framesToSend > configuration->get_max_number_of_network_manager_protocol_frames_per_update())
		{
			framesToSend = configuration->get_max_number_of_network_manager_protocol_frames_per_update();
		}

		// Try and send packets
		for (std::uint8_t i = 0; i < framesToSend; i++)
		{
			buffer[0] = session.get_last_sequence_number() + 1;

			std::uint8_t dataOffset = session.get_last_packet_number() * PROTOCOL_BYTES_PER_FRAME;
			for (std::uint8_t j = 0; j < PROTOCOL_BYTES_PER_FRAME; j++)
			{
				std::uint32_t index = dataOffset + j;
				if (index < session.get_message_length())
				{
					buffer[1 + j] = session.get_data().get_byte(index);
				}
				else
				{
					buffer[1 + j] = 0xFF;
				}
			}

			if (sendCANFrameCallback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolDataTransfer),
			                         CANDataSpan(buffer.data(), buffer.size()),
			                         std::static_pointer_cast<InternalControlFunction>(session.get_source()),
			                         session.get_destination(),
			                         CANIdentifier::CANPriority::PriorityLowest7))
			{
				session.set_last_sequency_number(session.get_last_sequence_number() + 1);
			}
			else
			{
				// Process more next time protocol is updated
				break;
			}
		}

		if (session.get_number_of_remaining_packets() == 0)
		{
			if (session.is_broadcast())
			{
				CANStackLogger::debug("[TP]: Completed broadcast tx session for 0x%05X", session.get_parameter_group_number());
				close_session(session, true);
			}
			else
			{
				session.set_state(StateMachineState::WaitForEndOfMessageAcknowledge);
			}
		}
		else if (session.get_cts_number_of_packets_remaining() == 0)
		{
			session.set_state(StateMachineState::WaitForClearToSend);
		}
	}

	void TransportProtocolManager::update_state_machine(TransportProtocolSession &session)
	{
		switch (session.state)
		{
			case StateMachineState::None:
				break;

			case StateMachineState::ClearToSend:
			{
				if (send_clear_to_send(session))
				{
					session.set_state(StateMachineState::RxDataSession);
				}
			}
			break;

			case StateMachineState::WaitForClearToSend:
			{
				if (SystemTiming::time_expired_ms(session.timestamp_ms, T2_T3_TIMEOUT_MS))
				{
					CANStackLogger::error("[TP]: Timeout rx session for 0x%05X (expected CTS)", session.get_parameter_group_number());
					if (session.get_cts_number_of_packets() > 0)
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
			case StateMachineState::WaitForEndOfMessageAcknowledge:
			{
				if (SystemTiming::time_expired_ms(session.timestamp_ms, T2_T3_TIMEOUT_MS))
				{
					CANStackLogger::error("[TP]: Timeout tx session for 0x%05X (expected EOMA)", session.get_parameter_group_number());
					abort_session(session, ConnectionAbortReason::Timeout);
				}
			}
			break;

			case StateMachineState::RequestToSend:
			{
				if (send_request_to_send(session))
				{
					session.set_state(StateMachineState::WaitForClearToSend);
				}
			}
			break;

			case StateMachineState::BroadcastAnnounce:
			{
				if (send_broadcast_announce_message(session))
				{
					session.set_state(StateMachineState::TxDataSession);
				}
			}
			break;

			case StateMachineState::TxDataSession:
			{
				if (session.is_broadcast() && (!SystemTiming::time_expired_ms(session.timestamp_ms, configuration->get_minimum_time_between_transport_protocol_bam_frames())))
				{
					// Need to wait before sending the next data frame of the broadcast session
				}
				else
				{
					send_data_transfer_packets(session);
				}
			}
			break;

			case StateMachineState::RxDataSession:
			{
				if (session.is_broadcast())
				{
					// Broadcast message timeout check
					if (SystemTiming::time_expired_ms(session.timestamp_ms, T1_TIMEOUT_MS))
					{
						CANStackLogger::warn("[TP]: Broadcast rx session timeout");
						close_session(session, false);
					}
				}
				else if (session.get_cts_number_of_packets_remaining() == session.get_cts_number_of_packets())
				{
					// Waiting to receive the first data frame after CTS
					if (SystemTiming::time_expired_ms(session.timestamp_ms, T2_T3_TIMEOUT_MS))
					{
						CANStackLogger::error("[TP]: Timeout for destination-specific rx session (expected first data frame)");
						abort_session(session, ConnectionAbortReason::Timeout);
					}
				}
				else
				{
					// Waiting on sequencial data frames
					if (SystemTiming::time_expired_ms(session.timestamp_ms, T1_TIMEOUT_MS))
					{
						CANStackLogger::error("[TP]: Timeout for destination-specific rx session (expected sequencial data frame)");
						abort_session(session, ConnectionAbortReason::Timeout);
					}
				}
			}
			break;
		}
	}

	bool TransportProtocolManager::abort_session(const TransportProtocolSession &session, ConnectionAbortReason reason)
	{
		bool retVal = false;
		std::shared_ptr<InternalControlFunction> myControlFunction;
		std::shared_ptr<ControlFunction> partnerControlFunction;
		if (TransportProtocolSession::Direction::Transmit == session.get_direction())
		{
			myControlFunction = std::static_pointer_cast<InternalControlFunction>(session.get_source());
			partnerControlFunction = session.get_destination();
		}
		else
		{
			myControlFunction = std::static_pointer_cast<InternalControlFunction>(session.get_destination());
			partnerControlFunction = session.get_source();
		}

		if ((nullptr != myControlFunction) && (nullptr != partnerControlFunction))
		{
			retVal = send_abort(myControlFunction, partnerControlFunction, session.get_parameter_group_number(), reason);
		}
		close_session(session, false);
		return retVal;
	}

	bool TransportProtocolManager::send_abort(std::shared_ptr<InternalControlFunction> sender,
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
		return sendCANFrameCallback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolConnectionManagement),
		                            CANDataSpan(buffer.data(), buffer.size()),
		                            sender,
		                            receiver,
		                            CANIdentifier::CANPriority::PriorityLowest7);
	}

	void TransportProtocolManager::close_session(const TransportProtocolSession &session, bool successful)
	{
		if ((nullptr != session.sessionCompleteCallback) && (TransportProtocolSession::Direction::Transmit == session.get_direction()))
		{
			if (auto source = session.get_source())
			{
				session.sessionCompleteCallback(session.get_parameter_group_number(),
				                                session.get_message_length(),
				                                std::static_pointer_cast<InternalControlFunction>(source),
				                                session.get_destination(),
				                                successful,
				                                session.parent);
			}
		}

		auto sessionLocation = std::find(activeSessions.begin(), activeSessions.end(), session);
		if (activeSessions.end() != sessionLocation)
		{
			activeSessions.erase(sessionLocation);
			CANStackLogger::debug("[TP]: Session Closed");
		}
	}

	bool TransportProtocolManager::send_broadcast_announce_message(const TransportProtocolSession &session) const
	{
		bool retVal = false;
		if (auto source = session.get_source())
		{
			const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer{
				BROADCAST_ANNOUNCE_MESSAGE_MULTIPLEXOR,
				static_cast<std::uint8_t>(session.get_message_length() & 0xFF),
				static_cast<std::uint8_t>((session.get_message_length() >> 8) & 0xFF),
				session.get_total_number_of_packets(),
				0xFF,
				static_cast<std::uint8_t>(session.get_parameter_group_number() & 0xFF),
				static_cast<std::uint8_t>((session.get_parameter_group_number() >> 8) & 0xFF),
				static_cast<std::uint8_t>((session.get_parameter_group_number() >> 16) & 0xFF)
			};
			retVal = sendCANFrameCallback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolConnectionManagement),
			                              CANDataSpan(buffer.data(), buffer.size()),
			                              std::static_pointer_cast<InternalControlFunction>(source),
			                              nullptr,
			                              CANIdentifier::CANPriority::PriorityLowest7);
		}
		return retVal;
	}

	bool TransportProtocolManager::send_clear_to_send(TransportProtocolSession &session) const
	{
		bool retVal = false;
		// Since we're the receiving side, we are the destination of the session
		if (auto ourControlFunction = session.get_destination())
		{
			std::uint8_t packetsThisSegment = session.get_number_of_remaining_packets();
			if (packetsThisSegment > session.get_rts_number_of_packet_limit())
			{
				packetsThisSegment = session.get_rts_number_of_packet_limit();
			}
			else if (packetsThisSegment > 16)
			{
				//! @todo apply CTS number of packets recommendation of 16 via a configuration option
				packetsThisSegment = 16;
			}

			const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer{
				CLEAR_TO_SEND_MULTIPLEXOR,
				packetsThisSegment,
				static_cast<std::uint8_t>(session.get_last_packet_number() + 1),
				0xFF,
				0xFF,
				static_cast<std::uint8_t>(session.get_parameter_group_number() & 0xFF),
				static_cast<std::uint8_t>((session.get_parameter_group_number() >> 8) & 0xFF),
				static_cast<std::uint8_t>((session.get_parameter_group_number() >> 16) & 0xFF)
			};
			retVal = sendCANFrameCallback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolConnectionManagement),
			                              CANDataSpan(buffer.data(), buffer.size()),
			                              std::static_pointer_cast<InternalControlFunction>(ourControlFunction),
			                              session.get_source(),
			                              CANIdentifier::CANPriority::PriorityLowest7);
			if (retVal)
			{
				session.set_cts_number_of_packets(packetsThisSegment);
				session.set_acknowledged_packet_number(session.get_last_packet_number());
			}
		}
		return retVal;
	}

	bool TransportProtocolManager::send_request_to_send(const TransportProtocolSession &session) const
	{
		bool retVal = false;
		if (auto source = session.get_source())
		{
			const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer{
				REQUEST_TO_SEND_MULTIPLEXOR,
				static_cast<std::uint8_t>(session.get_message_length() & 0xFF),
				static_cast<std::uint8_t>((session.get_message_length() >> 8) & 0xFF),
				session.get_total_number_of_packets(),
				session.get_rts_number_of_packet_limit(),
				static_cast<std::uint8_t>(session.get_parameter_group_number() & 0xFF),
				static_cast<std::uint8_t>((session.get_parameter_group_number() >> 8) & 0xFF),
				static_cast<std::uint8_t>((session.get_parameter_group_number() >> 16) & 0xFF)
			};
			retVal = sendCANFrameCallback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolConnectionManagement),
			                              CANDataSpan(buffer.data(), buffer.size()),
			                              std::static_pointer_cast<InternalControlFunction>(source),
			                              session.get_destination(),
			                              CANIdentifier::CANPriority::PriorityLowest7);
		}
		return retVal;
	}

	bool TransportProtocolManager::send_end_of_session_acknowledgement(const TransportProtocolSession &session) const
	{
		bool retVal = false;
		// Since we're the receiving side, we are the destination of the session
		if (auto ourControlFunction = session.get_destination())
		{
			std::uint32_t messageLength = session.get_message_length();
			std::uint32_t parameterGroupNumber = session.get_parameter_group_number();

			const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer{
				END_OF_MESSAGE_ACKNOWLEDGE_MULTIPLEXOR,
				static_cast<std::uint8_t>(messageLength & 0xFF),
				static_cast<std::uint8_t>((messageLength >> 8) & 0xFF),
				session.get_total_number_of_packets(),
				0xFF,
				static_cast<std::uint8_t>(parameterGroupNumber & 0xFF),
				static_cast<std::uint8_t>((parameterGroupNumber >> 8) & 0xFF),
				static_cast<std::uint8_t>((parameterGroupNumber >> 16) & 0xFF),
			};

			retVal = sendCANFrameCallback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolConnectionManagement),
			                              CANDataSpan(buffer.data(), buffer.size()),
			                              std::static_pointer_cast<InternalControlFunction>(ourControlFunction),
			                              session.get_source(),
			                              CANIdentifier::CANPriority::PriorityLowest7);
		}
		else
		{
			CANStackLogger::warn("[TP]: Attempted to send EOM to null session");
		}
		return retVal;
	}

	bool TransportProtocolManager::has_session(std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination)
	{
		return std::any_of(activeSessions.begin(), activeSessions.end(), [&](const TransportProtocolSession &session) {
			return session.matches(source, destination);
		});
	}

	TransportProtocolManager::TransportProtocolSession *TransportProtocolManager::get_session(std::shared_ptr<ControlFunction> source,
	                                                                                          std::shared_ptr<ControlFunction> destination)
	{
		auto result = std::find_if(activeSessions.begin(), activeSessions.end(), [&](const TransportProtocolSession &session) {
			return session.matches(source, destination);
		});
		// Instead of returning a pointer, we return by reference to indicate it should not be deleted or stored
		return (activeSessions.end() != result) ? &(*result) : nullptr;
	}
}
