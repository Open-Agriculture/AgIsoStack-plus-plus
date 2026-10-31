#include <gtest/gtest.h>

#include "isobus/isobus/can_extended_transport_protocol.hpp"

#include "helpers/control_function_helpers.hpp"
#include "helpers/test_fixture.hpp"

#include <atomic>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

using namespace isobus;

class ExtendedTransportProtocolTest : public AgIsoStackTestFixture
{
	// Wrapper to give tests a more meaningful name - no content.
};

// A transmit started on one thread while update() runs on another, as an application thread does
// beside the stack's own: each session sends its RTS once, and the sessions list stays intact. Both
// threads used to run the first state of the new session's state machine (now and then both sent
// the RTS), and the session was added to the list with no lock while update() walked it.
TEST_F(ExtendedTransportProtocolTest, TransmitFromAnotherThreadSendsOneRequestToSend)
{
	constexpr std::uint8_t RECEIVERS = 200;
	std::vector<std::uint8_t> dataToSend(2000, 0x55);

	auto originator = test_helpers::create_mock_internal_control_function(0x01);
	std::vector<std::shared_ptr<ControlFunction>> receivers;
	for (std::uint8_t i = 0; i < RECEIVERS; i++)
	{
		receivers.push_back(test_helpers::create_mock_control_function(static_cast<std::uint8_t>(0x10 + i)));
	}

	std::mutex countsMutex;
	std::map<std::uint8_t, int> requestsToSend; // per receiver address
	auto sendFrameCallback = [&](std::uint32_t parameterGroupNumber,
	                             CANDataSpan data,
	                             std::shared_ptr<InternalControlFunction>,
	                             std::shared_ptr<ControlFunction> destinationControlFunction,
	                             CANIdentifier::CANPriority) {
		if ((parameterGroupNumber == 0xC800) && (data[0] == 20))
		{
			std::lock_guard<std::mutex> lock(countsMutex);
			requestsToSend[destinationControlFunction->get_address()]++;
		}
		return true;
	};

	CANNetworkConfiguration configuration;
	configuration.set_max_number_transport_protocol_sessions(RECEIVERS);
	ExtendedTransportProtocolManager manager(sendFrameCallback, nullptr, &configuration);

	std::atomic<bool> stop{ false };
	std::thread stackThread([&]() {
		while (!stop)
		{
			manager.update();
		}
	});
	for (auto &receiver : receivers)
	{
		auto data = std::unique_ptr<CANMessageData>(new CANMessageDataView(dataToSend.data(), dataToSend.size()));
		ASSERT_TRUE(manager.protocol_transmit_message(0xFEEB, data, originator, receiver, nullptr, nullptr));
	}
	stop = true;
	stackThread.join();

	EXPECT_EQ(manager.get_sessions().size(), RECEIVERS);
	ASSERT_EQ(requestsToSend.size(), RECEIVERS);
	for (const auto &entry : requestsToSend)
	{
		EXPECT_EQ(entry.second, 1) << "receiver 0x" << std::hex << static_cast<int>(entry.first);
	}
}
