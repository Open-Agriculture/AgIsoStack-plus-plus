//================================================================================================
/// @file isobus_heartbeat.cpp
///
/// @brief Implements an interface for sending and receiving ISOBUS heartbeats.
/// The heartbeat message is used to determine the integrity of the communication of messages and
/// parameters being transmitted by a control function. There may be multiple instances of the
/// heartbeat message on the network, and CFs are required transmit the message on request.
/// As long as the heartbeat message is transmitted at the regular
/// time interval and the sequence number increases through the valid range, then the
/// heartbeat message indicates that the data source CF is operational and provides
/// correct data in all its messages
///
/// @author Adrian Del Grosso
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/isobus_heartbeat.hpp"

#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"

namespace isobus
{
	HeartbeatInterface::HeartbeatInterface(const CANMessageFrameCallback &sendCANFrameCallback) :
	  sendCANFrameCallback(sendCANFrameCallback)
	{
	}

	void HeartbeatInterface::set_enabled(bool enable)
	{
		if ((!enable) && (enable != enabled))
		{
			LOG_DEBUG("[HB]: Disabling ISOBUS heartbeat interface.");
		}
		enabled = enable;
	}

	bool HeartbeatInterface::is_enabled() const
	{
		return enabled;
	}

	bool HeartbeatInterface::request_heartbeat(std::shared_ptr<InternalControlFunction> sourceControlFunction,
	                                           std::shared_ptr<ControlFunction> destinationControlFunction) const
	{
		bool retVal = false;

		if ((nullptr != sourceControlFunction) &&
		    (nullptr != destinationControlFunction) &&
		    enabled)
		{
			retVal = ParameterGroupNumberRequestProtocol::request_repetition_rate(static_cast<std::uint32_t>(CANLibParameterGroupNumber::HeartbeatMessage),
			                                                                      SEQUENCE_REPETITION_RATE_MS,
			                                                                      sourceControlFunction,
			                                                                      destinationControlFunction);
		}
		return retVal;
	}

	void HeartbeatInterface::on_new_internal_control_function(std::shared_ptr<InternalControlFunction> newControlFunction)
	{
		auto pgnRequestProtocol = newControlFunction->get_pgn_request_protocol().lock();

		if (nullptr != pgnRequestProtocol)
		{
			pgnRequestProtocol->register_request_for_repetition_rate_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::HeartbeatMessage), process_request_for_heartbeat, this);
		}
	}

	void HeartbeatInterface::on_destroyed_internal_control_function(std::shared_ptr<InternalControlFunction> destroyedControlFunction)
	{
		auto pgnRequestProtocol = destroyedControlFunction->get_pgn_request_protocol().lock();

		if (nullptr != pgnRequestProtocol)
		{
			pgnRequestProtocol->remove_request_for_repetition_rate_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::HeartbeatMessage), process_request_for_heartbeat, this);
		}
	}

	EventDispatcher<HeartbeatInterface::HeartBeatError, std::shared_ptr<ControlFunction>> &HeartbeatInterface::get_heartbeat_error_event_dispatcher()
	{
		return heartbeatErrorEventDispatcher;
	}

	EventDispatcher<std::shared_ptr<ControlFunction>> &HeartbeatInterface::get_new_tracked_heartbeat_event_dispatcher()
	{
		return newTrackedHeartbeatEventDispatcher;
	}

	void HeartbeatInterface::update()
	{
		if (enabled)
		{
			trackedHeartbeats.erase(std::remove_if(trackedHeartbeats.begin(), trackedHeartbeats.end(), [this](Heartbeat &heartbeat) {
				                        bool retVal = false;

				                        if (nullptr != heartbeat.controlFunction)
				                        {
					                        if (ControlFunction::Type::Internal == heartbeat.controlFunction->get_type())
					                        {
						                        if ((SystemTiming::time_expired_ms(heartbeat.timestamp_ms, heartbeat.repetitionRate_ms)) &&
						                            heartbeat.send(*this))
						                        {
							                        heartbeat.sequenceCounter++;

							                        if (heartbeat.sequenceCounter > 250)
							                        {
								                        heartbeat.sequenceCounter = 0;
							                        }
						                        }
					                        }
					                        else if (SystemTiming::time_expired_ms(heartbeat.timestamp_ms, SEQUENCE_TIMEOUT_MS))
					                        {
						                        retVal = true; // External heartbeat is timed-out
						                        LOG_ERROR("[HB]: Heartbeat from control function at address 0x%02X timed out.", heartbeat.controlFunction->get_address());
						                        heartbeatErrorEventDispatcher.call(HeartBeatError::TimedOut, heartbeat.controlFunction);
					                        }
				                        }
				                        else
				                        {
					                        retVal = true; // Invalid state
				                        }
				                        return retVal;
			                        }),
			                        trackedHeartbeats.end());
		}
	}

	HeartbeatInterface::Heartbeat::Heartbeat(std::shared_ptr<ControlFunction> sendingControlFunction) :
	  controlFunction(sendingControlFunction),
	  timestamp_ms(SystemTiming::get_timestamp_ms())
	{
	}

	bool HeartbeatInterface::Heartbeat::send(const HeartbeatInterface &parent)
	{
		bool retVal = false;
		const std::array<std::uint8_t, 1> buffer = { sequenceCounter };

		retVal = parent.sendCANFrameCallback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::HeartbeatMessage),
		                                     CANDataSpan(buffer.data(), buffer.size()),
		                                     CANNetworkManager::CANNetwork.get_internal_control_function(controlFunction),
		                                     nullptr,
		                                     CANIdentifier::CANPriority::Priority3);
		if (retVal)
		{
			timestamp_ms = SystemTiming::get_timestamp_ms(); // Sent OK
		}
		return retVal;
	}

	void HeartbeatInterface::process_rx_message(const CANMessage &message)
	{
		if (enabled &&
		    (static_cast<std::uint32_t>(CANLibParameterGroupNumber::HeartbeatMessage) == message.get_identifier().get_parameter_group_number()) &&
		    (nullptr != message.get_source_control_function()) &&
		    (message.get_data_length() >= 1))
		{
			auto managedHeartbeat = std::find_if(trackedHeartbeats.begin(),
			                                     trackedHeartbeats.end(),
			                                     [&message](const Heartbeat &hb) {
				                                     return (message.get_source_control_function() == hb.controlFunction);
			                                     });

			if (managedHeartbeat != trackedHeartbeats.end())
			{
				managedHeartbeat->timestamp_ms = SystemTiming::get_timestamp_ms();

				if (message.get_uint8_at(0) == managedHeartbeat->sequenceCounter)
				{
					LOG_ERROR("[HB]: Duplicate sequence counter received in heartbeat.");
					heartbeatErrorEventDispatcher.call(HeartBeatError::InvalidSequenceCounter, message.get_source_control_function());
				}
				else if (message.get_uint8_at(0) != ((managedHeartbeat->sequenceCounter + 1) % 250))
				{
					LOG_ERROR("[HB]: Invalid sequence counter received in heartbeat.");
					heartbeatErrorEventDispatcher.call(HeartBeatError::InvalidSequenceCounter, message.get_source_control_function());
				}
				trackedHeartbeats.back().sequenceCounter = message.get_uint8_at(0);
			}
			else
			{
				LOG_DEBUG("[HB]: Tracking new heartbeat from control function at address 0x%02X.", message.get_source_control_function()->get_address());

				if (message.get_uint8_at(0) != static_cast<std::uint8_t>(HeartbeatInterface::SequenceCounterSpecialValue::Initial))
				{
					LOG_WARNING("[HB]: Initial heartbeat sequence counter not received from control function at address 0x%02X.", message.get_source_control_function()->get_address());
				}

				trackedHeartbeats.emplace_back(message.get_source_control_function());
				trackedHeartbeats.back().timestamp_ms = SystemTiming::get_timestamp_ms();
				trackedHeartbeats.back().sequenceCounter = message.get_uint8_at(0);
				newTrackedHeartbeatEventDispatcher.call(message.get_source_control_function());
			}
		}
	}

	bool HeartbeatInterface::process_request_for_heartbeat(std::uint32_t parameterGroupNumber,
	                                                       std::shared_ptr<ControlFunction> requestingControlFunction,
	                                                       std::shared_ptr<ControlFunction> targetControlFunction,
	                                                       std::uint32_t repetitionRate,
	                                                       void *parentPointer)
	{
		bool retVal = false;

		if (nullptr != parentPointer)
		{
			auto interface = static_cast<HeartbeatInterface *>(parentPointer);

			if ((interface->is_enabled()) &&
			    (static_cast<std::uint32_t>(CANLibParameterGroupNumber::HeartbeatMessage) == parameterGroupNumber))
			{
				retVal = true;

				if (SEQUENCE_REPETITION_RATE_MS != repetitionRate)
				{
					LOG_WARNING("[HB]: Control function at address 0x%02X requested the ISOBUS heartbeat at non-compliant interval. Interval should be 100ms.", requestingControlFunction->get_address());
				}
				else
				{
					LOG_DEBUG("[HB]: Control function at address 0x%02X requested the ISOBUS heartbeat from control function at address 0x%02X.", requestingControlFunction->get_address(), targetControlFunction->get_address());
				}

				auto managedHeartbeat = std::find_if(interface->trackedHeartbeats.begin(),
				                                     interface->trackedHeartbeats.end(),
				                                     [targetControlFunction](const Heartbeat &hb) {
					                                     return (targetControlFunction == hb.controlFunction);
				                                     });

				if (managedHeartbeat == interface->trackedHeartbeats.end())
				{
					interface->trackedHeartbeats.emplace_back(targetControlFunction); // Heartbeat will be sent on next update
				}
			}
		}
		return retVal;
	}
} // namespace isobus
