//================================================================================================
/// @file mac_can_pcan_plugin.cpp
///
/// @brief An interface for using a PEAK PCAN device through the MacCAN PCBUSB driver.
/// @attention Use of this is governed in part by the MacCAN EULA
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================

#include "isobus/hardware_integration/mac_can_pcan_plugin.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

#include <thread>

namespace isobus
{
	MacCANPCANPlugin::MacCANPCANPlugin(WORD channel) :
	  handle(channel),
	  openResult(PCAN_ERROR_OK)
	{
	}

	MacCANPCANPlugin::~MacCANPCANPlugin()
	{
	}

	std::string MacCANPCANPlugin::get_name() const
	{
		return "MacCAN";
	}

	bool MacCANPCANPlugin::get_is_valid() const
	{
		return (PCAN_ERROR_OK == openResult);
	}

	void MacCANPCANPlugin::close()
	{
		CAN_Uninitialize(handle);
	}

	void MacCANPCANPlugin::open()
	{
		openResult = CAN_Initialize(handle, PCAN_BAUD_250K);

		if (PCAN_ERROR_OK != openResult)
		{
			LOG_CRITICAL("[MacCAN]: Error trying to connect to PCAN probe");
		}
	}

	bool MacCANPCANPlugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		TPCANStatus result;
		TPCANMsg CANMsg;
		TPCANTimestamp CANTimeStamp;
		bool retVal = false;

		result = CAN_Read(handle, &CANMsg, &CANTimeStamp);

		if (PCAN_ERROR_OK == result)
		{
			canFrame.dataLength = CANMsg.LEN;
			memcpy(canFrame.data, CANMsg.DATA, CANMsg.LEN);
			canFrame.identifier = CANMsg.ID;
			canFrame.isExtendedFrame = (PCAN_MESSAGE_EXTENDED == CANMsg.MSGTYPE);
			canFrame.timestamp_us = (CANTimeStamp.millis * 1000) + CANTimeStamp.micros;
			retVal = true;
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		return retVal;
	}

	bool MacCANPCANPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
	{
		TPCANStatus result;
		TPCANMsg msgCanMessage;

		msgCanMessage.ID = canFrame.identifier;
		msgCanMessage.LEN = canFrame.dataLength;
		msgCanMessage.MSGTYPE = canFrame.isExtendedFrame ? PCAN_MESSAGE_EXTENDED : PCAN_MESSAGE_STANDARD;
		memcpy(msgCanMessage.DATA, canFrame.data, canFrame.dataLength);

		result = CAN_Write(handle, &msgCanMessage);

		return (PCAN_ERROR_OK == result);
	}
}
