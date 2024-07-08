//
// Created by abyl on 14.02.24.
//

#include <gtest/gtest.h>
#include "isobus/hardware_integration/can_hardware_interface.hpp"
#include "isobus/hardware_integration/virtual_can_plugin.hpp"
#include "isobus/isobus/can_message.hpp"
#include "isobus/isobus/can_message_frame.hpp"
#include "isobus/isobus/can_network_manager.hpp"

using namespace isobus;

std::uint64_t value64;
std::uint16_t value16;

void callback(const CANMessage &message, void *)
{
	value16 = message.get_int16_at(0);
	EXPECT_EQ(value16, 513);
	value16 = message.get_int16_at(0, CANMessage::ByteFormat::BigEndian);
	EXPECT_EQ(value16, 258);
	value64 = message.get_int64_at(0);
	EXPECT_EQ(value64, 578437695752307201);
	value64 = message.get_int64_at(0, CANMessage::ByteFormat::BigEndian);
	EXPECT_EQ(value64, 72623859790382856);
	value64 = message.get_data_custom_length(8, 16);
	EXPECT_EQ(value64, 770);
	value64 = message.get_data_custom_length(8, 16, CANMessage::ByteFormat::BigEndian);
	EXPECT_EQ(value64, 515);
	value64 = message.get_data_custom_length(8, 15);
	EXPECT_EQ(value64, 258);
	value64 = message.get_data_custom_length(8, 15, CANMessage::ByteFormat::BigEndian);
	EXPECT_EQ(value64, 513);
	value64 = message.get_data_custom_length(14, 3);
	EXPECT_EQ(value64, 4);
	value64 = message.get_data_custom_length(14, 3, CANMessage::ByteFormat::BigEndian);
	EXPECT_EQ(value64, 4);
	value64 = message.get_data_custom_length(63, 999999);
	EXPECT_EQ(value64, 0);
	value64 = message.get_data_custom_length(65748321, 1);
	EXPECT_EQ(value64, 0);
}

TEST(CAN_MESSAGE_TESTS, DataCorrectnessTest)
{
	CANHardwareInterface::set_number_of_can_channels(1);
	CANHardwareInterface::assign_can_channel_frame_handler(0, std::make_shared<VirtualCANPlugin>());
	CANHardwareInterface::start();
	CANNetworkManager::CANNetwork.update();
	CANNetworkManager::CANNetwork.add_global_parameter_group_number_callback(0xE100, callback, nullptr);

	CANMessageFrame testFrame = {};
	// First we are sending with EE00 PGN so manager could add to its control function list
	testFrame.identifier = 0x18EEFFAA;
	testFrame.isExtendedFrame = true;
	testFrame.dataLength = 8;
	testFrame.data[0] = 0x01;
	testFrame.data[1] = 0x02;
	testFrame.data[2] = 0x03;
	testFrame.data[3] = 0x04;
	testFrame.data[4] = 0x05;
	testFrame.data[5] = 0x06;
	testFrame.data[6] = 0x07;
	testFrame.data[7] = 0x08;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	testFrame.identifier = 0x18E1FFAA;
	CANNetworkManager::CANNetwork.process_receive_can_message_frame(testFrame);
	CANNetworkManager::CANNetwork.update();
	CANNetworkManager::CANNetwork.remove_global_parameter_group_number_callback(0xE100, callback, nullptr);
	CANHardwareInterface::stop();
}
