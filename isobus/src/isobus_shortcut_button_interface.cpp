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
				commandedState = newState;

				if (StopAllImplementOperationsState::StopImplementOperations == newState)
				{
					LOG_ERROR("[ISB]: All implement operations must stop. (Triggered internally)");
				}
				else
				{
					LOG_INFO("[ISB]: Internal ISB state is now permitted.");
				}
				txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendStopAllImplementOperationsSwitchState));
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
		if (SystemTiming::time_expired_ms(allImplementsStopOperationsSwitchStateTimestamp_ms, TRANSMISSION_RATE_MS))
		{
			// Prune old ISBs
			isobusShorcutButtonList.erase(std::remove_if(isobusShorcutButtonList.begin(), isobusShorcutButtonList.end(), [](const ISBServerData &isb) {
				                              return SystemTiming::time_expired_ms(isb.messageReceivedTimestamp_ms, TRANSMISSION_TIMEOUT_MS);
			                              }),
			                              isobusShorcutButtonList.end());

			txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendStopAllImplementOperationsSwitchState));
		}
		txFlags.process_all_flags();
	}

	void ShortcutButtonInterface::process_rx_message(const CANMessage &message, void *parentPointer)
	{
		assert(nullptr != parentPointer);

		static_cast<ShortcutButtonInterface *>(parentPointer)->process_message(message);
	}

	void ShortcutButtonInterface::process_flags(std::uint32_t flag, void *parent)
	{
		auto myInterface = static_cast<ShortcutButtonInterface *>(parent);
		bool transmitSuccessful = true;
		assert(nullptr != parent);

		if (flag == static_cast<std::uint32_t>(TransmitFlags::SendStopAllImplementOperationsSwitchState))
		{
			transmitSuccessful = myInterface->send_stop_all_implement_operations_switch_state();

			if (transmitSuccessful)
			{
				myInterface->stopAllImplementOperationsTransitionNumber++;
			}
		}

		if (!transmitSuccessful)
		{
			myInterface->txFlags.set_flag(flag);
		}
	}

	void ShortcutButtonInterface::process_message(const CANMessage &message)
	{
		if ((message.get_can_port_index() == sourceControlFunction->get_can_port()) &&
		    (static_cast<std::uint32_t>(CANLibParameterGroupNumber::AllImplementsStopOperationsSwitchState) == message.get_identifier().get_parameter_group_number()))
		{
			if (CAN_DATA_LENGTH == message.get_data_length())
			{
				auto messageNAME = message.get_source_control_function()->get_NAME();
				auto matches_isoname = [messageNAME](const ISBServerData &isb) { return isb.ISONAME == messageNAME; };
				auto ISB = std::find_if(isobusShorcutButtonList.begin(), isobusShorcutButtonList.end(), matches_isoname);
				auto &messageData = message.get_data();
				StopAllImplementOperationsState previousState = get_state();

				if (isobusShorcutButtonList.end() == ISB)
				{
					ISBServerData newISB;

					LOG_DEBUG("[ISB]: New ISB detected at address %u", message.get_identifier().get_source_address());
					newISB.ISONAME = messageNAME;
					isobusShorcutButtonList.emplace_back(newISB);
					ISB = std::prev(isobusShorcutButtonList.end());
				}

				if (isobusShorcutButtonList.end() != ISB)
				{
					std::uint8_t newTransitionCount = messageData.at(6);

					if (((ISB->stopAllImplementOperationsTransitionNumber == 255) &&
					     (0 != newTransitionCount)) ||
					    ((ISB->stopAllImplementOperationsTransitionNumber < 255) &&
					     (newTransitionCount > ISB->stopAllImplementOperationsTransitionNumber + 1)))
					{
						// A Working Set shall consider an increase in the transitions without detecting a corresponding
						// transition of the Stop all implement operations state as an error and react accordingly.
						ISB->commandedState = StopAllImplementOperationsState::StopImplementOperations;
						LOG_ERROR("[ISB]: Missed an ISB transition from ISB at address %u", message.get_identifier().get_source_address());
					}
					else
					{
						ISB->commandedState = static_cast<StopAllImplementOperationsState>(messageData.at(7) & 0x03);
					}
					ISB->messageReceivedTimestamp_ms = SystemTiming::get_timestamp_ms();
					ISB->stopAllImplementOperationsTransitionNumber = messageData.at(6);

					auto newState = get_state();
					if (previousState != newState)
					{
						if (StopAllImplementOperationsState::StopImplementOperations == newState)
						{
							LOG_ERROR("[ISB]: All implement operations must stop. (ISB at address %u has commanded it)", message.get_identifier().get_source_address());
						}
						else
						{
							LOG_INFO("[ISB]: Implement operations now permitted.");
						}
						ISBEventDispatcher.call(newState);
					}
				}
			}
			else
			{
				LOG_WARNING("[ISB]: Received malformed All Implements Stop Operations Switch State. DLC must be 8.");
			}
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
