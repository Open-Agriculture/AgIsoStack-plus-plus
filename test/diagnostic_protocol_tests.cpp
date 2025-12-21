#include <gtest/gtest.h>

#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/isobus_diagnostic_protocol.hpp"
#include "isobus/utility/system_timing.hpp"

#include "helpers/control_function_helpers.hpp"
#include "helpers/messaging_helpers.hpp"

using namespace isobus;

TEST(DIAGNOSTIC_PROTOCOL_TESTS, CreateAndDestroyProtocolObjects)
{
	NAME TestDeviceNAME(0);
	auto TestInternalECU = CANNetworkManager::CANNetwork.create_internal_control_function(TestDeviceNAME, 0, 0x1C);

	std::unique_ptr<DiagnosticProtocol> diagnosticProtocol;
	diagnosticProtocol.reset(new DiagnosticProtocol(TestInternalECU));
	EXPECT_TRUE(diagnosticProtocol->initialize());
	EXPECT_FALSE(diagnosticProtocol->initialize()); // Should not be able to initialize twice

	auto pgnRequestProtocol = TestInternalECU->get_pgn_request_protocol().lock();
	ASSERT_TRUE(pgnRequestProtocol);

	EXPECT_NO_THROW(diagnosticProtocol->terminate());
	diagnosticProtocol.reset();

	EXPECT_EQ(pgnRequestProtocol->get_number_registered_pgn_request_callbacks(), 0);
	EXPECT_EQ(pgnRequestProtocol->get_number_registered_request_for_repetition_rate_callbacks(), 1); // The heartbeat is registered by default

	pgnRequestProtocol.reset();

	CANNetworkManager::CANNetwork.deactivate_control_function(TestInternalECU);
}

TEST(DIAGNOSTIC_PROTOCOL_TESTS, MessageEncoding)
{
	VirtualCANPlugin testPlugin;
	testPlugin.open();

	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();

	auto TestInternalECU = test_helpers::claim_internal_control_function(0xAA, 0);
	auto TestPartneredECU = test_helpers::force_claim_partnered_control_function(0xAB, 0);
	DiagnosticProtocol protocolUnderTest(TestInternalECU, DiagnosticProtocol::NetworkType::SAEJ1939Network1PrimaryVehicleNetwork);

	EXPECT_FALSE(protocolUnderTest.get_initialized());
	protocolUnderTest.initialize();
	EXPECT_TRUE(protocolUnderTest.get_initialized());

	// Get the virtual CAN plugin back to a known state
	CANMessageFrame testFrame = {};
	while (!testPlugin.get_queue_empty())
	{
		testPlugin.read_frame(testFrame);
	}
	ASSERT_TRUE(testPlugin.get_queue_empty());

	// Ready to run some tests
	std::cerr << "These tests use TP CM to transmit." << std::endl;

	{
		// Test ECU ID format against J1939-71
		protocolUnderTest.set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::HardwareID, "Some Hardware ID");
		protocolUnderTest.set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::Location, "The Internet");
		protocolUnderTest.set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::ManufacturerName, "None");
		protocolUnderTest.set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::PartNumber, "1234");
		protocolUnderTest.set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::SerialNumber, "9876");
		protocolUnderTest.set_ecu_id_field(isobus::DiagnosticProtocol::ECUIdentificationFields::Type, "AgISOStack");

		// Use a PGN request to trigger sending it from the protocol
		testFrame.dataLength = 3;
		testFrame.identifier = test_helpers::create_ext_can_id(6, 0xEA00, TestInternalECU, TestPartneredECU);
		testFrame.data[0] = 0xC5;
		testFrame.data[1] = 0xFD;
		testFrame.data[2] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		// Make sure we're using ISO mode for this parsing to work
		ASSERT_FALSE(protocolUnderTest.get_j1939_mode());

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		std::uint16_t expectedLength = 56; // This is all strings lengths plus delimiters

		// RTS Message
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CECABAA, testFrame.identifier); // TP CM from address AA
		EXPECT_EQ(0x10, testFrame.data[0]); // RTS Multiplexer
		EXPECT_EQ(expectedLength & 0xFF, testFrame.data[1]); // Length LSB
		EXPECT_EQ((expectedLength >> 8) & 0xFF, testFrame.data[2]); // Length MSB
		EXPECT_EQ(0x08, testFrame.data[3]); // Number of frames in session (based on length)
		EXPECT_EQ(0x10, testFrame.data[4]); // Always 0xFF
		EXPECT_EQ(0xC5, testFrame.data[5]); // PGN LSB
		EXPECT_EQ(0xFD, testFrame.data[6]); // PGN
		EXPECT_EQ(0x00, testFrame.data[7]); // PGN MSB

		// Send CTS message
		testFrame.dataLength = 8;
		testFrame.identifier = test_helpers::create_ext_can_id(6, 0xEC00, TestInternalECU, TestPartneredECU);
		testFrame.data[0] = 0x11; // CTS Multiplexer
		testFrame.data[1] = 0x08; // Number of frames to send
		testFrame.data[2] = 0x01;
		testFrame.data[3] = 0xFF;
		testFrame.data[4] = 0xFF;
		testFrame.data[5] = 0xC5;
		testFrame.data[6] = 0xFD;
		testFrame.data[7] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// CM DATA Payload Frame 1
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // DT from address AA
		EXPECT_EQ(0x01, testFrame.data[0]); // Sequence 1
		EXPECT_EQ('1', testFrame.data[1]); // Part Number index 0
		EXPECT_EQ('2', testFrame.data[2]); // Part Number index 1
		EXPECT_EQ('3', testFrame.data[3]); // Part Number index 2
		EXPECT_EQ('4', testFrame.data[4]); // Part Number index 3
		EXPECT_EQ('*', testFrame.data[5]); // Delimiter
		EXPECT_EQ('9', testFrame.data[6]); // Serial number index 0
		EXPECT_EQ('8', testFrame.data[7]); // Serial number index 1

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// CM DATA Payload Frame 2
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // DT from address AA
		EXPECT_EQ(0x02, testFrame.data[0]); // Sequence 2
		EXPECT_EQ('7', testFrame.data[1]); // Serial number index 2
		EXPECT_EQ('6', testFrame.data[2]); // Serial number index 3
		EXPECT_EQ('*', testFrame.data[3]); // Delimiter
		EXPECT_EQ('T', testFrame.data[4]); // Location index 0
		EXPECT_EQ('h', testFrame.data[5]); // Location index 1
		EXPECT_EQ('e', testFrame.data[6]); // Location index 2
		EXPECT_EQ(' ', testFrame.data[7]); // Location index 3

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// CM DATA Payload Frame 3
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // DT from address AA
		EXPECT_EQ(0x03, testFrame.data[0]); // Sequence 3
		EXPECT_EQ('I', testFrame.data[1]); // Location index 4
		EXPECT_EQ('n', testFrame.data[2]); // Location index 5
		EXPECT_EQ('t', testFrame.data[3]); // Location index 6
		EXPECT_EQ('e', testFrame.data[4]); // Location index 7
		EXPECT_EQ('r', testFrame.data[5]); // Location index 8
		EXPECT_EQ('n', testFrame.data[6]); // Location index 9
		EXPECT_EQ('e', testFrame.data[7]); // Location index 10

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// CM DATA Payload Frame 4
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // DT from address AA
		EXPECT_EQ(0x04, testFrame.data[0]); // Sequence 4
		EXPECT_EQ('t', testFrame.data[1]); // Location index 11
		EXPECT_EQ('*', testFrame.data[2]); // Delimiter
		EXPECT_EQ('A', testFrame.data[3]); // Type Index 0
		EXPECT_EQ('g', testFrame.data[4]); // Type Index 1
		EXPECT_EQ('I', testFrame.data[5]); // Type Index 2
		EXPECT_EQ('S', testFrame.data[6]); // Type Index 3
		EXPECT_EQ('O', testFrame.data[7]); // Type Index 4

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// CM DATA Payload Frame 5
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // DT from address AA
		EXPECT_EQ(0x05, testFrame.data[0]); // Sequence 5
		EXPECT_EQ('S', testFrame.data[1]); // Type Index 5
		EXPECT_EQ('t', testFrame.data[2]); // Type Index 6
		EXPECT_EQ('a', testFrame.data[3]); // Type Index 7
		EXPECT_EQ('c', testFrame.data[4]); // Type Index 8
		EXPECT_EQ('k', testFrame.data[5]); // Type Index 9
		EXPECT_EQ('*', testFrame.data[6]); // Delimiter
		EXPECT_EQ('N', testFrame.data[7]); // Manufacturer index 0

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// CM DATA Payload Frame 6
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // DT from address AA
		EXPECT_EQ(0x06, testFrame.data[0]); // Sequence 6
		EXPECT_EQ('o', testFrame.data[1]); // Manufacturer index 1
		EXPECT_EQ('n', testFrame.data[2]); // Manufacturer index 2
		EXPECT_EQ('e', testFrame.data[3]); // Manufacturer index 3
		EXPECT_EQ('*', testFrame.data[4]); // Delimiter
		EXPECT_EQ('S', testFrame.data[5]); // Hardware ID Index 0
		EXPECT_EQ('o', testFrame.data[6]); // Hardware ID Index 1
		EXPECT_EQ('m', testFrame.data[7]); // Hardware ID Index 2

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// CM DATA Payload Frame 7
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // DT from address AA
		EXPECT_EQ(0x07, testFrame.data[0]); // Sequence 7
		EXPECT_EQ('e', testFrame.data[1]); // Hardware ID Index 3
		EXPECT_EQ(' ', testFrame.data[2]); // Hardware ID Index 4
		EXPECT_EQ('H', testFrame.data[3]); // Hardware ID Index 5
		EXPECT_EQ('a', testFrame.data[4]); // Hardware ID Index 6
		EXPECT_EQ('r', testFrame.data[5]); // Hardware ID Index 7
		EXPECT_EQ('d', testFrame.data[6]); // Hardware ID Index 8
		EXPECT_EQ('w', testFrame.data[7]); // Hardware ID Index 9

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// CM DATA Payload Frame 8
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // DT from address AA
		EXPECT_EQ(0x08, testFrame.data[0]); // Sequence 8
		EXPECT_EQ('a', testFrame.data[1]); // Hardware ID Index 10
		EXPECT_EQ('r', testFrame.data[2]); // Hardware ID Index 11
		EXPECT_EQ('e', testFrame.data[3]); // Hardware ID Index 12
		EXPECT_EQ(' ', testFrame.data[4]); // Hardware ID Index 13
		EXPECT_EQ('I', testFrame.data[5]); // Hardware ID Index 14
		EXPECT_EQ('D', testFrame.data[6]); // Hardware ID Index 15
		EXPECT_EQ('*', testFrame.data[7]); // Delimiter (end of the message)

		// Send EOM ACK
		testFrame.dataLength = 8;
		testFrame.identifier = test_helpers::create_ext_can_id(6, 0xEC00, TestInternalECU, TestPartneredECU);
		testFrame.data[0] = 0x13; // EOM Multiplexer
		testFrame.data[1] = expectedLength & 0xFF;
		testFrame.data[2] = (expectedLength >> 8) & 0xFF;
		testFrame.data[3] = 0x08; // Number of frames
		testFrame.data[4] = 0xFF;
		testFrame.data[5] = 0xC5;
		testFrame.data[6] = 0xFD;
		testFrame.data[7] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();
	}

	{
		// Re-test in J1939 mode. Should omit the hardware ID
		protocolUnderTest.set_j1939_mode(true);

		// Use a PGN request to trigger sending it from the protocol
		testFrame.dataLength = 3;
		testFrame.identifier = 0x18EAAAAB;
		testFrame.data[0] = 0xC5;
		testFrame.data[1] = 0xFD;
		testFrame.data[2] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();

		protocolUnderTest.update();

		// Make sure we're using ISO mode for this parsing to work
		ASSERT_TRUE(protocolUnderTest.get_j1939_mode());

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// DM1 might be sent in j1939 mode, need to screen it out
		if (((testFrame.identifier >> 8) & 0xFFFF) == 0xFECA)
		{
			EXPECT_TRUE(testPlugin.read_frame(testFrame));
		}

		std::uint16_t expectedLength = 39; // This is all strings lengths plus delimiters

		// RTS Message
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CECABAA, testFrame.identifier); // TP CM from address AA
		EXPECT_EQ(0x10, testFrame.data[0]); // RTS Multiplexer
		EXPECT_EQ(expectedLength & 0xFF, testFrame.data[1]); // Length LSB
		EXPECT_EQ((expectedLength >> 8) & 0xFF, testFrame.data[2]); // Length MSB
		EXPECT_EQ(0x06, testFrame.data[3]); // Number of frames in session (based on length)
		EXPECT_EQ(0x10, testFrame.data[4]); // Always 0x10
		EXPECT_EQ(0xC5, testFrame.data[5]); // PGN LSB
		EXPECT_EQ(0xFD, testFrame.data[6]); // PGN
		EXPECT_EQ(0x00, testFrame.data[7]); // PGN MSB

		// Send CTS message
		testFrame.dataLength = 8;
		testFrame.identifier = test_helpers::create_ext_can_id(6, 0xEC00, TestInternalECU, TestPartneredECU);
		testFrame.data[0] = 0x11; // CTS Multiplexer
		testFrame.data[1] = 0x06; // Number of frames to send
		testFrame.data[2] = 0x01;
		testFrame.data[3] = 0xFF;
		testFrame.data[4] = 0xFF;
		testFrame.data[5] = 0xC5;
		testFrame.data[6] = 0xFD;
		testFrame.data[7] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// DM1 might be sent in j1939 mode, need to screen it out
		if (((testFrame.identifier >> 8) & 0xFFFF) == 0xFECA)
		{
			EXPECT_TRUE(testPlugin.read_frame(testFrame));
		}

		// BAM Payload Frame 1
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x01, testFrame.data[0]); // Sequence 1
		EXPECT_EQ('1', testFrame.data[1]); // Part Number index 0
		EXPECT_EQ('2', testFrame.data[2]); // Part Number index 1
		EXPECT_EQ('3', testFrame.data[3]); // Part Number index 2
		EXPECT_EQ('4', testFrame.data[4]); // Part Number index 3
		EXPECT_EQ('*', testFrame.data[5]); // Delimiter
		EXPECT_EQ('9', testFrame.data[6]); // Serial number index 0
		EXPECT_EQ('8', testFrame.data[7]); // Serial number index 1

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// DM1 might be sent in j1939 mode, need to screen it out
		if (((testFrame.identifier >> 8) & 0xFFFF) == 0xFECA)
		{
			EXPECT_TRUE(testPlugin.read_frame(testFrame));
		}

		// BAM Payload Frame 2
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x02, testFrame.data[0]); // Sequence 2
		EXPECT_EQ('7', testFrame.data[1]); // Serial number index 2
		EXPECT_EQ('6', testFrame.data[2]); // Serial number index 3
		EXPECT_EQ('*', testFrame.data[3]); // Delimiter
		EXPECT_EQ('T', testFrame.data[4]); // Location index 0
		EXPECT_EQ('h', testFrame.data[5]); // Location index 1
		EXPECT_EQ('e', testFrame.data[6]); // Location index 2
		EXPECT_EQ(' ', testFrame.data[7]); // Location index 3

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// DM1 might be sent in j1939 mode, need to screen it out
		if (((testFrame.identifier >> 8) & 0xFFFF) == 0xFECA)
		{
			EXPECT_TRUE(testPlugin.read_frame(testFrame));
		}

		// BAM Payload Frame 3
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x03, testFrame.data[0]); // Sequence 3
		EXPECT_EQ('I', testFrame.data[1]); // Location index 4
		EXPECT_EQ('n', testFrame.data[2]); // Location index 5
		EXPECT_EQ('t', testFrame.data[3]); // Location index 6
		EXPECT_EQ('e', testFrame.data[4]); // Location index 7
		EXPECT_EQ('r', testFrame.data[5]); // Location index 8
		EXPECT_EQ('n', testFrame.data[6]); // Location index 9
		EXPECT_EQ('e', testFrame.data[7]); // Location index 10

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// DM1 might be sent in j1939 mode, need to screen it out
		if (((testFrame.identifier >> 8) & 0xFFFF) == 0xFECA)
		{
			EXPECT_TRUE(testPlugin.read_frame(testFrame));
		}

		// BAM Payload Frame 4
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x04, testFrame.data[0]); // Sequence 4
		EXPECT_EQ('t', testFrame.data[1]); // Location index 11
		EXPECT_EQ('*', testFrame.data[2]); // Delimiter
		EXPECT_EQ('A', testFrame.data[3]); // Type Index 0
		EXPECT_EQ('g', testFrame.data[4]); // Type Index 1
		EXPECT_EQ('I', testFrame.data[5]); // Type Index 2
		EXPECT_EQ('S', testFrame.data[6]); // Type Index 3
		EXPECT_EQ('O', testFrame.data[7]); // Type Index 4

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// DM1 might be sent in j1939 mode, need to screen it out
		if (((testFrame.identifier >> 8) & 0xFFFF) == 0xFECA)
		{
			EXPECT_TRUE(testPlugin.read_frame(testFrame));
		}

		// BAM Payload Frame 5
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x05, testFrame.data[0]); // Sequence 5
		EXPECT_EQ('S', testFrame.data[1]); // Type Index 5
		EXPECT_EQ('t', testFrame.data[2]); // Type Index 6
		EXPECT_EQ('a', testFrame.data[3]); // Type Index 7
		EXPECT_EQ('c', testFrame.data[4]); // Type Index 8
		EXPECT_EQ('k', testFrame.data[5]); // Type Index 9
		EXPECT_EQ('*', testFrame.data[6]); // Delimiter
		EXPECT_EQ('N', testFrame.data[7]); // Manufacturer index 0

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// DM1 might be sent in j1939 mode, need to screen it out
		if (((testFrame.identifier >> 8) & 0xFFFF) == 0xFECA)
		{
			EXPECT_TRUE(testPlugin.read_frame(testFrame));
		}

		// BAM Payload Frame 6
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x06, testFrame.data[0]); // Sequence 6
		EXPECT_EQ('o', testFrame.data[1]); // Manufacturer index 1
		EXPECT_EQ('n', testFrame.data[2]); // Manufacturer index 2
		EXPECT_EQ('e', testFrame.data[3]); // Manufacturer index 3
		EXPECT_EQ('*', testFrame.data[4]); // Delimiter
		EXPECT_EQ(0xFF, testFrame.data[5]); // Padding
		EXPECT_EQ(0xFF, testFrame.data[6]); // Padding
		EXPECT_EQ(0xFF, testFrame.data[7]); // Padding

		// Send EOM ACK
		testFrame.dataLength = 8;
		testFrame.identifier = test_helpers::create_ext_can_id(6, 0xEC00, TestInternalECU, TestPartneredECU);
		testFrame.data[0] = 0x13; // EOM Multiplexer
		testFrame.data[1] = expectedLength & 0xFF;
		testFrame.data[2] = (expectedLength >> 8) & 0xFF;
		testFrame.data[3] = 0x06; // Number of frames
		testFrame.data[4] = 0xFF;
		testFrame.data[5] = 0xC5;
		testFrame.data[6] = 0xFD;
		testFrame.data[7] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		protocolUnderTest.set_j1939_mode(false);
		EXPECT_FALSE(protocolUnderTest.get_j1939_mode());
	}

	{
		/// Now, test software ID against J1939-71
		protocolUnderTest.set_software_id_field(0, "Unit Test 1.0.0");
		protocolUnderTest.set_software_id_field(1, "Another version x.x.x.x");

		// Use a PGN request to trigger sending it from the protocol
		testFrame.dataLength = 3;
		testFrame.identifier = 0x18EAAAAB;
		testFrame.data[0] = 0xDA;
		testFrame.data[1] = 0xFE;
		testFrame.data[2] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();

		protocolUnderTest.update();

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		std::uint16_t expectedLength = 40; // This is all strings lengths plus delimiters

		// RTS Message
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CECABAA, testFrame.identifier); // TP CM from address AA
		EXPECT_EQ(0x10, testFrame.data[0]); // RTS Multiplexer
		EXPECT_EQ(expectedLength & 0xFF, testFrame.data[1]); // Length LSB
		EXPECT_EQ((expectedLength >> 8) & 0xFF, testFrame.data[2]); // Length MSB
		EXPECT_EQ(0x06, testFrame.data[3]); // Number of frames in session (based on length)
		EXPECT_EQ(0x10, testFrame.data[4]); // Always 0x10
		EXPECT_EQ(0xDA, testFrame.data[5]); // PGN LSB
		EXPECT_EQ(0xFE, testFrame.data[6]); // PGN
		EXPECT_EQ(0x00, testFrame.data[7]); // PGN MSB

		// Send CTS message
		testFrame.dataLength = 8;
		testFrame.identifier = test_helpers::create_ext_can_id(6, 0xEC00, TestInternalECU, TestPartneredECU);
		testFrame.data[0] = 0x11; // CTS Multiplexer
		testFrame.data[1] = 0x06; // Number of frames to send
		testFrame.data[2] = 0x01;
		testFrame.data[3] = 0xFF;
		testFrame.data[4] = 0xFF;
		testFrame.data[5] = 0xDA;
		testFrame.data[6] = 0xFE;
		testFrame.data[7] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 1
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x01, testFrame.data[0]); // Sequence 1
		EXPECT_EQ('U', testFrame.data[1]); // Version 0, index 0
		EXPECT_EQ('n', testFrame.data[2]); // Version 0, index 1
		EXPECT_EQ('i', testFrame.data[3]); // Version 0, index 2
		EXPECT_EQ('t', testFrame.data[4]); // Version 0, index 3
		EXPECT_EQ(' ', testFrame.data[5]); // Version 0, index 4
		EXPECT_EQ('T', testFrame.data[6]); // Version 0, index 5
		EXPECT_EQ('e', testFrame.data[7]); // Version 0, index 6

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 2
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x02, testFrame.data[0]); // Sequence 2
		EXPECT_EQ('s', testFrame.data[1]); // Version 0, index 7
		EXPECT_EQ('t', testFrame.data[2]); // Version 0, index 8
		EXPECT_EQ(' ', testFrame.data[3]); // Version 0, index 9
		EXPECT_EQ('1', testFrame.data[4]); // Version 0, index 10
		EXPECT_EQ('.', testFrame.data[5]); // Version 0, index 11
		EXPECT_EQ('0', testFrame.data[6]); // Version 0, index 12
		EXPECT_EQ('.', testFrame.data[7]); // Version 0, index 13

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 3
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x03, testFrame.data[0]); // Sequence 3
		EXPECT_EQ('0', testFrame.data[1]); // Version 0, index 7
		EXPECT_EQ('*', testFrame.data[2]); // Delimiter
		EXPECT_EQ('A', testFrame.data[3]); // Version 1, index 0
		EXPECT_EQ('n', testFrame.data[4]); // Version 1, index 1
		EXPECT_EQ('o', testFrame.data[5]); // Version 1, index 2
		EXPECT_EQ('t', testFrame.data[6]); // Version 1, index 3
		EXPECT_EQ('h', testFrame.data[7]); // Version 1, index 4

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 4
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x04, testFrame.data[0]); // Sequence 4
		EXPECT_EQ('e', testFrame.data[1]); // Version 0, index 7
		EXPECT_EQ('r', testFrame.data[2]); // Delimiter
		EXPECT_EQ(' ', testFrame.data[3]); // Version 1, index 5
		EXPECT_EQ('v', testFrame.data[4]); // Version 1, index 6
		EXPECT_EQ('e', testFrame.data[5]); // Version 1, index 7
		EXPECT_EQ('r', testFrame.data[6]); // Version 1, index 8
		EXPECT_EQ('s', testFrame.data[7]); // Version 1, index 9

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 5
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x05, testFrame.data[0]); // Sequence 5
		EXPECT_EQ('i', testFrame.data[1]); // Version 0, index 7
		EXPECT_EQ('o', testFrame.data[2]); // Delimiter
		EXPECT_EQ('n', testFrame.data[3]); // Version 1, index 5
		EXPECT_EQ(' ', testFrame.data[4]); // Version 1, index 6
		EXPECT_EQ('x', testFrame.data[5]); // Version 1, index 7
		EXPECT_EQ('.', testFrame.data[6]); // Version 1, index 8
		EXPECT_EQ('x', testFrame.data[7]); // Version 1, index 9

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 6
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x06, testFrame.data[0]); // Sequence 6
		EXPECT_EQ('.', testFrame.data[1]); // Version 0, index 10
		EXPECT_EQ('x', testFrame.data[2]); // Version 0, index 11
		EXPECT_EQ('.', testFrame.data[3]); // Version 1, index 12
		EXPECT_EQ('x', testFrame.data[4]); // Version 1, index 13
		EXPECT_EQ('*', testFrame.data[5]); // Delimiter
		EXPECT_EQ(0xFF, testFrame.data[6]); // Padding
		EXPECT_EQ(0xFF, testFrame.data[7]); // Padding

		// Send EOM ACK
		testFrame.dataLength = 8;
		testFrame.identifier = test_helpers::create_ext_can_id(6, 0xEC00, TestInternalECU, TestPartneredECU);
		testFrame.data[0] = 0x13; // EOM Multiplexer
		testFrame.data[1] = expectedLength & 0xFF;
		testFrame.data[2] = (expectedLength >> 8) & 0xFF;
		testFrame.data[3] = 0x06; // Number of frames
		testFrame.data[4] = 0xFF;
		testFrame.data[5] = 0xDA;
		testFrame.data[6] = 0xFE;
		testFrame.data[7] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();
	}

	{
		// Test diagnostic protocol identification message
		// Use a PGN request to trigger sending it from the protocol
		testFrame.dataLength = 3;
		testFrame.identifier = 0x18EAAAAB;
		testFrame.data[0] = 0x32;
		testFrame.data[1] = 0xFD;
		testFrame.data[2] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();

		protocolUnderTest.update();

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x18FD32AA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x01, testFrame.data[0]); // J1939-73
		EXPECT_EQ(0xFF, testFrame.data[1]); // Reserved
		EXPECT_EQ(0xFF, testFrame.data[2]); // Reserved
		EXPECT_EQ(0xFF, testFrame.data[3]); // Reserved
		EXPECT_EQ(0xFF, testFrame.data[4]); // Reserved
		EXPECT_EQ(0xFF, testFrame.data[5]); // Reserved
		EXPECT_EQ(0xFF, testFrame.data[6]); // Padding
		EXPECT_EQ(0xFF, testFrame.data[7]); // Padding
	}

	{
		// Test Product Identification
		protocolUnderTest.set_product_identification_code("1234567890ABC");
		protocolUnderTest.set_product_identification_brand("Open-Agriculture");
		protocolUnderTest.set_product_identification_model("AgIsoStack++");
		// Use a PGN request to trigger sending it
		testFrame.dataLength = 3;
		testFrame.identifier = 0x18EAAAAB;
		testFrame.data[0] = 0x8D;
		testFrame.data[1] = 0xFC;
		testFrame.data[2] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();

		protocolUnderTest.update();

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		std::uint16_t expectedLength = 44; // This is all strings lengths plus delimiters

		// RTS Message
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CECABAA, testFrame.identifier); // TP CM from address AA
		EXPECT_EQ(0x10, testFrame.data[0]); // RTS Multiplexer
		EXPECT_EQ(expectedLength & 0xFF, testFrame.data[1]); // Length LSB
		EXPECT_EQ((expectedLength >> 8) & 0xFF, testFrame.data[2]); // Length MSB
		EXPECT_EQ(0x07, testFrame.data[3]); // Number of frames in session (based on length)
		EXPECT_EQ(0x10, testFrame.data[4]); // Always 0x10
		EXPECT_EQ(0x8D, testFrame.data[5]); // PGN LSB
		EXPECT_EQ(0xFC, testFrame.data[6]); // PGN
		EXPECT_EQ(0x00, testFrame.data[7]); // PGN MSB

		// Send CTS message
		testFrame.dataLength = 8;
		testFrame.identifier = test_helpers::create_ext_can_id(6, 0xEC00, TestInternalECU, TestPartneredECU);
		testFrame.data[0] = 0x11; // CTS Multiplexer
		testFrame.data[1] = 0x07; // Number of frames to send
		testFrame.data[2] = 0x01;
		testFrame.data[3] = 0xFF;
		testFrame.data[4] = 0xFF;
		testFrame.data[5] = 0x8D;
		testFrame.data[6] = 0xFC;
		testFrame.data[7] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 1
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x01, testFrame.data[0]); // Sequence 1
		EXPECT_EQ('1', testFrame.data[1]); // ID Code index 0
		EXPECT_EQ('2', testFrame.data[2]); // ID Code index 1
		EXPECT_EQ('3', testFrame.data[3]); // ID Code index 2
		EXPECT_EQ('4', testFrame.data[4]); // ID Code index 3
		EXPECT_EQ('5', testFrame.data[5]); // ID Code index 4
		EXPECT_EQ('6', testFrame.data[6]); // ID Code index 5
		EXPECT_EQ('7', testFrame.data[7]); // ID Code index 6

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 2
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x02, testFrame.data[0]); // Sequence 2
		EXPECT_EQ('8', testFrame.data[1]); // ID Code index 7
		EXPECT_EQ('9', testFrame.data[2]); // ID Code index 8
		EXPECT_EQ('0', testFrame.data[3]); // ID Code index 9
		EXPECT_EQ('A', testFrame.data[4]); // ID Code index 10
		EXPECT_EQ('B', testFrame.data[5]); // ID Code index 11
		EXPECT_EQ('C', testFrame.data[6]); // ID Code index 12
		EXPECT_EQ('*', testFrame.data[7]); // Delimiter

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 3
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x03, testFrame.data[0]); // Sequence 3
		EXPECT_EQ('O', testFrame.data[1]); // Brand index 0
		EXPECT_EQ('p', testFrame.data[2]); // Brand index 1
		EXPECT_EQ('e', testFrame.data[3]); // Brand index 2
		EXPECT_EQ('n', testFrame.data[4]); // Brand index 3
		EXPECT_EQ('-', testFrame.data[5]); // Brand index 4
		EXPECT_EQ('A', testFrame.data[6]); // Brand index 5
		EXPECT_EQ('g', testFrame.data[7]); // Brand index 6

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 4
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x04, testFrame.data[0]); // Sequence 4
		EXPECT_EQ('r', testFrame.data[1]); // Brand index 7
		EXPECT_EQ('i', testFrame.data[2]); // Brand index 8
		EXPECT_EQ('c', testFrame.data[3]); // Brand index 9
		EXPECT_EQ('u', testFrame.data[4]); // Brand index 10
		EXPECT_EQ('l', testFrame.data[5]); // Brand index 11
		EXPECT_EQ('t', testFrame.data[6]); // Brand index 12
		EXPECT_EQ('u', testFrame.data[7]); // Brand index 13

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 5
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x05, testFrame.data[0]); // Sequence 5
		EXPECT_EQ('r', testFrame.data[1]); // Brand index 14
		EXPECT_EQ('e', testFrame.data[2]); // Brand index 15
		EXPECT_EQ('*', testFrame.data[3]); // Delimiter
		EXPECT_EQ('A', testFrame.data[4]); // Model index 0
		EXPECT_EQ('g', testFrame.data[5]); // Model index 1
		EXPECT_EQ('I', testFrame.data[6]); // Model index 2
		EXPECT_EQ('s', testFrame.data[7]); // Model index 3

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 6
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x06, testFrame.data[0]); // Sequence 6
		EXPECT_EQ('o', testFrame.data[1]); // Model index 4
		EXPECT_EQ('S', testFrame.data[2]); // Model index 5
		EXPECT_EQ('t', testFrame.data[3]); // Model index 6
		EXPECT_EQ('a', testFrame.data[4]); // Model index 7
		EXPECT_EQ('c', testFrame.data[5]); // Model index 8
		EXPECT_EQ('k', testFrame.data[6]); // Model index 9
		EXPECT_EQ('+', testFrame.data[7]); // Model index 10

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 7
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x07, testFrame.data[0]); // Sequence 7
		EXPECT_EQ('+', testFrame.data[1]); // Model index 11
		EXPECT_EQ('*', testFrame.data[2]); // Delimiter
		EXPECT_EQ(0xFF, testFrame.data[3]); // Padding
		EXPECT_EQ(0xFF, testFrame.data[4]); // Padding
		EXPECT_EQ(0xFF, testFrame.data[5]); // Padding
		EXPECT_EQ(0xFF, testFrame.data[6]); // Padding
		EXPECT_EQ(0xFF, testFrame.data[7]); // Padding

		// Send EOM ACK
		testFrame.dataLength = 8;
		testFrame.identifier = test_helpers::create_ext_can_id(6, 0xEC00, TestInternalECU, TestPartneredECU);
		testFrame.data[0] = 0x13; // EOM Multiplexer
		testFrame.data[1] = expectedLength & 0xFF;
		testFrame.data[2] = (expectedLength >> 8) & 0xFF;
		testFrame.data[3] = 0x07; // Number of frames
		testFrame.data[4] = 0xFF;
		testFrame.data[5] = 0x8D;
		testFrame.data[6] = 0xFC;
		testFrame.data[7] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();
	}

	// Make a few test DTCs
	isobus::DiagnosticProtocol::DiagnosticTroubleCode testDTC1(1234, isobus::DiagnosticProtocol::FailureModeIdentifier::ConditionExists, isobus::DiagnosticProtocol::LampStatus::None);
	isobus::DiagnosticProtocol::DiagnosticTroubleCode testDTC2(567, isobus::DiagnosticProtocol::FailureModeIdentifier::DataErratic, isobus::DiagnosticProtocol::LampStatus::AmberWarningLampSlowFlash);
	isobus::DiagnosticProtocol::DiagnosticTroubleCode testDTC3(8910, isobus::DiagnosticProtocol::FailureModeIdentifier::BadIntelligentDevice, isobus::DiagnosticProtocol::LampStatus::RedStopLampSolid);

	{
		// Test DM1
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC1, true);

		// Use a PGN request to trigger sending it immediately
		testFrame.dataLength = 3;
		testFrame.identifier = 0x18EAAAAB;
		testFrame.data[0] = 0xCA;
		testFrame.data[1] = 0xFE;
		testFrame.data[2] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();

		protocolUnderTest.update();

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// A single DTC is 1 frame
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x18FECAAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0xFF, testFrame.data[0]); // Lamp (unused in ISO11783 mode)
		EXPECT_EQ(0xFF, testFrame.data[1]); // Lamp (unused in ISO11783 mode)
		EXPECT_EQ(0xD2, testFrame.data[2]); // SPN LSB
		EXPECT_EQ(0x04, testFrame.data[3]); // SPN
		EXPECT_EQ(31, testFrame.data[4]); // SPN + FMI
		EXPECT_EQ(1, testFrame.data[5]); // Occurrence Count  + Conversion Method
		EXPECT_EQ(0xFF, testFrame.data[6]); // Padding
		EXPECT_EQ(0xFF, testFrame.data[7]); // Padding

		protocolUnderTest.set_j1939_mode(true);
		EXPECT_TRUE(protocolUnderTest.get_j1939_mode());

		// Validate in J1939 mode
		testFrame.dataLength = 3;
		testFrame.identifier = 0x18EAAAAB;
		testFrame.data[0] = 0xCA;
		testFrame.data[1] = 0xFE;
		testFrame.data[2] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x18FECAAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x00, testFrame.data[0]); // Lamp
		EXPECT_EQ(0xFF, testFrame.data[1]); // Flash (do not flash / solid)
		EXPECT_EQ(0xD2, testFrame.data[2]); // SPN LSB
		EXPECT_EQ(0x04, testFrame.data[3]); // SPN
		EXPECT_EQ(31, testFrame.data[4]); // SPN + FMI
		EXPECT_EQ(1, testFrame.data[5]); // Occurrence Count  + Conversion Method
		EXPECT_EQ(0xFF, testFrame.data[6]); // Padding
		EXPECT_EQ(0xFF, testFrame.data[7]); // Padding

		protocolUnderTest.set_j1939_mode(false);
		EXPECT_FALSE(protocolUnderTest.get_j1939_mode());

		// Test a DM1 with multiple DTCs in it
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC2, true);
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC3, true);
		testFrame.dataLength = 3;
		testFrame.identifier = 0x18EAAAAB;
		testFrame.data[0] = 0xCA;
		testFrame.data[1] = 0xFE;
		testFrame.data[2] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		std::uint16_t expectedBAMLength = 14; // This is 2 + 4 * number of DTCs

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// Broadcast Announce Message
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CECFFAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x20, testFrame.data[0]); // BAM Multiplexer
		EXPECT_EQ(expectedBAMLength & 0xFF, testFrame.data[1]); // Length LSB
		EXPECT_EQ((expectedBAMLength >> 8) & 0xFF, testFrame.data[2]); // Length MSB
		EXPECT_EQ(0x02, testFrame.data[3]); // Number of frames in session (based on length)
		EXPECT_EQ(0xFF, testFrame.data[4]); // Always 0xFF
		EXPECT_EQ(0xCA, testFrame.data[5]); // PGN LSB
		EXPECT_EQ(0xFE, testFrame.data[6]); // PGN
		EXPECT_EQ(0x00, testFrame.data[7]); // PGN MSB

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 1
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBFFAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x01, testFrame.data[0]); // Sequence 1
		EXPECT_EQ(0xFF, testFrame.data[1]); // Lamp / reserved
		EXPECT_EQ(0xFF, testFrame.data[2]); // Flash / reserved
		EXPECT_EQ(0xD2, testFrame.data[3]); // SPN 1
		EXPECT_EQ(0x04, testFrame.data[4]); // SPN 1
		EXPECT_EQ(31, testFrame.data[5]); // FMI 1
		EXPECT_EQ(1, testFrame.data[6]); // Count 1
		EXPECT_EQ(0x37, testFrame.data[7]); // SPN2

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 2
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBFFAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x02, testFrame.data[0]); // Sequence 2
		EXPECT_EQ(0x02, testFrame.data[1]); // SPN 2
		EXPECT_EQ(2, testFrame.data[2]); // FMI 2
		EXPECT_EQ(01, testFrame.data[3]); // Count 2
		EXPECT_EQ(0xCE, testFrame.data[4]); // SPN 3
		EXPECT_EQ(0x22, testFrame.data[5]); // SPN 3
		EXPECT_EQ(12, testFrame.data[6]); // FMI 3
		EXPECT_EQ(1, testFrame.data[7]); // Count 3
	}

	{
		// Test DM2
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC1, false);
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC2, false);
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC3, false);

		testFrame.dataLength = 3;
		testFrame.identifier = 0x18EAAAAB;
		testFrame.data[0] = 0xCB;
		testFrame.data[1] = 0xFE;
		testFrame.data[2] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		EXPECT_TRUE(testPlugin.read_frame(testFrame));
		std::uint16_t expectedBAMLength = 14; // This is 2 + 4 * number of DTCs

		// Broadcast Announce Message
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CECFFAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x20, testFrame.data[0]); // BAM Multiplexer
		EXPECT_EQ(expectedBAMLength & 0xFF, testFrame.data[1]); // Length LSB
		EXPECT_EQ((expectedBAMLength >> 8) & 0xFF, testFrame.data[2]); // Length MSB
		EXPECT_EQ(0x02, testFrame.data[3]); // Number of frames in session (based on length)
		EXPECT_EQ(0xFF, testFrame.data[4]); // Always 0xFF
		EXPECT_EQ(0xCB, testFrame.data[5]); // PGN LSB
		EXPECT_EQ(0xFE, testFrame.data[6]); // PGN
		EXPECT_EQ(0x00, testFrame.data[7]); // PGN MSB

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 1
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBFFAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x01, testFrame.data[0]); // Sequence 1
		EXPECT_EQ(0xFF, testFrame.data[1]); // Lamp / reserved
		EXPECT_EQ(0xFF, testFrame.data[2]); // Flash / reserved
		EXPECT_EQ(0xD2, testFrame.data[3]); // SPN 1
		EXPECT_EQ(0x04, testFrame.data[4]); // SPN 1
		EXPECT_EQ(31, testFrame.data[5]); // FMI 1
		EXPECT_EQ(1, testFrame.data[6]); // Count 1
		EXPECT_EQ(0x37, testFrame.data[7]); // SPN2

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// BAM Payload Frame 2
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x1CEBFFAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0x02, testFrame.data[0]); // Sequence 2
		EXPECT_EQ(0x02, testFrame.data[1]); // SPN 2
		EXPECT_EQ(2, testFrame.data[2]); // FMI 2
		EXPECT_EQ(01, testFrame.data[3]); // Count 2
		EXPECT_EQ(0xCE, testFrame.data[4]); // SPN 3
		EXPECT_EQ(0x22, testFrame.data[5]); // SPN 3
		EXPECT_EQ(12, testFrame.data[6]); // FMI 3
		EXPECT_EQ(1, testFrame.data[7]); // Count 3

		// Clear the DTCs
		protocolUnderTest.clear_inactive_diagnostic_trouble_codes();

		testFrame.dataLength = 3;
		testFrame.identifier = 0x18EAAAAB;
		testFrame.data[0] = 0xCB;
		testFrame.data[1] = 0xFE;
		testFrame.data[2] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// Now zero DTCs
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x18FECBAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0xFF, testFrame.data[0]); // Lamp (unused in ISO11783 mode)
		EXPECT_EQ(0xFF, testFrame.data[1]); // Lamp (unused in ISO11783 mode)
		EXPECT_EQ(0x00, testFrame.data[2]); // SPN LSB
		EXPECT_EQ(0x00, testFrame.data[3]); // SPN
		EXPECT_EQ(0x00, testFrame.data[4]); // SPN + FMI
		EXPECT_EQ(0x00, testFrame.data[5]); // Occurrence Count  + Conversion Method
		EXPECT_EQ(0xFF, testFrame.data[6]); // Padding
		EXPECT_EQ(0xFF, testFrame.data[7]); // Padding

		// Try in J1939 Mode, make sure lamps are not reserved values
		protocolUnderTest.set_j1939_mode(true);
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC1, true);
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC1, false);

		testFrame.dataLength = 3;
		testFrame.identifier = 0x18EAAAAB;
		testFrame.data[0] = 0xCB;
		testFrame.data[1] = 0xFE;
		testFrame.data[2] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x18FECBAA, testFrame.identifier); // BAM from address AA
		EXPECT_NE(0xFF, testFrame.data[0]); // Lamp
		EXPECT_EQ(0xFF, testFrame.data[1]); // Solid Lamp
		EXPECT_EQ(0xD2, testFrame.data[2]); // SPN LSB
		EXPECT_EQ(0x04, testFrame.data[3]); // SPN
		EXPECT_EQ(31, testFrame.data[4]); // SPN + FMI
		EXPECT_EQ(0x01, testFrame.data[5]); // Occurrence Count  + Conversion Method
		EXPECT_EQ(0xFF, testFrame.data[6]); // Padding
		EXPECT_EQ(0xFF, testFrame.data[7]); // Padding

		protocolUnderTest.set_j1939_mode(false);
		protocolUnderTest.clear_inactive_diagnostic_trouble_codes();
	}

	{
		// Test DM13 against J1939-73
		EXPECT_TRUE(protocolUnderTest.get_broadcast_state());
		EXPECT_TRUE(protocolUnderTest.suspend_broadcasts(5));

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// When we are announcing a suspension, we're supposed to set
		// all values to NA except for the time, which we set to 5 in this case
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x18DFFFAA, testFrame.identifier); // DM13 from address AA
		EXPECT_EQ(0xFF, testFrame.data[0]);
		EXPECT_EQ(0xFF, testFrame.data[1]);
		EXPECT_EQ(0xFF, testFrame.data[2]);
		EXPECT_EQ(0xFF, testFrame.data[3]);
		EXPECT_EQ(0x05, testFrame.data[4]);
		EXPECT_EQ(0x00, testFrame.data[5]);
		EXPECT_EQ(0xFF, testFrame.data[6]);
		EXPECT_EQ(0xFF, testFrame.data[7]);

		EXPECT_FALSE(protocolUnderTest.get_broadcast_state());

		// Wait suspension to be lifted
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		protocolUnderTest.update();
		EXPECT_TRUE(protocolUnderTest.get_broadcast_state());

		// Test a suspension by another ECU. Set only our network.
		testFrame.dataLength = 8;
		testFrame.identifier = 0x18DFFFAB;
		testFrame.data[0] = 0xFC;
		testFrame.data[1] = 0xFF;
		testFrame.data[2] = 0xFF;
		testFrame.data[3] = 0x03;
		testFrame.data[4] = 0x0A;
		testFrame.data[5] = 0x00;
		testFrame.data[6] = 0xFF;
		testFrame.data[7] = 0xFF;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();
		EXPECT_FALSE(protocolUnderTest.get_broadcast_state());

		// Restart broadcasts
		testFrame.dataLength = 8;
		testFrame.data[0] = 0xFD;
		testFrame.data[1] = 0xFF;
		testFrame.data[2] = 0xFF;
		testFrame.data[3] = 0x00;
		testFrame.data[4] = 0xFF;
		testFrame.data[5] = 0xFF;
		testFrame.data[6] = 0xFF;
		testFrame.data[7] = 0xFF;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();
		EXPECT_TRUE(protocolUnderTest.get_broadcast_state());

		// Test suspending the current data link
		testFrame.dataLength = 8;
		testFrame.data[0] = 0x3F;
		testFrame.data[1] = 0xFF;
		testFrame.data[2] = 0xFF;
		testFrame.data[3] = 0x00;
		testFrame.data[4] = 0x0A;
		testFrame.data[5] = 0x00;
		testFrame.data[6] = 0xFF;
		testFrame.data[7] = 0xFF;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();
		EXPECT_FALSE(protocolUnderTest.get_broadcast_state());

		// Restart broadcasts
		testFrame.dataLength = 8;
		testFrame.data[0] = 0x7F;
		testFrame.data[1] = 0xFF;
		testFrame.data[2] = 0xFF;
		testFrame.data[3] = 0x00;
		testFrame.data[4] = 0xFF;
		testFrame.data[5] = 0xFF;
		testFrame.data[6] = 0xFF;
		testFrame.data[7] = 0xFF;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();
		EXPECT_TRUE(protocolUnderTest.get_broadcast_state());
	}

	{
		// Test DM22
		protocolUnderTest.suspend_broadcasts(2); // Since DM1 could be sent during the test, suspend broadcasts for now
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC1, true);
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC2, true);
		protocolUnderTest.update();
		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		testFrame.dataLength = 8;
		testFrame.identifier = 0x18C3AAAB;
		testFrame.data[0] = 17; // Request to clear/reset a specific active DTC 5.7.22.1
		testFrame.data[1] = 0xFF; // Control Byte Specific Indicator for Individual DTC Clear (N/A)
		testFrame.data[2] = 0xFF; // Reserved
		testFrame.data[3] = 0xFF; // Reserved
		testFrame.data[4] = 0xFF; // Reserved
		testFrame.data[5] = 0xD2; // SPN
		testFrame.data[6] = 0x04; // SPN
		testFrame.data[7] = 31; // FMI (5 bits)
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		// Check for a positive acknowledge that the DTC was cleared
		EXPECT_TRUE(testPlugin.read_frame(testFrame));
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x18C3ABAA, testFrame.identifier);
		EXPECT_EQ(18, testFrame.data[0]); // Positive acknowledge of clear/reset of a specific active DTC
		EXPECT_EQ(0xFF, testFrame.data[1]); // NA
		EXPECT_EQ(0xFF, testFrame.data[2]); // Reserved
		EXPECT_EQ(0xFF, testFrame.data[3]); // Reserved
		EXPECT_EQ(0xFF, testFrame.data[4]); // Reserved
		EXPECT_EQ(0xD2, testFrame.data[5]); // SPN
		EXPECT_EQ(0x04, testFrame.data[6]); // SPN
		EXPECT_EQ(31, testFrame.data[7]); // 5 bits of FMI

		// Try and clear a non-existant active DTC (re-clear the one we just cleared)
		testFrame.dataLength = 8;
		testFrame.identifier = 0x18C3AAAB;
		testFrame.data[0] = 17; // Request to clear/reset a specific active DTC 5.7.22.1
		testFrame.data[1] = 0xFF; // Control Byte Specific Indicator for Individual DTC Clear (N/A)
		testFrame.data[2] = 0xFF; // Reserved
		testFrame.data[3] = 0xFF; // Reserved
		testFrame.data[4] = 0xFF; // Reserved
		testFrame.data[5] = 0xD2; // SPN
		testFrame.data[6] = 0x04; // SPN
		testFrame.data[7] = 31; // FMI (5 bits)
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		// Check for a negative acknowledge that the DTC was cleared
		EXPECT_TRUE(testPlugin.read_frame(testFrame));
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x18C3ABAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(19, testFrame.data[0]); // Negative acknowledge of clear/reset of a specific active DTC
		EXPECT_EQ(0x04, testFrame.data[1]); // Diagnostic trouble code no longer active
		EXPECT_EQ(0xFF, testFrame.data[2]); // Reserved
		EXPECT_EQ(0xFF, testFrame.data[3]); // Reserved
		EXPECT_EQ(0xFF, testFrame.data[4]); // Reserved
		EXPECT_EQ(0xD2, testFrame.data[5]); // SPN
		EXPECT_EQ(0x04, testFrame.data[6]); // SPN
		EXPECT_EQ(31, testFrame.data[7]); // 5 bits of FMI

		// Try to clear the DTC from the inactive list
		testFrame.dataLength = 8;
		testFrame.identifier = 0x18C3AAAB;
		testFrame.data[0] = 1; // Request to clear/reset a specific previously active DTC 5.7.22.1
		testFrame.data[1] = 0xFF; // Control Byte Specific Indicator for Individual DTC Clear (N/A)
		testFrame.data[2] = 0xFF; // Reserved
		testFrame.data[3] = 0xFF; // Reserved
		testFrame.data[4] = 0xFF; // Reserved
		testFrame.data[5] = 0xD2; // SPN
		testFrame.data[6] = 0x04; // SPN
		testFrame.data[7] = 31; // FMI (5 bits)
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		// Check for a positive acknowledge that the DTC was cleared
		EXPECT_TRUE(testPlugin.read_frame(testFrame));
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x18C3ABAA, testFrame.identifier);
		EXPECT_EQ(2, testFrame.data[0]); // Positive acknowledge of clear/reset of a specific active DTC
		EXPECT_EQ(0xFF, testFrame.data[1]); // NA
		EXPECT_EQ(0xFF, testFrame.data[2]); // Reserved
		EXPECT_EQ(0xFF, testFrame.data[3]); // Reserved
		EXPECT_EQ(0xFF, testFrame.data[4]); // Reserved
		EXPECT_EQ(0xD2, testFrame.data[5]); // SPN
		EXPECT_EQ(0x04, testFrame.data[6]); // SPN
		EXPECT_EQ(31, testFrame.data[7]); // 5 bits of FMI

		// Try to clear the DTC again from the inactive list (which is not valid)
		testFrame.dataLength = 8;
		testFrame.identifier = 0x18C3AAAB;
		testFrame.data[0] = 1; // Request to clear/reset a specific previously active DTC 5.7.22.1
		testFrame.data[1] = 0xFF; // Control Byte Specific Indicator for Individual DTC Clear (N/A)
		testFrame.data[2] = 0xFF; // Reserved
		testFrame.data[3] = 0xFF; // Reserved
		testFrame.data[4] = 0xFF; // Reserved
		testFrame.data[5] = 0xD2; // SPN
		testFrame.data[6] = 0x04; // SPN
		testFrame.data[7] = 31; // FMI (5 bits)
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		// Check for a negative acknowledge that the DTC was cleared
		EXPECT_TRUE(testPlugin.read_frame(testFrame));
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x18C3ABAA, testFrame.identifier);
		EXPECT_EQ(3, testFrame.data[0]); // Positive acknowledge of clear/reset of a specific active DTC
		EXPECT_EQ(0x02, testFrame.data[1]); // Since the DTC is not active, it is unknown to us.
		EXPECT_EQ(0xFF, testFrame.data[2]); // Reserved
		EXPECT_EQ(0xFF, testFrame.data[3]); // Reserved
		EXPECT_EQ(0xFF, testFrame.data[4]); // Reserved
		EXPECT_EQ(0xD2, testFrame.data[5]); // SPN
		EXPECT_EQ(0x04, testFrame.data[6]); // SPN
		EXPECT_EQ(31, testFrame.data[7]); // 5 bits of FMI

		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC1, true);

		// Try clearing an inactive DTC that is in the active list to check the error code
		testFrame.dataLength = 8;
		testFrame.identifier = 0x18C3AAAB;
		testFrame.data[0] = 1; // Request to clear/reset a specific previously active DTC 5.7.22.1
		testFrame.data[1] = 0xFF; // Control Byte Specific Indicator for Individual DTC Clear (N/A)
		testFrame.data[2] = 0xFF; // Reserved
		testFrame.data[3] = 0xFF; // Reserved
		testFrame.data[4] = 0xFF; // Reserved
		testFrame.data[5] = 0xD2; // SPN
		testFrame.data[6] = 0x04; // SPN
		testFrame.data[7] = 31; // FMI (5 bits)
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		EXPECT_TRUE(testPlugin.read_frame(testFrame));
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x18C3ABAA, testFrame.identifier);
		EXPECT_EQ(3, testFrame.data[0]); // Positive acknowledge of clear/reset of a specific active DTC
		EXPECT_EQ(0x03, testFrame.data[1]); // DTC is not inactive (because it's in the active list)
		EXPECT_EQ(0xFF, testFrame.data[2]); // Reserved
		EXPECT_EQ(0xFF, testFrame.data[3]); // Reserved
		EXPECT_EQ(0xFF, testFrame.data[4]); // Reserved
		EXPECT_EQ(0xD2, testFrame.data[5]); // SPN
		EXPECT_EQ(0x04, testFrame.data[6]); // SPN
		EXPECT_EQ(31, testFrame.data[7]); // 5 bits of FMI

		// Reset back to a known state
		protocolUnderTest.clear_active_diagnostic_trouble_codes();
		protocolUnderTest.clear_inactive_diagnostic_trouble_codes();
	}

	{
		// Test DM11
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC1, true);
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC2, true);
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC3, true);

		// This tests that when DM1 is requested after a DM11 request is received, no DTCs are active
		testFrame.dataLength = 3;
		testFrame.identifier = 0x18EAAAAB;
		testFrame.data[0] = 0xD3;
		testFrame.data[1] = 0xFE;
		testFrame.data[2] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		testFrame.dataLength = 3;
		testFrame.identifier = 0x18EAAAAB;
		testFrame.data[0] = 0xCA;
		testFrame.data[1] = 0xFE;
		testFrame.data[2] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();

		protocolUnderTest.update();

		// The stack will have sent an ACK since we sent it as destination specific
		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x18E8FFAA, testFrame.identifier);
		EXPECT_EQ(0x00, testFrame.data[0]); // Positive Ack
		EXPECT_EQ(0xFF, testFrame.data[1]);
		EXPECT_EQ(0xFF, testFrame.data[2]);
		EXPECT_EQ(0xFF, testFrame.data[3]);
		EXPECT_EQ(0xAB, testFrame.data[4]); // Address
		EXPECT_EQ(0xD3, testFrame.data[5]); // PGN
		EXPECT_EQ(0xFE, testFrame.data[6]); // PGN
		EXPECT_EQ(0x00, testFrame.data[7]); // PGN

		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// Parse DM1 response
		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x18FECAAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0xFF, testFrame.data[0]); // Lamp (unused in ISO11783 mode)
		EXPECT_EQ(0xFF, testFrame.data[1]); // Lamp (unused in ISO11783 mode)
		EXPECT_EQ(0x00, testFrame.data[2]); // SPN LSB
		EXPECT_EQ(0x00, testFrame.data[3]); // SPN
		EXPECT_EQ(0x00, testFrame.data[4]); // SPN + FMI
		EXPECT_EQ(0x00, testFrame.data[5]); // Occurrence Count  + Conversion Method
		EXPECT_EQ(0xFF, testFrame.data[6]); // Padding
		EXPECT_EQ(0xFF, testFrame.data[7]); // Padding

		// Reset back to a known state
		protocolUnderTest.clear_active_diagnostic_trouble_codes();
		protocolUnderTest.clear_inactive_diagnostic_trouble_codes();
	}

	{
		// Test DM3
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC1, true);
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC2, true);
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC3, true);
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC1, false);
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC2, false);
		protocolUnderTest.set_diagnostic_trouble_code_active(testDTC3, false);

		// Should have some DTCs in the inactive list now, as tested by a previous unit test

		// Send the DM3 request
		testFrame.dataLength = 3;
		testFrame.identifier = 0x18EAAAAB;
		testFrame.data[0] = 0xCC;
		testFrame.data[1] = 0xFE;
		testFrame.data[2] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		// The stack will have sent an ACK since we sent it as destination specific
		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// Screen out DM1
		if (((testFrame.identifier >> 8) & 0xFFFF) == 0xFECA)
		{
			EXPECT_TRUE(testPlugin.read_frame(testFrame));
		}

		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x18E8FFAA, testFrame.identifier);
		EXPECT_EQ(0x00, testFrame.data[0]); // Positive Ack
		EXPECT_EQ(0xFF, testFrame.data[1]);
		EXPECT_EQ(0xFF, testFrame.data[2]);
		EXPECT_EQ(0xFF, testFrame.data[3]);
		EXPECT_EQ(0xAB, testFrame.data[4]); // Address
		EXPECT_EQ(0xCC, testFrame.data[5]); // PGN
		EXPECT_EQ(0xFE, testFrame.data[6]); // PGN
		EXPECT_EQ(0x00, testFrame.data[7]); // PGN

		// Request DM2 to see if it has been cleared by our request for DM3
		testFrame.dataLength = 3;
		testFrame.identifier = 0x18EAAAAB;
		testFrame.data[0] = 0xCB;
		testFrame.data[1] = 0xFE;
		testFrame.data[2] = 0x00;
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		// Parse DM2 response
		EXPECT_TRUE(testPlugin.read_frame(testFrame));

		// Screen out DM1
		if (((testFrame.identifier >> 8) & 0xFFFF) == 0xFECA)
		{
			EXPECT_TRUE(testPlugin.read_frame(testFrame));
		}

		EXPECT_EQ(CAN_DATA_LENGTH, testFrame.dataLength);
		EXPECT_EQ(0x18FECBAA, testFrame.identifier); // BAM from address AA
		EXPECT_EQ(0xFF, testFrame.data[0]); // Lamp (unused in ISO11783 mode)
		EXPECT_EQ(0xFF, testFrame.data[1]); // Lamp (unused in ISO11783 mode)
		EXPECT_EQ(0x00, testFrame.data[2]); // SPN LSB
		EXPECT_EQ(0x00, testFrame.data[3]); // SPN
		EXPECT_EQ(0x00, testFrame.data[4]); // SPN + FMI
		EXPECT_EQ(0x00, testFrame.data[5]); // Occurrence Count  + Conversion Method
		EXPECT_EQ(0xFF, testFrame.data[6]); // Padding
		EXPECT_EQ(0xFF, testFrame.data[7]); // Padding

		// Reset back to a known state
		protocolUnderTest.clear_active_diagnostic_trouble_codes();
		protocolUnderTest.clear_inactive_diagnostic_trouble_codes();
	}

	{
		// Test DTC Getters  and setters
		EXPECT_TRUE(protocolUnderTest.set_diagnostic_trouble_code_active(testDTC1, true));
		EXPECT_TRUE(protocolUnderTest.set_diagnostic_trouble_code_active(testDTC2, true));
		EXPECT_TRUE(protocolUnderTest.set_diagnostic_trouble_code_active(testDTC3, true));

		EXPECT_TRUE(protocolUnderTest.get_diagnostic_trouble_code_active(testDTC1));
		EXPECT_TRUE(protocolUnderTest.get_diagnostic_trouble_code_active(testDTC2));
		EXPECT_TRUE(protocolUnderTest.get_diagnostic_trouble_code_active(testDTC3));

		EXPECT_TRUE(protocolUnderTest.set_diagnostic_trouble_code_active(testDTC2, false));

		EXPECT_TRUE(protocolUnderTest.get_diagnostic_trouble_code_active(testDTC1));
		EXPECT_FALSE(protocolUnderTest.get_diagnostic_trouble_code_active(testDTC2));
		EXPECT_TRUE(protocolUnderTest.get_diagnostic_trouble_code_active(testDTC3));

		EXPECT_FALSE(protocolUnderTest.set_diagnostic_trouble_code_active(testDTC1, true));

		EXPECT_TRUE(protocolUnderTest.get_diagnostic_trouble_code_active(testDTC1));
		EXPECT_FALSE(protocolUnderTest.get_diagnostic_trouble_code_active(testDTC2));
		EXPECT_TRUE(protocolUnderTest.get_diagnostic_trouble_code_active(testDTC3));

		EXPECT_EQ(1234, testDTC1.get_suspect_parameter_number());
		EXPECT_EQ(567, testDTC2.get_suspect_parameter_number());
		EXPECT_EQ(8910, testDTC3.get_suspect_parameter_number());

		EXPECT_EQ(isobus::DiagnosticProtocol::FailureModeIdentifier::ConditionExists, testDTC1.get_failure_mode_identifier());
		EXPECT_EQ(isobus::DiagnosticProtocol::FailureModeIdentifier::DataErratic, testDTC2.get_failure_mode_identifier());
		EXPECT_EQ(isobus::DiagnosticProtocol::FailureModeIdentifier::BadIntelligentDevice, testDTC3.get_failure_mode_identifier());

		// Reset back to a known state
		protocolUnderTest.clear_active_diagnostic_trouble_codes();
		protocolUnderTest.clear_inactive_diagnostic_trouble_codes();
	}

	{
		// Test address violation
		// Construct a random message from our address of 0xAA
		testFrame.identifier = 0x18EFFFAA;
		memset(testFrame.data, 0, sizeof(testFrame.data));
		CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
		CANNetworkManager::CANNetwork.update();
		protocolUnderTest.update();

		DiagnosticProtocol::DiagnosticTroubleCode addressViolationDTC(2000 + 0xAA, DiagnosticProtocol::FailureModeIdentifier::ConditionExists, DiagnosticProtocol::LampStatus::None);
		EXPECT_TRUE(protocolUnderTest.get_diagnostic_trouble_code_active(addressViolationDTC));

		// Reset back to a known state
		protocolUnderTest.clear_active_diagnostic_trouble_codes();
		protocolUnderTest.clear_inactive_diagnostic_trouble_codes();
	}
	protocolUnderTest.terminate();
	EXPECT_FALSE(protocolUnderTest.get_initialized());
	CANHardwareInterface::stop();

	CANNetworkManager::CANNetwork.deactivate_control_function(TestInternalECU);
}
