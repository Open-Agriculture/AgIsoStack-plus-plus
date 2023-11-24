//================================================================================================
/// @file isobus_task_controller_server.cpp
///
/// @brief Implements portions of an abstract task controller server class.
/// You can consume this file and implement the pure virtual functions to create your own
/// task controller or data logger server.
/// @author Adrian Del Grosso
///
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/isobus_task_controller_server.hpp"

#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"

#include <cassert>

namespace isobus
{
	TaskControllerServer::TaskControllerServer(std::shared_ptr<InternalControlFunction> internalControlFunction,
	                                           std::uint8_t numberBoomsSupported,
	                                           std::uint8_t numberSectionsSupported,
	                                           std::uint8_t numberChannelsSupportedForPositionBasedControl,
	                                           std::uint8_t optionsBitfield) :
	  languageCommandInterface(internalControlFunction, true),
	  serverControlFunction(internalControlFunction),
	  numberBoomsSupportedToReport(numberBoomsSupported),
	  numberSectionsSupportedToReport(numberSectionsSupported),
	  numberChannelsSupportedForPositionBasedControlToReport(numberChannelsSupportedForPositionBasedControl),
	  optionsBitfieldToReport(optionsBitfield)
	{
	}

	void TaskControllerServer::initialize()
	{
		if (!initialized)
		{
			assert(nullptr != serverControlFunction); // You can't have a server without a control function to send messages from! That wouldn't make any sense.
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData), process_rx_message, this);
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::WorkingSetMaster), process_rx_message, this);
			initialized = true;
		}
	}

	bool TaskControllerServer::get_initialized() const
	{
		return initialized;
	}

	LanguageCommandInterface &TaskControllerServer::get_language_command_interface()
	{
		return languageCommandInterface;
	}

	void TaskControllerServer::update()
	{
		if ((true == SystemTiming::time_expired_ms(lastStatusMessageTimestamp_ms, STATUS_MESSAGE_RATE_MS)) &&
		    (true == send_status_message()))
		{
			lastStatusMessageTimestamp_ms = SystemTiming::get_timestamp_ms();
		}
	}

	void TaskControllerServer::process_rx_message(const CANMessage &message, void *parentPointer)
	{
		if (nullptr != parentPointer)
		{
			switch (message.get_identifier().get_parameter_group_number())
			{
				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData):
				{
				}
				break;

				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::WorkingSetMaster):
				{
				}
				break;

				default:
				{
				}
				break;
			}
		}
	}

	bool TaskControllerServer::send_generic_process_data_default_payload(std::uint8_t multiplexer, std::shared_ptr<ControlFunction> destination) const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> payload = { multiplexer, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData), payload.data(), payload.size(), serverControlFunction, destination);
	}

	bool TaskControllerServer::send_status_message() const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> payload = {
			static_cast<std::uint8_t>(ProcessDataCommands::Status) | 0xF0U,
			0xFF,
			0xFF,
			0xFF,
			currentStatusByte,
			currentCommandSourceAddress,
			currentCommandByte,
			0xFF
		};

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData),
		                                                      payload.data(),
		                                                      payload.size(),
		                                                      serverControlFunction);
	}
}
