#include "can_transport_protocol.hpp"

#include "can_network_manager.hpp"
#include "can_lib_parameter_group_numbers.hpp"

namespace isobus
{

std::vector<TransportProtocol::TransportProtocolSession *> TransportProtocol::activeSessions;

TransportProtocol::TransportProtocol()
{

}

TransportProtocol::~TransportProtocol()
{
    
}

void TransportProtocol::update()
{

}

bool TransportProtocol::abort_session(TransportProtocolSession &session, ConnectionAbortReason reason)
{
    InternalControlFunction *myControlFunction;
    ControlFunction *partnerControlFunction;
    std::array<std::uint8_t,8> data;

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

bool TransportProtocol::get_session(TransportProtocolSession *&session, ControlFunction *source, ControlFunction *destination, std::uint32_t parameterGroupNumber)
{
    session = nullptr;

    for(auto i : activeSessions)
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

}