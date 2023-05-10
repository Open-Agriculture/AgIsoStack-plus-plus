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
/// @copyright 2023 Adrian Del Grosso
//================================================================================================
#include "isobus/isobus/isobus_guidance_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"

#include <cassert>
#include <cmath>

namespace isobus
{
	GuidanceInterface::GuidanceInterface(std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination) :
	  txFlags(static_cast<std::uint32_t>(TransmitFlags::NumberOfFlags), process_flags, this),
	  sourceControlFunction(source),
	  destinationControlFunction(destination)
	{
	}

	GuidanceInterface::~GuidanceInterface()
	{
		if (initialized)
		{
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AgriculturalGuidanceMachineInfo), process_rx_message, this);
			CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AgriculturalGuidanceSystemCommand), process_rx_message, this);
		}
	}

	bool GuidanceInterface::GuidanceSystemCommand::operator==(const GuidanceSystemCommand &obj)
	{
		return obj.controlFunction == this->controlFunction;
	}

	void GuidanceInterface::GuidanceSystemCommand::set_status(CurvatureCommandStatus newStatus)
	{
		commandedStatus = newStatus;
	}

	GuidanceInterface::GuidanceSystemCommand::CurvatureCommandStatus GuidanceInterface::GuidanceSystemCommand::get_status() const
	{
		return commandedStatus;
	}

	void GuidanceInterface::GuidanceSystemCommand::set_curvature(float curvature)
	{
		commandedCurvature = curvature;
	}

	float GuidanceInterface::GuidanceSystemCommand::get_curvature() const
	{
		return commandedCurvature;
	}

	bool GuidanceInterface::AgriculturalGuidanceMachineInfo::operator==(const AgriculturalGuidanceMachineInfo &obj)
	{
		return obj.controlFunction == this->controlFunction;
	}

	void GuidanceInterface::AgriculturalGuidanceMachineInfo::set_estimated_curvature(float curvature)
	{
		estimatedCurvature = curvature;
	}

	float GuidanceInterface::AgriculturalGuidanceMachineInfo::get_estimated_curvature() const
	{
		return estimatedCurvature;
	}

	void GuidanceInterface::AgriculturalGuidanceMachineInfo::set_mechanical_system_lockout_state(MechanicalSystemLockout state)
	{
		mechanicalSystemLockoutState = state;
	}

	GuidanceInterface::AgriculturalGuidanceMachineInfo::MechanicalSystemLockout GuidanceInterface::AgriculturalGuidanceMachineInfo::get_mechanical_system_lockout() const
	{
		return mechanicalSystemLockoutState;
	}

	void GuidanceInterface::AgriculturalGuidanceMachineInfo::set_guidance_steering_system_readiness_state(GenericSAEbs02SlotValue state)
	{
		guidanceSteeringSystemReadinessState = state;
	}

	GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue GuidanceInterface::AgriculturalGuidanceMachineInfo::get_guidance_steering_system_readiness_state() const
	{
		return guidanceSteeringSystemReadinessState;
	}

	void GuidanceInterface::AgriculturalGuidanceMachineInfo::set_guidance_steering_input_position_status(GenericSAEbs02SlotValue state)
	{
		guidanceSteeringInputPositionStatus = state;
	}

	GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue GuidanceInterface::AgriculturalGuidanceMachineInfo::get_guidance_steering_input_position_status() const
	{
		return guidanceSteeringInputPositionStatus;
	}

	void GuidanceInterface::AgriculturalGuidanceMachineInfo::set_request_reset_command_status(RequestResetCommandStatus state)
	{
		requestResetCommandStatus = state;
	}

	GuidanceInterface::AgriculturalGuidanceMachineInfo::RequestResetCommandStatus GuidanceInterface::AgriculturalGuidanceMachineInfo::get_request_reset_command_status() const
	{
		return requestResetCommandStatus;
	}

	void GuidanceInterface::AgriculturalGuidanceMachineInfo::set_guidance_limit_status(GuidanceLimitStatus status)
	{
		guidanceLimitStatus = status;
	}

	GuidanceInterface::AgriculturalGuidanceMachineInfo::GuidanceLimitStatus GuidanceInterface::AgriculturalGuidanceMachineInfo::get_guidance_limit_status() const
	{
		return guidanceLimitStatus;
	}

	void GuidanceInterface::AgriculturalGuidanceMachineInfo::set_guidance_system_command_exit_reason_code(std::uint8_t exitCode)
	{
		guidanceSystemCommandExitReasonCode = exitCode;
	}

	std::uint8_t GuidanceInterface::AgriculturalGuidanceMachineInfo::get_guidance_system_command_exit_reason_code() const
	{
		return guidanceSystemCommandExitReasonCode;
	}

	void GuidanceInterface::AgriculturalGuidanceMachineInfo::set_guidance_system_remote_engage_switch_status(GenericSAEbs02SlotValue switchStatus)
	{
		guidanceSystemRemoteEngageSwitchStatus = switchStatus;
	}

	GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue GuidanceInterface::AgriculturalGuidanceMachineInfo::get_guidance_system_remote_engage_switch_status() const
	{
		return guidanceSystemRemoteEngageSwitchStatus;
	}

	ControlFunction *GuidanceInterface::AgriculturalGuidanceMachineInfo::get_sender_control_function() const
	{
		return controlFunction;
	}

	void GuidanceInterface::initialize()
	{
		if (!initialized)
		{
			if (nullptr != sourceControlFunction)
			{
				// Make sure you know what you are doing... consider reviewing the guidance messaging in ISO 11783-7 if you haven't already.
				CANStackLogger::warn("[Guidance]: Use extreme caution! You have configured the ISOBUS guidance interface with the ability to steer a machine.");
			}
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AgriculturalGuidanceMachineInfo), process_rx_message, this);
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AgriculturalGuidanceSystemCommand), process_rx_message, this);
			initialized = true;
		}
	}

	std::size_t GuidanceInterface::get_number_received_guidance_system_command_sources() const
	{
		return receivedGuidanceSystemCommandMessages.size();
	}

	std::size_t GuidanceInterface::get_number_received_agricultural_guidance_machine_info_message_sources() const
	{
		return receivedAgriculturalGuidanceMachineInfoMessages.size();
	}

	bool GuidanceInterface::send_guidance_system_command() const
	{
		bool retVal = false;

		if (nullptr != sourceControlFunction)
		{
			float scaledCurvature = std::roundf(4 * ((GuidanceSystemCommandTransmitData.get_curvature() + CURVATURE_COMMAND_OFFSET_INVERSE_KM) / CURVATURE_COMMAND_RESOLUTION_PER_BIT)) / 4.0f;
			std::uint16_t encodedCurvature = ZERO_CURVATURE_INVERSE_KM;

			if (scaledCurvature > CURVATURE_COMMAND_MAX_INVERSE_KM)
			{
				encodedCurvature = 32127 + ZERO_CURVATURE_INVERSE_KM; // Clamp to maximum value
				CANStackLogger::warn("[Guidance]: Transmitting a commanded curvature clamped to maximum value. Verify guidance calculations are accurate!");
			}
			else if (scaledCurvature < 0) // 0 In this case is -8032 km-1 due to the addition of the offset earlier
			{
				encodedCurvature = 0; // Clamp to minimum value
				CANStackLogger::warn("[Guidance]: Transmitting a commanded curvature clamped to minimum value. Verify guidance calculations are accurate!");
			}
			else
			{
				encodedCurvature = scaledCurvature;
			}

			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { static_cast<std::uint8_t>(encodedCurvature & 0xFF),
				                                                   static_cast<std::uint8_t>((encodedCurvature >> 8) & 0xFF),
				                                                   static_cast<std::uint8_t>(static_cast<std::uint8_t>(GuidanceSystemCommandTransmitData.get_status()) | static_cast<std::uint8_t>(0xFC)),
				                                                   0xFF,
				                                                   0xFF,
				                                                   0xFF,
				                                                   0xFF,
				                                                   0xFF };

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AgriculturalGuidanceSystemCommand),
			                                                        buffer.data(),
			                                                        buffer.size(),
			                                                        sourceControlFunction.get(),
			                                                        destinationControlFunction.get(),
			                                                        CANIdentifier::Priority3);
		}
		return retVal;
	}

	bool GuidanceInterface::send_agricultural_guidance_machine_info() const
	{
		bool retVal = false;

		if (nullptr != sourceControlFunction)
		{
			float scaledCurvature = std::roundf(4 * ((AgriculturalGuidanceMachineInfoTransmitData.get_estimated_curvature() + CURVATURE_COMMAND_OFFSET_INVERSE_KM) / CURVATURE_COMMAND_RESOLUTION_PER_BIT)) / 4.0f;
			std::uint16_t encodedCurvature = ZERO_CURVATURE_INVERSE_KM;

			if (scaledCurvature > CURVATURE_COMMAND_MAX_INVERSE_KM)
			{
				encodedCurvature = 32127 + ZERO_CURVATURE_INVERSE_KM; // Clamp to maximum value
				CANStackLogger::warn("[Guidance]: Transmitting an estimated curvature clamped to maximum value. Verify guidance calculations are accurate!");
			}
			else if (scaledCurvature < 0) // 0 In this case is -8032 km-1 due to the addition of the offset earlier
			{
				encodedCurvature = 0; // Clamp to minimum value
				CANStackLogger::warn("[Guidance]: Transmitting an estimated curvature clamped to minimum value. Verify guidance calculations are accurate!");
			}
			else
			{
				encodedCurvature = scaledCurvature;
			}

			std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {
				static_cast<std::uint8_t>(encodedCurvature & 0xFF),
				static_cast<std::uint8_t>((encodedCurvature >> 8) & 0xFF),
				static_cast<std::uint8_t>((static_cast<std::uint8_t>(AgriculturalGuidanceMachineInfoTransmitData.get_mechanical_system_lockout()) & 0x03) |
				                          ((static_cast<std::uint8_t>(AgriculturalGuidanceMachineInfoTransmitData.get_guidance_steering_system_readiness_state()) & 0x03) << 2) |
				                          ((static_cast<std::uint8_t>(AgriculturalGuidanceMachineInfoTransmitData.get_guidance_steering_input_position_status()) & 0x03) << 4) |
				                          ((static_cast<std::uint8_t>(AgriculturalGuidanceMachineInfoTransmitData.get_request_reset_command_status()) & 0x03) << 6)),
				static_cast<std::uint8_t>(static_cast<std::uint8_t>(AgriculturalGuidanceMachineInfoTransmitData.get_guidance_limit_status()) << 5),
				static_cast<std::uint8_t>((AgriculturalGuidanceMachineInfoTransmitData.get_guidance_system_command_exit_reason_code() & 0x3F) |
				                          (static_cast<std::uint8_t>(AgriculturalGuidanceMachineInfoTransmitData.get_guidance_system_remote_engage_switch_status()) << 6)),
				0xFF, // Reserved
				0xFF // Reserved
			};

			retVal = CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::AgriculturalGuidanceMachineInfo),
			                                                        buffer.data(),
			                                                        buffer.size(),
			                                                        sourceControlFunction.get(),
			                                                        destinationControlFunction.get(),
			                                                        CANIdentifier::Priority3);
		}
		return retVal;
	}

	void GuidanceInterface::update()
	{
		if (nullptr != sourceControlFunction)
		{
			if (SystemTiming::time_expired_ms(agriculturalGuidanceMachineInfoTransmitTimestamp_ms, GUIDANCE_MESSAGE_TX_INTERVAL_MS))
			{
				txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendGuidanceMachineInfo));
				agriculturalGuidanceMachineInfoTransmitTimestamp_ms = SystemTiming::get_timestamp_ms();
			}
			if (SystemTiming::time_expired_ms(guidanceSystemCommandTransmitTimestamp_ms, GUIDANCE_MESSAGE_TX_INTERVAL_MS))
			{
				txFlags.set_flag(static_cast<std::uint32_t>(TransmitFlags::SendGuidanceSystemCommand));
				guidanceSystemCommandTransmitTimestamp_ms = SystemTiming::get_timestamp_ms();
			}
			txFlags.process_all_flags();
		}
	}

	void GuidanceInterface::process_flags(std::uint32_t flag, void *parentPointer)
	{
		if (nullptr != parentPointer)
		{
			auto targetInterface = static_cast<GuidanceInterface *>(parentPointer);
			bool transmitSuccessful = false;

			switch (flag)
			{
				case static_cast<std::uint32_t>(TransmitFlags::SendGuidanceMachineInfo):
				{
					transmitSuccessful = targetInterface->send_agricultural_guidance_machine_info();
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

	void GuidanceInterface::process_rx_message(CANMessage *message, void *parentPointer)
	{
		assert(nullptr != message);
		assert(nullptr != parentPointer);

		switch (message->get_identifier().get_parameter_group_number())
		{
			case static_cast<std::uint32_t>(CANLibParameterGroupNumber::AgriculturalGuidanceSystemCommand):
			{
			}
			break;

			case static_cast<std::uint32_t>(CANLibParameterGroupNumber::AgriculturalGuidanceMachineInfo):
			{
			}
			break;

			default:
				break;
		}
	}
} // namespace isobus
