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

#include <sstream>
#include <thread>

namespace isobus
{
	PCANBasicWindowsPlugin::PCANBasicWindowsPlugin(WORD channel) :
	  handle(channel),
	  openResult(PCAN_ERROR_INITIALIZE)
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
		// The handle is the channel number, not something this object owns, so only the plugin that opened it may uninitialize it
		if (PCAN_ERROR_OK == openResult)
		{
			CAN_Uninitialize(handle);
			openResult = PCAN_ERROR_INITIALIZE;
		}
	}

	void PCANBasicWindowsPlugin::open()
	{
		lastError.clear();
		openResult = CAN_Initialize(handle, PCAN_BAUD_250K);

		if (PCAN_ERROR_OK != openResult)
		{
			char errorText[256] = {};
			CAN_GetErrorText(openResult, 0, errorText);

			std::ostringstream message;
			message << std::hex << std::uppercase;
			message << "Unable to open PCAN channel 0x" << handle << ", " << errorText << " (error 0x" << openResult << ")";

			if (PCAN_ERROR_ILLHW == openResult)
			{
				message << ". Is the PEAK adapter plugged in?";
			}
			else if (PCAN_ERROR_NODRIVER == openResult)
			{
				message << ". Is the PEAK driver installed?";
			}
			else if ((PCAN_ERROR_HWINUSE == openResult) || (PCAN_ERROR_NETINUSE == openResult))
			{
				message << ". Is another program, such as PCAN-View, using it?";
			}
			lastError = message.str();
			LOG_CRITICAL("[PCAN]: " + lastError);
		}
	}

	std::string PCANBasicWindowsPlugin::get_last_error() const
	{
		return lastError;
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
