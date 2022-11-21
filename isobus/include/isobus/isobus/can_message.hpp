//================================================================================================
/// @file can_message.hpp
///
/// @brief An abstraction  of a CAN message, could be > 8 data bytes.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_MESSAGE_HPP
#define CAN_MESSAGE_HPP

#include "isobus/isobus/can_control_function.hpp"
#include "isobus/isobus/can_identifier.hpp"

#include <vector>

namespace isobus
{
	//================================================================================================
	/// @class CANMessage
	///
	/// @brief A class that represents a generic CAN message of arbitrary length.
	//================================================================================================
	class CANMessage
	{
	public:
		/// @brief The internal message type
		enum class Type
		{
			Transmit, ///< Message is to be transmitted from the stack
			Receive, ///< Message is being received
			Internal ///< Message is being used internally as data storage for a protocol
		};

		/// @brief Constructor for a CAN message
		/// @param[in] CANPort The can channel index the message uses
		CANMessage(std::uint8_t CANPort);

		/// @brief Returns the CAN message type
		/// @returns The type of the CAN message
		Type get_type() const;

		/// @brief Gets a reference to the data in the CAN message
		/// @returns A reference to the data in the CAN message
		std::vector<std::uint8_t> &get_data();

		/// @brief Returns the length of the data in the CAN message
		/// @returns The message data payload length
		virtual std::uint32_t get_data_length() const;

		/// @brief Gets the source control function that the message is from
		/// @returns The source control function that the message is from
		ControlFunction *get_source_control_function() const;

		/// @brief Gets the destination control function that the message is to
		/// @returns The destination control function that the message is to
		ControlFunction *get_destination_control_function() const;

		/// @brief Returns the identifier of the message
		/// @returns The identifier of the message
		CANIdentifier get_identifier() const;

		/// @brief Returns the unique message ID
		/// @returns The unique message ID
		std::uint32_t get_message_unique_id() const;

		/// @brief Returns the CAN channel index associated with the message
		/// @returns The CAN channel index associated with the message
		std::uint8_t get_can_port_index() const;

		/// @brief ISO11783-3 defines this: The maximum number of packets that can be sent in a single connection
		/// with extended transport protocol is restricted by the extended data packet offset (3 bytes).
		/// This yields a maximum message size of (2^24-1 packets) x (7 bytes/packet) = 117440505 bytes
		/// @returns The maximum length of any CAN message as defined by ETP in ISO11783
		static const std::uint32_t ABSOLUTE_MAX_MESSAGE_LENGTH = 117440505;

	protected:
		std::vector<std::uint8_t> data; ///< A data buffer for the message, used when not using data chunk callbacks
		ControlFunction *source; ///< The source control function of the message
		ControlFunction *destination; ///< The destination control function of the message
		CANIdentifier identifier; ///< The CAN ID of the message
		Type messageType; ///< The internal message type associated with the message
		const std::uint32_t messageUniqueID; ///< The unique ID of the message, an internal value for tracking and stats
		const std::uint8_t CANPortIndex; ///< The CAN channel index associated with the message

	private:
		static std::uint32_t lastGeneratedUniqueID; ///< A unique, sequential ID for this CAN message
	};

} // namespace isobus

#endif // CAN_MESSAGE_HPP
