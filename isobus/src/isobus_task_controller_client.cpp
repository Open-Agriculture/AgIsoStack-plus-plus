#include "isobus/isobus/isobus_task_controller_client.hpp"

#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"

#include <array>

namespace isobus
{
	TaskControllerClient::TaskControllerClient(std::shared_ptr<PartneredControlFunction> partner, std::shared_ptr<InternalControlFunction> clientSource) :
	  partnerControlFunction(partner),
	  myControlFunction(clientSource)
	{
	}

	bool TaskControllerClient::send_working_set_master() const
	{
		const std::array<std::uint8_t, CAN_DATA_LENGTH> buffer = { numberOfWorkingSetMembers, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

		return CANNetworkManager::CANNetwork.send_can_message(static_cast<std::uint32_t>(CANLibParameterGroupNumber::WorkingSetMaster),
		                                                      buffer.data(),
		                                                      CAN_DATA_LENGTH,
		                                                      myControlFunction.get(),
		                                                      partnerControlFunction.get());
	}
} // namespace isobus
