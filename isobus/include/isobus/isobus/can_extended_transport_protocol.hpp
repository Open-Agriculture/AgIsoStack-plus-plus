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

#include "isobus/isobus/can_badge.hpp"
#include "isobus/isobus/can_control_function.hpp"
#include "isobus/isobus/can_protocol.hpp"

namespace isobus
{
	//================================================================================================
	/// @class ExtendedTransportProtocolManager
	///
	/// @brief A class that handles the ISO11783 extended transport protocol.
	/// @details This class handles transmission and reception of CAN messages more than 1785 bytes.
	/// Simply call send_can_message on the network manager with an appropriate data length,
	/// and the protocol will be automatically selected to be used.
	//================================================================================================
	class ExtendedTransportProtocolManager : public CANLibProtocol
	{
	public:
		///  @brief A list of all defined abort reasons in ISO11783
		enum class ConnectionAbortReason
		{
			Reserved = 0, ///< Reserved, not to be used, but should be tolerated
			AlreadyInConnectionManagedSessionAndCannotSupportAnother = 1, ///< We are already in a session and can't support another
			SystemResourcesNeededForAnotherTask = 2, ///< Session must be aborted because the system needs resources
			Timeout = 3, ///< General timeout
			ClearToSendReceivedWhenDataTransferInProgress = 4, ///< A CTS was received while already processing the last CTS
			MaximumRetransmitRequestLimitReached = 5, ///< Maxmimum retries for the data has been reached
			UnexpectedDataTransferPacket = 6, ///< A data packet was received outside the proper state
			BadSequenceNumber = 7, ///< Incorrect sequence number was received and cannot be recovered
			DuplicateSequenceNumber = 8, ///< Re-received a sequence number we've already processed
			UnexpectedEDPOPacket = 9, ///< EDPO Received in an invalid state
			UnexpectedEDPOPgn = 10, ///< Unexpected PGN in received EDPO
			EDPONumberOfPacketsGreaterThanClearToSend = 11, ///< Number of EDPO packets is > CTS
			BadEDPOOffset = 12, ///< EDPO offset was invalid
			DeprecatedReason = 13, ///< Don't use this. Use AnyOtherReason instead
			UnexpectedECTSPgn = 14, ///< PGN in received ECTS doesn't match session
			ECTSRequestedPacketsExceedsMessageSize = 15, ///< ETP Can't support a message this large (CANMessage::ABSOLUTE_MAX_MESSAGE_LENGTH)
			AnyOtherReason = 254 ///< Any other error not enumerated above, 0xFE
		};

		/// @brief The states that a ETP session could be in. Used for the internal state machine.
		enum class StateMachineState
		{
			None, ///< Protocol session is not in progress
			RequestToSend, ///< We are sending the request to send message
			WaitForClearToSend, ///< We are waiting for a clear to send message
			RxDataSession, ///< We are receiving data packets
			ClearToSend, ///< We are sending clear to send message
			WaitForExtendedDataPacketOffset, ///< We are waiting for an EDPO message
			TxDataSession, ///< We are transmitting EDPOs and data packets
			WaitForEndOfMessageAcknowledge ///< We are waiting for an end of message acknowledgement
		};

		//================================================================================================
		/// @class ExtendedTransportProtocolSession
		///
		/// @brief A storage object to keep track of session information internally
		//================================================================================================
		class ExtendedTransportProtocolSession
		{
		public:
			/// @brief Enumerates the possible session directions, Rx or Tx
			enum class Direction
			{
				Transmit, ///< We are transmitting a message
				Receive ///< We are receving a message
			};

			/// @brief A useful way to compare sesson objects to each other for equality
			bool operator==(const ExtendedTransportProtocolSession &obj);

			/// @brief Get the total number of bytes that will be sent or received in this session
			/// @return The length of the message in number of bytes
			std::uint32_t get_message_data_length() const;

		private:
			friend class ExtendedTransportProtocolManager; ///< Allows the ETP manager full access

			/// @brief The constructor for an ETP session
			/// @param[in] sessionDirection Tx or Rx
			/// @param[in] canPortIndex The CAN channel index for the session
			ExtendedTransportProtocolSession(Direction sessionDirection, std::uint8_t canPortIndex);

			/// @brief The destructor for a ETP session
			~ExtendedTransportProtocolSession();

			StateMachineState state; ///< The state machine state for this session
			CANMessage sessionMessage; ///< A CAN message is used in the session to represent and store data like PGN
			TransmitCompleteCallback sessionCompleteCallback; ///< A callback that is to be called when the session is completed
			DataChunkCallback frameChunkCallback; ///< A callback that might be used to get chunks of data to send
			std::uint32_t frameChunkCallbackMessageLength; ///< The length of the message that is being sent in chunks
			void *parent; ///< A generic context variable that helps identify what object callbacks are destined for. Can be nullptr
			std::uint32_t timestamp_ms; ///< A timestamp used to track session timeouts
			std::uint32_t lastPacketNumber; ///< The last processed sequence number for this set of packets
			std::uint32_t packetCount; ///< The total number of packets to receive or send in this session
			std::uint32_t processedPacketsThisSession; ///< The total processed packet count for the whole session so far
			const Direction sessionDirection; ///< Represents Tx or Rx session
		};

		/// @brief The constructor for the TransportProtocolManager
		ExtendedTransportProtocolManager();

		/// @brief The destructor for the TransportProtocolManager
		~ExtendedTransportProtocolManager() final;

		/// @brief The protocol's initializer function
		void initialize(CANLibBadge<CANNetworkManager>) override;

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		void process_message(const CANMessage &message) override;

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		/// @param[in] parent Provides the context to the actual TP manager object
		static void process_message(const CANMessage &message, void *parent);

		/// @brief The network manager calls this to see if the protocol can accept a long CAN message for processing
		/// @param[in] parameterGroupNumber The PGN of the message
		/// @param[in] data The data to be sent
		/// @param[in] messageLength The length of the data to be sent
		/// @param[in] source The source control function
		/// @param[in] destination The destination control function
		/// @param[in] transmitCompleteCallback A callback for when the protocol completes its work
		/// @param[in] parentPointer A generic context object for the tx complete and chunk callbacks
		/// @param[in] frameChunkCallback A callback to get some data to send
		/// @returns true if the message was accepted by the protocol for processing
		bool protocol_transmit_message(std::uint32_t parameterGroupNumber,
		                               const std::uint8_t *data,
		                               std::uint32_t messageLength,
		                               std::shared_ptr<ControlFunction> source,
		                               std::shared_ptr<ControlFunction> destination,
		                               TransmitCompleteCallback transmitCompleteCallback,
		                               void *parentPointer,
		                               DataChunkCallback frameChunkCallback) override;

		/// @brief Updates the protocol cyclically
		void update(CANLibBadge<CANNetworkManager>) override;

	private:
		static constexpr std::uint32_t MAX_PROTOCOL_DATA_LENGTH = CANMessage::ABSOLUTE_MAX_MESSAGE_LENGTH; ///< The max payload this protocol can support
		static constexpr std::uint32_t MIN_PROTOCOL_DATA_LENGTH = 1786; ///< The min payload this protocol can support
		static constexpr std::uint32_t TR_TIMEOUT_MS = 200; ///< The Tr timeout as defined by the standard
		static constexpr std::uint32_t T1_TIMEOUT_MS = 750; ///< The t1 timeout as defined by the standard
		static constexpr std::uint32_t T2_3_TIMEOUT_MS = 1250; ///< The t2/t3 timeouts as defined by the standard
		static constexpr std::uint32_t TH_TIMEOUT_MS = 500; ///< The Th timout as defined by the standard
		static constexpr std::uint8_t EXTENDED_REQUEST_TO_SEND_MULTIPLEXOR = 0x14; ///< The multiplexor for the extended request to send message
		static constexpr std::uint8_t EXTENDED_CLEAR_TO_SEND_MULTIPLEXOR = 0x15; ///< The multiplexor for the extended clear to send message
		static constexpr std::uint8_t EXTENDED_DATA_PACKET_OFFSET_MULTIPLEXOR = 0x16; ///< The multiplexor for the extended data packet offset message
		static constexpr std::uint8_t EXTENDED_END_OF_MESSAGE_ACKNOWLEDGEMENT = 0x17; ///< Multiplexor for the extended end of message acknowledgement message
		static constexpr std::uint8_t EXTENDED_CONNECTION_ABORT_MULTIPLEXOR = 0xFF; ///< Multiplexor for the extended connection abort message
		static constexpr std::uint8_t PROTOCOL_BYTES_PER_FRAME = 7; ///< The number of payload bytes per frame minus overhead of sequence number
		static constexpr std::uint8_t SEQUENCE_NUMBER_DATA_INDEX = 0; ///< The index of the sequence number in a frame

		/// @brief Aborts the session with the specified abort reason. Sends a CAN message.
		/// @param[in] session The session to abort
		/// @param[in] reason The reason we're aborting the session
		/// @returns true if the abort was send OK, false if not sent
		bool abort_session(ExtendedTransportProtocolSession *session, ConnectionAbortReason reason);

		/// @brief Aborts Tp with no corresponding session with the specified abort reason. Sends a CAN message.
		/// @param[in] parameterGroupNumber The PGN of the ETP "session" we're aborting
		/// @param[in] reason The reason we're aborting the session
		/// @param[in] source The source control function from which we'll send the abort
		/// @param[in] destination The destination control function to which we'll send the abort
		bool abort_session(std::uint32_t parameterGroupNumber, ConnectionAbortReason reason, std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination);

		/// @brief Gracefully closes a session to prepare for a new session
		/// @param[in] session The session to close
		/// @param[in] successfull True if the session was closed successfully, false if not
		void close_session(ExtendedTransportProtocolSession *session, bool successfull);

		/// @brief Gets an ETP session from the passed in source and destination combination
		/// @param[in] source The source control function for the session
		/// @param[in] destination The destination control function for the session
		/// @param[out] session The found session, or nullptr if no session matched the supplied parameters
		bool get_session(ExtendedTransportProtocolSession *&session, std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination) const;

		/// @brief Gets an ETP session from the passed in source and destination and PGN combination
		/// @param[in] source The source control function for the session
		/// @param[in] destination The destination control function for the session
		/// @param[in] parameterGroupNumber The PGN of the session
		/// @param[out] session The found session, or nullptr if no session matched the supplied parameters
		bool get_session(ExtendedTransportProtocolSession *&session, std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination, std::uint32_t parameterGroupNumber) const;

		/// @brief Processes end of session callbacks
		/// @param[in] session The session we've just completed
		/// @param[in] success Denotes if the session was successful
		void process_session_complete_callback(ExtendedTransportProtocolSession *session, bool success);

		/// @brief Sends the "end of message acknowledgement" message for the provided session
		/// @param[in] session The session for which we're sending the EOM ACK
		/// @returns true if the EOM was sent, false if sending was not successful
		bool send_end_of_session_acknowledgement(ExtendedTransportProtocolSession *session) const; // ETP.CM_EOMA

		/// @brief Sends the "clear to send" message
		/// @param[in] session The session for which we're sending the CTS
		/// @returns true if the CTS was sent, false if sending was not successful
		bool send_extended_connection_mode_clear_to_send(ExtendedTransportProtocolSession *session) const; // ETP.CM_CTS

		/// @brief Sends the "request to send" message as part of initiating a transmit
		/// @param[in] session The session for which we're sending the RTS
		/// @returns true if the RTS was sent, false if sending was not successful
		bool send_extended_connection_mode_request_to_send(const ExtendedTransportProtocolSession *session) const; // ETP.CM_RTS

		/// @brief Sends the data packet offset message for the supplied session
		/// @param[in] session The session for which we're sending the EDPO
		/// @returns true if the EDPO was sent, false if sending was not successful
		bool send_extended_connection_mode_data_packet_offset(const ExtendedTransportProtocolSession *session) const; // ETP.CM_DPO

		/// @brief Sets the state machine state of the ETP session
		/// @param[in] session The session to update
		/// @param[in] value The state to update the session to
		void set_state(ExtendedTransportProtocolSession *session, StateMachineState value);

		/// @brief Updates the state machine of a ETP session
		/// @param[in] session The session to update
		void update_state_machine(ExtendedTransportProtocolSession *session);

		std::vector<ExtendedTransportProtocolSession *> activeSessions; ///< A list of all active TP sessions
	};

} // namespace isobus

#endif // CAN_EXTENDED_TRANSPORT_PROTOCOL_HPP
