//================================================================================================
/// @file can_extended_transport_protocol.cpp
///
/// @brief A protocol class that handles the ISO11783 extended transport protocol.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#include "can_extended_transport_protocol.hpp"

#include "can_general_parameter_group_numbers.hpp"
#include "can_network_configuration.hpp"
#include "can_warning_logger.hpp"
#include "system_timing.hpp"

#include <algorithm>

namespace isobus
{

	ExtendedTransportProtocolManager ExtendedTransportProtocolManager::Protocol;

	ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::ExtendedTransportProtocolSession(Direction sessionDirection, std::uint8_t canPortIndex) :
	  state(StateMachineState::None),
	  sessionMessage(canPortIndex),
	  sessionCompleteCallback(nullptr),
	  frameChunkCallback(nullptr),
	  timestamp_ms(0),
	  lastPacketNumber(0),
	  packetCount(0),
	  processedPacketsThisSession(0),
	  sessionDirection(sessionDirection)
	{
	}

	bool ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::operator==(const ExtendedTransportProtocolSession &obj)
	{
		return ((sessionMessage.get_source_control_function() == obj.sessionMessage.get_source_control_function()) &&
		        (sessionMessage.get_destination_control_function() == obj.sessionMessage.get_destination_control_function()) &&
		        (sessionMessage.get_identifier().get_parameter_group_number() == obj.sessionMessage.get_identifier().get_parameter_group_number()));
	}

	ExtendedTransportProtocolManager::ExtendedTransportProtocolSession::~ExtendedTransportProtocolSession()
	{
	}

	ExtendedTransportProtocolManager::ExtendedTransportProtocolManager()
	{
	}

	ExtendedTransportProtocolManager ::~ExtendedTransportProtocolManager()
	{
		if (initialized)
		{
			CANNetworkManager::CANNetwork.remove_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolDataTransfer), process_message, this);
			CANNetworkManager::CANNetwork.remove_protocol_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement), process_message, this);
		}
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

	void ExtendedTransportProtocolManager::process_message(CANMessage *const message)
	{
		if (nullptr != message)
		{
			switch (message->get_identifier().get_parameter_group_number())
			{
				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement):
				{
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolDataTransfer):
				{
				}
				break;
			}
		}
	}

	void ExtendedTransportProtocolManager::process_message(CANMessage *const message, void *parent)
	{
		if (nullptr != parent)
		{
			reinterpret_cast<ExtendedTransportProtocolManager *>(parent)->process_message(message);
		}
	}

	bool ExtendedTransportProtocolManager::protocol_transmit_message(std::uint32_t parameterGroupNumber,
	                                                                 const std::uint8_t *dataBuffer,
	                                                                 std::uint32_t messageLength,
	                                                                 ControlFunction *source,
	                                                                 ControlFunction *destination,
	                                                                 TransmitCompleteCallback sessionCompleteCallback,
	                                                                 void *parentPointer,
	                                                                 DataChunkCallback frameChunkCallback)
	{
		ExtendedTransportProtocolSession *session;
		bool retVal = false;

		if ((messageLength < MAX_PROTOCOL_DATA_LENGTH) &&
		    (messageLength > 8) &&
		    ((nullptr != dataBuffer) ||
		     (nullptr != frameChunkCallback)) &&
		    (nullptr != source) &&
		    (true == source->get_address_valid()) &&
		    ((nullptr == destination) ||
		     (destination->get_address_valid())) &&
		    (!get_session(session, source, destination, parameterGroupNumber)))
		{
			ExtendedTransportProtocolSession *newSession = new ExtendedTransportProtocolSession(ExtendedTransportProtocolSession::Direction::Transmit,
			                                                                                    source->get_can_port());

			newSession->sessionMessage.set_data(dataBuffer, messageLength);
			newSession->sessionMessage.set_source_control_function(source);
			newSession->sessionMessage.set_destination_control_function(destination);
			newSession->packetCount = (messageLength / PROTOCOL_BYTES_PER_FRAME);
			newSession->lastPacketNumber = 0;
			newSession->processedPacketsThisSession = 0;
			newSession->sessionCompleteCallback = sessionCompleteCallback;
			newSession->frameChunkCallback = frameChunkCallback;
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
			CANStackLogger::CAN_stack_log("[ETP]: New ETP Session. Dest: " + std::to_string(static_cast<int>(destination->get_address())));
		}
		return retVal;
	}

	void ExtendedTransportProtocolManager::update(CANLibBadge<CANNetworkManager>)
	{
	}

	bool ExtendedTransportProtocolManager::abort_session(ExtendedTransportProtocolSession *session, ConnectionAbortReason reason)
	{
		bool retVal = false;

		if (nullptr != session)
		{
			InternalControlFunction *myControlFunction;
			ControlFunction *partnerControlFunction;
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

	bool ExtendedTransportProtocolManager::abort_session(std::uint32_t parameterGroupNumber, ConnectionAbortReason reason, InternalControlFunction *source, ControlFunction *destination)
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

	void ExtendedTransportProtocolManager::close_session(ExtendedTransportProtocolSession *session)
	{
		if (nullptr != session)
		{
			auto sessionLocation = std::find(activeSessions.begin(), activeSessions.end(), session);
			if (activeSessions.end() != sessionLocation)
			{
				activeSessions.erase(sessionLocation);
				delete session;
				CANStackLogger::CAN_stack_log("[ETP]: Session Closed");
			}
		}
	}

	bool ExtendedTransportProtocolManager::get_session(ExtendedTransportProtocolSession *&session, ControlFunction *source, ControlFunction *destination)
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

	bool ExtendedTransportProtocolManager::get_session(ExtendedTransportProtocolSession *&session, ControlFunction *source, ControlFunction *destination, std::uint32_t parameterGroupNumber)
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

	bool ExtendedTransportProtocolManager::send_end_of_session_acknowledgement(ExtendedTransportProtocolSession *session)
	{
		bool retVal = false;

		if (nullptr != session)
		{
			std::uint32_t totalBytesTransferred = (session->processedPacketsThisSession * PROTOCOL_BYTES_PER_FRAME);
			const std::uint8_t dataBuffer[8] = { EXTENDED_END_OF_MESSAGE_ACKNOWLEDGEMENT,
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
			                                                        reinterpret_cast<InternalControlFunction *>(session->sessionMessage.get_source_control_function()),
			                                                        session->sessionMessage.get_destination_control_function(),
			                                                        CANIdentifier::CANPriority::PriorityDefault6);
		}
		return retVal;
	}

	bool ExtendedTransportProtocolManager::send_extended_connection_mode_request_to_send(const ExtendedTransportProtocolSession *session)
	{
		bool retVal = false;

		if (nullptr != session)
		{
			const std::uint8_t dataBuffer[8] = { EXTENDED_REQUEST_TO_SEND_MULTIPLEXOR,
				                         static_cast<std::uint8_t>(session->sessionMessage.get_data_length() & 0xFF),
				                         static_cast<std::uint8_t>((session->sessionMessage.get_data_length() >> 8) & 0xFF),
				                         static_cast<std::uint8_t>((session->sessionMessage.get_data_length() >> 16) & 0xFF),
				                         static_cast<std::uint8_t>((session->sessionMessage.get_data_length() >> 24) & 0xFF),
				                         static_cast<std::uint8_t>(session->sessionMessage.get_identifier().get_parameter_group_number() & 0xFF),
				                         static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 8) & 0xFF),
				                         static_cast<std::uint8_t>((session->sessionMessage.get_identifier().get_parameter_group_number() >> 16) & 0xFF) };
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ExtendedTransportProtocolConnectionManagement),
			                                                        dataBuffer,
			                                                        CAN_DATA_LENGTH,
			                                                        reinterpret_cast<InternalControlFunction *>(session->sessionMessage.get_source_control_function()),
			                                                        session->sessionMessage.get_destination_control_function(),
			                                                        CANIdentifier::CANPriority::PriorityDefault6);
		}
		return retVal;
	}

	bool ExtendedTransportProtocolManager::send_extended_connection_mode_data_packet_offset(const ExtendedTransportProtocolSession *session)
	{
		bool retVal = false;

		if (nullptr != session)
		{
			const std::uint8_t dataBuffer[8] = { EXTENDED_DATA_PACKET_OFFSET_MULTIPLEXOR,
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
			                                                        reinterpret_cast<InternalControlFunction *>(session->sessionMessage.get_source_control_function()),
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

} // namespace isobus
