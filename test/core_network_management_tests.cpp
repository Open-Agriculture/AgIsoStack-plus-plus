#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_general_parameter_group_numbers.hpp"
#include "isobus/isobus/can_internal_control_function.hpp"
#include "isobus/isobus/can_message_frame.hpp"
#include "isobus/isobus/can_network_manager.hpp"
#include "isobus/isobus/can_parameter_group_number_request_protocol.hpp"
#include "isobus/isobus/can_partnered_control_function.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"
#include "helpers/test_fixture.hpp"

using namespace isobus;

static bool wasAnyControlFunctionCallbackHit = false;

static void test_any_control_function_callback(const CANMessage &, void *)
{
	wasAnyControlFunctionCallbackHit = true;
}

static std::uint32_t sniffedMessageCount = 0;
static std::shared_ptr<ControlFunction> sniffedMessageSource = nullptr;
static std::shared_ptr<ControlFunction> sniffedMessageDestination = nullptr;

static void test_sniffed_message_callback(const CANMessage &message, void *)
{
	sniffedMessageCount++;
	sniffedMessageSource = message.get_source_control_function();
	sniffedMessageDestination = message.get_destination_control_function();
}

static std::uint32_t sniffedPayloadLength = 0;
static std::vector<std::uint8_t> sniffedPayload;

static void test_sniffed_payload_callback(const CANMessage &message, void *)
{
	sniffedMessageCount++;
	sniffedPayloadLength = message.get_data_length();
	sniffedPayload = message.get_data();
	sniffedMessageSource = message.get_source_control_function();
	sniffedMessageDestination = message.get_destination_control_function();
}

static void test_reentrant_sniffed_message_callback(const CANMessage &message, void *)
{
	sniffedMessageCount++;
	// Calling back into the registry from inside a callback must not deadlock
	CANNetworkManager::CANNetwork.is_sniffed_parameter_group_number_of_interest(message.get_identifier().get_parameter_group_number());
	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(message.get_identifier().get_parameter_group_number(), test_reentrant_sniffed_message_callback, nullptr);
}

class CoreTest : public AgIsoStackTestFixture
{
protected:
	void SetUp() override
	{
		AgIsoStackTestFixture::SetUp();
		runningTest = this;
	}

	static void test_control_function_state_callback(std::shared_ptr<ControlFunction> controlFunction, ControlFunctionState state)
	{
		runningTest->testControlFunction = controlFunction;
		runningTest->testControlFunctionState = state;
		runningTest->wasTestStateCallbackHit = true;
	}

	static void count_online_callbacks(std::shared_ptr<ControlFunction>, ControlFunctionState state)
	{
		if (ControlFunctionState::Online == state)
		{
			runningTest->onlineCallbackCount++;
		}
	}

	static void record_partner_state_change(std::shared_ptr<ControlFunction> controlFunction, ControlFunctionState state)
	{
		if (ControlFunction::Type::Partnered == controlFunction->get_type())
		{
			runningTest->partnerStateChanges.emplace_back(controlFunction, state);
		}
	}

	static void later_callback(const CANMessage &, void *)
	{
		runningTest->laterCallbackHits++;
	}

	static void reregistering_any_control_function_callback(const CANMessage &, void *)
	{
		CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(0xFF00, later_callback, nullptr);
		CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(0xFF00, reregistering_any_control_function_callback, nullptr);
	}

	static void removing_state_callback(std::shared_ptr<ControlFunction>, ControlFunctionState)
	{
		runningTest->removingStateCallbackHits++;
		CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(removing_state_callback);
	}

	static bool later_pgn_request_callback(std::uint32_t, std::shared_ptr<ControlFunction>, std::shared_ptr<ControlFunction>, bool &, AcknowledgementType &, void *)
	{
		runningTest->laterCallbackHits++;
		return false;
	}

	static bool reregistering_pgn_request_callback(std::uint32_t, std::shared_ptr<ControlFunction>, std::shared_ptr<ControlFunction>, bool &, AcknowledgementType &, void *parent)
	{
		auto protocol = static_cast<ParameterGroupNumberRequestProtocol *>(parent);
		protocol->register_pgn_request_callback(0xFF02, later_pgn_request_callback, nullptr);
		protocol->remove_pgn_request_callback(0xFF02, reregistering_pgn_request_callback, parent);
		return false;
	}

	static bool deactivating_pgn_request_callback(std::uint32_t, std::shared_ptr<ControlFunction>, std::shared_ptr<ControlFunction>, bool &, AcknowledgementType &, void *)
	{
		// Destroys its ParameterGroupNumberRequestProtocol, which removes that protocol's network manager callbacks
		CANNetworkManager::CANNetwork.deactivate_control_function(runningTest->otherInternalControlFunction);
		return false;
	}

	static bool later_repetition_rate_callback(std::uint32_t, std::shared_ptr<ControlFunction>, std::shared_ptr<ControlFunction>, std::uint32_t, void *)
	{
		runningTest->laterCallbackHits++;
		return false;
	}

	static bool reregistering_repetition_rate_callback(std::uint32_t, std::shared_ptr<ControlFunction>, std::shared_ptr<ControlFunction>, std::uint32_t, void *parent)
	{
		auto protocol = static_cast<ParameterGroupNumberRequestProtocol *>(parent);
		protocol->register_request_for_repetition_rate_callback(0xFF04, later_repetition_rate_callback, nullptr);
		protocol->remove_request_for_repetition_rate_callback(0xFF04, reregistering_repetition_rate_callback, parent);
		return false;
	}

	static void deactivating_partner_state_callback(std::shared_ptr<ControlFunction> controlFunction, ControlFunctionState state)
	{
		if ((ControlFunctionState::Online == state) && (ControlFunction::Type::Partnered == controlFunction->get_type()))
		{
			CANNetworkManager::CANNetwork.deactivate_control_function(std::static_pointer_cast<PartneredControlFunction>(controlFunction));
		}
	}

	static void application_locking_state_callback(std::shared_ptr<ControlFunction>, ControlFunctionState)
	{
		runningTest->applicationLockingCallbackEntered = true;
		const std::lock_guard<std::mutex> applicationLock(runningTest->applicationMutex);
	}

	static void blocking_callback(const CANMessage &, void *)
	{
		runningTest->blockingCallbackHits++;
		runningTest->blockingCallbackStarted = true;
		while (!runningTest->blockingCallbackReleased)
		{
			std::this_thread::yield();
		}
	}

	static bool self_deactivating_pgn_request_callback(std::uint32_t, std::shared_ptr<ControlFunction>, std::shared_ptr<ControlFunction>, bool &shouldAck, AcknowledgementType &ackType, void *)
	{
		// Destroys the ParameterGroupNumberRequestProtocol that is running this callback, which then sends the ACK
		CANNetworkManager::CANNetwork.deactivate_control_function(runningTest->selfDeactivatingInternalControlFunction);
		shouldAck = true;
		ackType = AcknowledgementType::Positive;
		return true;
	}

	static bool counting_pgn_request_callback(std::uint32_t, std::shared_ptr<ControlFunction>, std::shared_ptr<ControlFunction>, bool &, AcknowledgementType &, void *)
	{
		runningTest->standaloneRequestHits++;
		return false;
	}

	static void deactivating_other_partner_state_callback(std::shared_ptr<ControlFunction> controlFunction, ControlFunctionState state)
	{
		if ((ControlFunctionState::Online == state) && (runningTest->deactivatingPartner == controlFunction))
		{
			CANNetworkManager::CANNetwork.deactivate_control_function(runningTest->partnerToDeactivate);
		}
		else if ((ControlFunctionState::Online == state) && (runningTest->partnerToDeactivate == controlFunction))
		{
			runningTest->deactivatedPartnerOnlineCalls++;
		}
	}

	void send_commanded_address(std::shared_ptr<ControlFunction> sender, std::uint64_t targetNAME, std::uint8_t commandedAddress)
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(
		  7,
		  0xEC00,
		  sender,
		  { 0x20, 9, 0, 2, 0xFF, 0xD8, 0xFE, 0x00 }));
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(
		  7,
		  0xEB00,
		  sender,
		  { 1,
		    static_cast<std::uint8_t>(targetNAME),
		    static_cast<std::uint8_t>(targetNAME >> 8),
		    static_cast<std::uint8_t>(targetNAME >> 16),
		    static_cast<std::uint8_t>(targetNAME >> 24),
		    static_cast<std::uint8_t>(targetNAME >> 32),
		    static_cast<std::uint8_t>(targetNAME >> 40),
		    static_cast<std::uint8_t>(targetNAME >> 48) }));
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(
		  7,
		  0xEB00,
		  sender,
		  { 2, static_cast<std::uint8_t>(targetNAME >> 56), commandedAddress, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }));
	}

	void drain_frames(VirtualCANPlugin &plugin)
	{
		CANMessageFrame frame;
		while (!plugin.get_queue_empty())
		{
			plugin.read_frame(frame);
		}
	}

	bool was_address_claim_sent_from(VirtualCANPlugin &plugin, std::uint8_t sourceAddress)
	{
		bool found = false;
		CANMessageFrame frame;
		while (!plugin.get_queue_empty())
		{
			plugin.read_frame(frame);
			if ((0xEEFF == ((frame.identifier >> 8) & 0x3FFFF)) && (sourceAddress == (frame.identifier & 0xFF)))
			{
				found = true;
			}
		}
		return found;
	}

	void request_address_claim(std::shared_ptr<InternalControlFunction> internalControlFunction)
	{
		const CANMessageFrame requestFrame = test_helpers::create_message_frame_pgn_request(0xEE00, nullptr, nullptr);
		const CANMessage request(CANMessage::Type::Receive,
		                         CANIdentifier(requestFrame.identifier),
		                         requestFrame.data,
		                         requestFrame.dataLength,
		                         nullptr,
		                         nullptr,
		                         0,
		                         0);
		internalControlFunction->process_rx_message_for_address_claiming(request);
	}

	void start_virtual_can()
	{
		CANHardwareInterface::set_number_of_can_channels(1);
		CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
		CANHardwareInterface::start(false);
	}

	// A request timestamp of 0 would mean "no request pending"
	void move_past_time_zero()
	{
		CANNetworkManager::CANNetwork.update();
		time_source.update_for_ms(250);
	}

	void broadcast_address_claim_request(std::uint8_t channel)
	{
		CANMessageFrame request = test_helpers::create_message_frame_pgn_request(0xEE00, nullptr, nullptr);
		request.channel = channel;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
	}

	void miss_address_claim_request(std::uint8_t channel)
	{
		broadcast_address_claim_request(channel);
		CANNetworkManager::CANNetwork.update();
		time_source.update_for_ms(1500);
		CANNetworkManager::CANNetwork.update();
	}

	void answer_address_claim_request(std::uint8_t channel, std::uint8_t address, const NAME &name)
	{
		broadcast_address_claim_request(channel);
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(channel, address, name));
		CANNetworkManager::CANNetwork.update();
		time_source.update_for_ms(1500);
		CANNetworkManager::CANNetwork.update();
	}

	// An inactive CF sits at the null address, the same SA a Cannot Claim comes from
	std::shared_ptr<ControlFunction> create_inactive_control_function(std::uint8_t channel)
	{
		constexpr std::uint8_t ADDRESS = 0x9D;
		move_past_time_zero();

		CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(channel, ADDRESS, test_helpers::find_available_name(channel)));
		auto controlFunction = CANNetworkManager::CANNetwork.get_control_function(channel, ADDRESS);

		// Two unanswered requests for address claim make it inactive
		miss_address_claim_request(channel);
		miss_address_claim_request(channel);
		return controlFunction;
	}

	std::uint32_t count_online_callbacks_after_cannot_claim(std::uint8_t channel)
	{
		CANNetworkManager::CANNetwork.add_control_function_status_change_callback(count_online_callbacks);
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(channel, 0xFE, test_helpers::find_available_name(channel)));
		CANNetworkManager::CANNetwork.update();
		CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(count_online_callbacks);
		return onlineCallbackCount;
	}

	// A callback that deadlocks never returns, so the work runs on a thread the test can abandon.
	static bool returns_within_a_second(std::function<void()> work)
	{
		auto finished = std::make_shared<std::promise<void>>();
		std::future<void> result = finished->get_future();
		std::thread([work, finished]() {
			work();
			finished->set_value();
		}).detach();
		const bool returned = (std::future_status::ready == result.wait_for(std::chrono::seconds(1)));
		if (!returned)
		{
			// The abandoned thread can hold the stack's lock and still reads this fixture, so no later test is safe
			std::abort();
		}
		return returned;
	}

	CANMessageFrame create_address_claim_frame(std::uint8_t channel, std::uint8_t address, const NAME &name)
	{
		CANMessageFrame addressClaim = {};
		addressClaim.channel = channel;
		addressClaim.identifier = 0x18EEFF00 | address;
		addressClaim.isExtendedFrame = true;
		addressClaim.dataLength = 8;

		const std::uint64_t fullName = name.get_full_name();
		for (std::uint8_t byteIndex = 0; byteIndex < addressClaim.dataLength; byteIndex++)
		{
			addressClaim.data[byteIndex] = static_cast<std::uint8_t>(fullName >> (8 * byteIndex));
		}
		return addressClaim;
	}

	// The callback APIs take plain function pointers, so the callbacks reach the running test through this
	static CoreTest *runningTest;

	std::shared_ptr<ControlFunction> testControlFunction = nullptr;
	ControlFunctionState testControlFunctionState = ControlFunctionState::Offline;
	bool wasTestStateCallbackHit = false;
	std::uint32_t onlineCallbackCount = 0;
	std::vector<std::pair<std::shared_ptr<ControlFunction>, ControlFunctionState>> partnerStateChanges;
	std::uint32_t laterCallbackHits = 0;
	std::uint32_t removingStateCallbackHits = 0;
	std::shared_ptr<InternalControlFunction> otherInternalControlFunction = nullptr;
	std::mutex applicationMutex;
	std::atomic_bool applicationLockingCallbackEntered{ false };
	std::atomic_bool blockingCallbackStarted{ false };
	std::atomic_bool blockingCallbackReleased{ false };
	std::atomic<std::uint32_t> blockingCallbackHits{ 0 };
	std::shared_ptr<InternalControlFunction> selfDeactivatingInternalControlFunction = nullptr;
	std::uint32_t standaloneRequestHits = 0;
	std::shared_ptr<PartneredControlFunction> deactivatingPartner = nullptr;
	std::shared_ptr<PartneredControlFunction> partnerToDeactivate = nullptr;
	std::uint32_t deactivatedPartnerOnlineCalls = 0;
};

CoreTest *CoreTest::runningTest = nullptr;

TEST_F(CoreTest, TestCreateAndDestroyPartners)
{
	std::vector<isobus::NAMEFilter> vtNameFilters;
	const isobus::NAMEFilter testFilter(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::VirtualTerminal));

	auto testPartner1 = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);
	auto testPartner2 = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);
	CANNetworkManager::CANNetwork.deactivate_control_function(testPartner2);
	auto TestPartner3 = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, vtNameFilters);

	CANNetworkManager::CANNetwork.deactivate_control_function(testPartner1);
	CANNetworkManager::CANNetwork.deactivate_control_function(TestPartner3);
}

TEST_F(CoreTest, TestCreateAndDestroyICFs)
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
	TestDeviceNAME.set_manufacturer_code(1407);

	auto testICF1 = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);

	TestDeviceNAME.set_ecu_instance(1);
	auto testICF2 = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);
	CANNetworkManager::CANNetwork.deactivate_control_function(testICF2);

	TestDeviceNAME.set_ecu_instance(2);
	auto testICF3 = isobus::CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0);

	CANNetworkManager::CANNetwork.deactivate_control_function(testICF1);
	CANNetworkManager::CANNetwork.deactivate_control_function(testICF3);
}

TEST_F(CoreTest, PGNRequestProtocolDoesNotKeepItsICFAlive)
{
	auto internalECU = std::make_shared<InternalControlFunction>(NAME(0), 0x80, 0);
	std::weak_ptr<InternalControlFunction> weakInternalECU = internalECU;
	ParameterGroupNumberRequestProtocol protocol(internalECU);

	internalECU.reset();
	EXPECT_TRUE(weakInternalECU.expired());
}

TEST_F(CoreTest, PGNRequestProtocolCanOutliveTheNetworkManager)
{
	// The listener keeps both protocols alive until the network manager's members are destroyed, after its callback list.
	// Only a sanitizer catches that use-after-free; threadsafe style keeps other tests' leaks out of the child's exit code.
	::testing::GTEST_FLAG(death_test_style) = "threadsafe";
	EXPECT_EXIT(
	  {
		  {
			  auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(NAME(0), 0, 0x80);
			  auto deactivatedECU = CANNetworkManager::CANNetwork.create_internal_control_function(NAME(0), 0, 0x81);
			  auto protocol = internalECU->get_pgn_request_protocol().lock();
			  auto deactivatedProtocol = deactivatedECU->get_pgn_request_protocol().lock();
			  internalECU->get_address_claimed_event_dispatcher().add_listener([protocol, deactivatedProtocol](const std::uint8_t &) {});
			  CANNetworkManager::CANNetwork.deactivate_control_function(deactivatedECU);
		  }
		  std::exit(0);
	  },
	  ::testing::ExitedWithCode(0),
	  "");
	::testing::GTEST_FLAG(death_test_style) = "fast";
}

TEST_F(CoreTest, BusloadTest)
{
	EXPECT_EQ(0.0f, CANNetworkManager::CANNetwork.get_estimated_busload(200)); // Invalid channel should return zero load

#ifndef DISABLE_BUSLOAD_MONITORING
	// Send a bunch of messages through the receive process
	CANMessageFrame testFrame;
	testFrame.dataLength = 8;
	testFrame.channel = 0;
	testFrame.isExtendedFrame = true;
	testFrame.identifier = 0x18EFFFFE;
	memset(testFrame.data, 0, sizeof(testFrame.data));

	CANNetworkManager::CANNetwork.update(); // Make sure the network manager is initialized
	for (std::uint_fast8_t i = 0; i < 25; i++)
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame); // Send a bunch of junk messages
	}
	testFrame.isExtendedFrame = false;
	testFrame.identifier = 0x7F;
	for (std::uint_fast8_t i = 0; i < 25; i++)
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame); // Send a bunch of junk messages
	}
	time_source.simulate_delay_ms(101);
	CANNetworkManager::CANNetwork.update();

	// Bus load should be non zero, and less than 100%
	EXPECT_NE(0.0f, CANNetworkManager::CANNetwork.get_estimated_busload(0));
	EXPECT_LT(CANNetworkManager::CANNetwork.get_estimated_busload(0), 100.0f);
#else
	// When busload monitoring is disabled, get_estimated_busload should always return 0
	EXPECT_EQ(0.0f, CANNetworkManager::CANNetwork.get_estimated_busload(0));
#endif
}

TEST_F(CoreTest, CommandedAddress)
{
	start_virtual_can();

	auto internalECU = test_helpers::claim_internal_control_function(0x43, 0, time_source);
	auto externalECU = test_helpers::force_claim_partnered_control_function(0xF8, 0);

	// Let's construct a short BAM session for commanded address
	// We'll ignore the 50ms timing for the unit test

	// Broadcast Announce Message
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  externalECU,
	  {
	    0x20, // BAM Mux
	    9, // Data Length
	    0, // Data Length MSB
	    2, // Packet count
	    0xFF, // Reserved
	    0xD8, // PGN LSB
	    0xFE, // PGN middle byte
	    0x00, // PGN MSB
	  }));

	std::uint64_t rawNAME = internalECU->get_NAME().get_full_name();

	// data packet 1
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  externalECU,
	  {
	    1, // Sequence Number
	    static_cast<std::uint8_t>(rawNAME),
	    static_cast<std::uint8_t>(rawNAME >> 8),
	    static_cast<std::uint8_t>(rawNAME >> 16),
	    static_cast<std::uint8_t>(rawNAME >> 24),
	    static_cast<std::uint8_t>(rawNAME >> 32),
	    static_cast<std::uint8_t>(rawNAME >> 40),
	    static_cast<std::uint8_t>(rawNAME >> 48),
	  }));

	// data packet 2
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  externalECU,
	  {
	    2, // Sequence Number
	    static_cast<std::uint8_t>(rawNAME >> 56),
	    0x04, // Address
	    0xFF,
	    0xFF,
	    0xFF,
	    0xFF,
	    0xFF,
	  }));
	CANNetworkManager::CANNetwork.update();

	time_source.update_for_ms(500);
	EXPECT_EQ(0x04, internalECU->get_address());

	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANNetworkManager::CANNetwork.deactivate_control_function(externalECU);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, InvalidatingControlFunctions)
{
	start_virtual_can();

	// Request the address claim PGN to simulate a control function starting to claim an address
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_pgn_request(
	  0xEE00, // Address Claim PGN
	  nullptr,
	  nullptr));
	CANNetworkManager::CANNetwork.update();

	// Simulate waiting for some contention
	time_source.update_for_ms(15);
	CANNetworkManager::CANNetwork.update();

	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(test_control_function_state_callback);
	EXPECT_FALSE(wasTestStateCallbackHit);
	EXPECT_EQ(testControlFunction, nullptr);
	EXPECT_EQ(testControlFunctionState, ControlFunctionState::Offline);

	// Make a control function claim it's address
	auto testPartner = test_helpers::force_claim_partnered_control_function(0x79, 0);
	EXPECT_TRUE(wasTestStateCallbackHit);
	EXPECT_NE(testControlFunction, nullptr);
	EXPECT_EQ(testControlFunctionState, ControlFunctionState::Online);
	wasTestStateCallbackHit = false;

	// Request the address claim PGN again
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_pgn_request(
	  0xEE00, // Address Claim PGN
	  nullptr,
	  nullptr));
	CANNetworkManager::CANNetwork.update();

	// The partner doesn't claim again, but one missed request is not enough to take it offline
	time_source.update_for_ms(2000);
	CANNetworkManager::CANNetwork.update();

	EXPECT_TRUE(testPartner->get_address_valid());
	EXPECT_FALSE(wasTestStateCallbackHit && (testControlFunction == testPartner));
	wasTestStateCallbackHit = false;

	// It misses the next request as well, so now it should be invalid.
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_pgn_request(
	  0xEE00, // Address Claim PGN
	  nullptr,
	  nullptr));
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(2000);
	CANNetworkManager::CANNetwork.update();

	EXPECT_FALSE(testPartner->get_address_valid());
	EXPECT_TRUE(wasTestStateCallbackHit);
	EXPECT_NE(testControlFunction, nullptr);
	EXPECT_EQ(testControlFunctionState, ControlFunctionState::Offline);

	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(test_control_function_state_callback);
	CANNetworkManager::CANNetwork.deactivate_control_function(testPartner);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, NewExternalControlFunctionTriggersStateCallback)
{
	CANNetworkManager::CANNetwork.update();
	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(test_control_function_state_callback);

	constexpr std::uint8_t TEST_CHANNEL = 3;
	constexpr std::uint8_t INITIAL_ADDRESS = 0x91;
	constexpr std::uint64_t DUMMY_NAME = 0x0123456789ABCDEFULL;

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, INITIAL_ADDRESS, NAME(DUMMY_NAME)));

	EXPECT_TRUE(wasTestStateCallbackHit);
	ASSERT_NE(nullptr, testControlFunction);
	EXPECT_EQ(isobus::ControlFunctionState::Online, testControlFunctionState);
	EXPECT_EQ(INITIAL_ADDRESS, testControlFunction->get_address());
	EXPECT_EQ(TEST_CHANNEL, testControlFunction->get_can_port());

	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(test_control_function_state_callback);
}

TEST_F(CoreTest, ControlFunctionAddressChangeTriggersStateCallback)
{
	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(test_control_function_state_callback);

	constexpr std::uint8_t INITIAL_ADDRESS = 0x92;
	constexpr std::uint8_t NEW_ADDRESS = 0x93;
	auto partner = test_helpers::force_claim_partnered_control_function(INITIAL_ADDRESS, 0);

	// Reset after the initial Online event so we can capture the address change notification
	wasTestStateCallbackHit = false;
	testControlFunction.reset();
	testControlFunctionState = ControlFunctionState::Offline;

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(partner->get_can_port(), NEW_ADDRESS, partner->get_NAME()));

	EXPECT_TRUE(wasTestStateCallbackHit);
	EXPECT_EQ(isobus::ControlFunctionState::Online, testControlFunctionState);
	ASSERT_EQ(partner, testControlFunction);
	EXPECT_EQ(NEW_ADDRESS, partner->get_address());

	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(test_control_function_state_callback);
	CANNetworkManager::CANNetwork.deactivate_control_function(partner);
}

TEST_F(CoreTest, PartnerCreatedAfterExternalClaimHasValidAddress)
{
	constexpr std::uint8_t TEST_CHANNEL = 3;
	constexpr std::uint8_t CLAIMED_ADDRESS = 0x96;

	const isobus::NAME claimedName = test_helpers::find_available_name(TEST_CHANNEL);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, CLAIMED_ADDRESS, claimedName));

	std::vector<isobus::NAMEFilter> nameFilters{
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::IdentityNumber, claimedName.get_identity_number()),
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::ManufacturerCode, claimedName.get_manufacturer_code()),
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::FunctionCode, claimedName.get_function_code()),
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::FunctionInstance, claimedName.get_function_instance()),
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::EcuInstance, claimedName.get_ecu_instance()),
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::DeviceClass, claimedName.get_device_class()),
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::DeviceClassInstance, claimedName.get_device_class_instance()),
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::IndustryGroup, claimedName.get_industry_group()),
		isobus::NAMEFilter(isobus::NAME::NAMEParameters::ArbitraryAddressCapable, claimedName.get_arbitrary_address_capable())
	};

	auto partner = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(TEST_CHANNEL, nameFilters);

	ASSERT_NE(nullptr, partner);
	EXPECT_TRUE(partner->get_address_valid());
	EXPECT_EQ(CLAIMED_ADDRESS, partner->get_address());
	EXPECT_EQ(claimedName.get_full_name(), partner->get_NAME().get_full_name());

	isobus::CANNetworkManager::CANNetwork.deactivate_control_function(partner);
}

TEST_F(CoreTest, RestoredControlFunctionSurvivesTheNextPrune)
{
	constexpr std::uint8_t TEST_CHANNEL = 3;
	constexpr std::uint8_t DEPARTED_ADDRESS = 0x98;
	constexpr std::uint8_t RETURNING_ADDRESS = 0x99;
	move_past_time_zero();

	const NAME departedName = test_helpers::find_available_name(TEST_CHANNEL);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, DEPARTED_ADDRESS, departedName));
	const NAME returningName = test_helpers::find_available_name(TEST_CHANNEL);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, RETURNING_ADDRESS, returningName));
	CANNetworkManager::CANNetwork.update();
	auto returningCF = CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, RETURNING_ADDRESS);
	ASSERT_NE(nullptr, returningCF);
	ASSERT_EQ(returningName, returningCF->get_NAME());

	// Neither CF answers two requests for address claim in a row, so both are pruned, the departed one first
	miss_address_claim_request(TEST_CHANNEL);
	miss_address_claim_request(TEST_CHANNEL);
	ASSERT_FALSE(returningCF->get_address_valid());

	// The returning CF answers the next request, which restores it to the table
	broadcast_address_claim_request(TEST_CHANNEL);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, RETURNING_ADDRESS, returningName));
	CANNetworkManager::CANNetwork.update();
	ASSERT_TRUE(returningCF->get_address_valid());

	// It claimed after that request, so it must survive the prune when the request's timer runs out
	time_source.update_for_ms(1500);
	CANNetworkManager::CANNetwork.update();
	EXPECT_TRUE(returningCF->get_address_valid());
	EXPECT_EQ(returningCF, CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, RETURNING_ADDRESS));

	// Restored with a clean record: one missed request after that does not take it offline again
	miss_address_claim_request(TEST_CHANNEL);
	EXPECT_TRUE(returningCF->get_address_valid());
}

TEST_F(CoreTest, ControlFunctionThatSkipsOneRequestStaysOnline)
{
	constexpr std::uint8_t TEST_CHANNEL = 3;
	constexpr std::uint8_t SKIPPING_ADDRESS = 0x9B;
	move_past_time_zero();

	const NAME skippingName = test_helpers::find_available_name(TEST_CHANNEL);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, SKIPPING_ADDRESS, skippingName));
	CANNetworkManager::CANNetwork.update();
	auto skippingCF = CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, SKIPPING_ADDRESS);
	ASSERT_NE(nullptr, skippingCF);

	// Skipping every other request never takes it offline: each claim clears the missed request
	miss_address_claim_request(TEST_CHANNEL);
	EXPECT_TRUE(skippingCF->get_address_valid());
	answer_address_claim_request(TEST_CHANNEL, SKIPPING_ADDRESS, skippingName);
	EXPECT_TRUE(skippingCF->get_address_valid());
	miss_address_claim_request(TEST_CHANNEL);
	EXPECT_TRUE(skippingCF->get_address_valid());
	answer_address_claim_request(TEST_CHANNEL, SKIPPING_ADDRESS, skippingName);
	EXPECT_TRUE(skippingCF->get_address_valid());

	// Two misses in a row do
	miss_address_claim_request(TEST_CHANNEL);
	EXPECT_TRUE(skippingCF->get_address_valid());
	miss_address_claim_request(TEST_CHANNEL);
	EXPECT_FALSE(skippingCF->get_address_valid());
	EXPECT_EQ(nullptr, CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, SKIPPING_ADDRESS));
}

TEST_F(CoreTest, AnswerWithinTheResponseTimeoutCounts)
{
	constexpr std::uint8_t TEST_CHANNEL = 3;
	constexpr std::uint8_t SLOW_ADDRESS = 0x9C;
	move_past_time_zero();

	const NAME slowName = test_helpers::find_available_name(TEST_CHANNEL);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, SLOW_ADDRESS, slowName));
	CANNetworkManager::CANNetwork.update();
	auto slowCF = CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, SLOW_ADDRESS);
	ASSERT_NE(nullptr, slowCF);

	// One missed request
	miss_address_claim_request(TEST_CHANNEL);
	ASSERT_TRUE(slowCF->get_address_valid());

	// The next one is answered after 1 s: later than the 755 ms address claim timing, but within
	// the 1.25 s a CF waits for a response (ISO 11783-3 T3), so it is not a second miss
	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(test_control_function_state_callback);
	broadcast_address_claim_request(TEST_CHANNEL);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(1000);
	CANNetworkManager::CANNetwork.update();
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, SLOW_ADDRESS, slowName));
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(500);
	CANNetworkManager::CANNetwork.update();

	EXPECT_TRUE(slowCF->get_address_valid());
	EXPECT_FALSE(wasTestStateCallbackHit && (testControlFunction == slowCF));
	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(test_control_function_state_callback);
}

TEST_F(CoreTest, PartnerCreatedAfterItsDeviceAnsweredARequestSurvivesThePrune)
{
	constexpr std::uint8_t TEST_CHANNEL = 3;
	constexpr std::uint8_t CLAIMED_ADDRESS = 0x9A;
	move_past_time_zero();

	const NAME claimedName = test_helpers::find_available_name(TEST_CHANNEL);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, CLAIMED_ADDRESS, claimedName));
	CANNetworkManager::CANNetwork.update();

	// The device answers a request for address claim before the application creates a partner for it
	broadcast_address_claim_request(TEST_CHANNEL);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, CLAIMED_ADDRESS, claimedName));
	CANNetworkManager::CANNetwork.update();

	const std::vector<NAMEFilter> nameFilters{
		NAMEFilter(NAME::NAMEParameters::IdentityNumber, claimedName.get_identity_number()),
		NAMEFilter(NAME::NAMEParameters::ManufacturerCode, claimedName.get_manufacturer_code()),
		NAMEFilter(NAME::NAMEParameters::FunctionCode, claimedName.get_function_code()),
		NAMEFilter(NAME::NAMEParameters::EcuInstance, claimedName.get_ecu_instance())
	};
	auto partner = CANNetworkManager::CANNetwork.create_partnered_control_function(TEST_CHANNEL, nameFilters);
	ASSERT_TRUE(partner->get_address_valid());

	// The device claimed after that request, so its partner must survive the prune when the request's timer runs out
	time_source.update_for_ms(1000);
	CANNetworkManager::CANNetwork.update();
	EXPECT_TRUE(partner->get_address_valid());
	EXPECT_EQ(partner, CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, CLAIMED_ADDRESS));

	CANNetworkManager::CANNetwork.deactivate_control_function(partner);
}

TEST_F(CoreTest, PartnerCreatedWhileTwoMatchingDevicesAreOfflineGetsTheOneThatReturns)
{
	constexpr std::uint8_t TEST_CHANNEL = 3;
	constexpr std::uint8_t FIRST_ADDRESS = 0x9E;
	constexpr std::uint8_t SECOND_ADDRESS = 0x9F;
	constexpr std::uint8_t DEVICE_CLASS_INSTANCE = 9; // No other test uses it, so only these two devices match the partner
	move_past_time_zero();

	NAME firstName = test_helpers::find_available_name(TEST_CHANNEL);
	firstName.set_device_class_instance(DEVICE_CLASS_INSTANCE);
	NAME secondName = firstName;
	secondName.set_identity_number(firstName.get_identity_number() + 1);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, FIRST_ADDRESS, firstName));
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, SECOND_ADDRESS, secondName));
	CANNetworkManager::CANNetwork.update();

	// Neither device answers two requests for address claim in a row, so both go offline
	miss_address_claim_request(TEST_CHANNEL);
	miss_address_claim_request(TEST_CHANNEL);
	ASSERT_EQ(nullptr, CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, FIRST_ADDRESS));
	ASSERT_EQ(nullptr, CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, SECOND_ADDRESS));

	const std::vector<NAMEFilter> nameFilters{
		NAMEFilter(NAME::NAMEParameters::ManufacturerCode, firstName.get_manufacturer_code()),
		NAMEFilter(NAME::NAMEParameters::FunctionCode, firstName.get_function_code()),
		NAMEFilter(NAME::NAMEParameters::DeviceClassInstance, DEVICE_CLASS_INSTANCE)
	};
	auto partner = CANNetworkManager::CANNetwork.create_partnered_control_function(TEST_CHANNEL, nameFilters);
	CANNetworkManager::CANNetwork.update();

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, SECOND_ADDRESS, secondName));
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(secondName, partner->get_NAME());
	EXPECT_EQ(partner, CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, SECOND_ADDRESS));

	CANNetworkManager::CANNetwork.deactivate_control_function(partner);
}

TEST_F(CoreTest, OfflinePartnerTakesOverAMatchingDeviceThatIsAlreadyOnline)
{
	constexpr std::uint8_t TEST_CHANNEL = 3;
	constexpr std::uint8_t FIRST_ADDRESS = 0x92;
	constexpr std::uint8_t SECOND_ADDRESS = 0x93;
	constexpr std::uint16_t MANUFACTURER_CODE = 1234;
	move_past_time_zero();

	const std::vector<NAMEFilter> nameFilters{
		NAMEFilter(NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(NAME::Function::VirtualTerminal)),
		NAMEFilter(NAME::NAMEParameters::ManufacturerCode, MANUFACTURER_CODE)
	};
	auto partner = CANNetworkManager::CANNetwork.create_partnered_control_function(TEST_CHANNEL, nameFilters);

	// Two VTs match the partner's filters
	NAME firstName(0);
	firstName.set_function_code(static_cast<std::uint8_t>(NAME::Function::VirtualTerminal));
	firstName.set_manufacturer_code(MANUFACTURER_CODE);
	firstName.set_identity_number(1);
	NAME secondName = firstName;
	secondName.set_identity_number(2);

	// The first becomes the partner, the second an external CF
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, FIRST_ADDRESS, firstName));
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, SECOND_ADDRESS, secondName));
	CANNetworkManager::CANNetwork.update();
	ASSERT_TRUE(partner->get_address_valid());
	ASSERT_EQ(firstName, partner->get_NAME());
	auto secondCF = CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, SECOND_ADDRESS);
	ASSERT_NE(nullptr, secondCF);
	ASSERT_EQ(ControlFunction::Type::External, secondCF->get_type());

	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(record_partner_state_change);

	// The first stops answering requests for address claim, while the second answers them.
	// No claim comes after the first goes offline.
	answer_address_claim_request(TEST_CHANNEL, SECOND_ADDRESS, secondName);
	answer_address_claim_request(TEST_CHANNEL, SECOND_ADDRESS, secondName);

	// The partner takes over the second right away
	EXPECT_TRUE(partner->get_address_valid());
	EXPECT_EQ(SECOND_ADDRESS, partner->get_address());
	EXPECT_EQ(secondName, partner->get_NAME());
	EXPECT_EQ(partner, CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, SECOND_ADDRESS));
	EXPECT_FALSE(secondCF->get_address_valid());
	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(record_partner_state_change);
	ASSERT_EQ(2u, partnerStateChanges.size());
	EXPECT_EQ(partner, partnerStateChanges[0].first);
	EXPECT_EQ(ControlFunctionState::Offline, partnerStateChanges[0].second);
	EXPECT_EQ(partner, partnerStateChanges[1].first);
	EXPECT_EQ(ControlFunctionState::Online, partnerStateChanges[1].second);

	// It stays the partner, with the second's claims credited to it
	answer_address_claim_request(TEST_CHANNEL, SECOND_ADDRESS, secondName);
	EXPECT_TRUE(partner->get_address_valid());
	EXPECT_EQ(partner, CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, SECOND_ADDRESS));

	// An online partner isn't taken over
	NAME thirdName = firstName;
	thirdName.set_identity_number(3);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, FIRST_ADDRESS, thirdName));
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(secondName, partner->get_NAME());
	EXPECT_EQ(SECOND_ADDRESS, partner->get_address());

	CANNetworkManager::CANNetwork.deactivate_control_function(partner);
}

TEST_F(CoreTest, OfflinePartnerTakesTheNextMatchingName)
{
	constexpr std::uint8_t TEST_CHANNEL = 3;
	constexpr std::uint8_t FIRST_ADDRESS = 0x94;
	constexpr std::uint8_t SECOND_ADDRESS = 0x95;
	constexpr std::uint16_t MANUFACTURER_CODE = 1235;
	move_past_time_zero();

	const std::vector<NAMEFilter> nameFilters{
		NAMEFilter(NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(NAME::Function::VirtualTerminal)),
		NAMEFilter(NAME::NAMEParameters::ManufacturerCode, MANUFACTURER_CODE)
	};
	auto partner = CANNetworkManager::CANNetwork.create_partnered_control_function(TEST_CHANNEL, nameFilters);

	NAME firstName(0);
	firstName.set_function_code(static_cast<std::uint8_t>(NAME::Function::VirtualTerminal));
	firstName.set_manufacturer_code(MANUFACTURER_CODE);
	firstName.set_identity_number(1);
	NAME secondName = firstName;
	secondName.set_identity_number(2);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, FIRST_ADDRESS, firstName));
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, SECOND_ADDRESS, secondName));
	CANNetworkManager::CANNetwork.update();
	ASSERT_EQ(firstName, partner->get_NAME());
	auto secondCF = CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, SECOND_ADDRESS);
	ASSERT_NE(nullptr, secondCF);

	// Both stop answering, so there is no matching device online for the partner to take over
	miss_address_claim_request(TEST_CHANNEL);
	miss_address_claim_request(TEST_CHANNEL);
	ASSERT_FALSE(partner->get_address_valid());
	ASSERT_FALSE(secondCF->get_address_valid());

	// The second claims again, and its claim makes it the partner
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(TEST_CHANNEL, SECOND_ADDRESS, secondName));
	CANNetworkManager::CANNetwork.update();
	EXPECT_TRUE(partner->get_address_valid());
	EXPECT_EQ(SECOND_ADDRESS, partner->get_address());
	EXPECT_EQ(secondName, partner->get_NAME());
	EXPECT_EQ(partner, CANNetworkManager::CANNetwork.get_control_function(TEST_CHANNEL, SECOND_ADDRESS));
	EXPECT_FALSE(secondCF->get_address_valid());

	CANNetworkManager::CANNetwork.deactivate_control_function(partner);
}

TEST_F(CoreTest, SimilarControlFunctions)
{
	CANNetworkManager::CANNetwork.update();

	// Make a partner that is a fuel system
	// Using a less common function to avoid interfering with other tests when not running under CTest
	const isobus::NAMEFilter filterFuelSystem(isobus::NAME::NAMEParameters::FunctionCode, static_cast<std::uint8_t>(isobus::NAME::Function::FuelSystem));
	const std::vector<isobus::NAMEFilter> nameFilters = { filterFuelSystem };
	auto TestPartner = isobus::CANNetworkManager::CANNetwork.create_partnered_control_function(0, nameFilters);

	// Quick test to make sure partner is working
	EXPECT_EQ(1, TestPartner->get_number_name_filters_with_parameter_type(isobus::NAME::NAMEParameters::FunctionCode));

	// Request the address claim PGN
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_pgn_request(
	  0xEE00, // Address Claim PGN
	  nullptr,
	  nullptr));
	CANNetworkManager::CANNetwork.update();
	CANNetworkManager::CANNetwork.update();

	// Simulate waiting for some contention
	std::this_thread::sleep_for(std::chrono::milliseconds(15));
	CANNetworkManager::CANNetwork.update();

	std::uint64_t rawNAME = 0xa0000F000425e9f8;

	// Force claim some kind of TC
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(0, 0x7A, NAME(rawNAME)));
	CANNetworkManager::CANNetwork.update();

	// Partner should be valid with that same NAME
	EXPECT_EQ(TestPartner->get_NAME().get_full_name(), rawNAME);

	// Now, claim something else that matches a TC. The original partner should remain the same.
	auto secondTCNAME = NAME(rawNAME);
	secondTCNAME.set_ecu_instance(1);
	secondTCNAME.set_function_instance(1);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(0, 0x7B, secondTCNAME));
	CANNetworkManager::CANNetwork.update();

	// Partner should never change
	EXPECT_EQ(TestPartner->get_NAME().get_full_name(), 0xa0000F000425e9f8);
	CANNetworkManager::CANNetwork.deactivate_control_function(TestPartner);
}

TEST_F(CoreTest, SniffedMessageCallbacksSeeExternalToExternalTraffic)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);

	// The network manager discards received frames until update() has initialized it
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x7A, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x7B, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	sniffedMessageSource = nullptr;
	sniffedMessageDestination = nullptr;
	wasAnyControlFunctionCallbackHit = false;

	EXPECT_FALSE(CANNetworkManager::CANNetwork.is_sniffed_parameter_group_number_of_interest(0xEF00));
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xEF00, test_sniffed_message_callback, nullptr);
	EXPECT_TRUE(CANNetworkManager::CANNetwork.is_sniffed_parameter_group_number_of_interest(0xEF00));
	EXPECT_FALSE(CANNetworkManager::CANNetwork.is_sniffed_parameter_group_number_of_interest(0xFE00));

	// The legacy any-control-function callbacks must keep ignoring traffic that isn't addressed to us
	CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(0xEF00, test_any_control_function_callback, nullptr);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  6,
	  0xEF00, // Proprietary A, addressed from one external control function to another
	  listener,
	  talker,
	  { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 }));
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(1, sniffedMessageCount);
	EXPECT_EQ(talker, sniffedMessageSource);
	EXPECT_EQ(listener, sniffedMessageDestination);
	EXPECT_FALSE(wasAnyControlFunctionCallbackHit);

	// A PGN nobody registered for must not reach the sniffing callback
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  6,
	  0x9F00, // A destination specific PGN that no sniffing callback asked for
	  listener,
	  talker,
	  { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 }));
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1, sniffedMessageCount);

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xEF00, test_sniffed_message_callback, nullptr);
	EXPECT_FALSE(CANNetworkManager::CANNetwork.is_sniffed_parameter_group_number_of_interest(0xEF00));

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  6,
	  0xEF00,
	  listener,
	  talker,
	  { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 }));
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1, sniffedMessageCount);

	CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(0xEF00, test_any_control_function_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedMessageCallbackCanMutateTheRegistryFromInsideTheCallback)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x7C, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x7D, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xEF00, test_reentrant_sniffed_message_callback, nullptr);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  6,
	  0xEF00,
	  listener,
	  talker,
	  { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 }));
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(1, sniffedMessageCount);
	// The callback removed itself, so a second identical message must not reach it
	EXPECT_FALSE(CANNetworkManager::CANNetwork.is_sniffed_parameter_group_number_of_interest(0xEF00));

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  6,
	  0xEF00,
	  listener,
	  talker,
	  { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 }));
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1, sniffedMessageCount);

	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedMessageCallbacksSeeAssembledBroadcastMessages)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x7E, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	sniffedPayloadLength = 0;
	sniffedMessageSource = nullptr;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	// Broadcast announce message for a 17 byte payload sent as 3 data frames
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  talker,
	  {
	    32, // BAM Mux
	    17, // Data Length
	    0, // Data Length MSB
	    3, // Packet count
	    0xFF, // Reserved
	    0xEC, // PGN LSB
	    0xFE, // PGN middle byte
	    0x00, // PGN MSB
	  }));
	CANNetworkManager::CANNetwork.update();

	for (std::uint8_t sequenceNumber = 1; sequenceNumber <= 3; sequenceNumber++)
	{
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_broadcast(
		  7,
		  0xEB00, // Transport Protocol Data Transfer
		  talker,
		  { sequenceNumber, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07 }));
		CANNetworkManager::CANNetwork.update();
	}

	EXPECT_EQ(1, sniffedMessageCount);
	EXPECT_EQ(17, sniffedPayloadLength);
	EXPECT_EQ(talker, sniffedMessageSource);

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANHardwareInterface::stop();
}

// Sends the connection management frame that opens a 17 byte, 3 packet connection mode transfer
static void send_sniffed_request_to_send(std::shared_ptr<ControlFunction> talker, std::shared_ptr<ControlFunction> listener, std::uint32_t parameterGroupNumber)
{
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  listener,
	  talker,
	  {
	    16, // Request To Send Mux
	    17, // Data Length
	    0, // Data Length MSB
	    3, // Packet count
	    0xFF, // Packets per CTS
	    static_cast<std::uint8_t>(parameterGroupNumber & 0xFF),
	    static_cast<std::uint8_t>((parameterGroupNumber >> 8) & 0xFF),
	    static_cast<std::uint8_t>((parameterGroupNumber >> 16) & 0xFF),
	  }));
	CANNetworkManager::CANNetwork.update();
}

// Sends a non request to send connection management frame; the observer reads only the multiplexor and PGN
static void send_sniffed_connection_management(std::shared_ptr<ControlFunction> sender,
                                               std::shared_ptr<ControlFunction> receiver,
                                               std::uint8_t multiplexor,
                                               std::uint32_t parameterGroupNumber)
{
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  receiver,
	  sender,
	  {
	    multiplexor,
	    3,
	    0xFF,
	    0xFF,
	    0xFF,
	    static_cast<std::uint8_t>(parameterGroupNumber & 0xFF),
	    static_cast<std::uint8_t>((parameterGroupNumber >> 8) & 0xFF),
	    static_cast<std::uint8_t>((parameterGroupNumber >> 16) & 0xFF),
	  }));
	CANNetworkManager::CANNetwork.update();
}

// The byte at the given position of the given packet, or 0xFF padding past the 17 byte payload
static std::uint8_t sniffed_payload_byte(std::uint8_t sequenceNumber, std::uint8_t indexInPacket)
{
	const std::uint16_t payloadIndex = ((sequenceNumber - 1) * 7) + indexInPacket;
	return (payloadIndex < 17) ? static_cast<std::uint8_t>(payloadIndex + 1) : 0xFF;
}

// Sends one data transfer frame of the transfer opened by send_sniffed_request_to_send
static void send_sniffed_data_transfer(std::shared_ptr<ControlFunction> talker, std::shared_ptr<ControlFunction> listener, std::uint8_t sequenceNumber)
{
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  listener,
	  talker,
	  { sequenceNumber,
	    sniffed_payload_byte(sequenceNumber, 0),
	    sniffed_payload_byte(sequenceNumber, 1),
	    sniffed_payload_byte(sequenceNumber, 2),
	    sniffed_payload_byte(sequenceNumber, 3),
	    sniffed_payload_byte(sequenceNumber, 4),
	    sniffed_payload_byte(sequenceNumber, 5),
	    sniffed_payload_byte(sequenceNumber, 6) }));
	CANNetworkManager::CANNetwork.update();
}

TEST_F(CoreTest, SniffedMessageCallbacksSeeAssembledConnectionModeMessages)
{
	// Sees everything the stack transmits
	VirtualCANPlugin busObserver;
	busObserver.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x80, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x81, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	sniffedPayloadLength = 0;
	sniffedPayload.clear();
	sniffedMessageSource = nullptr;
	sniffedMessageDestination = nullptr;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	CANMessageFrame transmittedFrame = {};
	while (!busObserver.get_queue_empty())
	{
		busObserver.read_frame(transmittedFrame);
	}

	send_sniffed_request_to_send(talker, listener, 0xFEEC);

	// Answering would corrupt the transfer we are only watching
	EXPECT_TRUE(busObserver.get_queue_empty());

	// Out of order and repeated packets happen on a retransmission
	send_sniffed_data_transfer(talker, listener, 2);
	send_sniffed_data_transfer(talker, listener, 2);
	EXPECT_EQ(0, sniffedMessageCount);
	send_sniffed_data_transfer(talker, listener, 1);
	send_sniffed_data_transfer(talker, listener, 3);

	EXPECT_EQ(1, sniffedMessageCount);
	EXPECT_EQ(17, sniffedPayloadLength);
	EXPECT_EQ(talker, sniffedMessageSource);
	EXPECT_EQ(listener, sniffedMessageDestination);
	EXPECT_EQ(17, sniffedPayload.size());
	for (std::uint8_t i = 0; (i < 17) && (i < sniffedPayload.size()); i++)
	{
		EXPECT_EQ(i + 1, sniffedPayload[i]);
	}

	// The receiver's end of message acknowledgement must not deliver a second time
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  7,
	  0xEC00,
	  talker,
	  listener,
	  { 19, 17, 0, 3, 0xFF, 0xEC, 0xFE, 0x00 }));
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1, sniffedMessageCount);
	EXPECT_TRUE(busObserver.get_queue_empty());

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedConnectionModeSessionSurvivesRemovingTheCallbackMidSession)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x84, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x85, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	send_sniffed_request_to_send(talker, listener, 0xFEEC);
	send_sniffed_data_transfer(talker, listener, 1);

	// Interest is decided at the request to send, so unregistering mid transfer only stops the callbacks
	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	send_sniffed_data_transfer(talker, listener, 2);
	send_sniffed_data_transfer(talker, listener, 3);
	EXPECT_EQ(0, sniffedMessageCount);

	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedConnectionModeSessionIsDroppedWhenTheConnectionAborts)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x82, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x83, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	send_sniffed_request_to_send(talker, listener, 0xFEEC);
	send_sniffed_data_transfer(talker, listener, 1);

	// The receiver gives up
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
	  7,
	  0xEC00,
	  talker,
	  listener,
	  {
	    255, // Connection Abort Mux
	    3, // Timeout
	    0xFF,
	    0xFF,
	    0xFF,
	    0xEC, // PGN LSB
	    0xFE, // PGN middle byte
	    0x00, // PGN MSB
	  }));
	CANNetworkManager::CANNetwork.update();

	// A sender ignoring the abort must not complete the message
	send_sniffed_data_transfer(talker, listener, 2);
	send_sniffed_data_transfer(talker, listener, 3);
	EXPECT_EQ(0, sniffedMessageCount);

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

// Both directions carry the same PGN, so the role of the sender says which of them the abort closes
TEST_F(CoreTest, SniffedConnectionModeAbortClosesOnlyTheSessionOfTheSendersRole)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0xA0, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0xA1, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	for (std::uint8_t round = 0; round < 2; round++)
	{
		send_sniffed_request_to_send(talker, listener, 0xFEEC);
		send_sniffed_request_to_send(listener, talker, 0xFEEC);
		send_sniffed_data_transfer(talker, listener, 1);
		send_sniffed_data_transfer(listener, talker, 1);

		// Round 0 is the receiver of talker to listener giving up, round 1 is its originator
		const bool receiverAborts = (0 == round);
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(
		  7,
		  0xEC00,
		  receiverAborts ? talker : listener,
		  receiverAborts ? listener : talker,
		  { 255, 3, static_cast<std::uint8_t>(receiverAborts ? 0xFD : 0xFC), 0xFF, 0xFF, 0xEC, 0xFE, 0x00 }));
		CANNetworkManager::CANNetwork.update();

		send_sniffed_data_transfer(talker, listener, 2);
		send_sniffed_data_transfer(talker, listener, 3);
		EXPECT_EQ(round, sniffedMessageCount);

		send_sniffed_data_transfer(listener, talker, 2);
		send_sniffed_data_transfer(listener, talker, 3);
		EXPECT_EQ(round + 1, sniffedMessageCount);
		EXPECT_EQ(listener, sniffedMessageSource);
		EXPECT_EQ(talker, sniffedMessageDestination);
	}

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedConnectionModeSessionIsReplacedByAnUninterestedRequestToSend)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x86, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x87, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	send_sniffed_request_to_send(talker, listener, 0xFEEC);
	send_sniffed_data_transfer(talker, listener, 1);

	// The same pair starts an unwanted transfer, whose packets must not land in the half finished session
	send_sniffed_request_to_send(talker, listener, 0xEF00);

	send_sniffed_data_transfer(talker, listener, 2);
	send_sniffed_data_transfer(talker, listener, 3);
	EXPECT_EQ(0, sniffedMessageCount);

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedConnectionModeEndOfMessageAcknowledgeLeavesTheReverseSessionAlone)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x88, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x89, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	// Both control functions send the same PGN to each other at the same time, which the standard allows
	send_sniffed_request_to_send(talker, listener, 0xFEEC);
	send_sniffed_request_to_send(listener, talker, 0xFEEC);

	send_sniffed_data_transfer(talker, listener, 1);
	send_sniffed_data_transfer(talker, listener, 2);
	send_sniffed_data_transfer(talker, listener, 3);
	EXPECT_EQ(1, sniffedMessageCount);

	send_sniffed_data_transfer(listener, talker, 1);

	// Closes only the finished transfer, not the one still running in reverse
	send_sniffed_connection_management(listener, talker, 19, 0xFEEC);

	send_sniffed_data_transfer(listener, talker, 2);
	send_sniffed_data_transfer(listener, talker, 3);
	EXPECT_EQ(2, sniffedMessageCount);
	EXPECT_EQ(listener, sniffedMessageSource);
	EXPECT_EQ(talker, sniffedMessageDestination);

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedConnectionModeClearToSendIsCheckedAgainstTheSessionPgn)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x8A, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x8B, 0);
	auto otherTalker = test_helpers::force_claim_partnered_control_function(0x8C, 0);
	auto otherListener = test_helpers::force_claim_partnered_control_function(0x8D, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	// A clear to send for the followed PGN is just pacing, so the session lives
	send_sniffed_request_to_send(talker, listener, 0xFEEC);
	send_sniffed_data_transfer(talker, listener, 1);
	send_sniffed_connection_management(listener, talker, 17, 0xFEEC);
	send_sniffed_data_transfer(talker, listener, 2);
	send_sniffed_data_transfer(talker, listener, 3);
	EXPECT_EQ(1, sniffedMessageCount);

	// A clear to send for another PGN means we missed a request to send
	send_sniffed_request_to_send(otherTalker, otherListener, 0xFEEC);
	send_sniffed_data_transfer(otherTalker, otherListener, 1);
	send_sniffed_connection_management(otherListener, otherTalker, 17, 0xEF00);
	send_sniffed_data_transfer(otherTalker, otherListener, 2);
	send_sniffed_data_transfer(otherTalker, otherListener, 3);
	EXPECT_EQ(1, sniffedMessageCount);

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANNetworkManager::CANNetwork.deactivate_control_function(otherTalker);
	CANNetworkManager::CANNetwork.deactivate_control_function(otherListener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedConnectionModeSessionIsDroppedAfterTheProtocolTimeout)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x8E, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x8F, 0);
	CANNetworkManager::CANNetwork.update();

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	send_sniffed_request_to_send(talker, listener, 0xFEEC);
	send_sniffed_data_transfer(talker, listener, 1);

	time_source.simulate_delay_ms(1300); // T2/T3 = 1250ms
	CANNetworkManager::CANNetwork.update();

	// Late packets must not complete the timed out message
	send_sniffed_data_transfer(talker, listener, 2);
	send_sniffed_data_transfer(talker, listener, 3);
	EXPECT_EQ(0, sniffedMessageCount);

	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, SniffedConnectionModeCapacityIsNotSpentOnUninterestedConnections)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	CANNetworkManager::CANNetwork.update();

	auto talker = test_helpers::force_claim_partnered_control_function(0x90, 0);
	auto listener = test_helpers::force_claim_partnered_control_function(0x91, 0);
	auto otherTalker = test_helpers::force_claim_partnered_control_function(0x92, 0);
	auto otherListener = test_helpers::force_claim_partnered_control_function(0x93, 0);
	CANNetworkManager::CANNetwork.update();

	const auto previousSessionLimit = CANNetworkManager::CANNetwork.get_configuration().get_max_number_transport_protocol_sessions();
	CANNetworkManager::CANNetwork.get_configuration().set_max_number_transport_protocol_sessions(1);

	sniffedMessageCount = 0;
	CANNetworkManager::CANNetwork.add_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);

	// With room for one observed session, an unwanted transfer must not take it
	send_sniffed_request_to_send(otherTalker, otherListener, 0xEF00);
	send_sniffed_data_transfer(otherTalker, otherListener, 1);

	send_sniffed_request_to_send(talker, listener, 0xFEEC);
	send_sniffed_data_transfer(talker, listener, 1);
	send_sniffed_data_transfer(talker, listener, 2);
	send_sniffed_data_transfer(talker, listener, 3);
	EXPECT_EQ(1, sniffedMessageCount);
	EXPECT_EQ(talker, sniffedMessageSource);

	CANNetworkManager::CANNetwork.get_configuration().set_max_number_transport_protocol_sessions(previousSessionLimit);
	CANNetworkManager::CANNetwork.remove_sniffed_message_callback(0xFEEC, test_sniffed_payload_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(talker);
	CANNetworkManager::CANNetwork.deactivate_control_function(listener);
	CANNetworkManager::CANNetwork.deactivate_control_function(otherTalker);
	CANNetworkManager::CANNetwork.deactivate_control_function(otherListener);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, DeactivatingAPartnerWhileMessagesAreProcessed)
{
	start_virtual_can();
	auto internalECU = test_helpers::claim_internal_control_function(0x8C, 0, time_source);
	auto firstPartner = test_helpers::force_claim_partnered_control_function(0x8D, 0);
	CANHardwareInterface::stop(); // From here the thread below is the only one that updates the stack
	const NAME partnerName = firstPartner->get_NAME();
	const std::vector<NAMEFilter> nameFilters = {
		NAMEFilter(NAME::NAMEParameters::IdentityNumber, partnerName.get_identity_number()),
		NAMEFilter(NAME::NAMEParameters::ManufacturerCode, partnerName.get_manufacturer_code()),
		NAMEFilter(NAME::NAMEParameters::FunctionCode, partnerName.get_function_code())
	};
	// Leaves an external control function with that NAME at 0x8D, for the partners below to match
	CANNetworkManager::CANNetwork.deactivate_control_function(firstPartner);

	// The stack runs on one thread, receiving messages from 0x8D for our control function, while the
	// application creates and deactivates a partner for 0x8D on another: the receive path walks the
	// partner list and reads the address table that deactivate_control_function() changes.
	std::atomic_bool done = { false };
	std::thread stack([&]() {
		const auto source = test_helpers::create_mock_control_function(0x8D);
		while (!done)
		{
			CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame(6, 0xEF00, internalECU, source, { 1, 2, 3, 4, 5, 6, 7, 8 }));
			CANNetworkManager::CANNetwork.update();
		}
	});
	for (int round = 0; round < 2000; round++)
	{
		auto partner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, nameFilters);
		std::this_thread::yield();
		CANNetworkManager::CANNetwork.deactivate_control_function(partner);
	}
	done = true;
	stack.join();

	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANHardwareInterface::set_number_of_can_channels(0);
}

TEST_F(CoreTest, AnyControlFunctionCallbackCanChangeTheCallbacks)
{
	CANNetworkManager::CANNetwork.update(); // Make sure the network manager is initialized
	auto source = test_helpers::force_claim_partnered_control_function(0x90, 0);
	CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(0xFF00, reregistering_any_control_function_callback, nullptr);
	const CANMessageFrame frame = test_helpers::create_message_frame_broadcast(6, 0xFF00, source, { 1, 2, 3, 4, 5, 6, 7, 8 });

	ASSERT_TRUE(returns_within_a_second([frame]() {
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(frame);
		CANNetworkManager::CANNetwork.update();
	}));
	EXPECT_EQ(0u, laterCallbackHits);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(frame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1u, laterCallbackHits);

	CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(0xFF00, later_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(source);
}

TEST_F(CoreTest, ControlFunctionStateCallbackCanRemoveItself)
{
	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(removing_state_callback);
	auto partners = std::make_shared<std::vector<std::shared_ptr<PartneredControlFunction>>>();

	ASSERT_TRUE(returns_within_a_second([partners]() {
		partners->push_back(test_helpers::force_claim_partnered_control_function(0x92, 0));
	}));
	EXPECT_EQ(1u, removingStateCallbackHits);

	partners->push_back(test_helpers::force_claim_partnered_control_function(0x93, 0));
	EXPECT_EQ(1u, removingStateCallbackHits);

	for (const auto &partner : *partners)
	{
		CANNetworkManager::CANNetwork.deactivate_control_function(partner);
	}
}

TEST_F(CoreTest, PgnRequestCallbackCanChangeTheCallbacks)
{
	start_virtual_can();
	auto internalECU = test_helpers::claim_internal_control_function(0x94, 0, time_source);
	auto requester = test_helpers::force_claim_partnered_control_function(0x95, 0);
	CANHardwareInterface::stop();
	auto protocol = internalECU->get_pgn_request_protocol().lock();
	ASSERT_NE(nullptr, protocol);
	protocol->register_pgn_request_callback(0xFF02, reregistering_pgn_request_callback, protocol.get());
	const CANMessageFrame request = test_helpers::create_message_frame_pgn_request(0xFF02, requester, internalECU);

	ASSERT_TRUE(returns_within_a_second([request]() {
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
		CANNetworkManager::CANNetwork.update();
	}));
	EXPECT_EQ(0u, laterCallbackHits);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1u, laterCallbackHits);

	protocol->remove_pgn_request_callback(0xFF02, later_pgn_request_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(requester);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANHardwareInterface::set_number_of_can_channels(0);
}

TEST_F(CoreTest, PgnRequestCallbackCanDeactivateAnotherInternalControlFunction)
{
	start_virtual_can();
	auto internalECU = test_helpers::claim_internal_control_function(0x96, 0, time_source);
	auto requester = test_helpers::force_claim_partnered_control_function(0x97, 0);
	CANHardwareInterface::stop();
	otherInternalControlFunction = CANNetworkManager::CANNetwork.create_internal_control_function(test_helpers::find_available_name(0), 0, 0x98);
	auto protocol = internalECU->get_pgn_request_protocol().lock();
	ASSERT_NE(nullptr, protocol);
	protocol->register_pgn_request_callback(0xFF01, deactivating_pgn_request_callback, nullptr);
	const CANMessageFrame request = test_helpers::create_message_frame_pgn_request(0xFF01, requester, internalECU);

	ASSERT_TRUE(returns_within_a_second([request]() {
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
		CANNetworkManager::CANNetwork.update();
	}));
	EXPECT_EQ(nullptr, otherInternalControlFunction->get_pgn_request_protocol().lock());

	protocol->remove_pgn_request_callback(0xFF01, deactivating_pgn_request_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(requester);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANHardwareInterface::set_number_of_can_channels(0);
}

TEST_F(CoreTest, RepetitionRateCallbackCanChangeTheCallbacks)
{
	start_virtual_can();
	auto internalECU = test_helpers::claim_internal_control_function(0x9C, 0, time_source);
	auto requester = test_helpers::force_claim_partnered_control_function(0x9D, 0);
	CANHardwareInterface::stop();
	auto protocol = internalECU->get_pgn_request_protocol().lock();
	ASSERT_NE(nullptr, protocol);
	protocol->register_request_for_repetition_rate_callback(0xFF04, reregistering_repetition_rate_callback, protocol.get());
	const CANMessageFrame request = test_helpers::create_message_frame(6,
	                                                                   static_cast<std::uint32_t>(CANLibParameterGroupNumber::RequestForRepetitionRate),
	                                                                   internalECU,
	                                                                   requester,
	                                                                   { 0x04, 0xFF, 0x00, 0x64, 0x00, 0xFF, 0xFF, 0xFF });

	ASSERT_TRUE(returns_within_a_second([request]() {
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
		CANNetworkManager::CANNetwork.update();
	}));
	EXPECT_EQ(0u, laterCallbackHits);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1u, laterCallbackHits);

	protocol->remove_request_for_repetition_rate_callback(0xFF04, later_repetition_rate_callback, nullptr);
	CANNetworkManager::CANNetwork.deactivate_control_function(requester);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANHardwareInterface::set_number_of_can_channels(0);
}

TEST_F(CoreTest, PartnerOnlineCallbackCanDeactivateThePartner)
{
	CANNetworkManager::CANNetwork.update();
	const NAME name = test_helpers::find_available_name(0);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(0, 0x99, name));
	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(deactivating_partner_state_callback);
	auto partner = std::make_shared<std::shared_ptr<PartneredControlFunction>>();

	ASSERT_TRUE(returns_within_a_second([name, partner]() {
		*partner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, { NAMEFilter(NAME::NAMEParameters::IdentityNumber, name.get_identity_number()) });
		CANNetworkManager::CANNetwork.update();
	}));
	auto partners = CANNetworkManager::CANNetwork.get_partnered_control_functions();
	EXPECT_EQ(partners.end(), std::find(partners.begin(), partners.end(), *partner));

	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(deactivating_partner_state_callback);
}

TEST_F(CoreTest, StateCallbackCanWaitForAThreadThatUsesTheNetworkManager)
{
	CANNetworkManager::CANNetwork.update();
	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(application_locking_state_callback);
	const CANMessageFrame addressClaim = create_address_claim_frame(0, 0x9A, test_helpers::find_available_name(0));

	ASSERT_TRUE(returns_within_a_second([this, addressClaim]() {
		std::unique_lock<std::mutex> applicationLock(applicationMutex);
		std::thread stackThread([addressClaim]() {
			CANNetworkManager::CANNetwork.process_receive_can_message_frame(addressClaim);
		});
		while (!applicationLockingCallbackEntered)
		{
			std::this_thread::yield();
		}
		CANNetworkManager::CANNetwork.get_partnered_control_functions();
		applicationLock.unlock();
		stackThread.join();
	}));

	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(application_locking_state_callback);
}

TEST_F(CoreTest, RemovingACallbackDoesNotWaitForACallInProgressOnAnotherThread)
{
	CANNetworkManager::CANNetwork.update();
	auto source = test_helpers::force_claim_partnered_control_function(0x9B, 0);
	CANNetworkManager::CANNetwork.add_any_control_function_parameter_group_number_callback(0xFF03, blocking_callback, nullptr);
	const CANMessageFrame frame = test_helpers::create_message_frame_broadcast(6, 0xFF03, source, { 1, 2, 3, 4, 5, 6, 7, 8 });

	std::thread stackThread([frame]() {
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(frame);
		CANNetworkManager::CANNetwork.update();
	});
	const bool started = returns_within_a_second([this]() {
		while (!blockingCallbackStarted)
		{
			std::this_thread::yield();
		}
	});
	const bool removedWhileTheCallbackRan = returns_within_a_second([]() {
		CANNetworkManager::CANNetwork.remove_any_control_function_parameter_group_number_callback(0xFF03, blocking_callback, nullptr);
	});
	blockingCallbackReleased = true;
	stackThread.join();
	ASSERT_TRUE(started);
	EXPECT_TRUE(removedWhileTheCallbackRan);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(frame);
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1u, blockingCallbackHits);

	CANNetworkManager::CANNetwork.deactivate_control_function(source);
}

TEST_F(CoreTest, PgnRequestCallbackCanDeactivateItsOwnInternalControlFunction)
{
	start_virtual_can();
	selfDeactivatingInternalControlFunction = test_helpers::claim_internal_control_function(0x96, 0, time_source);
	auto requester = test_helpers::force_claim_partnered_control_function(0x97, 0);
	CANHardwareInterface::stop();
	auto protocol = selfDeactivatingInternalControlFunction->get_pgn_request_protocol().lock();
	ASSERT_NE(nullptr, protocol);
	protocol->register_pgn_request_callback(0xFF05, self_deactivating_pgn_request_callback, nullptr);
	std::weak_ptr<ParameterGroupNumberRequestProtocol> protocolLifetime = protocol;
	protocol = nullptr;
	const CANMessageFrame request = test_helpers::create_message_frame_pgn_request(0xFF05, requester, selfDeactivatingInternalControlFunction);

	ASSERT_TRUE(returns_within_a_second([request]() {
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(request);
		CANNetworkManager::CANNetwork.update();
	}));
	EXPECT_TRUE(protocolLifetime.expired());

	CANNetworkManager::CANNetwork.deactivate_control_function(requester);
	CANHardwareInterface::set_number_of_can_channels(0);
}

TEST_F(CoreTest, PgnRequestProtocolOfAStandaloneInternalControlFunctionHandlesRequests)
{
	CANNetworkManager::CANNetwork.update();
	auto requester = test_helpers::force_claim_partnered_control_function(0x94, 0);
	auto internalControlFunction = std::make_shared<InternalControlFunction>(test_helpers::find_available_name(0), 0x95, 0);
	ParameterGroupNumberRequestProtocol protocol(internalControlFunction);
	protocol.register_pgn_request_callback(0xFF06, counting_pgn_request_callback, nullptr);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_pgn_request(0xFF06, requester, nullptr));
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(1u, standaloneRequestHits);

	CANNetworkManager::CANNetwork.deactivate_control_function(requester);
}

TEST_F(CoreTest, PartnerDeactivatedByAnEarlierOnlineCallbackIsNotReportedOnline)
{
	CANNetworkManager::CANNetwork.update();
	const NAME firstName = test_helpers::find_available_name(0);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(0, 0x92, firstName));
	const NAME secondName = test_helpers::find_available_name(0);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(0, 0x93, secondName));
	deactivatingPartner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, { NAMEFilter(NAME::NAMEParameters::IdentityNumber, firstName.get_identity_number()) });
	partnerToDeactivate = CANNetworkManager::CANNetwork.create_partnered_control_function(0, { NAMEFilter(NAME::NAMEParameters::IdentityNumber, secondName.get_identity_number()) });
	CANNetworkManager::CANNetwork.add_control_function_status_change_callback(deactivating_other_partner_state_callback);

	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(0u, deactivatedPartnerOnlineCalls);

	CANNetworkManager::CANNetwork.remove_control_function_status_change_callback(deactivating_other_partner_state_callback);
	CANNetworkManager::CANNetwork.deactivate_control_function(deactivatingPartner);
}

TEST_F(CoreTest, CannotClaimDoesNotRestoreAnInactiveControlFunction)
{
	auto inactiveCF = create_inactive_control_function(0);
	ASSERT_NE(nullptr, inactiveCF);
	ASSERT_FALSE(inactiveCF->get_address_valid());

	EXPECT_EQ(0u, count_online_callbacks_after_cannot_claim(0));
	EXPECT_FALSE(inactiveCF->get_address_valid());
}

TEST_F(CoreTest, CannotClaimOnTheLastChannelDoesNotRestoreAnInactiveControlFunction)
{
	constexpr std::uint8_t LAST_CHANNEL = CAN_PORT_MAXIMUM - 1;
	auto inactiveCF = create_inactive_control_function(LAST_CHANNEL);
	ASSERT_NE(nullptr, inactiveCF);
	ASSERT_FALSE(inactiveCF->get_address_valid());

	EXPECT_EQ(0u, count_online_callbacks_after_cannot_claim(LAST_CHANNEL));
	EXPECT_FALSE(inactiveCF->get_address_valid());
}

TEST_F(CoreTest, CannotClaimFromAPartnerNameDoesNotTouchTheTable)
{
	const NAME partnerName = test_helpers::find_available_name(0);
	auto partner = CANNetworkManager::CANNetwork.create_partnered_control_function(0, { NAMEFilter(NAME::NAMEParameters::IdentityNumber, partnerName.get_identity_number()), NAMEFilter(NAME::NAMEParameters::ManufacturerCode, partnerName.get_manufacturer_code()) });
	ASSERT_NE(nullptr, partner);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(0, 0xFE, partnerName));
	CANNetworkManager::CANNetwork.update();

	EXPECT_FALSE(partner->get_address_valid());
	EXPECT_EQ(0u, partner->get_NAME().get_full_name());
	CANNetworkManager::CANNetwork.deactivate_control_function(partner);
}

TEST_F(CoreTest, CommandedAddressOutsideTheValidRangeIsIgnored)
{
	start_virtual_can();
	VirtualCANPlugin plugin;
	plugin.open();

	auto internalECU = test_helpers::claim_internal_control_function(0x43, 0, time_source);
	auto externalECU = test_helpers::force_claim_partnered_control_function(0xF8, 0);
	const std::uint64_t rawNAME = internalECU->get_NAME().get_full_name();

	for (std::uint8_t commandedAddress : { 0xFE, 0xFF })
	{
		drain_frames(plugin);
		send_commanded_address(externalECU, rawNAME, commandedAddress);
		CANNetworkManager::CANNetwork.update();
		time_source.update_for_ms(500);

		EXPECT_EQ(0x43, internalECU->get_address());
		EXPECT_EQ(InternalControlFunction::State::AddressClaimingComplete, internalECU->get_current_state());
		EXPECT_TRUE(was_address_claim_sent_from(plugin, 0x43));
	}

	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANNetworkManager::CANNetwork.deactivate_control_function(externalECU);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, RequestShorterThanThreeBytesIsIgnored)
{
	// A request carries the requested PGN in 3 bytes. A shorter one, such as "cansend can1 18EAFF7F#", used to throw
	// std::out_of_range from the address claim handling and end the application.
	start_virtual_can();
	VirtualCANPlugin plugin;
	plugin.open();

	auto internalECU = test_helpers::claim_internal_control_function(0x43, 0, time_source);
	drain_frames(plugin);

	EXPECT_NO_THROW(CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_raw(0x18EAFF7F, {})));
	EXPECT_NO_THROW(CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_raw(0x18EAFF7F, { 0x00, 0xEE })));
	EXPECT_NO_THROW(CANNetworkManager::CANNetwork.update());
	time_source.update_for_ms(500);
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(InternalControlFunction::State::AddressClaimingComplete, internalECU->get_current_state());
	EXPECT_FALSE(was_address_claim_sent_from(plugin, 0x43));

	// A complete request for the address claim is still answered
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_raw(0x18EAFF7F, { 0x00, 0xEE, 0x00 }));
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(500);
	CANNetworkManager::CANNetwork.update();
	EXPECT_TRUE(was_address_claim_sent_from(plugin, 0x43));

	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, ReclaimingControlFunctionLosesItsAddressToALowerName)
{
	start_virtual_can();

	auto losingECU = test_helpers::claim_internal_control_function(0x45, 0, time_source);
	request_address_claim(losingECU);
	ASSERT_EQ(InternalControlFunction::State::SendReclaimAddressOnRequest, losingECU->get_current_state());
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(0, 0x45, NAME(losingECU->get_NAME().get_full_name() - 1)));
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(InternalControlFunction::State::WaitForRequestContentionPeriod, losingECU->get_current_state());
	EXPECT_FALSE(losingECU->get_address_valid());

	CANNetworkManager::CANNetwork.deactivate_control_function(losingECU);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, ReclaimingControlFunctionKeepsItsAddressAgainstAHigherName)
{
	start_virtual_can();

	auto winningECU = test_helpers::claim_internal_control_function(0x46, 0, time_source);
	request_address_claim(winningECU);
	ASSERT_EQ(InternalControlFunction::State::SendReclaimAddressOnRequest, winningECU->get_current_state());
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(0, 0x46, NAME(winningECU->get_NAME().get_full_name() + 1)));
	CANNetworkManager::CANNetwork.update();
	EXPECT_EQ(InternalControlFunction::State::AddressClaimingComplete, winningECU->get_current_state());
	EXPECT_EQ(0x46, winningECU->get_address());
	EXPECT_EQ(winningECU, CANNetworkManager::CANNetwork.get_control_function(0, 0x46));

	CANNetworkManager::CANNetwork.deactivate_control_function(winningECU);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, FailedArbitraryAddressClaimSendRestartsClaiming)
{
	constexpr std::uint8_t PREFERRED_ADDRESS = 0x50;
	start_virtual_can();

	// A lower NAME holds the preferred address, so the internal control function has to claim an arbitrary one
	const NAME ourName = test_helpers::find_available_name(0);
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(0, PREFERRED_ADDRESS, NAME(ourName.get_full_name() - 1)));
	auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(ourName, 0, PREFERRED_ADDRESS);
	for (std::uint32_t i = 0; (i < 1000) && (InternalControlFunction::State::SendArbitraryAddressClaim != internalECU->get_current_state()); i++)
	{
		time_source.update_for_ms(1);
	}
	ASSERT_EQ(InternalControlFunction::State::SendArbitraryAddressClaim, internalECU->get_current_state());

	CANHardwareInterface::stop();
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(InternalControlFunction::State::None, internalECU->get_current_state());
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
}

TEST_F(CoreTest, CommandedAddressHeldByALowerNameIsRefused)
{
	start_virtual_can();
	VirtualCANPlugin plugin;
	plugin.open();

	auto internalECU = test_helpers::claim_internal_control_function(0x43, 0, time_source);
	auto externalECU = test_helpers::force_claim_partnered_control_function(0xF8, 0);
	const std::uint64_t rawNAME = internalECU->get_NAME().get_full_name();

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(0, 0x50, NAME(rawNAME - 1)));
	CANNetworkManager::CANNetwork.update();

	drain_frames(plugin);
	send_commanded_address(externalECU, rawNAME, 0x50);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(500);

	EXPECT_EQ(0x43, internalECU->get_address());
	EXPECT_EQ(InternalControlFunction::State::AddressClaimingComplete, internalECU->get_current_state());
	EXPECT_TRUE(was_address_claim_sent_from(plugin, 0x43));
	EXPECT_NE(internalECU, CANNetworkManager::CANNetwork.get_control_function(0, 0x50));

	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANNetworkManager::CANNetwork.deactivate_control_function(externalECU);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, CommandedAddressHeldByAHigherNameIsTaken)
{
	start_virtual_can();
	VirtualCANPlugin plugin;
	plugin.open();

	auto internalECU = test_helpers::claim_internal_control_function(0x43, 0, time_source);
	auto externalECU = test_helpers::force_claim_partnered_control_function(0xF8, 0);
	const std::uint64_t rawNAME = internalECU->get_NAME().get_full_name();

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(0, 0x50, NAME(rawNAME + 1)));
	CANNetworkManager::CANNetwork.update();

	drain_frames(plugin);
	send_commanded_address(externalECU, rawNAME, 0x50);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(500);

	EXPECT_EQ(0x50, internalECU->get_address());
	EXPECT_EQ(InternalControlFunction::State::AddressClaimingComplete, internalECU->get_current_state());
	EXPECT_TRUE(was_address_claim_sent_from(plugin, 0x50));
	EXPECT_EQ(internalECU, CANNetworkManager::CANNetwork.get_control_function(0, 0x50));
	EXPECT_EQ(nullptr, CANNetworkManager::CANNetwork.get_control_function(0, 0x43));

	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANNetworkManager::CANNetwork.deactivate_control_function(externalECU);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, AddressClaimedEventReportsEachClaim)
{
	start_virtual_can();

	auto externalECU = test_helpers::force_claim_partnered_control_function(0xF8, 0);
	auto internalECU = CANNetworkManager::CANNetwork.create_internal_control_function(test_helpers::find_available_name(0), 0, 0x43);
	std::vector<std::uint8_t> claimedAddresses;
	internalECU->get_address_claimed_event_dispatcher().add_listener([&claimedAddresses](std::uint8_t address) {
		claimedAddresses.push_back(address);
	});

	for (std::uint32_t i = 0; (i < 100) && !internalECU->get_address_valid(); i++)
	{
		time_source.update_for_ms(100);
	}
	ASSERT_TRUE(internalECU->get_address_valid());
	EXPECT_EQ(std::vector<std::uint8_t>({ 0x43 }), claimedAddresses);

	send_commanded_address(externalECU, internalECU->get_NAME().get_full_name(), 0x04);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(500);

	ASSERT_FALSE(claimedAddresses.empty());
	EXPECT_EQ(0x04, claimedAddresses.back());

	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANNetworkManager::CANNetwork.deactivate_control_function(externalECU);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, AddressClaimedListenerCanDeactivateItsOwnControlFunction)
{
	start_virtual_can();

	auto firstECU = test_helpers::claim_internal_control_function(0x47, 0, time_source);
	std::atomic_bool listenerCalled{ false };
	auto deactivatingECU = CANNetworkManager::CANNetwork.create_internal_control_function(test_helpers::find_available_name(0), 0, 0x48);
	deactivatingECU->get_address_claimed_event_dispatcher().add_listener([&deactivatingECU, &listenerCalled](std::uint8_t) {
		listenerCalled = true;
		CANNetworkManager::CANNetwork.deactivate_control_function(deactivatingECU);
		// Drop the test's reference too, so only the stack keeps the ICF alive while its event runs
		deactivatingECU = nullptr;
	});

	for (std::uint32_t i = 0; (i < 100) && !listenerCalled; i++)
	{
		time_source.update_for_ms(100);
	}
	ASSERT_TRUE(listenerCalled);
	time_source.update_for_ms(500);

	EXPECT_EQ(0x47, firstECU->get_address());
	EXPECT_EQ(firstECU, CANNetworkManager::CANNetwork.get_control_function(0, 0x47));

	CANNetworkManager::CANNetwork.deactivate_control_function(firstECU);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, LocalControlFunctionEvictedByCommandedAddressReclaims)
{
	start_virtual_can();

	auto first = test_helpers::claim_internal_control_function(0x43, 0, time_source);
	auto second = test_helpers::claim_internal_control_function(0x44, 0, time_source);
	auto externalECU = test_helpers::force_claim_partnered_control_function(0xF8, 0);
	ASSERT_LT(first->get_NAME().get_full_name(), second->get_NAME().get_full_name());

	send_commanded_address(externalECU, first->get_NAME().get_full_name(), 0x44);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(1000);

	EXPECT_EQ(0x44, first->get_address());
	EXPECT_EQ(first, CANNetworkManager::CANNetwork.get_control_function(0, 0x44));
	EXPECT_EQ(InternalControlFunction::State::AddressClaimingComplete, second->get_current_state());
	ASSERT_TRUE(second->get_address_valid());
	EXPECT_NE(0x44, second->get_address());
	EXPECT_EQ(second, CANNetworkManager::CANNetwork.get_control_function(0, second->get_address()));

	CANNetworkManager::CANNetwork.deactivate_control_function(first);
	CANNetworkManager::CANNetwork.deactivate_control_function(second);
	CANNetworkManager::CANNetwork.deactivate_control_function(externalECU);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, LocalControlFunctionWithTheLowerNameKeepsASharedPreferredAddress)
{
	start_virtual_can();

	constexpr std::uint8_t SHARED_ADDRESS = 0x60;
	const NAME lowerName = test_helpers::find_available_name(0);
	auto lower = CANNetworkManager::CANNetwork.create_internal_control_function(lowerName, 0, SHARED_ADDRESS);
	const NAME higherName(lowerName.get_full_name() + 1);
	auto higher = CANNetworkManager::CANNetwork.create_internal_control_function(higherName, 0, SHARED_ADDRESS);

	bool higherTookTheAddress = false;
	for (std::uint32_t i = 0; (i < 1000) && !higherTookTheAddress; i++)
	{
		time_source.update_for_ms(1);
		higherTookTheAddress = (SHARED_ADDRESS == higher->get_address()) ||
		  (lower->get_address_valid() && (lower != CANNetworkManager::CANNetwork.get_control_function(0, SHARED_ADDRESS)));
	}

	EXPECT_FALSE(higherTookTheAddress);
	EXPECT_EQ(SHARED_ADDRESS, lower->get_address());
	EXPECT_NE(SHARED_ADDRESS, higher->get_address());

	CANNetworkManager::CANNetwork.deactivate_control_function(lower);
	CANNetworkManager::CANNetwork.deactivate_control_function(higher);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, ExternalControlFunctionEvictedByOurClaimIsKeptWhenItClaimsAgain)
{
	start_virtual_can();

	auto internalECU = test_helpers::claim_internal_control_function(0x43, 0, time_source);
	auto externalECU = test_helpers::force_claim_partnered_control_function(0xF8, 0);
	const std::uint64_t rawNAME = internalECU->get_NAME().get_full_name();
	const NAME evictedName(rawNAME + 2);

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(0, 0x50, evictedName));
	CANNetworkManager::CANNetwork.update();
	auto evicted = CANNetworkManager::CANNetwork.get_control_function(0, 0x50);
	ASSERT_NE(nullptr, evicted);

	send_commanded_address(externalECU, rawNAME, 0x50);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(500);
	ASSERT_EQ(0x50, internalECU->get_address());

	CANNetworkManager::CANNetwork.process_receive_can_message_frame(create_address_claim_frame(0, 0x61, evictedName));
	CANNetworkManager::CANNetwork.update();

	EXPECT_EQ(evicted, CANNetworkManager::CANNetwork.get_control_function(0, 0x61));
	EXPECT_EQ(0x61, evicted->get_address());

	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANNetworkManager::CANNetwork.deactivate_control_function(externalECU);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, AddressClaimedEventIsNotRepeatedForAReclaim)
{
	start_virtual_can();

	auto internalECU = test_helpers::claim_internal_control_function(0x49, 0, time_source);
	std::uint32_t claimCount = 0;
	internalECU->get_address_claimed_event_dispatcher().add_listener([&claimCount](std::uint8_t) {
		claimCount++;
	});

	request_address_claim(internalECU);
	CANNetworkManager::CANNetwork.update();
	time_source.update_for_ms(100);

	ASSERT_EQ(InternalControlFunction::State::AddressClaimingComplete, internalECU->get_current_state());
	EXPECT_EQ(0u, claimCount);

	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANHardwareInterface::stop();
}

TEST_F(CoreTest, AddressClaimedListenerDoesNotRunForAControlFunctionDeactivatedByAnEarlierListener)
{
	start_virtual_can();

	const NAME firstName = test_helpers::find_available_name(0);
	auto first = CANNetworkManager::CANNetwork.create_internal_control_function(firstName, 0, 0x4A);
	auto second = CANNetworkManager::CANNetwork.create_internal_control_function(NAME(firstName.get_full_name() + 1), 0, 0x4B);
	bool firstListenerRan = false;
	bool secondListenerRan = false;
	first->get_address_claimed_event_dispatcher().add_listener([&](std::uint8_t) {
		firstListenerRan = true;
		CANNetworkManager::CANNetwork.deactivate_control_function(second);
	});
	second->get_address_claimed_event_dispatcher().add_listener([&secondListenerRan](std::uint8_t) {
		secondListenerRan = true;
	});

	for (std::uint32_t i = 0; (i < 1000) && !firstListenerRan; i++)
	{
		time_source.update_for_ms(1);
	}
	ASSERT_TRUE(firstListenerRan);

	EXPECT_FALSE(secondListenerRan);

	CANNetworkManager::CANNetwork.deactivate_control_function(first);
	CANHardwareInterface::stop();
}

static std::vector<std::shared_ptr<ControlFunction>> requestDestinations;

static bool record_request_destination(std::uint32_t, std::shared_ptr<ControlFunction>, std::shared_ptr<ControlFunction> destination, bool &, AcknowledgementType &, void *)
{
	requestDestinations.push_back(destination);
	return true;
}

TEST_F(CoreTest, PgnRequestCallbackIsToldWhereTheRequestWasSent)
{
	requestDestinations.clear();
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start(false);
	auto internalECU = test_helpers::claim_internal_control_function(0xEE, 0, time_source);
	auto requester = test_helpers::force_claim_partnered_control_function(0xEF, 0);
	CANHardwareInterface::stop();
	auto protocol = internalECU->get_pgn_request_protocol().lock();
	ASSERT_NE(nullptr, protocol);

	EXPECT_TRUE(protocol->register_pgn_request_callback(0xFF03, record_request_destination, nullptr));
	EXPECT_FALSE(protocol->register_pgn_request_callback(0xFF03, record_request_destination, nullptr)); // Already registered
	EXPECT_EQ(1u, protocol->get_number_registered_pgn_request_callbacks());

	// A destination-specific request, then a global one
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_pgn_request(0xFF03, requester, internalECU));
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(test_helpers::create_message_frame_pgn_request(0xFF03, requester, nullptr));
	CANNetworkManager::CANNetwork.update();

	ASSERT_EQ(2u, requestDestinations.size());
	EXPECT_EQ(internalECU, requestDestinations.at(0));
	EXPECT_EQ(nullptr, requestDestinations.at(1));

	EXPECT_TRUE(protocol->remove_pgn_request_callback(0xFF03, record_request_destination, nullptr));
	EXPECT_FALSE(protocol->remove_pgn_request_callback(0xFF03, record_request_destination, nullptr));
	EXPECT_EQ(0u, protocol->get_number_registered_pgn_request_callbacks());

	CANNetworkManager::CANNetwork.deactivate_control_function(requester);
	CANNetworkManager::CANNetwork.deactivate_control_function(internalECU);
	CANHardwareInterface::set_number_of_can_channels(0);
}
