#include "can_transport_protocol.hpp"

#include "can_lib_parameter_group_numbers.hpp"
#include "can_lib_configuration.hpp"

#include <cassert>

namespace isobus
{

	std::vector<TransportProtocolManager::TransportProtocolSession *> TransportProtocolManager::activeSessions;

	TransportProtocolManager::TransportProtocolSession::TransportProtocolSession(Direction sessionDirection) :
	  state(StateMachineState::None),
	  source(nullptr),
	  destination(nullptr),
	  parameterGroupNumber(0),
	  messageLengthBytes(0),
	  sessionData(nullptr),
	  sessionDirection(sessionDirection)
	{
	}

	TransportProtocolManager::TransportProtocolSession::~TransportProtocolSession()
	{
		if (nullptr != sessionData)
		{
			sessionData->empty();
			delete sessionData;
			sessionData = nullptr;
		}
	}

    bool TransportProtocolManager::TransportProtocolSession::allocate(std::uint16_t size)
    {
        assert(size <= MAX_PROTOCOL_DATA_LENGTH); // Protocol Max should be checked before selecting it.

        if (activeSessions.size() < CANLibConfiguration::get_max_number_transport_protcol_sessions())
        {
            sessionData = new std::vector<std::uint8_t>;
            sessionData->reserve(size);
        }
        else
        {
            // TODO log no sessions can be created
        }
        return (nullptr != sessionData);
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

    void TransportProtocolManager::initialize()
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

                        }
                        break;
                        
                        case REQUEST_TO_SEND_MULTIPLEXOR:
                        {

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

					if (get_session(tempSession, message->get_source_control_function(), message->get_destination_control_function(), message->get_identifier().get_parameter_group_number()))
					{
					}
					else
					{
						// TODO Log no session was found, ECU is behaving badly or bus is flaky
					}
				}
				break;

				default:
				{
                    // This is not a runtime error, should never happen.
                    // Bad PGN passed to protocol. Check PGN registrations.
					assert(false);
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

	void TransportProtocolManager::update_all_sessions(CANLibBadge<CANNetworkManager>)
	{
		for (auto i : activeSessions)
		{
			update_state_machine(i);
		}
	}

	bool TransportProtocolManager::abort_session(TransportProtocolSession &session, ConnectionAbortReason reason)
	{
		InternalControlFunction *myControlFunction;
		ControlFunction *partnerControlFunction;
		std::array<std::uint8_t, 8> data;

		if (TransportProtocolSession::Direction::Transmit == session.sessionDirection)
		{
			myControlFunction = CANNetworkManager::CANNetwork.get_internal_control_function(session.source);
			partnerControlFunction = session.destination;
		}
		else
		{
			myControlFunction = CANNetworkManager::CANNetwork.get_internal_control_function(session.destination);
			partnerControlFunction = session.source;
		}

		data[0] = CONNECTION_ABORT_MULTIPLEXOR;
		data[1] = static_cast<std::uint8_t>(reason);
		data[2] = 0xFF;
		data[3] = 0xFF;
		data[4] = 0xFF;
		data[5] = static_cast<std::uint8_t>(session.parameterGroupNumber & 0xFF);
		data[6] = static_cast<std::uint8_t>((session.parameterGroupNumber >> 8) & 0xFF);
		data[7] = static_cast<std::uint8_t>((session.parameterGroupNumber >> 16) & 0xFF);
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::TransportProtocolCommand),
		                                                      data.data(),
		                                                      8,
		                                                      myControlFunction,
		                                                      partnerControlFunction,
		                                                      CANIdentifier::CANPriority::PriorityDefault6);
	}

	bool TransportProtocolManager::get_session(TransportProtocolSession *&session, ControlFunction *source, ControlFunction *destination, std::uint32_t parameterGroupNumber)
	{
		session = nullptr;

		for (auto i : activeSessions)
		{
			// TODO make an overload for this
			if ((source == i->source) &&
			    (destination == i->destination) &&
			    (parameterGroupNumber == i->parameterGroupNumber))
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
				case StateMachineState::ClearToSend:
				case StateMachineState::RxDataSession:
				case StateMachineState::WaitForClearToSend:
				case StateMachineState::WaitForEndOfMessageAcknowledge:
				case StateMachineState::TxDataSession:
				{

				}
				break;
			}
		}
	}

}