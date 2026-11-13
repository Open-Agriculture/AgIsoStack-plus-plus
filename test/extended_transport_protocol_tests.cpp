#include <gtest/gtest.h>

#include "isobus/isobus/can_extended_transport_protocol.hpp"

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"
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

// The DPO covers as many packets as the receiver's CTS allows, whatever that is. Some VTs reject
// an object pool sent with fewer packets per DPO than their CTS allowed, even though the
// transfer itself completes.
TEST_F(ExtendedTransportProtocolTest, DataPacketOffsetFollowsTheClearToSend)
{
	constexpr std::uint32_t MESSAGE_LENGTH = 3000; // 429 packets
	std::vector<std::uint8_t> dataToSend(MESSAGE_LENGTH, 0x55);

	auto originator = test_helpers::create_mock_internal_control_function(0x01);
	auto receiver = test_helpers::create_mock_control_function(0x02);

	std::vector<std::uint8_t> packetsPerDataPacketOffset;
	std::uint32_t dataPackets = 0;
	auto sendFrameCallback = [&](std::uint32_t parameterGroupNumber,
	                             CANDataSpan data,
	                             std::shared_ptr<InternalControlFunction>,
	                             std::shared_ptr<ControlFunction>,
	                             CANIdentifier::CANPriority) {
		if ((parameterGroupNumber == 0xC800) && (data[0] == 22))
		{
			packetsPerDataPacketOffset.push_back(data[1]);
		}
		else if (parameterGroupNumber == 0xC700)
		{
			dataPackets++;
		}
		return true;
	};

	CANNetworkConfiguration configuration;
	ExtendedTransportProtocolManager manager(sendFrameCallback, nullptr, &configuration);

	auto clearToSend = [&](std::uint8_t numberOfPackets, std::uint32_t nextPacket) {
		manager.process_message(test_helpers::create_message(7,
		                                                     0xC800,
		                                                     originator,
		                                                     receiver,
		                                                     { 21,
		                                                       numberOfPackets,
		                                                       static_cast<std::uint8_t>(nextPacket & 0xFF),
		                                                       static_cast<std::uint8_t>((nextPacket >> 8) & 0xFF),
		                                                       static_cast<std::uint8_t>((nextPacket >> 16) & 0xFF),
		                                                       0xEB,
		                                                       0xFE,
		                                                       0x00 }));
		manager.update(); // Sends the DPO
		manager.update(); // Sends the data packets
	};

	auto data = std::unique_ptr<CANMessageData>(new CANMessageDataView(dataToSend.data(), dataToSend.size()));
	ASSERT_TRUE(manager.protocol_transmit_message(0xFEEB, data, originator, receiver, nullptr, nullptr));
	manager.update();

	clearToSend(255, 1);
	clearToSend(30, 256);
	clearToSend(50, 286);
	clearToSend(255, 336);

	EXPECT_EQ(packetsPerDataPacketOffset, (std::vector<std::uint8_t>{ 255, 30, 50, 94 }));
	EXPECT_EQ(dataPackets, 429u);

	// A configured maximum still limits the DPO below the CTS
	configuration.set_number_of_packets_per_dpo_message(16);
	packetsPerDataPacketOffset.clear();
	auto otherReceiver = test_helpers::create_mock_control_function(0x03);
	data = std::unique_ptr<CANMessageData>(new CANMessageDataView(dataToSend.data(), dataToSend.size()));
	ASSERT_TRUE(manager.protocol_transmit_message(0xFEEB, data, originator, otherReceiver, nullptr, nullptr));
	manager.update();
	manager.process_message(test_helpers::create_message(7, 0xC800, originator, otherReceiver, { 21, 255, 1, 0, 0, 0xEB, 0xFE, 0x00 }));
	manager.update();
	EXPECT_EQ(packetsPerDataPacketOffset, (std::vector<std::uint8_t>{ 16 }));
}

// The CTS the stack sends when it receives ETP has its own limit, so letting the DPO follow the
// receiver's CTS on transmit doesn't make the stack ask for 255 packets at a time.
TEST_F(ExtendedTransportProtocolTest, ClearToSendUsesTheReceiveLimit)
{
	auto originator = test_helpers::create_mock_control_function(0x04);
	auto receiver = test_helpers::create_mock_internal_control_function(0x05);

	std::vector<std::uint8_t> packetsPerClearToSend;
	auto sendFrameCallback = [&](std::uint32_t parameterGroupNumber,
	                             CANDataSpan data,
	                             std::shared_ptr<InternalControlFunction>,
	                             std::shared_ptr<ControlFunction>,
	                             CANIdentifier::CANPriority) {
		if ((parameterGroupNumber == 0xC800) && (data[0] == 21))
		{
			packetsPerClearToSend.push_back(data[1]);
		}
		return true;
	};

	CANNetworkConfiguration configuration;
	ExtendedTransportProtocolManager manager(sendFrameCallback, nullptr, &configuration);

	// 3000 bytes: 0x00000BB8
	manager.process_message(test_helpers::create_message(7, 0xC800, receiver, originator, { 20, 0xB8, 0x0B, 0x00, 0x00, 0xEB, 0xFE, 0x00 }));
	manager.update();
	ASSERT_EQ(packetsPerClearToSend, (std::vector<std::uint8_t>{ 16 }));

	configuration.set_number_of_packets_per_etp_cts_message(50);
	auto otherOriginator = test_helpers::create_mock_control_function(0x06);
	packetsPerClearToSend.clear();
	manager.process_message(test_helpers::create_message(7, 0xC800, receiver, otherOriginator, { 20, 0xB8, 0x0B, 0x00, 0x00, 0xEB, 0xFE, 0x00 }));
	manager.update();
	EXPECT_EQ(packetsPerClearToSend, (std::vector<std::uint8_t>{ 50 }));
}
