//================================================================================================
/// @file can_transport_protocol_sniffer.hpp
///
/// @brief A passive observer that reassembles ISO11783/J1939 transport protocol messages which
/// are sent between two external control functions.
/// @author Sujan Dumaru
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================

#ifndef CAN_TRANSPORT_PROTOCOL_SNIFFER_HPP
#define CAN_TRANSPORT_PROTOCOL_SNIFFER_HPP

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <vector>

#include "isobus/isobus/can_callbacks.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/isobus/can_network_configuration.hpp"

namespace isobus
{
	/// @brief Reassembles transport protocol messages exchanged between two other control functions.
	/// @details The regular TransportProtocolManager cannot be made to follow somebody else's
	/// connection, because reassembling a connection mode message also means replying with clear to
	/// send, end of message acknowledgement and abort, and sending any of those for a session we are
	/// not part of would corrupt that transfer. This class keeps its own sessions instead, has no way
	/// to transmit anything at all, and silently drops any session it loses track of. Broadcast (BAM)
	/// sessions are not handled here, as the regular manager already delivers those to sniffers.
	/// @note Data transfer frames carry no PGN, so a session is only as trustworthy as the request to
	/// send that opened it. A request to send we never observe leaves a stale session behind until the
	/// protocol timeout expires, which is inherent to watching a connection rather than taking part.
	class TransportProtocolSniffer
	{
	public:
		/// @brief A callback used to ask whether a message with the given PGN is worth reassembling
		using ParameterGroupNumberFilter = std::function<bool(std::uint32_t parameterGroupNumber)>;

		/// @brief The constructor for the TransportProtocolSniffer
		/// @param[in] parameterGroupNumberFilter A callback that returns true for PGNs that should be reassembled
		/// @param[in] canMessageAssembledCallback A callback for when an observed message has been fully reassembled
		/// @param[in] configuration The configuration to use for this observer
		TransportProtocolSniffer(const ParameterGroupNumberFilter &parameterGroupNumberFilter,
		                         const CANMessageCallback &canMessageAssembledCallback,
		                         const CANNetworkConfiguration *configuration);

		/// @brief Drops every observed session that has gone quiet for longer than the protocol allows
		void update();

		/// @brief Observes a received CAN message, and reassembles it if it belongs to a session of interest
		/// @param[in] message A received CAN message
		void process_message(const CANMessage &message);

	private:
		/// @brief Storage for one observed transport protocol connection
		struct SnifferSession
		{
			std::uint32_t parameterGroupNumber = 0; ///< The PGN of the message being reassembled
			std::shared_ptr<ControlFunction> source; ///< The control function sending the message
			std::shared_ptr<ControlFunction> destination; ///< The control function the message is addressed to
			std::vector<std::uint8_t> data; ///< The reassembled payload, sized from the request to send
			std::vector<bool> receivedPackets; ///< Marks which packets have been observed, so retransmissions are harmless
			std::uint16_t remainingPacketCount = 0; ///< The number of packets we have yet to observe
			std::uint32_t timestamp_ms = 0; ///< When we last saw any frame belonging to this session
		};

		/// @brief Processes an observed connection management message
		/// @param[in] message The observed CAN message
		void process_connection_management_message(const CANMessage &message);

		/// @brief Processes an observed data transfer message
		/// @param[in] message The observed CAN message
		void process_data_transfer_message(const CANMessage &message);

		/// @brief Stops following an observed session, if it exists and carries the expected PGN
		/// @param[in] session An iterator to the session to drop, which may be the end iterator
		/// @param[in] parameterGroupNumber The PGN named by the frame that closed the connection
		void close_session(std::list<SnifferSession>::iterator session, std::uint32_t parameterGroupNumber);

		/// @brief Starts following an observed connection, if we want it and have room for it
		/// @param[in] message The observed request to send message
		/// @param[in] parameterGroupNumber The PGN that the connection will carry
		/// @param[in] totalMessageSize The total size of the message in bytes, as claimed by the sender
		/// @param[in] totalNumberOfPackets The total number of packets, as claimed by the sender
		void open_session(const CANMessage &message,
		                  std::uint32_t parameterGroupNumber,
		                  std::uint16_t totalMessageSize,
		                  std::uint8_t totalNumberOfPackets);

		/// @brief Finds the observed session for the given source and destination combination
		/// @param[in] source The source control function for the session
		/// @param[in] destination The destination control function for the session
		/// @returns An iterator to the matching session, or the end iterator if there is no match
		std::list<SnifferSession>::iterator get_session(const std::shared_ptr<ControlFunction> &source,
		                                                const std::shared_ptr<ControlFunction> &destination);

		std::list<SnifferSession> sessions; ///< Every connection we are currently following
		const ParameterGroupNumberFilter parameterGroupNumberFilter; ///< Tells us which PGNs are worth reassembling
		const CANMessageCallback canMessageAssembledCallback; ///< Called with each fully reassembled message
		const CANNetworkConfiguration *configuration; ///< The configuration to use for this observer
	};

} // namespace isobus

#endif // CAN_TRANSPORT_PROTOCOL_SNIFFER_HPP
