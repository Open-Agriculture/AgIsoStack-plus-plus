//================================================================================================
/// @file nmea2000_message_interface.cpp
///
/// @brief Implements a message interface for processing or sending NMEA2K messages commonly used
/// on an ISO 11783 network.
///
/// @details This interface provides a common interface for sending and receiving common
/// NMEA2000 messages that might be found on an ISO 11783 network. ISO11783-7 defines
/// that GNSS information be sent using NMEA2000 parameter groups like the ones included
/// in this interface.
///
/// @note This library and its authors are not affiliated with the National Marine
/// Electronics Association in any way.
///
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/nmea2000_message_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/isobus/nmea2000_fast_packet_protocol.hpp"
#include "isobus/utility/system_timing.hpp"

#include <algorithm>

namespace isobus
{
	using namespace NMEA2000Messages;

	/// @brief Decodes a message into a copy of its sender's last record and stores the copy in place of that record
	/// @param[in] messages The received messages, one per sender
	/// @param[in] receivedMessagesMutex The mutex that guards the received messages
	/// @param[in] message The message to decode
	/// @param[out] anySignalChanged Set to true if any decoded signal differs from the sender's last record
	/// @returns The stored record, which must be treated as read-only
	template<typename T>
	static std::shared_ptr<T> receive_message(std::vector<std::shared_ptr<T>> &messages, Mutex &receivedMessagesMutex, const CANMessage &message, bool &anySignalChanged)
	{
		(void)receivedMessagesMutex; // Unused when threads are disabled
		const std::shared_ptr<ControlFunction> source = message.get_source_control_function();
		auto isFromSource = [&source](const std::shared_ptr<T> &received) {
			return (nullptr != received) && (received->get_control_function() == source);
		};

		std::shared_ptr<T> previous;
		{
			LOCK_GUARD(Mutex, receivedMessagesMutex);
			auto result = std::find_if(messages.begin(), messages.end(), isFromSource);
			if (result != messages.end())
			{
				previous = *result;
			}
		}

		std::shared_ptr<T> updated;
		bool stored = false;
		while (!stored)
		{
			// Decoding can log, so it runs without the lock
			if (nullptr != previous)
			{
				updated = std::make_shared<T>(*previous);
			}
			else
			{
				updated = std::make_shared<T>(source);
			}
			anySignalChanged = updated->deserialize(message);

			LOCK_GUARD(Mutex, receivedMessagesMutex);
			auto result = std::find_if(messages.begin(), messages.end(), isFromSource);
			std::shared_ptr<T> current;
			if (result != messages.end())
			{
				current = *result;
			}

			if (current == previous)
			{
				if (result != messages.end())
				{
					*result = updated;
				}
				else
				{
					messages.push_back(updated);
				}
				stored = true;
			}
			else
			{
				// update() expired the record while this message was decoded, so decode it again against what is stored now
				previous = current;
			}
		}
		return updated;
	}

	/// @brief Erases the records that have not been updated within the timeout
	/// @param[in] messages The received messages, one per sender
	/// @param[in] timeout_ms How long a record may go without an update
	/// @returns The number of records erased
	template<typename T>
	static std::size_t erase_timed_out_messages(std::vector<std::shared_ptr<T>> &messages, std::uint32_t timeout_ms)
	{
		const std::size_t sizeBefore = messages.size();
		messages.erase(std::remove_if(messages.begin(), messages.end(), [timeout_ms](const std::shared_ptr<T> &message) {
			               return SystemTiming::time_expired_ms(message->get_timestamp(), timeout_ms);
		               }),
		               messages.end());
		return sizeBefore - messages.size();
	}

	NMEA2000MessageInterface::NMEA2000MessageInterface(std::shared_ptr<InternalControlFunction> sendingControlFunction,
	                                                   bool enableSendingCogSogCyclically,
	                                                   bool enableSendingDatumCyclically,
	                                                   bool enableSendingGNSSPositionDataCyclically,
	                                                   bool enableSendingPositionDeltaHighPrecisionRapidUpdateCyclically,
	                                                   bool enableSendingPositionRapidUpdateCyclically,
	                                                   bool enableSendingRateOfTurnCyclically,
	                                                   bool enableSendingVesselHeadingCyclically) :
	  txFlags(static_cast<std::uint32_t>(TransmitFlags::NumberOfFlags), process_flags, this),
	  cogSogTransmitMessage(sendingControlFunction),
	  datumTransmitMessage(sendingControlFunction),
	  gnssPositionDataTransmitMessage(sendingControlFunction),
	  positionDeltaHighPrecisionRapidUpdateTransmitMessage(sendingControlFunction),
	  positionRapidUpdateTransmitMessage(sendingControlFunction),
	  rateOfTurnTransmitMessage(sendingControlFunction),
	  vesselHeadingTransmitMessage(sendingControlFunction),
	  sendCogSogCyclically(enableSendingCogSogCyclically),
	  sendDatumCyclically(enableSendingDatumCyclically),
	  sendGNSSPositionDataCyclically(enableSendingGNSSPositionDataCyclically),
	  sendPositionDeltaHighPrecisionRapidUpdateCyclically(enableSendingPositionDeltaHighPrecisionRapidUpdateCyclically),
	  sendPositionRapidUpdateCyclically(enableSendingPositionRapidUpdateCyclically),
	  sendRateOfTurnCyclically(enableSendingRateOfTurnCyclically),
	  sendVesselHeadingCyclically(enableSendingVesselHeadingCyclically)
	{
	}

	NMEA2000MessageInterface::~NMEA2000MessageInterface()
	{
		terminate();
	}

	NMEA2000Messages::CourseOverGroundSpeedOverGroundRapidUpdate &NMEA2000MessageInterface::get_cog_sog_transmit_message()
	{
		return cogSogTransmitMessage;
	}

	NMEA2000Messages::Datum &NMEA2000MessageInterface::get_datum_transmit_message()
	{
		return datumTransmitMessage;
	}

	NMEA2000Messages::GNSSPositionData &NMEA2000MessageInterface::get_gnss_position_data_transmit_message()
	{
		return gnssPositionDataTransmitMessage;
	}

	NMEA2000Messages::PositionDeltaHighPrecisionRapidUpdate &NMEA2000MessageInterface::get_position_delta_high_precision_rapid_update_transmit_message()
	{
		return positionDeltaHighPrecisionRapidUpdateTransmitMessage;
	}

	NMEA2000Messages::PositionRapidUpdate &NMEA2000MessageInterface::get_position_rapid_update_transmit_message()
	{
		return positionRapidUpdateTransmitMessage;
	}

	NMEA2000Messages::RateOfTurn &NMEA2000MessageInterface::get_rate_of_turn_transmit_message()
	{
		return rateOfTurnTransmitMessage;
	}

	NMEA2000Messages::VesselHeading &NMEA2000MessageInterface::get_vessel_heading_transmit_message()
	{
		return vesselHeadingTransmitMessage;
	}

	std::size_t NMEA2000MessageInterface::get_number_received_course_speed_over_ground_message_sources() const
	{
		LOCK_GUARD(Mutex, receivedMessagesMutex);
		return receivedCogSogMessages.size();
	}

	std::size_t NMEA2000MessageInterface::get_number_received_datum_message_sources() const
	{
		LOCK_GUARD(Mutex, receivedMessagesMutex);
		return receivedDatumMessages.size();
	}

	std::size_t NMEA2000MessageInterface::get_number_received_gnss_position_data_message_sources() const
	{
		LOCK_GUARD(Mutex, receivedMessagesMutex);
		return receivedGNSSPositionDataMessages.size();
	}

	std::size_t NMEA2000MessageInterface::get_number_received_position_delta_high_precision_rapid_update_message_sources() const
	{
		LOCK_GUARD(Mutex, receivedMessagesMutex);
		return receivedPositionDeltaHighPrecisionRapidUpdateMessages.size();
	}

	std::size_t NMEA2000MessageInterface::get_number_received_position_rapid_update_message_sources() const
	{
		LOCK_GUARD(Mutex, receivedMessagesMutex);
		return receivedPositionRapidUpdateMessages.size();
	}

	std::size_t NMEA2000MessageInterface::get_number_received_rate_of_turn_message_sources() const
	{
		LOCK_GUARD(Mutex, receivedMessagesMutex);
		return receivedRateOfTurnMessages.size();
	}

	std::size_t NMEA2000MessageInterface::get_number_received_vessel_heading_message_sources() const
	{
		LOCK_GUARD(Mutex, receivedMessagesMutex);
		return receivedVesselHeadingMessages.size();
	}

	std::shared_ptr<CourseOverGroundSpeedOverGroundRapidUpdate> NMEA2000MessageInterface::get_received_course_speed_over_ground_message(std::size_t index) const
	{
		std::shared_ptr<CourseOverGroundSpeedOverGroundRapidUpdate> retVal = nullptr;

		LOCK_GUARD(Mutex, receivedMessagesMutex);

		if (index < receivedCogSogMessages.size())
		{
			retVal = receivedCogSogMessages.at(index);
		}
		return retVal;
	}

	std::shared_ptr<Datum> NMEA2000MessageInterface::get_received_datum_message(std::size_t index) const
	{
		std::shared_ptr<Datum> retVal = nullptr;

		LOCK_GUARD(Mutex, receivedMessagesMutex);

		if (index < receivedDatumMessages.size())
		{
			retVal = receivedDatumMessages.at(index);
		}
		return retVal;
	}

	std::shared_ptr<GNSSPositionData> NMEA2000MessageInterface::get_received_gnss_position_data_message(std::size_t index) const
	{
		std::shared_ptr<GNSSPositionData> retVal = nullptr;

		LOCK_GUARD(Mutex, receivedMessagesMutex);

		if (index < receivedGNSSPositionDataMessages.size())
		{
			retVal = receivedGNSSPositionDataMessages.at(index);
		}
		return retVal;
	}

	std::shared_ptr<PositionDeltaHighPrecisionRapidUpdate> NMEA2000MessageInterface::get_received_position_delta_high_precision_rapid_update_message(std::size_t index) const
	{
		std::shared_ptr<PositionDeltaHighPrecisionRapidUpdate> retVal = nullptr;

		LOCK_GUARD(Mutex, receivedMessagesMutex);

		if (index < receivedPositionDeltaHighPrecisionRapidUpdateMessages.size())
		{
			retVal = receivedPositionDeltaHighPrecisionRapidUpdateMessages.at(index);
		}
		return retVal;
	}

	std::shared_ptr<PositionRapidUpdate> NMEA2000MessageInterface::get_received_position_rapid_update_message(std::size_t index) const
	{
		std::shared_ptr<PositionRapidUpdate> retVal = nullptr;

		LOCK_GUARD(Mutex, receivedMessagesMutex);

		if (index < receivedPositionRapidUpdateMessages.size())
		{
			retVal = receivedPositionRapidUpdateMessages.at(index);
		}
		return retVal;
	}

	std::shared_ptr<RateOfTurn> NMEA2000MessageInterface::get_received_rate_of_turn_message(std::size_t index) const
	{
		std::shared_ptr<RateOfTurn> retVal = nullptr;

		LOCK_GUARD(Mutex, receivedMessagesMutex);

		if (index < receivedRateOfTurnMessages.size())
		{
			retVal = receivedRateOfTurnMessages.at(index);
		}
		return retVal;
	}

	std::shared_ptr<VesselHeading> NMEA2000MessageInterface::get_received_vessel_heading_message(std::size_t index) const
	{
		std::shared_ptr<VesselHeading> retVal = nullptr;

		LOCK_GUARD(Mutex, receivedMessagesMutex);

		if (index < receivedVesselHeadingMessages.size())
		{
			retVal = receivedVesselHeadingMessages.at(index);
		}
		return retVal;
	}

	EventDispatcher<const std::shared_ptr<NMEA2000Messages::CourseOverGroundSpeedOverGroundRapidUpdate>, bool> &NMEA2000MessageInterface::get_course_speed_over_ground_rapid_update_event_publisher()
	{
		return cogSogEventPublisher;
	}

	EventDispatcher<const std::shared_ptr<NMEA2000Messages::Datum>, bool> &NMEA2000MessageInterface::get_datum_event_publisher()
	{
		return datumEventPublisher;
	}

	EventDispatcher<const std::shared_ptr<NMEA2000Messages::GNSSPositionData>, bool> &NMEA2000MessageInterface::get_gnss_position_data_event_publisher()
	{
		return gnssPositionDataEventPublisher;
	}

	EventDispatcher<const std::shared_ptr<NMEA2000Messages::PositionDeltaHighPrecisionRapidUpdate>, bool> &NMEA2000MessageInterface::get_position_delta_high_precision_rapid_update_event_publisher()
	{
		return positionDeltaHighPrecisionRapidUpdateEventPublisher;
	}

	EventDispatcher<const std::shared_ptr<NMEA2000Messages::PositionRapidUpdate>, bool> &NMEA2000MessageInterface::get_position_rapid_update_event_publisher()
	{
		return positionRapidUpdateEventPublisher;
	}

	EventDispatcher<const std::shared_ptr<NMEA2000Messages::RateOfTurn>, bool> &NMEA2000MessageInterface::get_rate_of_turn_event_publisher()
	{
		return rateOfTurnEventPublisher;
	}

	EventDispatcher<const std::shared_ptr<NMEA2000Messages::VesselHeading>, bool> &NMEA2000MessageInterface::get_vessel_heading_event_publisher()
	{
		return vesselHeadingEventPublisher;
	}

	bool NMEA2000MessageInterface::get_enable_sending_cog_sog_cyclically() const
	{
		return sendCogSogCyclically;
	}

	void NMEA2000MessageInterface::set_enable_sending_cog_sog_cyclically(bool enable)
	{
		sendCogSogCyclically = enable;
	}

	bool NMEA2000MessageInterface::get_enable_sending_datum_cyclically() const
	{
		return sendDatumCyclically;
	}

	void NMEA2000MessageInterface::set_enable_sending_datum_cyclically(bool enable)
	{
		sendDatumCyclically = enable;
	}

	bool NMEA2000MessageInterface::get_enable_sending_gnss_position_data_cyclically() const
	{
		return sendGNSSPositionDataCyclically;
	}

	void NMEA2000MessageInterface::set_enable_sending_gnss_position_data_cyclically(bool enable)
	{
		sendGNSSPositionDataCyclically = enable;
	}

	bool NMEA2000MessageInterface::get_enable_sending_position_delta_high_precision_rapid_update_cyclically() const
	{
		return sendPositionDeltaHighPrecisionRapidUpdateCyclically;
	}

	void NMEA2000MessageInterface::set_enable_sending_position_delta_high_precision_rapid_update_cyclically(bool enable)
	{
		sendPositionDeltaHighPrecisionRapidUpdateCyclically = enable;
	}

	bool NMEA2000MessageInterface::get_enable_sending_position_rapid_update_cyclically() const
	{
		return sendPositionRapidUpdateCyclically;
	}

	void NMEA2000MessageInterface::set_enable_sending_position_rapid_update_cyclically(bool enable)
	{
		sendPositionRapidUpdateCyclically = enable;
	}

	bool NMEA2000MessageInterface::get_enable_sending_rate_of_turn_cyclically() const
	{
		return sendRateOfTurnCyclically;
	}

	void NMEA2000MessageInterface::set_enable_sending_rate_of_turn_cyclically(bool enable)
	{
		sendRateOfTurnCyclically = enable;
	}

	bool NMEA2000MessageInterface::get_enable_sending_vessel_heading_cyclically() const
	{
		return sendVesselHeadingCyclically;
	}

	void NMEA2000MessageInterface::set_enable_sending_vessel_heading_cyclically(bool enable)
	{
		sendVesselHeadingCyclically = enable;
	}

	void NMEA2000MessageInterface::initialize()
	{
		if (!initialized)
		{
			const auto &fastPacketProtocol = CANNetworkManager::CANNetwork.get_fast_packet_protocol(0); // TODO: This should be a configurable can index (will be solved with the new CAN network manager)
			fastPacketProtocol->register_multipacket_message_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::Datum), process_rx_message, this);
			fastPacketProtocol->register_multipacket_message_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::GNSSPositionData), process_rx_message, this);
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::CourseOverGroundSpeedOverGroundRapidUpdate), process_rx_message, this);
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::PositionDeltaHighPrecisionRapidUpdate), process_rx_message, this);
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::PositionRapidUpdate), process_rx_message, this);
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::RateOfTurn), process_rx_message, this);
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VesselHeading), process_rx_message, this);
			initialized = true;
		}
	}

	bool NMEA2000MessageInterface::get_initialized() const
	{
		return initialized;
	}

	void NMEA2000MessageInterface::terminate()
	{
		if (initialized)
		{
			const auto &fastPacketProtocol = CANNetworkManager::CANNetwork.get_fast_packet_protocol(0); // TODO: This should be a configurable can index (will be solved with the new CAN network manager)
			fastPacketProtocol->remove_multipacket_message_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::Datum), process_rx_message, this);
			fastPacketProtocol->remove_multipacket_message_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::GNSSPositionData), process_rx_message, this);
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::CourseOverGroundSpeedOverGroundRapidUpdate), process_rx_message, this);
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::PositionDeltaHighPrecisionRapidUpdate), process_rx_message, this);
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::PositionRapidUpdate), process_rx_message, this);
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::RateOfTurn), process_rx_message, this);
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VesselHeading), process_rx_message, this);
			initialized = false;
		}
	}

	void NMEA2000MessageInterface::update()
	{
		if (initialized)
		{
			check_transmit_timeouts();
			txFlags.process_all_flags();
			check_receive_timeouts();
		}
		else
		{
			LOG_ERROR("[NMEA2K]: Interface not initialized!");
		}
	}

	void NMEA2000MessageInterface::process_flags(std::uint32_t flag, void *parentPointer)
	{
		if ((nullptr != parentPointer) &&
		    (flag < static_cast<std::uint32_t>(TransmitFlags::NumberOfFlags)))
		{
			auto targetInterface = static_cast<NMEA2000MessageInterface *>(parentPointer);
			std::vector<std::uint8_t> messageBuffer;
			bool transmitSuccessful = true;

			switch (static_cast<TransmitFlags>(flag))
			{
				case TransmitFlags::CourseOverGroundSpeedOverGroundRapidUpdate:
				{
					if (nullptr != targetInterface->cogSogTransmitMessage.get_control_function())
					{
						targetInterface->cogSogTransmitMessage.serialize(messageBuffer);
						transmitSuccessful = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::CourseOverGroundSpeedOverGroundRapidUpdate),
						                                                                    messageBuffer.data(),
						                                                                    messageBuffer.size(),
						                                                                    std::static_pointer_cast<InternalControlFunction>(targetInterface->cogSogTransmitMessage.get_control_function()),
						                                                                    nullptr,
						                                                                    CANIdentifier::CANPriority::Priority2);
					}
				}
				break;

				case TransmitFlags::Datum:
				{
					if (nullptr != targetInterface->datumTransmitMessage.get_control_function())
					{
						targetInterface->datumTransmitMessage.serialize(messageBuffer);
						transmitSuccessful = CANNetworkManager::CANNetwork.get_fast_packet_protocol(0)->send_multipacket_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::Datum),
						                                                                                                         messageBuffer.data(),
						                                                                                                         messageBuffer.size(),
						                                                                                                         std::static_pointer_cast<InternalControlFunction>(targetInterface->datumTransmitMessage.get_control_function()),
						                                                                                                         nullptr,
						                                                                                                         CANIdentifier::CANPriority::PriorityDefault6);
					}
				}
				break;

				case TransmitFlags::GNSSPositionData:
				{
					if (nullptr != targetInterface->gnssPositionDataTransmitMessage.get_control_function())
					{
						targetInterface->gnssPositionDataTransmitMessage.serialize(messageBuffer);
						transmitSuccessful = CANNetworkManager::CANNetwork.get_fast_packet_protocol(0)->send_multipacket_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::GNSSPositionData),
						                                                                                                         messageBuffer.data(),
						                                                                                                         messageBuffer.size(),
						                                                                                                         std::static_pointer_cast<InternalControlFunction>(targetInterface->gnssPositionDataTransmitMessage.get_control_function()),
						                                                                                                         nullptr,
						                                                                                                         CANIdentifier::CANPriority::Priority3);
					}
				}
				break;

				case TransmitFlags::PositionDeltaHighPrecisionRapidUpdate:
				{
					if (nullptr != targetInterface->positionDeltaHighPrecisionRapidUpdateTransmitMessage.get_control_function())
					{
						targetInterface->positionDeltaHighPrecisionRapidUpdateTransmitMessage.serialize(messageBuffer);
						transmitSuccessful = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::PositionDeltaHighPrecisionRapidUpdate),
						                                                                    messageBuffer.data(),
						                                                                    messageBuffer.size(),
						                                                                    std::static_pointer_cast<InternalControlFunction>(targetInterface->positionDeltaHighPrecisionRapidUpdateTransmitMessage.get_control_function()),
						                                                                    nullptr,
						                                                                    CANIdentifier::CANPriority::Priority2);
					}
				}
				break;

				case TransmitFlags::PositionRapidUpdate:
				{
					if (nullptr != targetInterface->positionRapidUpdateTransmitMessage.get_control_function())
					{
						targetInterface->positionRapidUpdateTransmitMessage.serialize(messageBuffer);
						transmitSuccessful = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::PositionRapidUpdate),
						                                                                    messageBuffer.data(),
						                                                                    messageBuffer.size(),
						                                                                    std::static_pointer_cast<InternalControlFunction>(targetInterface->positionRapidUpdateTransmitMessage.get_control_function()),
						                                                                    nullptr,
						                                                                    CANIdentifier::CANPriority::Priority2);
					}
				}
				break;

				case TransmitFlags::RateOfTurn:
				{
					if (nullptr != targetInterface->rateOfTurnTransmitMessage.get_control_function())
					{
						targetInterface->rateOfTurnTransmitMessage.serialize(messageBuffer);
						transmitSuccessful = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::RateOfTurn),
						                                                                    messageBuffer.data(),
						                                                                    messageBuffer.size(),
						                                                                    std::static_pointer_cast<InternalControlFunction>(targetInterface->rateOfTurnTransmitMessage.get_control_function()),
						                                                                    nullptr,
						                                                                    CANIdentifier::CANPriority::Priority2);
					}
				}
				break;

				case TransmitFlags::VesselHeading:
				{
					if (nullptr != targetInterface->vesselHeadingTransmitMessage.get_control_function())
					{
						targetInterface->vesselHeadingTransmitMessage.serialize(messageBuffer);
						transmitSuccessful = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::VesselHeading),
						                                                                    messageBuffer.data(),
						                                                                    messageBuffer.size(),
						                                                                    std::static_pointer_cast<InternalControlFunction>(targetInterface->vesselHeadingTransmitMessage.get_control_function()),
						                                                                    nullptr,
						                                                                    CANIdentifier::CANPriority::Priority2);
					}
				}
				break;

				default:
					break;
			}

			if (!transmitSuccessful)
			{
				targetInterface->txFlags.set_flag(static_cast<std::uint32_t>(flag));
			}
		}
	}

	void NMEA2000MessageInterface::process_rx_message(const CANMessage &message, void *parentPointer)
	{
		if (nullptr != parentPointer)
		{
			auto targetInterface = static_cast<NMEA2000MessageInterface *>(parentPointer);

			switch (message.get_identifier().get_parameter_group_number())
			{
				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::CourseOverGroundSpeedOverGroundRapidUpdate):
				{
					if (message.get_source_control_function() != nullptr)
					{
						bool anySignalChanged = false;
						const auto updated = receive_message(targetInterface->receivedCogSogMessages, targetInterface->receivedMessagesMutex, message, anySignalChanged);
						targetInterface->cogSogEventPublisher.call(updated, anySignalChanged);
					}
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::Datum):
				{
					if (message.get_source_control_function() != nullptr)
					{
						bool anySignalChanged = false;
						const auto updated = receive_message(targetInterface->receivedDatumMessages, targetInterface->receivedMessagesMutex, message, anySignalChanged);
						targetInterface->datumEventPublisher.call(updated, anySignalChanged);
					}
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::GNSSPositionData):
				{
					if (message.get_source_control_function() != nullptr)
					{
						bool anySignalChanged = false;
						const auto updated = receive_message(targetInterface->receivedGNSSPositionDataMessages, targetInterface->receivedMessagesMutex, message, anySignalChanged);
						targetInterface->gnssPositionDataEventPublisher.call(updated, anySignalChanged);
					}
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::PositionDeltaHighPrecisionRapidUpdate):
				{
					if (message.get_source_control_function() != nullptr)
					{
						bool anySignalChanged = false;
						const auto updated = receive_message(targetInterface->receivedPositionDeltaHighPrecisionRapidUpdateMessages, targetInterface->receivedMessagesMutex, message, anySignalChanged);
						targetInterface->positionDeltaHighPrecisionRapidUpdateEventPublisher.call(updated, anySignalChanged);
					}
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::PositionRapidUpdate):
				{
					if (message.get_source_control_function() != nullptr)
					{
						bool anySignalChanged = false;
						const auto updated = receive_message(targetInterface->receivedPositionRapidUpdateMessages, targetInterface->receivedMessagesMutex, message, anySignalChanged);
						targetInterface->positionRapidUpdateEventPublisher.call(updated, anySignalChanged);
					}
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::RateOfTurn):
				{
					if (message.get_source_control_function() != nullptr)
					{
						bool anySignalChanged = false;
						const auto updated = receive_message(targetInterface->receivedRateOfTurnMessages, targetInterface->receivedMessagesMutex, message, anySignalChanged);
						targetInterface->rateOfTurnEventPublisher.call(updated, anySignalChanged);
					}
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::VesselHeading):
				{
					if (message.get_source_control_function() != nullptr)
					{
						bool anySignalChanged = false;
						const auto updated = receive_message(targetInterface->receivedVesselHeadingMessages, targetInterface->receivedMessagesMutex, message, anySignalChanged);
						targetInterface->vesselHeadingEventPublisher.call(updated, anySignalChanged);
					}
				}
				break;

				default:
					break;
			}
		}
	}

	void NMEA2000MessageInterface::check_receive_timeouts()
	{
		if (initialized)
		{
			std::size_t cogSogTimeouts = 0;
			std::size_t datumTimeouts = 0;
			std::size_t gnssPositionDataTimeouts = 0;
			std::size_t positionDeltaHighPrecisionRapidUpdateTimeouts = 0;
			std::size_t positionRapidUpdateTimeouts = 0;
			std::size_t rateOfTurnTimeouts = 0;
			std::size_t vesselHeadingTimeouts = 0;

			{
				LOCK_GUARD(Mutex, receivedMessagesMutex);
				cogSogTimeouts = erase_timed_out_messages(receivedCogSogMessages, 3 * CourseOverGroundSpeedOverGroundRapidUpdate::get_timeout());
				datumTimeouts = erase_timed_out_messages(receivedDatumMessages, 3 * Datum::get_timeout());
				gnssPositionDataTimeouts = erase_timed_out_messages(receivedGNSSPositionDataMessages, 3 * GNSSPositionData::get_timeout());
				positionDeltaHighPrecisionRapidUpdateTimeouts = erase_timed_out_messages(receivedPositionDeltaHighPrecisionRapidUpdateMessages, 3 * PositionDeltaHighPrecisionRapidUpdate::get_timeout());
				positionRapidUpdateTimeouts = erase_timed_out_messages(receivedPositionRapidUpdateMessages, 3 * PositionRapidUpdate::get_timeout());
				rateOfTurnTimeouts = erase_timed_out_messages(receivedRateOfTurnMessages, 3 * RateOfTurn::get_timeout());
				vesselHeadingTimeouts = erase_timed_out_messages(receivedVesselHeadingMessages, 3 * VesselHeading::get_timeout());
			}

			// Logged after the lock is released, because a log sink may call back into this interface
			if (0 != cogSogTimeouts)
			{
				LOG_WARNING("[NMEA2K]: COG & SOG message Rx timeout.");
			}
			if (0 != datumTimeouts)
			{
				LOG_WARNING("[NMEA2K]: Datum message Rx timeout.");
			}
			if (0 != gnssPositionDataTimeouts)
			{
				LOG_WARNING("[NMEA2K]: GNSS position data message Rx timeout.");
			}
			if (0 != positionDeltaHighPrecisionRapidUpdateTimeouts)
			{
				LOG_WARNING("[NMEA2K]: Position Delta High Precision Rapid Update Rx timeout.");
			}
			if (0 != positionRapidUpdateTimeouts)
			{
				LOG_WARNING("[NMEA2K]: Position rapid update message Rx timeout.");
			}
			if (0 != rateOfTurnTimeouts)
			{
				LOG_WARNING("[NMEA2K]: Rate of turn message Rx timeout.");
			}
			if (0 != vesselHeadingTimeouts)
			{
				LOG_WARNING("[NMEA2K]: Vessel heading message Rx timeout.");
			}
		}
	}

	void NMEA2000MessageInterface::check_transmit_timeouts()
	{
		if (sendCogSogCyclically && SystemTiming::time_expired_ms(cogSogTransmitMessage.get_timestamp(), CourseOverGroundSpeedOverGroundRapidUpdate::get_timeout()))
		{
			txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::CourseOverGroundSpeedOverGroundRapidUpdate));
			cogSogTransmitMessage.set_timestamp(SystemTiming::get_timestamp_ms());
		}
		if (sendDatumCyclically && SystemTiming::time_expired_ms(datumTransmitMessage.get_timestamp(), Datum::get_timeout()))
		{
			txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::Datum));
			datumTransmitMessage.set_timestamp(SystemTiming::get_timestamp_ms());
		}
		if (sendPositionDeltaHighPrecisionRapidUpdateCyclically && SystemTiming::time_expired_ms(positionDeltaHighPrecisionRapidUpdateTransmitMessage.get_timestamp(), PositionDeltaHighPrecisionRapidUpdate::get_timeout()))
		{
			txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::PositionDeltaHighPrecisionRapidUpdate));
			positionDeltaHighPrecisionRapidUpdateTransmitMessage.set_timestamp(SystemTiming::get_timestamp_ms());
		}
		if (sendGNSSPositionDataCyclically && SystemTiming::time_expired_ms(gnssPositionDataTransmitMessage.get_timestamp(), GNSSPositionData::get_timeout()))
		{
			txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::GNSSPositionData));
			gnssPositionDataTransmitMessage.set_timestamp(SystemTiming::get_timestamp_ms());
		}
		if (sendPositionRapidUpdateCyclically && SystemTiming::time_expired_ms(positionRapidUpdateTransmitMessage.get_timestamp(), PositionRapidUpdate::get_timeout()))
		{
			txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::PositionRapidUpdate));
			positionRapidUpdateTransmitMessage.set_timestamp(SystemTiming::get_timestamp_ms());
		}
		if (sendRateOfTurnCyclically && SystemTiming::time_expired_ms(rateOfTurnTransmitMessage.get_timestamp(), RateOfTurn::get_timeout()))
		{
			txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::RateOfTurn));
			rateOfTurnTransmitMessage.set_timestamp(SystemTiming::get_timestamp_ms());
		}
		if (sendVesselHeadingCyclically && SystemTiming::time_expired_ms(vesselHeadingTransmitMessage.get_timestamp(), VesselHeading::get_timeout()))
		{
			txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::VesselHeading));
			vesselHeadingTransmitMessage.set_timestamp(SystemTiming::get_timestamp_ms());
		}
	}
} // namespace isobus
