//================================================================================================
/// @file heartbeat_tests.cpp
///
/// @brief Unit tests for the ISOBUS Heartbeat Message interface.
///
/// @author Adrian Del Grosso
/// @author Sujan Dumaru
///
/// @copyright 2024 The Open-Agriculture Developers
//================================================================================================
#include <gtest/gtest.h>

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"
#include "helpers/test_fixture.hpp"
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/isobus_heartbeat.hpp"

#include <algorithm>
#include <cstring>
#include <utility>
#include <vector>

using namespace isobus;

/// @brief Repetition-rate payload that asks for, or reports, a heartbeat every 100 ms
constexpr std::uint8_t REPETITION_RATE_100_MS[8] = { 0xE4, 0xF0, 0x00, 0x64, 0x00, 0xFF, 0xFF, 0xFF };
/// @brief One millisecond past the 300 ms heartbeat timeout
constexpr std::uint32_t JUST_PAST_TIMEOUT_MS = 301;
/// @brief One millisecond past the 250 ms wait for a Response for Repetition Rate
constexpr std::uint32_t JUST_PAST_RESPONSE_TIMEOUT_MS = 251;

class HeartbeatTest : public AgIsoStackTestFixture
{
protected:
	void SetUp() override
	{
		AgIsoStackTestFixture::SetUp();
		testPlugin.open();

		CANHardwareInterface::set_number_of_can_channels(1);
		CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
		CANHardwareInterface::start(false);

		interface().set_enabled(true);
		CANNetworkManager::CANNetwork.update(); // The network manager drops received frames until it has been updated once

		// Heartbeats left tracked by earlier tests keep raising events, so every count is per control function
		errorListener = interface().get_heartbeat_error_event_dispatcher().add_listener([this](HeartbeatInterface::HeartBeatError error, std::shared_ptr<ControlFunction> controlFunction) {
			errors.push_back(std::make_pair(error, controlFunction));
		});
		recoveredListener = interface().get_heartbeat_recovered_event_dispatcher().add_listener([this](std::shared_ptr<ControlFunction> controlFunction) {
			recovered.push_back(controlFunction);
		});
		newTrackedListener = interface().get_new_tracked_heartbeat_event_dispatcher().add_listener([this](std::shared_ptr<ControlFunction> controlFunction) {
			newTracked.push_back(controlFunction);
		});
		testPlugin.clear_queue();
	}

	void TearDown() override
	{
		interface().get_heartbeat_error_event_dispatcher().remove_listener(errorListener);
		interface().get_heartbeat_recovered_event_dispatcher().remove_listener(recoveredListener);
		interface().get_new_tracked_heartbeat_event_dispatcher().remove_listener(newTrackedListener);
		CANHardwareInterface::stop();
		interface().set_enabled(false);
		AgIsoStackTestFixture::TearDown();
	}

	HeartbeatInterface &interface()
	{
		return CANNetworkManager::CANNetwork.get_heartbeat_interface(0);
	}

	void receive_heartbeat(std::shared_ptr<ControlFunction> source, std::uint8_t counter)
	{
		auto frame = test_helpers::create_message_frame_broadcast(3, static_cast<std::uint32_t>(CANLibParameterGroupNumber::HeartbeatMessage), source, { counter, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF });
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(frame);
		CANNetworkManager::CANNetwork.update();
	}

	void receive_rate_request(std::uint8_t sourceAddress, std::uint8_t destinationAddress, std::uint16_t rate)
	{
		const std::uint32_t identifier = 0x18CC0000 | (static_cast<std::uint32_t>(destinationAddress) << 8) | sourceAddress;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_raw(identifier, { 0xE4, 0xF0, 0x00, static_cast<std::uint8_t>(rate & 0xFF), static_cast<std::uint8_t>(rate >> 8), 0xFF, 0xFF, 0xFF }));
		CANNetworkManager::CANNetwork.update();
		CANNetworkManager::CANNetwork.update();
		time_source.update_for_ms(5);
	}

	void receive_rate_response(std::shared_ptr<ControlFunction> source, std::uint32_t pgn)
	{
		auto frame = test_helpers::create_message_frame_broadcast(6, static_cast<std::uint32_t>(CANLibParameterGroupNumber::ResponseForRepetitionRate), source, { static_cast<std::uint8_t>(pgn & 0xFF), static_cast<std::uint8_t>((pgn >> 8) & 0xFF), static_cast<std::uint8_t>((pgn >> 16) & 0xFF), 0x64, 0x00, 0xFF, 0xFF, 0xFF });
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(frame);
		CANNetworkManager::CANNetwork.update();
	}

	/// @brief Drains the plugin and returns the first data byte of every frame with this identifier
	std::vector<std::uint8_t> first_data_bytes(std::uint32_t identifier)
	{
		std::vector<std::uint8_t> counters;
		CANMessageFrame frame = {};

		while (testPlugin.read_frame(frame))
		{
			if (identifier == frame.identifier)
			{
				counters.push_back(frame.data[0]);
			}
		}
		return counters;
	}

	std::size_t count_frames(std::uint32_t identifier)
	{
		return first_data_bytes(identifier).size();
	}

	std::size_t count_errors(std::shared_ptr<ControlFunction> controlFunction, HeartbeatInterface::HeartBeatError error) const
	{
		return static_cast<std::size_t>(std::count(errors.begin(), errors.end(), std::make_pair(error, controlFunction)));
	}

	std::size_t count_errors(std::shared_ptr<ControlFunction> controlFunction) const
	{
		return static_cast<std::size_t>(std::count_if(errors.begin(), errors.end(), [&controlFunction](const std::pair<HeartbeatInterface::HeartBeatError, std::shared_ptr<ControlFunction>> &entry) {
			return (controlFunction == entry.second);
		}));
	}

	std::size_t count_recovered(std::shared_ptr<ControlFunction> controlFunction) const
	{
		return static_cast<std::size_t>(std::count(recovered.begin(), recovered.end(), controlFunction));
	}

	std::size_t count_new_tracked(std::shared_ptr<ControlFunction> controlFunction) const
	{
		return static_cast<std::size_t>(std::count(newTracked.begin(), newTracked.end(), controlFunction));
	}

	void send_next_heartbeat()
	{
		time_source.update_for_ms(101);
		CANNetworkManager::CANNetwork.update();
		time_source.update_for_ms(5);
	}

	/// @brief Makes the next heartbeat due without sending it: unlike update_for_ms, simulate_delay_ms does not pump the stack
	void make_frame_due()
	{
		time_source.simulate_delay_ms(101);
	}

	VirtualCANPlugin testPlugin;
	std::vector<std::pair<HeartbeatInterface::HeartBeatError, std::shared_ptr<ControlFunction>>> errors;
	std::vector<std::shared_ptr<ControlFunction>> recovered;
	std::vector<std::shared_ptr<ControlFunction>> newTracked;
	EventCallbackHandle errorListener = 0;
	EventCallbackHandle recoveredListener = 0;
	EventCallbackHandle newTrackedListener = 0;
};

TEST_F(HeartbeatTest, HeartBeat)
{
	auto internalECU = test_helpers::claim_internal_control_function(0x41, 0, time_source);
	auto partner = test_helpers::force_claim_partnered_control_function(0xF4, 0);
	testPlugin.clear_queue();

	interface().request_heartbeat(internalECU, partner);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);

	CANMessageFrame frame = {};
	ASSERT_TRUE(testPlugin.read_frame(frame));
	EXPECT_EQ(frame.identifier, 0x18CCF441);
	EXPECT_EQ(frame.dataLength, 8);
	EXPECT_EQ(0, std::memcmp(REPETITION_RATE_100_MS, frame.data, sizeof(REPETITION_RATE_100_MS)));

	receive_rate_request(0xF4, 0x41, 100);

	ASSERT_TRUE(testPlugin.read_frame(frame));
	EXPECT_EQ(frame.identifier, 0x18FE0E41);
	EXPECT_EQ(frame.dataLength, 8);
	EXPECT_EQ(0, std::memcmp(REPETITION_RATE_100_MS, frame.data, sizeof(REPETITION_RATE_100_MS)));

	ASSERT_TRUE(testPlugin.read_frame(frame));
	EXPECT_EQ(frame.identifier, 0x0CF0E441);
	EXPECT_EQ(frame.dataLength, 8);
	const std::uint8_t expectedFirstHeartbeat[8] = { 251, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	EXPECT_EQ(0, std::memcmp(expectedFirstHeartbeat, frame.data, sizeof(expectedFirstHeartbeat)));

	send_next_heartbeat();
	ASSERT_TRUE(testPlugin.read_frame(frame));
	EXPECT_EQ(frame.identifier, 0x0CF0E441);
	EXPECT_EQ(frame.dataLength, 8);
	EXPECT_EQ(frame.data[0], 0);

	EXPECT_EQ(0, count_new_tracked(partner));
	receive_heartbeat(partner, 251);
	EXPECT_EQ(1, count_new_tracked(partner));

	time_source.update_for_ms(400);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1, count_errors(partner, HeartbeatInterface::HeartBeatError::TimedOut));
	EXPECT_EQ(0, count_errors(partner, HeartbeatInterface::HeartBeatError::RequestNotAccepted));

	testPlugin.clear_queue();
	interface().set_enabled(false);
	EXPECT_FALSE(interface().is_enabled());
	time_source.update_for_ms(5);
	EXPECT_TRUE(testPlugin.get_queue_empty());
}

TEST_F(HeartbeatTest, RequestToSendingControlFunctionIsNotRepeated)
{
	auto internalECU = test_helpers::claim_internal_control_function(0x41, 0, time_source);
	auto partner = test_helpers::force_claim_partnered_control_function(0xF4, 0);

	interface().request_heartbeat(internalECU, partner);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);
	receive_heartbeat(partner, 251);
	testPlugin.clear_queue();

	EXPECT_TRUE(interface().request_heartbeat(internalECU, partner));
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);

	EXPECT_EQ(0, count_frames(0x18CCF441));
}

TEST_F(HeartbeatTest, RequestedPartnerThatNeverAnswersIsNotAccepted)
{
	auto internalECU = test_helpers::claim_internal_control_function(0x65, 0, time_source);
	auto silentPartner = test_helpers::force_claim_partnered_control_function(0x67, 0);
	testPlugin.clear_queue();

	EXPECT_TRUE(interface().request_heartbeat(internalECU, silentPartner));
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);
	EXPECT_EQ(1, count_frames(0x18CC6765));

	time_source.simulate_delay_ms(245);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(0, count_errors(silentPartner));

	time_source.simulate_delay_ms(1);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1, count_errors(silentPartner, HeartbeatInterface::HeartBeatError::RequestNotAccepted));

	time_source.update_for_ms(JUST_PAST_TIMEOUT_MS);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1, count_errors(silentPartner));
}

TEST_F(HeartbeatTest, RateResponseAcceptsTheRequest)
{
	auto internalECU = test_helpers::claim_internal_control_function(0x71, 0, time_source);
	auto partner = test_helpers::force_claim_partnered_control_function(0x72, 0);
	EXPECT_TRUE(interface().request_heartbeat(internalECU, partner));
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);

	receive_rate_response(partner, static_cast<std::uint32_t>(CANLibParameterGroupNumber::HeartbeatMessage));
	time_source.simulate_delay_ms(JUST_PAST_RESPONSE_TIMEOUT_MS);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(0, count_errors(partner));

	receive_heartbeat(partner, 251);
	EXPECT_EQ(1, count_new_tracked(partner));
	EXPECT_EQ(0, count_errors(partner));
}

TEST_F(HeartbeatTest, RateResponseForAnotherPgnIsIgnored)
{
	auto internalECU = test_helpers::claim_internal_control_function(0x73, 0, time_source);
	auto partner = test_helpers::force_claim_partnered_control_function(0x74, 0);
	EXPECT_TRUE(interface().request_heartbeat(internalECU, partner));
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);

	receive_rate_response(partner, 0xFEE6);
	time_source.simulate_delay_ms(JUST_PAST_RESPONSE_TIMEOUT_MS);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1, count_errors(partner, HeartbeatInterface::HeartBeatError::RequestNotAccepted));
}

TEST_F(HeartbeatTest, RequestToInternalControlFunctionIsNotTracked)
{
	auto requester = test_helpers::claim_internal_control_function(0x6D, 0, time_source);
	auto target = test_helpers::claim_internal_control_function(0x6E, 0, time_source);
	testPlugin.clear_queue();

	EXPECT_TRUE(interface().request_heartbeat(requester, target));
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);
	for (std::uint8_t i = 0; i < 3; i++)
	{
		send_next_heartbeat();
	}
	time_source.update_for_ms(JUST_PAST_TIMEOUT_MS);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);

	const auto counters = first_data_bytes(0x0CF0E46E);
	EXPECT_EQ(0, std::count(counters.begin(), counters.end(), 255));
	EXPECT_EQ(0, count_errors(target, HeartbeatInterface::HeartBeatError::TimedOut));
}

TEST_F(HeartbeatTest, RetryAtExpiryIsSent)
{
	auto internalECU = test_helpers::claim_internal_control_function(0x6F, 0, time_source);
	auto silentPartner = test_helpers::force_claim_partnered_control_function(0x6B, 0);
	testPlugin.clear_queue();

	EXPECT_TRUE(interface().request_heartbeat(internalECU, silentPartner));
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);
	EXPECT_EQ(1, count_frames(0x18CC6B6F));

	time_source.simulate_delay_ms(JUST_PAST_RESPONSE_TIMEOUT_MS);
	EXPECT_TRUE(interface().request_heartbeat(internalECU, silentPartner));
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);

	EXPECT_EQ(1, count_errors(silentPartner, HeartbeatInterface::HeartBeatError::RequestNotAccepted));
	EXPECT_EQ(1, count_frames(0x18CC6B6F));
}

TEST_F(HeartbeatTest, DisableDropsQueuedCommands)
{
	auto internalECU = test_helpers::claim_internal_control_function(0x68, 0, time_source);
	auto partner = test_helpers::force_claim_partnered_control_function(0x6C, 0);
	testPlugin.clear_queue();

	EXPECT_TRUE(interface().request_heartbeat(internalECU, partner));
	interface().set_enabled(false);
	EXPECT_FALSE(interface().request_heartbeat(internalECU, partner));
	interface().set_enabled(true);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);

	EXPECT_EQ(0, count_frames(0x18CC6C68));
}

// Each test claims its own partner address, because tracked heartbeats and claimed addresses outlive a test in the CANNetwork singleton
class HeartbeatReceiverTest : public HeartbeatTest
{
};

TEST_F(HeartbeatReceiverTest, InitialSequenceCounterThenZero)
{
	auto partner = test_helpers::force_claim_partnered_control_function(0x80, 0);

	// A CF sends 251 once on initialization, then restarts the range at 0
	receive_heartbeat(partner, 251);
	receive_heartbeat(partner, 0);

	EXPECT_EQ(0, count_errors(partner, HeartbeatInterface::HeartBeatError::InvalidSequenceCounter));
}

TEST_F(HeartbeatReceiverTest, SequenceCounterWrap)
{
	auto partner = test_helpers::force_claim_partnered_control_function(0x81, 0);

	receive_heartbeat(partner, 249);
	receive_heartbeat(partner, 250);
	receive_heartbeat(partner, 0);
	receive_heartbeat(partner, 1);

	EXPECT_EQ(0, count_errors(partner, HeartbeatInterface::HeartBeatError::InvalidSequenceCounter));
}

TEST_F(HeartbeatReceiverTest, PerControlFunctionSequenceCounters)
{
	auto firstPartner = test_helpers::force_claim_partnered_control_function(0x82, 0);
	auto secondPartner = test_helpers::force_claim_partnered_control_function(0x83, 0);

	receive_heartbeat(firstPartner, 10);
	receive_heartbeat(secondPartner, 20);
	receive_heartbeat(firstPartner, 11);
	receive_heartbeat(secondPartner, 21);
	receive_heartbeat(firstPartner, 12);
	receive_heartbeat(secondPartner, 22);

	EXPECT_EQ(0, count_errors(firstPartner, HeartbeatInterface::HeartBeatError::InvalidSequenceCounter) + count_errors(secondPartner, HeartbeatInterface::HeartBeatError::InvalidSequenceCounter));
}

TEST_F(HeartbeatReceiverTest, InvalidSequenceCounterStillDetected)
{
	auto partner = test_helpers::force_claim_partnered_control_function(0x84, 0);

	receive_heartbeat(partner, 5);
	receive_heartbeat(partner, 6);
	EXPECT_EQ(0, count_errors(partner, HeartbeatInterface::HeartBeatError::InvalidSequenceCounter));

	receive_heartbeat(partner, 9);
	EXPECT_EQ(0, count_errors(partner, HeartbeatInterface::HeartBeatError::InvalidSequenceCounter));

	receive_heartbeat(partner, 13);
	EXPECT_EQ(1, count_errors(partner, HeartbeatInterface::HeartBeatError::InvalidSequenceCounter));

	receive_heartbeat(partner, 13);
	EXPECT_EQ(1, count_errors(partner, HeartbeatInterface::HeartBeatError::InvalidSequenceCounter));

	receive_heartbeat(partner, 250);
	EXPECT_EQ(1, count_errors(partner, HeartbeatInterface::HeartBeatError::InvalidSequenceCounter));
}

TEST_F(HeartbeatReceiverTest, RecoveryNeedsEightInARow)
{
	auto partner = test_helpers::force_claim_partnered_control_function(0x61, 0);

	receive_heartbeat(partner, 10);
	receive_heartbeat(partner, 20);
	EXPECT_EQ(1, count_errors(partner, HeartbeatInterface::HeartBeatError::InvalidSequenceCounter));

	for (std::uint8_t counter = 21; counter <= 24; counter++)
	{
		receive_heartbeat(partner, counter);
	}
	receive_heartbeat(partner, 26);
	for (std::uint8_t counter = 27; counter <= 33; counter++)
	{
		receive_heartbeat(partner, counter);
	}
	EXPECT_EQ(0, count_recovered(partner));

	receive_heartbeat(partner, 34);
	EXPECT_EQ(1, count_recovered(partner));

	receive_heartbeat(partner, 50);
	EXPECT_EQ(2, count_errors(partner, HeartbeatInterface::HeartBeatError::InvalidSequenceCounter));
}

TEST_F(HeartbeatReceiverTest, TransmissionErrorThenResync)
{
	auto partner = test_helpers::force_claim_partnered_control_function(0x62, 0);

	receive_heartbeat(partner, 10);
	receive_heartbeat(partner, 254);
	EXPECT_EQ(1, count_errors(partner, HeartbeatInterface::HeartBeatError::TransmissionError));

	receive_heartbeat(partner, 254);
	EXPECT_EQ(1, count_errors(partner, HeartbeatInterface::HeartBeatError::TransmissionError));

	receive_heartbeat(partner, 100);
	EXPECT_EQ(0, count_errors(partner, HeartbeatInterface::HeartBeatError::InvalidSequenceCounter));

	for (std::uint8_t counter = 101; counter <= 106; counter++)
	{
		receive_heartbeat(partner, counter);
	}
	EXPECT_EQ(0, count_recovered(partner));

	receive_heartbeat(partner, 107);
	EXPECT_EQ(1, count_recovered(partner));
}

TEST_F(HeartbeatReceiverTest, ReservedValuesIgnored)
{
	auto partner = test_helpers::force_claim_partnered_control_function(0x63, 0);

	receive_heartbeat(partner, 10);
	receive_heartbeat(partner, 252);
	receive_heartbeat(partner, 253);
	receive_heartbeat(partner, 11);
	EXPECT_EQ(0, count_errors(partner));

	time_source.update_for_ms(200);
	receive_heartbeat(partner, 252);
	time_source.update_for_ms(150);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1, count_errors(partner));
	EXPECT_EQ(1, count_errors(partner, HeartbeatInterface::HeartBeatError::TimedOut));
}

TEST_F(HeartbeatReceiverTest, GracefulShutdownStopsTracking)
{
	auto partner = test_helpers::force_claim_partnered_control_function(0x64, 0);

	receive_heartbeat(partner, 10);
	EXPECT_EQ(1, count_new_tracked(partner));
	receive_heartbeat(partner, 255);
	EXPECT_EQ(1, count_errors(partner, HeartbeatInterface::HeartBeatError::GracefulShutdown));
	EXPECT_EQ(1, count_errors(partner));

	receive_heartbeat(partner, 255);
	EXPECT_EQ(1, count_errors(partner));

	time_source.update_for_ms(400);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1, count_errors(partner));

	receive_heartbeat(partner, 251);
	EXPECT_EQ(2, count_new_tracked(partner));
	receive_heartbeat(partner, 0);
	EXPECT_EQ(1, count_errors(partner));
}

TEST_F(HeartbeatReceiverTest, StaleHeartbeatStillTimesOut)
{
	auto partner = test_helpers::force_claim_partnered_control_function(0x6A, 0);

	receive_heartbeat(partner, 10);
	EXPECT_EQ(1, count_new_tracked(partner));

	// simulate_delay_ms does not pump the stack, so the receive path has to detect the timeout, not the update sweep
	time_source.simulate_delay_ms(JUST_PAST_TIMEOUT_MS);
	receive_heartbeat(partner, 11);

	EXPECT_EQ(1, count_errors(partner, HeartbeatInterface::HeartBeatError::TimedOut));
	EXPECT_EQ(2, count_new_tracked(partner));
	EXPECT_EQ(0, count_errors(partner, HeartbeatInterface::HeartBeatError::InvalidSequenceCounter));
}

TEST_F(HeartbeatReceiverTest, TimeoutNeedsMoreThan300ms)
{
	auto partner = test_helpers::force_claim_partnered_control_function(0x70, 0);

	receive_heartbeat(partner, 10);
	time_source.simulate_delay_ms(300);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(0, count_errors(partner, HeartbeatInterface::HeartBeatError::TimedOut));

	time_source.simulate_delay_ms(1);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1, count_errors(partner, HeartbeatInterface::HeartBeatError::TimedOut));
}

class HeartbeatProducerTest : public HeartbeatTest
{
protected:
	void SetUp() override
	{
		HeartbeatTest::SetUp();
		internalECU = test_helpers::claim_internal_control_function(0x65, 0, time_source);
		partner = test_helpers::force_claim_partnered_control_function(0x66, 0);
		testPlugin.clear_queue();
	}

	/// @brief The partner asks for a heartbeat every 100 ms, so internalECU starts sending
	void start_sending()
	{
		receive_rate_request(0x66, 0x65, 100);
		testPlugin.clear_queue();
	}

	std::vector<std::uint8_t> sent_counters()
	{
		return first_data_bytes(0x0CF0E465);
	}

	std::shared_ptr<InternalControlFunction> internalECU;
	std::shared_ptr<PartneredControlFunction> partner;
};

TEST_F(HeartbeatProducerTest, RateRequestFromUnknownSourceIsIgnored)
{
	receive_rate_request(0xFE, 0x65, 100);

	EXPECT_TRUE(testPlugin.get_queue_empty());
}

TEST_F(HeartbeatProducerTest, NoChangeRateRequestIsIgnored)
{
	receive_rate_request(0x66, 0x65, 0xFFFF);

	EXPECT_TRUE(testPlugin.get_queue_empty());
}

TEST_F(HeartbeatProducerTest, RateRequestIsAnsweredWithTheActualRate)
{
	receive_rate_request(0x66, 0x65, 0);

	CANMessageFrame frame = {};
	ASSERT_TRUE(testPlugin.read_frame(frame));
	EXPECT_EQ(frame.identifier, 0x18FE0E65);
	EXPECT_EQ(frame.dataLength, 8);
	EXPECT_EQ(0, std::memcmp(REPETITION_RATE_100_MS, frame.data, sizeof(REPETITION_RATE_100_MS)));
	ASSERT_TRUE(testPlugin.read_frame(frame));
	EXPECT_EQ(frame.identifier, 0x0CF0E465);
	EXPECT_EQ(frame.data[0], 251);
}

TEST_F(HeartbeatProducerTest, TransmissionErrorSends254UntilCleared)
{
	start_sending();

	interface().set_transmission_error(internalECU, true);
	send_next_heartbeat();
	EXPECT_EQ(std::vector<std::uint8_t>{ 254 }, sent_counters());

	interface().set_transmission_error(internalECU, false);
	send_next_heartbeat();
	const auto counters = sent_counters();
	ASSERT_EQ(1, counters.size());
	EXPECT_LE(counters.front(), 250);
}

TEST_F(HeartbeatProducerTest, ShutdownSends255)
{
	start_sending();

	interface().send_shutdown(internalECU);
	send_next_heartbeat();

	EXPECT_EQ(std::vector<std::uint8_t>{ 255 }, sent_counters());
}

TEST_F(HeartbeatProducerTest, ShutdownAppliesToTheFrameThatIsDue)
{
	start_sending();

	make_frame_due();
	interface().send_shutdown(internalECU);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);

	EXPECT_EQ(std::vector<std::uint8_t>{ 255 }, sent_counters());
}

TEST_F(HeartbeatProducerTest, TransmissionErrorAppliesToTheFrameThatIsDue)
{
	start_sending();

	make_frame_due();
	interface().set_transmission_error(internalECU, true);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(5);

	EXPECT_EQ(std::vector<std::uint8_t>{ 254 }, sent_counters());
}

TEST_F(HeartbeatProducerTest, DeactivatedProducerSendsNothing)
{
	start_sending();

	make_frame_due();
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANNetworkManager::CANNetwork.update();
	send_next_heartbeat();
	send_next_heartbeat();

	EXPECT_TRUE(sent_counters().empty());
}
