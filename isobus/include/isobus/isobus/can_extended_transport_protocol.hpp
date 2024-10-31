//================================================================================================
/// @file can_extended_transport_protocol.hpp
///
/// @brief A protocol class that handles the ISO11783 extended transport protocol.
/// Designed for destination specific packets larger than 1785 bytes.
/// @author Adrian Del Grosso
/// @author Daan Steenbergen
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================

#ifndef CAN_EXTENDED_TRANSPORT_PROTOCOL_HPP
#define CAN_EXTENDED_TRANSPORT_PROTOCOL_HPP

#include "isobus/isobus/can_message_frame.hpp"
#include "isobus/isobus/can_network_configuration.hpp"
#include "isobus/isobus/can_transport_protocol_base.hpp"

namespace isobus
{
	/// @brief A class that handles the ISO11783 extended transport protocol.
	/// @details This class handles transmission and reception of CAN messages more than 1785 bytes.
	/// Simply call Simply call `CANNetworkManager::send_can_message()`
	/// with an appropriate data length, and the protocol will be automatically selected to be used.
	class ExtendedTransportProtocolManager
	{
	public:
		/// @brief The states that a ETP session could be in. Used for the internal state machine.
		enum class StateMachineState
		{
			None, ///< Protocol session is not in progress
			SendRequestToSend, ///< We are sending the request to send message
			WaitForClearToSend, ///< We are waiting for a clear to send message
			SendClearToSend, ///< We are sending clear to send message
			WaitForDataPacketOffset, ///< We are waiting for a data packet offset message
			SendDataPacketOffset, ///< We are sending a data packet offset message
			WaitForDataTransferPacket, ///< We are waiting for data transfer packets
			SendDataTransferPackets, ///< A Tx data session is in progress
			WaitForEndOfMessageAcknowledge, ///< We are waiting for an end of message acknowledgement
		};

		/// @brief A list of all defined abort reasons in ISO11783
		enum class ConnectionAbortReason : std::uint8_t
		{
			Reserved = 0, ///< Reserved, not to be used, but should be tolerated
			AlreadyInCMSession = 1, ///< We are already in a connection mode session and can't support another
			SystemResourcesNeeded = 2, ///< Session must be aborted because the system needs resources
			Timeout = 3, ///< General timeout
			ClearToSendReceivedWhileTransferInProgress = 4, ///< A CTS was received while already processing the last CTS
			MaximumRetransmitRequestLimitReached = 5, ///< Maximum retries for the data has been reached
			UnexpectedDataTransferPacketReceived = 6, ///< A data packet was received outside the proper state
			BadSequenceNumber = 7, ///< Incorrect sequence number was received and cannot be recovered
			DuplicateSequenceNumber = 8, ///< Re-received a sequence number we've already processed
			UnexpectedDataPacketOffsetReceived = 9, // Received a data packet offset outside the proper state
			UnexpectedDataPacketOffsetPGN = 10, ///< Received a data packet offset with an unexpected PGN
			DataPacketOffsetExceedsClearToSend = 11, ///< Received a number of packets in EDPO greater than CTS
			BadDataPacketOffset = 12, ///< Received a data packet offset that is incorrect
			UnexpectedClearToSendPGN = 14, ///< Received a CTS with an unexpected PGN
			NumberOfClearToSendPacketsExceedsMessage = 15, ///< Received a CTS with a number of packets greater than the message
			AnyOtherError = 250 ///< Any reason not defined in the standard
		};

		/// @brief A storage object to keep track of session information internally
		class ExtendedTransportProtocolSession : public TransportProtocolSessionBase
		{
		public:
			using TransportProtocolSessionBase::TransportProtocolSessionBase; ///< Inherit the base constructor

			/// @brief Get the state of the session
			/// @return The state of the session
			StateMachineState get_state() const;

			/// @brief Get the number of bytes that have been sent or received in this session
			/// @return The number of bytes that have been sent or received
			std::uint32_t get_total_bytes_transferred() const override;

		protected:
			friend class ExtendedTransportProtocolManager; ///< Allows the ETP manager full access

			/// @brief Set the state of the session
			/// @param[in] value The state to set the session to
			void set_state(StateMachineState value);

			/// @brief Get the number of packets to be sent with the current DPO
			/// @return The number of packets to be sent with the current DPO
			std::uint8_t get_dpo_number_of_packets_remaining() const;

			/// @brief Set the number of packets to be sent with the current DPO
			/// @param[in] value The number of packets to be sent with the current DPO
			void set_dpo_number_of_packets(std::uint8_t value);

			/// @brief Get the number of packets that will be sent with the current DPO
			/// @return The number of packets that will be sent with the current DPO
			std::uint8_t get_dpo_number_of_packets() const;

			/// @brief Get the maximum number of packets that can be sent per DPO as indicated by the CTS message
			/// @return The maximum number of packets that can be sent per DPO as indicated by the CTS message
			std::uint8_t get_cts_number_of_packet_limit() const;

			/// @brief Set the maximum number of packets that can be sent per DPO as indicated by the CTS message
			/// @param value The maximum number of packets that can be sent per DPO as indicated by the CTS message
			void set_cts_number_of_packet_limit(std::uint8_t value);

			/// @brief Get the last sequence number that was processed
			/// @return The last sequence number that was processed
			std::uint8_t get_last_sequence_number() const;

			/// @brief Get the last packet number that was processed
			/// @return The last packet number that was processed
			std::uint32_t get_last_packet_number() const;

			/// @brief Set the last packet number that was processed
			/// @param[in] value The last packet number that was processed
			void set_sequence_number_offset(std::uint32_t value);

			/// @brief Set the last sequence number that has be processed
			/// @param[in] value The last sequence number that has be processed
			void set_last_sequency_number(std::uint8_t value);

			/// @brief Set the last acknowledged packet number by the receiver
			/// @param[in] value The last acknowledged packet number by the receiver
			void set_acknowledged_packet_number(std::uint32_t value);

			/// @brief Get the last acknowledged packet number by the receiver
			/// @return The last acknowledged packet number by the receiver
			std::uint32_t get_last_acknowledged_packet_number() const;

			/// @brief Get the number of packets that remain to be sent or received in this session
			/// @return The number of packets that remain to be sent or received in this session
			std::uint32_t get_number_of_remaining_packets() const;

			/// @brief Get the total number of packets that will be sent or received in this session
			/// @return The total number of packets that will be sent or received in this session
			std::uint32_t get_total_number_of_packets() const;

		private:
			StateMachineState state = StateMachineState::None; ///< The state machine state for this session

			std::uint32_t lastAcknowledgedPacketNumber = 0; ///< The last acknowledged packet number by the receiver
			std::uint32_t sequenceNumberOffset = 0; ///< The offset of the sequence number relative to the packet number
			std::uint8_t lastSequenceNumber = 0; ///< The last processed sequence number for this set of packets
			std::uint8_t dataPacketOffsetPacketCount = 0; ///< The number of packets that will be sent with the current DPO
			std::uint8_t clearToSendPacketCountLimit = 0xFF; ///< The max packets that can be sent per DPO as indicated by the CTS message
		};

		static constexpr std::uint32_t REQUEST_TO_SEND_MULTIPLEXOR = 0x14; ///< (20) ETP.CM_RTS Multiplexor
		static constexpr std::uint32_t CLEAR_TO_SEND_MULTIPLEXOR = 0x15; ///< (21) ETP.CM_CTS Multiplexor
		static constexpr std::uint32_t DATA_PACKET_OFFSET_MULTIPLXOR = 0x16; ///< (22) ETP.CM_DPO Multiplexor
		static constexpr std::uint32_t END_OF_MESSAGE_ACKNOWLEDGE_MULTIPLEXOR = 0x17; ///< (23) TP.CM_EOMA Multiplexor
		static constexpr std::uint32_t CONNECTION_ABORT_MULTIPLEXOR = 0xFF; ///< (255) Abort multiplexor
		static constexpr std::uint32_t MAX_PROTOCOL_DATA_LENGTH = 117440505; ///< The max number of bytes that this protocol can transfer
		static constexpr std::uint16_t T1_TIMEOUT_MS = 750; ///< The t1 timeout as defined by the standard
		static constexpr std::uint16_t T2_T3_TIMEOUT_MS = 1250; ///< The t2/t3 timeouts as defined by the standard
		static constexpr std::uint16_t T4_TIMEOUT_MS = 1050; ///< The t4 timeout as defined by the standard
		static constexpr std::uint8_t TR_TIMEOUT_MS = 200; ///< The Tr Timeout as defined by the standard
		static constexpr std::uint8_t SEQUENCE_NUMBER_DATA_INDEX = 0; ///< The index of the sequence number in a frame
		static constexpr std::uint8_t PROTOCOL_BYTES_PER_FRAME = 7; ///< The number of payload bytes per frame minus overhead of sequence number

		/// @brief The constructor for the ExtendedTransportProtocolManager, for advanced use only.
		/// In most cases, you should use the CANNetworkManager::send_can_message() function to transmit messages.
		/// @param[in] sendCANFrameCallback A callback for sending a CAN frame to hardware
		/// @param[in] canMessageReceivedCallback A callback for when a complete CAN message is received using the ETP protocol
		/// @param[in] configuration The configuration to use for this protocol
		ExtendedTransportProtocolManager(const CANMessageFrameCallback &sendCANFrameCallback,
		                                 const CANMessageCallback &canMessageReceivedCallback,
		                                 const CANNetworkConfiguration *configuration);

		/// @brief Updates all sessions managed by this protocol manager instance.
		void update();

		/// @brief Checks if the source and destination control function have an active session/connection.
		/// @param[in] source The source control function for the session
		/// @param[in] destination The destination control function for the session
		/// @returns true if a matching session was found, false if not
		bool has_session(std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination);

		/// @brief Gets all the active transport protocol sessions that are currently active
		/// @note The list returns pointers to the transport protocol sessions, but they can disappear at any time
		/// @returns A list of all the active transport protocol sessions
		const std::vector<std::shared_ptr<ExtendedTransportProtocolSession>> &get_sessions() const;

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		void process_message(const CANMessage &message);

		/// @brief The network manager calls this to see if the protocol can accept a long CAN message for processing
		/// @param[in] parameterGroupNumber The PGN of the message
		/// @param[in] data The data to be sent
		/// @param[in] source The source control function
		/// @param[in] destination The destination control function
		/// @param[in] sessionCompleteCallback A callback for when the protocol completes its work
		/// @param[in] parentPointer A generic context object for the tx complete and chunk callbacks
		/// @returns true if the message was accepted by the protocol for processing
		bool protocol_transmit_message(std::uint32_t parameterGroupNumber,
		                               std::unique_ptr<CANMessageData> &data,
		                               std::shared_ptr<ControlFunction> source,
		                               std::shared_ptr<ControlFunction> destination,
		                               TransmitCompleteCallback sessionCompleteCallback,
		                               void *parentPointer);

	private:
		/// @brief Aborts the session with the specified abort reason. Sends a CAN message.
		/// @param[in] session The session to abort
		/// @param[in] reason The reason we're aborting the session
		/// @returns true if the abort was send OK, false if not sent
		bool abort_session(const std::shared_ptr<ExtendedTransportProtocolSession> &session, ConnectionAbortReason reason);

		/// @brief Send an abort with no corresponding session with the specified abort reason. Sends a CAN message.
		/// @param[in] sender The sender of the abort
		/// @param[in] receiver The receiver of the abort
		/// @param[in] parameterGroupNumber The PGN of the ETP "session" we're aborting
		/// @param[in] reason The reason we're aborting the session
		/// @returns true if the abort was send OK, false if not sent
		bool send_abort(std::shared_ptr<InternalControlFunction> sender,
		                std::shared_ptr<ControlFunction> receiver,
		                std::uint32_t parameterGroupNumber,
		                ConnectionAbortReason reason) const;

		/// @brief Gracefully closes a session to prepare for a new session
		/// @param[in] session The session to close
		/// @param[in] successful Denotes if the session was successful
		void close_session(const std::shared_ptr<ExtendedTransportProtocolSession> &session, bool successful);

		/// @brief Sends the "request to send" message as part of initiating a transmit
		/// @param[in] session The session for which we're sending the RTS
		/// @returns true if the RTS was sent, false if sending was not successful
		bool send_request_to_send(const std::shared_ptr<ExtendedTransportProtocolSession> &session) const;

		/// @brief Sends the "clear to send" message
		/// @param[in] session The session for which we're sending the CTS
		/// @returns true if the CTS was sent, false if sending was not successful
		bool send_clear_to_send(const std::shared_ptr<ExtendedTransportProtocolSession> &session) const;

		/// @brief Sends the "data packet offset" message for the provided session
		/// @param[in] session The session for which we're sending the DPO
		/// @returns true if the DPO was sent, false if sending was not successful
		bool send_data_packet_offset(const std::shared_ptr<ExtendedTransportProtocolSession> &session) const;

		/// @brief Sends the "end of message acknowledgement" message for the provided session
		/// @param[in] session The session for which we're sending the EOM ACK
		/// @returns true if the EOM was sent, false if sending was not successful
		bool send_end_of_session_acknowledgement(const std::shared_ptr<ExtendedTransportProtocolSession> &session) const;

		///@brief Sends data transfer packets for the specified ExtendedTransportProtocolSession.
		/// @param[in] session The ExtendedTransportProtocolSession for which to send data transfer packets.
		void send_data_transfer_packets(const std::shared_ptr<ExtendedTransportProtocolSession> &session) const;

		/// @brief Processes a request to send a message over the CAN transport protocol.
		/// @param[in] source The shared pointer to the source control function.
		/// @param[in] destination The shared pointer to the destination control function.
		/// @param[in] parameterGroupNumber The Parameter Group Number of the message.
		/// @param[in] totalMessageSize The total size of the message in bytes.
		void process_request_to_send(const std::shared_ptr<ControlFunction> source, const std::shared_ptr<ControlFunction> destination, std::uint32_t parameterGroupNumber, std::uint32_t totalMessageSize);

		/// @brief Processes the Clear To Send (CTS) message.
		/// @param[in] source The shared pointer to the source control function.
		/// @param[in] destination The shared pointer to the destination control function.
		/// @param[in] parameterGroupNumber The Parameter Group Number (PGN) of the message.
		/// @param[in] packetsToBeSent The number of packets to be sent.
		/// @param[in] nextPacketNumber The next packet number.
		void process_clear_to_send(const std::shared_ptr<ControlFunction> source, const std::shared_ptr<ControlFunction> destination, std::uint32_t parameterGroupNumber, std::uint8_t packetsToBeSent, std::uint32_t nextPacketNumber);

		/// @brief Processes the Data Packet Offset (DPO) message.
		/// @param[in] source The shared pointer to the source control function.
		/// @param[in] destination The shared pointer to the destination control function.
		/// @param[in] parameterGroupNumber The Parameter Group Number (PGN) of the message.
		/// @param[in] numberOfPackets The number of packets that will follow.
		/// @param[in] packetOffset The packet offset (always 1 less than CTS next packet number)
		void process_data_packet_offset(const std::shared_ptr<ControlFunction> source, const std::shared_ptr<ControlFunction> destination, std::uint32_t parameterGroupNumber, std::uint8_t numberOfPackets, std::uint32_t packetOffset);

		/// @brief Processes the end of session acknowledgement.
		/// @param[in] source The source control function.
		/// @param[in] destination The destination control function.
		/// @param[in] parameterGroupNumber The parameter group number.
		/// @param[in] numberOfBytesTransferred The number of bytes transferred.
		void process_end_of_session_acknowledgement(const std::shared_ptr<ControlFunction> source, const std::shared_ptr<ControlFunction> destination, std::uint32_t parameterGroupNumber, std::uint32_t numberOfBytesTransferred);

		/// @brief Processes an abort message in the CAN transport protocol.
		/// @param[in] source The shared pointer to the source control function.
		/// @param[in] destination The shared pointer to the destination control function.
		/// @param[in] parameterGroupNumber The PGN (Parameter Group Number) of the message.
		/// @param[in] reason The reason for the connection abort.
		void process_abort(const std::shared_ptr<ControlFunction> source, const std::shared_ptr<ControlFunction> destination, std::uint32_t parameterGroupNumber, ExtendedTransportProtocolManager::ConnectionAbortReason reason);

		/// @brief Processes a connection management message.
		/// @param[in] message The CAN message to be processed.
		void process_connection_management_message(const CANMessage &message);

		/// @brief Processes a data transfer message.
		/// @param[in] message The CAN message to be processed.
		void process_data_transfer_message(const CANMessage &message);

		/// @brief Gets a ETP session from the passed in source and destination and PGN combination
		/// @param[in] source The source control function for the session
		/// @param[in] destination The destination control function for the session
		/// @returns a matching session, or nullptr if no session matched the supplied parameters
		std::shared_ptr<ExtendedTransportProtocolSession> get_session(std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination);

		/// @brief Update the state machine for the passed in session
		/// @param[in] session The session to update
		void update_state_machine(std::shared_ptr<ExtendedTransportProtocolSession> &session);

		std::vector<std::shared_ptr<ExtendedTransportProtocolSession>> activeSessions; ///< A list of all active ETP sessions
		const CANMessageFrameCallback sendCANFrameCallback; ///< A callback for sending a CAN frame
		const CANMessageCallback canMessageReceivedCallback; ///< A callback for when a complete CAN message is received using the ETP protocol
		const CANNetworkConfiguration *configuration; ///< The configuration to use for this protocol
	};

} // namespace isobus

#endif // CAN_EXTENDED_TRANSPORT_PROTOCOL_HPP
