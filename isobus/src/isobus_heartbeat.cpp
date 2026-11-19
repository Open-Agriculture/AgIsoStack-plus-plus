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
/// @author Sujan Dumaru
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

		LOCK_GUARD(Mutex, pendingCommandsMutex);
		enabled = enable;

		if (!enable)
		{
			pendingCommands.erase(std::remove_if(pendingCommands.begin(), pendingCommands.end(), [](const PendingCommand &command) {
				                      return (CommandType::Remove != command.type);
			                      }),
			                      pendingCommands.end());
		}
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
		    (nullptr != destinationControlFunction))
		{
			LOCK_GUARD(Mutex, pendingCommandsMutex);

			if (enabled)
			{
				pendingCommands.push_back({ CommandType::RequestHeartbeat, sourceControlFunction, destinationControlFunction });
				retVal = true;
			}
		}
		return retVal;
	}

	void HeartbeatInterface::send_shutdown(std::shared_ptr<InternalControlFunction> controlFunction)
	{
		if (nullptr != controlFunction)
		{
			LOCK_GUARD(Mutex, pendingCommandsMutex);

			if (enabled)
			{
				pendingCommands.push_back({ CommandType::Shutdown, nullptr, controlFunction });
			}
		}
	}

	void HeartbeatInterface::set_transmission_error(std::shared_ptr<InternalControlFunction> controlFunction, bool error)
	{
		if (nullptr != controlFunction)
		{
			LOCK_GUARD(Mutex, pendingCommandsMutex);

			if (enabled)
			{
				pendingCommands.push_back({ error ? CommandType::SetTransmissionError : CommandType::ClearTransmissionError, nullptr, controlFunction });
			}
		}
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

		LOCK_GUARD(Mutex, pendingCommandsMutex);
		pendingCommands.push_back({ CommandType::Remove, nullptr, destroyedControlFunction });
	}

	EventDispatcher<HeartbeatInterface::HeartBeatError, std::shared_ptr<ControlFunction>> &HeartbeatInterface::get_heartbeat_error_event_dispatcher()
	{
		return heartbeatErrorEventDispatcher;
	}

	EventDispatcher<std::shared_ptr<ControlFunction>> &HeartbeatInterface::get_new_tracked_heartbeat_event_dispatcher()
	{
		return newTrackedHeartbeatEventDispatcher;
	}

	EventDispatcher<std::shared_ptr<ControlFunction>> &HeartbeatInterface::get_heartbeat_recovered_event_dispatcher()
	{
		return heartbeatRecoveredEventDispatcher;
	}

	void HeartbeatInterface::update()
	{
		std::vector<PendingCommand> commands;
		{
			LOCK_GUARD(Mutex, pendingCommandsMutex);
			commands.swap(pendingCommands);
		}

		// Flags and removals go first so they apply to a frame that is due in this update. Requests go after the timeout sweep so a retry for a heartbeat that just timed out is sent.
		for (const auto &command : commands)
		{
			if ((CommandType::RequestHeartbeat != command.type) && (enabled || (CommandType::Remove == command.type)))
			{
				apply_command(command);
			}
		}

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

							                        if (heartbeat.sequenceCounter > MAX_SEQUENCE_COUNTER)
							                        {
								                        heartbeat.sequenceCounter = 0;
							                        }
						                        }
					                        }
					                        else if (heartbeat.requestPending && (SystemTiming::get_time_elapsed_ms(heartbeat.timestamp_ms) > SEQUENCE_INITIAL_RESPONSE_TIMEOUT_MS))
					                        {
						                        retVal = true;
						                        LOG_WARNING("[HB]: Control function at address 0x%02X did not accept the heartbeat request.", heartbeat.controlFunction->get_address());
						                        heartbeatErrorEventDispatcher.call(HeartBeatError::RequestNotAccepted, heartbeat.controlFunction);
					                        }
					                        else if (SystemTiming::get_time_elapsed_ms(heartbeat.timestamp_ms) > SEQUENCE_TIMEOUT_MS)
					                        {
						                        retVal = true;
						                        LOG_ERROR("[HB]: Heartbeat from control function at address 0x%02X timed out.", heartbeat.controlFunction->get_address());
						                        heartbeatErrorEventDispatcher.call(HeartBeatError::TimedOut, heartbeat.controlFunction);
					                        }
				                        }
				                        else
				                        {
					                        retVal = true;
				                        }
				                        return retVal;
			                        }),
			                        trackedHeartbeats.end());

			for (const auto &command : commands)
			{
				if (CommandType::RequestHeartbeat == command.type)
				{
					apply_command(command);
				}
			}
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
		std::uint8_t counterToSend = sequenceCounter;

		if (shutdown)
		{
			counterToSend = static_cast<std::uint8_t>(SequenceCounterSpecialValue::NotAvailable);
		}
		else if (transmissionError)
		{
			counterToSend = static_cast<std::uint8_t>(SequenceCounterSpecialValue::Error);
		}
		const std::array<std::uint8_t, 8> buffer = { counterToSend, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

		retVal = parent.sendCANFrameCallback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::HeartbeatMessage),
		                                     CANDataSpan(buffer.data(), buffer.size()),
		                                     CANNetworkManager::CANNetwork.get_internal_control_function(controlFunction),
		                                     nullptr,
		                                     CANIdentifier::CANPriority::Priority3);
		if (retVal)
		{
			timestamp_ms = SystemTiming::get_timestamp_ms();
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
			const std::uint8_t sequenceCounter = message.get_uint8_at(0);

			// Receivers shall ignore 252 and 253, so they don't count as a received heartbeat either
			if ((static_cast<std::uint8_t>(SequenceCounterSpecialValue::FirstReserved) != sequenceCounter) &&
			    (static_cast<std::uint8_t>(SequenceCounterSpecialValue::LastReserved) != sequenceCounter))
			{
				auto managedHeartbeat = find_tracked_heartbeat(message.get_source_control_function());

				if ((managedHeartbeat != trackedHeartbeats.end()) &&
				    (ControlFunction::Type::Internal != managedHeartbeat->controlFunction->get_type()) &&
				    (SystemTiming::get_time_elapsed_ms(managedHeartbeat->timestamp_ms) > SEQUENCE_TIMEOUT_MS))
				{
					LOG_ERROR("[HB]: Heartbeat from control function at address 0x%02X timed out.", managedHeartbeat->controlFunction->get_address());
					heartbeatErrorEventDispatcher.call(HeartBeatError::TimedOut, managedHeartbeat->controlFunction);
					trackedHeartbeats.erase(managedHeartbeat);
					managedHeartbeat = trackedHeartbeats.end();
				}

				if (static_cast<std::uint8_t>(SequenceCounterSpecialValue::NotAvailable) == sequenceCounter)
				{
					if (managedHeartbeat != trackedHeartbeats.end())
					{
						LOG_INFO("[HB]: Control function at address 0x%02X is shutting down.", message.get_source_control_function()->get_address());
						heartbeatErrorEventDispatcher.call(HeartBeatError::GracefulShutdown, message.get_source_control_function());
						trackedHeartbeats.erase(managedHeartbeat);
					}
				}
				else if (managedHeartbeat != trackedHeartbeats.end())
				{
					managedHeartbeat->requestPending = false;
					managedHeartbeat->timestamp_ms = SystemTiming::get_timestamp_ms();
					process_sequence_counter(*managedHeartbeat, sequenceCounter);
				}
				else
				{
					trackedHeartbeats.emplace_back(message.get_source_control_function());
					trackedHeartbeats.back().sequenceCounter = static_cast<std::uint8_t>(SequenceCounterSpecialValue::NotAvailable);
					process_sequence_counter(trackedHeartbeats.back(), sequenceCounter);
				}
			}
		}
		else if (enabled &&
		         (static_cast<std::uint32_t>(CANLibParameterGroupNumber::ResponseForRepetitionRate) == message.get_identifier().get_parameter_group_number()) &&
		         (nullptr != message.get_source_control_function()) &&
		         (message.get_data_length() >= 3) &&
		         (static_cast<std::uint32_t>(CANLibParameterGroupNumber::HeartbeatMessage) == message.get_uint24_at(0)))
		{
			auto managedHeartbeat = find_tracked_heartbeat(message.get_source_control_function());

			if ((managedHeartbeat != trackedHeartbeats.end()) && managedHeartbeat->requestPending)
			{
				managedHeartbeat->requestPending = false;
				managedHeartbeat->timestamp_ms = SystemTiming::get_timestamp_ms(); // The first heartbeat may come up to 100 ms after the response
			}
		}
	}

	void HeartbeatInterface::apply_command(const PendingCommand &command)
	{
		auto managedHeartbeat = find_tracked_heartbeat(command.controlFunction);
		const bool found = (managedHeartbeat != trackedHeartbeats.end());

		switch (command.type)
		{
			case CommandType::RequestHeartbeat:
			{
				if ((!found) &&
				    ParameterGroupNumberRequestProtocol::request_repetition_rate(static_cast<std::uint32_t>(CANLibParameterGroupNumber::HeartbeatMessage),
				                                                                 SEQUENCE_REPETITION_RATE_MS,
				                                                                 command.source,
				                                                                 command.controlFunction))
				{
					if (ControlFunction::Type::Internal != command.controlFunction->get_type())
					{
						trackedHeartbeats.emplace_back(command.controlFunction);
						trackedHeartbeats.back().sequenceCounter = static_cast<std::uint8_t>(SequenceCounterSpecialValue::NotAvailable);
						trackedHeartbeats.back().requestPending = true;
					}
				}
			}
			break;

			case CommandType::Shutdown:
			{
				if (found)
				{
					managedHeartbeat->shutdown = true;
				}
			}
			break;

			case CommandType::SetTransmissionError:
			case CommandType::ClearTransmissionError:
			{
				if (found)
				{
					managedHeartbeat->transmissionError = (CommandType::SetTransmissionError == command.type);
				}
			}
			break;

			case CommandType::Remove:
			{
				if (found)
				{
					trackedHeartbeats.erase(managedHeartbeat);
				}
			}
			break;
		}
	}

	void HeartbeatInterface::process_sequence_counter(Heartbeat &heartbeat, std::uint8_t sequenceCounter)
	{
		const std::uint8_t previous = heartbeat.sequenceCounter;
		const std::uint8_t address = heartbeat.controlFunction->get_address();
		heartbeat.sequenceCounter = sequenceCounter;

		if (static_cast<std::uint8_t>(SequenceCounterSpecialValue::NotAvailable) == previous)
		{
			LOG_DEBUG("[HB]: Tracking new heartbeat from control function at address 0x%02X.", address);

			if (static_cast<std::uint8_t>(SequenceCounterSpecialValue::Initial) != sequenceCounter)
			{
				LOG_WARNING("[HB]: Initial heartbeat sequence counter not received from control function at address 0x%02X.", address);
			}
			newTrackedHeartbeatEventDispatcher.call(heartbeat.controlFunction);
		}

		if (static_cast<std::uint8_t>(SequenceCounterSpecialValue::Error) == sequenceCounter)
		{
			LOG_WARNING("[HB]: Control function at address 0x%02X reported a transmission error.", address);
			enter_error_state(heartbeat, ReceiveState::TransmissionError);
		}
		else if (static_cast<std::uint8_t>(SequenceCounterSpecialValue::Initial) == sequenceCounter)
		{
			heartbeat.correctMessagesInARow = 0; // The sender restarted, which is a resync and not an error
		}
		else if (static_cast<std::uint8_t>(SequenceCounterSpecialValue::Error) == previous)
		{
			heartbeat.correctMessagesInARow = 1; // The first valid counter after 254 starts the recovery count
		}
		else if (static_cast<std::uint8_t>(SequenceCounterSpecialValue::NotAvailable) != previous)
		{
			const std::uint8_t base = (static_cast<std::uint8_t>(SequenceCounterSpecialValue::Initial) == previous) ? MAX_SEQUENCE_COUNTER : previous; // 251 is followed by 0
			const std::uint8_t increment = static_cast<std::uint8_t>((sequenceCounter + MAX_SEQUENCE_COUNTER + 1 - base) % (MAX_SEQUENCE_COUNTER + 1));

			if ((0 == increment) || (MAX_ALLOWED_INCREMENT < increment))
			{
				LOG_ERROR("[HB]: Invalid sequence counter %u after %u received in heartbeat from control function at address 0x%02X.", sequenceCounter, previous, address);
				enter_error_state(heartbeat, ReceiveState::SequenceError);
			}
			else if (1 == increment)
			{
				if (ReceiveState::Operational != heartbeat.state)
				{
					heartbeat.correctMessagesInARow++;

					if (RECOVERY_MESSAGE_COUNT <= heartbeat.correctMessagesInARow)
					{
						heartbeat.state = ReceiveState::Operational;
						heartbeat.correctMessagesInARow = 0;
						LOG_INFO("[HB]: Heartbeat from control function at address 0x%02X recovered.", address);
						heartbeatRecoveredEventDispatcher.call(heartbeat.controlFunction);
					}
				}
			}
			else
			{
				heartbeat.correctMessagesInARow = 0; // Missed messages are allowed, but they break "in a row"
			}
		}
	}

	void HeartbeatInterface::enter_error_state(Heartbeat &heartbeat, ReceiveState errorState)
	{
		heartbeat.correctMessagesInARow = 0;

		if (errorState != heartbeat.state)
		{
			heartbeat.state = errorState;
			heartbeatErrorEventDispatcher.call((ReceiveState::TransmissionError == errorState) ? HeartBeatError::TransmissionError : HeartBeatError::InvalidSequenceCounter, heartbeat.controlFunction);
		}
	}

	std::list<HeartbeatInterface::Heartbeat>::iterator HeartbeatInterface::find_tracked_heartbeat(const std::shared_ptr<ControlFunction> &controlFunction)
	{
		return std::find_if(trackedHeartbeats.begin(), trackedHeartbeats.end(), [&controlFunction](const Heartbeat &hb) {
			return (controlFunction == hb.controlFunction);
		});
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
			    (static_cast<std::uint32_t>(CANLibParameterGroupNumber::HeartbeatMessage) == parameterGroupNumber) &&
			    (REPETITION_RATE_NO_CHANGE != repetitionRate))
			{
				retVal = true;
				LOG_DEBUG("[HB]: Control function at address 0x%02X requested the ISOBUS heartbeat from control function at address 0x%02X at %u ms. It is sent every 100 ms.", requestingControlFunction->get_address(), targetControlFunction->get_address(), static_cast<unsigned>(repetitionRate));

				auto managedHeartbeat = interface->find_tracked_heartbeat(targetControlFunction);

				if (managedHeartbeat == interface->trackedHeartbeats.end())
				{
					interface->trackedHeartbeats.emplace_back(targetControlFunction);
					interface->trackedHeartbeats.back().timestamp_ms = 0; // Heartbeat will be sent on next update
				}

				const std::uint32_t pgn = static_cast<std::uint32_t>(CANLibParameterGroupNumber::HeartbeatMessage);
				const std::array<std::uint8_t, 8> response = { static_cast<std::uint8_t>(pgn & 0xFF),
					                                             static_cast<std::uint8_t>((pgn >> 8) & 0xFF),
					                                             static_cast<std::uint8_t>((pgn >> 16) & 0xFF),
					                                             static_cast<std::uint8_t>(SEQUENCE_REPETITION_RATE_MS & 0xFF),
					                                             static_cast<std::uint8_t>((SEQUENCE_REPETITION_RATE_MS >> 8) & 0xFF),
					                                             0xFF,
					                                             0xFF,
					                                             0xFF };
				interface->sendCANFrameCallback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ResponseForRepetitionRate),
				                                CANDataSpan(response.data(), response.size()),
				                                CANNetworkManager::CANNetwork.get_internal_control_function(targetControlFunction),
				                                nullptr,
				                                CANIdentifier::CANPriority::PriorityDefault6);
			}
		}
		return retVal;
	}
} // namespace isobus
