//================================================================================================
/// @file isobus_speed_distance_messages.cpp
///
/// @brief Implements an interface for sending and receiving ISOBUS speed/distance messages.
/// These messages are used to receive or transmit data about how fast the machine is going.
/// You can also use the machine selected speed command to command a machine to drive at a
/// desired speed.
///
/// @attention Please use extreme care if you try to control the speed of a machine
/// with this interface! Remember that this library is licensed under The MIT License,
/// and that by obtaining a copy of this library and of course by attempting to
/// control a machine with it, you are agreeing to our license.
///
/// @note Generally you will want to use the machine selected speed rather than the other
/// speeds, as the TECU chooses its favorite speed and reports it in that message.
///
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/isobus_speed_distance_messages.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"

#include <cassert>

namespace isobus
{
	SpeedMessagesInterface::SpeedMessagesInterface(std::shared_ptr<InternalControlFunction> source,
	                                               bool enableSendingGroundBasedSpeedPeriodically,
	                                               bool enableSendingWheelBasedSpeedPeriodically,
	                                               bool enableSendingMachineSelectedSpeedPeriodically,
	                                               bool enableSendingMachineSelectedSpeedCommandPeriodically) :
	  machineSelectedSpeedTransmitData(MachineSelectedSpeedData(enableSendingMachineSelectedSpeedPeriodically ? source : nullptr)),
	  wheelBasedSpeedTransmitData(WheelBasedMachineSpeedData(enableSendingWheelBasedSpeedPeriodically ? source : nullptr)),
	  groundBasedSpeedTransmitData(GroundBasedSpeedData(enableSendingGroundBasedSpeedPeriodically ? source : nullptr)),
	  machineSelectedSpeedCommandTransmitData(MachineSelectedSpeedCommandData(enableSendingMachineSelectedSpeedCommandPeriodically ? source : nullptr)),
	  txFlags(static_cast<std::uint32_t>(TransmitFlags::NumberOfFlags), process_flags, this)
	{
	}

	SpeedMessagesInterface::~SpeedMessagesInterface()
	{
		if (initialized)
		{
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::MachineSelectedSpeed), process_rx_message, this);
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::WheelBasedSpeedAndDistance), process_rx_message, this);
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::GroundBasedSpeedAndDistance), process_rx_message, this);
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::MachineSelectedSpeedCommand), process_rx_message, this);
		}
	}

	SpeedMessagesInterface::WheelBasedMachineSpeedData::WheelBasedMachineSpeedData(std::shared_ptr<ControlFunction> sender) :
	  controlFunction(sender)
	{
	}

	std::uint32_t SpeedMessagesInterface::WheelBasedMachineSpeedData::get_machine_distance() const
	{
		std::uint32_t retVal = wheelBasedMachineDistance_mm;

		// Values above the max are sort of implicitly defined and should be ignored.
		if (wheelBasedMachineDistance_mm > SAEds05_MAX_VALUE)
		{
			retVal = 0;
		}
		return retVal;
	}

	bool SpeedMessagesInterface::WheelBasedMachineSpeedData::set_machine_distance(std::uint32_t distance)
	{
		bool retVal = (distance != wheelBasedMachineDistance_mm);
		wheelBasedMachineDistance_mm = distance;
		return retVal;
	}

	std::uint16_t SpeedMessagesInterface::WheelBasedMachineSpeedData::get_machine_speed() const
	{
		std::uint16_t retVal = wheelBasedMachineSpeed_mm_per_sec;

		if (wheelBasedMachineSpeed_mm_per_sec > SAEvl01_MAX_VALUE)
		{
			retVal = 0;
		}
		return retVal;
	}

	bool SpeedMessagesInterface::WheelBasedMachineSpeedData::set_machine_speed(std::uint16_t speed)
	{
		bool retVal = (speed != wheelBasedMachineSpeed_mm_per_sec);
		wheelBasedMachineSpeed_mm_per_sec = speed;
		return retVal;
	}

	std::uint8_t SpeedMessagesInterface::WheelBasedMachineSpeedData::get_maximum_time_of_tractor_power() const
	{
		return maximumTimeOfTractorPower_min;
	}

	bool SpeedMessagesInterface::WheelBasedMachineSpeedData::set_maximum_time_of_tractor_power(std::uint8_t maxTime)
	{
		bool retVal = (maximumTimeOfTractorPower_min != maxTime);
		maximumTimeOfTractorPower_min = maxTime;
		return retVal;
	}

	SpeedMessagesInterface::MachineDirection SpeedMessagesInterface::WheelBasedMachineSpeedData::get_machine_direction_of_travel() const
	{
		return machineDirectionState;
	}

	bool SpeedMessagesInterface::WheelBasedMachineSpeedData::set_machine_direction_of_travel(MachineDirection direction)
	{
		bool retVal = (machineDirectionState != direction);
		machineDirectionState = direction;
		return retVal;
	}

	SpeedMessagesInterface::WheelBasedMachineSpeedData::KeySwitchState SpeedMessagesInterface::WheelBasedMachineSpeedData::get_key_switch_state() const
	{
		return keySwitchState;
	}

	bool SpeedMessagesInterface::WheelBasedMachineSpeedData::set_key_switch_state(KeySwitchState state)
	{
		bool retVal = (keySwitchState != state);
		keySwitchState = state;
		return retVal;
	}

	SpeedMessagesInterface::WheelBasedMachineSpeedData::ImplementStartStopOperations SpeedMessagesInterface::WheelBasedMachineSpeedData::get_implement_start_stop_operations_state() const
	{
		return implementStartStopOperationsState;
	}

	bool SpeedMessagesInterface::WheelBasedMachineSpeedData::set_implement_start_stop_operations_state(ImplementStartStopOperations state)
	{
		bool retVal = (implementStartStopOperationsState != state);
		implementStartStopOperationsState = state;
		return retVal;
	}

	SpeedMessagesInterface::WheelBasedMachineSpeedData::OperatorDirectionReversed SpeedMessagesInterface::WheelBasedMachineSpeedData::get_operator_direction_reversed_state() const
	{
		return operatorDirectionReversedState;
	}

	bool SpeedMessagesInterface::WheelBasedMachineSpeedData::set_operator_direction_reversed_state(OperatorDirectionReversed reverseState)
	{
		bool retVal = (operatorDirectionReversedState != reverseState);
		operatorDirectionReversedState = reverseState;
		return retVal;
	}

	std::shared_ptr<ControlFunction> SpeedMessagesInterface::WheelBasedMachineSpeedData::get_sender_control_function() const
	{
		return controlFunction;
	}

	void SpeedMessagesInterface::WheelBasedMachineSpeedData::set_timestamp_ms(std::uint32_t timestamp)
	{
		timestamp_ms = timestamp;
	}

	std::uint32_t SpeedMessagesInterface::WheelBasedMachineSpeedData::get_timestamp_ms() const
	{
		return timestamp_ms;
	}

	SpeedMessagesInterface::MachineSelectedSpeedData::MachineSelectedSpeedData(std::shared_ptr<ControlFunction> sender) :
	  controlFunction(sender)
	{
	}

	std::uint32_t SpeedMessagesInterface::MachineSelectedSpeedData::get_machine_distance() const
	{
		std::uint32_t retVal = machineSelectedSpeedDistance_mm;

		// Values above the max are sort of implicitly defined and should be ignored.
		if (machineSelectedSpeedDistance_mm > SAEds05_MAX_VALUE)
		{
			retVal = 0;
		}
		return retVal;
	}

	bool SpeedMessagesInterface::MachineSelectedSpeedData::set_machine_distance(std::uint32_t distance)
	{
		bool retVal = (machineSelectedSpeedDistance_mm != distance);
		machineSelectedSpeedDistance_mm = distance;
		return retVal;
	}

	std::uint16_t SpeedMessagesInterface::MachineSelectedSpeedData::get_machine_speed() const
	{
		std::uint16_t retVal = machineSelectedSpeed_mm_per_sec;

		if (machineSelectedSpeed_mm_per_sec > SAEvl01_MAX_VALUE)
		{
			retVal = 0;
		}
		return retVal;
	}

	bool SpeedMessagesInterface::MachineSelectedSpeedData::set_machine_speed(std::uint16_t speed)
	{
		bool retVal = (speed != machineSelectedSpeed_mm_per_sec);
		machineSelectedSpeed_mm_per_sec = speed;
		return retVal;
	}

	std::uint8_t SpeedMessagesInterface::MachineSelectedSpeedData::get_exit_reason_code() const
	{
		return exitReasonCode;
	}

	bool SpeedMessagesInterface::MachineSelectedSpeedData::set_exit_reason_code(std::uint8_t exitCode)
	{
		bool retVal = (exitCode != exitReasonCode);
		exitReasonCode = exitCode;
		return retVal;
	}

	SpeedMessagesInterface::MachineSelectedSpeedData::SpeedSource SpeedMessagesInterface::MachineSelectedSpeedData::get_speed_source() const
	{
		return source;
	}

	bool SpeedMessagesInterface::MachineSelectedSpeedData::set_speed_source(SpeedSource selectedSource)
	{
		bool retVal = (source != selectedSource);
		source = selectedSource;
		return retVal;
	}

	SpeedMessagesInterface::MachineSelectedSpeedData::LimitStatus SpeedMessagesInterface::MachineSelectedSpeedData::get_limit_status() const
	{
		return limitStatus;
	}

	bool SpeedMessagesInterface::MachineSelectedSpeedData::set_limit_status(LimitStatus statusToSet)
	{
		bool retVal = (limitStatus != statusToSet);
		limitStatus = statusToSet;
		return retVal;
	}

	SpeedMessagesInterface::MachineDirection SpeedMessagesInterface::MachineSelectedSpeedData::get_machine_direction_of_travel() const
	{
		return machineDirectionState;
	}

	bool SpeedMessagesInterface::MachineSelectedSpeedData::set_machine_direction_of_travel(MachineDirection directionOfTravel)
	{
		bool retVal = (directionOfTravel != machineDirectionState);
		machineDirectionState = directionOfTravel;
		return retVal;
	}

	std::shared_ptr<ControlFunction> SpeedMessagesInterface::MachineSelectedSpeedData::get_sender_control_function() const
	{
		return controlFunction;
	}

	void SpeedMessagesInterface::MachineSelectedSpeedData::set_timestamp_ms(std::uint32_t timestamp)
	{
		timestamp_ms = timestamp;
	}

	std::uint32_t SpeedMessagesInterface::MachineSelectedSpeedData::get_timestamp_ms() const
	{
		return timestamp_ms;
	}

	SpeedMessagesInterface::GroundBasedSpeedData::GroundBasedSpeedData(std::shared_ptr<ControlFunction> sender) :
	  controlFunction(sender)
	{
	}

	std::uint32_t SpeedMessagesInterface::GroundBasedSpeedData::get_machine_distance() const
	{
		std::uint32_t retVal = groundBasedMachineDistance_mm;

		// Values above the max are sort of implicitly defined and should be ignored.
		if (groundBasedMachineDistance_mm > SAEds05_MAX_VALUE)
		{
			retVal = 0;
		}
		return retVal;
	}

	bool SpeedMessagesInterface::GroundBasedSpeedData::set_machine_distance(std::uint32_t distance)
	{
		bool retVal = (distance != groundBasedMachineDistance_mm);
		groundBasedMachineDistance_mm = distance;
		return retVal;
	}

	std::uint16_t SpeedMessagesInterface::GroundBasedSpeedData::get_machine_speed() const
	{
		std::uint16_t retVal = groundBasedMachineSpeed_mm_per_sec;

		if (groundBasedMachineSpeed_mm_per_sec > SAEvl01_MAX_VALUE)
		{
			retVal = 0;
		}
		return retVal;
	}

	bool SpeedMessagesInterface::GroundBasedSpeedData::set_machine_speed(std::uint16_t speed)
	{
		bool retVal = (speed != groundBasedMachineSpeed_mm_per_sec);
		groundBasedMachineSpeed_mm_per_sec = speed;
		return retVal;
	}

	SpeedMessagesInterface::MachineDirection SpeedMessagesInterface::GroundBasedSpeedData::get_machine_direction_of_travel() const
	{
		return machineDirectionState;
	}

	bool SpeedMessagesInterface::GroundBasedSpeedData::set_machine_direction_of_travel(MachineDirection directionOfTravel)
	{
		bool retVal = (directionOfTravel != machineDirectionState);
		machineDirectionState = directionOfTravel;
		return retVal;
	}

	std::shared_ptr<ControlFunction> SpeedMessagesInterface::GroundBasedSpeedData::get_sender_control_function() const
	{
		return controlFunction;
	}

	void SpeedMessagesInterface::GroundBasedSpeedData::set_timestamp_ms(std::uint32_t timestamp)
	{
		timestamp_ms = timestamp;
	}

	std::uint32_t SpeedMessagesInterface::GroundBasedSpeedData::get_timestamp_ms() const
	{
		return timestamp_ms;
	}

	SpeedMessagesInterface::MachineSelectedSpeedCommandData::MachineSelectedSpeedCommandData(std::shared_ptr<ControlFunction> sender) :
	  controlFunction(sender)
	{
	}

	std::uint16_t SpeedMessagesInterface::MachineSelectedSpeedCommandData::get_machine_speed_setpoint_command() const
	{
		std::uint16_t retVal = speedCommandedSetpoint;

		// Values over the max are implicitly defined, best to treat as zeros.
		if (speedCommandedSetpoint > SAEvl01_MAX_VALUE)
		{
			retVal = 0;
		}
		return retVal;
	}

	bool SpeedMessagesInterface::MachineSelectedSpeedCommandData::set_machine_speed_setpoint_command(std::uint16_t speed)
	{
		bool retVal = (speed != speedCommandedSetpoint);
		speedCommandedSetpoint = speed;
		return retVal;
	}

	std::uint16_t SpeedMessagesInterface::MachineSelectedSpeedCommandData::get_machine_selected_speed_setpoint_limit() const
	{
		std::uint16_t retVal = speedSetpointLimit;

		// Values over the max are implicitly defined, best to treat as zeros.
		if (speedSetpointLimit > SAEvl01_MAX_VALUE)
		{
			retVal = 0;
		}
		return retVal;
	}

	bool SpeedMessagesInterface::MachineSelectedSpeedCommandData::set_machine_selected_speed_setpoint_limit(std::uint16_t speedLimit)
	{
		bool retVal = (speedSetpointLimit != speedLimit);
		speedSetpointLimit = speedLimit;
		return retVal;
	}

	SpeedMessagesInterface::MachineDirection SpeedMessagesInterface::MachineSelectedSpeedCommandData::get_machine_direction_command() const
	{
		return machineDirectionCommand;
	}

	bool SpeedMessagesInterface::MachineSelectedSpeedCommandData::set_machine_direction_of_travel(MachineDirection commandedDirection)
	{
		bool retVal = (commandedDirection != machineDirectionCommand);
		machineDirectionCommand = commandedDirection;
		return retVal;
	}

	std::shared_ptr<ControlFunction> SpeedMessagesInterface::MachineSelectedSpeedCommandData::get_sender_control_function() const
	{
		return controlFunction;
	}

	void SpeedMessagesInterface::MachineSelectedSpeedCommandData::set_timestamp_ms(std::uint32_t timestamp)
	{
		timestamp_ms = timestamp;
	}

	std::uint32_t SpeedMessagesInterface::MachineSelectedSpeedCommandData::get_timestamp_ms() const
	{
		return timestamp_ms;
	}

	void SpeedMessagesInterface::initialize()
	{
		if (!initialized)
		{
			if (nullptr != machineSelectedSpeedCommandTransmitData.get_sender_control_function())
			{
				LOG_WARNING("[Speed/Distance]: Use extreme cation! You have configured an interface to command the speed of the machine. The machine may move without warning!");
			}
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::MachineSelectedSpeed), process_rx_message, this);
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::WheelBasedSpeedAndDistance), process_rx_message, this);
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::GroundBasedSpeedAndDistance), process_rx_message, this);
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::MachineSelectedSpeedCommand), process_rx_message, this);
			initialized = true;
		}
	}

	bool SpeedMessagesInterface::get_initialized() const
	{
		return initialized;
	}

	std::size_t SpeedMessagesInterface::get_number_received_wheel_based_speed_sources() const
	{
		return receivedWheelBasedSpeedMessages.size();
	}

	std::size_t SpeedMessagesInterface::get_number_received_ground_based_speed_sources() const
	{
		return receivedGroundBasedSpeedMessages.size();
	}

	std::size_t SpeedMessagesInterface::get_number_received_machine_selected_speed_sources() const
	{
		return receivedMachineSelectedSpeedMessages.size();
	}

	std::size_t SpeedMessagesInterface::get_number_received_machine_selected_speed_command_sources() const
	{
		return receivedMachineSelectedSpeedCommandMessages.size();
	}

	std::shared_ptr<SpeedMessagesInterface::MachineSelectedSpeedData> SpeedMessagesInterface::get_received_machine_selected_speed(std::size_t index)
	{
		std::shared_ptr<MachineSelectedSpeedData> retVal = nullptr;

		if (index < receivedMachineSelectedSpeedMessages.size())
		{
			retVal = receivedMachineSelectedSpeedMessages.at(index);
		}
		return retVal;
	}

	std::shared_ptr<SpeedMessagesInterface::WheelBasedMachineSpeedData> SpeedMessagesInterface::get_received_wheel_based_speed(std::size_t index)
	{
		std::shared_ptr<WheelBasedMachineSpeedData> retVal = nullptr;

		if (index < receivedWheelBasedSpeedMessages.size())
		{
			retVal = receivedWheelBasedSpeedMessages.at(index);
		}
		return retVal;
	}

	std::shared_ptr<SpeedMessagesInterface::GroundBasedSpeedData> SpeedMessagesInterface::get_received_ground_based_speed(std::size_t index)
	{
		std::shared_ptr<GroundBasedSpeedData> retVal = nullptr;

		if (index < receivedGroundBasedSpeedMessages.size())
		{
			retVal = receivedGroundBasedSpeedMessages.at(index);
		}
		return retVal;
	}

	std::shared_ptr<SpeedMessagesInterface::MachineSelectedSpeedCommandData> SpeedMessagesInterface::get_received_machine_selected_speed_command(std::size_t index)
	{
		std::shared_ptr<MachineSelectedSpeedCommandData> retVal = nullptr;

		if (index < receivedMachineSelectedSpeedCommandMessages.size())
		{
			retVal = receivedMachineSelectedSpeedCommandMessages.at(index);
		}
		return retVal;
	}

	EventDispatcher<const std::shared_ptr<SpeedMessagesInterface::WheelBasedMachineSpeedData>, bool> &SpeedMessagesInterface::get_wheel_based_machine_speed_data_event_publisher()
	{
		return wheelBasedMachineSpeedDataEventPublisher;
	}

	EventDispatcher<const std::shared_ptr<SpeedMessagesInterface::MachineSelectedSpeedData>, bool> &SpeedMessagesInterface::get_machine_selected_speed_data_event_publisher()
	{
		return machineSelectedSpeedDataEventPublisher;
	}

	EventDispatcher<const std::shared_ptr<SpeedMessagesInterface::GroundBasedSpeedData>, bool> &SpeedMessagesInterface::get_ground_based_machine_speed_data_event_publisher()
	{
		return groundBasedSpeedDataEventPublisher;
	}

	EventDispatcher<const std::shared_ptr<SpeedMessagesInterface::MachineSelectedSpeedCommandData>, bool> &SpeedMessagesInterface::get_machine_selected_speed_command_data_event_publisher()
	{
		return machineSelectedSpeedCommandDataEventPublisher;
	}

	void SpeedMessagesInterface::update()
	{
		if (initialized)
		{
			receivedMachineSelectedSpeedMessages.erase(std::remove_if(receivedMachineSelectedSpeedMessages.begin(),
			                                                          receivedMachineSelectedSpeedMessages.end(),
			                                                          [](std::shared_ptr<MachineSelectedSpeedData> messageInfo) {
				                                                          return SystemTiming::time_expired_ms(messageInfo->get_timestamp_ms(), SPEED_DISTANCE_MESSAGE_RX_TIMEOUT_MS);
			                                                          }),
			                                           receivedMachineSelectedSpeedMessages.end());
			receivedWheelBasedSpeedMessages.erase(std::remove_if(receivedWheelBasedSpeedMessages.begin(),
			                                                     receivedWheelBasedSpeedMessages.end(),
			                                                     [](std::shared_ptr<WheelBasedMachineSpeedData> messageInfo) {
				                                                     return SystemTiming::time_expired_ms(messageInfo->get_timestamp_ms(), SPEED_DISTANCE_MESSAGE_RX_TIMEOUT_MS);
			                                                     }),
			                                      receivedWheelBasedSpeedMessages.end());
			receivedGroundBasedSpeedMessages.erase(std::remove_if(receivedGroundBasedSpeedMessages.begin(),
			                                                      receivedGroundBasedSpeedMessages.end(),
			                                                      [](std::shared_ptr<GroundBasedSpeedData> messageInfo) {
				                                                      return SystemTiming::time_expired_ms(messageInfo->get_timestamp_ms(), SPEED_DISTANCE_MESSAGE_RX_TIMEOUT_MS);
			                                                      }),
			                                       receivedGroundBasedSpeedMessages.end());
			receivedMachineSelectedSpeedCommandMessages.erase(std::remove_if(receivedMachineSelectedSpeedCommandMessages.begin(),
			                                                                 receivedMachineSelectedSpeedCommandMessages.end(),
			                                                                 [](std::shared_ptr<MachineSelectedSpeedCommandData> messageInfo) {
				                                                                 return SystemTiming::time_expired_ms(messageInfo->get_timestamp_ms(), SPEED_DISTANCE_MESSAGE_RX_TIMEOUT_MS);
			                                                                 }),
			                                                  receivedMachineSelectedSpeedCommandMessages.end());

			if (SystemTiming::time_expired_ms(machineSelectedSpeedTransmitTimestamp_ms, SPEED_DISTANCE_MESSAGE_TX_INTERVAL_MS) &&
			    (nullptr != machineSelectedSpeedTransmitData.get_sender_control_function()))
			{
				txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendMachineSelectedSpeed));
				machineSelectedSpeedTransmitTimestamp_ms = SystemTiming::get_timestamp_ms();
			}
			if (SystemTiming::time_expired_ms(wheelBasedSpeedTransmitTimestamp_ms, SPEED_DISTANCE_MESSAGE_TX_INTERVAL_MS) &&
			    (nullptr != wheelBasedSpeedTransmitData.get_sender_control_function()))
			{
				txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendWheelBasedSpeed));
				wheelBasedSpeedTransmitTimestamp_ms = SystemTiming::get_timestamp_ms();
			}
			if (SystemTiming::time_expired_ms(groundBasedSpeedTransmitTimestamp_ms, SPEED_DISTANCE_MESSAGE_TX_INTERVAL_MS) &&
			    (nullptr != groundBasedSpeedTransmitData.get_sender_control_function()))
			{
				txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendGroundBasedSpeed));
				groundBasedSpeedTransmitTimestamp_ms = SystemTiming::get_timestamp_ms();
			}
			if (SystemTiming::time_expired_ms(machineSelectedSpeedCommandTransmitTimestamp_ms, SPEED_DISTANCE_MESSAGE_TX_INTERVAL_MS) &&
			    (nullptr != machineSelectedSpeedCommandTransmitData.get_sender_control_function()))
			{
				txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendMachineSelectedSpeedCommand));
				machineSelectedSpeedCommandTransmitTimestamp_ms = SystemTiming::get_timestamp_ms();
			}
			txFlags.process_all_flags();
		}
		else
		{
			LOG_ERROR("[Speed/Distance]: ISOBUS speed messages interface has not been initialized yet.");
		}
	}

	void SpeedMessagesInterface::process_flags(std::uint32_t flag, void *parentPointer)
	{
		if (nullptr != parentPointer)
		{
			auto targetInterface = static_cast<SpeedMessagesInterface *>(parentPointer);
			bool transmitSuccessful = false;

			switch (flag)
			{
				case static_cast<std::uint32_t>(TransmitFlags::SendMachineSelectedSpeed):
				{
					transmitSuccessful = targetInterface->send_machine_selected_speed();
				}
				break;

				case static_cast<std::uint32_t>(TransmitFlags::SendWheelBasedSpeed):
				{
					transmitSuccessful = targetInterface->send_wheel_based_speed();
				}
				break;

				case static_cast<std::uint32_t>(TransmitFlags::SendGroundBasedSpeed):
				{
					transmitSuccessful = targetInterface->send_ground_based_speed();
				}
				break;

				case static_cast<std::uint32_t>(TransmitFlags::SendMachineSelectedSpeedCommand):
				{
					transmitSuccessful = targetInterface->send_machine_selected_speed_command();
				}
				break;

				default:
					break;
			}

			if (false == transmitSuccessful)
			{
				targetInterface->txFlags.set_flag(flag);
			}
		}
	}

	void SpeedMessagesInterface::process_rx_message(const CANMessage &message, void *parentPointer)
	{
		assert(nullptr != parentPointer);
		auto targetInterface = static_cast<SpeedMessagesInterface *>(parentPointer);

		switch (message.get_identifier().get_parameter_group_number())
		{
			case static_cast<std::uint32_t>(CANLibParameterGroupNumber::MachineSelectedSpeed):
			{
				if (CAN_DATA_LENGTH == message.get_data_length())
				{
					if (nullptr != message.get_source_control_function())
					{
						auto result = std::find_if(targetInterface->receivedMachineSelectedSpeedMessages.cbegin(),
						                           targetInterface->receivedMachineSelectedSpeedMessages.cend(),
						                           [&message](const std::shared_ptr<MachineSelectedSpeedData> &receivedInfo) {
							                           return (nullptr != receivedInfo) && (receivedInfo->get_sender_control_function() == message.get_source_control_function());
						                           });

						if (result == targetInterface->receivedMachineSelectedSpeedMessages.end())
						{
							// There is no existing message object from this control function, so create a new one
							targetInterface->receivedMachineSelectedSpeedMessages.push_back(std::make_shared<MachineSelectedSpeedData>(message.get_source_control_function()));
							result = targetInterface->receivedMachineSelectedSpeedMessages.end() - 1;
						}

						auto &mssMessage = *result;
						bool changed = false;

						changed |= mssMessage->set_machine_speed(message.get_uint16_at(0));
						changed |= mssMessage->set_machine_distance(message.get_uint32_at(2));
						changed |= mssMessage->set_exit_reason_code(message.get_uint8_at(6) & 0x3F);
						changed |= mssMessage->set_machine_direction_of_travel(static_cast<MachineDirection>(message.get_uint8_at(7) & 0x03));
						changed |= mssMessage->set_speed_source(static_cast<MachineSelectedSpeedData::SpeedSource>((message.get_uint8_at(7) >> 2) & 0x07));
						changed |= mssMessage->set_limit_status(static_cast<MachineSelectedSpeedData::LimitStatus>((message.get_uint8_at(7) >> 5) & 0x03));
						mssMessage->set_timestamp_ms(SystemTiming::get_timestamp_ms());

						targetInterface->machineSelectedSpeedDataEventPublisher.call(mssMessage, changed);
					}
				}
				else
				{
					LOG_ERROR("[Speed/Distance]: Received a malformed machine selected speed. DLC must be 8.");
				}
			}
			break;

			case static_cast<std::uint32_t>(CANLibParameterGroupNumber::WheelBasedSpeedAndDistance):
			{
				if (CAN_DATA_LENGTH == message.get_data_length())
				{
					if (nullptr != message.get_source_control_function())
					{
						auto result = std::find_if(targetInterface->receivedWheelBasedSpeedMessages.cbegin(),
						                           targetInterface->receivedWheelBasedSpeedMessages.cend(),
						                           [&message](const std::shared_ptr<WheelBasedMachineSpeedData> &receivedInfo) {
							                           return (nullptr != receivedInfo) && (receivedInfo->get_sender_control_function() == message.get_source_control_function());
						                           });

						if (result == targetInterface->receivedWheelBasedSpeedMessages.end())
						{
							// There is no existing message object from this control function, so create a new one
							targetInterface->receivedWheelBasedSpeedMessages.push_back(std::make_shared<WheelBasedMachineSpeedData>(message.get_source_control_function()));
							result = targetInterface->receivedWheelBasedSpeedMessages.end() - 1;
						}

						auto &wheelSpeedMessage = *result;
						bool changed = false;

						changed |= wheelSpeedMessage->set_machine_speed(message.get_uint16_at(0));
						changed |= wheelSpeedMessage->set_machine_distance(message.get_uint32_at(2));
						changed |= wheelSpeedMessage->set_maximum_time_of_tractor_power(message.get_uint8_at(6));
						changed |= wheelSpeedMessage->set_machine_direction_of_travel(static_cast<MachineDirection>(message.get_uint8_at(7) & 0x03));
						changed |= wheelSpeedMessage->set_key_switch_state(static_cast<WheelBasedMachineSpeedData::KeySwitchState>((message.get_uint8_at(7) >> 2) & 0x03));
						changed |= wheelSpeedMessage->set_implement_start_stop_operations_state(static_cast<WheelBasedMachineSpeedData::ImplementStartStopOperations>((message.get_uint8_at(7) >> 4) & 0x03));
						changed |= wheelSpeedMessage->set_operator_direction_reversed_state(static_cast<WheelBasedMachineSpeedData::OperatorDirectionReversed>((message.get_uint8_at(7) >> 6) & 0x03));
						wheelSpeedMessage->set_timestamp_ms(SystemTiming::get_timestamp_ms());

						targetInterface->wheelBasedMachineSpeedDataEventPublisher.call(wheelSpeedMessage, changed);
					}
				}
				else
				{
					LOG_ERROR("[Speed/Distance]: Received a malformed wheel-based speed and distance message. DLC must be 8.");
				}
			}
			break;

			case static_cast<std::uint32_t>(CANLibParameterGroupNumber::GroundBasedSpeedAndDistance):
			{
				if (CAN_DATA_LENGTH == message.get_data_length())
				{
					if (nullptr != message.get_source_control_function())
					{
						auto result = std::find_if(targetInterface->receivedGroundBasedSpeedMessages.cbegin(),
						                           targetInterface->receivedGroundBasedSpeedMessages.cend(),
						                           [&message](const std::shared_ptr<GroundBasedSpeedData> &receivedInfo) {
							                           return (nullptr != receivedInfo) && (receivedInfo->get_sender_control_function() == message.get_source_control_function());
						                           });

						if (result == targetInterface->receivedGroundBasedSpeedMessages.end())
						{
							// There is no existing message object from this control function, so create a new one
							targetInterface->receivedGroundBasedSpeedMessages.push_back(std::make_shared<GroundBasedSpeedData>(message.get_source_control_function()));
							result = targetInterface->receivedGroundBasedSpeedMessages.end() - 1;
						}

						auto &groundSpeedMessage = *result;
						bool changed = false;

						changed |= groundSpeedMessage->set_machine_speed(message.get_uint16_at(0));
						changed |= groundSpeedMessage->set_machine_distance(message.get_uint32_at(2));
						changed |= groundSpeedMessage->set_machine_direction_of_travel(static_cast<MachineDirection>(message.get_uint8_at(7) & 0x03));
						groundSpeedMessage->set_timestamp_ms(SystemTiming::get_timestamp_ms());

						targetInterface->groundBasedSpeedDataEventPublisher.call(groundSpeedMessage, changed);
					}
				}
				else
				{
					LOG_ERROR("[Speed/Distance]: Received a malformed ground-based speed and distance message. DLC must be 8.");
				}
			}
			break;

			case static_cast<std::uint32_t>(CANLibParameterGroupNumber::MachineSelectedSpeedCommand):
			{
				if (CAN_DATA_LENGTH == message.get_data_length())
				{
					if (nullptr != message.get_source_control_function())
					{
						auto result = std::find_if(targetInterface->receivedMachineSelectedSpeedCommandMessages.cbegin(),
						                           targetInterface->receivedMachineSelectedSpeedCommandMessages.cend(),
						                           [&message](const std::shared_ptr<MachineSelectedSpeedCommandData> &receivedInfo) {
							                           return (nullptr != receivedInfo) && (receivedInfo->get_sender_control_function() == message.get_source_control_function());
						                           });

						if (result == targetInterface->receivedMachineSelectedSpeedCommandMessages.end())
						{
							// There is no existing message object from this control function, so create a new one
							targetInterface->receivedMachineSelectedSpeedCommandMessages.push_back(std::make_shared<MachineSelectedSpeedCommandData>(message.get_source_control_function()));
							result = targetInterface->receivedMachineSelectedSpeedCommandMessages.end() - 1;
						}

						auto &commandMessage = *result;
						bool changed = false;

						commandMessage->set_machine_speed_setpoint_command(message.get_uint16_at(0));
						commandMessage->set_machine_selected_speed_setpoint_limit(message.get_uint16_at(2));
						commandMessage->set_machine_direction_of_travel(static_cast<MachineDirection>(message.get_uint8_at(7) & 0x03));
						commandMessage->set_timestamp_ms(SystemTiming::get_timestamp_ms());

						targetInterface->machineSelectedSpeedCommandDataEventPublisher.call(commandMessage, changed);
					}
				}
				else
				{
					LOG_ERROR("[Speed/Distance]: Received a malformed machine selected speed command message. DLC must be 8.");
				}
			}
			break;

			default:
				break;
		}
	}

	bool SpeedMessagesInterface::send_machine_selected_speed() const
	{
		bool retVal = false;

		if (nullptr != machineSelectedSpeedTransmitData.get_sender_control_function())
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { static_cast<std::uint8_t>(machineSelectedSpeedTransmitData.get_machine_speed() & 0xFF),
				                                                   static_cast<std::uint8_t>((machineSelectedSpeedTransmitData.get_machine_speed() >> 8) & 0xFF),
				                                                   static_cast<std::uint8_t>(machineSelectedSpeedTransmitData.get_machine_distance() & 0xFF),
				                                                   static_cast<std::uint8_t>((machineSelectedSpeedTransmitData.get_machine_distance() >> 8) & 0xFF),
				                                                   static_cast<std::uint8_t>((machineSelectedSpeedTransmitData.get_machine_distance() >> 16) & 0xFF),
				                                                   static_cast<std::uint8_t>((machineSelectedSpeedTransmitData.get_machine_distance() >> 24) & 0xFF),
				                                                   static_cast<std::uint8_t>(0xC0 | static_cast<std::uint8_t>(machineSelectedSpeedTransmitData.get_exit_reason_code() & 0x7F)), // 0xC0 sets reserved bits to 1s
				                                                   static_cast<std::uint8_t>(static_cast<std::uint8_t>(machineSelectedSpeedTransmitData.get_machine_direction_of_travel()) |
				                                                                             (static_cast<std::uint8_t>(machineSelectedSpeedTransmitData.get_speed_source()) << 2) |
				                                                                             (static_cast<std::uint8_t>(machineSelectedSpeedTransmitData.get_limit_status()) << 5))

			};
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::MachineSelectedSpeed),
			                                                        buffer.data(),
			                                                        buffer.size(),
			                                                        std::static_pointer_cast<InternalControlFunction>(machineSelectedSpeedTransmitData.get_sender_control_function()),
			                                                        nullptr,
			                                                        CANIdentifier::CANPriority::Priority3);
		}
		return retVal;
	}

	bool SpeedMessagesInterface::send_wheel_based_speed() const
	{
		bool retVal = false;

		if (nullptr != wheelBasedSpeedTransmitData.get_sender_control_function())
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { static_cast<std::uint8_t>(wheelBasedSpeedTransmitData.get_machine_speed() & 0xFF),
				                                                   static_cast<std::uint8_t>((wheelBasedSpeedTransmitData.get_machine_speed() >> 8) & 0xFF),
				                                                   static_cast<std::uint8_t>(wheelBasedSpeedTransmitData.get_machine_distance() & 0xFF),
				                                                   static_cast<std::uint8_t>((wheelBasedSpeedTransmitData.get_machine_distance() >> 8) & 0xFF),
				                                                   static_cast<std::uint8_t>((wheelBasedSpeedTransmitData.get_machine_distance() >> 16) & 0xFF),
				                                                   static_cast<std::uint8_t>((wheelBasedSpeedTransmitData.get_machine_distance() >> 24) & 0xFF),
				                                                   wheelBasedSpeedTransmitData.get_maximum_time_of_tractor_power(),
				                                                   static_cast<std::uint8_t>(static_cast<std::uint8_t>(wheelBasedSpeedTransmitData.get_machine_direction_of_travel()) |
				                                                                             (static_cast<std::uint8_t>(wheelBasedSpeedTransmitData.get_key_switch_state()) << 2) |
				                                                                             (static_cast<std::uint8_t>(wheelBasedSpeedTransmitData.get_implement_start_stop_operations_state()) << 4) |
				                                                                             (static_cast<std::uint8_t>(wheelBasedSpeedTransmitData.get_operator_direction_reversed_state()) << 6)) };
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::WheelBasedSpeedAndDistance),
			                                                        buffer.data(),
			                                                        buffer.size(),
			                                                        std::static_pointer_cast<InternalControlFunction>(wheelBasedSpeedTransmitData.get_sender_control_function()),
			                                                        nullptr,
			                                                        CANIdentifier::CANPriority::Priority3);
		}
		return retVal;
	}

	bool SpeedMessagesInterface::send_ground_based_speed() const
	{
		bool retVal = false;

		if (nullptr != groundBasedSpeedTransmitData.get_sender_control_function())
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { static_cast<std::uint8_t>(groundBasedSpeedTransmitData.get_machine_speed() & 0xFF),
				                                                   static_cast<std::uint8_t>((groundBasedSpeedTransmitData.get_machine_speed() >> 8) & 0xFF),
				                                                   static_cast<std::uint8_t>(groundBasedSpeedTransmitData.get_machine_distance() & 0xFF),
				                                                   static_cast<std::uint8_t>((groundBasedSpeedTransmitData.get_machine_distance() >> 8) & 0xFF),
				                                                   static_cast<std::uint8_t>((groundBasedSpeedTransmitData.get_machine_distance() >> 16) & 0xFF),
				                                                   static_cast<std::uint8_t>((groundBasedSpeedTransmitData.get_machine_distance() >> 24) & 0xFF),
				                                                   0xFF, // Reserved
				                                                   static_cast<std::uint8_t>(0xFC | static_cast<std::uint8_t>(groundBasedSpeedTransmitData.get_machine_direction_of_travel())) }; // 0xFC sets reserved bits to 1s
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::GroundBasedSpeedAndDistance),
			                                                        buffer.data(),
			                                                        buffer.size(),
			                                                        std::static_pointer_cast<InternalControlFunction>(groundBasedSpeedTransmitData.get_sender_control_function()),
			                                                        nullptr,
			                                                        CANIdentifier::CANPriority::Priority3);
		}
		return retVal;
	}

	bool SpeedMessagesInterface::send_machine_selected_speed_command() const
	{
		bool retVal = false;

		if (nullptr != machineSelectedSpeedCommandTransmitData.get_sender_control_function())
		{
			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { static_cast<std::uint8_t>(machineSelectedSpeedCommandTransmitData.get_machine_speed_setpoint_command() & 0xFF),
				                                                   static_cast<std::uint8_t>((machineSelectedSpeedCommandTransmitData.get_machine_speed_setpoint_command() >> 8) & 0xFF),
				                                                   static_cast<std::uint8_t>(machineSelectedSpeedCommandTransmitData.get_machine_selected_speed_setpoint_limit() & 0xFF),
				                                                   static_cast<std::uint8_t>((machineSelectedSpeedCommandTransmitData.get_machine_selected_speed_setpoint_limit() >> 8) & 0xFF),
				                                                   0xFF,
				                                                   0xFF,
				                                                   0xFF,
				                                                   static_cast<std::uint8_t>(0xFC | static_cast<std::uint8_t>(machineSelectedSpeedCommandTransmitData.get_machine_direction_command())) };
			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::MachineSelectedSpeedCommand),
			                                                        buffer.data(),
			                                                        buffer.size(),
			                                                        std::static_pointer_cast<InternalControlFunction>(machineSelectedSpeedCommandTransmitData.get_sender_control_function()),
			                                                        nullptr,
			                                                        CANIdentifier::CANPriority::Priority3);
		}
		return retVal;
	}

} // namespace isobus
