//================================================================================================
/// @file nmea2000_fast_packet_protocol.hpp
///
/// @brief A protocol that handles the NMEA 2000 (IEC 61162-3) fast packet protocol.
///
/// @details This higher layer protocol is used primarily on boats and ships to connect
/// equipment such as GPS, auto pilots, depth sounders, navigation instruments, engines, etc.
/// The Fast Packet protocol provides a means to stream up to 223 bytes of data, with the
/// advantage that each frame retains the parameter group number and priority.
/// The first frame transmitted uses 2 bytes to identify sequential Fast Packet parameter groups
/// and sequential frames within a single parameter group transmission.
/// The first byte contains a sequence counter to distinguish consecutive transmission
/// of the same parameter groups and a frame counter set to frame zero.
/// The second byte in the first frame identifies the total size of the
/// parameter group to follow. Successive frames use just single data byte for the
/// sequence counter and the frame counter.
///
/// @note This library and its authors are not affiliated with the National Marine
/// Electronics Association in any way.
///
/// @author Adrian Del Grosso
/// @author Daan Steenbergen
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================

#ifndef NMEA2000_FAST_PACKET_PROTOCOL_HPP
#define NMEA2000_FAST_PACKET_PROTOCOL_HPP

#include "isobus/isobus/can_transport_protocol_base.hpp"
#include "isobus/utility/event_dispatcher.hpp"
#include "isobus/utility/thread_synchronization.hpp"

namespace isobus
{
	/// @brief A protocol that handles the NMEA 2000 fast packet protocol.
	class FastPacketProtocol
	{
	public:
		/// @brief An object for tracking fast packet session state
		class FastPacketProtocolSession : public TransportProtocolSessionBase
		{
		public:
			/// @brief The constructor for a session, for advanced use only.
			/// In most cases, you should use the CANNetworkManager::get_fast_packet_protocol().send_message() function to transmit messages.
			/// @param[in] direction The direction of the session
			/// @param[in] data Data buffer (will be moved into the session)
			/// @param[in] parameterGroupNumber The PGN of the message
			/// @param[in] totalMessageSize The total size of the message in bytes
			/// @param[in] sequenceNumber The sequence number for this PGN
			/// @param[in] priority The priority to encode in the IDs of the component CAN messages
			/// @param[in] source The source control function
			/// @param[in] destination The destination control function
			/// @param[in] sessionCompleteCallback A callback for when the session completes
			/// @param[in] parentPointer A generic context object for the tx complete and chunk callbacks
			FastPacketProtocolSession(TransportProtocolSessionBase::Direction direction,
			                          std::unique_ptr<CANMessageData> data,
			                          std::uint32_t parameterGroupNumber,
			                          std::uint16_t totalMessageSize,
			                          std::uint8_t sequenceNumber,
			                          CANIdentifier::CANPriority priority,
			                          std::shared_ptr<ControlFunction> source,
			                          std::shared_ptr<ControlFunction> destination,
			                          TransmitCompleteCallback sessionCompleteCallback,
			                          void *parentPointer);

			/// @brief Get the total number of bytes that will be sent or received in this session
			/// @note The maximum number of bytes that can be sent in a single session is 6 + 31 * 7 = 223
			/// @return The length of the message in number of bytes
			std::uint8_t get_message_length() const;

			/// @brief Get the number of bytes that have been sent or received in this session
			/// @return The number of bytes that have been sent or received
			std::uint32_t get_total_bytes_transferred() const override;

			/// @brief Get whether or not this session is a broadcast session (BAM)
			/// @return True if this session is a broadcast session, false if not
			bool is_broadcast() const;

		protected:
			friend class FastPacketProtocol; ///< Allows the TP manager full access

			/// @brief Get the last packet number that was sent or received in this session
			/// @return The last packet number that was sent or received in this session
			std::uint8_t get_last_packet_number() const;

			/// @brief Get the number of packets that remain to be sent or received in this session
			/// @return The number of packets that remain to be sent or received in this session
			std::uint8_t get_number_of_remaining_packets() const;

			/// @brief Get the total number of packets that will be sent or received in this session
			/// @return The total number of packets that will be sent or received in this session
			std::uint8_t get_total_number_of_packets() const;

			/// @brief Add number of bytes to the total number of bytes that have been sent or received in this session
			/// @param[in] bytes The number of bytes to add to the total
			void add_number_of_bytes_transferred(std::uint8_t bytes);

		private:
			std::uint8_t numberOfBytesTransferred = 0; ///< The total number of bytes that have been processed in this session
			std::uint8_t sequenceNumber; ///< The sequence number for this PGN
			CANIdentifier::CANPriority priority; ///< The priority to encode in the IDs of the component CAN messages
		};

		/// @brief A structure for keeping track of past sessions so we can resume with the right session number
		struct FastPacketHistory
		{
			NAME isoName; ///< The ISO name of the internal control function used in a session
			std::uint32_t parameterGroupNumber; ///< The PGN of the session being saved
			std::uint8_t sequenceNumber; ///< The sequence number to use in the next matching session
		};

		/// @brief The constructor for the FastPacketProtocol, for advanced use only.
		/// In most cases, you should use the CANNetworkManager::get_fast_packet_protocol().send_message() function to transmit messages.
		/// @param[in] sendCANFrameCallback A callback for sending a CAN frame to hardware
		explicit FastPacketProtocol(const CANMessageFrameCallback &sendCANFrameCallback);

		/// @brief Add a callback to be called when a message is received by the Fast Packet protocol
		/// @param[in] parameterGroupNumber The PGN to parse as fast packet
		/// @param[in] callback The callback that the stack will call when a matching message is received
		/// @param[in] parent Generic context variable for the callback
		/// @param[in] internalControlFunction An internal control function to use as an additional filter for the callback.
		/// Only messages destined for the specified ICF will generate a callback. Use nullptr to receive messages for
		/// destined for any ICF and broadcast messages.
		/// @note You can also sniff all messages by allowing messages for messages destined to non-internal control functions
		/// to be parsed by this protocol, use the "allow_any_control_function()" function to enable this.
		void register_multipacket_message_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent, std::shared_ptr<InternalControlFunction> internalControlFunction = nullptr);

		// @brief Removes a callback previously added with register_multipacket_message_callback
		/// @param[in] parameterGroupNumber The PGN to parse as fast packet
		/// @param[in] callback The callback that the stack will call when a matching message is received
		/// @param[in] parent Generic context variable
		/// @param[in] internalControlFunction An internal control function used as an additional filter for the callback
		void remove_multipacket_message_callback(std::uint32_t parameterGroupNumber, CANLibCallback callback, void *parent, std::shared_ptr<InternalControlFunction> internalControlFunction = nullptr);

		/// @brief Used to send CAN messages using fast packet
		/// @details You have to use this function instead of the network manager
		/// because otherwise the CAN stack has no way of knowing to send your message
		/// with FP instead of TP.
		/// @param[in] parameterGroupNumber The PGN of the message
		/// @param[in] data The data to be sent
		/// @param[in] messageLength The length of the data to be sent
		/// @param[in] source The source control function
		/// @param[in] destination The destination control function
		/// @param[in] priority The priority to encode in the IDs of the component CAN messages
		/// @param[in] txCompleteCallback A callback for when the protocol completes its work
		/// @param[in] parentPointer A generic context object for the tx complete and chunk callbacks
		/// @param[in] frameChunkCallback A callback to get some data to send
		/// @returns true if the message was accepted by the protocol for processing
		bool send_multipacket_message(std::uint32_t parameterGroupNumber,
		                              const std::uint8_t *data,
		                              std::uint8_t messageLength,
		                              std::shared_ptr<InternalControlFunction> source,
		                              std::shared_ptr<ControlFunction> destination,
		                              CANIdentifier::CANPriority priority = CANIdentifier::CANPriority::PriorityDefault6,
		                              TransmitCompleteCallback txCompleteCallback = nullptr,
		                              void *parentPointer = nullptr,
		                              DataChunkCallback frameChunkCallback = nullptr);

		/// @brief Set whether or not to allow messages for non-internal control functions to be parsed by this protocol
		/// @param[in] allow Denotes if messages for non-internal control functions should be parsed by this protocol
		void allow_any_control_function(bool allow);

		/// @brief Updates all sessions managed by this protocol manager instance.
		void update();

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		void process_message(const CANMessage &message);

		/// @brief Calculates the number of frames needed for a message
		/// @param[in] messageLength The length of the message in bytes
		/// @returns The number of frames needed for the message
		static std::uint8_t calculate_number_of_frames(std::uint8_t messageLength);

	private:
		/// @brief Adds a session's info to the history so that we can continue the sequence number later
		/// @param[in] session The session to add to the history
		void add_session_history(const std::shared_ptr<FastPacketProtocolSession> &session);

		/// @brief Gracefully closes a session to prepare for a new session
		/// @param[in] session The session to close
		/// @param[in] successful Denotes if the session was successful
		void close_session(std::shared_ptr<FastPacketProtocolSession> session, bool successful);

		/// @brief Get the sequence number to use for a new session based on the history of past sessions
		/// @param[in] name The ISO name of the internal control function used in a session
		/// @param[in] parameterGroupNumber The PGN of the session being started
		/// @returns The sequence number to use for the new session
		std::uint8_t get_new_sequence_number(NAME name, std::uint32_t parameterGroupNumber) const;

		/// @brief Gets a FP session from the passed in source and destination and PGN combination
		/// @param[in] parameterGroupNumber The PGN of the session
		/// @param[in] source The source control function for the session
		/// @param[in] destination The destination control function for the session
		/// @returns a matching session, or nullptr if no session matched the supplied parameters
		std::shared_ptr<FastPacketProtocolSession> get_session(std::uint32_t parameterGroupNumber, std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination);

		/// @brief Checks if a session by the passed in source and destination and PGN combination exists
		/// @param[in] parameterGroupNumber The PGN of the session
		/// @param[in] source The source control function for the session
		/// @param[in] destination The destination control function for the session
		/// @returns true if a matching session exists, false if not
		bool has_session(std::uint32_t parameterGroupNumber, std::shared_ptr<ControlFunction> source, std::shared_ptr<ControlFunction> destination);

		/// @brief Update a single session
		/// @param[in] session The session to update
		void update_session(const std::shared_ptr<FastPacketProtocolSession> &session);

		static constexpr std::uint32_t FP_MIN_PARAMETER_GROUP_NUMBER = 0x1F000; ///< Start of PGNs that can be received via Fast Packet
		static constexpr std::uint32_t FP_MAX_PARAMETER_GROUP_NUMBER = 0x1FFFF; ///< End of PGNs that can be received via Fast Packet
		static constexpr std::uint32_t FP_TIMEOUT_MS = 750; ///< Protocol timeout in milliseconds
		static constexpr std::uint8_t MAX_PROTOCOL_MESSAGE_LENGTH = 223; ///< Max message length based on there being 5 bits of sequence data
		static constexpr std::uint8_t FRAME_COUNTER_BIT_MASK = 0x1F; ///< Bit mask for masking out the frame counter
		static constexpr std::uint8_t SEQUENCE_NUMBER_BIT_MASK = 0x07; ///< Bit mask for masking out the sequence number bits
		static constexpr std::uint8_t SEQUENCE_NUMBER_BIT_OFFSET = 5; ///< The bit offset into the first byte of data to get the seq number
		static constexpr std::uint8_t PROTOCOL_BYTES_PER_FRAME = 7; ///< The number of payload bytes per frame for all but the first message, which has 6

		std::vector<std::shared_ptr<FastPacketProtocolSession>> activeSessions; ///< A list of all active TP sessions
		Mutex sessionMutex; ///< A mutex to lock the sessions list in case someone starts a Tx while the stack is processing sessions
		std::vector<FastPacketHistory> sessionHistory; ///< Used to keep track of sequence numbers for future sessions
		std::vector<ParameterGroupNumberCallbackData> parameterGroupNumberCallbacks; ///< A list of all parameter group number callbacks that will be parsed as fast packet messages
		bool allowAnyControlFunction = false; ///< Denotes if messages for non-internal control functions should be parsed by this protocol
		const CANMessageFrameCallback sendCANFrameCallback; ///< A callback for sending a CAN frame
	};

} // namespace isobus

#endif // NMEA2000_FAST_PACKET_PROTOCOL_HPP
