//================================================================================================
/// @file can_managed_message.hpp
///
/// @brief A CAN message that allows setter access to private  data, to be used by the library
/// itself internally under some circumstances.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_MANAGED_MESSAGE_HPP
#define CAN_MANAGED_MESSAGE_HPP

#include "isobus/isobus/can_message.hpp"

namespace isobus
{
	//================================================================================================
	/// @class CANLibManagedMessage
	///
	/// @brief A derived CAN message class that the stack can use to manipulate CAN message data
	/// in order to construct a message to send or track message details in a protocol class.
	//================================================================================================
	class CANLibManagedMessage : public CANMessage
	{
	public:
		/// @brief Constructor for the CANLibManagedMessage
		/// @param[in] CANPort The can channel index the message uses
		CANLibManagedMessage(std::uint8_t CANPort);

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

		/// @brief Gets the size of the data payload
		/// @returns The length of the data payload
		std::uint32_t get_data_length() const override;

		/// @brief Sets the source control function for the message
		/// @param[in] value The source control function
		void set_source_control_function(ControlFunction *value);

		/// @brief Sets the destination control function for the message
		/// @param[in] value The destination control function
		void set_destination_control_function(ControlFunction *value);

		/// @brief Sets the CAN ID of the message
		/// @param[in] value The CAN ID for the message
		void set_identifier(CANIdentifier value);

		/// @brief Gets the size of the message when using callbacks and not the internal data vector
		std::uint32_t get_callback_message_size() const;

	private:
		std::uint32_t callbackMessageSize; ///< The size of the message when using callbacks and not the internal data vector
	};

} // namespace isobus

#endif // CAN_MANAGED_MESSAGE_HPP
