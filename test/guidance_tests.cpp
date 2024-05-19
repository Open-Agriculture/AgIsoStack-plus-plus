#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_guidance_interface.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"

#include <cmath>

using namespace isobus;

class TestGuidanceInterface : public AgriculturalGuidanceInterface
{
public:
	TestGuidanceInterface(std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination) :
	  AgriculturalGuidanceInterface(source, destination){

	  };

	TestGuidanceInterface(std::shared_ptr<InternalControlFunction> source, std::shared_ptr<ControlFunction> destination, bool sendSystemCommandPeriodically, bool sendMachineInfoPeriodically) :
	  AgriculturalGuidanceInterface(source, destination, sendSystemCommandPeriodically, sendMachineInfoPeriodically){

	  };

	void test_wrapper_set_flag(std::uint32_t flag)
	{
		txFlags.set_flag(flag);
	}

	bool test_wrapper_send_guidance_system_command() const
	{
		return send_guidance_system_command();
	}

	bool test_wrapper_send_guidance_machine_info() const
	{
		return send_guidance_machine_info();
	}

	static void test_guidance_system_command_callback(const std::shared_ptr<GuidanceSystemCommand>, bool)
	{
		wasGuidanceSystemCommandCallbackHit = true;
	}

	static void test_guidance_machine_info_callback(const std::shared_ptr<GuidanceMachineInfo>, bool)
	{
		wasGuidanceMachineInfoCallbackHit = true;
	}

	static bool wasGuidanceSystemCommandCallbackHit;
	static bool wasGuidanceMachineInfoCallbackHit;
};

bool TestGuidanceInterface::wasGuidanceSystemCommandCallbackHit = false;
bool TestGuidanceInterface::wasGuidanceMachineInfoCallbackHit = false;

TEST(GUIDANCE_TESTS, GuidanceMessages)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	auto testECU = test_helpers::claim_internal_control_function(0x44, 0);

	// Get the virtual CAN plugin back to a known state
	CANMessageFrame testFrame = {};
	while (!testPlugin.get_queue_empty())
	{
		testPlugin.read_frame(testFrame);
	}
	ASSERT_TRUE(testPlugin.get_queue_empty());

	{
		TestGuidanceInterface interfaceUnderTest(testECU, nullptr); // Configured for broadcasts, but no message is configured periodically
		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_guidance_machine_info());
		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_guidance_system_command());

		// Test fresh state
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_guidance_machine_info_message_sources());
		EXPECT_EQ(0, interfaceUnderTest.get_number_received_guidance_system_command_sources());
		EXPECT_EQ(nullptr, interfaceUnderTest.get_received_guidance_machine_info(0));
		EXPECT_EQ(nullptr, interfaceUnderTest.get_received_guidance_system_command(0));
		interfaceUnderTest.test_wrapper_set_flag(0);
		interfaceUnderTest.update(); // Nothing should happen, since not initialized yet
		EXPECT_TRUE(testPlugin.get_queue_empty());

		EXPECT_EQ(0.0f, interfaceUnderTest.guidanceMachineInfoTransmitData.get_estimated_curvature());
		EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::GuidanceLimitStatus::NotAvailable, interfaceUnderTest.guidanceMachineInfoTransmitData.get_guidance_limit_status());
		EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::GenericSAEbs02SlotValue::NotAvailableTakeNoAction, interfaceUnderTest.guidanceMachineInfoTransmitData.get_guidance_steering_system_readiness_state());
		EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::GenericSAEbs02SlotValue::NotAvailableTakeNoAction, interfaceUnderTest.guidanceMachineInfoTransmitData.get_guidance_steering_input_position_status());
		EXPECT_EQ(static_cast<std::uint8_t>(AgriculturalGuidanceInterface::GuidanceMachineInfo::GuidanceSystemCommandExitReasonCode::NotAvailable), interfaceUnderTest.guidanceMachineInfoTransmitData.get_guidance_system_command_exit_reason_code());
		EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::GenericSAEbs02SlotValue::NotAvailableTakeNoAction, interfaceUnderTest.guidanceMachineInfoTransmitData.get_guidance_system_remote_engage_switch_status());
		EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::RequestResetCommandStatus::NotAvailable, interfaceUnderTest.guidanceMachineInfoTransmitData.get_request_reset_command_status());
		EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::MechanicalSystemLockout::NotAvailable, interfaceUnderTest.guidanceMachineInfoTransmitData.get_mechanical_system_lockout());
	}

	{
		TestGuidanceInterface interfaceUnderTest(testECU, nullptr, false, true); // Configured for broadcasts, and only guidance machine info is sent periodically

		interfaceUnderTest.guidanceMachineInfoTransmitData.set_estimated_curvature(10.0f);
		EXPECT_NEAR(10.0f, interfaceUnderTest.guidanceMachineInfoTransmitData.get_estimated_curvature(), 0.01f);

		interfaceUnderTest.guidanceMachineInfoTransmitData.set_guidance_limit_status(AgriculturalGuidanceInterface::GuidanceMachineInfo::GuidanceLimitStatus::LimitedLow);
		EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::GuidanceLimitStatus::LimitedLow, interfaceUnderTest.guidanceMachineInfoTransmitData.get_guidance_limit_status());

		interfaceUnderTest.guidanceMachineInfoTransmitData.set_guidance_steering_input_position_status(AgriculturalGuidanceInterface::GuidanceMachineInfo::GenericSAEbs02SlotValue::DisabledOffPassive);
		EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::GenericSAEbs02SlotValue::DisabledOffPassive, interfaceUnderTest.guidanceMachineInfoTransmitData.get_guidance_steering_input_position_status());

		interfaceUnderTest.guidanceMachineInfoTransmitData.set_guidance_steering_system_readiness_state(AgriculturalGuidanceInterface::GuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive);
		EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive, interfaceUnderTest.guidanceMachineInfoTransmitData.get_guidance_steering_system_readiness_state());

		interfaceUnderTest.guidanceMachineInfoTransmitData.set_guidance_system_remote_engage_switch_status(AgriculturalGuidanceInterface::GuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive);
		EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive, interfaceUnderTest.guidanceMachineInfoTransmitData.get_guidance_system_remote_engage_switch_status());

		interfaceUnderTest.guidanceMachineInfoTransmitData.set_mechanical_system_lockout_state(AgriculturalGuidanceInterface::GuidanceMachineInfo::MechanicalSystemLockout::NotActive);
		EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::MechanicalSystemLockout::NotActive, interfaceUnderTest.guidanceMachineInfoTransmitData.get_mechanical_system_lockout());

		interfaceUnderTest.guidanceMachineInfoTransmitData.set_request_reset_command_status(AgriculturalGuidanceInterface::GuidanceMachineInfo::RequestResetCommandStatus::ResetNotRequired);
		EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::RequestResetCommandStatus::ResetNotRequired, interfaceUnderTest.guidanceMachineInfoTransmitData.get_request_reset_command_status());

		interfaceUnderTest.guidanceMachineInfoTransmitData.set_guidance_system_command_exit_reason_code(27);
		EXPECT_EQ(27, interfaceUnderTest.guidanceMachineInfoTransmitData.get_guidance_system_command_exit_reason_code());

		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_guidance_system_command());
		ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_guidance_machine_info());
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
	}

	{
		TestGuidanceInterface interfaceUnderTest(testECU, nullptr, true, false);
		// Test the command message next. It's much simpler.
		interfaceUnderTest.guidanceSystemCommandTransmitData.set_curvature(-43.4f);
		interfaceUnderTest.guidanceSystemCommandTransmitData.set_status(AgriculturalGuidanceInterface::GuidanceSystemCommand::CurvatureCommandStatus::IntendedToSteer);
		EXPECT_NEAR(-43.5f, interfaceUnderTest.guidanceSystemCommandTransmitData.get_curvature(), 0.24f); // This also tests rounding to the nearest 0.25 km-1
		EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceSystemCommand::CurvatureCommandStatus::IntendedToSteer, interfaceUnderTest.guidanceSystemCommandTransmitData.get_status());

		ASSERT_FALSE(interfaceUnderTest.test_wrapper_send_guidance_machine_info());
		ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_guidance_system_command());
		ASSERT_TRUE(testPlugin.read_frame(testFrame));

		std::uint16_t decodedCurvature = static_cast<std::uint16_t>(testFrame.data[0]) | (static_cast<std::uint16_t>(testFrame.data[1]) << 8);
		float descaledCurvature = (decodedCurvature * 0.25f) - 8032.0f;
		EXPECT_NEAR(descaledCurvature, -43.5f, 0.24f);

		EXPECT_EQ(1, (testFrame.data[2] & 0x03));
	}
	{
		TestGuidanceInterface interfaceUnderTest(testECU, nullptr, true, true); // Configured for broadcasts, and both guidance system command and guidance machine info are sent periodically
		ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_guidance_machine_info());
		ASSERT_TRUE(testPlugin.read_frame(testFrame));
		ASSERT_TRUE(interfaceUnderTest.test_wrapper_send_guidance_system_command());
		ASSERT_TRUE(testPlugin.read_frame(testFrame));
		EXPECT_NE(nullptr, interfaceUnderTest.guidanceMachineInfoTransmitData.get_sender_control_function());
		EXPECT_NE(nullptr, interfaceUnderTest.guidanceSystemCommandTransmitData.get_sender_control_function());

		interfaceUnderTest.initialize();
		EXPECT_TRUE(interfaceUnderTest.get_initialized());
		interfaceUnderTest.initialize();
		EXPECT_TRUE(interfaceUnderTest.get_initialized());

		std::this_thread::sleep_for(std::chrono::milliseconds(105));
		interfaceUnderTest.update();
		ASSERT_TRUE(testPlugin.read_frame(testFrame)); // Message should get sent on a 100ms interval

		CANHardwareInterface::stop();
		testPlugin.close();
	}

	CANNetworkManager::CANNetwork.update(); //! @todo: quick hack for clearing the transmit queue, can be removed once network manager' singleton is removed
	CANNetworkManager::CANNetwork.deactivate_control_function(testECU);
}

TEST(GUIDANCE_TESTS, ListenOnlyModeAndDecoding)
{
	TestGuidanceInterface interfaceUnderTest(nullptr, nullptr);

	EXPECT_FALSE(interfaceUnderTest.test_wrapper_send_guidance_system_command());
	EXPECT_FALSE(interfaceUnderTest.test_wrapper_send_guidance_machine_info());
	EXPECT_EQ(nullptr, interfaceUnderTest.guidanceMachineInfoTransmitData.get_sender_control_function());
	EXPECT_EQ(nullptr, interfaceUnderTest.guidanceSystemCommandTransmitData.get_sender_control_function());

	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(false, interfaceUnderTest.get_initialized());
	interfaceUnderTest.initialize();
	EXPECT_EQ(true, interfaceUnderTest.get_initialized());

	EXPECT_EQ(0, interfaceUnderTest.get_number_received_guidance_machine_info_message_sources());
	EXPECT_EQ(0, interfaceUnderTest.get_number_received_guidance_system_command_sources());
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_guidance_machine_info(0));
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_guidance_system_command(0));

	test_helpers::force_claim_partnered_control_function(0x46, 0);

	// Register callbacks to test
	interfaceUnderTest.get_guidance_system_command_event_publisher().add_listener(TestGuidanceInterface::test_guidance_system_command_callback);
	interfaceUnderTest.get_guidance_machine_info_event_publisher().add_listener(TestGuidanceInterface::test_guidance_machine_info_callback);
	EXPECT_EQ(false, TestGuidanceInterface::wasGuidanceMachineInfoCallbackHit);
	EXPECT_EQ(false, TestGuidanceInterface::wasGuidanceSystemCommandCallbackHit);

	// Test commanded curvature
	std::uint16_t testCurvature = std::roundf(4 * ((94.25f + 8032) / 0.25f)) / 4.0f; // manually encode a curvature of 94.25 km-1
	CANMessageFrame testFrame = {};
	testFrame.identifier = 0xCADFF46;
	testFrame.isExtendedFrame = true;
	testFrame.dataLength = 8;
	testFrame.data[0] = static_cast<std::uint8_t>(testCurvature & 0xFF);
	testFrame.data[1] = static_cast<std::uint8_t>((testCurvature >> 8) & 0xFF);
	testFrame.data[2] = 0xFD; // Intended to steer + reserved bits set to 1
	testFrame.data[3] = 0xFF;
	testFrame.data[4] = 0xFF;
	testFrame.data[5] = 0xFF;
	testFrame.data[6] = 0xFF;
	testFrame.data[7] = 0xFF;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(false, TestGuidanceInterface::wasGuidanceMachineInfoCallbackHit);
	EXPECT_EQ(true, TestGuidanceInterface::wasGuidanceSystemCommandCallbackHit);
	TestGuidanceInterface::wasGuidanceSystemCommandCallbackHit = false;

	EXPECT_EQ(0, interfaceUnderTest.get_number_received_guidance_machine_info_message_sources());
	EXPECT_EQ(1, interfaceUnderTest.get_number_received_guidance_system_command_sources());
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_guidance_machine_info(0));
	ASSERT_NE(nullptr, interfaceUnderTest.get_received_guidance_system_command(0));

	auto guidanceCommand = interfaceUnderTest.get_received_guidance_system_command(0);

	EXPECT_NEAR(94.25, guidanceCommand->get_curvature(), 0.2f);
	EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceSystemCommand::CurvatureCommandStatus::IntendedToSteer, guidanceCommand->get_status());

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
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(true, TestGuidanceInterface::wasGuidanceMachineInfoCallbackHit);
	EXPECT_EQ(false, TestGuidanceInterface::wasGuidanceSystemCommandCallbackHit);

	EXPECT_EQ(1, interfaceUnderTest.get_number_received_guidance_machine_info_message_sources());
	EXPECT_EQ(1, interfaceUnderTest.get_number_received_guidance_system_command_sources());
	EXPECT_NE(nullptr, interfaceUnderTest.get_received_guidance_machine_info(0));
	EXPECT_NE(nullptr, interfaceUnderTest.get_received_guidance_system_command(0));

	auto estimatedCurvatureInfo = interfaceUnderTest.get_received_guidance_machine_info(0);
	EXPECT_NEAR(estimatedCurvatureInfo->get_estimated_curvature(), -47.75f, 0.2f);
	EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::GuidanceLimitStatus::NotAvailable, estimatedCurvatureInfo->get_guidance_limit_status());
	EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive, estimatedCurvatureInfo->get_guidance_steering_input_position_status());
	EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive, estimatedCurvatureInfo->get_guidance_steering_system_readiness_state());
	EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::GenericSAEbs02SlotValue::EnabledOnActive, estimatedCurvatureInfo->get_guidance_system_remote_engage_switch_status());
	EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::MechanicalSystemLockout::Active, estimatedCurvatureInfo->get_mechanical_system_lockout());
	EXPECT_EQ(AgriculturalGuidanceInterface::GuidanceMachineInfo::RequestResetCommandStatus::ResetRequired, estimatedCurvatureInfo->get_request_reset_command_status());

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
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(1, interfaceUnderTest.get_number_received_guidance_machine_info_message_sources());
	EXPECT_EQ(1, interfaceUnderTest.get_number_received_guidance_system_command_sources());
	EXPECT_NE(nullptr, interfaceUnderTest.get_received_guidance_machine_info(0));
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
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(1, interfaceUnderTest.get_number_received_guidance_machine_info_message_sources());
	EXPECT_EQ(1, interfaceUnderTest.get_number_received_guidance_system_command_sources());
	EXPECT_NE(nullptr, interfaceUnderTest.get_received_guidance_machine_info(0));
	EXPECT_NE(nullptr, interfaceUnderTest.get_received_guidance_system_command(0));

	// Test timeouts
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	interfaceUnderTest.update();
	EXPECT_EQ(0, interfaceUnderTest.get_number_received_guidance_machine_info_message_sources());
	EXPECT_EQ(0, interfaceUnderTest.get_number_received_guidance_system_command_sources());
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_guidance_machine_info(0));
	EXPECT_EQ(nullptr, interfaceUnderTest.get_received_guidance_system_command(0));
}
