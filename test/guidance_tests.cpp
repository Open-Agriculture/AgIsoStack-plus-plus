#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_guidance_interface.hpp"
#include "isobus/utility/system_timing.hpp"

using namespace isobus;

class TestGuidanceInterface : public GuidanceInterface
{
public:
	TestGuidanceInterface(std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination) :
	  GuidanceInterface(source, destination){

	  };

	void test_wrapper_set_flag(std::uint32_t flag)
	{
		txFlags.set_flag(flag);
	}

	bool test_wrapper_send_guidance_system_command() const
	{
		return send_guidance_system_command();
	}

	bool test_wrapper_send_guidance_info() const
	{
		return send_agricultural_guidance_machine_info();
	}
};

TEST(GUIDANCE_TESTS, GuidanceMessages)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	isobus::NAME TestDeviceNAME(0);
	TestDeviceNAME.set_arbitrary_address_capable(true);
	TestDeviceNAME.set_industry_group(3);
	TestDeviceNAME.set_device_class(4);
	TestDeviceNAME.set_function_code(static_cast<std::uint8_t>(isobus::NAME::Function::AdaptiveFrontLightingSystem));
	TestDeviceNAME.set_identity_number(2);
	TestDeviceNAME.set_ecu_instance(4);
	TestDeviceNAME.set_function_instance(0);
	TestDeviceNAME.set_device_class_instance(0);
	TestDeviceNAME.set_manufacturer_code(64);

	auto testECU = std::make_shared<isobus::InternalControlFunction>(TestDeviceNAME, 0x44, 0);

	HardwareInterfaceCANFrame testFrame = { 0 };

	std::uint32_t waitingTimestamp_ms = SystemTiming::get_timestamp_ms();

	while ((!testECU->get_address_valid()) &&
	       (!SystemTiming::time_expired_ms(waitingTimestamp_ms, 2000)))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	ASSERT_TRUE(testECU->get_address_valid());

	// Get the virtual CAN plugin back to a known state
	while (!testPlugin.get_queue_empty())
	{
		testPlugin.read_frame(testFrame);
	}
	ASSERT_TRUE(testPlugin.get_queue_empty());

	TestGuidanceInterface interfaceUnderTest(testECU, nullptr); // Configured for broadcasts

	// Test fresh state
	EXPECT_EQ(0, interfaceUnderTest.get_number_received_agricultural_guidance_machine_info_message_sources());
	EXPECT_EQ(0, interfaceUnderTest.get_number_received_guidance_system_command_sources());
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_agricultural_guidance_machine_info(0));
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_guidance_system_command(0));
	interfaceUnderTest.test_wrapper_set_flag(0);
	interfaceUnderTest.update(); // Nothing should happen, since not initialized yet
	EXPECT_TRUE(testPlugin.get_queue_empty());

	EXPECT_EQ(0.0f, interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.get_estimated_curvature());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GuidanceLimitStatus::NotAvailable, interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.get_guidance_limit_status());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::NotAvailableTakeNoAction, interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.get_guidance_steering_system_readiness_state());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::NotAvailableTakeNoAction, interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.get_guidance_steering_input_position_status());
	EXPECT_EQ(static_cast<std::uint8_t>(GuidanceInterface::AgriculturalGuidanceMachineInfo::GuidanceSystemCommandExitReasonCode::NotAvailable), interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.get_guidance_system_command_exit_reason_code());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::NotAvailableTakeNoAction, interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.get_guidance_system_remote_engage_switch_status());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::RequestResetCommandStatus::NotAvailable, interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.get_request_reset_command_status());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::MechanicalSystemLockout::NotAvailable, interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.get_mechanical_system_lockout());

	interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.set_estimated_curvature(10.0f);
	EXPECT_NEAR(10.0f, interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.get_estimated_curvature(), 0.01f);

	interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.set_guidance_limit_status(GuidanceInterface::AgriculturalGuidanceMachineInfo::GuidanceLimitStatus::LimitedLow);
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GuidanceLimitStatus::LimitedLow, interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.get_guidance_limit_status());

	interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.set_guidance_steering_input_position_status(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::DisabledOffPassive);
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::DisabledOffPassive, interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.get_guidance_steering_input_position_status());

	interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.set_guidance_steering_system_readiness_state(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive);
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive, interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.get_guidance_steering_system_readiness_state());

	interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.set_guidance_system_remote_engage_switch_status(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive);
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive, interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.get_guidance_system_remote_engage_switch_status());

	interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.set_mechanical_system_lockout_state(GuidanceInterface::AgriculturalGuidanceMachineInfo::MechanicalSystemLockout::NotActive);
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::MechanicalSystemLockout::NotActive, interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.get_mechanical_system_lockout());

	interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.set_request_reset_command_status(GuidanceInterface::AgriculturalGuidanceMachineInfo::RequestResetCommandStatus::ResetNotRequired);
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::RequestResetCommandStatus::ResetNotRequired, interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.get_request_reset_command_status());

	interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.set_guidance_system_command_exit_reason_code(27);
	EXPECT_EQ(27, interfaceUnderTest.AgriculturalGuidanceMachineInfoTransmitData.get_guidance_system_command_exit_reason_code());

	ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_guidance_info());
	ASSERT_TRUE(testPlugin.read_frame(testFrame));

	// Validate message encoding
	EXPECT_EQ(0, testFrame.channel);
	EXPECT_EQ(8, testFrame.dataLength);
	EXPECT_EQ(0x0CACFF44, testFrame.identifier);

	std::uint16_t decodedCurvature = static_cast<std::uint16_t>(testFrame.data[0]) | (static_cast<std::uint16_t>(testFrame.data[1]) << 8);
	float descaledCurvature = (decodedCurvature * 0.25f) - 8032.0f;
	EXPECT_NEAR(descaledCurvature, 10.0f, 0.24f);

	EXPECT_EQ(0, (testFrame.data[2] & 0x03));
	EXPECT_EQ(1, ((testFrame.data[2] >> 2) & 0x03));
	EXPECT_EQ(0, ((testFrame.data[2] >> 4) & 0x03));
	EXPECT_EQ(0, ((testFrame.data[2] >> 6) & 0x03));
	EXPECT_EQ(3, ((testFrame.data[3] >> 5) & 0x07));
	EXPECT_EQ(27, ((testFrame.data[4]) & 0x3F));
	EXPECT_EQ(1, ((testFrame.data[4] >> 6) & 0x03));
	EXPECT_EQ(0xFF, testFrame.data[5]);
	EXPECT_EQ(0xFF, testFrame.data[6]);
	EXPECT_EQ(0xFF, testFrame.data[7]);

	// Test the command message next. It's much simpler.
	interfaceUnderTest.GuidanceSystemCommandTransmitData.set_curvature(-43.4f);
	interfaceUnderTest.GuidanceSystemCommandTransmitData.set_status(GuidanceInterface::GuidanceSystemCommand::CurvatureCommandStatus::IntendedToSteer);
	ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_guidance_system_command());
	ASSERT_TRUE(testPlugin.read_frame(testFrame));

	decodedCurvature = static_cast<std::uint16_t>(testFrame.data[0]) | (static_cast<std::uint16_t>(testFrame.data[1]) << 8);
	descaledCurvature = (decodedCurvature * 0.25f) - 8032.0f;
	EXPECT_NEAR(descaledCurvature, -43.5f, 0.24f);

	EXPECT_EQ(1, (testFrame.data[2] & 0x03));

	CANHardwareInterface::stop();
	testPlugin.close();
}

TEST(GUIDANCE_TESTS, ListenOnlyModeAndDecoding)
{
	TestGuidanceInterface interfaceUnderTest(nullptr, nullptr);
	HardwareInterfaceCANFrame testFrame = { 0 };

	EXPECT_FALSE(interfaceUnderTest.test_wrapper_send_guidance_system_command());
	EXPECT_FALSE(interfaceUnderTest.test_wrapper_send_guidance_info());

	CANNetworkManager::CANNetwork.update();

	interfaceUnderTest.initialize();

	EXPECT_EQ(0, interfaceUnderTest.get_number_received_agricultural_guidance_machine_info_message_sources());
	EXPECT_EQ(0, interfaceUnderTest.get_number_received_guidance_system_command_sources());
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_agricultural_guidance_machine_info(0));
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_guidance_system_command(0));

	// Force claim some other ECU
	testFrame.dataLength = 8;
	testFrame.channel = 0;
	testFrame.isExtendedFrame = true;
	testFrame.identifier = 0x18EEFF46;
	testFrame.data[0] = 0x03;
	testFrame.data[1] = 0x05;
	testFrame.data[2] = 0x04;
	testFrame.data[3] = 0x12;
	testFrame.data[4] = 0x00;
	testFrame.data[5] = 0x82;
	testFrame.data[6] = 0x01;
	testFrame.data[7] = 0xA0;
	CANNetworkManager::process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	std::uint16_t testCurvature = std::roundf(4 * ((94.25 + 8032) / 0.25)) / 4.0f; // manually encode a curvature of 94.25 km-1
	testFrame.dataLength = 8;
	testFrame.identifier = 0xCADFF46;
	testFrame.data[0] = static_cast<std::uint8_t>(testCurvature & 0xFF);
	testFrame.data[1] = static_cast<std::uint8_t>((testCurvature >> 8) & 0xFF);
	testFrame.data[2] = 0xFD; // Intended to steer + reserved bits set to 1
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0xFF;
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0xFF;
	testFrame.data[7] = 0xFF;
	CANNetworkManager::process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(0, interfaceUnderTest.get_number_received_agricultural_guidance_machine_info_message_sources());
	EXPECT_EQ(1, interfaceUnderTest.get_number_received_guidance_system_command_sources());
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_agricultural_guidance_machine_info(0));
	EXPECT_NE(nullptr, interfaceUnderTest.get_received_guidance_system_command(0));
}
