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

		/// @brief The different byte formats that can be used when reading bytes from the buffer.
		enum class ByteFormat
		{
			LittleEndian,
			BigEndian
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

		/// @brief Get a 8-bit unsigned byte from the buffer at a specific index.
		/// A 8-bit unsigned byte can hold a value between 0 and 255.
		/// @details This function will return the byte at the specified index in the buffer.
		/// @param[in] index The index to get the byte from
		/// @return The 8-bit unsigned byte
		std::uint8_t get_uint8_at(const std::size_t index);

		/// @brief Get a 16-bit unsigned integer from the buffer at a specific index.
		/// A 16-bit unsigned integer can hold a value between 0 and 65535.
		/// @details This function will return the 2 bytes at the specified index in the buffer.
		/// @param[in] index The index to get the 16-bit unsigned integer from
		/// @param[in] format The byte format to use when reading the integer
		/// @return The 16-bit unsigned integer
		std::uint16_t get_uint16_at(const std::size_t index, const ByteFormat format = ByteFormat::LittleEndian);

		/// @brief Get a right-aligned 24-bit integer from the buffer (returned as a uint32_t) at a specific index.
		/// A 24-bit number can hold a value between 0 and 16,777,215.
		/// @details This function will return the 3 bytes at the specified index in the buffer.
		/// @param[in] index The index to get the 24-bit unsigned integer from
		/// @param[in] format The byte format to use when reading the integer
		/// @return The 24-bit unsigned integer, right aligned into a uint32_t
		std::uint32_t get_uint24_at(const std::size_t index, const ByteFormat format = ByteFormat::LittleEndian);

		/// @brief Get a 32-bit unsigned integer from the buffer at a specific index.
		/// A 32-bit unsigned integer can hold a value between 0 and 4294967295.
		/// @details This function will return the 4 bytes at the specified index in the buffer.
		/// @param[in] index The index to get the 32-bit unsigned integer from
		/// @param[in] format The byte format to use when reading the integer
		/// @return The 32-bit unsigned integer
		std::uint32_t get_uint32_at(const std::size_t index, const ByteFormat format = ByteFormat::LittleEndian);

		/// @brief Get a 64-bit unsigned integer from the buffer at a specific index.
		/// A 64-bit unsigned integer can hold a value between 0 and 18446744073709551615.
		/// @details This function will return the 8 bytes at the specified index in the buffer.
		/// @param[in] index The index to get the 64-bit unsigned integer from
		/// @param[in] format The byte format to use when reading the integer
		/// @return The 64-bit unsigned integer
		std::uint64_t get_uint64_at(const std::size_t index, const ByteFormat format = ByteFormat::LittleEndian);

		/// @brief Get a bit-boolean from the buffer at a specific index.
		/// @details This function will return whether the bit(s) at the specified index in the buffer is/are (all) equal to 1.
		/// @param[in] byteIndex The byte index to start reading the boolean from
		/// @param[in] bitIndex The bit index to start reading the boolean from, ranging from 0 to 7
		/// @param[in] length The number of bits to read, maximum of (8 - bitIndex)
		/// @return True if (all) the bit(s) are set, false otherwise
		bool get_bool_at(const std::size_t byteIndex, const std::uint8_t bitIndex, const std::uint8_t length = 1);

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
