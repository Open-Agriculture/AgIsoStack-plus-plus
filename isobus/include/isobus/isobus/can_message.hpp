//================================================================================================
/// @file can_message.hpp
///
/// @brief An abstraction  of a CAN message, could be > 8 data bytes.
/// @author Adrian Del Grosso
/// @author Daan Steenbergen
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

		/// @brief ISO11783-3 defines this: The maximum number of packets that can be sent in a single connection
		/// with extended transport protocol is restricted by the extended data packet offset (3 bytes).
		/// This yields a maximum message size of (2^24-1 packets) x (7 bytes/packet) = 117440505 bytes
		/// @returns The maximum length of any CAN message as defined by ETP in ISO11783
		static const std::uint32_t ABSOLUTE_MAX_MESSAGE_LENGTH = 117440505;

		/// @brief Constructor for a CAN message
		/// @param[in] CANPort The can channel index the message uses
		explicit CANMessage(std::uint8_t CANPort);

		/// @brief Destructor for a CAN message
		virtual ~CANMessage() = default;

		/// @brief Returns the CAN message type
		/// @returns The type of the CAN message
		Type get_type() const;

		/// @brief Gets a reference to the data in the CAN message
		/// @returns A reference to the data in the CAN message
		const std::vector<std::uint8_t> &get_data() const;

		/// @brief Returns the length of the data in the CAN message
		/// @returns The message data payload length
		virtual std::uint32_t get_data_length() const;

		/// @brief Gets the source control function that the message is from
		/// @returns The source control function that the message is from
		std::shared_ptr<ControlFunction> get_source_control_function() const;

		/// @brief Gets the destination control function that the message is to
		/// @returns The destination control function that the message is to
		std::shared_ptr<ControlFunction> get_destination_control_function() const;

		/// @brief Returns the identifier of the message
		/// @returns The identifier of the message
		CANIdentifier get_identifier() const;

		/// @brief Returns the CAN channel index associated with the message
		/// @returns The CAN channel index associated with the message
		std::uint8_t get_can_port_index() const;

		/// @brief Sets the message data to the value supplied. Creates a copy.
		/// @param[in] dataBuffer The data payload
		/// @param[in] length the length of the data payload in bytes
		void set_data(const std::uint8_t *dataBuffer, std::uint32_t length);

		/// @brief Sets one byte of data in the message data payload
		/// @param[in] dataByte One byte of data
		/// @param[in] insertPosition The position in the message at which to insert the data byte
		void set_data(std::uint8_t dataByte, const std::uint32_t insertPosition);

		/// @brief Sets the size of the data payload
		/// @param[in] length The desired length of the data payload
		void set_data_size(std::uint32_t length);

		/// @brief Sets the source control function for the message
		/// @param[in] value The source control function
		void set_source_control_function(std::shared_ptr<ControlFunction> value);

		/// @brief Sets the destination control function for the message
		/// @param[in] value The destination control function
		void set_destination_control_function(std::shared_ptr<ControlFunction> value);

		/// @brief Sets the CAN ID of the message
		/// @param[in] value The CAN ID for the message
		void set_identifier(CANIdentifier value);

		/// @brief Get a 8-bit unsigned byte from the buffer at a specific index.
		/// A 8-bit unsigned byte can hold a value between 0 and 255.
		/// @details This function will return the byte at the specified index in the buffer.
		/// @param[in] index The index to get the byte from
		/// @return The 8-bit unsigned byte
		std::uint8_t get_uint8_at(const std::uint32_t index) const;

		/// @brief Get a 16-bit unsigned integer from the buffer at a specific index.
		/// A 16-bit unsigned integer can hold a value between 0 and 65535.
		/// @details This function will return the 2 bytes at the specified index in the buffer.
		/// @param[in] index The index to get the 16-bit unsigned integer from
		/// @param[in] format The byte format to use when reading the integer
		/// @return The 16-bit unsigned integer
		std::uint16_t get_uint16_at(const std::uint32_t index, const ByteFormat format = ByteFormat::LittleEndian) const;

		/// @brief Get a right-aligned 24-bit integer from the buffer (returned as a uint32_t) at a specific index.
		/// A 24-bit number can hold a value between 0 and 16,777,215.
		/// @details This function will return the 3 bytes at the specified index in the buffer.
		/// @param[in] index The index to get the 24-bit unsigned integer from
		/// @param[in] format The byte format to use when reading the integer
		/// @return The 24-bit unsigned integer, right aligned into a uint32_t
		std::uint32_t get_uint24_at(const std::uint32_t index, const ByteFormat format = ByteFormat::LittleEndian) const;

		/// @brief Get a 32-bit unsigned integer from the buffer at a specific index.
		/// A 32-bit unsigned integer can hold a value between 0 and 4294967295.
		/// @details This function will return the 4 bytes at the specified index in the buffer.
		/// @param[in] index The index to get the 32-bit unsigned integer from
		/// @param[in] format The byte format to use when reading the integer
		/// @return The 32-bit unsigned integer
		std::uint32_t get_uint32_at(const std::uint32_t index, const ByteFormat format = ByteFormat::LittleEndian) const;

		/// @brief Get a 64-bit unsigned integer from the buffer at a specific index.
		/// A 64-bit unsigned integer can hold a value between 0 and 18446744073709551615.
		/// @details This function will return the 8 bytes at the specified index in the buffer.
		/// @param[in] index The index to get the 64-bit unsigned integer from
		/// @param[in] format The byte format to use when reading the integer
		/// @return The 64-bit unsigned integer
		std::uint64_t get_uint64_at(const std::uint32_t index, const ByteFormat format = ByteFormat::LittleEndian) const;

		/// @brief Get a bit-boolean from the buffer at a specific index.
		/// @details This function will return whether the bit(s) at the specified index in the buffer is/are (all) equal to 1.
		/// @param[in] byteIndex The byte index to start reading the boolean from
		/// @param[in] bitIndex The bit index to start reading the boolean from, ranging from 0 to 7
		/// @param[in] length The number of bits to read, maximum of (8 - bitIndex)
		/// @return True if (all) the bit(s) are set, false otherwise
		bool get_bool_at(const std::uint32_t byteIndex, const std::uint8_t bitIndex, const std::uint8_t length = 1) const;

	private:
		Type messageType = Type::Receive; ///< The internal message type associated with the message
		CANIdentifier identifier = CANIdentifier(0); ///< The CAN ID of the message
		std::vector<std::uint8_t> data; ///< A data buffer for the message, used when not using data chunk callbacks
		std::shared_ptr<ControlFunction> source = nullptr; ///< The source control function of the message
		std::shared_ptr<ControlFunction> destination = nullptr; ///< The destination control function of the message
		const std::uint8_t CANPortIndex; ///< The CAN channel index associated with the message
	};

} // namespace isobus

#endif // CAN_MESSAGE_HPP
