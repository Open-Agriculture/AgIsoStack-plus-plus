//================================================================================================
/// @file isobus_maintain_power_interface.cpp
///
/// @brief Implements an interface for sending and receiving the maintain power message (PGN 65095).
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================

#include "isobus/isobus/isobus_maintain_power_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"

#include <algorithm>
#include <array>
#include <cassert>

namespace isobus
{
	MaintainPowerInterface::MaintainPowerInterface(std::shared_ptr<InternalControlFunction> sourceControlFunction) :
	  maintainPowerTransmitData(sourceControlFunction),
	  txFlags(static_cast<std::uint32_t>(TransmitFlags::NumberOfFlags), process_flags, this)
	{
	}

	MaintainPowerInterface::~MaintainPowerInterface()
	{
		if (initialized)
		{
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::MaintainPower), process_rx_message, this);
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::WheelBasedSpeedAndDistance), process_rx_message, this);
		}
	}

	EventDispatcher<const std::shared_ptr<MaintainPowerInterface::MaintainPowerData>, bool> &MaintainPowerInterface::get_maintain_power_data_event_publisher()
	{
		return maintainPowerDataEventPublisher;
	}

	EventDispatcher<> &MaintainPowerInterface::get_key_switch_transition_off_event_publisher()
	{
		return keySwitchOffEventPublisher;
	}

	void MaintainPowerInterface::update()
	{
		if (initialized)
		{
			receivedMaintainPowerMessages.erase(std::remove_if(receivedMaintainPowerMessages.begin(),
			                                                   receivedMaintainPowerMessages.end(),
			                                                   [](std::shared_ptr<MaintainPowerData> messageInfo) {
				                                                   return SystemTiming::time_expired_ms(messageInfo->get_timestamp_ms(), MAINTAIN_POWER_TIMEOUT_MS);
			                                                   }),
			                                    receivedMaintainPowerMessages.end());
			if (SystemTiming::time_expired_ms(maintainPowerTransmitTimestamp_ms, MAINTAIN_POWER_TIMEOUT_MS / 2) &&
			    (0 != maintainPowerTransmitTimestamp_ms) &&
			    (SystemTiming::get_time_elapsed_ms(keyOffTimestamp) < maintainPowerTime_ms) &&
			    (nullptr != maintainPowerTransmitData.get_sender_control_function()))
			{
				txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendMaintainPower));
				maintainPowerTransmitTimestamp_ms = SystemTiming::get_timestamp_ms();
			}
			txFlags.process_all_flags();
		}
		else
		{
			LOG_ERROR("[Maintain Power]: Interface has not been initialized yet.");
		}
	}

	MaintainPowerInterface::MaintainPowerData::MaintainPowerData(std::shared_ptr<ControlFunction> sendingControlFunction) :
	  sendingControlFunction(sendingControlFunction)
	{
	}

	bool MaintainPowerInterface::MaintainPowerData::set_implement_in_work_state(ImplementInWorkState inWorkState)
	{
		bool retVal = (inWorkState != currentImplementInWorkState);
		currentImplementInWorkState = inWorkState;
		return retVal;
	}

	MaintainPowerInterface::MaintainPowerData::ImplementInWorkState MaintainPowerInterface::MaintainPowerData::get_implement_in_work_state() const
	{
		return currentImplementInWorkState;
	}

	bool MaintainPowerInterface::MaintainPowerData::set_implement_ready_to_work_state(ImplementReadyToWorkState readyToWorkState)
	{
		bool retVal = (readyToWorkState != currentImplementReadyToWorkState);
		currentImplementReadyToWorkState = readyToWorkState;
		return retVal;
	}

	MaintainPowerInterface::MaintainPowerData::ImplementReadyToWorkState MaintainPowerInterface::MaintainPowerData::get_implement_ready_to_work_state() const
	{
		return currentImplementReadyToWorkState;
	}

	bool MaintainPowerInterface::MaintainPowerData::set_implement_park_state(ImplementParkState parkState)
	{
		bool retVal = (parkState != currentImplementParkState);
		currentImplementParkState = parkState;
		return retVal;
	}

	MaintainPowerInterface::MaintainPowerData::ImplementParkState MaintainPowerInterface::MaintainPowerData::get_implement_park_state() const
	{
		return currentImplementParkState;
	}

	bool MaintainPowerInterface::MaintainPowerData::set_implement_transport_state(ImplementTransportState transportState)
	{
		bool retVal = (transportState != currentImplementTransportState);
		currentImplementTransportState = transportState;
		return retVal;
	}

	MaintainPowerInterface::MaintainPowerData::ImplementTransportState MaintainPowerInterface::MaintainPowerData::get_implement_transport_state() const
	{
		return currentImplementTransportState;
	}

	bool MaintainPowerInterface::MaintainPowerData::set_maintain_actuator_power(MaintainActuatorPower maintainState)
	{
		bool retVal = (currentMaintainActuatorPowerState != maintainState);
		currentMaintainActuatorPowerState = maintainState;
		return retVal;
	}

	MaintainPowerInterface::MaintainPowerData::MaintainActuatorPower MaintainPowerInterface::MaintainPowerData::get_maintain_actuator_power() const
	{
		return currentMaintainActuatorPowerState;
	}

	bool MaintainPowerInterface::MaintainPowerData::set_maintain_ecu_power(MaintainECUPower maintainState)
	{
		bool retVal = (currentMaintainECUPowerState != maintainState);
		currentMaintainECUPowerState = maintainState;
		return retVal;
	}

	MaintainPowerInterface::MaintainPowerData::MaintainECUPower MaintainPowerInterface::MaintainPowerData::get_maintain_ecu_power() const
	{
		return currentMaintainECUPowerState;
	}

	std::shared_ptr<ControlFunction> MaintainPowerInterface::MaintainPowerData::get_sender_control_function() const
	{
		return sendingControlFunction;
	}

	void MaintainPowerInterface::MaintainPowerData::set_timestamp_ms(std::uint32_t timestamp)
	{
		timestamp_ms = timestamp;
	}

	std::uint32_t MaintainPowerInterface::MaintainPowerData::get_timestamp_ms() const
	{
		return timestamp_ms;
	}

	void MaintainPowerInterface::initialize()
	{
		if (!initialized)
		{
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::MaintainPower), process_rx_message, this);
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::WheelBasedSpeedAndDistance), process_rx_message, this);
			initialized = true;
		}
	}

	bool MaintainPowerInterface::get_initialized() const
	{
		return initialized;
	}

	void MaintainPowerInterface::set_maintain_power_time(std::uint32_t timeToMaintainPower)
	{
		maintainPowerTime_ms = timeToMaintainPower;
	}

	std::uint32_t MaintainPowerInterface::get_maintain_power_time() const
	{
		return maintainPowerTime_ms;
	}

	std::size_t MaintainPowerInterface::get_number_received_maintain_power_sources() const
	{
		return receivedMaintainPowerMessages.size();
	}

	std::shared_ptr<MaintainPowerInterface::MaintainPowerData> MaintainPowerInterface::get_received_maintain_power(std::size_t index)
	{
		std::shared_ptr<MaintainPowerInterface::MaintainPowerData> retVal = nullptr;

		if (index < receivedMaintainPowerMessages.size())
		{
			retVal = receivedMaintainPowerMessages.at(index);
		}
		return retVal;
	}

	bool MaintainPowerInterface::send_maintain_power() const
	{
		const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
			static_cast<std::uint8_t>(0x0F | (static_cast<std::uint8_t>(maintainPowerTransmitData.get_maintain_actuator_power()) << 4) | (static_cast<std::uint8_t>(maintainPowerTransmitData.get_maintain_ecu_power()) << 6)),
			static_cast<std::uint8_t>(static_cast<std::uint8_t>(maintainPowerTransmitData.get_implement_in_work_state()) |
			                          static_cast<std::uint8_t>(static_cast<std::uint8_t>(maintainPowerTransmitData.get_implement_ready_to_work_state()) << 2) |
			                          static_cast<std::uint8_t>(static_cast<std::uint8_t>(maintainPowerTransmitData.get_implement_park_state()) << 4) |
			                          static_cast<std::uint8_t>(static_cast<std::uint8_t>(maintainPowerTransmitData.get_implement_transport_state()) << 6)),
			0xFF,
			0xFF,
			0xFF,
			0xFF,
			0xFF,
			0xFF
		};

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::MaintainPower),
		                                                      buffer.data(),
		                                                      buffer.size(),
		                                                      std::static_pointer_cast<InternalControlFunction>(maintainPowerTransmitData.get_sender_control_function()));
	}

	void MaintainPowerInterface::process_flags(std::uint32_t flag, void *parentPointer)
	{
		if (static_cast<std::uint32_t>(TransmitFlags::SendMaintainPower) == flag)
		{
			assert(nullptr != parentPointer);
			auto targetInterface = static_cast<MaintainPowerInterface *>(parentPointer);

			if (!targetInterface->send_maintain_power())
			{
				targetInterface->txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendMaintainPower));
			}
		}
	}

	void MaintainPowerInterface::process_rx_message(const CANMessage &message, void *parentPointer)
	{
		assert(nullptr != parentPointer);

		auto targetInterface = static_cast<MaintainPowerInterface *>(parentPointer);

		if (static_cast<std::uint32_t>(CANLibParameterGroupNumber::WheelBasedSpeedAndDistance) == message.get_identifier().get_parameter_group_number())
		{
			// This is checked in the speed interface as well, but we need to know about it for the key switch state.
			if (CAN_DATA_LENGTH == message.get_data_length())
			{
				if (nullptr != message.get_source_control_function())
				{
					// We don't care who's sending this really, we just need to detect a transition from not-off to off.
					const auto decodedKeySwitchState = static_cast<KeySwitchState>((message.get_uint8_at(7) >> 2) & 0x03);

					switch (decodedKeySwitchState)
					{
						case KeySwitchState::Off:
						{
							if (0 != targetInterface->keyNotOffTimestamp)
							{
								LOG_INFO("[Maintain Power]: The key switch state has transitioned from NOT OFF to OFF.");
								targetInterface->keyNotOffTimestamp = 0;

								// Send the maintain power message based on the key state transition
								targetInterface->keyOffTimestamp = SystemTiming::get_timestamp_ms();
								targetInterface->txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendMaintainPower));
								targetInterface->maintainPowerTransmitTimestamp_ms = SystemTiming::get_timestamp_ms();
								targetInterface->keySwitchOffEventPublisher.invoke();
							}
							else if (0 == targetInterface->keyOffTimestamp)
							{
								LOG_INFO("[Maintain Power]: The key switch state is detected as OFF.");
								targetInterface->keyOffTimestamp = SystemTiming::get_timestamp_ms();
							}
						}
						break;

						case KeySwitchState::NotOff:
						{
							if (0 != targetInterface->keyOffTimestamp)
							{
								LOG_INFO("[Maintain Power]: The key switch state has transitioned from OFF to NOT OFF.");
								targetInterface->keyOffTimestamp = 0;
								targetInterface->keyNotOffTimestamp = SystemTiming::get_timestamp_ms();
							}
							else if (0 == targetInterface->keyNotOffTimestamp)
							{
								LOG_INFO("[Maintain Power]: The key switch state is detected as NOT OFF.");
								targetInterface->keyNotOffTimestamp = SystemTiming::get_timestamp_ms();
							}
							targetInterface->maintainPowerTransmitTimestamp_ms = 0;
						}
						break;

						case KeySwitchState::Error:
						{
							LOG_WARNING("[Maintain Power]: The key switch is in an error state.");
							targetInterface->keyOffTimestamp = 0;
							targetInterface->keyNotOffTimestamp = 0;
							targetInterface->maintainPowerTransmitTimestamp_ms = 0;
						}
						break;

						default:
						{
							// The "take no action" state, so we'll ignore it.
						}
						break;
					}
				}
			}
			else
			{
				LOG_WARNING("[Maintain Power]: Received malformed wheel based speed PGN. DLC must be 8.");
			}
		}
		else if (static_cast<std::uint32_t>(CANLibParameterGroupNumber::MaintainPower) == message.get_identifier().get_parameter_group_number())
		{
			if (CAN_DATA_LENGTH == message.get_data_length())
			{
				if (nullptr != message.get_source_control_function())
				{
					auto result = std::find_if(targetInterface->receivedMaintainPowerMessages.cbegin(),
					                           targetInterface->receivedMaintainPowerMessages.cend(),
					                           [&message](const std::shared_ptr<MaintainPowerData> &receivedInfo) {
						                           return (nullptr != receivedInfo) && (receivedInfo->get_sender_control_function() == message.get_source_control_function());
					                           });

					if (result == targetInterface->receivedMaintainPowerMessages.end())
					{
						// There is no existing message object from this control function, so create a new one
						targetInterface->receivedMaintainPowerMessages.push_back(std::make_shared<MaintainPowerData>(message.get_source_control_function()));
						result = targetInterface->receivedMaintainPowerMessages.end() - 1;
					}

					auto &mpMessage = *result;
					bool changed = false;

					changed |= mpMessage->set_maintain_actuator_power(static_cast<MaintainPowerData::MaintainActuatorPower>((message.get_uint8_at(0) >> 4) & 0x03));
					changed |= mpMessage->set_maintain_ecu_power(static_cast<MaintainPowerData::MaintainECUPower>((message.get_uint8_at(0) >> 6) & 0x03));
					changed |= mpMessage->set_implement_in_work_state(static_cast<MaintainPowerData::ImplementInWorkState>(message.get_uint8_at(1) & 0x03));
					changed |= mpMessage->set_implement_ready_to_work_state(static_cast<MaintainPowerData::ImplementReadyToWorkState>((message.get_uint8_at(1) >> 2) & 0x03));
					changed |= mpMessage->set_implement_park_state(static_cast<MaintainPowerData::ImplementParkState>((message.get_uint8_at(1) >> 4) & 0x03));
					changed |= mpMessage->set_implement_transport_state(static_cast<MaintainPowerData::ImplementTransportState>((message.get_uint8_at(1) >> 6) & 0x03));
					mpMessage->set_timestamp_ms(SystemTiming::get_timestamp_ms());

					targetInterface->maintainPowerDataEventPublisher.call(mpMessage, changed);
				}
			}
			else
			{
				LOG_WARNING("[Maintain Power]: Received malformed maintain power PGN. DLC must be 8.");
			}
		}
	}
} // namespace isobus
