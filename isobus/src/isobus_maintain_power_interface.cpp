#include "isobus/isobus/isobus_maintain_power_interface.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

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

	void MaintainPowerInterface::initialize()
	{
		if (!initialized)
		{
			CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(static_cast<std::uint32_t>(CANLibParameterGroupNumber::MaintainPower), process_rx_message, this);
			initialized = true;
		}
	}

	bool MaintainPowerInterface::get_initialized() const
	{
		return initialized;
	}

	bool MaintainPowerInterface::send_maintain_power() const
	{
		bool retVal = false;

		std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = {

		};

		//retVal = CANNetworkManager::CANNetwork.send_can_message()
		return retVal;
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
		if (CAN_DATA_LENGTH == message.get_data_length())
		{
		}
		else
		{
			CANStackLogger::warn("[Maintain Power]: Received malformed maintain power PGN. DLC must be 8.");
		}
	}
} // namespace isobus
