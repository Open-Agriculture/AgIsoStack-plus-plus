#include "can_network_manager.hpp"
#include "socket_can_interface.hpp"
#include "test_CAN_glue.hpp"
#include "can_partnered_control_function.hpp"
#include "can_general_parameter_group_numbers.hpp"

#include <csignal>

static isobus::InternalControlFunction *TestInternalECU = nullptr;

void signal_handler(int signum)
{
    CANHardwareInterface::stop();
    exit(signum);
}

void setup()
{
    CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, "vcan0");
	CANHardwareInterface::start();

	CANHardwareInterface::add_can_lib_update_callback(update_CAN_network, nullptr);
	CANHardwareInterface::add_raw_can_message_rx_callback(raw_can_glue, nullptr);

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	isobus::NAME TestDeviceNAME(0);
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(1);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(138);
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(69);

	TestInternalECU = new isobus::InternalControlFunction(TestDeviceNAME, 0x80, 0);
    std::signal(SIGINT,signal_handler);
}

void testPropACallback(isobus::CANMessage *message, void *parentPointer)
{

}

int main()
{
    setup();

    isobus::CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA), &testPropACallback);

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    CANHardwareInterface::stop();
    return 0;
}
