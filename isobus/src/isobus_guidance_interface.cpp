//================================================================================================
/// @file isobus_guidance_interface.cpp
///
/// @brief Implements an interface for sending and receiving ISOBUS guidance messages.
/// These messages are used to steer ISOBUS compliant machines, steering valves, and
/// implements in general.
///
/// @attention Please use extreme care if you try to steer a machine with this interface!
/// Remember that this library is licensed under The MIT License, and that by obtaining a
/// copy of this library and of course by attempting to steer a machine with it, you are agreeing
/// to our license.
///
/// @note These messages are expected to be deprecated or at least made redundant in favor
/// of Tractor Implement Management (TIM) at some point by the AEF, though the timeline on that
/// is not known at the time of writing this, and it's likely that many machines will
/// continue to support this interface going forward due to its simplicity over TIM.
///
/// @author Adrian Del Grosso
///
/// @copyright 2023 The Open-Agriculture Developers
//================================================================================================
#include "isobus/isobus/isobus_guidance_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <memory>

namespace isobus
{
	AgriculturalGuidanceInterface::AgriculturalGuidanceInterface(std::shared_ptr<InternalControlFunction> source,
	                                                             std::shared_ptr<ControlFunction> destination,
	                                                             bool enableSendingSystemCommandPeriodically,
	                                                             bool enableSendingMachineInfoPeriodically) :
	  guidanceMachineInfoTransmitData(GuidanceMachineInfo(enableSendingMachineInfoPeriodically ? source : nullptr)),
	  guidanceSystemCommandTransmitData(GuidanceSystemCommand(enableSendingSystemCommandPeriodically ? source : nullptr)),
	  txFlags(static_cast<std::uint32_t>(TransmitFlags::NumberOfFlags), process_flags, this),
	  destinationControlFunction(destination)
	{
	}

	AgriculturalGuidanceInterface::~AgriculturalGuidanceInterface()
	{
		if (initialized)
		{
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AgriculturalGuidanceMachineInfo), process_rx_message, this);
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AgriculturalGuidanceSystemCommand), process_rx_message, this);
		}
	}

	AgriculturalGuidanceInterface::GuidanceSystemCommand::GuidanceSystemCommand(std::shared_ptr<ControlFunction> sender) :
	  controlFunction(sender)
	{
	}

	bool AgriculturalGuidanceInterface::GuidanceSystemCommand::set_status(CurvatureCommandStatus newStatus)
	{
		if (commandedStatus != newStatus)
		{
			commandedStatus = newStatus;
			return true;
		}
		return false;
	}

	AgriculturalGuidanceInterface::GuidanceSystemCommand::CurvatureCommandStatus AgriculturalGuidanceInterface::GuidanceSystemCommand::get_status() const
	{
		return commandedStatus;
	}

	bool AgriculturalGuidanceInterface::GuidanceSystemCommand::set_curvature(float curvature)
	{
		if (std::fabs(commandedCurvature - curvature) > std::numeric_limits<float>::epsilon())
		{
			commandedCurvature = curvature;
			return true;
		}
		return false;
	}

	float AgriculturalGuidanceInterface::GuidanceSystemCommand::get_curvature() const
	{
		return commandedCurvature;
	}

	std::shared_ptr<ControlFunction> AgriculturalGuidanceInterface::GuidanceSystemCommand::get_sender_control_function() const
	{
		return controlFunction;
	}

	void AgriculturalGuidanceInterface::GuidanceSystemCommand::set_timestamp_ms(std::uint32_t timestamp)
	{
		timestamp_ms = timestamp;
	}

	std::uint32_t AgriculturalGuidanceInterface::GuidanceSystemCommand::get_timestamp_ms() const
	{
		return timestamp_ms;
	}

	AgriculturalGuidanceInterface::GuidanceMachineInfo::GuidanceMachineInfo(std::shared_ptr<ControlFunction> sender) :
	  controlFunction(sender)
	{
	}

	bool AgriculturalGuidanceInterface::GuidanceMachineInfo::set_estimated_curvature(float curvature)
	{
		if (std::fabs(estimatedCurvature - curvature) > std::numeric_limits<float>::epsilon())
		{
			estimatedCurvature = curvature;
			return true;
		}
		return false;
	}

	float AgriculturalGuidanceInterface::GuidanceMachineInfo::get_estimated_curvature() const
	{
		return estimatedCurvature;
	}

	bool AgriculturalGuidanceInterface::GuidanceMachineInfo::set_mechanical_system_lockout_state(MechanicalSystemLockout state)
	{
		if (mechanicalSystemLockoutState != state)
		{
			mechanicalSystemLockoutState = state;
			return true;
		}
		return false;
	}

	AgriculturalGuidanceInterface::GuidanceMachineInfo::MechanicalSystemLockout AgriculturalGuidanceInterface::GuidanceMachineInfo::get_mechanical_system_lockout() const
	{
		return mechanicalSystemLockoutState;
	}

	bool AgriculturalGuidanceInterface::GuidanceMachineInfo::set_guidance_steering_system_readiness_state(GenericSAEbs02SlotValue state)
	{
		if (guidanceSteeringSystemReadinessState != state)
		{
			guidanceSteeringSystemReadinessState = state;
			return true;
		}
		return false;
	}

	AgriculturalGuidanceInterface::GuidanceMachineInfo::GenericSAEbs02SlotValue AgriculturalGuidanceInterface::GuidanceMachineInfo::get_guidance_steering_system_readiness_state() const
	{
		return guidanceSteeringSystemReadinessState;
	}

	bool AgriculturalGuidanceInterface::GuidanceMachineInfo::set_guidance_steering_input_position_status(GenericSAEbs02SlotValue state)
	{
		if (guidanceSteeringInputPositionStatus != state)
		{
			guidanceSteeringInputPositionStatus = state;
			return true;
		}
		return false;
	}

	AgriculturalGuidanceInterface::GuidanceMachineInfo::GenericSAEbs02SlotValue AgriculturalGuidanceInterface::GuidanceMachineInfo::get_guidance_steering_input_position_status() const
	{
		return guidanceSteeringInputPositionStatus;
	}

	bool AgriculturalGuidanceInterface::GuidanceMachineInfo::set_request_reset_command_status(RequestResetCommandStatus state)
	{
		if (requestResetCommandStatus != state)
		{
			requestResetCommandStatus = state;
			return true;
		}
		return false;
	}

	AgriculturalGuidanceInterface::GuidanceMachineInfo::RequestResetCommandStatus AgriculturalGuidanceInterface::GuidanceMachineInfo::get_request_reset_command_status() const
	{
		return requestResetCommandStatus;
	}

	bool AgriculturalGuidanceInterface::GuidanceMachineInfo::set_guidance_limit_status(GuidanceLimitStatus status)
	{
		if (guidanceLimitStatus != status)
		{
			guidanceLimitStatus = status;
			return true;
		}
		return false;
	}

	AgriculturalGuidanceInterface::GuidanceMachineInfo::GuidanceLimitStatus AgriculturalGuidanceInterface::GuidanceMachineInfo::get_guidance_limit_status() const
	{
		return guidanceLimitStatus;
	}

	bool AgriculturalGuidanceInterface::GuidanceMachineInfo::set_guidance_system_command_exit_reason_code(std::uint8_t exitCode)
	{
		if (guidanceSystemCommandExitReasonCode != exitCode)
		{
			guidanceSystemCommandExitReasonCode = exitCode;
			return true;
		}
		return false;
	}

	std::uint8_t AgriculturalGuidanceInterface::GuidanceMachineInfo::get_guidance_system_command_exit_reason_code() const
	{
		return guidanceSystemCommandExitReasonCode;
	}

	bool AgriculturalGuidanceInterface::GuidanceMachineInfo::set_guidance_system_remote_engage_switch_status(GenericSAEbs02SlotValue switchStatus)
	{
		if (guidanceSystemRemoteEngageSwitchStatus != switchStatus)
		{
			guidanceSystemRemoteEngageSwitchStatus = switchStatus;
			return true;
		}
		return false;
	}

	AgriculturalGuidanceInterface::GuidanceMachineInfo::GenericSAEbs02SlotValue AgriculturalGuidanceInterface::GuidanceMachineInfo::get_guidance_system_remote_engage_switch_status() const
	{
		return guidanceSystemRemoteEngageSwitchStatus;
	}

	std::shared_ptr<ControlFunction> AgriculturalGuidanceInterface::GuidanceMachineInfo::get_sender_control_function() const
	{
		return controlFunction;
	}

	void AgriculturalGuidanceInterface::GuidanceMachineInfo::set_timestamp_ms(std::uint32_t timestamp)
	{
		timestamp_ms = timestamp;
	}

	std::uint32_t AgriculturalGuidanceInterface::GuidanceMachineInfo::get_timestamp_ms() const
	{
		return timestamp_ms;
	}

	void AgriculturalGuidanceInterface::initialize()
	{
		if (!initialized)
		{
			if ((nullptr != guidanceSystemCommandTransmitData.get_sender_control_function()) || (nullptr != guidanceMachineInfoTransmitData.get_sender_control_function()))
			{
				// Make sure you know what you are doing... consider reviewing the guidance messaging in ISO 11783-7 if you haven't already.
				LOG_WARNING("[Guidance]: Use extreme caution! You have configured the ISOBUS guidance interface with the ability to steer a machine.");
			}
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AgriculturalGuidanceMachineInfo), process_rx_message, this);
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AgriculturalGuidanceSystemCommand), process_rx_message, this);
			initialized = true;
		}
	}

	bool AgriculturalGuidanceInterface::get_initialized() const
	{
		return initialized;
	}

	std::size_t AgriculturalGuidanceInterface::get_number_received_guidance_system_command_sources() const
	{
		return receivedGuidanceSystemCommandMessages.size();
	}

	std::size_t AgriculturalGuidanceInterface::get_number_received_guidance_machine_info_message_sources() const
	{
		return receivedGuidanceMachineInfoMessages.size();
	}

	std::shared_ptr<AgriculturalGuidanceInterface::GuidanceMachineInfo> AgriculturalGuidanceInterface::get_received_guidance_machine_info(std::size_t index)
	{
		std::shared_ptr<AgriculturalGuidanceInterface::GuidanceMachineInfo> retVal = nullptr;

		if (index < receivedGuidanceMachineInfoMessages.size())
		{
			retVal = receivedGuidanceMachineInfoMessages.at(index);
		}
		return retVal;
	}

	std::shared_ptr<AgriculturalGuidanceInterface::GuidanceSystemCommand> AgriculturalGuidanceInterface::get_received_guidance_system_command(std::size_t index)
	{
		std::shared_ptr<AgriculturalGuidanceInterface::GuidanceSystemCommand> retVal = nullptr;

		if (index < receivedGuidanceSystemCommandMessages.size())
		{
			retVal = receivedGuidanceSystemCommandMessages.at(index);
		}
		return retVal;
	}

	EventDispatcher<const std::shared_ptr<AgriculturalGuidanceInterface::GuidanceMachineInfo>, bool> &AgriculturalGuidanceInterface::get_guidance_machine_info_event_publisher()
	{
		return guidanceMachineInfoEventPublisher;
	}

	EventDispatcher<const std::shared_ptr<AgriculturalGuidanceInterface::GuidanceSystemCommand>, bool> &AgriculturalGuidanceInterface::get_guidance_system_command_event_publisher()
	{
		return guidanceSystemCommandEventPublisher;
	}

	bool AgriculturalGuidanceInterface::send_guidance_system_command() const
	{
		bool retVal = false;

		if (nullptr != guidanceSystemCommandTransmitData.get_sender_control_function())
		{
			float scaledCurvature = std::roundf(4 * ((guidanceSystemCommandTransmitData.get_curvature() + CURVATURE_COMMAND_OFFSET_INVERSE_KM) / CURVATURE_COMMAND_RESOLUTION_PER_BIT)) / 4.0f;
			std::uint16_t encodedCurvature = ZERO_CURVATURE_INVERSE_KM;

			if (guidanceMachineInfoTransmitData.get_estimated_curvature() > CURVATURE_COMMAND_MAX_INVERSE_KM)
			{
				encodedCurvature = 32127 + ZERO_CURVATURE_INVERSE_KM; // Clamp to maximum value
				LOG_WARNING("[Guidance]: Transmitting a commanded curvature clamped to maximum value. Verify guidance calculations are accurate!");
			}
			else if (scaledCurvature < 0) // 0 In this case is -8032 km-1 due to the addition of the offset earlier
			{
				encodedCurvature = 0; // Clamp to minimum value
				LOG_WARNING("[Guidance]: Transmitting a commanded curvature clamped to minimum value. Verify guidance calculations are accurate!");
			}
			else
			{
				encodedCurvature = static_cast<std::uint16_t>(scaledCurvature);
			}

			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { static_cast<std::uint8_t>(encodedCurvature & 0xFF),
				                                                   static_cast<std::uint8_t>((encodedCurvature >> 8) & 0xFF),
				                                                   static_cast<std::uint8_t>(static_cast<std::uint8_t>(guidanceSystemCommandTransmitData.get_status()) | static_cast<std::uint8_t>(0xFC)),
				                                                   0xFF,
				                                                   0xFF,
				                                                   0xFF,
				                                                   0xFF,
				                                                   0xFF };

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AgriculturalGuidanceSystemCommand),
			                                                        buffer.data(),
			                                                        buffer.size(),
			                                                        std::static_pointer_cast<InternalControlFunction>(guidanceSystemCommandTransmitData.get_sender_control_function()),
			                                                        destinationControlFunction,
			                                                        CANIdentifier::CANPriority::Priority3);
		}
		return retVal;
	}

	bool AgriculturalGuidanceInterface::send_guidance_machine_info() const
	{
		bool retVal = false;

		if (nullptr != guidanceMachineInfoTransmitData.get_sender_control_function())
		{
			float scaledCurvature = std::roundf(4 * ((guidanceMachineInfoTransmitData.get_estimated_curvature() + CURVATURE_COMMAND_OFFSET_INVERSE_KM) / CURVATURE_COMMAND_RESOLUTION_PER_BIT)) / 4.0f;
			std::uint16_t encodedCurvature = ZERO_CURVATURE_INVERSE_KM;

			if (guidanceMachineInfoTransmitData.get_estimated_curvature() > CURVATURE_COMMAND_MAX_INVERSE_KM)
			{
				encodedCurvature = 32127 + ZERO_CURVATURE_INVERSE_KM; // Clamp to maximum value
				LOG_WARNING("[Guidance]: Transmitting an estimated curvature clamped to maximum value. Verify guidance calculations are accurate!");
			}
			else if (scaledCurvature < 0) // 0 In this case is -8032 km-1 due to the addition of the offset earlier
			{
				encodedCurvature = 0; // Clamp to minimum value
				LOG_WARNING("[Guidance]: Transmitting an estimated curvature clamped to minimum value. Verify guidance calculations are accurate!");
			}
			else
			{
				encodedCurvature = static_cast<std::uint16_t>(scaledCurvature);
			}

			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(encodedCurvature & 0xFF),
				static_cast<std::uint8_t>((encodedCurvature >> 8) & 0xFF),
				static_cast<std::uint8_t>((static_cast<std::uint8_t>(guidanceMachineInfoTransmitData.get_mechanical_system_lockout()) & 0x03) |
				                          ((static_cast<std::uint8_t>(guidanceMachineInfoTransmitData.get_guidance_steering_system_readiness_state()) & 0x03) << 2) |
				                          ((static_cast<std::uint8_t>(guidanceMachineInfoTransmitData.get_guidance_steering_input_position_status()) & 0x03) << 4) |
				                          ((static_cast<std::uint8_t>(guidanceMachineInfoTransmitData.get_request_reset_command_status()) & 0x03) << 6)),
				static_cast<std::uint8_t>(static_cast<std::uint8_t>(guidanceMachineInfoTransmitData.get_guidance_limit_status()) << 5),
				static_cast<std::uint8_t>((guidanceMachineInfoTransmitData.get_guidance_system_command_exit_reason_code() & 0x3F) |
				                          (static_cast<std::uint8_t>(guidanceMachineInfoTransmitData.get_guidance_system_remote_engage_switch_status()) << 6)),
				0xFF, // Reserved
				0xFF, // Reserved
				0xFF // Reserved
			};

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AgriculturalGuidanceMachineInfo),
			                                                        buffer.data(),
			                                                        buffer.size(),
			                                                        std::static_pointer_cast<InternalControlFunction>(guidanceMachineInfoTransmitData.get_sender_control_function()),
			                                                        destinationControlFunction,
			                                                        CANIdentifier::CANPriority::Priority3);
		}
		return retVal;
	}

	void AgriculturalGuidanceInterface::update()
	{
		if (initialized)
		{
			receivedGuidanceMachineInfoMessages.erase(std::remove_if(receivedGuidanceMachineInfoMessages.begin(),
			                                                         receivedGuidanceMachineInfoMessages.end(),
			                                                         [](std::shared_ptr<GuidanceMachineInfo> guidanceInfo) {
				                                                         return SystemTiming::time_expired_ms(guidanceInfo->get_timestamp_ms(), GUIDANCE_MESSAGE_TIMEOUT_MS);
			                                                         }),
			                                          receivedGuidanceMachineInfoMessages.end());
			receivedGuidanceSystemCommandMessages.erase(std::remove_if(receivedGuidanceSystemCommandMessages.begin(),
			                                                           receivedGuidanceSystemCommandMessages.end(),
			                                                           [](std::shared_ptr<GuidanceSystemCommand> guidanceCommand) {
				                                                           return SystemTiming::time_expired_ms(guidanceCommand->get_timestamp_ms(), GUIDANCE_MESSAGE_TIMEOUT_MS);
			                                                           }),
			                                            receivedGuidanceSystemCommandMessages.end());

			if (SystemTiming::time_expired_ms(guidanceMachineInfoTransmitTimestamp_ms, GUIDANCE_MESSAGE_TX_INTERVAL_MS) &&
			    (nullptr != guidanceMachineInfoTransmitData.get_sender_control_function()))
			{
				txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendGuidanceMachineInfo));
				guidanceMachineInfoTransmitTimestamp_ms = SystemTiming::get_timestamp_ms();
			}
			if (SystemTiming::time_expired_ms(guidanceSystemCommandTransmitTimestamp_ms, GUIDANCE_MESSAGE_TX_INTERVAL_MS) &&
			    (nullptr != guidanceSystemCommandTransmitData.get_sender_control_function()))
			{
				txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendGuidanceSystemCommand));
				guidanceSystemCommandTransmitTimestamp_ms = SystemTiming::get_timestamp_ms();
			}
			txFlags.process_all_flags();
		}
		else
		{
			LOG_ERROR("[Guidance]: Guidance interface has not been initialized yet.");
		}
	}

	void AgriculturalGuidanceInterface::process_flags(std::uint32_t flag, void *parentPointer)
	{
		if (nullptr != parentPointer)
		{
			auto targetInterface = static_cast<AgriculturalGuidanceInterface *>(parentPointer);
			bool transmitSuccessful = false;

			switch (flag)
			{
				case static_cast<std::uint32_t>(TransmitFlags::SendGuidanceMachineInfo):
				{
					transmitSuccessful = targetInterface->send_guidance_machine_info();
				}
				break;

				case static_cast<std::uint32_t>(TransmitFlags::SendGuidanceSystemCommand):
				{
					transmitSuccessful = targetInterface->send_guidance_system_command();
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

	void AgriculturalGuidanceInterface::process_rx_message(const CANMessage &message, void *parentPointer)
	{
		assert(nullptr != parentPointer);
		auto targetInterface = static_cast<AgriculturalGuidanceInterface *>(parentPointer);

		switch (message.get_identifier().get_parameter_group_number())
		{
			case static_cast<std::uint32_t>(CANLibParameterGroupNumber::AgriculturalGuidanceSystemCommand):
			{
				if (CAN_DATA_LENGTH == message.get_data_length())
				{
					if (message.get_source_control_function() != nullptr)
					{
						auto result = std::find_if(targetInterface->receivedGuidanceSystemCommandMessages.begin(),
						                           targetInterface->receivedGuidanceSystemCommandMessages.end(),
						                           [&message](const std::shared_ptr<GuidanceSystemCommand> &receivedCommand) {
							                           return (nullptr != receivedCommand) && (receivedCommand->get_sender_control_function() == message.get_source_control_function());
						                           });

						if (result == targetInterface->receivedGuidanceSystemCommandMessages.end())
						{
							// There is no existing message object from this control function, so create a new one
							targetInterface->receivedGuidanceSystemCommandMessages.push_back(std::make_shared<GuidanceSystemCommand>(message.get_source_control_function()));
							result = targetInterface->receivedGuidanceSystemCommandMessages.end() - 1;
						}

						auto guidanceCommand = *result;
						bool changed = false;

						changed |= guidanceCommand->set_curvature((message.get_uint16_at(0) * CURVATURE_COMMAND_RESOLUTION_PER_BIT) - CURVATURE_COMMAND_OFFSET_INVERSE_KM);
						changed |= guidanceCommand->set_status(static_cast<GuidanceSystemCommand::CurvatureCommandStatus>(message.get_uint8_at(2) & 0x03));
						guidanceCommand->set_timestamp_ms(SystemTiming::get_timestamp_ms());

						targetInterface->guidanceSystemCommandEventPublisher.call(guidanceCommand, changed);
					}
				}
				else
				{
					LOG_WARNING("[Guidance]: Received a malformed guidance system command message. DLC must be 8.");
				}
			}
			break;

			case static_cast<std::uint32_t>(CANLibParameterGroupNumber::AgriculturalGuidanceMachineInfo):
			{
				if (CAN_DATA_LENGTH == message.get_data_length())
				{
					if (message.get_source_control_function() != nullptr)
					{
						auto result = std::find_if(targetInterface->receivedGuidanceMachineInfoMessages.cbegin(),
						                           targetInterface->receivedGuidanceMachineInfoMessages.cend(),
						                           [&message](const std::shared_ptr<GuidanceMachineInfo> &receivedInfo) {
							                           return (nullptr != receivedInfo) && (receivedInfo->get_sender_control_function() == message.get_source_control_function());
						                           });

						if (result == targetInterface->receivedGuidanceMachineInfoMessages.cend())
						{
							// There is no existing message object from this control function, so create a new one
							targetInterface->receivedGuidanceMachineInfoMessages.push_back(std::make_shared<GuidanceMachineInfo>(message.get_source_control_function()));
							result = targetInterface->receivedGuidanceMachineInfoMessages.cend() - 1;
						}

						auto machineInfo = *result;
						bool changed = false;

						changed |= machineInfo->set_estimated_curvature((message.get_uint16_at(0) * CURVATURE_COMMAND_RESOLUTION_PER_BIT) - CURVATURE_COMMAND_OFFSET_INVERSE_KM);
						changed |= machineInfo->set_mechanical_system_lockout_state(static_cast<GuidanceMachineInfo::MechanicalSystemLockout>(message.get_uint8_at(2) & 0x03));
						changed |= machineInfo->set_guidance_steering_system_readiness_state(static_cast<GuidanceMachineInfo::GenericSAEbs02SlotValue>((message.get_uint8_at(2) >> 2) & 0x03));
						changed |= machineInfo->set_guidance_steering_input_position_status(static_cast<GuidanceMachineInfo::GenericSAEbs02SlotValue>((message.get_uint8_at(2) >> 4) & 0x03));
						changed |= machineInfo->set_request_reset_command_status(static_cast<GuidanceMachineInfo::RequestResetCommandStatus>((message.get_uint8_at(2) >> 6) & 0x03));
						changed |= machineInfo->set_guidance_limit_status(static_cast<GuidanceMachineInfo::GuidanceLimitStatus>(message.get_uint8_at(3) >> 5));
						changed |= machineInfo->set_guidance_system_command_exit_reason_code(message.get_uint8_at(4) & 0x3F);
						changed |= machineInfo->set_guidance_system_remote_engage_switch_status(static_cast<GuidanceMachineInfo::GenericSAEbs02SlotValue>((message.get_uint8_at(4) >> 6) & 0x03));
						machineInfo->set_timestamp_ms(SystemTiming::get_timestamp_ms());

						targetInterface->guidanceMachineInfoEventPublisher.call(machineInfo, changed);
					}
				}
				else
				{
					LOG_WARNING("[Guidance]: Received a malformed guidance machine info message. DLC must be 8.");
				}
			}
			break;

			default:
				break;
		}
	}
} // namespace isobus
