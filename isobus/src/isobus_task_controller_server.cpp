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

	bool TaskControllerServer::send_request_value(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber) const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> payload = { 0 };

		payload[0] = (static_cast<std::uint8_t>(ProcessDataCommands::RequestValue) & 0x0F);
		payload[0] |= (elementNumber && 0x0F) << 4;
		payload[1] = static_cast<std::uint8_t>((elementNumber >> 4) & 0xFF);
		payload[2] = static_cast<std::uint8_t>(dataDescriptionIndex & 0xFF);
		payload[3] = static_cast<std::uint8_t>((dataDescriptionIndex >> 8) & 0xFF);
		payload[4] = 0xFF;
		payload[5] = 0xFF;
		payload[6] = 0xFF;
		payload[7] = 0xFF;
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData),
		                                                      payload.data(),
		                                                      payload.size(),
		                                                      serverControlFunction,
		                                                      clientControlFunction,
		                                                      CANIdentifier::CANPriority::Priority5);
	}

	bool TaskControllerServer::send_time_interval_measurement_command(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber, std::uint32_t timeInterval) const
	{
		return send_measurement_command(clientControlFunction, static_cast<std::uint8_t>(ProcessDataCommands::MeasurementTimeInterval), dataDescriptionIndex, elementNumber, timeInterval);
	}

	bool TaskControllerServer::send_distance_interval_measurement_command(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber, std::uint32_t distanceInterval) const
	{
		return send_measurement_command(clientControlFunction, static_cast<std::uint8_t>(ProcessDataCommands::MeasurementDistanceInterval), dataDescriptionIndex, elementNumber, distanceInterval);
	}

	bool TaskControllerServer::send_minimum_threshold_measurement_command(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber, std::uint32_t minimum) const
	{
		return send_measurement_command(clientControlFunction, static_cast<std::uint8_t>(ProcessDataCommands::MeasurementMinimumWithinThreshold), dataDescriptionIndex, elementNumber, minimum);
	}

	bool TaskControllerServer::send_maximum_threshold_measurement_command(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber, std::uint32_t maximum) const
	{
		return send_measurement_command(clientControlFunction, static_cast<std::uint8_t>(ProcessDataCommands::MeasurementMaximumWithinThreshold), dataDescriptionIndex, elementNumber, maximum);
	}

	bool TaskControllerServer::send_change_threshold_measurement_command(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber, std::uint32_t threshold) const
	{
		return send_measurement_command(clientControlFunction, static_cast<std::uint8_t>(ProcessDataCommands::MeasurementChangeThreshold), dataDescriptionIndex, elementNumber, threshold);
	}

	bool TaskControllerServer::send_set_value_and_acknowledge(std::shared_ptr<ControlFunction> clientControlFunction, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber, std::uint32_t processDataValue) const
	{
		return send_measurement_command(clientControlFunction, static_cast<std::uint8_t>(ProcessDataCommands::SetValueAndAcknowledge), dataDescriptionIndex, elementNumber, processDataValue);
	}

	void TaskControllerServer::set_task_totals_active(bool isTaskActive)
	{
		if (isTaskActive != get_task_totals_active())
		{
			currentStatusByte &= ~static_cast<std::uint8_t>(ServerStatusBit::TaskTotalsActive);
			currentStatusByte |= (static_cast<std::uint8_t>(isTaskActive) & static_cast<std::uint8_t>(ServerStatusBit::TaskTotalsActive));
			lastStatusMessageTimestamp_ms = 0; // Force a status message to be sent on the next update.
		}
	}

	bool TaskControllerServer::get_task_totals_active() const
	{
		return (0 != (currentStatusByte & static_cast<std::uint8_t>(ServerStatusBit::TaskTotalsActive)));
	}

	void TaskControllerServer::initialize()
	{
		if (!initialized)
		{
			assert(nullptr != serverControlFunction); // You can't have a server without a control function to send messages from! That wouldn't make any sense.
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData), store_rx_message, this);
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::WorkingSetMaster), store_rx_message, this);
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
		process_rx_messages();
		if ((true == SystemTiming::time_expired_ms(lastStatusMessageTimestamp_ms, STATUS_MESSAGE_RATE_MS)) &&
		    (true == send_status_message()))
		{
			lastStatusMessageTimestamp_ms = SystemTiming::get_timestamp_ms();
		}
	}

	void TaskControllerServer::store_rx_message(const CANMessage &message, void *parentPointer)
	{
		if (nullptr != parentPointer)
		{
			static_cast<TaskControllerServer *>(parentPointer)->rxMessageQueue.push_back(message);
			switch (message.get_identifier().get_parameter_group_number())
			{
				case static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData):
				{
					switch (static_cast<ProcessDataCommands>(message.get_uint8_at(0) & 0x0F))
					{
						case ProcessDataCommands::TechnicalCapabilities:
						case ProcessDataCommands::DeviceDescriptor:
						case ProcessDataCommands::RequestValue:
						case ProcessDataCommands::Value:
						case ProcessDataCommands::MeasurementTimeInterval:
						case ProcessDataCommands::MeasurementDistanceInterval:
						case ProcessDataCommands::MeasurementMinimumWithinThreshold:
						case ProcessDataCommands::MeasurementMaximumWithinThreshold:
						case ProcessDataCommands::MeasurementChangeThreshold:
						case ProcessDataCommands::PeerControlAssignment:
						case ProcessDataCommands::SetValueAndAcknowledge:
						case ProcessDataCommands::Acknowledge:
						case ProcessDataCommands::Status:
						case ProcessDataCommands::ClientTask:
						default:
						{
						}
						break;
					}
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

	void TaskControllerServer::process_rx_messages()
	{
		while (!rxMessageQueue.empty())
		{
			auto &rxMessage = rxMessageQueue.front();

			switch (rxMessage.get_identifier().get_parameter_group_number())
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
			rxMessageQueue.pop_front();
		}
	}

	bool TaskControllerServer::send_generic_process_data_default_payload(std::uint8_t multiplexer, std::shared_ptr<ControlFunction> destination) const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> payload = { multiplexer, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData), payload.data(), payload.size(), serverControlFunction, destination);
	}

	bool TaskControllerServer::send_measurement_command(std::shared_ptr<ControlFunction> clientControlFunction, std::uint8_t commandValue, std::uint16_t dataDescriptionIndex, std::uint16_t elementNumber, std::uint32_t processDataValue) const
	{
		std::array<std::uint8_t, CAN_DATA_LENGTH> payload = { 0 };
		bool retVal = false;

		payload[0] = (commandValue & 0x0F);
		payload[0] |= (elementNumber && 0x0F) << 4;
		payload[1] = static_cast<std::uint8_t>((elementNumber >> 4) & 0xFF);
		payload[2] = static_cast<std::uint8_t>(dataDescriptionIndex & 0xFF);
		payload[3] = static_cast<std::uint8_t>((dataDescriptionIndex >> 8) & 0xFF);
		payload[4] = static_cast<std::uint8_t>(processDataValue & 0xFF);
		payload[5] = static_cast<std::uint8_t>((processDataValue >> 8) & 0xFF);
		payload[6] = static_cast<std::uint8_t>((processDataValue >> 16) & 0xFF);
		payload[7] = static_cast<std::uint8_t>((processDataValue >> 24) & 0xFF);

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::ProcessData),
		                                                      payload.data(),
		                                                      payload.size(),
		                                                      serverControlFunction,
		                                                      clientControlFunction,
		                                                      static_cast<std::uint8_t>(ProcessDataCommands::SetValueAndAcknowledge) == commandValue ? CANIdentifier::CANPriority::Priority3 : CANIdentifier::CANPriority::Priority5);
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
