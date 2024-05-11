//================================================================================================
/// @file can_transport_protocol_base.hpp
///
/// @brief Abstract base class for CAN transport protocols.
/// @author Daan Steenbergen
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================

#ifndef CAN_TRANSPORT_PROTOCOL_BASE_HPP
#define CAN_TRANSPORT_PROTOCOL_BASE_HPP

#include "isobus/isobus/can_control_function.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/isobus/can_message_data.hpp"

namespace isobus
{
	/// @brief An object to keep track of session information internally
	class TransportProtocolSessionBase
	{
	public:
		/// @brief Enumerates the possible session directions
		enum class Direction
		{
			Transmit, ///< We are transmitting a message
			Receive ///< We are receiving a message
		};

		/// @brief The constructor for a session
		/// @param[in] direction The direction of the session
		/// @param[in] data Data buffer (will be moved into the session)
		/// @param[in] parameterGroupNumber The PGN of the message
		/// @param[in] totalMessageSize The total size of the message in bytes
		/// @param[in] source The source control function
		/// @param[in] destination The destination control function
		/// @param[in] sessionCompleteCallback A callback for when the session completes
		/// @param[in] parentPointer A generic context object for the tx complete and chunk callbacks
		TransportProtocolSessionBase(TransportProtocolSessionBase::Direction direction,
		                             std::unique_ptr<CANMessageData> data,
		                             std::uint32_t parameterGroupNumber,
		                             std::uint32_t totalMessageSize,
		                             std::shared_ptr<ControlFunction> source,
		                             std::shared_ptr<ControlFunction> destination,
		                             TransmitCompleteCallback sessionCompleteCallback,
		                             void *parentPointer);

		/// @brief The move constructor
		/// @param[in] other The object to move
		TransportProtocolSessionBase(TransportProtocolSessionBase &&other) = default;

		/// @brief The move assignment operator
		/// @param[in] other The object to move
		/// @return A reference to the moved object
		TransportProtocolSessionBase &operator=(TransportProtocolSessionBase &&other) = default;

		/// @brief The destructor for a session
		virtual ~TransportProtocolSessionBase() = default;

		/// @brief Get the direction of the session
		/// @return The direction of the session
		Direction get_direction() const;

		/// @brief A useful way to compare session objects to each other for equality,
		/// @details A session is considered equal when the source and destination control functions
		/// and parameter group number match. Note that we don't compare the super class,
		/// so this should only be used to compare sessions of the same type.
		/// @param[in] obj The object to compare to
		/// @returns true if the objects are equal, false if not
		bool operator==(const TransportProtocolSessionBase &obj) const;

		/// @brief Checks if the source and destination control functions match the given control functions.
		/// @param[in] other_source The control function to compare with the source control function.
		/// @param[in] other_destination The control function to compare with the destination control function.
		/// @returns True if the source and destination control functions match the given control functions, false otherwise.
		bool matches(std::shared_ptr<ControlFunction> other_source, std::shared_ptr<ControlFunction> other_destination) const;

		/// @brief Get the data buffer for the session
		/// @return The data buffer for the session
		CANMessageData &get_data() const;

		/// @brief Get the total number of bytes that will be sent or received in this session
		/// @return The length of the message in number of bytes
		std::uint32_t get_message_length() const;

		/// @brief Get the number of bytes that have been sent or received in this session
		/// @return The number of bytes that have been sent or received
		virtual std::uint32_t get_total_bytes_transferred() const = 0;

		/// @brief Get the percentage of bytes that have been sent or received in this session
		/// @return The percentage of bytes that have been sent or received (between 0 and 100)
		float get_percentage_bytes_transferred() const;

		/// @brief Get the control function that is sending the message
		/// @return The source control function
		std::shared_ptr<ControlFunction> get_source() const;

		/// @brief Get the control function that is receiving the message
		/// @return The destination control function
		std::shared_ptr<ControlFunction> get_destination() const;

		/// @brief Get the parameter group number of the message
		/// @return The PGN of the message
		std::uint32_t get_parameter_group_number() const;

	protected:
		/// @brief Update the timestamp of the session
		void update_timestamp();

		/// @brief Get the time that has passed since the last update of the timestamp
		/// @return The duration in milliseconds
		std::uint32_t get_time_since_last_update() const;

		/// @brief Complete the session
		/// @param[in] success True if the session was successful, false otherwise
		void complete(bool success) const;

	private:
		Direction direction; ///< The direction of the session
		std::uint32_t parameterGroupNumber; ///< The PGN of the message
		std::unique_ptr<CANMessageData> data; ///< The data buffer for the message
		std::shared_ptr<ControlFunction> source; ///< The source control function
		std::shared_ptr<ControlFunction> destination; ///< The destination control function
		std::uint32_t timestamp_ms = 0; ///< A timestamp used to track session timeouts

		std::uint32_t totalMessageSize; ///< The total size of the message in bytes (the maximum size of a message is from ETP and can fit in an uint32_t)

		TransmitCompleteCallback sessionCompleteCallback = nullptr; ///< A callback that is to be called when the session is completed
		void *parent = nullptr; ///< A generic context variable that helps identify what object callbacks are destined for. Can be nullptr
	};
} // namespace isobus

#endif // CAN_TRANSPORT_PROTOCOL_BASE_HPP
