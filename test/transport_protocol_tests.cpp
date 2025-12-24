#include <gtest/gtest.h>

#include "isobus/isobus/can_transport_protocol.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <deque>
#include <future>
#include <thread>

using namespace isobus;

// Test case for receiving a broadcast message
TEST(TRANSPORT_PROTOCOL_TESTS, BroadcastMessageReceiving)
{
	constexpr std::uint32_t pgnToReceive = 0xFEEC;
	constexpr std::array<std::uint8_t, 17> dataToReceive = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11 };

	auto originator = test_helpers::create_mock_control_function(0x01);

	std::uint8_t messageCount = 0;
	auto receiveMessageCallback = [&](const CANMessage &message) {
		CANIdentifier identifier = message.get_identifier();
		EXPECT_EQ(identifier.get_parameter_group_number(), pgnToReceive);
		EXPECT_EQ(identifier.get_priority(), CANIdentifier::CANPriority::PriorityDefault6);
		EXPECT_EQ(message.get_source_control_function(), originator);
		EXPECT_TRUE(message.is_broadcast());
		EXPECT_EQ(message.get_data_length(), dataToReceive.size());
		for (std::size_t i = 0; i < dataToReceive.size(); i++)
		{
			EXPECT_EQ(message.get_data()[i], dataToReceive[i]);
		}
		messageCount++;
	};

	// Create the transport protocol manager
	CANNetworkConfiguration defaultConfiguration;
	TransportProtocolManager manager(nullptr, receiveMessageCallback, &defaultConfiguration);

	// Receive broadcast announcement message (BAM)
	manager.process_message(test_helpers::create_message_broadcast(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  originator,
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

	EXPECT_TRUE(manager.has_session(originator, nullptr));

	// Receive the first data frame
	manager.process_message(test_helpers::create_message_broadcast(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  originator,
	  {
	    1, // Sequence number
	    dataToReceive[0],
	    dataToReceive[1],
	    dataToReceive[2],
	    dataToReceive[3],
	    dataToReceive[4],
	    dataToReceive[5],
	    dataToReceive[6],
	  }));

	// Receive the second data frame
	manager.process_message(test_helpers::create_message_broadcast(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  originator,
	  {
	    2, // Sequence number
	    dataToReceive[7],
	    dataToReceive[8],
	    dataToReceive[9],
	    dataToReceive[10],
	    dataToReceive[11],
	    dataToReceive[12],
	    dataToReceive[13],
	  }));

	// Receive the third data frame
	manager.process_message(test_helpers::create_message_broadcast(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  originator,
	  {
	    3, // Sequence number
	    dataToReceive[14],
	    dataToReceive[15],
	    dataToReceive[16],
	    0xFF,
	    0xFF,
	    0xFF,
	    0xFF,
	  }));

	// We now expect the message to be received
	ASSERT_EQ(messageCount, 1);

	// After the transmission is finished, the session should be removed as indication that connection is closed
	ASSERT_FALSE(manager.has_session(originator, nullptr));
}

// Test case for timeout when receiving broadcast message
TEST(TRANSPORT_PROTOCOL_TESTS, BroadcastMessageTimeout)
{
	auto originator = test_helpers::create_mock_control_function(0x01);

	std::uint8_t messageCount = 0;
	auto receiveMessageCallback = [&](const CANMessage &) {
		messageCount++;
	};

	// Create the transport protocol manager
	CANNetworkConfiguration defaultConfiguration;
	TransportProtocolManager manager(nullptr, receiveMessageCallback, &defaultConfiguration);

	// Receive broadcast announcement message (BAM)
	std::uint32_t sessionUpdateTime = SystemTiming::get_timestamp_ms();
	manager.process_message(test_helpers::create_message_broadcast(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  originator,
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

	EXPECT_TRUE(manager.has_session(originator, nullptr));

	// We expect the session to exists for T1=750ms before timing out
	std::uint32_t sessionRemovalTime = 0;
	while (SystemTiming::get_time_elapsed_ms(sessionUpdateTime) < 1000)
	{
		manager.update();
		if (!manager.has_session(originator, nullptr))
		{
			sessionRemovalTime = SystemTiming::get_timestamp_ms();
			break;
		}
	}
	EXPECT_EQ(messageCount, 0);
	EXPECT_NEAR(sessionRemovalTime - sessionUpdateTime, 750, 5);

	// After the transmission is finished, the session should be removed as indication that connection is closed
	ASSERT_FALSE(manager.has_session(originator, nullptr));

	// Now when we try again but stop after the first data frame, we expect the session to also exists for T1=750ms before timing out
	manager.process_message(test_helpers::create_message_broadcast(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  originator,
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

	ASSERT_TRUE(manager.has_session(originator, nullptr));

	// Receive the first data frame
	sessionUpdateTime = SystemTiming::get_timestamp_ms();
	manager.process_message(test_helpers::create_message_broadcast(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  originator,
	  {
	    1, // Sequence number
	    0x01,
	    0x02,
	    0x03,
	    0x04,
	    0x05,
	    0x06,
	    0x07,
	  }));

	ASSERT_TRUE(manager.has_session(originator, nullptr));

	// We expect the session to exists for T1=750ms before timing out
	sessionRemovalTime = 0;
	while (SystemTiming::get_time_elapsed_ms(sessionUpdateTime) < 1000)
	{
		manager.update();
		if (!manager.has_session(originator, nullptr))
		{
			sessionRemovalTime = SystemTiming::get_timestamp_ms();
			break;
		}
	}

	EXPECT_EQ(messageCount, 0);
	EXPECT_NEAR(sessionRemovalTime - sessionUpdateTime, 750, 5);

	// After the transmission is finished, the session should be removed as indication that connection is closed
	ASSERT_FALSE(manager.has_session(originator, nullptr));
};

// Test case for multiple concurrent broadcast messages
TEST(TRANSPORT_PROTOCOL_TESTS, BroadcastConcurrentMessaging)
{
	// We setup five sources, two of them sending the same PGN and data, and the other three sending the different PGNs and data combinations
	constexpr std::uint32_t pgn1ToReceive = 0xFEEC;
	constexpr std::uint32_t pgn2ToReceive = 0xFEEB;
	constexpr std::array<std::uint8_t, 17> dataToReceive1 = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11 };
	constexpr std::array<std::uint8_t, 12> dataToReceive2 = { 0xAC, 0xAB, 0xAA, 0xA9, 0xA8, 0xA7, 0xA6, 0xA5, 0xA4, 0xA3, 0xA2, 0xA1 };

	auto originator1 = test_helpers::create_mock_control_function(0x01);
	auto originator2 = test_helpers::create_mock_control_function(0x02);
	auto originator3 = test_helpers::create_mock_control_function(0x03);
	auto originator4 = test_helpers::create_mock_control_function(0x04);
	auto originator5 = test_helpers::create_mock_control_function(0x05);

	std::array<bool, 5> messagesReceived = { false };
	auto receiveMessageCallback = [&](const CANMessage &message) {
		CANIdentifier identifier = message.get_identifier();
		ASSERT_EQ(identifier.get_priority(), CANIdentifier::CANPriority::PriorityDefault6);
		ASSERT_TRUE(message.is_broadcast());

		std::uint32_t pgnToCheck;
		const std::uint8_t *dataToCheck;
		std::size_t dataLengthToCheck;

		if (message.get_source_control_function() == originator1)
		{
			pgnToCheck = pgn1ToReceive;
			dataToCheck = dataToReceive1.data();
			dataLengthToCheck = dataToReceive1.size();
			messagesReceived[0] = true;
		}
		else if (message.get_source_control_function() == originator2)
		{
			pgnToCheck = pgn1ToReceive;
			dataToCheck = dataToReceive1.data();
			dataLengthToCheck = dataToReceive1.size();
			messagesReceived[1] = true;
		}
		else if (message.get_source_control_function() == originator3)
		{
			pgnToCheck = pgn1ToReceive;
			dataToCheck = dataToReceive2.data();
			dataLengthToCheck = dataToReceive2.size();
			messagesReceived[2] = true;
		}
		else if (message.get_source_control_function() == originator4)
		{
			pgnToCheck = pgn2ToReceive;
			dataToCheck = dataToReceive1.data();
			dataLengthToCheck = dataToReceive1.size();
			messagesReceived[3] = true;
		}
		else if (message.get_source_control_function() == originator5)
		{
			pgnToCheck = pgn2ToReceive;
			dataToCheck = dataToReceive2.data();
			dataLengthToCheck = dataToReceive2.size();
			messagesReceived[4] = true;
		}
		else
		{
			// Unexpected source, fail the test
			ASSERT_TRUE(false);
		}

		ASSERT_EQ(identifier.get_parameter_group_number(), pgnToCheck);
		ASSERT_EQ(message.get_data_length(), dataLengthToCheck);
		for (std::size_t i = 0; i < dataLengthToCheck; i++)
		{
			ASSERT_EQ(message.get_data()[i], dataToCheck[i]);
		}
	};

	// Create the receiving transport protocol manager
	CANNetworkConfiguration configuration;
	configuration.set_max_number_transport_protocol_sessions(5); // We need to increase the number of sessions to 5 for this test
	TransportProtocolManager rxManager(nullptr, receiveMessageCallback, &configuration);

	// Create the sending transport protocol manager
	auto sendFrameCallback = [&](std::uint32_t parameterGroupNumber,
	                             CANDataSpan data,
	                             std::shared_ptr<InternalControlFunction> sourceControlFunction,
	                             std::shared_ptr<ControlFunction> destinationControlFunction,
	                             CANIdentifier::CANPriority priority) {
		EXPECT_EQ(destinationControlFunction, nullptr);
		CANMessage message = test_helpers::create_message_broadcast(static_cast<std::uint8_t>(priority),
		                                                            parameterGroupNumber,
		                                                            sourceControlFunction,
		                                                            data.begin(),
		                                                            data.size());
		rxManager.process_message(message);
		return true;
	};
	TransportProtocolManager txManager(sendFrameCallback, nullptr, &configuration);

	// Send the messages
	auto data = std::unique_ptr<CANMessageData>(new CANMessageDataView(dataToReceive1.data(), dataToReceive1.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgn1ToReceive, data, originator1, nullptr, nullptr, nullptr));
	data.reset(new CANMessageDataView(dataToReceive1.data(), dataToReceive1.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgn1ToReceive, data, originator2, nullptr, nullptr, nullptr));
	data.reset(new CANMessageDataView(dataToReceive2.data(), dataToReceive2.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgn1ToReceive, data, originator3, nullptr, nullptr, nullptr));
	data.reset(new CANMessageDataView(dataToReceive1.data(), dataToReceive1.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgn2ToReceive, data, originator4, nullptr, nullptr, nullptr));
	data.reset(new CANMessageDataView(dataToReceive2.data(), dataToReceive2.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgn2ToReceive, data, originator5, nullptr, nullptr, nullptr));

	ASSERT_TRUE(txManager.has_session(originator1, nullptr));
	ASSERT_TRUE(txManager.has_session(originator2, nullptr));
	ASSERT_TRUE(txManager.has_session(originator3, nullptr));
	ASSERT_TRUE(txManager.has_session(originator4, nullptr));
	ASSERT_TRUE(txManager.has_session(originator5, nullptr));

	// Wait for the transmissions to finish (or timeout)
	std::uint32_t time = SystemTiming::get_timestamp_ms();
	while (std::any_of(messagesReceived.begin(), messagesReceived.end(), [](bool received) { return !received; }) &&
	       (SystemTiming::get_time_elapsed_ms(time) < 5 * 200))
	{
		txManager.update();
		rxManager.update();
	}

	ASSERT_FALSE(rxManager.has_session(originator1, nullptr));
	ASSERT_FALSE(rxManager.has_session(originator2, nullptr));
	ASSERT_FALSE(rxManager.has_session(originator3, nullptr));
	ASSERT_FALSE(rxManager.has_session(originator4, nullptr));
	ASSERT_FALSE(rxManager.has_session(originator5, nullptr));
	ASSERT_FALSE(txManager.has_session(originator1, nullptr));
	ASSERT_FALSE(txManager.has_session(originator2, nullptr));
	ASSERT_FALSE(txManager.has_session(originator3, nullptr));
	ASSERT_FALSE(txManager.has_session(originator4, nullptr));
	ASSERT_FALSE(txManager.has_session(originator5, nullptr));
	ASSERT_TRUE(std::all_of(messagesReceived.begin(), messagesReceived.end(), [](bool received) { return received; }));
}

// Test case for sending a destination specific message
TEST(TRANSPORT_PROTOCOL_TESTS, DestinationSpecificMessageSending)
{
	constexpr std::array<std::uint8_t, 23> dataToSent = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17 };

	auto originator = test_helpers::create_mock_internal_control_function(0x01);
	auto receiver = test_helpers::create_mock_control_function(0x02);
	std::deque<CANMessage> responseQueue;

	std::size_t frameCount = 0;
	auto sendFrameCallback = [&](std::uint32_t parameterGroupNumber,
	                             CANDataSpan data,
	                             std::shared_ptr<InternalControlFunction> sourceControlFunction,
	                             std::shared_ptr<ControlFunction> destinationControlFunction,
	                             CANIdentifier::CANPriority priority) {
		EXPECT_EQ(data.size(), 8);
		EXPECT_EQ(sourceControlFunction, originator);
		EXPECT_EQ(destinationControlFunction, receiver);
		EXPECT_EQ(priority, CANIdentifier::CANPriority::PriorityLowest7);

		switch (frameCount)
		{
			case 0:
				// First we expect a Request to Send (RTS) message
				EXPECT_EQ(parameterGroupNumber, 0xEC00);
				EXPECT_EQ(data[0], 16); // RTS control byte
				EXPECT_EQ(data[1], 23);
				EXPECT_EQ(data[2], 0);
				EXPECT_EQ(data[3], 4); // Number of packets
				EXPECT_EQ(data[4], 1); // Limit number of packets in CTS as per configuration
				EXPECT_EQ(data[5], 0xEB); // PGN LSB
				EXPECT_EQ(data[6], 0xFE); // PGN middle byte
				EXPECT_EQ(data[7], 0x00); // PGN MSB

				// We respond with a clear to send (CTS) message
				responseQueue.push_back(test_helpers::create_message(
				  7,
				  0xEC00, // Transport Protocol Connection Management
				  sourceControlFunction,
				  destinationControlFunction,
				  {
				    17, // CTS Mux
				    2, // Number of packets (ignores the limit in the RTS message)
				    1, // Next packet to send
				    0xFF, // Reserved
				    0xFF, // Reserved
				    0xEB, // PGN LSB
				    0xFE, // PGN middle byte
				    0x00, // PGN MSB
				  }));
				break;

			case 1:
				// Then we expect the first data frame
				EXPECT_EQ(parameterGroupNumber, 0xEB00);
				EXPECT_EQ(data[0], 1); // Sequence number
				EXPECT_EQ(data[1], dataToSent[0]);
				EXPECT_EQ(data[2], dataToSent[1]);
				EXPECT_EQ(data[3], dataToSent[2]);
				EXPECT_EQ(data[4], dataToSent[3]);
				EXPECT_EQ(data[5], dataToSent[4]);
				EXPECT_EQ(data[6], dataToSent[5]);
				EXPECT_EQ(data[7], dataToSent[6]);
				break;

			case 2:
				// Then we expect the second data frame
				EXPECT_EQ(parameterGroupNumber, 0xEB00);
				EXPECT_EQ(data[0], 2); // Sequence number
				EXPECT_EQ(data[1], dataToSent[7]);
				EXPECT_EQ(data[2], dataToSent[8]);
				EXPECT_EQ(data[3], dataToSent[9]);
				EXPECT_EQ(data[4], dataToSent[10]);
				EXPECT_EQ(data[5], dataToSent[11]);
				EXPECT_EQ(data[6], dataToSent[12]);
				EXPECT_EQ(data[7], dataToSent[13]);

				// We respond with another clear to send (CTS) message
				responseQueue.push_back(test_helpers::create_message(
				  7,
				  0xEC00, // Transport Protocol Connection Management
				  sourceControlFunction,
				  destinationControlFunction,
				  {
				    17, // CTS Mux
				    2, // Number of packets
				    3, // Next packet to send
				    0xFF, // Reserved
				    0xFF, // Reserved
				    0xEB, // PGN LSB
				    0xFE, // PGN middle byte
				    0x00, // PGN MSB
				  }));
				break;

			case 3:
				// Then we expect the third data frame
				EXPECT_EQ(parameterGroupNumber, 0xEB00);
				EXPECT_EQ(data[0], 3); // Sequence number
				EXPECT_EQ(data[1], dataToSent[14]);
				EXPECT_EQ(data[2], dataToSent[15]);
				EXPECT_EQ(data[3], dataToSent[16]);
				EXPECT_EQ(data[4], dataToSent[17]);
				EXPECT_EQ(data[5], dataToSent[18]);
				EXPECT_EQ(data[6], dataToSent[19]);
				EXPECT_EQ(data[7], dataToSent[20]);
				break;

			case 4:
				// Then we expect the fourth data frame
				EXPECT_EQ(parameterGroupNumber, 0xEB00);
				EXPECT_EQ(data[0], 4); // Sequence number
				EXPECT_EQ(data[1], dataToSent[21]);
				EXPECT_EQ(data[2], dataToSent[22]);
				EXPECT_EQ(data[3], 0xFF);
				EXPECT_EQ(data[4], 0xFF);
				EXPECT_EQ(data[5], 0xFF);
				EXPECT_EQ(data[6], 0xFF);
				EXPECT_EQ(data[7], 0xFF);

				// We respond with a end of message acknowledge (EOMA) message
				responseQueue.push_back(test_helpers::create_message(
				  7,
				  0xEC00, // Transport Protocol Connection Management
				  sourceControlFunction,
				  destinationControlFunction,
				  {
				    19, // EOMA Mux
				    23, // Data Length
				    0, // Data Length MSB
				    4, // Number of packets
				    0xFF, // Reserved
				    0xEB, // PGN LSB
				    0xFE, // PGN middle byte
				    0x00, // PGN MSB
				  }));
				break;

			default:
				EXPECT_TRUE(false);
		}

		frameCount++;
		return true;
	};

	// Create the transport protocol manager
	// We ask the originator to send only one packet per CTS message, but then we simulate it ignoring the request in the CTS message
	// to test the manager compliance with the receiving control function's CTS limit.
	CANNetworkConfiguration configuration;
	configuration.set_number_of_packets_per_cts_message(1);
	TransportProtocolManager manager(sendFrameCallback, nullptr, &configuration);

	// Send the message
	auto data = std::unique_ptr<CANMessageData>(new CANMessageDataView(dataToSent.data(), dataToSent.size()));
	ASSERT_TRUE(manager.protocol_transmit_message(0xFEEB, data, originator, receiver, nullptr, nullptr));
	ASSERT_TRUE(manager.has_session(originator, receiver));
	// We shouldn't be able to transmit another message
	data.reset(new CANMessageDataView(dataToSent.data(), dataToSent.size())); // Data get's moved, so we need to create a new one
	ASSERT_FALSE(manager.protocol_transmit_message(0xFEEB, data, originator, receiver, nullptr, nullptr));
	// Also not a message with a different PGN
	data.reset(new CANMessageDataView(dataToSent.data(), dataToSent.size())); // Data get's moved, so we need to create a new one
	ASSERT_FALSE(manager.protocol_transmit_message(0xFEEC, data, originator, receiver, nullptr, nullptr));

	// Wait for the transmission to finish (or timeout)
	std::uint32_t time = SystemTiming::get_timestamp_ms();
	while ((!responseQueue.empty()) || ((frameCount < 5) && (SystemTiming::get_time_elapsed_ms(time) < 1250 + 200 + 200 + 1250 + 200 + 200 + 1250))) // maximum time for 4 packets with 2 CTS according to ISO 11783-3
	{
		if (!responseQueue.empty())
		{
			manager.process_message(responseQueue.front());
			responseQueue.pop_front();
		}
		manager.update();
	}

	ASSERT_EQ(frameCount, 5);

	// After the transmission is finished, the session should be removed as indication that connection is closed
	ASSERT_FALSE(manager.has_session(originator, receiver));
}
// Test case for sending a broadcast message
TEST(TRANSPORT_PROTOCOL_TESTS, BroadcastMessageSending)
{
	constexpr std::uint32_t pgnToSent = 0xFEEC;
	constexpr std::array<std::uint8_t, 17> dataToSent = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11 };

	auto originator = test_helpers::create_mock_control_function(0x01);

	std::size_t frameCount = 0;
	std::uint32_t frameTime = 0;
	auto sendFrameCallback = [&](std::uint32_t parameterGroupNumber,
	                             CANDataSpan data,
	                             std::shared_ptr<InternalControlFunction> sourceControlFunction,
	                             std::shared_ptr<ControlFunction> destinationControlFunction,
	                             CANIdentifier::CANPriority priority) {
		EXPECT_EQ(data.size(), 8);
		EXPECT_EQ(sourceControlFunction, originator);
		EXPECT_EQ(destinationControlFunction, nullptr);
		EXPECT_EQ(priority, CANIdentifier::CANPriority::PriorityLowest7);

		switch (frameCount)
		{
			case 0:
				// First we expect broadcast announcement message (BAM)
				EXPECT_EQ(parameterGroupNumber, 0xEC00);
				EXPECT_EQ(data[0], 32); // BAM control byte
				EXPECT_EQ(data[1], 17); // Data Length
				EXPECT_EQ(data[2], 0); // Data Length MSB
				EXPECT_EQ(data[3], 3); // Number of packets
				EXPECT_EQ(data[4], 0xFF); // Reserved
				EXPECT_EQ(data[5], 0xEC); // PGN LSB
				EXPECT_EQ(data[6], 0xFE); // PGN middle byte
				EXPECT_EQ(data[7], 0x00); // PGN MSB
				break;

			case 1:
				// Then we expect the first data frame
				EXPECT_EQ(parameterGroupNumber, 0xEB00);
				EXPECT_EQ(data[0], 1); // Sequence number
				EXPECT_EQ(data[1], dataToSent[0]);
				EXPECT_EQ(data[2], dataToSent[1]);
				EXPECT_EQ(data[3], dataToSent[2]);
				EXPECT_EQ(data[4], dataToSent[3]);
				EXPECT_EQ(data[5], dataToSent[4]);
				EXPECT_EQ(data[6], dataToSent[5]);
				EXPECT_EQ(data[7], dataToSent[6]);
				EXPECT_NEAR(SystemTiming::get_time_elapsed_ms(frameTime), 50, 5); // We expect the first frame to be sent after 50ms (default = J1939 requirement)
				break;

			case 2:
				// Then we expect the second data frame
				EXPECT_EQ(parameterGroupNumber, 0xEB00);
				EXPECT_EQ(data[0], 2); // Sequence number
				EXPECT_EQ(data[1], dataToSent[7]);
				EXPECT_EQ(data[2], dataToSent[8]);
				EXPECT_EQ(data[3], dataToSent[9]);
				EXPECT_EQ(data[4], dataToSent[10]);
				EXPECT_EQ(data[5], dataToSent[11]);
				EXPECT_EQ(data[6], dataToSent[12]);
				EXPECT_EQ(data[7], dataToSent[13]);
				EXPECT_NEAR(SystemTiming::get_time_elapsed_ms(frameTime), 50, 5); // We expect the time between frames to be 50ms (default = J1939 requirement)
				break;

			case 3:
				// Then we expect the third data frame
				EXPECT_EQ(parameterGroupNumber, 0xEB00);
				EXPECT_EQ(data[0], 3); // Sequence number
				EXPECT_EQ(data[1], dataToSent[14]);
				EXPECT_EQ(data[2], dataToSent[15]);
				EXPECT_EQ(data[3], dataToSent[16]);
				EXPECT_EQ(data[4], 0xFF);
				EXPECT_EQ(data[5], 0xFF);
				EXPECT_EQ(data[6], 0xFF);
				EXPECT_EQ(data[7], 0xFF);
				EXPECT_NEAR(SystemTiming::get_time_elapsed_ms(frameTime), 50, 5); // We expect the time between frames to be 50ms (default = J1939 requirement)
				break;

			default:
				EXPECT_TRUE(false);
		}

		frameCount++;
		frameTime = SystemTiming::get_timestamp_ms();
		return true;
	};

	// Create the transport protocol manager
	CANNetworkConfiguration defaultConfiguration;
	TransportProtocolManager manager(sendFrameCallback, nullptr, &defaultConfiguration);

	// Send the message
	auto data = std::unique_ptr<CANMessageData>(new CANMessageDataView(dataToSent.data(), dataToSent.size()));
	ASSERT_TRUE(manager.protocol_transmit_message(pgnToSent, data, originator, nullptr, nullptr, nullptr));
	ASSERT_TRUE(manager.has_session(originator, nullptr));
	// We shouldn't be able to broadcast another message
	data.reset(new CANMessageDataView(dataToSent.data(), dataToSent.size())); // Data get's moved, so we need to create a new one
	ASSERT_FALSE(manager.protocol_transmit_message(pgnToSent, data, originator, nullptr, nullptr, nullptr));
	// Also not a message with a different PGN
	data.reset(new CANMessageDataView(dataToSent.data(), dataToSent.size())); // Data get's moved, so we need to create a new one
	ASSERT_FALSE(manager.protocol_transmit_message(pgnToSent + 1, data, originator, nullptr, nullptr, nullptr));

	// Wait for the transmission to finish (or timeout)
	std::uint32_t time = SystemTiming::get_timestamp_ms();
	while ((frameCount < 4) && (SystemTiming::get_time_elapsed_ms(time) < 3 * 200))
	{
		manager.update();
	}
	ASSERT_EQ(frameCount, 4);

	// We expect the transmission to take the minimum time between frames as we update continuously, plus some margin, by default that should be 50ms
	EXPECT_NEAR(SystemTiming::get_time_elapsed_ms(time), 3 * 50, 5);

	// After the transmission is finished, the session should be removed as indication that connection is closed
	ASSERT_FALSE(manager.has_session(originator, nullptr));
}

// Test case for receiving a destination specific message
TEST(TRANSPORT_PROTOCOL_TESTS, DestinationSpecificMessageReceiving)
{
	constexpr std::array<std::uint8_t, 23> dataToReceive = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17 };

	auto originator = test_helpers::create_mock_control_function(0x01);
	auto receiver = test_helpers::create_mock_internal_control_function(0x02);

	std::uint8_t messageCount = 0;
	auto receiveMessageCallback = [&](const CANMessage &message) {
		CANIdentifier identifier = message.get_identifier();
		EXPECT_EQ(identifier.get_parameter_group_number(), 0xFEEB);
		EXPECT_EQ(identifier.get_priority(), CANIdentifier::CANPriority::PriorityDefault6);
		EXPECT_EQ(message.get_source_control_function(), originator); // Since we are the receiver, the originator should be the source
		EXPECT_EQ(message.get_destination_control_function(), receiver); // Since we are the receiver, the receiver should be the destination
		EXPECT_FALSE(message.is_broadcast());
		EXPECT_EQ(message.get_data_length(), dataToReceive.size());
		for (std::size_t i = 0; i < dataToReceive.size(); i++)
		{
			EXPECT_EQ(message.get_data()[i], dataToReceive[i]);
		}
		messageCount++;
	};

	std::uint8_t frameCount = 0;
	auto sendFrameCallback = [&](std::uint32_t parameterGroupNumber,
	                             CANDataSpan data,
	                             std::shared_ptr<InternalControlFunction> sourceControlFunction,
	                             std::shared_ptr<ControlFunction> destinationControlFunction,
	                             CANIdentifier::CANPriority priority) {
		EXPECT_EQ(data.size(), 8);
		EXPECT_EQ(sourceControlFunction, receiver); // Since it's a response, the receiver should be the source
		EXPECT_EQ(destinationControlFunction, originator); // Since it's a response, the originator should be the destination
		EXPECT_EQ(priority, CANIdentifier::CANPriority::PriorityLowest7);

		switch (frameCount)
		{
			case 0:
				// We expect a clear to send (CTS) message as response to the request to send (RTS) message
				EXPECT_EQ(parameterGroupNumber, 0xEC00);
				EXPECT_EQ(data[0], 17); // CTS control byte
				EXPECT_EQ(data[1], 2); // Number of packets
				EXPECT_EQ(data[2], 1); // Next packet to send
				EXPECT_EQ(data[3], 0xFF); // Reserved
				EXPECT_EQ(data[4], 0xFF); // Reserved
				EXPECT_EQ(data[5], 0xEB); // PGN LSB
				EXPECT_EQ(data[6], 0xFE); // PGN middle byte
				EXPECT_EQ(data[7], 0x00); // PGN MSB
				break;

			case 1:
				// We expect another clear to send (CTS) message as response to the second data frame
				EXPECT_EQ(parameterGroupNumber, 0xEC00);
				EXPECT_EQ(data[0], 17); // CTS control byte
				EXPECT_EQ(data[1], 2); // Number of packets
				EXPECT_EQ(data[2], 3); // Next packet to send
				EXPECT_EQ(data[3], 0xFF); // Reserved
				EXPECT_EQ(data[4], 0xFF); // Reserved
				EXPECT_EQ(data[5], 0xEB); // PGN LSB
				EXPECT_EQ(data[6], 0xFE); // PGN middle byte
				EXPECT_EQ(data[7], 0x00); // PGN MSB
				break;

			case 2:
				// We expect a end of message acknowledge (EOMA) message as response to the fourth data frame
				EXPECT_EQ(parameterGroupNumber, 0xEC00);
				EXPECT_EQ(data[0], 19); // EOMA control byte
				EXPECT_EQ(data[1], 23); // Data Length
				EXPECT_EQ(data[2], 0); // Data Length MSB
				EXPECT_EQ(data[3], 4); // Number of packets
				EXPECT_EQ(data[4], 0xFF); // Reserved
				EXPECT_EQ(data[5], 0xEB); // PGN LSB
				EXPECT_EQ(data[6], 0xFE); // PGN middle byte
				EXPECT_EQ(data[7], 0x00); // PGN MSB
				break;

			default:
				EXPECT_TRUE(false);
		}
		frameCount++;
		return true;
	};

	// Create the transport protocol manager
	CANNetworkConfiguration defaultConfiguration;
	TransportProtocolManager manager(sendFrameCallback, receiveMessageCallback, &defaultConfiguration);

	// Make the manager receive request to send (RTS) message
	manager.process_message(test_helpers::create_message(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  receiver, // Since this is a request, the receiver should be the destination
	  originator, // Since this is a request, the originator should be the source
	  {
	    16, // RTS Mux
	    23, // Data Length
	    0, // Data Length MSB
	    4, // Number of packets
	    2, // Limit max number of packets in CTS
	    0xEB, // PGN LSB
	    0xFE, // PGN middle byte
	    0x00, // PGN MSB
	  }));

	ASSERT_TRUE(manager.has_session(originator, receiver));

	// Wait for a CTS message to be sent
	std::uint32_t time = SystemTiming::get_timestamp_ms();
	while ((frameCount < 1) && (SystemTiming::get_time_elapsed_ms(time) < 1250)) // timeout T3=1250ms
	{
		manager.update();
	}
	ASSERT_EQ(frameCount, 1);

	// Make the manager receive the first two data frames
	manager.process_message(test_helpers::create_message(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  receiver, // Since this is data transfer, the receiver should be the destination
	  originator, // Since this is data transfer, the originator should be the source
	  {
	    1, // Sequence number
	    dataToReceive[0],
	    dataToReceive[1],
	    dataToReceive[2],
	    dataToReceive[3],
	    dataToReceive[4],
	    dataToReceive[5],
	    dataToReceive[6],
	  }));
	manager.process_message(test_helpers::create_message(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  receiver, // Since this is data transfer, the receiver should be the destination
	  originator, // Since this is data transfer, the originator should be the source
	  {
	    2, // Sequence number
	    dataToReceive[7],
	    dataToReceive[8],
	    dataToReceive[9],
	    dataToReceive[10],
	    dataToReceive[11],
	    dataToReceive[12],
	    dataToReceive[13],
	  }));

	// Wait for a CTS message to be sent
	time = SystemTiming::get_timestamp_ms();
	while ((frameCount < 2) && (SystemTiming::get_time_elapsed_ms(time) < 1250)) // timeout T3=1250ms
	{
		manager.update();
	}

	ASSERT_EQ(frameCount, 2);

	// Make the manager receive the third and fourth data frame
	manager.process_message(test_helpers::create_message(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  receiver, // Since this is data transfer, the receiver should be the destination
	  originator, // Since this is data transfer, the originator should be the source
	  {
	    3, // Sequence number
	    dataToReceive[14],
	    dataToReceive[15],
	    dataToReceive[16],
	    dataToReceive[17],
	    dataToReceive[18],
	    dataToReceive[19],
	    dataToReceive[20],
	  }));
	manager.process_message(test_helpers::create_message(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  receiver, // Since this is data transfer, the receiver should be the destination
	  originator, // Since this is data transfer, the originator should be the source
	  {
	    4, // Sequence number
	    dataToReceive[21],
	    dataToReceive[22],
	    0xFF,
	    0xFF,
	    0xFF,
	    0xFF,
	    0xFF,
	  }));

	// Wait for a EOMA message to be sent
	time = SystemTiming::get_timestamp_ms();
	while ((frameCount < 3) && (SystemTiming::get_time_elapsed_ms(time) < 1250)) // timeout T3=1250ms
	{
		manager.update();
	}
	ASSERT_EQ(frameCount, 3);

	// We now expect the message to be received
	ASSERT_EQ(messageCount, 1);

	// After the transmission is finished, the session should be removed as indication that connection is closed
	ASSERT_FALSE(manager.has_session(originator, receiver));
}

void check_abort_message(const CANMessage &message, const std::uint8_t abortReason, const std::uint32_t parameterGroupNumber)
{
	ASSERT_EQ(message.get_identifier().get_parameter_group_number(), 0xEC00); // Transport Protocol Connection Management
	ASSERT_EQ(message.get_data_length(), 8);
	ASSERT_EQ(message.get_data()[0], 255); // Abort control byte
	ASSERT_EQ(message.get_data()[1], abortReason);
	ASSERT_EQ(message.get_data()[2], 0xFF); // Reserved
	ASSERT_EQ(message.get_data()[3], 0xFF); // Reserved
	ASSERT_EQ(message.get_data()[4], 0xFF); // Reserved
	ASSERT_EQ(message.get_data()[5], parameterGroupNumber & 0xFF);
	ASSERT_EQ(message.get_data()[6], (parameterGroupNumber >> 8) & 0xFF);
	ASSERT_EQ(message.get_data()[7], (parameterGroupNumber >> 16) & 0xFF);
}

// Test case for timeout when initiating destination specific message
TEST(TRANSPORT_PROTOCOL_TESTS, DestinationSpecificTimeoutInitiation)
{
	constexpr std::array<std::uint8_t, 17> dataToTransfer = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11 };

	auto originator = test_helpers::create_mock_internal_control_function(0x01);
	auto receiver = test_helpers::create_mock_internal_control_function(0x02);
	std::deque<CANMessage> originatorQueue;
	std::deque<CANMessage> receiverQueue;

	std::size_t messageCount = 0;
	auto receiveMessageCallback = [&](const CANMessage &) {
		messageCount++;
	};

	auto sendFrameCallback = [&](std::uint32_t parameterGroupNumber,
	                             CANDataSpan data,
	                             std::shared_ptr<InternalControlFunction> sourceControlFunction,
	                             std::shared_ptr<ControlFunction> destinationControlFunction,
	                             CANIdentifier::CANPriority priority) {
		CANMessage message = test_helpers::create_message(static_cast<std::uint8_t>(priority),
		                                                  parameterGroupNumber,
		                                                  destinationControlFunction,
		                                                  sourceControlFunction,
		                                                  data.begin(),
		                                                  data.size());

		if (sourceControlFunction == originator)
		{
			originatorQueue.push_back(message);
		}
		else if (sourceControlFunction == receiver)
		{
			receiverQueue.push_back(message);
		}
		else
		{
			// Unexpected source, fail the test
			EXPECT_TRUE(false);
		}
		return true;
	};

	// Create the transport protocol managers
	CANNetworkConfiguration defaultConfiguration;
	TransportProtocolManager txManager(sendFrameCallback, nullptr, &defaultConfiguration);
	TransportProtocolManager rxManager(sendFrameCallback, receiveMessageCallback, &defaultConfiguration);

	// TX will experience no response to request to send (RTS) message, and is expected to timeout after T3=1250ms
	// RX will experience no response to clear to send (CTS) message, and is expected to timeout after T2=1250ms
	auto data = std::unique_ptr<CANMessageData>(new CANMessageDataView(dataToTransfer.data(), dataToTransfer.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(0xFEEB, data, originator, receiver, nullptr, nullptr));
	ASSERT_TRUE(txManager.has_session(originator, receiver));

	// Make the originator send the request to send (RTS) message and forward it to the receiver
	txManager.update();
	ASSERT_EQ(originatorQueue.size(), 1);
	std::uint32_t txSessionUpdateTime = SystemTiming::get_timestamp_ms();
	rxManager.process_message(originatorQueue.front()); // Notify receiver of the request to send (RTS) message
	originatorQueue.pop_front();
	ASSERT_TRUE(rxManager.has_session(originator, receiver));

	// The receiver should respond with a clear to send (CTS) message
	while (receiverQueue.empty() || SystemTiming::time_expired_ms(txSessionUpdateTime, 200)) // Receiver should respond within Tr=200ms
	{
		rxManager.update();
	}
	std::uint32_t rxSessionUpdateTime = SystemTiming::get_timestamp_ms();
	ASSERT_EQ(receiverQueue.size(), 1);
	receiverQueue.pop_front(); // Discard the clear to send (CTS) message

	// Wait for both the originator and receiver to timeout
	std::uint32_t txSessionRemovalTime = 0;
	std::uint32_t rxSessionRemovalTime = 0;
	while (SystemTiming::get_time_elapsed_ms(rxSessionUpdateTime) < 1500 && (txSessionRemovalTime == 0 || rxSessionRemovalTime == 0))
	{
		txManager.update();
		if (!txManager.has_session(originator, receiver) && (txSessionRemovalTime == 0))
		{
			txSessionRemovalTime = SystemTiming::get_timestamp_ms();
		}

		rxManager.update();
		if (!rxManager.has_session(originator, receiver) && (rxSessionRemovalTime == 0))
		{
			rxSessionRemovalTime = SystemTiming::get_timestamp_ms();
		}
	}

	// For the originator side, a connection is established only when the first CTS is received, hence we expect no message to be sent
	ASSERT_TRUE(originatorQueue.empty());

	// For the receiver side, a connection is established as soon as the CTS is sent, hence we do expect an abort message to be sent
	ASSERT_EQ(receiverQueue.size(), 1);
	check_abort_message(receiverQueue.front(), 3, 0xFEEB); // Abort reason 3: Connection timeout
	receiverQueue.pop_front();

	// Check for correct timeouts, and session removal
	ASSERT_NEAR(txSessionRemovalTime - txSessionUpdateTime, 1250, 5); // T3=1250ms
	ASSERT_NEAR(rxSessionRemovalTime - rxSessionUpdateTime, 1250, 5); // T2=1250ms
	ASSERT_FALSE(txManager.has_session(originator, receiver));
	ASSERT_FALSE(rxManager.has_session(originator, receiver));
	ASSERT_EQ(messageCount, 0); // No message should be received
}

// Test case for timeout of destination specific message completion
TEST(TRANSPORT_PROTOCOL_TESTS, DestinationSpecificTimeoutCompletion)
{
	constexpr std::array<std::uint8_t, 17> dataToTransfer = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11 };

	auto originator = test_helpers::create_mock_internal_control_function(0x01);
	auto receiver = test_helpers::create_mock_internal_control_function(0x02);
	std::deque<CANMessage> originatorQueue;
	std::deque<CANMessage> receiverQueue;

	std::size_t messageCount = 0;
	auto receiveMessageCallback = [&](const CANMessage &) {
		messageCount++;
	};

	auto sendFrameCallback = [&](std::uint32_t parameterGroupNumber,
	                             CANDataSpan data,
	                             std::shared_ptr<InternalControlFunction> sourceControlFunction,
	                             std::shared_ptr<ControlFunction> destinationControlFunction,
	                             CANIdentifier::CANPriority priority) {
		CANMessage message = test_helpers::create_message(static_cast<std::uint8_t>(priority),
		                                                  parameterGroupNumber,
		                                                  destinationControlFunction,
		                                                  sourceControlFunction,
		                                                  data.begin(),
		                                                  data.size());

		if (sourceControlFunction == originator)
		{
			originatorQueue.push_back(message);
		}
		else if (sourceControlFunction == receiver)
		{
			receiverQueue.push_back(message);
		}
		else
		{
			// Unexpected source, fail the test
			EXPECT_TRUE(false);
		}
		return true;
	};

	// Create the transport protocol managers
	CANNetworkConfiguration defaultConfiguration;
	TransportProtocolManager txManager(sendFrameCallback, nullptr, &defaultConfiguration);
	TransportProtocolManager rxManager(sendFrameCallback, receiveMessageCallback, &defaultConfiguration);

	// RX will experience a missing last data frame, and is expected to timeout after T1=750ms
	// TX will experience a missing end of message acknowledge (EOMA) message, and is expected to timeout after T3=1250ms
	auto data = std::unique_ptr<CANMessageData>(new CANMessageDataView(dataToTransfer.data(), dataToTransfer.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(0xFEEB, data, originator, receiver, nullptr, nullptr));
	ASSERT_TRUE(txManager.has_session(originator, receiver));

	// Make the originator send the request to send (RTS) message and forward it to the receiver
	txManager.update();
	ASSERT_EQ(originatorQueue.size(), 1);
	rxManager.process_message(originatorQueue.front()); // Notify receiver of request to send (RTS) message
	originatorQueue.pop_front();
	ASSERT_TRUE(rxManager.has_session(originator, receiver));

	// Wait for the receiver to respond with a clear to send (CTS) message and forward it to the originator
	std::uint32_t rxSessionUpdateTime = SystemTiming::get_timestamp_ms();
	while (receiverQueue.empty() || SystemTiming::time_expired_ms(rxSessionUpdateTime, 200)) // Receiver should respond within Tr=200ms
	{
		rxManager.update();
	}
	ASSERT_EQ(receiverQueue.size(), 1);
	txManager.process_message(receiverQueue.front()); // Notify originator of clear to send (CTS) message
	receiverQueue.pop_front();

	// Wait for the originator to send all 3 data frames and forward them to the receiver
	std::uint32_t txSessionUpdateTime = SystemTiming::get_timestamp_ms();
	while ((originatorQueue.size() != 3) || SystemTiming::time_expired_ms(txSessionUpdateTime, 600)) // Originator should respond with all 3 data frames within 3*(Tr=200ms)=600ms
	{
		txManager.update();
	}
	ASSERT_EQ(originatorQueue.size(), 3);
	rxManager.process_message(originatorQueue.front()); // Notify receiver of first data frame
	originatorQueue.pop_front();
	std::this_thread::sleep_for(std::chrono::milliseconds(125)); // Arbitrary delay the second data frame
	rxManager.process_message(originatorQueue.front()); // Notify receiver of second data frame
	rxSessionUpdateTime = SystemTiming::get_timestamp_ms();
	originatorQueue.pop_front();
	originatorQueue.pop_front(); // Discard the third data frame

	// Wait for both the originator and receiver to timeout
	std::uint32_t txSessionRemovalTime = 0;
	std::uint32_t rxSessionRemovalTime = 0;
	while (SystemTiming::get_time_elapsed_ms(rxSessionUpdateTime) < 1500 && (txSessionRemovalTime == 0 || rxSessionRemovalTime == 0))
	{
		txManager.update();
		if (!txManager.has_session(originator, receiver) && (txSessionRemovalTime == 0))
		{
			txSessionRemovalTime = SystemTiming::get_timestamp_ms();
		}

		rxManager.update();
		if (!rxManager.has_session(originator, receiver) && (rxSessionRemovalTime == 0))
		{
			rxSessionRemovalTime = SystemTiming::get_timestamp_ms();
		}
	}

	// For both sides, a connection should've been established, hence we expect an abort message to be sent from both the originator and receiver
	ASSERT_EQ(originatorQueue.size(), 1);
	ASSERT_EQ(receiverQueue.size(), 1);
	check_abort_message(receiverQueue.front(), 3, 0xFEEB); // Abort reason 3: Connection timeout
	originatorQueue.pop_front();
	receiverQueue.pop_front();

	// Check for correct timeouts, and session removal
	ASSERT_NEAR(txSessionRemovalTime - txSessionUpdateTime, 1250, 5); // T3=1250ms
	ASSERT_NEAR(rxSessionRemovalTime - rxSessionUpdateTime, 750, 5); // T1=750ms
	ASSERT_FALSE(txManager.has_session(originator, receiver));
	ASSERT_FALSE(rxManager.has_session(originator, receiver));
	ASSERT_EQ(messageCount, 0); // No message should've been received
}

// Test case for concurrent destination specific messages
TEST(TRANSPORT_PROTOCOL_TESTS, DestinationSpecificConcurrentMessaging)
{
	// We setup a total of 10 concurrent connections:
	//
	// To test data transfer from different sources to the same destination:
	// - 2 connections transferring the same pgn and data
	// - 3 connections transferring other combinations of pgn and data
	//
	// To test data transfer from the same source to different destinations:
	// - 2 connections transferring the same pgn and data
	// - 3 connections transferring other combinations of pgn and data

	constexpr std::uint32_t pgn1ToReceive = 0xFEEC;
	constexpr std::uint32_t pgn2ToReceive = 0xFEEB;
	constexpr std::array<std::uint8_t, 17> dataToReceive1 = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11 };
	constexpr std::array<std::uint8_t, 12> dataToReceive2 = { 0xAC, 0xAB, 0xAA, 0xA9, 0xA8, 0xA7, 0xA6, 0xA5, 0xA4, 0xA3, 0xA2, 0xA1 };

	auto originator1 = test_helpers::create_mock_internal_control_function(0x01); // Send pgn1ToReceive, dataToReceive1
	auto originator2 = test_helpers::create_mock_internal_control_function(0x02); // Send pgn1ToReceive, dataToReceive1
	auto originator3 = test_helpers::create_mock_internal_control_function(0x03); // Send pgn1ToReceive, dataToReceive2
	auto originator4 = test_helpers::create_mock_internal_control_function(0x04); // Send pgn2ToReceive, dataToReceive1
	auto originator5 = test_helpers::create_mock_internal_control_function(0x05); // Send pgn2ToReceive, dataToReceive2
	auto convergingReceiver = test_helpers::create_mock_internal_control_function(0x07);

	auto divergingOriginator = test_helpers::create_mock_internal_control_function(0x06);
	auto receiver1 = test_helpers::create_mock_internal_control_function(0x08); // Receive pgn1ToReceive, dataToReceive1
	auto receiver2 = test_helpers::create_mock_internal_control_function(0x09); // Receive pgn1ToReceive, dataToReceive1
	auto receiver3 = test_helpers::create_mock_internal_control_function(0x0A); // Receive pgn1ToReceive, dataToReceive2
	auto receiver4 = test_helpers::create_mock_internal_control_function(0x0B); // Receive pgn2ToReceive, dataToReceive1
	auto receiver5 = test_helpers::create_mock_internal_control_function(0x0C); // Receive pgn2ToReceive, dataToReceive2
	std::deque<CANMessage> originatingQueue;
	std::deque<CANMessage> receivingQueue;

	std::array<bool, 10> completedConnections = { false };

	auto receiveMessageCallback = [&](const CANMessage &message) {
		CANIdentifier identifier = message.get_identifier();
		EXPECT_EQ(identifier.get_priority(), CANIdentifier::CANPriority::PriorityDefault6);
		EXPECT_FALSE(message.is_broadcast());

		std::uint32_t pgnToCheck;
		const std::uint8_t *dataToCheck;
		std::size_t dataLengthToCheck = 0;

		if ((message.get_destination_control_function() == receiver1) || (message.get_source_control_function() == originator1))
		{
			pgnToCheck = pgn1ToReceive;
			dataToCheck = dataToReceive1.data();
			dataLengthToCheck = dataToReceive1.size();
			if (message.get_destination_control_function() == receiver1)
			{
				EXPECT_TRUE(message.get_source_control_function() == divergingOriginator);
				completedConnections[0] = true;
			}
			else
			{
				EXPECT_TRUE(message.get_destination_control_function() == convergingReceiver);
				completedConnections[5] = true;
			}
		}
		else if ((message.get_destination_control_function() == receiver2) || (message.get_source_control_function() == originator2))
		{
			pgnToCheck = pgn1ToReceive;
			dataToCheck = dataToReceive1.data();
			dataLengthToCheck = dataToReceive1.size();
			if (message.get_destination_control_function() == receiver2)
			{
				EXPECT_TRUE(message.get_source_control_function() == divergingOriginator);
				completedConnections[1] = true;
			}
			else
			{
				EXPECT_TRUE(message.get_destination_control_function() == convergingReceiver);
				completedConnections[6] = true;
			}
		}
		else if ((message.get_destination_control_function() == receiver3) || (message.get_source_control_function() == originator3))
		{
			pgnToCheck = pgn1ToReceive;
			dataToCheck = dataToReceive2.data();
			dataLengthToCheck = dataToReceive2.size();
			if (message.get_destination_control_function() == receiver3)
			{
				EXPECT_TRUE(message.get_source_control_function() == divergingOriginator);
				completedConnections[2] = true;
			}
			else
			{
				EXPECT_TRUE(message.get_destination_control_function() == convergingReceiver);
				completedConnections[7] = true;
			}
		}
		else if ((message.get_destination_control_function() == receiver4) || (message.get_source_control_function() == originator4))
		{
			pgnToCheck = pgn2ToReceive;
			dataToCheck = dataToReceive1.data();
			dataLengthToCheck = dataToReceive1.size();
			if (message.get_destination_control_function() == receiver4)
			{
				EXPECT_TRUE(message.get_source_control_function() == divergingOriginator);
				completedConnections[3] = true;
			}
			else
			{
				EXPECT_TRUE(message.get_destination_control_function() == convergingReceiver);
				completedConnections[8] = true;
			}
		}
		else if ((message.get_destination_control_function() == receiver5) || (message.get_source_control_function() == originator5))
		{
			pgnToCheck = pgn2ToReceive;
			dataToCheck = dataToReceive2.data();
			dataLengthToCheck = dataToReceive2.size();
			if (message.get_destination_control_function() == receiver5)
			{
				EXPECT_TRUE(message.get_source_control_function() == divergingOriginator);
				completedConnections[4] = true;
			}
			else
			{
				EXPECT_TRUE(message.get_destination_control_function() == convergingReceiver);
				completedConnections[9] = true;
			}
		}
		else
		{
			// Unexpected source or destination, fail the test
			EXPECT_TRUE(false);
		}

		EXPECT_EQ(identifier.get_parameter_group_number(), pgnToCheck);
		EXPECT_EQ(message.get_data_length(), dataLengthToCheck);
		for (std::size_t i = 0; i < dataLengthToCheck; i++)
		{
			EXPECT_EQ(message.get_data()[i], dataToCheck[i]);
		}
	};

	auto sendFrameCallback = [&](std::uint32_t parameterGroupNumber,
	                             CANDataSpan data,
	                             std::shared_ptr<InternalControlFunction> sourceControlFunction,
	                             std::shared_ptr<ControlFunction> destinationControlFunction,
	                             CANIdentifier::CANPriority priority) {
		CANMessage message = test_helpers::create_message(static_cast<std::uint8_t>(priority),
		                                                  parameterGroupNumber,
		                                                  destinationControlFunction,
		                                                  sourceControlFunction,
		                                                  data.begin(),
		                                                  data.size());

		if ((sourceControlFunction == originator1) ||
		    (sourceControlFunction == originator2) ||
		    (sourceControlFunction == originator3) ||
		    (sourceControlFunction == originator4) ||
		    (sourceControlFunction == originator5) ||
		    (sourceControlFunction == divergingOriginator))
		{
			originatingQueue.push_back(message);
		}
		else if ((sourceControlFunction == receiver1) ||
		         (sourceControlFunction == receiver2) ||
		         (sourceControlFunction == receiver3) ||
		         (sourceControlFunction == receiver4) ||
		         (sourceControlFunction == receiver5) ||
		         (sourceControlFunction == convergingReceiver))
		{
			receivingQueue.push_back(message);
		}
		else
		{
			// Unexpected source, fail the test
			EXPECT_TRUE(false);
		}
		return true;
	};

	// Create the transport protocol managers
	CANNetworkConfiguration configuration;
	configuration.set_max_number_transport_protocol_sessions(10); // We need to increase the number of sessions to 10 for this test
	TransportProtocolManager txManager(sendFrameCallback, nullptr, &configuration);
	TransportProtocolManager rxManager(sendFrameCallback, receiveMessageCallback, &configuration);

	// Send the converging messages
	auto data = std::unique_ptr<CANMessageData>(new CANMessageDataView(dataToReceive1.data(), dataToReceive1.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgn1ToReceive, data, originator1, convergingReceiver, nullptr, nullptr));
	data.reset(new CANMessageDataView(dataToReceive1.data(), dataToReceive1.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgn1ToReceive, data, originator2, convergingReceiver, nullptr, nullptr));
	data.reset(new CANMessageDataView(dataToReceive2.data(), dataToReceive2.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgn1ToReceive, data, originator3, convergingReceiver, nullptr, nullptr));
	data.reset(new CANMessageDataView(dataToReceive1.data(), dataToReceive1.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgn2ToReceive, data, originator4, convergingReceiver, nullptr, nullptr));
	data.reset(new CANMessageDataView(dataToReceive2.data(), dataToReceive2.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgn2ToReceive, data, originator5, convergingReceiver, nullptr, nullptr));

	// Send the diverging message
	data.reset(new CANMessageDataView(dataToReceive1.data(), dataToReceive1.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgn1ToReceive, data, divergingOriginator, receiver1, nullptr, nullptr));
	data.reset(new CANMessageDataView(dataToReceive1.data(), dataToReceive1.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgn1ToReceive, data, divergingOriginator, receiver2, nullptr, nullptr));
	data.reset(new CANMessageDataView(dataToReceive2.data(), dataToReceive2.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgn1ToReceive, data, divergingOriginator, receiver3, nullptr, nullptr));
	data.reset(new CANMessageDataView(dataToReceive1.data(), dataToReceive1.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgn2ToReceive, data, divergingOriginator, receiver4, nullptr, nullptr));
	data.reset(new CANMessageDataView(dataToReceive2.data(), dataToReceive2.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgn2ToReceive, data, divergingOriginator, receiver5, nullptr, nullptr));

	ASSERT_TRUE(txManager.has_session(originator1, convergingReceiver));
	ASSERT_TRUE(txManager.has_session(originator2, convergingReceiver));
	ASSERT_TRUE(txManager.has_session(originator3, convergingReceiver));
	ASSERT_TRUE(txManager.has_session(originator4, convergingReceiver));
	ASSERT_TRUE(txManager.has_session(originator5, convergingReceiver));
	ASSERT_TRUE(txManager.has_session(divergingOriginator, receiver1));
	ASSERT_TRUE(txManager.has_session(divergingOriginator, receiver2));
	ASSERT_TRUE(txManager.has_session(divergingOriginator, receiver3));
	ASSERT_TRUE(txManager.has_session(divergingOriginator, receiver4));
	ASSERT_TRUE(txManager.has_session(divergingOriginator, receiver5));

	// Wait for the transmissions to finish (or timeout)
	std::uint32_t time = SystemTiming::get_timestamp_ms();
	while ((std::any_of(completedConnections.begin(), completedConnections.end(), [](bool completed) { return !completed; })) && // Wait for all connections to be completed
	       (SystemTiming::get_time_elapsed_ms(time) < 1250 + 200 + 200 + 200 + 200 + 1250)) // Or, maximum time exceeded for 4 packets with 1 CTS according to ISO 11783-3
	{
		if (!originatingQueue.empty())
		{
			rxManager.process_message(originatingQueue.front());
			originatingQueue.pop_front();
		}
		if (!receivingQueue.empty())
		{
			txManager.process_message(receivingQueue.front());
			receivingQueue.pop_front();
		}
		txManager.update();
		rxManager.update();
	}

	// Check that all connections are completed
	ASSERT_TRUE(std::all_of(completedConnections.begin(), completedConnections.end(), [](bool completed) { return completed; }));

	// After the transmission is finished, the sessions should be removed as indication that connection is closed
	ASSERT_FALSE(txManager.has_session(originator1, convergingReceiver));
	ASSERT_FALSE(txManager.has_session(originator2, convergingReceiver));
	ASSERT_FALSE(txManager.has_session(originator3, convergingReceiver));
	ASSERT_FALSE(txManager.has_session(originator4, convergingReceiver));
	ASSERT_FALSE(txManager.has_session(originator5, convergingReceiver));
	ASSERT_FALSE(txManager.has_session(divergingOriginator, receiver1));
	ASSERT_FALSE(txManager.has_session(divergingOriginator, receiver2));
	ASSERT_FALSE(txManager.has_session(divergingOriginator, receiver3));
	ASSERT_FALSE(txManager.has_session(divergingOriginator, receiver4));
	ASSERT_FALSE(txManager.has_session(divergingOriginator, receiver5));
	ASSERT_FALSE(rxManager.has_session(originator1, convergingReceiver));
	ASSERT_FALSE(rxManager.has_session(originator2, convergingReceiver));
	ASSERT_FALSE(rxManager.has_session(originator3, convergingReceiver));
	ASSERT_FALSE(rxManager.has_session(originator4, convergingReceiver));
	ASSERT_FALSE(rxManager.has_session(originator5, convergingReceiver));
	ASSERT_FALSE(rxManager.has_session(divergingOriginator, receiver1));
	ASSERT_FALSE(rxManager.has_session(divergingOriginator, receiver2));
	ASSERT_FALSE(rxManager.has_session(divergingOriginator, receiver3));
	ASSERT_FALSE(rxManager.has_session(divergingOriginator, receiver4));
	ASSERT_FALSE(rxManager.has_session(divergingOriginator, receiver5));
}

// Test case for concurrent destination specific and broadcast messages from same source
TEST(TRANSPORT_PROTOCOL_TESTS, DestinationSpecificAndBroadcastMessageConcurrent)
{
	constexpr std::uint32_t pgnToReceiveBroadcast = 0xFEEC;
	constexpr std::uint32_t pgnToReceiveSpecific = 0xFEEB;
	constexpr std::array<std::uint8_t, 17> dataToReceiveBroadcast = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11 };
	constexpr std::array<std::uint8_t, 12> dataToReceiveSpecific = { 0xAC, 0xAB, 0xAA, 0xA9, 0xA8, 0xA7, 0xA6, 0xA5, 0xA4, 0xA3, 0xA2, 0xA1 };

	auto originator = test_helpers::create_mock_internal_control_function(0x01);
	auto receiver = test_helpers::create_mock_internal_control_function(0x02);

	std::deque<CANMessage> originatingQueue;
	std::deque<CANMessage> receivingQueue;

	bool broadcastCompleted = false;
	bool specificCompleted = false;

	auto receiveMessageCallback = [&](const CANMessage &message) {
		CANIdentifier identifier = message.get_identifier();
		EXPECT_EQ(identifier.get_priority(), CANIdentifier::CANPriority::PriorityDefault6);

		if (message.is_broadcast())
		{
			EXPECT_EQ(identifier.get_parameter_group_number(), pgnToReceiveBroadcast);
			EXPECT_EQ(message.get_data_length(), dataToReceiveBroadcast.size());
			for (std::size_t i = 0; i < dataToReceiveBroadcast.size(); i++)
			{
				EXPECT_EQ(message.get_data()[i], dataToReceiveBroadcast[i]);
			}
			broadcastCompleted = true;
		}
		else
		{
			EXPECT_EQ(identifier.get_parameter_group_number(), pgnToReceiveSpecific);
			EXPECT_EQ(message.get_data_length(), dataToReceiveSpecific.size());
			for (std::size_t i = 0; i < dataToReceiveSpecific.size(); i++)
			{
				EXPECT_EQ(message.get_data()[i], dataToReceiveSpecific[i]);
			}
			specificCompleted = true;
		}
	};

	auto sendFrameCallback = [&](std::uint32_t parameterGroupNumber,
	                             CANDataSpan data,
	                             std::shared_ptr<InternalControlFunction> sourceControlFunction,
	                             std::shared_ptr<ControlFunction> destinationControlFunction,
	                             CANIdentifier::CANPriority priority) {
		if (destinationControlFunction == nullptr)
		{
			// Broadcast message
			CANMessage message = test_helpers::create_message_broadcast(static_cast<std::uint8_t>(priority),
			                                                            parameterGroupNumber,
			                                                            sourceControlFunction,
			                                                            data.begin(),
			                                                            data.size());
			if (sourceControlFunction == originator)
			{
				originatingQueue.push_back(message);
			}
			else
			{
				// Unexpected source, fail the test
				EXPECT_TRUE(false);
			}
		}
		else
		{
			// Destination specific message
			CANMessage message = test_helpers::create_message(static_cast<std::uint8_t>(priority),
			                                                  parameterGroupNumber,
			                                                  destinationControlFunction,
			                                                  sourceControlFunction,
			                                                  data.begin(),
			                                                  data.size());
			if (sourceControlFunction == originator)
			{
				originatingQueue.push_back(message);
			}
			else if (sourceControlFunction == receiver)
			{
				receivingQueue.push_back(message);
			}
			else
			{
				// Unexpected source or destination, fail the test
				EXPECT_TRUE(false);
			}
		}
		return true;
	};

	// Create the transport protocol managers
	CANNetworkConfiguration defaultConfiguration;
	TransportProtocolManager txManager(sendFrameCallback, nullptr, &defaultConfiguration);
	TransportProtocolManager rxManager(sendFrameCallback, receiveMessageCallback, &defaultConfiguration);

	// Send the broadcast message
	auto data = std::unique_ptr<CANMessageData>(new CANMessageDataView(dataToReceiveBroadcast.data(), dataToReceiveBroadcast.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgnToReceiveBroadcast, data, originator, nullptr, nullptr, nullptr));
	ASSERT_TRUE(txManager.has_session(originator, nullptr));

	// Send the destination specific message
	data.reset(new CANMessageDataView(dataToReceiveSpecific.data(), dataToReceiveSpecific.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(pgnToReceiveSpecific, data, originator, receiver, nullptr, nullptr));
	ASSERT_TRUE(txManager.has_session(originator, receiver));

	// Wait for the transmissions to finish (or timeout)
	std::uint32_t time = SystemTiming::get_timestamp_ms();
	while ((!broadcastCompleted || !specificCompleted) && // Wait for both connections to be completed, or
	       (SystemTiming::get_time_elapsed_ms(time) < 1250 + 200 + 200 + 1250)) // maximum time exceeded for 2 packets with 1 CTS according to ISO 11783-3
	{
		if (!originatingQueue.empty())
		{
			rxManager.process_message(originatingQueue.front());
			originatingQueue.pop_front();
		}
		if (!receivingQueue.empty())
		{
			txManager.process_message(receivingQueue.front());
			receivingQueue.pop_front();
		}
		txManager.update();
		rxManager.update();
	}

	// Check that both transmissions are completed
	ASSERT_TRUE(broadcastCompleted);
	ASSERT_TRUE(specificCompleted);

	// After the transmission is finished, the sessions should be removed as indication that connection is closed
	ASSERT_FALSE(txManager.has_session(originator, nullptr));
	ASSERT_FALSE(txManager.has_session(originator, receiver));
	ASSERT_FALSE(rxManager.has_session(originator, nullptr));
	ASSERT_FALSE(rxManager.has_session(originator, receiver));
}

// Test case for abortion of sending destination specific message during initialization
TEST(TRANSPORT_PROTOCOL_TESTS, DestinationSpecificAbortInitiation)
{
	constexpr std::array<std::uint8_t, 9> dataToSent = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };

	auto originator = test_helpers::create_mock_internal_control_function(0x01);
	auto receiver = test_helpers::create_mock_control_function(0x02);
	std::deque<CANMessage> responseQueue;

	std::size_t frameCount = 0;
	auto sendFrameCallback = [&](std::uint32_t parameterGroupNumber,
	                             CANDataSpan data,
	                             std::shared_ptr<InternalControlFunction> sourceControlFunction,
	                             std::shared_ptr<ControlFunction> destinationControlFunction,
	                             CANIdentifier::CANPriority priority) {
		EXPECT_EQ(data.size(), 8);
		EXPECT_EQ(sourceControlFunction, originator);
		EXPECT_EQ(destinationControlFunction, receiver);
		EXPECT_EQ(priority, CANIdentifier::CANPriority::PriorityLowest7);

		switch (frameCount)
		{
			case 0:
				// First we expect a Request to Send (RTS) message
				EXPECT_EQ(parameterGroupNumber, 0xEC00);
				EXPECT_EQ(data[0], 16); // RTS control byte
				EXPECT_EQ(data[1], 9); // Message size
				EXPECT_EQ(data[2], 0); // Message size MSB
				EXPECT_EQ(data[3], 2); // Number of packets
				EXPECT_EQ(data[4], 16); // Limit number of packets in CTS (should be 16 by default to follow recommendation in ISO 11783-3)
				EXPECT_EQ(data[5], 0xEB); // PGN LSB
				EXPECT_EQ(data[6], 0xFE); // PGN middle byte
				EXPECT_EQ(data[7], 0x00); // PGN MSB

				// We respond with an abort message, to deny the connection
				responseQueue.push_back(test_helpers::create_message(
				  7,
				  0xEC00, // Transport Protocol Connection Management
				  sourceControlFunction,
				  destinationControlFunction,
				  {
				    255, // Abort control byte
				    1, // Abort reason 1: Cannot support another connection
				    0xFF, // Reserved
				    0xFF, // Reserved
				    0xFF, // Reserved
				    0xEB, // PGN LSB
				    0xFE, // PGN middle byte
				    0x00, // PGN MSB
				  }));
				break;

			default:
				EXPECT_TRUE(false);
		}

		frameCount++;
		return true;
	};

	// Create the transport protocol manager
	CANNetworkConfiguration defaultConfiguration;
	TransportProtocolManager manager(sendFrameCallback, nullptr, &defaultConfiguration);

	// Send the message
	auto data = std::unique_ptr<CANMessageData>(new CANMessageDataView(dataToSent.data(), dataToSent.size()));
	ASSERT_TRUE(manager.protocol_transmit_message(0xFEEB, data, originator, receiver, nullptr, nullptr));
	ASSERT_TRUE(manager.has_session(originator, receiver));

	// Wait for the transmission to finish (or timeout)
	std::uint32_t time = SystemTiming::get_timestamp_ms();
	while ((!responseQueue.empty()) || ((frameCount < 1) && (SystemTiming::get_time_elapsed_ms(time) < 1250)))
	{
		if (!responseQueue.empty())
		{
			manager.process_message(responseQueue.front());
			responseQueue.pop_front();
		}
		manager.update();
	}

	ASSERT_EQ(frameCount, 1);
	ASSERT_FALSE(manager.has_session(originator, receiver));
}

// Test case for aborting when multiple CTS received by originator after a connection is already established
TEST(TRANSPORT_PROTOCOL_TESTS, DestinationSpecificMultipleCTS)
{
	constexpr std::array<std::uint8_t, 9> dataToSent = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };

	auto originator = test_helpers::create_mock_internal_control_function(0x01);
	auto receiver = test_helpers::create_mock_control_function(0x02);
	std::deque<CANMessage> responseQueue;

	std::size_t frameCount = 0;
	auto sendFrameCallback = [&](std::uint32_t parameterGroupNumber,
	                             CANDataSpan data,
	                             std::shared_ptr<InternalControlFunction> sourceControlFunction,
	                             std::shared_ptr<ControlFunction> destinationControlFunction,
	                             CANIdentifier::CANPriority priority) {
		EXPECT_EQ(data.size(), 8);
		EXPECT_EQ(sourceControlFunction, originator);
		EXPECT_EQ(destinationControlFunction, receiver);
		EXPECT_EQ(priority, CANIdentifier::CANPriority::PriorityLowest7);

		switch (frameCount)
		{
			case 0:
			{
				// First we expect a Request to Send (RTS) message
				EXPECT_EQ(parameterGroupNumber, 0xEC00);
				EXPECT_EQ(data[0], 16); // RTS control byte
				EXPECT_EQ(data[1], 9); // Message size
				EXPECT_EQ(data[2], 0); // Message size MSB
				EXPECT_EQ(data[3], 2); // Number of packets
				EXPECT_EQ(data[4], 16); // Limit number of packets in CTS (should be 16 by default to follow recommendation in ISO 11783-3)
				EXPECT_EQ(data[5], 0xEB); // PGN LSB
				EXPECT_EQ(data[6], 0xFE); // PGN middle byte
				EXPECT_EQ(data[7], 0x00); // PGN MSB

				// We respond with two clear to send (CTS) message
				auto response = test_helpers::create_message(
				  7,
				  0xEC00, // Transport Protocol Connection Management
				  sourceControlFunction,
				  destinationControlFunction,
				  {
				    17, // CTS Mux
				    2, // Number of packets
				    1, // Next packet to send
				    0xFF, // Reserved
				    0xFF, // Reserved
				    0xEB, // PGN LSB
				    0xFE, // PGN middle byte
				    0x00, // PGN MSB
				  });
				responseQueue.push_back(response);
				responseQueue.push_back(response);
			}
			break;

			case 1:
				// Then we expect an abort message
				EXPECT_EQ(parameterGroupNumber, 0xEC00);
				EXPECT_EQ(data[0], 255); // Abort control byte
				EXPECT_EQ(data[1], 4); // Abort reason 4: Unexpected CTS
				EXPECT_EQ(data[2], 0xFF); // Reserved
				EXPECT_EQ(data[3], 0xFF); // Reserved
				EXPECT_EQ(data[4], 0xFF); // Reserved
				EXPECT_EQ(data[5], 0xEB); // PGN LSB
				EXPECT_EQ(data[6], 0xFE); // PGN middle byte
				EXPECT_EQ(data[7], 0x00); // PGN MSB
				break;

			default:
				EXPECT_TRUE(false);
		}

		frameCount++;
		return true;
	};

	// Create the transport protocol manager
	CANNetworkConfiguration defaultConfiguration;
	TransportProtocolManager manager(sendFrameCallback, nullptr, &defaultConfiguration);

	// Send the message
	auto data = std::unique_ptr<CANMessageData>(new CANMessageDataView(dataToSent.data(), dataToSent.size()));
	ASSERT_TRUE(manager.protocol_transmit_message(0xFEEB, data, originator, receiver, nullptr, nullptr));
	ASSERT_TRUE(manager.has_session(originator, receiver));

	// Wait for the transmission to finish (or timeout)
	std::uint32_t time = SystemTiming::get_timestamp_ms();
	while ((!responseQueue.empty()) || ((frameCount < 2) && (SystemTiming::get_time_elapsed_ms(time) < 1250)))
	{
		while (!responseQueue.empty())
		{
			manager.process_message(responseQueue.front());
			responseQueue.pop_front();
		}
		manager.update();
	}

	ASSERT_EQ(frameCount, 2);
	ASSERT_FALSE(manager.has_session(originator, receiver));
}

// Test case for ignoring random CTS messages
TEST(TRANSPORT_PROTOCOL_TESTS, DestinationSpecificRandomCTS)
{
	constexpr std::array<std::uint8_t, 23> dataToSent = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17 };

	auto originator = test_helpers::create_mock_internal_control_function(0x01);
	auto receiver = test_helpers::create_mock_internal_control_function(0x02);
	auto randomControlFunction = test_helpers::create_mock_control_function(0x03);
	std::deque<CANMessage> originatorQueue;
	std::deque<CANMessage> receiverQueue;

	std::size_t messageCount = 0;
	auto receiveMessageCallback = [&](const CANMessage &) {
		messageCount++;
	};

	std::size_t rxFrameCount = 0;
	auto sendFrameCallback = [&](std::uint32_t parameterGroupNumber,
	                             CANDataSpan data,
	                             std::shared_ptr<InternalControlFunction> sourceControlFunction,
	                             std::shared_ptr<ControlFunction> destinationControlFunction,
	                             CANIdentifier::CANPriority priority) {
		CANMessage message = test_helpers::create_message(static_cast<std::uint8_t>(priority),
		                                                  parameterGroupNumber,
		                                                  destinationControlFunction,
		                                                  sourceControlFunction,
		                                                  data.begin(),
		                                                  data.size());

		if (sourceControlFunction == originator)
		{
			originatorQueue.push_back(message);
		}
		else if (sourceControlFunction == receiver)
		{
			receiverQueue.push_back(message);
			rxFrameCount++;
		}
		else
		{
			// Unexpected source, fail the test
			EXPECT_TRUE(false);
		}
		return true;
	};

	CANNetworkConfiguration defaultConfiguration;
	TransportProtocolManager txManager(sendFrameCallback, nullptr, &defaultConfiguration);
	TransportProtocolManager rxManager(sendFrameCallback, receiveMessageCallback, &defaultConfiguration);

	// Send random CTS message
	rxManager.process_message(test_helpers::create_message(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  randomControlFunction,
	  receiver,
	  {
	    17, // CTS Mux
	    2, // Number of packets
	    1, // Next packet to send
	    0xFF, // Reserved
	    0xFF, // Reserved
	    0xEB, // PGN LSB
	    0xFE, // PGN middle byte
	    0x00, // PGN MSB
	  }));

	// Send the message
	auto data = std::unique_ptr<CANMessageData>(new CANMessageDataView(dataToSent.data(), dataToSent.size()));
	ASSERT_TRUE(txManager.protocol_transmit_message(0xFEEB, data, originator, receiver, nullptr, nullptr));
	ASSERT_TRUE(txManager.has_session(originator, receiver));

	// Wait for the transmission to finish (or timeout), while sending some more random CTS messages
	std::uint32_t time = SystemTiming::get_timestamp_ms();
	while ((messageCount < 1) && (SystemTiming::get_time_elapsed_ms(time) < 1250 + 200 + 200 + 200 + 200 + 1250))
	{
		if (!originatorQueue.empty())
		{
			rxManager.process_message(originatorQueue.front());
			originatorQueue.pop_front();
		}
		if (!receiverQueue.empty())
		{
			txManager.process_message(receiverQueue.front());
			receiverQueue.pop_front();
		}
		txManager.update();
		rxManager.update();

		// Send random CTS message
		rxManager.process_message(test_helpers::create_message(
		  7,
		  0xEC00, // Transport Protocol Connection Management
		  randomControlFunction,
		  receiver,
		  {
		    17, // CTS Mux
		    4, // Number of packets
		    2, // Next packet to send
		    0xFF, // Reserved
		    0xFF, // Reserved
		    0xEB, // PGN LSB
		    0xFE, // PGN middle byte
		    0x00, // PGN MSB
		  }));
	}

	ASSERT_EQ(messageCount, 1);
	ASSERT_EQ(rxFrameCount, 2); // One for the CTS, and one for the end of message acknowledgement.
	ASSERT_FALSE(txManager.has_session(originator, receiver));
	ASSERT_FALSE(rxManager.has_session(originator, receiver));
}

// Test case for rejecting a RTS when exceeding the maximum number of sessions
TEST(TRANSPORT_PROTOCOL_TESTS, DestinationSpecificRejectForOutOfResources)
{
	auto originator1 = test_helpers::create_mock_control_function(0x01);
	auto originator2 = test_helpers::create_mock_control_function(0x02);
	auto receiver = test_helpers::create_mock_internal_control_function(0x0B);

	bool originator1CTSReceived = false;
	bool originator2AbortReceived = false;
	auto sendFrameCallback = [&](std::uint32_t parameterGroupNumber,
	                             CANDataSpan data,
	                             std::shared_ptr<InternalControlFunction> sourceControlFunction,
	                             std::shared_ptr<ControlFunction> destinationControlFunction,
	                             CANIdentifier::CANPriority priority) {
		EXPECT_EQ(data.size(), 8);
		EXPECT_EQ(sourceControlFunction, receiver);
		EXPECT_EQ(priority, CANIdentifier::CANPriority::PriorityLowest7);

		if (destinationControlFunction == originator1)
		{
			// We expect a CTS message for originator1
			EXPECT_EQ(parameterGroupNumber, 0xEC00);
			EXPECT_EQ(data[0], 17); // CTS control byte
			EXPECT_EQ(data[1], 2); // Number of packets
			EXPECT_EQ(data[2], 1); // Next packet to send
			EXPECT_EQ(data[3], 0xFF);
			EXPECT_EQ(data[4], 0xFF); // Reserved
			EXPECT_EQ(data[5], 0xEC); // PGN LSB
			EXPECT_EQ(data[6], 0xFE); // PGN middle byte
			EXPECT_EQ(data[7], 0x00); // PGN MSB
			originator1CTSReceived = true;
		}
		else if (destinationControlFunction == originator2)
		{
			// We expect an abort message for originator2
			EXPECT_EQ(parameterGroupNumber, 0xEC00);
			EXPECT_EQ(data[0], 255); // Abort control byte
			EXPECT_EQ(data[1], 1); // Abort reason 1: Cannot support another connection
			EXPECT_EQ(data[2], 0xFF); // Reserved
			EXPECT_EQ(data[3], 0xFF); // Reserved
			EXPECT_EQ(data[4], 0xFF); // Reserved
			EXPECT_EQ(data[5], 0xEB); // PGN LSB
			EXPECT_EQ(data[6], 0xFE); // PGN middle byte
			EXPECT_EQ(data[7], 0x00); // PGN MSB
			originator2AbortReceived = true;
		}
		else
		{
			// Unexpected source, fail the test
			EXPECT_TRUE(false);
		}
		return true;
	};

	// Create the transport protocol manager
	CANNetworkConfiguration configuration;
	configuration.set_max_number_transport_protocol_sessions(1); // We  limit the number of sessions to 1 for this test
	TransportProtocolManager manager(sendFrameCallback, nullptr, &configuration);

	// Send first RTS from originator1
	manager.process_message(test_helpers::create_message(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  receiver,
	  originator1,
	  {
	    16, // RTS control byte
	    9, // Message size
	    0, // Message size MSB
	    2, // Number of packets
	    0xFF,
	    0xEC, // PGN LSB
	    0xFE, // PGN middle byte
	    0x00, // PGN MSB
	  }));

	// Send second RTS from originator2
	manager.process_message(test_helpers::create_message(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  receiver,
	  originator2,
	  {
	    16, // RTS control byte
	    9, // Message size
	    0, // Message size MSB
	    2, // Number of packets
	    0xFF,
	    0xEB, // PGN LSB
	    0xFE, // PGN middle byte
	    0x00, // PGN MSB
	  }));

	// Wait for the transmission to finish (or timeout)
	std::uint32_t time = SystemTiming::get_timestamp_ms();
	while ((!originator1CTSReceived || !originator2AbortReceived) && // Wait for both frames to be sent, or
	       (SystemTiming::get_time_elapsed_ms(time) < 1250 + 200 + 200 + 1250)) // maximum time exceeded for 2 packets with 1 CTS according to ISO 11783-3
	{
		manager.update();
	}

	ASSERT_TRUE(originator1CTSReceived);
	ASSERT_TRUE(originator2AbortReceived);
	ASSERT_TRUE(manager.has_session(originator1, receiver)); // The first connection should still be active
	ASSERT_FALSE(manager.has_session(originator2, receiver)); // The second connection should be rejected
}

// A test case for overwriting a session when a new RTS is received
TEST(TRANSPORT_PROTOCOL_TESTS, DestinationSpecificOverwriteSession)
{
	auto originator = test_helpers::create_mock_control_function(0x01);
	auto receiver = test_helpers::create_mock_internal_control_function(0x0B);

	std::size_t messageCount = 0;
	auto receiveMessageCallback = [&](const CANMessage &message) {
		ASSERT_EQ(message.get_data_length(), 15);
		ASSERT_EQ(message.get_source_control_function(), originator);
		ASSERT_EQ(message.get_destination_control_function(), receiver);
		ASSERT_EQ(message.get_identifier().get_parameter_group_number(), 0xFEEC);
		ASSERT_EQ(message.get_data()[0], 0x01);
		ASSERT_EQ(message.get_data()[1], 0x02);
		ASSERT_EQ(message.get_data()[2], 0x03);
		ASSERT_EQ(message.get_data()[3], 0x04);
		ASSERT_EQ(message.get_data()[4], 0x05);
		ASSERT_EQ(message.get_data()[5], 0x06);
		ASSERT_EQ(message.get_data()[6], 0x07);
		ASSERT_EQ(message.get_data()[7], 0x08);
		ASSERT_EQ(message.get_data()[8], 0x09);
		ASSERT_EQ(message.get_data()[9], 0x0A);
		ASSERT_EQ(message.get_data()[10], 0x0B);
		ASSERT_EQ(message.get_data()[11], 0x0C);
		ASSERT_EQ(message.get_data()[12], 0x0D);
		ASSERT_EQ(message.get_data()[13], 0x0E);
		ASSERT_EQ(message.get_data()[14], 0x0F);
		messageCount++;
	};

	std::size_t frameCount = 0;
	auto sendFrameCallback = [&](std::uint32_t parameterGroupNumber,
	                             CANDataSpan data,
	                             std::shared_ptr<InternalControlFunction> sourceControlFunction,
	                             std::shared_ptr<ControlFunction> destinationControlFunction,
	                             CANIdentifier::CANPriority priority) {
		EXPECT_EQ(data.size(), 8);
		EXPECT_EQ(sourceControlFunction, receiver);
		EXPECT_EQ(destinationControlFunction, originator);
		EXPECT_EQ(priority, CANIdentifier::CANPriority::PriorityLowest7);

		switch (frameCount)
		{
			case 0:
				// First we expect a CTS message for the first RTS
				EXPECT_EQ(parameterGroupNumber, 0xEC00);
				EXPECT_EQ(data[0], 17); // CTS control byte
				EXPECT_EQ(data[1], 2); // Number of packets
				EXPECT_EQ(data[2], 1); // Next packet to send
				EXPECT_EQ(data[3], 0xFF);
				EXPECT_EQ(data[4], 0xFF); // Reserved
				EXPECT_EQ(data[5], 0xEC); // PGN LSB
				EXPECT_EQ(data[6], 0xFE); // PGN middle byte
				EXPECT_EQ(data[7], 0x00); // PGN MSB
				break;

			case 1:
				// Then we expect a CTS message for the second RTS
				EXPECT_EQ(parameterGroupNumber, 0xEC00);
				EXPECT_EQ(data[0], 17); // CTS control byte
				EXPECT_EQ(data[1], 3); // Number of packets
				EXPECT_EQ(data[2], 1); // Next packet to send
				EXPECT_EQ(data[3], 0xFF);
				EXPECT_EQ(data[4], 0xFF); // Reserved
				EXPECT_EQ(data[5], 0xEC); // PGN LSB
				EXPECT_EQ(data[6], 0xFE); // PGN middle byte
				EXPECT_EQ(data[7], 0x00); // PGN MSB
				break;

			case 2:
				// Then we expect a End of Message Acknowledgement for the overwritten session
				EXPECT_EQ(parameterGroupNumber, 0xEC00);
				EXPECT_EQ(data[0], 19); // End of Message Acknowledgement control byte
				EXPECT_EQ(data[1], 15); // Number of bytes
				EXPECT_EQ(data[2], 0); // Number of bytes MSB
				EXPECT_EQ(data[3], 3); // Total number of packets
				EXPECT_EQ(data[4], 0xFF); // Reserved
				EXPECT_EQ(data[5], 0xEC); // PGN LSB
				EXPECT_EQ(data[6], 0xFE); // PGN middle byte
				EXPECT_EQ(data[7], 0x00); // PGN MSB
				break;

			default:
				EXPECT_TRUE(false);
		}

		frameCount++;
		return true;
	};

	// Create the transport protocol manager
	CANNetworkConfiguration defaultConfiguration;
	TransportProtocolManager manager(sendFrameCallback, receiveMessageCallback, &defaultConfiguration);

	// Send first RTS
	manager.process_message(test_helpers::create_message(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  receiver,
	  originator,
	  {
	    16, // RTS control byte
	    9, // Message size
	    0, // Message size MSB
	    2, // Number of packets
	    0xFF,
	    0xEC, // PGN LSB
	    0xFE, // PGN middle byte
	    0x00, // PGN MSB
	  }));

	// Wait for the first CTS to be sent
	std::uint32_t time = SystemTiming::get_timestamp_ms();
	while ((frameCount < 1) && (SystemTiming::get_time_elapsed_ms(time) < 1250))
	{
		manager.update();
	}

	ASSERT_EQ(frameCount, 1);

	// Send the first data frame
	manager.process_message(test_helpers::create_message(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  receiver,
	  originator,
	  {
	    1, // Sequence number
	    0x01,
	    0x02,
	    0x03,
	    0x04,
	    0x05,
	    0x06,
	    0x07,
	  }));

	// Now we overwrite with a new RTS
	manager.process_message(test_helpers::create_message(
	  7,
	  0xEC00, // Transport Protocol Connection Management
	  receiver,
	  originator,
	  {
	    16, // RTS control byte
	    15, // Message size
	    0, // Message size MSB
	    3, // Number of packets
	    0xFF,
	    0xEC, // PGN LSB
	    0xFE, // PGN middle byte
	    0x00, // PGN MSB
	  }));

	// Wait for the second CTS to be sent
	time = SystemTiming::get_timestamp_ms();
	while ((frameCount < 2) && (SystemTiming::get_time_elapsed_ms(time) < 1250))
	{
		manager.update();
	}

	ASSERT_EQ(frameCount, 2);

	// Send the 3 data frames
	manager.process_message(test_helpers::create_message(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  receiver,
	  originator,
	  {
	    1, // Sequence number
	    0x01,
	    0x02,
	    0x03,
	    0x04,
	    0x05,
	    0x06,
	    0x07,
	  }));
	manager.process_message(test_helpers::create_message(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  receiver,
	  originator,
	  {
	    2, // Sequence number
	    0x08,
	    0x09,
	    0x0A,
	    0x0B,
	    0x0C,
	    0x0D,
	    0x0E,
	  }));
	manager.process_message(test_helpers::create_message(
	  7,
	  0xEB00, // Transport Protocol Data Transfer
	  receiver,
	  originator,
	  {
	    3, // Sequence number
	    0x0F,
	    0xFF,
	    0xFF,
	    0xFF,
	    0xFF,
	    0xFF,
	    0xFF,
	  }));

	// Wait for the End of Message Acknowledgement to be sent
	time = SystemTiming::get_timestamp_ms();
	while ((frameCount < 3) && (SystemTiming::get_time_elapsed_ms(time) < 1250))
	{
		manager.update();
	}

	ASSERT_EQ(frameCount, 3);
	ASSERT_EQ(messageCount, 1);

	// After the transmission is finished, the sessions should be removed as indication that connection is closed
	ASSERT_FALSE(manager.has_session(originator, receiver));
}
