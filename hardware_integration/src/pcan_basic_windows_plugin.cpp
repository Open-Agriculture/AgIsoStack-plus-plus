//================================================================================================
/// @file pcan_basic_windows_plugin.cpp
///
/// @brief An interface for using a PEAK PCAN device.
/// @attention Use of the PEAK driver is governed in part by their license, and requires you
/// to install their driver first, which in-turn requires you to agree to their terms and conditions.
/// @author Adrian Del Grosso
///
/// @copyright 2022 The Open-Agriculture Developers
//================================================================================================

#include "isobus/hardware_integration/pcan_basic_windows_plugin.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

#include <thread>

namespace isobus
{
	PCANBasicWindowsPlugin::PCANBasicWindowsPlugin(WORD channel) :
	  handle(channel),
	  openResult(PCAN_ERROR_OK)
	{
	}

	PCANBasicWindowsPlugin::~PCANBasicWindowsPlugin()
	{
	}

	std::string PCANBasicWindowsPlugin::get_name() const
	{
		return "PEAK CAN";
	}

	bool PCANBasicWindowsPlugin::get_is_valid() const
	{
		return (PCAN_ERROR_OK == openResult);
	}

	void PCANBasicWindowsPlugin::close()
	{
		CAN_Uninitialize(handle);
	}

	void PCANBasicWindowsPlugin::open()
	{
		openResult = CAN_Initialize(handle, PCAN_BAUD_250K);

#ifndef DISABLE_CAN_STACK_LOGGER
		if (PCAN_ERROR_OK != openResult)
		{
			char strMsg[256];
			CAN_GetErrorText(openResult, 0, strMsg);
			LOG_CRITICAL("[PCAN]: Error trying to connect to PCAN probe, error code: %d %s", static_cast<std::uint32_t>(openResult), strMsg);
		}
#endif
	}

	bool PCANBasicWindowsPlugin::read_frame(isobus::CANMessageFrame &canFrame)
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

	bool PCANBasicWindowsPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
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
