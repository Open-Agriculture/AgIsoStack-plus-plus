#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_NAME_filter.hpp"
#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"

#include <chrono>
#include <thread>

using namespace isobus;

TEST(ADDRESS_CLAIM_TESTS, PartneredClaim)
{
	std::shared_ptr<VirtualCANPlugin> firstDevice = std::make_shared<VirtualCANPlugin>();
	std::shared_ptr<VirtualCANPlugin> secondDevice = std::make_shared<VirtualCANPlugin>();
	CANHardwareInterface::set_number_of_can_channels(2);
	CANHardwareInterface::assign_can_channel_frame_handler(0, firstDevice);
	CANHardwareInterface::assign_can_channel_frame_handler(1, secondDevice);
	CANHardwareInterface::start();

	CANHardwareInterface::add_can_lib_update_callback(
	  [] {
		  CANNetworkManager::CANNetwork.update();
	  },
	  nullptr);
	CANHardwareInterface::add_raw_can_message_rx_callback(
	  [](HardwareInterfaceCANFrame &rxFrame, void *parentPointer) {
		  CANNetworkManager::CANNetwork.can_lib_process_rx_message(rxFrame, parentPointer);
	  },
	  nullptr);

	std::this_thread::sleep_for(std::chrono::milliseconds(250));

	NAME firstName(0);
	firstName.set_arbitrary_address_capable(true);
	firstName.set_industry_group(1);
	firstName.set_device_class(0);
	firstName.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::CabClimateControl));
	firstName.set_identity_number(1);
	firstName.set_ecu_instance(0);
	firstName.set_function_instance(0);
	firstName.set_device_class_instance(0);
	firstName.set_manufacturer_code(69);
	InternalControlFunction firstInternalECU(firstName, 0x1C, 0);

	isobus::NAME secondName(0);
	secondName.set_arbitrary_address_capable(true);
	secondName.set_industry_group(1);
	secondName.set_device_class(0);
	secondName.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::SeatControl));
	secondName.set_identity_number(2);
	secondName.set_ecu_instance(0);
	secondName.set_function_instance(0);
	secondName.set_device_class_instance(0);
	secondName.set_manufacturer_code(69);
	InternalControlFunction secondInternalECU2(secondName, 0x1D, 1);

	const NAMEFilter filterSecond(NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(NAME::Function::SeatControl));
	PartneredControlFunction firstPartneredSecondECU(0, { filterSecond });
	const isobus::NAMEFilter filterFirst(NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(NAME::Function::CabClimateControl));
	PartneredControlFunction secondPartneredFirstEcu(1, { filterFirst });

	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	EXPECT_TRUE(firstPartneredSecondECU.get_address_valid());
	EXPECT_TRUE(secondPartneredFirstEcu.get_address_valid());

	CANHardwareInterface::stop();
}
