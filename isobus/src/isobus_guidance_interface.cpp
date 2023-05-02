#include "isobus/isobus/isobus_guidance_interface.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

namespace isobus
{
	GuidanceInterface::GuidanceInterface(std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination) :
	  txFlags(static_cast<std::uint32_t>(TransmitFlags::NumberOfFlags), process_flags, this),
	  sourceControlFunction(source),
	  destinationControlFunction(destination)
	{
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

	void GuidanceInterface::AgriculturalGuidanceMachineInfo::set_estimated_curvature(float curvature)
	{
		estimatedCurvature = curvature;
	}

	float GuidanceInterface::AgriculturalGuidanceMachineInfo::get_estimated_curvature() const
	{
		return estimatedCurvature;
	}

	void GuidanceInterface::initialize()
	{
		if (!initialized)
		{
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

	bool GuidanceInterface::send_guidance_curvature_command()
	{
		bool retVal = false;

		if (nullptr != sourceControlFunction)
		{
			//retVal = CANNetworkManager::CANNetwork.send_can_message()
		}
		return retVal;
	}

	void GuidanceInterface::process_flags(std::uint32_t flag, void *parentPointer)
	{
		if (nullptr != parentPointer)
		{
			auto targetInterface = static_cast<GuidanceInterface *>(parentPointer);
			bool transmitSuccessful = false;

			switch (flag)
			{
				default:
					break;
			}

			if (false == transmitSuccessful)
			{
				targetInterface->txFlags.set_flag(flag);
			}
		}
	}
} // namespace isobus
