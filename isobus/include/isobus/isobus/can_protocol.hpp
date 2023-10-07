//================================================================================================
/// @file can_protocol.hpp
///
/// @brief A base class for all protocol classes. Allows the network manager to update them
/// in a generic, dynamic way.
/// @author Adrian Del Grosso
///
/// @copyright 2022 Adrian Del Grosso
//================================================================================================

#ifndef CAN_PROTOCOL_HPP
#define CAN_PROTOCOL_HPP

#include "isobus/isobus/can_badge.hpp"
#include "isobus/isobus/can_callbacks.hpp"
#include "isobus/isobus/can_control_function.hpp"
#include "isobus/isobus/can_message.hpp"

#include <vector>

namespace isobus
{
	class CANNetworkManager;

	//================================================================================================
	/// @class CANLibProtocol
	///
	/// @brief A base class for a CAN protocol
	/// @details CANLibProtocols are objects that manage different statful CAN protocols defined by
	/// ISO11783 and/or J1939. They could also be used for abitrary processing inside the CAN stack.
	//================================================================================================
	class CANLibProtocol
	{
	public:
		/// @brief The base class constructor for a CANLibProtocol
		CANLibProtocol();

		/// @brief Deleted copy constructor for a CANLibProtocol
		CANLibProtocol(CANLibProtocol &) = delete;

		/// @brief The base class destructor for a CANLibProtocol
		virtual ~CANLibProtocol();

		/// @brief Returns whether or not the protocol has been initialized by the network manager
		/// @returns true if the protocol has been initialized by the network manager
		bool get_is_initialized() const;

		/// @brief Gets a CAN protocol by index from the list of all protocols
		/// @param[in] index The index of the protocol to get from the list of protocols
		/// @param[out] returnedProtocol The returned protocol
		/// @returns true if a protocol was successfully returned, false if index was out of range
		static bool get_protocol(std::uint32_t index, CANLibProtocol *&returnedProtocol);

		/// @brief Returns the number of all created protocols
		/// @returns The number of all created protocols
		static std::size_t get_number_protocols();

		/// @brief A generic way to initialize a protocol
		/// @details The network manager will call a protocol's initialize function
		/// when it is first updated, if it has yet to be initialized.
		virtual void initialize(CANLibBadge<CANNetworkManager>);

		/// @brief A generic way for a protocol to process a received message
		/// @param[in] message A received CAN message
		virtual void process_message(const CANMessage &message) = 0;

		/// @brief The network manager calls this to see if the protocol can accept a non-raw CAN message for processing
		/// @param[in] parameterGroupNumber The PGN of the message
		/// @param[in] data The data to be sent
		/// @param[in] messageLength The length of the data to be sent
		/// @param[in] source The source control function
		/// @param[in] destination The destination control function
		/// @param[in] transmitCompleteCallback A callback for when the protocol completes its work
		/// @param[in] parentPointer A generic context object for the tx complete and chunk callbacks
		/// @param[in] frameChunkCallback A callback to get some data to send
		/// @returns true if the message was accepted by the protocol for processing
		virtual bool protocol_transmit_message(std::uint32_t parameterGroupNumber,
		                                       const std::uint8_t *data,
		                                       std::uint32_t messageLength,
		                                       std::shared_ptr<ControlFunction> source,
		                                       std::shared_ptr<ControlFunction> destination,
		                                       TransmitCompleteCallback transmitCompleteCallback,
		                                       void *parentPointer,
		                                       DataChunkCallback frameChunkCallback) = 0;

		/// @brief This will be called by the network manager on every cyclic update of the stack
		virtual void update(CANLibBadge<CANNetworkManager>) = 0;

	protected:
		bool initialized; ///< Keeps track of if the protocol has been initialized by the network manager
	};

} // namespace isobus

#endif // CAN_PROTOCOL_HPP
