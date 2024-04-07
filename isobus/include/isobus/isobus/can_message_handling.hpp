//================================================================================================
/// @file can_message_handling.hpp
///
/// @brief Defines an interface for interacting with incoming and outgoing CAN messages. This is
/// used to abstract the CAN messaging layer from the rest of the application. This allows for
/// easy testing and swapping out of the CAN messaging layer. Furthermore, it ensures that the
/// implementing class is not intertwined with the CAN messaging layer.
///
/// @details The interfaces are more generic than raw CAN messaging, and is designed to be used
/// with J1939 and ISOBUS protocols.
///
///
/// @author Daan Steenbergen
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#ifndef CAN_MESSAGE_HANDLER_HPP
#define CAN_MESSAGE_HANDLER_HPP

#include "isobus/isobus/can_callbacks.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_message.hpp"

namespace isobus
{
	/// @brief An interface that provides a way to send CAN messages to the bus.
	/// @note This is a pure virtual interface, and must be implemented by a concrete class.
	class CANMessagingProvider
	{
	public:
		/// @brief Default destructor
		virtual ~CANMessagingProvider() = default;

		/// @brief This is the main way to send a CAN message of any length.
		/// @details This function will automatically choose an appropriate transport protocol if needed.
		/// If you don't specify a destination (or use nullptr) you message will be sent as a broadcast
		/// if it is valid to do so.
		/// You can also get a callback on success or failure of the transmit.
		/// @param[in] parameterGroupNumber The PGN to use when sending the message
		/// @param[in] dataBuffer A pointer to the data buffer to send from
		/// @param[in] dataLength The size of the message to send
		/// @param[in] sourceControlFunction The control function that is sending the message
		/// @param[in] destinationControlFunction The control function that the message is destined for or nullptr if broadcast
		/// @param[in] priority The CAN priority of the message being sent
		/// @param[in] txCompleteCallback A callback to be called when the message is sent or fails to send
		/// @param[in] parentPointer A generic context variable that helps identify what object the callback is destined for
		/// @param[in] frameChunkCallback A callback which can be supplied to have the tack call you back to get chunks of the message as they are sent
		/// @returns `true` if the message was sent, otherwise `false`
		virtual bool send_can_message(std::uint32_t parameterGroupNumber,
		                              const std::uint8_t *dataBuffer,
		                              std::uint32_t dataLength,
		                              std::shared_ptr<InternalControlFunction> sourceControlFunction,
		                              std::shared_ptr<ControlFunction> destinationControlFunction = nullptr,
		                              CANIdentifier::CANPriority priority = CANIdentifier::CANPriority::PriorityDefault6,
		                              TransmitCompleteCallback txCompleteCallback = nullptr,
		                              void *parentPointer = nullptr,
		                              DataChunkCallback frameChunkCallback = nullptr) = 0;
	};

	/// @brief A class that provides a way to interact with incoming and outgoing CAN messages.
	/// @details This should be extended by a class that wants to interact with incoming and outgoing
	/// CAN messages. It provides a way to process incoming and outgoing messages, and send messages
	/// to the bus.
	class CANMessagingConsumer
	{
	public:
		/// @brief Default destructor
		virtual ~CANMessagingConsumer() = default;

		/// @brief Processes incoming CAN messages
		/// @param message The incoming CAN message to process
		virtual void process_rx_message(const CANMessage &)
		{
			// Override this function in the derived class, if you want to process incoming messages
		}

		/// @brief Processes outgoing CAN messages
		/// @param message The outgoing CAN message to process
		virtual void process_tx_message(const CANMessage &)
		{
			// Override this function in the derived class, if you want to process outgoing messages
		}

	protected:
		/// @brief This is the main way to send a CAN message of any length.
		/// @details This function will automatically choose an appropriate transport protocol if needed.
		/// If you don't specify a destination (or use nullptr) you message will be sent as a broadcast
		/// if it is valid to do so.
		/// You can also get a callback on success or failure of the transmit.
		/// @param[in] parameterGroupNumber The PGN to use when sending the message
		/// @param[in] dataBuffer A pointer to the data buffer to send from
		/// @param[in] dataLength The size of the message to send
		/// @param[in] sourceControlFunction The control function that is sending the message
		/// @param[in] destinationControlFunction The control function that the message is destined for or nullptr if broadcast
		/// @param[in] priority The CAN priority of the message being sent
		/// @param[in] txCompleteCallback A callback to be called when the message is sent or fails to send
		/// @param[in] parentPointer A generic context variable that helps identify what object the callback is destined for
		/// @param[in] frameChunkCallback A callback which can be supplied to have the tack call you back to get chunks of the message as they are sent
		/// @returns `true` if the message was sent, otherwise `false`
		bool send_can_message(std::uint32_t parameterGroupNumber,
		                      const std::uint8_t *dataBuffer,
		                      std::uint32_t dataLength,
		                      std::shared_ptr<InternalControlFunction> sourceControlFunction,
		                      std::shared_ptr<ControlFunction> destinationControlFunction = nullptr,
		                      CANIdentifier::CANPriority priority = CANIdentifier::CANPriority::PriorityDefault6,
		                      TransmitCompleteCallback txCompleteCallback = nullptr,
		                      void *parentPointer = nullptr,
		                      DataChunkCallback frameChunkCallback = nullptr) const;

		friend class CANMessageHandler; ///< Allow the CANMessageHandler to modify the messaging provider
		std::weak_ptr<CANMessagingProvider> messagingProvider; ///< The messaging provider to use for sending messages
	};

	/// @brief A class for managing the routing of incoming and outgoing CAN messages
	class CANMessageHandler : public CANMessagingConsumer
	{
	public:
		/// @brief Processes incoming CAN messages
		/// @param message The incoming CAN message to process
		void process_rx_message(const CANMessage &message) override;

		/// @brief Processes outgoing CAN messages
		/// @param message The outgoing CAN message to process
		void process_tx_message(const CANMessage &message) override;

		/// @brief Adds a consumer to the list of consumers
		/// @param consumer The consumer to add
		void add_consumer(std::shared_ptr<CANMessagingConsumer> consumer);

		/// @brief Removes a consumer from the list of consumers
		/// @param consumer The consumer to remove
		void remove_consumer(std::shared_ptr<CANMessagingConsumer> consumer);

		/// @brief Sets the messaging provider to use for sending messages
		/// @param provider The messaging provider to use for sending messages
		void set_messaging_provider(std::shared_ptr<CANMessagingProvider> provider);

	private:
		std::vector<std::weak_ptr<CANMessagingConsumer>> consumers; ///< The list of consumers to route messages to
	};
} // namespace isobus

#endif // CAN_MESSAGE_HANDLER_HPP
