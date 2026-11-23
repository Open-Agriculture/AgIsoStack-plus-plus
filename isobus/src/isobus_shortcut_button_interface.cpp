//================================================================================================
/// @file isobus_shortcut_button_interface.cpp
///
/// @brief Implements the interface for an ISOBUS shortcut button
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/isobus_shortcut_button_interface.hpp"

#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"

#include <algorithm>
#include <cassert>
#include <vector>

namespace isobus
{
	ShortcutButtonInterface::ShortcutButtonInterface(std::shared_ptr<InternalControlFunction> internalControlFunction, bool serverEnabled) :
	  sourceControlFunction(internalControlFunction),
	  txFlags(static_cast<std::uint32_t>(TransmitFlags::NumberOfFlags), process_flags, this),
	  actAsISBServer(serverEnabled)
	{
		assert(nullptr != sourceControlFunction); // You need an internal control function for the interface to function
	}

	ShortcutButtonInterface::~ShortcutButtonInterface()
	{
		if (initialized)
		{
			CANNetworkManager::CANNetwork.remove_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AllImplementsStopOperationsSwitchState), process_rx_message, this);
		}
	}

	void ShortcutButtonInterface::initialize()
	{
		if (!initialized)
		{
			CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AllImplementsStopOperationsSwitchState),
			                                                                         process_rx_message,
			                                                                         this);
			initialized = true;
		}
	}

	bool ShortcutButtonInterface::get_is_initialized() const
	{
		return initialized;
	}

	EventDispatcher<ShortcutButtonInterface::StopAllImplementOperationsState> &ShortcutButtonInterface::get_stop_all_implement_operations_state_event_dispatcher()
	{
		return ISBEventDispatcher;
	}

	void ShortcutButtonInterface::set_stop_all_implement_operations_state(StopAllImplementOperationsState newState)
	{
		if (actAsISBServer)
		{
			if (newState != commandedState)
			{
				// Send a pending retry with the old state first, so a failed Stop is not replaced by the next state.
				txFlags.process_all_flags();
				if ((StopAllImplementOperationsState::PermitAllImplementsToOperationOn == commandedState) &&
				    (StopAllImplementOperationsState::StopImplementOperations == newState))
				{
					stopAllImplementOperationsTransitionNumber++;
				}
				commandedState = newState;

				if (StopAllImplementOperationsState::StopImplementOperations == newState)
				{
					LOG_ERROR("[ISB]: All implement operations must stop. (Triggered internally)");
				}
				else
				{
					LOG_INFO("[ISB]: Internal ISB state is now permitted.");
				}
				if (!send_stop_all_implement_operations_switch_state())
				{
					txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendStopAllImplementOperationsSwitchState));
				}
			}
		}
		else
		{
			LOG_ERROR("[ISB]: You are attempting to set the internal ISB state but the ISB interface is not configured as a server!");
		}
	}

	ShortcutButtonInterface::StopAllImplementOperationsState ShortcutButtonInterface::get_state() const
	{
		StopAllImplementOperationsState retVal = StopAllImplementOperationsState::PermitAllImplementsToOperationOn;

		if (StopAllImplementOperationsState::StopImplementOperations == commandedState)
		{
			retVal = commandedState;
		}
		else
		{
			for (auto ISB = isobusShorcutButtonList.cbegin(); ISB != isobusShorcutButtonList.end(); ISB++)
			{
				if (StopAllImplementOperationsState::StopImplementOperations == ISB->commandedState)
				{
					retVal = ISB->commandedState; // Any stop condition will be returned.
					break;
				}
			}
		}
		return retVal;
	}

	void ShortcutButtonInterface::update()
	{
		// Taken out before parsing, so messages that arrive while a listener runs wait for the next update() instead of keeping this one going
		std::vector<ReceivedFrame> frames;
		ReceivedFrame frame;
		while (receivedFrames.pop(&frame))
		{
			frames.push_back(frame);
		}
		// Read before the listeners run, so a slow listener can't expire an ISB whose next messages are already queued
		const std::uint32_t updateTimestamp_ms = SystemTiming::get_timestamp_ms();
		for (const auto &receivedFrame : frames)
		{
			// Parsed even when older than the timeout: a Stop that waited here was still pressed, and the transition count needs every message; the timeout pass below expires it
			process_received_frame(receivedFrame);
		}

		StopAllImplementOperationsState previousState = get_state();

		// Timed-out ISBs stay in the list so their transition count remains the baseline when they come back.
		for (auto &isb : isobusShorcutButtonList)
		{
			if (TRANSMISSION_TIMEOUT_MS <= (updateTimestamp_ms - isb.messageReceivedTimestamp_ms))
			{
				isb.commandedState = StopAllImplementOperationsState::NotAvailable;
			}
		}

		if (previousState != get_state())
		{
			LOG_WARNING("[ISB]: An ISB that commanded a stop timed out. Implement operations now permitted.");
			ISBEventDispatcher.call(get_state());
		}

		if (actAsISBServer && SystemTiming::time_expired_ms(allImplementsStopOperationsSwitchStateTimestamp_ms, TRANSMISSION_RATE_MS))
		{
			allImplementsStopOperationsSwitchStateTimestamp_ms = SystemTiming::get_timestamp_ms();
			txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendStopAllImplementOperationsSwitchState));
		}
		txFlags.process_all_flags();
	}

	void ShortcutButtonInterface::process_rx_message(const CANMessage &message, void *parentPointer)
	{
		assert(nullptr != parentPointer);
		auto targetInterface = static_cast<ShortcutButtonInterface *>(parentPointer);

		if ((message.get_can_port_index() == targetInterface->sourceControlFunction->get_can_port()) &&
		    (static_cast<std::uint32_t>(CANLibParameterGroupNumber::AllImplementsStopOperationsSwitchState) == message.get_identifier().get_parameter_group_number()))
		{
			if (CAN_DATA_LENGTH == message.get_data_length())
			{
				ReceivedFrame frame;
				frame.ISONAME = message.get_source_control_function()->get_NAME();
				frame.timestamp_ms = SystemTiming::get_timestamp_ms();
				frame.sourceAddress = message.get_identifier().get_source_address();
				frame.transitionNumber = message.get_uint8_at(6);
				frame.state = static_cast<StopAllImplementOperationsState>(message.get_uint8_at(7) & 0x03);

				targetInterface->receivedFrames.push(frame);
			}
			else
			{
				LOG_WARNING("[ISB]: Received malformed All Implements Stop Operations Switch State. DLC must be 8.");
			}
		}
	}

	void ShortcutButtonInterface::process_flags(std::uint32_t flag, void *parent)
	{
		auto myInterface = static_cast<ShortcutButtonInterface *>(parent);
		bool transmitSuccessful = true;
		assert(nullptr != parent);

		if (flag == static_cast<std::uint32_t>(TransmitFlags::SendStopAllImplementOperationsSwitchState))
		{
			transmitSuccessful = myInterface->send_stop_all_implement_operations_switch_state();
		}

		if (!transmitSuccessful)
		{
			myInterface->txFlags.set_flag(flag);
		}
	}

	void ShortcutButtonInterface::process_received_frame(const ReceivedFrame &frame)
	{
		auto matches_isoname = [&frame](const ISBServerData &isb) { return isb.ISONAME == frame.ISONAME; };
		auto ISB = std::find_if(isobusShorcutButtonList.begin(), isobusShorcutButtonList.end(), matches_isoname);
		StopAllImplementOperationsState previousState = get_state();

		if (isobusShorcutButtonList.end() == ISB)
		{
			ISBServerData newISB;

			LOG_DEBUG("[ISB]: New ISB detected at address %u", frame.sourceAddress);
			newISB.ISONAME = frame.ISONAME;
			newISB.commandedState = frame.state;
			isobusShorcutButtonList.emplace_back(newISB);
			ISB = std::prev(isobusShorcutButtonList.end());
		}
		else
		{
			bool countChanged = (frame.transitionNumber != ISB->stopAllImplementOperationsTransitionNumber);
			bool expectedTransition = (static_cast<std::uint8_t>(ISB->stopAllImplementOperationsTransitionNumber + 1) == frame.transitionNumber) &&
			  (StopAllImplementOperationsState::PermitAllImplementsToOperationOn == ISB->reportedState) &&
			  (StopAllImplementOperationsState::StopImplementOperations == frame.state);

			// A sender that only reports NotAvailable has no switch, and older stack versions raise its count on every send.
			if (((StopAllImplementOperationsState::NotAvailable != frame.state) || ISB->reportedSwitchState) && countChanged && (!expectedTransition))
			{
				ISB->commandedState = StopAllImplementOperationsState::StopImplementOperations;
				LOG_ERROR("[ISB]: Missed an ISB transition from ISB at address %u", frame.sourceAddress);
			}
			else
			{
				ISB->commandedState = frame.state;
			}
		}
		ISB->messageReceivedTimestamp_ms = frame.timestamp_ms;
		ISB->stopAllImplementOperationsTransitionNumber = frame.transitionNumber;
		ISB->reportedState = frame.state;
		if (StopAllImplementOperationsState::NotAvailable != frame.state)
		{
			ISB->reportedSwitchState = true;
		}

		auto newState = get_state();
		if (previousState != newState)
		{
			if (StopAllImplementOperationsState::StopImplementOperations == newState)
			{
				LOG_ERROR("[ISB]: All implement operations must stop. (ISB at address %u has commanded it)", frame.sourceAddress);
			}
			else
			{
				LOG_INFO("[ISB]: Implement operations now permitted.");
			}
			ISBEventDispatcher.call(newState);
		}
	}

	bool ShortcutButtonInterface::send_stop_all_implement_operations_switch_state() const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
			0xFF,
			0xFF,
			0xFF,
			0xFF,
			0xFF,
			0xFF,
			stopAllImplementOperationsTransitionNumber,
			static_cast<std::uint8_t>(0xFC | static_cast<std::uint8_t>(commandedState))
		};

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AllImplementsStopOperationsSwitchState),
		                                                      buffer.data(),
		                                                      static_cast<std::uint32_t>(buffer.size()),
		                                                      sourceControlFunction,
		                                                      nullptr,
		                                                      CANIdentifier::CANPriority::Priority3);
	}
}
