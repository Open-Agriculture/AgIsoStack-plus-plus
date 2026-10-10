#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_NAME_filter.hpp"
#include "isobus/isobus/can_constants.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"

#include "helpers/test_fixture.hpp"

#include <chrono>
#include <thread>

using namespace isobus;

class AddressClaimTest : public AgIsoStackTestFixture
{
	// Wrapper to give tests a more meaningful name - no content.
};

TEST_F(AddressClaimTest, AddressClaim_PartneredClaim)
{
	auto firstDevice = std::make_shared<VirtualCANPlugin>();
	auto secondDevice = std::make_shared<VirtualCANPlugin>();
	CANHardwareInterface::set_number_of_can_channels(2);
	CANHardwareInterface::assign_can_channel_frame_handler(0, firstDevice);
	CANHardwareInterface::assign_can_channel_frame_handler(1, secondDevice);
	CANHardwareInterface::start(false);

	time_source.update_for_ms(250);

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
	auto firstInternalECU = CANNetworkManager::CANNetwork.create_internal_control_function(firstName, 0, 0x1C);

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
	auto secondInternalECU2 = CANNetworkManager::CANNetwork.create_internal_control_function(secondName, 1);

	const NAMEFilter filterSecond(NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(NAME::Function::SeatControl));
	auto firstPartneredSecondECU = CANNetworkManager::CANNetwork.create_partnered_control_function(0, { filterSecond });
	const isobus::NAMEFilter filterFirst(NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(NAME::Function::CabClimateControl));
	auto secondPartneredFirstEcu = CANNetworkManager::CANNetwork.create_partnered_control_function(1, { filterFirst });

	time_source.update_for_ms(500);
	EXPECT_TRUE(firstInternalECU->get_address_valid());
	EXPECT_TRUE(secondInternalECU2->get_address_valid());
	EXPECT_TRUE(firstPartneredSecondECU->get_address_valid());
	EXPECT_TRUE(secondPartneredFirstEcu->get_address_valid());

	// Test sending a forced request for address claim
	EXPECT_TRUE(CANNetworkManager::CANNetwork.send_request_for_address_claim(0));

	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(firstPartneredSecondECU);
	CANNetworkManager::CANNetwork.deactivate_control_function(secondPartneredFirstEcu);
	CANNetworkManager::CANNetwork.deactivate_control_function(firstInternalECU);
	CANNetworkManager::CANNetwork.deactivate_control_function(secondInternalECU2);
}

TEST_F(AddressClaimTest, CannotClaim)
{
	VirtualCANPlugin plugin;
	plugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	time_source.update_for_ms(250);

	// Claim a very low name on every address
	NAME firstName(0);
	firstName.set_arbitrary_address_capable(true);
	firstName.set_industry_group(0);
	firstName.set_device_class(0);
	firstName.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::Engine));

	firstName.set_ecu_instance(0);
	firstName.set_function_instance(0);
	firstName.set_device_class_instance(0);
	firstName.set_manufacturer_code(1);

	// Force claim message
	CANMessageFrame testFrame = {};
	testFrame.channel = 0;

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	for (std::uint8_t i = 0; i < 0xFE; i++)
	{
		firstName.set_identity_number(i);

		std::uint64_t fullName = firstName.get_full_name();
		testFrame.identifier = 0x18EEFF00 | i;
		testFrame.isExtendedFrame = true;
		testFrame.dataLength = 8;
		testFrame.data[0] = static_cast<std::uint8_t>(fullName);
		testFrame.data[1] = static_cast<std::uint8_t>(fullName >> 8);
		testFrame.data[2] = static_cast<std::uint8_t>(fullName >> 16);
		testFrame.data[3] = static_cast<std::uint8_t>(fullName >> 24);
		testFrame.data[4] = static_cast<std::uint8_t>(fullName >> 32);
		testFrame.data[5] = static_cast<std::uint8_t>(fullName >> 40);
		testFrame.data[6] = static_cast<std::uint8_t>(fullName >> 48);
		testFrame.data[7] = static_cast<std::uint8_t>(fullName >> 56);

		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	}

	// Verify an internal control function with higher name cannot claim
	isobus::NAME secondName(0);
	secondName.set_arbitrary_address_capable(true);
	secondName.set_industry_group(1);
	secondName.set_device_class(6);
	secondName.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::RateControl));
	secondName.set_identity_number(65534);
	secondName.set_ecu_instance(1);
	secondName.set_function_instance(2);
	secondName.set_device_class_instance(0);
	secondName.set_manufacturer_code(1407);

	// Get the virtual CAN plugin back to a known state
	while (!plugin.get_queue_empty())
	{
		plugin.read_frame(testFrame);
	}
	ASSERT_TRUE(plugin.get_queue_empty());

	auto secondInternalECU2 = CANNetworkManager::CANNetwork.create_internal_control_function(secondName, 0);

	time_source.update_for_ms(1500);

	bool cannot_claim_message_seen = false;
	while (!plugin.get_queue_empty())
	{
		plugin.read_frame(testFrame);

		if ((8 == testFrame.dataLength) && (0xEEFF == ((testFrame.identifier >> 8) & 0x3FFFF)))
		{
			EXPECT_EQ(0xFE, (testFrame.identifier & 0xFF));
			cannot_claim_message_seen = true;
			break;
		}
	}
	EXPECT_TRUE(cannot_claim_message_seen);
	EXPECT_FALSE(secondInternalECU2->get_address_valid());
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(secondInternalECU2);
}

TEST_F(AddressClaimTest, PreferredAddressContention)
{
	constexpr std::uint8_t addressToTest = 0xF7;
	VirtualCANPlugin plugin;
	plugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	time_source.update_for_ms(250);

	// Claim a very high name on address F7
	NAME firstName(0);
	firstName.set_arbitrary_address_capable(true);
	firstName.set_industry_group(6);
	firstName.set_device_class(4);
	firstName.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::PropulsionSensorsAndGateway));
	firstName.set_identity_number(5000);
	firstName.set_ecu_instance(5);
	firstName.set_function_instance(5);
	firstName.set_device_class_instance(2);
	firstName.set_manufacturer_code(500);

	// Force claim message
	CANMessageFrame testFrame = {};
	testFrame.channel = 0;

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);

	std::uint64_t fullName = firstName.get_full_name();
	testFrame.identifier = 0x18EEFF00 | addressToTest;
	testFrame.isExtendedFrame = true;
	testFrame.dataLength = 8;
	testFrame.data[0] = static_cast<std::uint8_t>(fullName);
	testFrame.data[1] = static_cast<std::uint8_t>(fullName >> 8);
	testFrame.data[2] = static_cast<std::uint8_t>(fullName >> 16);
	testFrame.data[3] = static_cast<std::uint8_t>(fullName >> 24);
	testFrame.data[4] = static_cast<std::uint8_t>(fullName >> 32);
	testFrame.data[5] = static_cast<std::uint8_t>(fullName >> 40);
	testFrame.data[6] = static_cast<std::uint8_t>(fullName >> 48);
	testFrame.data[7] = static_cast<std::uint8_t>(fullName >> 56);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);

	// Verify an internal control function with lower name wins its address
	isobus::NAME secondName(0);
	secondName.set_arbitrary_address_capable(true);
	secondName.set_industry_group(0);
	secondName.set_device_class(0);
	secondName.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::Engine));
	secondName.set_identity_number(1);
	secondName.set_ecu_instance(0);
	secondName.set_function_instance(0);
	secondName.set_device_class_instance(0);
	secondName.set_manufacturer_code(1);

	auto secondInternalECU2 = CANNetworkManager::CANNetwork.create_internal_control_function(secondName, 0, addressToTest);

	time_source.update_for_ms(1500);

	EXPECT_TRUE(secondInternalECU2->get_address_valid());
	EXPECT_EQ(addressToTest, secondInternalECU2->get_address());
	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(secondInternalECU2);
}

// A partnered control function that is created while the network manager's prune timer for the last
// global request for address claim is still pending must not be declared offline if its device
// already answered that request.
TEST_F(AddressClaimTest, PartnerCreatedInsideRequestWindowIsNotPruned)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	time_source.update_for_ms(250);

	// An external device (a VT) that is already on the bus. Frames are injected as if they came from the wire.
	isobus::NAME vtName(0);
	vtName.set_arbitrary_address_capable(false);
	vtName.set_industry_group(2);
	vtName.set_device_class(0);
	vtName.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));
	vtName.set_identity_number(1);
	vtName.set_ecu_instance(0);
	vtName.set_function_instance(0);
	vtName.set_device_class_instance(0);
	vtName.set_manufacturer_code(69);

	constexpr std::uint8_t vtAddress = 0x26;
	const auto inject_vt_address_claim = [&]() {
		CANMessageFrame claim = {};
		claim.channel = 0;
		claim.isExtendedFrame = true;
		claim.identifier = 0x18EEFF00 | vtAddress;
		claim.dataLength = 8;
		const std::uint64_t fullName = vtName.get_full_name();
		for (std::uint8_t i = 0; i < 8; i++)
		{
			claim.data[i] = static_cast<std::uint8_t>(fullName >> (8 * i));
		}
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(claim);
	};

	inject_vt_address_claim();
	time_source.update_for_ms(50);

	// Some device on the bus (here: one that has not claimed an address yet, so it uses the null address
	// 254) sends a global request for address claim, and the VT answers straight away.
	CANMessageFrame request = {};
	request.channel = 0;
	request.isExtendedFrame = true;
	request.identifier = 0x18EAFFFE;
	request.dataLength = 3;
	request.data[0] = 0x00; // PGN 0x00EE00 (address claim), little endian
	request.data[1] = 0xEE;
	request.data[2] = 0x00;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
	time_source.update_for_ms(10);
	inject_vt_address_claim();
	time_source.update_for_ms(50);

	// The application only now creates its partner for the VT, ~60 ms after that request
	const NAMEFilter filterVirtualTerminal(NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(NAME::Function::VirtualTerminal));
	auto partnerVT = CANNetworkManager::CANNetwork.create_partnered_control_function(0, { filterVirtualTerminal });
	time_source.update_for_ms(50);

	// The partner is matched with the device that is already on the bus
	ASSERT_TRUE(partnerVT->get_address_valid());
	EXPECT_EQ(vtAddress, partnerVT->get_address());

	// The prune timer for the request above runs out 755 ms after it. The VT answered the request, so it
	// must still be online afterwards.
	time_source.update_for_ms(1000);
	EXPECT_TRUE(partnerVT->get_address_valid()) << "The VT answered the request for address claim but the partnered control function was marked offline anyway";

	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.deactivate_control_function(partnerVT);
}
