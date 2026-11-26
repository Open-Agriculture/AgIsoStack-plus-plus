//================================================================================================
/// @file vt_client_state_tracker_tests.cpp
///
/// @brief Unit tests for the Virtual Terminal client state tracker.
///
/// @author Sujan Dumaru
///
/// @copyright 2026 The Open-Agriculture Developers
//================================================================================================
#include <gtest/gtest.h>

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"
#include "helpers/test_fixture.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_virtual_terminal_client_state_tracker.hpp"

#include <memory>

using namespace isobus;

class VirtualTerminalClientStateTrackerTest : public AgIsoStackTestFixture
{
protected:
	void SetUp() override
	{
		AgIsoStackTestFixture::SetUp();

		CANHardwareInterface::set_number_of_can_channels(1);
		CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
		CANHardwareInterface::start(false);
		CANNetworkManager::CANNetwork.update(); // The network manager drops received frames until it has been updated once
	}

	void start_tracker(std::shared_ptr<ControlFunction> client)
	{
		tracker.reset(new VirtualTerminalClientStateTracker(client));
		tracker->initialize();
	}

	void receive_to_ecu(std::shared_ptr<ControlFunction> vt, std::shared_ptr<ControlFunction> destination, std::initializer_list<std::uint8_t> data)
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(5, static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), destination, vt, data));
		CANNetworkManager::CANNetwork.update();
	}

	void receive_status(std::shared_ptr<ControlFunction> vt, std::uint8_t workingSetAddress, std::uint16_t dataMask)
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(5, static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), vt, { 0xFE, workingSetAddress, static_cast<std::uint8_t>(dataMask & 0xFF), static_cast<std::uint8_t>(dataMask >> 8), 0xFF, 0xFF, 0xFF, 0xFF }));
		CANNetworkManager::CANNetwork.update();
	}

	void receive_change_active_mask_response(std::shared_ptr<ControlFunction> vt, std::shared_ptr<ControlFunction> destination, std::uint16_t maskId)
	{
		receive_to_ecu(vt, destination, { 0xAD, static_cast<std::uint8_t>(maskId & 0xFF), static_cast<std::uint8_t>(maskId >> 8), 0x00, 0xFF, 0xFF, 0xFF, 0xFF });
	}

	std::unique_ptr<VirtualTerminalClientStateTracker> tracker;
};

TEST_F(VirtualTerminalClientStateTrackerTest, ShortFramesAreIgnored)
{
	auto client = test_helpers::claim_internal_control_function(0x50, 0, time_source);
	auto vt = test_helpers::force_claim_partnered_control_function(0x51, 0);
	start_tracker(client);
	receive_status(vt, 0x50, 1000);
	tracker->add_tracked_soft_key_mask(1000, 2000);
	ASSERT_TRUE(tracker->is_working_set_active());
	ASSERT_EQ(1000, tracker->get_active_mask());

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(5, static_cast<std::uint32_t>(CANLibParameterGroupNumber::VirtualTerminalToECU), vt, { 0xFE, 0x01, 0xE9 }));
	CANNetworkManager::CANNetwork.update();
	receive_to_ecu(vt, client, { 0xFE, 0x01, 0xE9 });
	receive_to_ecu(vt, client, { 0xAD, 0xE9, 0x03 });
	receive_to_ecu(vt, client, { 0xAE, 0xE8, 0x03 });

	EXPECT_TRUE(tracker->is_working_set_active());
	EXPECT_EQ(1000, tracker->get_active_mask());
	EXPECT_EQ(2000, tracker->get_soft_key_mask(1000));
}

TEST_F(VirtualTerminalClientStateTrackerTest, ChangeSoftKeyMaskResponseReadsErrorAtByte6)
{
	auto client = test_helpers::claim_internal_control_function(0x52, 0, time_source);
	auto vt = test_helpers::force_claim_partnered_control_function(0x53, 0);
	start_tracker(client);
	receive_status(vt, 0x52, 1000);
	tracker->add_tracked_soft_key_mask(1000, 2000);

	receive_to_ecu(vt, client, { 0xAE, 0xE8, 0x03, 0xD1, 0x07, 0x00, 0xFF, 0xFF });
	EXPECT_EQ(2001, tracker->get_soft_key_mask(1000));

	receive_to_ecu(vt, client, { 0xAE, 0xE8, 0x03, 0xD2, 0x07, 0x01, 0xFF, 0xFF });
	EXPECT_EQ(2001, tracker->get_soft_key_mask(1000));
}

TEST_F(VirtualTerminalClientStateTrackerTest, StatusFromAnotherVtIsIgnored)
{
	auto client = test_helpers::claim_internal_control_function(0x54, 0, time_source);
	auto vtA = test_helpers::force_claim_partnered_control_function(0x55, 0);
	auto vtB = test_helpers::force_claim_partnered_control_function(0x56, 0);
	start_tracker(client);
	receive_status(vtA, 0x54, 1000);
	ASSERT_TRUE(tracker->is_working_set_active());

	receive_status(vtB, 0x01, 1001);

	EXPECT_TRUE(tracker->is_working_set_active());
	EXPECT_EQ(1000, tracker->get_active_mask());
}

TEST_F(VirtualTerminalClientStateTrackerTest, AnotherVtNamingOurClientTakesOver)
{
	auto client = test_helpers::claim_internal_control_function(0x5C, 0, time_source);
	auto vtA = test_helpers::force_claim_partnered_control_function(0x5D, 0);
	auto vtB = test_helpers::force_claim_partnered_control_function(0x5E, 0);
	start_tracker(client);
	receive_status(vtA, 0x5C, 1000);

	receive_status(vtB, 0x5C, 1001);
	EXPECT_EQ(1001, tracker->get_active_mask());

	receive_status(vtA, 0x01, 1002);
	EXPECT_TRUE(tracker->is_working_set_active());

	receive_change_active_mask_response(vtB, client, 1003);
	EXPECT_EQ(1003, tracker->get_active_mask());
}

TEST_F(VirtualTerminalClientStateTrackerTest, OversizedMessagesAreIgnored)
{
	auto client = test_helpers::claim_internal_control_function(0x5F, 0, time_source);
	auto vt = test_helpers::force_claim_partnered_control_function(0x60, 0);
	start_tracker(client);
	receive_status(vt, 0x5F, 1000);

	// 9 byte VT Status as a transport protocol BAM
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(7, 0xEC00, vt, { 0x20, 0x09, 0x00, 0x02, 0xFF, 0x00, 0xE6, 0x00 }));
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(7, 0xEB00, vt, { 0x01, 0xFE, 0x5F, 0xE9, 0x03, 0xFF, 0xFF, 0xFF }));
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(7, 0xEB00, vt, { 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }));
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(1000, tracker->get_active_mask());
}

TEST_F(VirtualTerminalClientStateTrackerTest, ResponseToAnotherWorkingSetIsIgnored)
{
	auto client = test_helpers::claim_internal_control_function(0x57, 0, time_source);
	auto otherClient = test_helpers::claim_internal_control_function(0x58, 0, time_source);
	auto vt = test_helpers::force_claim_partnered_control_function(0x59, 0);
	start_tracker(client);
	receive_status(vt, 0x57, 1000);
	ASSERT_EQ(1000, tracker->get_active_mask());

	receive_change_active_mask_response(vt, otherClient, 1001);
	EXPECT_EQ(1000, tracker->get_active_mask());

	receive_change_active_mask_response(vt, client, 1001);
	EXPECT_EQ(1001, tracker->get_active_mask());
}

TEST_F(VirtualTerminalClientStateTrackerTest, ShrinkingMaxMaskHistoryTrimsOldest)
{
	auto client = test_helpers::claim_internal_control_function(0x5A, 0, time_source);
	auto vt = test_helpers::force_claim_partnered_control_function(0x5B, 0);
	start_tracker(client);
	receive_status(vt, 0x5A, 1000);
	receive_change_active_mask_response(vt, client, 1001);
	receive_change_active_mask_response(vt, client, 1002);
	receive_change_active_mask_response(vt, client, 1003);
	ASSERT_EQ(3, tracker->get_mask_history().size());

	tracker->set_max_mask_history_size(1);

	ASSERT_EQ(1, tracker->get_mask_history().size());
	EXPECT_EQ(1002, tracker->get_mask_history().front());
}
