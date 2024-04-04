/// @file can_message_handling.hpp
///
/// @brief Defines interfaces for interacting with incoming and outgoing CAN messages.
///
/// @author Daan Steenbergen
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/can_message_handling.hpp"

#include <algorithm>

namespace isobus
{
	bool CANMessagingConsumer::send_can_message(std::uint32_t parameterGroupNumber,
	                                            const std::uint8_t *dataBuffer,
	                                            std::uint32_t dataLength,
	                                            std::shared_ptr<InternalControlFunction> sourceControlFunction,
	                                            std::shared_ptr<ControlFunction> destinationControlFunction,
	                                            CANIdentifier::CANPriority priority,
	                                            TransmitCompleteCallback txCompleteCallback,
	                                            void *parentPointer,
	                                            DataChunkCallback frameChunkCallback) const
	{
		if (nullptr != messagingProvider)
		{
			return messagingProvider->send_can_message(parameterGroupNumber,
			                                           dataBuffer,
			                                           dataLength,
			                                           sourceControlFunction,
			                                           destinationControlFunction,
			                                           priority,
			                                           txCompleteCallback,
			                                           parentPointer,
			                                           frameChunkCallback);
		}
		return false;
	}

	void CANMessageHandler::process_rx_message(const CANMessage &message)
	{
		for (auto it = consumers.begin(); it != consumers.end();)
		{
			const auto consumer = (*it).lock();
			if (nullptr != consumer)
			{
				consumer->process_rx_message(message);
				++it;
			}
			else
			{
				it = consumers.erase(it);
			}
		}
	}

	void CANMessageHandler::process_tx_message(const CANMessage &message)
	{
		for (auto it = consumers.begin(); it != consumers.end();)
		{
			const auto consumer = (*it).lock();
			if (nullptr != consumer)
			{
				consumer->process_tx_message(message);
				++it;
			}
			else
			{
				it = consumers.erase(it);
			}
		}
	}

	void CANMessageHandler::add_consumer(std::shared_ptr<CANMessagingConsumer> consumer)
	{
		// Ensure the consumer is not already in the list
		remove_consumer(consumer);
		consumer->messagingProvider = messagingProvider;
		consumers.push_back(consumer);
	}

	void CANMessageHandler::remove_consumer(std::shared_ptr<CANMessagingConsumer> consumer)
	{
		for (auto it = consumers.begin(); it != consumers.end();)
		{
			const auto consumerPtr = (*it).lock();
			if (consumerPtr == consumer)
			{
				it = consumers.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void CANMessageHandler::set_messaging_provider(std::shared_ptr<CANMessagingProvider> provider)
	{
		messagingProvider = provider;
		for (const auto &consumer : consumers)
		{
			const auto consumerPtr = consumer.lock();
			if (nullptr != consumerPtr)
			{
				consumerPtr->messagingProvider = provider;
			}
		}
	}
}; // namespace isobus
