#include <gtest/gtest.h>

#include "isobus/hardware_integration/virtual_can_plugin.hpp"

using namespace isobus;

TEST(VIRTUAL_CAN_PLUGIN_TESTS, ReceivesOwnMessages)
{
	VirtualCANPlugin testPlugin("", true);

	HardwareInterfaceCANFrame sentFrame;
	sentFrame.identifier = 0x18FFA227;
	sentFrame.isExtendedFrame = true;
	sentFrame.data[0] = 0x01;
	sentFrame.data[1] = 0x02;
	sentFrame.data[2] = 0x03;
	sentFrame.data[3] = 0x04;
	sentFrame.data[4] = 0x05;
	sentFrame.data[5] = 0x06;
	sentFrame.data[6] = 0x07;
	sentFrame.data[7] = 0x08;
	sentFrame.dataLength = 8;
	testPlugin.write_frame(sentFrame);

	HardwareInterfaceCANFrame receiveFrame;
	EXPECT_TRUE(testPlugin.read_frame(receiveFrame));
	EXPECT_EQ(receiveFrame.identifier, 0x18FFA227);
	EXPECT_EQ(receiveFrame.isExtendedFrame, true);
	EXPECT_EQ(receiveFrame.data[0], 0x01);
	EXPECT_EQ(receiveFrame.data[1], 0x02);
	EXPECT_EQ(receiveFrame.data[2], 0x03);
	EXPECT_EQ(receiveFrame.data[3], 0x04);
	EXPECT_EQ(receiveFrame.data[4], 0x05);
	EXPECT_EQ(receiveFrame.data[5], 0x06);
	EXPECT_EQ(receiveFrame.data[6], 0x07);
	EXPECT_EQ(receiveFrame.data[7], 0x08);
	EXPECT_EQ(receiveFrame.dataLength, 8);
}

TEST(VIRTUAL_CAN_PLUGIN_TESTS, OtherReceivesMessage)
{
	VirtualCANPlugin testPlugin;
	VirtualCANPlugin otherPlugin;

	HardwareInterfaceCANFrame sentFrame;
	sentFrame.identifier = 0x18FFA227;
	sentFrame.isExtendedFrame = true;
	sentFrame.data[0] = 0x01;
	sentFrame.data[1] = 0x02;
	sentFrame.data[2] = 0x03;
	sentFrame.data[3] = 0x04;
	sentFrame.data[4] = 0x05;
	sentFrame.data[5] = 0x06;
	sentFrame.data[6] = 0x07;
	sentFrame.data[7] = 0x08;
	sentFrame.dataLength = 8;
	testPlugin.write_frame(sentFrame);

	HardwareInterfaceCANFrame receiveFrame;
	EXPECT_TRUE(otherPlugin.read_frame(receiveFrame));
	EXPECT_EQ(receiveFrame.identifier, 0x18FFA227);
	EXPECT_EQ(receiveFrame.isExtendedFrame, true);
	EXPECT_EQ(receiveFrame.data[0], 0x01);
	EXPECT_EQ(receiveFrame.data[1], 0x02);
	EXPECT_EQ(receiveFrame.data[2], 0x03);
	EXPECT_EQ(receiveFrame.data[3], 0x04);
	EXPECT_EQ(receiveFrame.data[4], 0x05);
	EXPECT_EQ(receiveFrame.data[5], 0x06);
	EXPECT_EQ(receiveFrame.data[6], 0x07);
	EXPECT_EQ(receiveFrame.data[7], 0x08);
	EXPECT_EQ(receiveFrame.dataLength, 8);
}
