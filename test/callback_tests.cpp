#include <gtest/gtest.h>

#include "can_internal_control_function.hpp"
#include "can_network_manager.hpp"
#include "can_types.hpp"
#include "socket_can_interface.hpp"
#include "test_CAN_glue.hpp"
#include "can_lib_parameter_group_numbers.hpp"

#include <chrono>
#include <thread>

static bool addressClaimCallbackHit = false;
static bool propACallbackHit = false;

void testAddressClaimCallback(isobus::CANMessage *message, void *)
{
	if (nullptr != message)
	{
		addressClaimCallbackHit = true;
	}
}

void testPropACallback(isobus::CANMessage *message, void *)
{
	if (nullptr != message)
	{
		propACallbackHit = true;
	}
}

TEST(CALLBACK_TESTS, PGNCallbacks)
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
	TestDeviceNAME.set_identity_number(1);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(69);

	isobus::InternalControlFunction TestInternalECU(TestDeviceNAME, 0x1C, 0);
	isobus::CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA), &testPropACallback);
	isobus::CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::AddressClaim), &testAddressClaimCallback);

	std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait for address claim process

	// Send some messages manually to fake another ECU
	isobus::HardwareInterfaceCANFrame testFrame;
	testFrame.channel = 0;
	testFrame.isExtendedFrame = true;
	testFrame.dataLength = 8;

	// Construct an address claim: 18EEFF81   [8]  00 00 BB 08 00 00 00 A0
	testFrame.identifier = 0x18EEFF81;
	testFrame.data[0] = 0x00;
	testFrame.data[1] = 0x00;
	testFrame.data[2] = 0xBB;
	testFrame.data[3] = 0x08;
	testFrame.data[4] = 0x00;
	testFrame.data[5] = 0x00;
	testFrame.data[6] = 0x00;
	testFrame.data[7] = 0xA0;
	isobus::CANNetworkManager::CANNetwork.can_lib_process_rx_message(testFrame, nullptr);

	// Construct a PROPA
	testFrame.identifier = 0x18EFFF81;
	isobus::CANNetworkManager::CANNetwork.can_lib_process_rx_message(testFrame, nullptr);

	std::this_thread::sleep_for(std::chrono::seconds(1)); // Wait for threads to run

	EXPECT_EQ(addressClaimCallbackHit, true);
	EXPECT_EQ(propACallbackHit, true);

	isobus::CANNetworkManager::CANNetwork.remove_global_parameter_group_number_callback(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::ProprietaryA), &testPropACallback);
	isobus::CANNetworkManager::CANNetwork.remove_global_parameter_group_number_callback(static_cast<std::uint32_t>(isobus::CANLibParameterGroupNumber::AddressClaim), &testAddressClaimCallback);

	CANHardwareInterface::stop();
}
