#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/socket_can_interface.hpp"
#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_network_manager.hpp"

#include "test_CAN_glue.hpp"

#include <chrono>
#include <thread>

TEST(ADDRESS_CLAIM_TESTS, NAMETests)
{
	isobus::NAME TestDeviceNAME(0);
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(1);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(138);
	TestDeviceNAME.set_identity_number(1);
	TestDeviceNAME.set_ecu_instance(4);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(69);

	EXPECT_EQ(TestDeviceNAME.get_arbitrary_address_capable(), true);
	EXPECT_EQ(TestDeviceNAME.get_industry_group(), 1);
	EXPECT_EQ(TestDeviceNAME.get_device_class(), 0);
	EXPECT_EQ(TestDeviceNAME.get_function_code(), 138);
	EXPECT_EQ(TestDeviceNAME.get_identity_number(), 1);
	EXPECT_EQ(TestDeviceNAME.get_ecu_instance(), 4);
	EXPECT_EQ(TestDeviceNAME.get_function_instance(), 0);
	EXPECT_EQ(TestDeviceNAME.get_device_class_instance(), 0);
	EXPECT_EQ(TestDeviceNAME.get_manufacturer_code(), 69);
}

TEST(ADDRESS_CLAIM_TESTS, AddressClaiming)
{
	SocketCANInterface canDriver("can0");
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, &canDriver);
	CANHardwareInterface::start();

	CANHardwareInterface::add_can_lib_update_callback(update_CAN_network, nullptr);
	CANHardwareInterface::add_raw_can_message_rx_callback(raw_can_glue, nullptr);

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	isobus::NAME TestDeviceNAME(0);
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(1);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(138);
	TestDeviceNAME.set_identity_number(1);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(69);

	isobus::InternalControlFunction TestInternalECU(TestDeviceNAME, 0x1C, 0);

	std::this_thread::sleep_for(std::chrono::seconds(2));

	CANHardwareInterface::stop();
}
