#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_task_controller_server.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"

using namespace isobus;

class DerivedTcServer : public TaskControllerServer
{
public:
	DerivedTcServer(std::shared_ptr<InternalControlFunction> internalControlFunction,
	                std::uint8_t numberBoomsSupported,
	                std::uint8_t numberSectionsSupported,
	                std::uint8_t numberChannelsSupportedForPositionBasedControl,
	                std::uint8_t optionsBitfield) :
	  TaskControllerServer(internalControlFunction,
	                       numberBoomsSupported,
	                       numberSectionsSupported,
	                       numberChannelsSupportedForPositionBasedControl,
	                       optionsBitfield)
	{
	}

	bool activate_object_pool(std::shared_ptr<ControlFunction>, ObjectPoolActivationError&, ObjectPoolErrorCodes&, std::uint16_t&, std::uint16_t&) override
	{
		return true;
	}

	bool change_designator(std::shared_ptr<ControlFunction>, std::uint16_t, const std::vector<std::uint8_t>&)
	{
		return true;
	}

	bool deactivate_object_pool(std::shared_ptr<ControlFunction>)
	{
		return true;
	}

	bool delete_device_descriptor_object_pool(std::shared_ptr<ControlFunction>, ObjectPoolDeletionErrors&)
	{
		return true;
	}

	bool get_is_stored_device_descriptor_object_pool_by_structure_label(std::shared_ptr<ControlFunction>, const std::vector<std::uint8_t>&, const std::vector<std::uint8_t>&)
	{
		return false;
	}

	bool get_is_stored_device_descriptor_object_pool_by_localization_label(std::shared_ptr<ControlFunction>, const std::array<std::uint8_t, 7>&)
	{
		return false;
	}

	bool get_is_enough_memory_available(std::uint32_t)
	{
		return true;
	}

	std::uint32_t get_number_of_complete_object_pools_stored_for_client(std::shared_ptr<ControlFunction>)
	{
		return 0;
	}

	void identify_task_controller(std::uint8_t)
	{

	}

	void on_client_timeout(std::shared_ptr<ControlFunction>)
	{

	}

	void on_process_data_acknowledge(std::shared_ptr<ControlFunction>, std::uint16_t, std::uint16_t, std::uint8_t, ProcessDataCommands )
	{

	}

	bool on_value_command(std::shared_ptr<ControlFunction>, std::uint16_t, std::uint16_t, std::int32_t, std::uint8_t&)
	{
		return true;
	}

	bool store_device_descriptor_object_pool(std::shared_ptr<ControlFunction>, const std::vector<std::uint8_t>&, bool)
	{
		return true;
	}
};

TEST(TASK_CONTROLLER_SERVER_TESTS, MessageEncoding)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	NAME clientNAME(0);
	clientNAME.set_industry_group(2);
	clientNAME.set_function_code(static_cast<std::uint8_t>(NAME::Function::TaskController));
	auto internalECU = test_helpers::claim_internal_control_function(0x87, 0);

	CANMessageFrame testFrame;

	DerivedTcServer server(internalECU, 4, 255, 16, 0x17);
	server.initialize();


	CANHardwareInterface::stop();
}
