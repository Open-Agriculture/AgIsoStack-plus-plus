//================================================================================================
/// @file can_transport_protocol.hpp
///
/// @brief A protocol that handles the ISO11783/J1939 transport protocol.
/// It handles both the broadcast version (BAM) and and the connection mode version.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_TRANSPORT_PROTOCOL_HPP
#define CAN_TRANSPORT_PROTOCOL_HPP

#include "can_protocol.hpp"
#include "can_control_function.hpp"
#include "can_network_manager.hpp"
#include "can_badge.hpp"
#include "can_managed_message.hpp"

namespace isobus
{

class TransportProtocolManager : public CANLibProtocol
{
    enum class StateMachineState
    {
        None,
        ClearToSend,
        RxDataSession,
        RequestToSend,
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

        TransportProtocolSession(Direction sessionDirection, std::uint8_t canPortIndex);
        ~TransportProtocolSession();

        StateMachineState state;
        CANLibManagedMessage sessionMessage;
        std::uint32_t timestamp_ms;
        std::uint16_t lastPacketNumber;
        std::uint8_t packetCount;
	    std::uint8_t processedPacketsThisSession; // For the whole session
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

    static constexpr std::uint32_t REQUEST_TO_SEND_MULTIPLEXOR = 0x10;
    static constexpr std::uint32_t CLEAR_TO_SEND_MULTIPLEXOR = 0x11;
    static constexpr std::uint32_t END_OF_MESSAGE_ACKNOWLEDGE_MULTIPLEXOR = 0x13;
    static constexpr std::uint32_t BROADCAST_ANNOUNCE_MESSAGE_MULTIPLEXOR = 0x20;
    static constexpr std::uint32_t CONNECTION_ABORT_MULTIPLEXOR = 0xFF;
    static constexpr std::uint32_t MAX_PROTOCOL_DATA_LENGTH = 1785;
    static constexpr std::uint32_t T1_TIMEOUT_MS = 750;
    static constexpr std::uint32_t T2_T3_TIMEOUT_MS = 1250;
    static constexpr std::uint32_t T4_TIMEOUT_MS = 1050;
    static constexpr std::uint8_t SEQUENCE_NUMBER_DATA_INDEX = 0;
    static constexpr std::uint8_t MESSAGE_TR_TIMEOUT_MS = 200;
    static constexpr std::uint8_t PROTOCOL_BYTES_PER_FRAME = 7;

    TransportProtocolManager();
    virtual ~TransportProtocolManager();

    static TransportProtocolManager Protocol;

    void initialize(CANLibBadge<CANNetworkManager>) override;

    void process_message(CANMessage *const message) override;
    static void process_message(CANMessage *const message, void *parent);

    bool protocol_transmit_message(std::uint32_t parameterGroupNumber,
		                               const std::uint8_t *data,
		                               std::uint32_t messageLength,
		                               ControlFunction *source,
		                               ControlFunction *destination) override;

    void update(CANLibBadge<CANNetworkManager>) override;

private:
    bool abort_session(TransportProtocolSession *session, ConnectionAbortReason reason);
    bool abort_session(std::uint32_t parameterGroupNumber, ConnectionAbortReason reason, InternalControlFunction *source, ControlFunction *destination);
    void close_session(TransportProtocolSession *session);
    bool send_request_to_send(TransportProtocolSession *session);
    bool send_end_of_session_acknowledgement(TransportProtocolSession *session);
    void set_state(TransportProtocolSession *session, StateMachineState value);
    bool get_session(TransportProtocolSession *&session, ControlFunction *source, ControlFunction *destination);
    bool get_session(TransportProtocolSession *&session, ControlFunction *source, ControlFunction *destination, std::uint32_t parameterGroupNumber);
    void update_state_machine(TransportProtocolSession *session);

    std::vector<TransportProtocolSession *> activeSessions;
};

} // namespace isobus

#endif // CAN_TRANSPORT_PROTOCOL_HPP
