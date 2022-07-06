#pragma once

#include "can_lib_protocol.hpp"
#include "can_control_function.hpp"
#include "can_network_manager.hpp"
#include "can_lib_badge.hpp"

namespace isobus
{

class TransportProtocolManager : public CANLibProtocol
{
    enum class StateMachineState
    {
        None,
        ClearToSend,
        RxDataSession,
        WaitForClearToSend,
        TxDataSession,
        WaitForEndOfMessageAcknowledge
    };

    class TransportProtocolSession
    {
    public:
        enum class Direction
        {
            Transmit,
            Receive
        };

        bool operator==(const TransportProtocolSession& obj);

    private:
        friend class TransportProtocolManager;

        TransportProtocolSession(Direction sessionDirection);
        ~TransportProtocolSession();

        StateMachineState state;
        ControlFunction *source;
        ControlFunction *destination;
        std::uint32_t parameterGroupNumber;
        std::uint32_t timestamp_ms;
        std::uint16_t packetCount;
        std::uint16_t messageLengthBytes;
        std::vector<std::uint8_t> sessionData;
        const Direction sessionDirection;
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
    static const std::uint32_t MAX_PROTOCOL_DATA_LENGTH = 1785;
    static const std::uint8_t SEQUENCE_NUMBER_DATA_INDEX = 0;
    static const std::uint8_t MESSAGE_TIMEOUT_MS = 200;

    TransportProtocolManager();
    virtual ~TransportProtocolManager();

    static TransportProtocolManager Protocol;

    void initialize(CANLibBadge<CANNetworkManager>) override;

    void process_message(CANMessage *const message) override;
    static void process_message(CANMessage *const message, void *parent);

    bool protocol_transmit_message(std::uint32_t parameterGroupNumber,
		                               std::uint8_t *data,
		                               std::uint32_t messageLength,
		                               ControlFunction *source,
		                               ControlFunction *destination) override;

    void update(CANLibBadge<CANNetworkManager>) override;

private:
    bool abort_session(TransportProtocolSession *session, ConnectionAbortReason reason);
    bool abort_session(std::uint32_t parameterGroupNumber, ConnectionAbortReason reason, InternalControlFunction *source, ControlFunction *destination);
    void close_session(TransportProtocolSession *session);
    bool send_end_of_session_acknowledgement(TransportProtocolSession &session);
    bool get_session(TransportProtocolSession *&session, ControlFunction *source, ControlFunction *destination, std::uint32_t parameterGroupNumber);
    void update_state_machine(TransportProtocolSession *session);

    static std::vector<TransportProtocolSession *> activeSessions;
};

}