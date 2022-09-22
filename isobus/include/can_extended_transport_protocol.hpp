//================================================================================================
/// @file can_extended_transport_protocol.hpp
///
/// @brief A protocol class that handles the ISO11783 extended transport protocol.
/// Designed for packets larger than 1785 bytes.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_EXTENDED_TRANSPORT_PROTOCOL_HPP
#define CAN_EXTENDED_TRANSPORT_PROTOCOL_HPP

#include "can_protocol.hpp"
#include "can_control_function.hpp"
#include "can_network_manager.hpp"
#include "can_badge.hpp"
#include "can_managed_message.hpp"

namespace isobus
{

	class ExtendedTransportProtocolManager : public CANLibProtocol
	{
	public:
		enum class ConnectionAbortReason
		{
			Reserved = 0,
			AlreadyInConnectionManagedSessionAndCannotSupportAnother = 1,
			SystemResourcesNeededForAnotherTask = 2,
			Timeout = 3,
			ClearToSendReceivedWhenDataTransferInProgress = 4,
			MaximumRetransmitRequestLimitReached = 5,
			UnexpectedDataTransferPacket = 6,
			BadSequenceNumber = 7,
			DuplicateSequenceNumber = 8,
			UnexpectedEDPOPacket = 9,
			UnexpectedEDPOPgn = 10,
			EDPONumberOfPacketsGreaterThanClearToSend = 11,
			BadEDPOOffset = 12,
			DeprecatedReason = 13,
			UnexpectedECTSPgn = 14,
			ECTSRequestedPacketsExceedsMessageSize = 15,
			AnyOtherReason = 250
		};

		enum class StateMachineState
		{
			None,
			RequestToSend,
			WaitForClearToSend,
			RxDataSession,
			ClearToSend,
			TxDataSession,
			WaitForEndOfMessageAcknowledge
		};

		class ExtendedTransportProtocolSession
		{
		public:
			enum class Direction
			{
				Transmit,
				Receive
			};

			bool operator==(const ExtendedTransportProtocolSession &obj);

		private:
			friend class ExtendedTransportProtocolManager;

			ExtendedTransportProtocolSession(Direction sessionDirection, std::uint8_t canPortIndex);
			~ExtendedTransportProtocolSession();

			StateMachineState state;
			CANLibManagedMessage sessionMessage;
			TransmitCompleteCallback sessionCompleteCallback;
			DataChunkCallback frameChunkCallback;
			void *parent;
			std::uint32_t timestamp_ms;
			std::uint32_t lastPacketNumber;
			std::uint32_t packetCount;
			std::uint32_t processedPacketsThisSession; // For the whole session
			const Direction sessionDirection;
		};

		ExtendedTransportProtocolManager();
		virtual ~ExtendedTransportProtocolManager();

		static ExtendedTransportProtocolManager Protocol;

		void initialize(CANLibBadge<CANNetworkManager>) override;

		void process_message(CANMessage *const message) override;
		static void process_message(CANMessage *const message, void *parent);

		bool protocol_transmit_message(std::uint32_t parameterGroupNumber,
		                               const std::uint8_t *data,
		                               std::uint32_t messageLength,
		                               ControlFunction *source,
		                               ControlFunction *destination,
		                               TransmitCompleteCallback transmitCompleteCallback,
		                               void *parentPointer,
		                               DataChunkCallback frameChunkCallback) override;

		void update(CANLibBadge<CANNetworkManager>) override;

	private:
		static constexpr std::uint32_t MAX_PROTOCOL_DATA_LENGTH = CANMessage::ABSOLUTE_MAX_MESSAGE_LENGTH;
		static constexpr std::uint8_t EXTENDED_REQUEST_TO_SEND_MULTIPLEXOR = 0x14;
		static constexpr std::uint8_t EXTENDED_CLEAR_TO_SEND_MULTIPLEXOR = 0x15;
		static constexpr std::uint8_t EXTENDED_DATA_PACKET_OFFSET_MULTIPLEXOR = 0x16;
		static constexpr std::uint8_t EXTENDED_END_OF_MESSAGE_ACKNOWLEDGEMENT = 0x17;
		static constexpr std::uint8_t EXTENDED_CONNECTION_ABORT_MULTIPLEXOR = 0xFF;
		static constexpr std::uint8_t PROTOCOL_BYTES_PER_FRAME = 7;

		bool abort_session(ExtendedTransportProtocolSession *session, ConnectionAbortReason reason);
		bool abort_session(std::uint32_t parameterGroupNumber, ConnectionAbortReason reason, InternalControlFunction *source, ControlFunction *destination);
		void close_session(ExtendedTransportProtocolSession *session);
		bool get_session(ExtendedTransportProtocolSession *&session, ControlFunction *source, ControlFunction *destination);
		bool get_session(ExtendedTransportProtocolSession *&session, ControlFunction *source, ControlFunction *destination, std::uint32_t parameterGroupNumber);
		bool send_end_of_session_acknowledgement(ExtendedTransportProtocolSession *session); // ETP.CM_EOMA
		bool send_extended_connection_mode_request_to_send(const ExtendedTransportProtocolSession *session); // ETP.CM_RTS
		bool send_extended_connection_mode_data_packet_offset(const ExtendedTransportProtocolSession *session); // ETP.CM_DPO
		void set_state(ExtendedTransportProtocolSession *session, StateMachineState value);

		std::vector<ExtendedTransportProtocolSession *> activeSessions;
	};

} // namespace isobus

#endif // CAN_EXTENDED_TRANSPORT_PROTOCOL_HPP
