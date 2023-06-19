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

#include "isobus/isobus/can_badge.hpp"
#include "isobus/isobus/can_control_function.hpp"
#include "isobus/isobus/can_protocol.hpp"

namespace isobus
{
	//================================================================================================
	/// @class TransportProtocolManager
	///
	/// @brief A class that handles the ISO11783/J1939 transport protocol.
	/// @details This class handles transmission and reception of CAN messages up to 1785 bytes.
	/// Both broadcast and connection mode are supported. Simply call send_can_message on the
	/// network manager with an appropriate data length, and the protocol will be automatically
	/// selected to be used. As a note, use of BAM is discouraged, as it has profound
	/// packet timing implications for your application, and is limited to only 1 active session at a time.
	/// That session could be busy if you are using DM1 or any other BAM protocol, causing intermittant
	/// transmit failures from this class. This is not a bug, rather a limitation of the protocol
	/// definition.
	//================================================================================================
	class TransportProtocolManager : public CANLibProtocol
	{
	public:
		/// @brief The states that a TP session could be in. Used for the internal state machine.
		enum class StateMachineState
		{
			None, ///< Protocol session is not in progress
			ClearToSend, ///< We are sending clear to send message
			RxDataSession, ///< Rx data session is in progress
			RequestToSend, ///< We are sending the request to send message
			WaitForClearToSend, ///< We are waiting for a clear to send message
			BroadcastAnnounce, ///< We are sending the broadcast announce message (BAM)
			TxDataSession, ///< A Tx data session is in progress
			WaitForEndOfMessageAcknowledge ///< We are waiting for an end of message acknowledgement
		};

		//================================================================================================
		/// @class TransportProtocolSession
		///
		/// @brief A storage object to keep track of session information internally
		//================================================================================================
		class TransportProtocolSession
		{
		public:
			/// @brief Enumerates the possible session directions, Rx or Tx
			enum class Direction
			{
				Transmit, ///< We are transmitting a message
				Receive ///< We are receving a message
			};

			/// @brief A useful way to compare sesson objects to each other for equality
			bool operator==(const TransportProtocolSession &obj);

			/// @brief Get the total number of bytes that will be sent or received in this session
			/// @return The length of the message in number of bytes
			std::uint32_t get_message_data_length() const;

		private:
			friend class TransportProtocolManager; ///< Allows the TP manager full access

			/// @brief The constructor for a TP session
			/// @param[in] sessionDirection Tx or Rx
			/// @param[in] canPortIndex The CAN channel index for the session
			TransportProtocolSession(Direction sessionDirection, std::uint8_t canPortIndex);

			/// @brief The destructor for a TP session
			~TransportProtocolSession();

			StateMachineState state; ///< The state machine state for this session
			CANMessage sessionMessage; ///< A CAN message is used in the session to represent and store data like PGN
			TransmitCompleteCallback sessionCompleteCallback; ///< A callback that is to be called when the session is completed
			DataChunkCallback frameChunkCallback; ///< A callback that might be used to get chunks of data to send
			std::uint32_t frameChunkCallbackMessageLength; ///< The length of the message that is being sent in chunks
			void *parent; ///< A generic context variable that helps identify what object callbacks are destined for. Can be nullptr
			std::uint32_t timestamp_ms; ///< A timestamp used to track session timeouts
			std::uint16_t lastPacketNumber; ///< The last processed sequence number for this set of packets
			std::uint8_t packetCount; ///< The total number of packets to receive or send in this session
			std::uint8_t processedPacketsThisSession; ///< The total processed packet count for the whole session so far
			std::uint8_t clearToSendPacketMax; ///< The max packets that can be sent per CTS as indicated by the RTS message
			const Direction sessionDirection; ///< Represents Tx or Rx session
		};

		///  @brief A list of all defined abort reasons in ISO11783
		enum class ConnectionAbortReason : std::uint8_t
		{
			Reserved = 0, ///< Reserved, not to be used, but should be tolerated
			AlreadyInCMSession = 1, ///< We are already in a connection mode session and can't support another
			SystemResourcesNeeded = 2, ///< Session must be aborted because the system needs resources
			Timeout = 3, ///< General timeout
			ClearToSendReceivedWhileTransferInProgress = 4, ///< A CTS was received while already processing the last CTS
			MaximumRetransmitRequestLimitReached = 5, ///< Maxmimum retries for the data has been reached
			UnexpectedDataTransferPacketReceived = 6, ///< A data packet was received outside the proper state
			BadSequenceNumber = 7, ///< Incorrect sequence number was received and cannot be recovered
			DuplicateSequenceNumber = 8, ///< Re-received a sequence number we've already processed
			TotalMessageSizeTooBig = 9, ///< TP Can't support a message this large (>1785 bytes)
			AnyOtherError = 250 ///< Any other error not enumerated above, 0xFE
		};

		static constexpr std::uint32_t REQUEST_TO_SEND_MULTIPLEXOR = 0x10; ///< TP.CM_RTS Multiplexor
		static constexpr std::uint32_t CLEAR_TO_SEND_MULTIPLEXOR = 0x11; ///< TP.CM_CTS Multiplexor
		static constexpr std::uint32_t END_OF_MESSAGE_ACKNOWLEDGE_MULTIPLEXOR = 0x13; ///< TP.CM_EOM_ACK Multiplexor
		static constexpr std::uint32_t BROADCAST_ANNOUNCE_MESSAGE_MULTIPLEXOR = 0x20; ///< TP.BAM Multiplexor
		static constexpr std::uint32_t CONNECTION_ABORT_MULTIPLEXOR = 0xFF; ///< Abort multiplexor
		static constexpr std::uint32_t MAX_PROTOCOL_DATA_LENGTH = 1785; ///< The max number of bytes that this protocol can transfer
		static constexpr std::uint32_t T1_TIMEOUT_MS = 750; ///< The t1 timeout as defined by the standard
		static constexpr std::uint32_t T2_T3_TIMEOUT_MS = 1250; ///< The t2/t3 timeouts as defined by the standard
		static constexpr std::uint32_t T4_TIMEOUT_MS = 1050; ///< The t4 timeout as defined by the standard
		static constexpr std::uint8_t SEQUENCE_NUMBER_DATA_INDEX = 0; ///< The index of the sequence number in a frame
		static constexpr std::uint8_t MESSAGE_TR_TIMEOUT_MS = 200; ///< The Tr Timeout as defined by the standard
		static constexpr std::uint8_t PROTOCOL_BYTES_PER_FRAME = 7; ///< The number of payload bytes per frame minus overhead of sequence number

		/// @brief The constructor for the TransportProtocolManager
		TransportProtocolManager();

		/// @brief The destructor for the TransportProtocolManager
		~TransportProtocolManager() final;

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
		/// @brief Aborts the session with the specified abort reason. Sends a CAN message.
		/// @param[in] session The session to abort
		/// @param[in] reason The reason we're aborting the session
		/// @returns true if the abort was send OK, false if not sent
		bool abort_session(TransportProtocolSession *session, ConnectionAbortReason reason);

		/// @brief Aborts Tp with no corresponding session with the specified abort reason. Sends a CAN message.
		/// @param[in] parameterGroupNumber The PGN of the TP "session" we're aborting
		/// @param[in] reason The reason we're aborting the session
		/// @param[in] source The source control function from which we'll send the abort
		/// @param[in] destination The destination control function to which we'll send the abort
		/// @returns true if the abort was send OK, false if not sent
		bool abort_session(std::uint32_t parameterGroupNumber, ConnectionAbortReason reason, std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination);

		/// @brief Gracefully closes a session to prepare for a new session
		/// @param[in] session The session to close
		/// @param[in] successfull Denotes if the session was successful
		void close_session(TransportProtocolSession *session, bool successfull);

		/// @brief Processes end of session callbacks
		/// @param[in] session The session we've just completed
		/// @param[in] success Denotes if the session was successful
		void process_session_complete_callback(TransportProtocolSession *session, bool success);

		/// @brief Sends the "broadcast announce" message
		/// @param[in] session The session for which we're sending the BAM
		/// @returns true if the BAM was sent, false if sending was not successful
		bool send_broadcast_announce_message(TransportProtocolSession *session) const;

		/// @brief Sends the "clear to send" message
		/// @param[in] session The session for which we're sending the CTS
		/// @returns true if the CTS was sent, false if sending was not successful
		bool send_clear_to_send(TransportProtocolSession *session) const;

		/// @brief Sends the "request to send" message as part of initiating a transmit
		/// @param[in] session The session for which we're sending the RTS
		/// @returns true if the RTS was sent, false if sending was not successful
		bool send_request_to_send(TransportProtocolSession *session) const;

		/// @brief Sends the "end of message acknowledgement" message for the provided session
		/// @param[in] session The session for which we're sending the EOM ACK
		/// @returns true if the EOM was sent, false if sending was not successful
		bool send_end_of_session_acknowledgement(TransportProtocolSession *session) const;

		/// @brief Sets the state machine state of the TP session
		/// @param[in] session The session to update
		/// @param[in] value The state to update the session to
		void set_state(TransportProtocolSession *session, StateMachineState value);

		/// @brief Gets a TP session from the passed in source and destination combination
		/// @param[in] source The source control function for the session
		/// @param[in] destination The destination control function for the session
		/// @param[out] session The found session, or nullptr if no session matched the supplied parameters
		bool get_session(TransportProtocolSession *&session, std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination);

		/// @brief Gets a TP session from the passed in source and destination and PGN combination
		/// @param[in] source The source control function for the session
		/// @param[in] destination The destination control function for the session
		/// @param[in] parameterGroupNumber The PGN of the session
		/// @param[out] session The found session, or nullptr if no session matched the supplied parameters
		bool get_session(TransportProtocolSession *&session, std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination, std::uint32_t parameterGroupNumber);

		/// @brief Updates the state machine of a Tp session
		/// @param[in] session The session to update
		void update_state_machine(TransportProtocolSession *session);

		std::vector<TransportProtocolSession *> activeSessions; ///< A list of all active TP sessions
	};

} // namespace isobus

#endif // CAN_TRANSPORT_PROTOCOL_HPP
