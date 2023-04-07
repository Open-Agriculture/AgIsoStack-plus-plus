#include <gtest/gtest.h>

#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"

#include <memory>
#include <thread>

using namespace isobus;

TEST(CORE_TESTS, TestCreateAndDestroyPartners)
{
	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));

	isobus::PartneredControlFunction TestPartner1(0, vtNameFilters);
	isobus::PartneredControlFunction *TestPartner2 = new isobus::PartneredControlFunction(0, vtNameFilters);
	delete TestPartner2;
	auto TestPartner3 = std::make_shared<isobus::PartneredControlFunction>(0, vtNameFilters);
}

TEST(CORE_TESTS, TestCreateAndDestroyICFs)
{
	isobus::NAME TestDeviceNAME(0);
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(0);
	TestDeviceNAME.set_device_class(0);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SteeringControl));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(0);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);

	isobus::InternalControlFunction TestIcf1(TestDeviceNAME, 0x1C, 0);
	auto TestIcf2 = new isobus::InternalControlFunction(TestDeviceNAME, 0x80, 0);
	delete TestIcf2;
	auto TestIcf3 = std::make_shared<isobus::InternalControlFunction>(TestDeviceNAME, 0x81, 0);
}

TEST(CORE_TESTS, BusloadTest)
{
	EXPECT_EQ(0.0f, CANNetworkManager::CANNetwork.get_estimated_busload(0)); // This test runs early in the testing, so load should be zero.
	EXPECT_EQ(0.0f, CANNetworkManager::CANNetwork.get_estimated_busload(200)); // Invalid channel should return zero load

	// Send a bunch of messages through the receive process
	HardwareInterfaceCANFrame testFrame;
	testFrame.dataLength = 8;
	testFrame.channel = 0;
	testFrame.isExtendedFrame = true;
	testFrame.identifier = 0x18EFFFFE;
	memset(testFrame.data, 0, sizeof(testFrame.data));

	CANNetworkManager::CANNetwork.update(); // Make sure the network manager is initialized
	for (std::uint_fast8_t i = 0; i < 25; i++)
	{
		CANNetworkManager::process_receive_can_message_frame(testFrame); // Send a bunch of junk messages
	}
	testFrame.isExtendedFrame = false;
	testFrame.identifier = 0x7F;
	for (std::uint_fast8_t i = 0; i < 25; i++)
	{
		CANNetworkManager::process_receive_can_message_frame(testFrame); // Send a bunch of junk messages
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(101));
	CANNetworkManager::CANNetwork.update();

	// Bus load should be non zero, and less than 100%
	EXPECT_NE(0.0f, CANNetworkManager::CANNetwork.get_estimated_busload(0));
	EXPECT_LT(CANNetworkManager::CANNetwork.get_estimated_busload(0), 100.0f);
}
