#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_guidance_interface.hpp"
#include "isobus/utility/system_timing.hpp"

#include <cmath>

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

	static void test_guidance_system_command_callback(const std::shared_ptr<GuidanceSystemCommand>)
	{
		wasGuidanceSystemCommandCallbackHit = true;
	}

	static void test_guidance_info_callback(const std::shared_ptr<AgriculturalGuidanceMachineInfo>)
	{
		wasGuidanceInfoCallbackHit = true;
	}

	static bool wasGuidanceSystemCommandCallbackHit;
	static bool wasGuidanceInfoCallbackHit;
};

bool TestGuidanceInterface::wasGuidanceSystemCommandCallbackHit = false;
bool TestGuidanceInterface::wasGuidanceInfoCallbackHit = false;

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

	HardwareInterfaceCANFrame testFrame;
	testFrame.timestamp_us = 0;
	testFrame.identifier = 0;
	testFrame.channel = 0;
	std::memset(testFrame.data, 0, sizeof(testFrame.data));
	testFrame.dataLength = 0; ///< The length of the data used in the frame
	testFrame.isExtendedFrame = true; ///< Denotes if the frame is extended format

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

	EXPECT_EQ(0.0f, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_estimated_curvature());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GuidanceLimitStatus::NotAvailable, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_guidance_limit_status());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::NotAvailableTakeNoAction, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_guidance_steering_system_readiness_state());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::NotAvailableTakeNoAction, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_guidance_steering_input_position_status());
	EXPECT_EQ(static_cast<std::uint8_t>(GuidanceInterface::AgriculturalGuidanceMachineInfo::GuidanceSystemCommandExitReasonCode::NotAvailable), interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_guidance_system_command_exit_reason_code());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::NotAvailableTakeNoAction, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_guidance_system_remote_engage_switch_status());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::RequestResetCommandStatus::NotAvailable, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_request_reset_command_status());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::MechanicalSystemLockout::NotAvailable, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_mechanical_system_lockout());

	interfaceUnderTest.initialize();
	EXPECT_TRUE(interfaceUnderTest.get_initialized());
	interfaceUnderTest.initialize();
	EXPECT_TRUE(interfaceUnderTest.get_initialized());

	interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.set_estimated_curvature(10.0f);
	EXPECT_NEAR(10.0f, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_estimated_curvature(), 0.01f);

	interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.set_guidance_limit_status(GuidanceInterface::AgriculturalGuidanceMachineInfo::GuidanceLimitStatus::LimitedLow);
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GuidanceLimitStatus::LimitedLow, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_guidance_limit_status());

	interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.set_guidance_steering_input_position_status(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::DisabledOffPassive);
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::DisabledOffPassive, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_guidance_steering_input_position_status());

	interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.set_guidance_steering_system_readiness_state(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive);
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_guidance_steering_system_readiness_state());

	interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.set_guidance_system_remote_engage_switch_status(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive);
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_guidance_system_remote_engage_switch_status());

	interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.set_mechanical_system_lockout_state(GuidanceInterface::AgriculturalGuidanceMachineInfo::MechanicalSystemLockout::NotActive);
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::MechanicalSystemLockout::NotActive, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_mechanical_system_lockout());

	interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.set_request_reset_command_status(GuidanceInterface::AgriculturalGuidanceMachineInfo::RequestResetCommandStatus::ResetNotRequired);
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::RequestResetCommandStatus::ResetNotRequired, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_request_reset_command_status());

	interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.set_guidance_system_command_exit_reason_code(27);
	EXPECT_EQ(27, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_guidance_system_command_exit_reason_code());

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
	interfaceUnderTest.guidanceSystemCommandTransmitData.set_curvature(-43.4f);
	interfaceUnderTest.guidanceSystemCommandTransmitData.set_status(GuidanceInterface::GuidanceSystemCommand::CurvatureCommandStatus::IntendedToSteer);
	EXPECT_NEAR(-43.5f, interfaceUnderTest.guidanceSystemCommandTransmitData.get_curvature(), 0.24f); // This also tests rounding to the nearest 0.25 km-1
	EXPECT_EQ(GuidanceInterface::GuidanceSystemCommand::CurvatureCommandStatus::IntendedToSteer, interfaceUnderTest.guidanceSystemCommandTransmitData.get_status());
	ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_guidance_system_command());
	ASSERT_TRUE(testPlugin.read_frame(testFrame));

	decodedCurvature = static_cast<std::uint16_t>(testFrame.data[0]) | (static_cast<std::uint16_t>(testFrame.data[1]) << 8);
	descaledCurvature = (decodedCurvature * 0.25f) - 8032.0f;
	EXPECT_NEAR(descaledCurvature, -43.5f, 0.24f);

	EXPECT_EQ(1, (testFrame.data[2] & 0x03));

	EXPECT_NE(nullptr, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_sender_control_function());
	EXPECT_NE(nullptr, interfaceUnderTest.guidanceSystemCommandTransmitData.get_sender_control_function());

	std::this_thread::sleep_for(std::chrono::milliseconds(105));
	interfaceUnderTest.update();
	ASSERT_TRUE(testPlugin.read_frame(testFrame)); // Message should get sent on a 100ms interval

	CANHardwareInterface::stop();
	testPlugin.close();
}

TEST(GUIDANCE_TESTS, ListenOnlyModeAndDecoding)
{
	TestGuidanceInterface interfaceUnderTest(nullptr, nullptr);
	HardwareInterfaceCANFrame testFrame;

	testFrame.timestamp_us = 0;
	testFrame.identifier = 0;
	testFrame.channel = 0;
	std::memset(testFrame.data, 0, sizeof(testFrame.data));
	testFrame.dataLength = 0;
	testFrame.isExtendedFrame = true;

	EXPECT_FALSE(interfaceUnderTest.test_wrapper_send_guidance_system_command());
	EXPECT_FALSE(interfaceUnderTest.test_wrapper_send_guidance_info());
	EXPECT_EQ(nullptr, interfaceUnderTest.agriculturalGuidanceMachineInfoTransmitData.get_sender_control_function());
	EXPECT_EQ(nullptr, interfaceUnderTest.guidanceSystemCommandTransmitData.get_sender_control_function());

	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(false, interfaceUnderTest.get_initialized());
	interfaceUnderTest.initialize();
	EXPECT_EQ(true, interfaceUnderTest.get_initialized());

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

	// Register callbacks to test
	auto guidanceCommandListener = interfaceUnderTest.get_guidance_guidance_system_command_event_publisher().add_listener(TestGuidanceInterface::test_guidance_system_command_callback);
	auto guidanceInfoListener = interfaceUnderTest.get_agricultural_guidance_machine_info_event_publisher().add_listener(TestGuidanceInterface::test_guidance_info_callback);
	EXPECT_EQ(false, TestGuidanceInterface::wasGuidanceInfoCallbackHit);
	EXPECT_EQ(false, TestGuidanceInterface::wasGuidanceSystemCommandCallbackHit);

	// Test commanded curvature
	std::uint16_t testCurvature = std::roundf(4 * ((94.25f + 8032) / 0.25f)) / 4.0f; // manually encode a curvature of 94.25 km-1
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

	EXPECT_EQ(false, TestGuidanceInterface::wasGuidanceInfoCallbackHit);
	EXPECT_EQ(true, TestGuidanceInterface::wasGuidanceSystemCommandCallbackHit);
	TestGuidanceInterface::wasGuidanceSystemCommandCallbackHit = false;

	EXPECT_EQ(0, interfaceUnderTest.get_number_received_agricultural_guidance_machine_info_message_sources());
	EXPECT_EQ(1, interfaceUnderTest.get_number_received_guidance_system_command_sources());
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_agricultural_guidance_machine_info(0));
	ASSERT_NE(nullptr, interfaceUnderTest.get_received_guidance_system_command(0));

	auto guidanceCommand = interfaceUnderTest.get_received_guidance_system_command(0);

	EXPECT_NEAR(94.25, guidanceCommand->get_curvature(), 0.2f);
	EXPECT_EQ(GuidanceInterface::GuidanceSystemCommand::CurvatureCommandStatus::IntendedToSteer, guidanceCommand->get_status());

	// Test estimated curvature
	testCurvature = std::roundf(4 * ((-47.75f + 8032) / 0.25f)) / 4.0f; // manually encode a curvature of -47.75 km-1
	testFrame.identifier = 0xCACFF46;
	testFrame.data[0] = static_cast<std::uint8_t>(testCurvature & 0xFF);
	testFrame.data[1] = static_cast<std::uint8_t>((testCurvature >> 8) & 0xFF);
	testFrame.data[2] = 0x55; // All components set to 1
	testFrame.data[3] = (0x07 << 5); // NA
	testFrame.data[4] = (0x24 | (1 << 6)); // Exit code 36 and 1 for engage switch
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0xFF;
	testFrame.data[7] = 0xFF;
	CANNetworkManager::process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(true, TestGuidanceInterface::wasGuidanceInfoCallbackHit);
	EXPECT_EQ(false, TestGuidanceInterface::wasGuidanceSystemCommandCallbackHit);

	EXPECT_EQ(1, interfaceUnderTest.get_number_received_agricultural_guidance_machine_info_message_sources());
	EXPECT_EQ(1, interfaceUnderTest.get_number_received_guidance_system_command_sources());
	EXPECT_NE(nullptr, interfaceUnderTest.get_received_agricultural_guidance_machine_info(0));
	EXPECT_NE(nullptr, interfaceUnderTest.get_received_guidance_system_command(0));

	auto estimatedCurvatureInfo = interfaceUnderTest.get_received_agricultural_guidance_machine_info(0);
	EXPECT_NEAR(estimatedCurvatureInfo->get_estimated_curvature(), -47.75f, 0.2f);
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GuidanceLimitStatus::NotAvailable, estimatedCurvatureInfo->get_guidance_limit_status());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive, estimatedCurvatureInfo->get_guidance_steering_input_position_status());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive, estimatedCurvatureInfo->get_guidance_steering_system_readiness_state());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive, estimatedCurvatureInfo->get_guidance_system_remote_engage_switch_status());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::MechanicalSystemLockout::Active, estimatedCurvatureInfo->get_mechanical_system_lockout());
	EXPECT_EQ(GuidanceInterface::AgriculturalGuidanceMachineInfo::RequestResetCommandStatus::ResetRequired, estimatedCurvatureInfo->get_request_reset_command_status());

	// Make a slightly different value to confirm we don't add a duplicate source
	testCurvature = std::roundf(4 * ((-44.75f + 8032) / 0.25f)) / 4.0f; // manually encode a curvature of -47.75 km-1
	testFrame.identifier = 0xCACFF46;
	testFrame.data[0] = static_cast<std::uint8_t>(testCurvature & 0xFF);
	testFrame.data[1] = static_cast<std::uint8_t>((testCurvature >> 8) & 0xFF);
	testFrame.data[2] = 0x00; // All components set to 1
	testFrame.data[3] = (0x04 << 5); // NA
	testFrame.data[4] = (0x24 | (0 << 6)); // Exit code 36 and 1 for engage switch
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0xFF;
	testFrame.data[7] = 0xFF;
	CANNetworkManager::process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(1, interfaceUnderTest.get_number_received_agricultural_guidance_machine_info_message_sources());
	EXPECT_EQ(1, interfaceUnderTest.get_number_received_guidance_system_command_sources());
	EXPECT_NE(nullptr, interfaceUnderTest.get_received_agricultural_guidance_machine_info(0));
	EXPECT_NE(nullptr, interfaceUnderTest.get_received_guidance_system_command(0));

	// Test different commanded curvature doesn't cause duplicates
	testCurvature = std::roundf(4 * ((99.25f + 8032) / 0.25f)) / 4.0f; // manually encode a curvature of 99.25 km-1
	testFrame.dataLength = 8;
	testFrame.identifier = 0xCADFF46;
	testFrame.data[0] = static_cast<std::uint8_t>(testCurvature & 0xFF);
	testFrame.data[1] = static_cast<std::uint8_t>((testCurvature >> 8) & 0xFF);
	testFrame.data[2] = 0xFF;
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0xFF;
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0xFF;
	testFrame.data[7] = 0xFF;
	CANNetworkManager::process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(1, interfaceUnderTest.get_number_received_agricultural_guidance_machine_info_message_sources());
	EXPECT_EQ(1, interfaceUnderTest.get_number_received_guidance_system_command_sources());
	EXPECT_NE(nullptr, interfaceUnderTest.get_received_agricultural_guidance_machine_info(0));
	EXPECT_NE(nullptr, interfaceUnderTest.get_received_guidance_system_command(0));

	// Test timeouts
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	interfaceUnderTest.update();
	EXPECT_EQ(0, interfaceUnderTest.get_number_received_agricultural_guidance_machine_info_message_sources());
	EXPECT_EQ(0, interfaceUnderTest.get_number_received_guidance_system_command_sources());
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_agricultural_guidance_machine_info(0));
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_guidance_system_command(0));
}
