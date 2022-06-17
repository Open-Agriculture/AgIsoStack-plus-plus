#pragma once

#include "can_lib_protocol.hpp"
#include "can_control_function.hpp"

namespace isobus
{

class TransportProtocol : public CANLibProtocol
{
    class TransportProtocolStateMachine
    {
    public:
        enum class State
        {
            None,
            ClearToSend,
            RxDataSession,
            WaitForClearToSend,
            TxDataSession,
            WaitForEndOfMessageAcknowledge
        };

        State get_state() const;

    private:
        State currentState;
    };

    class TransportProtocolSession
    {
    public:
        enum class Direction
        {
            Transmit,
            Receive
        };
    private:
        friend class TransportProtocol;
        ControlFunction *source;
        ControlFunction *destination;
        std::uint32_t parameterGroupNumber;
        std::uint16_t messageLengthBytes;
        std::uint8_t *dataBuffer;
        Direction sessionDirection;
    };

    enum class ConnectionAbortReason : std::uint8_t
    {
        Reserved = 0,
        AlreadyInCMSession = 1,
        SystemResourcesNeeded = 2,
        Timeout = 3,
        ClearToSendReceivedWhileTransferInProgress = 4,
        MaximumRetransmitRequestLimitReached = 5,
        UnexpedtedDataTransferPacketReceived = 6,
        BadSequenceNumber = 7,
        DuplicateSequenceNumber = 8,
        TotalMessageSizeTooBig = 9,
        AnyOtherError = 250 // 0xFE
    };

    static const std::uint32_t REQUEST_TO_SEND_MULTIPLEXOR = 0x10;
    static const std::uint32_t CLEAR_TO_SEND_MULTIPLEXOR = 0x11;
    static const std::uint32_t END_OF_MESSAGE_ACKNOWLEDGE_MULTIPLEXOR = 0x13;
    static const std::uint32_t BROADCAST_ANNOUNCE_MESSAGE_MULTIPLEXOR = 0x20;
    static const std::uint32_t CONNECTION_ABORT_MULTIPLEXOR = 0xFF;

    TransportProtocol();
    virtual ~TransportProtocol();

protected:
    virtual void update();

private:
    bool abort_session(TransportProtocolSession &session, ConnectionAbortReason reason);
    bool get_session(TransportProtocolSession *&session, ControlFunction *source, ControlFunction *destination, std::uint32_t parameterGroupNumber);

    static std::vector<TransportProtocolSession *> activeSessions;

    bool anySessionNeedsUpdate;
};

}