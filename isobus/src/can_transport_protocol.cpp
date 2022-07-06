#include "can_transport_protocol.hpp"

#include "can_lib_parameter_group_numbers.hpp"
#include "can_lib_configuration.hpp"
#include "system_timing.hpp"

#include <algorithm>

namespace isobus
{

	TransportProtocolManager TransportProtocolManager::Protocol;
	std::vector<TransportProtocolManager::TransportProtocolSession *> TransportProtocolManager::activeSessions;

	TransportProtocolManager::TransportProtocolSession::TransportProtocolSession(Direction sessionDirection) :
	  state(StateMachineState::None),
	  source(nullptr),
	  destination(nullptr),
	  parameterGroupNumber(0),
	  messageLengthBytes(0),
	  sessionDirection(sessionDirection)
	{
	}

	bool TransportProtocolManager::TransportProtocolSession::operator==(const TransportProtocolSession& obj)
	{
		return ((source == obj.source) &&
			    (destination == obj.destination) &&
			    (parameterGroupNumber == obj.parameterGroupNumber));
	}

	TransportProtocolManager::TransportProtocolSession::~TransportProtocolSession()
	{
		sessionData.clear();
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
									(activeSessions.size() < CANLibConfiguration::get_max_number_transport_protcol_sessions()) &&
									(!get_session(session, message->get_source_control_function(), message->get_destination_control_function(), pgn)))
								{
									TransportProtocolSession *newSession = new TransportProtocolSession(TransportProtocolSession::Direction::Receive);
									newSession->messageLengthBytes = (static_cast<std::uint16_t>(data[1]) | static_cast<std::uint16_t>(data[2] << 8));
									newSession->source = message->get_source_control_function();
									newSession->destination = nullptr;
									newSession->packetCount = data[3];
									newSession->parameterGroupNumber = pgn;
									newSession->sessionData.resize(newSession->messageLengthBytes);
									newSession->state = StateMachineState::RxDataSession;
									newSession->timestamp_ms = SystemTiming::get_timestamp_ms();
									activeSessions.push_back(newSession);
								}
								else
								{
									// TODO: Not sure what to do, this can't happen, so log something?
									// Don't send an abort, they're probably expecting a CTS so it'll timeout
									// Or maybe if we already had a session they sent a second BAM? Also bad
								}
							}
							else
							{
								// TODO: bad message length, log
							}
                        }
                        break;
                        
                        case REQUEST_TO_SEND_MULTIPLEXOR:
                        {
							TransportProtocolSession *session;
							auto data = message->get_data();
							const std::uint32_t pgn = (static_cast<std::uint32_t>(data[5]) | (static_cast<std::uint32_t>(data[6]) << 8) | (static_cast<std::uint32_t>(data[7]) << 16));

							if ((nullptr != message->get_destination_control_function()) &&
								(activeSessions.size() < CANLibConfiguration::get_max_number_transport_protcol_sessions()) &&
								(!get_session(session, message->get_source_control_function(), message->get_destination_control_function(), pgn)))
							{
								TransportProtocolSession *newSession = new TransportProtocolSession(TransportProtocolSession::Direction::Receive);
								newSession->messageLengthBytes = (static_cast<std::uint16_t>(data[1]) | static_cast<std::uint16_t>(data[2] << 8));
								newSession->source = message->get_source_control_function();
								newSession->destination = nullptr;
								newSession->packetCount = data[3];
								newSession->parameterGroupNumber = pgn;
								newSession->sessionData.resize(newSession->messageLengthBytes);
								newSession->state = StateMachineState::RxDataSession;
								newSession->timestamp_ms = SystemTiming::get_timestamp_ms();
								activeSessions.push_back(newSession);
							}
							else if ((get_session(session, message->get_source_control_function(), message->get_destination_control_function(), pgn)) &&
									(nullptr != message->get_destination_control_function()) &&
									(ControlFunction::Type::Internal == message->get_destination_control_function()->get_type()))
							{
								abort_session(pgn, ConnectionAbortReason::AlreadyInCMSession, reinterpret_cast<InternalControlFunction *>(message->get_destination_control_function()), message->get_source_control_function());
							}
							else if ((activeSessions.size() >= CANLibConfiguration::get_max_number_transport_protcol_sessions()) &&
									 (nullptr != message->get_destination_control_function()) &&
									 (ControlFunction::Type::Internal == message->get_destination_control_function()->get_type()))
							{
								abort_session(pgn, ConnectionAbortReason::SystemResourcesNeeded, reinterpret_cast<InternalControlFunction *>(message->get_destination_control_function()), message->get_source_control_function());
							}
                        }
                        break;

                        case CLEAR_TO_SEND_MULTIPLEXOR:
                        {

                        }
                        break;

                        case END_OF_MESSAGE_ACKNOWLEDGE_MULTIPLEXOR:
                        {

                        }
                        break;

                        case CONNECTION_ABORT_MULTIPLEXOR:
                        {
                            
                        }
                        break;

                        default:
                        {
                            // TODO Log bad mux, someone is misbehaving.
                        }
                        break;
                    }
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolData):
				{
					TransportProtocolSession *tempSession = nullptr;

					if ((CAN_DATA_LENGTH == message->get_data_length()) &&
						(get_session(tempSession, message->get_source_control_function(), message->get_destination_control_function(), message->get_identifier().get_parameter_group_number())) &&
						(StateMachineState::RxDataSession == tempSession->state) &&
						(message->get_data()[SEQUENCE_NUMBER_DATA_INDEX] == (tempSession->packetCount + 1)))
					{
						for (std::uint8_t i = SEQUENCE_NUMBER_DATA_INDEX + 1; i < CAN_DATA_LENGTH; i++)
						{
							std::uint16_t currentDataIndex = (8 * tempSession->packetCount + 1) + i;
							tempSession->sessionData[currentDataIndex] = message->get_data()[SEQUENCE_NUMBER_DATA_INDEX + i];
						}
					}
					else
					{
						// TODO Log that the BAM session was bad
						if (get_session(tempSession, message->get_source_control_function(), message->get_destination_control_function(), message->get_identifier().get_parameter_group_number()))
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
		                               std::uint8_t *data,
		                               std::uint32_t messageLength,
		                               ControlFunction *source,
		                               ControlFunction *destination)
	{
		bool retVal = false;

		if ((messageLength < MAX_PROTOCOL_DATA_LENGTH) &&
			(messageLength > 8) &&
			(nullptr != source) &&
			(true == source->get_address_valid()) &&
			((nullptr == destination) || 
			 (destination->get_address_valid())))
		{
			
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
		InternalControlFunction *myControlFunction;
		ControlFunction *partnerControlFunction;
		std::array<std::uint8_t, 8> data;

		if (TransportProtocolSession::Direction::Transmit == session->sessionDirection)
		{
			myControlFunction = CANNetworkManager::CANNetwork.get_internal_control_function(session->source);
			partnerControlFunction = session->destination;
		}
		else
		{
			myControlFunction = CANNetworkManager::CANNetwork.get_internal_control_function(session->destination);
			partnerControlFunction = session->source;
		}

		data[0] = CONNECTION_ABORT_MULTIPLEXOR;
		data[1] = static_cast<std::uint8_t>(reason);
		data[2] = 0xFF;
		data[3] = 0xFF;
		data[4] = 0xFF;
		data[5] = static_cast<std::uint8_t>(session->parameterGroupNumber & 0xFF);
		data[6] = static_cast<std::uint8_t>((session->parameterGroupNumber >> 8) & 0xFF);
		data[7] = static_cast<std::uint8_t>((session->parameterGroupNumber >> 16) & 0xFF);
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolCommand),
		                                                      data.data(),
		                                                      8,
		                                                      myControlFunction,
		                                                      partnerControlFunction,
		                                                      CANIdentifier::CANPriority::PriorityDefault6);
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
			}
		}
	}

	bool TransportProtocolManager::send_end_of_session_acknowledgement(TransportProtocolSession &session)
	{
		std::uint8_t dataBuffer[CAN_DATA_LENGTH] = {0};
		bool retVal = false;

		dataBuffer[0] = END_OF_MESSAGE_ACKNOWLEDGE_MULTIPLEXOR;
		dataBuffer[1] = static_cast<std::uint8_t>(session.messageLengthBytes);
		dataBuffer[2] = static_cast<std::uint8_t>(session.messageLengthBytes >> 8);
		dataBuffer[3] = (static_cast<std::uint8_t>((session.messageLengthBytes - 1) / 7) + 1);
		dataBuffer[4] = 0xFF;
		dataBuffer[5] = static_cast<std::uint8_t>(session.parameterGroupNumber);
		dataBuffer[6] = static_cast<std::uint8_t>(session.parameterGroupNumber >> 8);
		dataBuffer[7] = static_cast<std::uint8_t>(session.parameterGroupNumber >> 16);

		// This message only needs to be sent if we're the recipient. Sanity check the destination is us
		if (ControlFunction::Type::Internal == session.destination->get_type())
		{
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolCommand), 
			dataBuffer, 
			CAN_DATA_LENGTH, 
			reinterpret_cast<InternalControlFunction *>(session.destination),
			session.source,
			CANIdentifier::CANPriority::PriorityDefault6);
		}
		return retVal;
	}

	bool TransportProtocolManager::get_session(TransportProtocolSession *&session, ControlFunction *source, ControlFunction *destination, std::uint32_t parameterGroupNumber)
	{
		session = nullptr;

		for (auto i : activeSessions)
		{
			if ((i->source == source) &&
				(i->destination == destination) &&
				(i->parameterGroupNumber == parameterGroupNumber))
			{
				session = i;
				break;
			}
		}
		return (nullptr != session);
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
				case StateMachineState::WaitForClearToSend:
				case StateMachineState::WaitForEndOfMessageAcknowledge:
				case StateMachineState::TxDataSession:
				{

				}
				break;

				case StateMachineState::RxDataSession:
				{
					if (SystemTiming::time_expired_ms(session->timestamp_ms, MESSAGE_TIMEOUT_MS))
					{
						if (nullptr != session->destination)
						{
							// Not a BAM session, send abort
							abort_session(session, ConnectionAbortReason::Timeout);
						}
						close_session(session);
					}
				}
				break;
			}
		}
	}

}
