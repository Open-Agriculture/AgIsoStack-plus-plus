//================================================================================================
/// @file can_transport_protocol_sniffer.cpp
///
/// @brief A passive observer that reassembles ISO11783/J1939 transport protocol messages which
/// are sent between two external control functions.
/// @author Sujan Dumaru
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================

#include "isobus/isobus/can_transport_protocol_sniffer.hpp"

#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/can_transport_protocol.hpp"
#include "isobus/utility/system_timing.hpp"

#include <algorithm>
#include <cstring>

namespace isobus
{
	TransportProtocolSniffer::TransportProtocolSniffer(const ParameterGroupNumberFilter &parameterGroupNumberFilter,
	                                                   const CANMessageCallback &canMessageAssembledCallback,
	                                                   const CANNetworkConfiguration *configuration) :
	  parameterGroupNumberFilter(parameterGroupNumberFilter),
	  canMessageAssembledCallback(canMessageAssembledCallback),
	  configuration(configuration)
	{
	}

	void TransportProtocolSniffer::update()
	{
		auto currentSession = sessions.begin();

		while (sessions.end() != currentSession)
		{
			if (SystemTiming::time_expired_ms(currentSession->timestamp_ms, TransportProtocolManager::T2_T3_TIMEOUT_MS))
			{
				LOG_DEBUG("[TP-Sniffer]: Dropping the observed session for 0x%05X, nothing was seen within the protocol timeout.",
				          currentSession->parameterGroupNumber);
				currentSession = sessions.erase(currentSession);
			}
			else
			{
				++currentSession;
			}
		}
	}

	void TransportProtocolSniffer::process_message(const CANMessage &message)
	{
		if ((!message.has_valid_source_control_function()) ||
		    message.is_broadcast() ||
		    (nullptr == message.get_destination_control_function()))
		{
			return;
		}

		// Anything involving one of our own control functions is the regular manager's business, and
		// reassembling it here as well would deliver it to the sniffing callbacks twice
		if ((ControlFunction::Type::Internal == message.get_source_control_function()->get_type()) ||
		    (ControlFunction::Type::Internal == message.get_destination_control_function()->get_type()))
		{
			return;
		}

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

	void TransportProtocolSniffer::process_connection_management_message(const CANMessage &message)
	{
		if (CAN_DATA_LENGTH != message.get_data_length())
		{
			return;
		}

		auto source = message.get_source_control_function();
		auto destination = message.get_destination_control_function();

		switch (message.get_uint8_at(0))
		{
			case TransportProtocolManager::REQUEST_TO_SEND_MULTIPLEXOR:
			{
				open_session(message, message.get_uint24_at(5), message.get_uint16_at(1), message.get_uint8_at(3));
			}
			break;

			case TransportProtocolManager::CLEAR_TO_SEND_MULTIPLEXOR:
			{
				// Travels back towards the sender, so the roles are reversed compared to the session
				const auto parameterGroupNumber = message.get_uint24_at(5);
				auto currentSession = get_session(destination, source);

				if (sessions.end() != currentSession)
				{
					if (currentSession->parameterGroupNumber == parameterGroupNumber)
					{
						currentSession->timestamp_ms = SystemTiming::get_timestamp_ms();
					}
					else
					{
						// A connection we never admitted is under way, so what we hold is stale. Data
						// transfer frames carry no PGN, so its buffer would absorb the new connection
						LOG_DEBUG("[TP-Sniffer]: Dropping the stale observed session for 0x%05X, a clear to send arrived for 0x%05X instead.",
						          currentSession->parameterGroupNumber,
						          parameterGroupNumber);
						sessions.erase(currentSession);
					}
				}
			}
			break;

			case TransportProtocolManager::END_OF_MESSAGE_ACKNOWLEDGE_MULTIPLEXOR:
			{
				// Only ever travels from the receiver back towards the sender, so it closes the session
				// running the other way. Matching both ways could close an unrelated reverse transfer
				close_session(get_session(destination, source), message.get_uint24_at(5));
			}
			break;

			case TransportProtocolManager::CONNECTION_ABORT_MULTIPLEXOR:
			{
				// Either end of a connection may abort it, so like the regular manager we look both ways
				const auto parameterGroupNumber = message.get_uint24_at(5);
				close_session(get_session(source, destination), parameterGroupNumber);
				close_session(get_session(destination, source), parameterGroupNumber);
			}
			break;

			default:
				break;
		}
	}

	void TransportProtocolSniffer::close_session(std::list<SnifferSession>::iterator session, std::uint32_t parameterGroupNumber)
	{
		if ((sessions.end() != session) && (session->parameterGroupNumber == parameterGroupNumber))
		{
			LOG_DEBUG("[TP-Sniffer]: Dropping the incomplete observed session for 0x%05X, the connection was closed.",
			          session->parameterGroupNumber);
			sessions.erase(session);
		}
	}

	void TransportProtocolSniffer::open_session(const CANMessage &message,
	                                            std::uint32_t parameterGroupNumber,
	                                            std::uint16_t totalMessageSize,
	                                            std::uint8_t totalNumberOfPackets)
	{
		// Whatever we were following between these two is over. This has to happen before any check
		// that can reject the new connection, or the old buffer would absorb the new one's packets
		auto previousSession = get_session(message.get_source_control_function(), message.get_destination_control_function());
		if (sessions.end() != previousSession)
		{
			sessions.erase(previousSession);
		}

		if (!parameterGroupNumberFilter(parameterGroupNumber))
		{
			return;
		}

		const std::uint32_t expectedNumberOfPackets = (totalMessageSize + (TransportProtocolManager::PROTOCOL_BYTES_PER_FRAME - 1)) /
		  TransportProtocolManager::PROTOCOL_BYTES_PER_FRAME;

		if ((totalMessageSize <= CAN_DATA_LENGTH) ||
		    (totalMessageSize > TransportProtocolManager::MAX_PROTOCOL_DATA_LENGTH) ||
		    (totalNumberOfPackets != expectedNumberOfPackets))
		{
			LOG_DEBUG("[TP-Sniffer]: Ignoring an observed Request To Send for 0x%05X, the claimed size and packet count do not agree.",
			          parameterGroupNumber);
			return;
		}

		if (sessions.size() >= configuration->get_max_number_transport_protocol_sessions())
		{
			// Observed sessions get their own budget, so that traffic between other control functions
			// can never cost us a session we need for ourselves
			LOG_DEBUG("[TP-Sniffer]: Ignoring an observed Request To Send for 0x%05X, no room left for another observed session.",
			          parameterGroupNumber);
			return;
		}

		SnifferSession newSession;
		newSession.parameterGroupNumber = parameterGroupNumber;
		newSession.source = message.get_source_control_function();
		newSession.destination = message.get_destination_control_function();
		newSession.data.resize(totalMessageSize);
		newSession.receivedPackets.resize(totalNumberOfPackets, false);
		newSession.remainingPacketCount = totalNumberOfPackets;
		newSession.timestamp_ms = SystemTiming::get_timestamp_ms();

		LOG_DEBUG("[TP-Sniffer]: Following a new observed session for 0x%05X. Source: %hu, destination: %hu",
		          parameterGroupNumber,
		          newSession.source->get_address(),
		          newSession.destination->get_address());
		sessions.push_back(std::move(newSession));
	}

	void TransportProtocolSniffer::process_data_transfer_message(const CANMessage &message)
	{
		if (CAN_DATA_LENGTH != message.get_data_length())
		{
			return;
		}

		auto currentSession = get_session(message.get_source_control_function(), message.get_destination_control_function());
		if (sessions.end() == currentSession)
		{
			return;
		}

		const auto sequenceNumber = message.get_uint8_at(TransportProtocolManager::SEQUENCE_NUMBER_DATA_INDEX);
		if ((0 == sequenceNumber) || (sequenceNumber > currentSession->receivedPackets.size()))
		{
			LOG_DEBUG("[TP-Sniffer]: Dropping the observed session for 0x%05X, sequence number %hu is out of range.",
			          currentSession->parameterGroupNumber,
			          sequenceNumber);
			sessions.erase(currentSession);
			return;
		}

		currentSession->timestamp_ms = SystemTiming::get_timestamp_ms();

		const std::size_t packetIndex = sequenceNumber - 1;
		if (currentSession->receivedPackets[packetIndex])
		{
			// The sender is repeating a packet for the benefit of the real receiver. We already have it
			return;
		}

		// An absolute packet number, so it doubles as the offset even across retransmissions
		const std::size_t dataIndex = packetIndex * TransportProtocolManager::PROTOCOL_BYTES_PER_FRAME;
		const std::size_t bytesToCopy = std::min(static_cast<std::size_t>(TransportProtocolManager::PROTOCOL_BYTES_PER_FRAME),
		                                         currentSession->data.size() - dataIndex);
		memcpy(&currentSession->data[dataIndex], message.get_data().data() + 1, bytesToCopy);

		currentSession->receivedPackets[packetIndex] = true;
		currentSession->remainingPacketCount--;

		if (0 == currentSession->remainingPacketCount)
		{
			CANIdentifier identifier(CANIdentifier::Type::Extended,
			                         currentSession->parameterGroupNumber,
			                         CANIdentifier::CANPriority::PriorityDefault6,
			                         currentSession->destination->get_address(),
			                         currentSession->source->get_address());
			CANMessage completedMessage(CANMessage::Type::Receive,
			                            identifier,
			                            std::move(currentSession->data),
			                            currentSession->source,
			                            currentSession->destination,
			                            message.get_can_port_index(),
			                            message.get_timestamp_us());

			LOG_DEBUG("[TP-Sniffer]: Completed an observed session for 0x%05X. Source: %hu, destination: %hu",
			          currentSession->parameterGroupNumber,
			          currentSession->source->get_address(),
			          currentSession->destination->get_address());

			// Drop the session before the callback runs, so that whatever it does sees a consistent list
			sessions.erase(currentSession);
			canMessageAssembledCallback(completedMessage);
		}
	}

	std::list<TransportProtocolSniffer::SnifferSession>::iterator TransportProtocolSniffer::get_session(const std::shared_ptr<ControlFunction> &source,
	                                                                                                    const std::shared_ptr<ControlFunction> &destination)
	{
		return std::find_if(sessions.begin(), sessions.end(), [&source, &destination](const SnifferSession &session) {
			return (session.source == source) && (session.destination == destination);
		});
	}
} // namespace isobus
